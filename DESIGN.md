# Adafruit_ADS7128 Library Design Document

## 1. Chip Overview

### Part Information
- **Part Number:** ADS7128 (Texas Instruments)
- **Description:** 8-channel, 12-bit SAR ADC with GPIO, digital window comparator, CRC, RMS, and ZCD
- **Package:** WQFN-16, 3mm × 3mm
- **Supply Voltages:**
  - **AVDD:** 2.35–5.5V (analog supply)
  - **DVDD:** 1.65–5.5V (digital supply)
- **Temperature Range:** –40°C to +85°C

### Key Features
- **ADC:** 12-bit successive approximation register (SAR) ADC, up to 1 MSPS with internal oscillator
- **8 Channels:** Each independently configurable as:
  - Analog input (default)
  - Digital input
  - Digital output (push-pull or open-drain)
- **I2C Interface:** Up to 3.4 MHz (high-speed mode), 8 configurable addresses
- **Built-in Features:**
  - **CRC-8-CCITT** for data integrity (read/write operations)
  - **RMS Module:** 16-bit true RMS output
  - **Zero-Crossing Detection (ZCD):** Configurable threshold crossing detection
  - **Digital Window Comparator:** Programmable high/low thresholds per channel, hysteresis, event counter
  - **Statistics:** Per-channel min/max/recent value tracking
  - **Oversampling:** 2x to 128x averaging filter

### Applications
- Mobile robot CPU boards
- Refrigerators and freezers
- Digital multimeters (DMM)
- Rack servers
- I/O expansion systems

---

## 2. I2C Protocol — OPCODE-BASED (NOT Standard Register Access)

**⚠️ CRITICAL:** The ADS7128 uses an **opcode-based I2C protocol**, not standard register read/write. Every I2C transaction requires an opcode byte that specifies the operation type.

### 2.1 Opcode Table

| Opcode | Binary | Hex | Description |
|--------|--------|-----|-------------|
| 0x10 | 0001 0000 | 0x10 | Single register read |
| 0x08 | 0000 1000 | 0x08 | Single register write |
| 0x18 | 0001 1000 | 0x18 | Set bits (atomic OR) |
| 0x20 | 0010 0000 | 0x20 | Clear bits (atomic AND-NOT) |
| 0x30 | 0011 0000 | 0x30 | Block register read |
| 0x28 | 0010 1000 | 0x28 | Block register write |

### 2.2 I2C Frame Formats

#### Single Register Read
```
START → [slave_addr + W] → [0x10] → [reg_addr] → STOP/RESTART
START → [slave_addr + R] → [read data] → NACK → STOP
```

**Description:** 
1. Write frame with opcode 0x10 and register address
2. Restart and read frame to retrieve data
3. Same register data returned even if more clocks provided

#### Single Register Write
```
START → [slave_addr + W] → [0x08] → [reg_addr] → [data] → STOP
```

**Description:** Write opcode 0x08, register address, and data byte in one transaction.

#### Set Bits (Atomic)
```
START → [slave_addr + W] → [0x18] → [reg_addr] → [mask] → STOP
```

**Description:** 
- Bits with value **1** in mask are **SET**
- Bits with value **0** in mask are **unchanged**
- No read-modify-write cycle needed — atomic operation

#### Clear Bits (Atomic)
```
START → [slave_addr + W] → [0x20] → [reg_addr] → [mask] → STOP
```

**Description:** 
- Bits with value **1** in mask are **CLEARED**
- Bits with value **0** in mask are **unchanged**
- No read-modify-write cycle needed — atomic operation

#### Block Read
```
START → [slave_addr + W] → [0x30] → [first_reg_addr] → STOP/RESTART
START → [slave_addr + R] → [data0] → ACK → [data1] → ACK → ... → [dataN] → NACK → STOP
```

**Description:** 
- Reads consecutive registers starting from `first_reg_addr`
- Device auto-increments address for each byte
- Returns zeros for invalid register addresses

#### Block Write
```
START → [slave_addr + W] → [0x28] → [first_reg_addr] → [data0] → [data1] → ... → STOP
```

**Description:** 
- Writes consecutive registers starting from `first_reg_addr`
- Device auto-increments address for each byte
- Writes to invalid addresses have no effect

### 2.3 I2C Frame Acronyms

| Symbol | Description |
|--------|-------------|
| S | Start condition |
| Sr | Restart condition |
| P | Stop condition |
| A | ACK (low) |
| N | NACK (high) |
| R | Read bit (high) |
| W | Write bit (low) |

---

## 3. I2C Address Configuration

The ADS7128 I2C address is selected by a resistor divider on the ADDR pin.

### 3.1 Address Selection Table

| R1 (to DECAP/VDD) | R2 (to GND) | I2C Address |
|-------------------|-------------|-------------|
| 0Ω | DNP | 0x17 |
| 11kΩ | DNP | 0x16 |
| 33kΩ | DNP | 0x15 |
| 100kΩ | DNP | 0x14 |
| DNP | DNP | 0x10 |
| DNP | 11kΩ | **0x11** |
| DNP | 33kΩ | 0x12 |
| DNP | 100kΩ | 0x13 |

**Our Hardware Configuration:**
- ADDR tied to GND through 10kΩ ≈ 11kΩ
- **Expected address: 0x11**
- Verify with I2C scan before use

---

## 4. CRC Implementation

### 4.1 CRC Overview

- **Polynomial:** CRC-8-CCITT: x^8 + x^2 + x + 1 (0x07)
- **Bidirectional:** Appends CRC to read data, validates CRC on write data
- **Enable:** Set `CRC_EN` bit in `GENERAL_CFG` register (0x01)

### 4.2 CRC Behavior

When `CRC_EN = 1`:

**On READ operations:**
- Device appends 8-bit CRC byte after every byte read
- 2 bytes received per register read: [data] [CRC]

**On WRITE operations:**
- Device expects 8-bit CRC byte after every byte written
- Device validates incoming CRC
- On CRC error:
  - `CRC_ERR_IN` bit set in `SYSTEM_STATUS` register (0x00)
  - All register writes (except to 0x00 and 0x01) are **blocked**
  - Must clear error by writing `1b` to `CRC_ERR_IN`

### 4.3 CRC Computation

CRC-8-CCITT polynomial: 0x07

**Implementation plan:**
- Use lookup table for speed
- Generate table at compile time
- Process each byte sequentially

**Example C implementation:**
```cpp
uint8_t _crc8(uint8_t *data, uint8_t len) {
  uint8_t crc = 0x00;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x80)
        crc = (crc << 1) ^ 0x07;
      else
        crc <<= 1;
    }
  }
  return crc;
}
```

### 4.4 Design Decision

**Plan:** Enable CRC by default in `begin()` for data integrity. Implement CRC-8 lookup table for efficient computation.

---

## 5. BusIO Compatibility

### 5.1 Incompatibility with Standard BusIO

**Cannot use `Adafruit_I2CRegister` or `Adafruit_I2CRegisterBits`:**
- These classes assume standard I2C register protocol (write address, read/write data)
- ADS7128 requires opcode byte before every operation
- Standard BusIO classes don't support opcode-based protocols

### 5.2 Recommended Approach

**Use `Adafruit_I2CDevice` for Wire management, then implement custom register methods:**

```cpp
class Adafruit_ADS7128 {
private:
  Adafruit_I2CDevice *_i2c;
  bool _crc_enabled;
  
  // Custom register access methods
  uint8_t _readRegister(uint8_t reg);
  bool _writeRegister(uint8_t reg, uint8_t data);
  bool _setBits(uint8_t reg, uint8_t mask);
  bool _clearBits(uint8_t reg, uint8_t mask);
  bool _readBlock(uint8_t startReg, uint8_t *buf, uint8_t len);
  bool _writeBlock(uint8_t startReg, uint8_t *buf, uint8_t len);
  
  // CRC helper
  uint8_t _crc8(uint8_t *data, uint8_t len);
};
```

### 5.3 Implementation Notes

Each method must:
1. Construct proper opcode-based I2C frame
2. Handle CRC append/validate if `_crc_enabled == true`
3. Check for I2C errors and return status
4. Use `_i2c->write_then_read()` or `_i2c->write()` from `Adafruit_I2CDevice`

---

## 6. Complete Register Map

All addresses in hexadecimal. Reset values shown.

### 6.1 System and Configuration Registers (0x00–0x04)

| Address | Name | Reset | Description |
|---------|------|-------|-------------|
| 0x00 | SYSTEM_STATUS | 0x81 | BOR, CRC errors, seq status, OSR/RMS done |
| 0x01 | GENERAL_CFG | 0x00 | RMS_EN, CRC_EN, STATS_EN, DWC_EN, CNVST, CH_RST, CAL, RST |
| 0x02 | DATA_CFG | 0x00 | FIX_PAT, APPEND_STATUS |
| 0x03 | OSR_CFG | 0x00 | Oversampling ratio (1x–128x) |
| 0x04 | OPMODE_CFG | 0x00 | CONV_MODE, OSC_SEL, CLK_DIV |

### 6.2 GPIO Configuration Registers (0x05–0x0D)

| Address | Name | Reset | Description |
|---------|------|-------|-------------|
| 0x05 | PIN_CFG | 0x00 | Per-channel: 0=analog input, 1=GPIO |
| 0x07 | GPIO_CFG | 0x00 | Per-channel GPIO direction: 0=input, 1=output |
| 0x09 | GPO_DRIVE_CFG | 0x00 | Per-channel: 0=open-drain, 1=push-pull |
| 0x0B | GPO_VALUE | 0x00 | Digital output values (write) |
| 0x0D | GPI_VALUE | 0x00 | Digital input readback (read-only) |

### 6.3 Sequencer and Channel Selection (0x0F–0x12)

| Address | Name | Reset | Description |
|---------|------|-------|-------------|
| 0x0F | ZCD_BLANKING_CFG | 0x00 | Zero-crossing detect blanking config |
| 0x10 | SEQUENCE_CFG | 0x00 | SEQ_START, SEQ_MODE |
| 0x11 | CHANNEL_SEL | 0x00 | MANUAL_CHID, ZCD_CHID |
| 0x12 | AUTO_SEQ_CH_SEL | 0x00 | Channel enable for auto-sequence mode |

### 6.4 Alert and Event Registers (0x14–0x1E)

| Address | Name | Reset | Description |
|---------|------|-------|-------------|
| 0x14 | ALERT_CH_SEL | 0x00 | Per-channel alert enable |
| 0x16 | ALERT_MAP | 0x00 | RMS/CRC alert mapping to ALERT pin |
| 0x17 | ALERT_PIN_CFG | 0x00 | ALERT pin drive mode and logic |
| 0x18 | EVENT_FLAG | 0x00 | Per-channel event flags (read-only) |
| 0x1A | EVENT_HIGH_FLAG | 0x00 | Per-channel high threshold flags (W1C) |
| 0x1C | EVENT_LOW_FLAG | 0x00 | Per-channel low threshold flags (W1C) |
| 0x1E | EVENT_RGN | 0x00 | Per-channel: 0=out-of-band, 1=in-band |

### 6.5 Per-Channel Threshold Registers (0x20–0x3F)

Each channel has 4 consecutive registers:

**Channel 0 (0x20–0x23):**
| Address | Name | Reset | Description |
|---------|------|-------|-------------|
| 0x20 | HYSTERESIS_CH0 | 0xF0 | HIGH_TH_LSB[7:4], HYSTERESIS[3:0] |
| 0x21 | HIGH_TH_CH0 | 0xFF | High threshold MSB[7:0] |
| 0x22 | EVENT_COUNT_CH0 | 0x00 | LOW_TH_LSB[7:4], EVENT_COUNT[3:0] |
| 0x23 | LOW_TH_CH0 | 0x00 | Low threshold MSB[7:0] |

**Channels 1–7:** Same pattern at consecutive addresses (0x24–0x3F)

### 6.6 Statistics Registers (0x60–0xAF)

**MAX Registers (0x60–0x6F):** Per-channel max values (16 registers, LSB/MSB pairs)
- CH0: 0x60 (LSB), 0x61 (MSB)
- CH1: 0x62 (LSB), 0x63 (MSB)
- ...
- CH7: 0x6E (LSB), 0x6F (MSB)

**MIN Registers (0x80–0x8F):** Per-channel min values (16 registers, LSB/MSB pairs, reset 0xFF)
- CH0: 0x80 (LSB), 0x81 (MSB)
- CH1: 0x82 (LSB), 0x83 (MSB)
- ...
- CH7: 0x8E (LSB), 0x8F (MSB)

**RECENT Registers (0xA0–0xAF):** Per-channel last conversion (16 registers, LSB/MSB pairs)
- CH0: 0xA0 (LSB), 0xA1 (MSB)
- CH1: 0xA2 (LSB), 0xA3 (MSB)
- ...
- CH7: 0xAE (LSB), 0xAF (MSB)

### 6.7 RMS Registers (0xC0–0xC2)

| Address | Name | Reset | Description |
|---------|------|-------|-------------|
| 0xC0 | RMS_CFG | 0x00 | RMS channel selection and config |
| 0xC1 | RMS_LSB | 0x00 | 16-bit RMS result LSB |
| 0xC2 | RMS_MSB | 0x00 | 16-bit RMS result MSB |

### 6.8 GPO Trigger Event Registers (0xC3–0xD1)

| Address | Name | Reset | Description |
|---------|------|-------|-------------|
| 0xC3 | GPO0_TRIG_EVENT_SEL | 0x02 | GPO0 trigger event selection |
| 0xC5 | GPO1_TRIG_EVENT_SEL | 0x02 | GPO1 trigger event selection |
| 0xC7 | GPO2_TRIG_EVENT_SEL | 0x02 | GPO2 trigger event selection |
| 0xC9 | GPO3_TRIG_EVENT_SEL | 0x02 | GPO3 trigger event selection |
| 0xCB | GPO4_TRIG_EVENT_SEL | 0x02 | GPO4 trigger event selection |
| 0xCD | GPO5_TRIG_EVENT_SEL | 0x02 | GPO5 trigger event selection |
| 0xCF | GPO6_TRIG_EVENT_SEL | 0x02 | GPO6 trigger event selection |
| 0xD1 | GPO7_TRIG_EVENT_SEL | 0x02 | GPO7 trigger event selection |

### 6.9 ZCD and Trigger Registers (0xE3–0xEB)

| Address | Name | Reset | Description |
|---------|------|-------|-------------|
| 0xE3 | GPO_VALUE_ZCD_CFG_CH0_CH3 | 0x00 | ZCD GPO config for channels 0-3 |
| 0xE4 | GPO_VALUE_ZCD_CFG_CH4_CH7 | 0x00 | ZCD GPO config for channels 4-7 |
| 0xE7 | GPO_ZCD_UPDATE_EN | 0x00 | ZCD update enable |
| 0xE9 | GPO_TRIGGER_CFG | 0x00 | GPO trigger configuration |
| 0xEB | GPO_VALUE_TRIG | 0x00 | GPO trigger values |

---

## 7. Bit Field Details

### 7.1 SYSTEM_STATUS (0x00) [Reset: 0x81]

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 7 | RSVD | R | 1b | Reserved. Reads return 1b. |
| 6 | SEQ_STATUS | R | 0b | 0=Sequence stopped, 1=Sequence in progress |
| 5 | I2C_SPEED | R | 0b | 0=Standard/fast mode, 1=High-speed mode |
| 4 | RMS_DONE | R/W | 0b | RMS computation complete flag. Write 1b to clear. |
| 3 | OSR_DONE | R/W | 0b | Averaging complete flag. Write 1b to clear. |
| 2 | CRC_ERR_FUSE | R | 0b | Power-up configuration CRC error. 1=Error detected. |
| 1 | CRC_ERR_IN | R/W | 0b | Incoming data CRC error. Write 1b to clear. When set, register writes (except 0x00, 0x01) blocked. |
| 0 | BOR | R/W | 1b | Brown-out reset indicator. Write 1b to clear. Set on power-up or brown-out. |

### 7.2 GENERAL_CFG (0x01) [Reset: 0x00]

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 7 | RMS_EN | R/W | 0b | Enable RMS module. Writing 1b clears RMS result and starts new computation. |
| 6 | CRC_EN | R/W | 0b | Enable CRC on interface. 1=CRC appended to reads, validated on writes. |
| 5 | STATS_EN | R/W | 0b | Enable statistics module (min/max/recent). Writing 1b clears and restarts. |
| 4 | DWC_EN | R/W | 0b | Enable digital window comparator. |
| 3 | CNVST | W | 0b | Initiate conversion (write-only). 0=Normal, 1=Start conversion without SCL stretch. |
| 2 | CH_RST | R/W | 0b | Force all channels to analog inputs. 1=Override all GPIO settings. |
| 1 | CAL | R/W | 0b | ADC offset calibration. Write 1b to start; auto-clears when done. |
| 0 | RST | W | 0b | Software reset (write-only). Write 1b to reset; sets BOR flag on completion. |

### 7.3 DATA_CFG (0x02) [Reset: 0x00]

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 7 | FIX_PAT | R/W | 0b | Output fixed pattern 0xA5A for debugging. 1=Enable. |
| 6 | RESERVED | R | 0b | Reserved. |
| 5:4 | APPEND_STATUS[1:0] | R/W | 0b | 0=None, 1=4-bit channel ID, 10=4-bit status flags, 11=Reserved |
| 3:0 | RESERVED | R | 0b | Reserved. |

### 7.4 OSR_CFG (0x03) [Reset: 0x00]

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 7:3 | RESERVED | R | 0b | Reserved. |
| 2:0 | OSR[2:0] | R/W | 0b | Oversampling ratio: 0=None, 1=2x, 2=4x, 3=8x, 4=16x, 5=32x, 6=64x, 7=128x |

### 7.5 OPMODE_CFG (0x04) [Reset: 0x00]

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 7 | CONV_ON_ERR | R/W | 0b | 0=Continue on CRC error, 1=Pause sequencing and reset channels on CRC error |
| 6:5 | CONV_MODE[1:0] | R/W | 0b | 0=Manual mode, 1=Autonomous mode, 10/11=Reserved |
| 4 | OSC_SEL | R/W | 0b | 0=High-speed oscillator, 1=Low-power oscillator |
| 3:0 | CLK_DIV[3:0] | R/W | 0b | Sampling rate divider for autonomous mode (see Table 7.10) |

### 7.6 PIN_CFG (0x05) [Reset: 0x00]

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 7:0 | PIN_CFG[7:0] | R/W | 0b | Per-channel: 0=Analog input, 1=GPIO |

### 7.7 GPIO_CFG (0x07) [Reset: 0x00]

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 7:0 | GPIO_CFG[7:0] | R/W | 0b | Per-channel: 0=Digital input, 1=Digital output |

### 7.8 GPO_DRIVE_CFG (0x09) [Reset: 0x00]

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 7:0 | GPO_DRIVE_CFG[7:0] | R/W | 0b | Per-channel: 0=Open-drain output, 1=Push-pull output |

### 7.9 GPO_VALUE (0x0B) [Reset: 0x00]

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 7:0 | GPO_VALUE[7:0] | R/W | 0b | Per-channel output level: 0=Logic 0, 1=Logic 1 |

### 7.10 GPI_VALUE (0x0D) [Reset: 0x00]

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 7:0 | GPI_VALUE[7:0] | R | 0b | Per-channel input readback: 0=Logic 0, 1=Logic 1 (read-only) |

### 7.11 SEQUENCE_CFG (0x10) [Reset: 0x00]

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 7:5 | RESERVED | R | 0b | Reserved. |
| 4 | SEQ_START | R/W | 0b | Auto-sequence control: 0=Stop, 1=Start sequencing |
| 3:2 | RESERVED | R | 0b | Reserved. |
| 1:0 | SEQ_MODE[1:0] | R/W | 0b | 0=Manual mode, 1=Auto-sequence mode, 10/11=Reserved |

### 7.12 CHANNEL_SEL (0x11) [Reset: 0x00]

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 7:4 | ZCD_CHID[3:0] | R/W | 0b | Channel to use for zero-crossing detection |
| 3:0 | MANUAL_CHID[3:0] | R/W | 0b | Manual mode channel selection: 0–7=AIN0–AIN7, 8–15=Reserved |

### 7.13 AUTO_SEQ_CH_SEL (0x12) [Reset: 0x00]

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 7:0 | AUTO_SEQ_CH_SEL[7:0] | R/W | 0b | Per-channel enable for auto-sequence: 0=Disabled, 1=Enabled |

### 7.14 ALERT_CH_SEL (0x14) [Reset: 0x00]

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 7:0 | ALERT_CH_SEL[7:0] | R/W | 0b | Per-channel alert enable: 0=Disabled, 1=Alert flags assert ALERT pin |

### 7.15 ALERT_MAP (0x16) [Reset: 0x00]

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 7:2 | RESERVED | R | 0b | Reserved. |
| 1 | ALERT_RMS | R/W | 0b | Map RMS_DONE to ALERT pin: 0=Disabled, 1=Enabled |
| 0 | ALERT_CRCIN | R/W | 0b | Map CRC_ERR_IN to ALERT pin: 0=Disabled, 1=Enabled |

### 7.16 ALERT_PIN_CFG (0x17) [Reset: 0x00]

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 7:3 | RESERVED | R | 0b | Reserved. |
| 2 | ALERT_DRIVE | R/W | 0b | 0=Open-drain, 1=Push-pull |
| 1:0 | ALERT_LOGIC[1:0] | R/W | 0b | 0=Active low, 1=Active high, 10=Pulsed low, 11=Pulsed high |

### 7.17 EVENT_FLAG (0x18) [Reset: 0x00]

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 7:0 | EVENT_FLAG[7:0] | R | 0b | Per-channel event status (read-only). Clear by writing to EVENT_HIGH_FLAG or EVENT_LOW_FLAG. |

### 7.18 EVENT_HIGH_FLAG (0x1A) [Reset: 0x00]

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 7:0 | EVENT_HIGH_FLAG[7:0] | R/W | 0b | Per-channel high threshold flags. Write 1b to clear (W1C). |

### 7.19 EVENT_LOW_FLAG (0x1C) [Reset: 0x00]

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 7:0 | EVENT_LOW_FLAG[7:0] | R/W | 0b | Per-channel low threshold flags. Write 1b to clear (W1C). |

### 7.20 EVENT_RGN (0x1E) [Reset: 0x00]

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 7:0 | EVENT_RGN[7:0] | R/W | 0b | Per-channel region mode: 0=Out-of-band (result < low OR result > high), 1=In-band (low < result < high) |

### 7.21 Per-Channel Threshold Registers (0x20–0x3F)

Each channel has identical structure. Example for Channel 0:

**HYSTERESIS_CH0 (0x20) [Reset: 0xF0]**

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 7:4 | HIGH_THRESHOLD_CH0_LSB[3:0] | R/W | 1111b | Lower 4 bits of high threshold (compared with ADC[3:0]) |
| 3:0 | HYSTERESIS_CH0[3:0] | R/W | 0b | 4-bit hysteresis, left-shifted 3 bits: total = [4-bit, 000b] |

**HIGH_TH_CH0 (0x21) [Reset: 0xFF]**

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 7:0 | HIGH_THRESHOLD_CH0_MSB[7:0] | R/W | 11111111b | High threshold MSB (compared with ADC[11:4]) |

**EVENT_COUNT_CH0 (0x22) [Reset: 0x00]**

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 7:4 | LOW_THRESHOLD_CH0_LSB[3:0] | R/W | 0b | Lower 4 bits of low threshold (compared with ADC[3:0]) |
| 3:0 | EVENT_COUNT_CH0[3:0] | R/W | 0b | Consecutive samples count: trigger alert after (n+1) samples exceed threshold |

**LOW_TH_CH0 (0x23) [Reset: 0x00]**

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 7:0 | LOW_THRESHOLD_CH0_MSB[7:0] | R/W | 0b | Low threshold MSB (compared with ADC[11:4]) |

**Channels 1–7:** Same bit fields at addresses 0x24–0x3F (4 registers per channel, step 4)

---

## 8. Conversion Data Read

### 8.1 Manual Mode

1. Select channel in `MANUAL_CHID` (bits [3:0] of `CHANNEL_SEL` register)
2. Initiate conversion by reading data or writing `CNVST=1b`
3. Device stretches SCL during conversion (unless `CNVST=1b` used)
4. Read 12-bit result via I2C read transaction

### 8.2 Data Format

**Basic 12-bit ADC data:**
```
Frame A (2 bytes):
Byte 0: [D11 D10 D9 D8 D7 D6 D5 D4]
Byte 1: [D3 D2 D1 D0 0 0 0 0]
```

**With APPEND_STATUS = 01b (channel ID appended):**
```
Frame C (2 bytes):
Byte 0: [D11 D10 D9 D8 D7 D6 D5 D4]
Byte 1: [D3 D2 D1 D0 CHID3 CHID2 CHID1 CHID0]
```

**With APPEND_STATUS = 10b (status flags appended):**
```
Frame C (2 bytes):
Byte 0: [D11 D10 D9 D8 D7 D6 D5 D4]
Byte 1: [D3 D2 D1 D0 ALERT CRC_ERR RSVD RSVD]
```

**16-bit averaged result (OSR enabled):**
```
Frame B (2 bytes):
Byte 0: [D15 D14 D13 D12 D11 D10 D9 D8]
Byte 1: [D7 D6 D5 D4 D3 D2 D1 D0]
```

### 8.3 Auto-Sequence Mode

1. Set `SEQ_MODE = 01b` in `SEQUENCE_CFG`
2. Enable channels in `AUTO_SEQ_CH_SEL` (1 bit per channel)
3. Set `SEQ_START = 1b` to begin sequencing
4. Device sequences through enabled channels in ascending order
5. After each read, device advances to next channel
6. Read `RECENT_CHx_LSB/MSB` for buffered results

---

## 9. Sampling Rate Configuration

Controlled by `OSC_SEL` (bit 4) and `CLK_DIV[3:0]` (bits [3:0]) in `OPMODE_CFG` register (0x04).

### 9.1 Sampling Rate Table

| CLK_DIV[3:0] | OSC_SEL=0 (High-Speed) | OSC_SEL=1 (Low-Power) |
|--------------|------------------------|------------------------|
| 0000 | 1000 kSPS (1 µs) | 31.25 kSPS (32 µs) |
| 0001 | 666.7 kSPS (1.5 µs) | 20.83 kSPS (48 µs) |
| 0010 | 500 kSPS (2 µs) | 15.63 kSPS (64 µs) |
| 0011 | 333.3 kSPS (3 µs) | 10.42 kSPS (96 µs) |
| 0100 | 250 kSPS (4 µs) | 7.81 kSPS (128 µs) |
| 0101 | 166.7 kSPS (6 µs) | 5.21 kSPS (192 µs) |
| 0110 | 125 kSPS (8 µs) | 3.91 kSPS (256 µs) |
| 0111 | 83 kSPS (12 µs) | 2.60 kSPS (384 µs) |

**Note:** This applies to autonomous mode. In manual mode, host controls timing.

---

## 10. Phase 1 API Design (GPIO + CRC)

### 10.1 Enums

```cpp
typedef enum {
  ADS7128_ANALOG = 0,        // Analog input (default)
  ADS7128_INPUT,             // Digital input (GPIO input)
  ADS7128_OUTPUT,            // Digital output, push-pull
  ADS7128_OUTPUT_OPENDRAIN,  // Digital output, open-drain
} ads7128_pin_mode_t;
```

### 10.2 Public API

```cpp
class Adafruit_ADS7128 {
public:
  Adafruit_ADS7128();
  
  /**
   * @brief Initialize the ADS7128
   * @param addr I2C address (default 0x11)
   * @param wire Pointer to TwoWire instance
   * @return true on success, false on failure
   * 
   * Performs:
   * 1. I2C device initialization
   * 2. Software reset (write RST=1b to GENERAL_CFG)
   * 3. Wait for reset completion (BOR flag set)
   * 4. ADC calibration (write CAL=1b, wait for auto-clear)
   * 5. Clear BOR flag (write 1b to BOR)
   * 6. Enable CRC (write CRC_EN=1b to GENERAL_CFG)
   */
  bool begin(uint8_t addr = 0x11, TwoWire *wire = &Wire);
  
  /**
   * @brief Configure a channel as analog input, digital input, or digital output
   * @param channel Channel number (0–7)
   * @param mode Pin mode (see ads7128_pin_mode_t)
   * @return true on success, false on I2C error or invalid channel
   * 
   * Configures PIN_CFG, GPIO_CFG, and GPO_DRIVE_CFG registers accordingly.
   */
  bool pinMode(uint8_t channel, ads7128_pin_mode_t mode);
  
  /**
   * @brief Set digital output level
   * @param channel Channel number (0–7)
   * @param value true=HIGH, false=LOW
   * @return true on success, false if channel not configured as output or I2C error
   * 
   * Uses atomic set/clear bit operations for thread safety.
   */
  bool digitalWrite(uint8_t channel, bool value);
  
  /**
   * @brief Read digital input level
   * @param channel Channel number (0–7)
   * @return Input state (true=HIGH, false=LOW)
   * 
   * Reads GPI_VALUE register. Works for both inputs and outputs (readback).
   */
  bool digitalRead(uint8_t channel);
  
  /**
   * @brief Enable or disable CRC on I2C interface
   * @param enable true=enable CRC, false=disable
   * 
   * Sets CRC_EN bit in GENERAL_CFG register.
   */
  void enableCRC(bool enable);
  
  /**
   * @brief Check if CRC error detected
   * @return true if CRC_ERR_IN flag is set
   */
  bool getCRCError();
  
  /**
   * @brief Clear CRC error flag
   * @return true on success, false on I2C error
   * 
   * Writes 1b to CRC_ERR_IN bit to clear. Required to re-enable register writes.
   */
  bool clearCRCError();
  
private:
  Adafruit_I2CDevice *_i2c;
  bool _crc_enabled;
  
  // Low-level register access (handle opcode protocol + CRC)
  uint8_t _readRegister(uint8_t reg);
  bool _writeRegister(uint8_t reg, uint8_t data);
  bool _setBits(uint8_t reg, uint8_t mask);
  bool _clearBits(uint8_t reg, uint8_t mask);
  bool _readBlock(uint8_t startReg, uint8_t *buf, uint8_t len);
  bool _writeBlock(uint8_t startReg, uint8_t *buf, uint8_t len);
  
  // CRC helper
  uint8_t _crc8(uint8_t *data, uint8_t len);
};
```

### 10.3 Implementation Notes

**`begin()` sequence:**
1. Create `Adafruit_I2CDevice` with specified address and wire
2. Verify device communication (read `SYSTEM_STATUS`)
3. Software reset: write `RST=1b` to `GENERAL_CFG` (0x01)
4. Wait ~1ms for reset completion
5. Calibrate ADC: write `CAL=1b` to `GENERAL_CFG`, poll until auto-clears
6. Clear BOR flag: write `BOR=1b` to `SYSTEM_STATUS` (0x00)
7. Enable CRC: write `CRC_EN=1b` to `GENERAL_CFG`, set `_crc_enabled = true`
8. Return true on success

**`pinMode()` logic:**

| Mode | PIN_CFG bit | GPIO_CFG bit | GPO_DRIVE_CFG bit |
|------|-------------|--------------|-------------------|
| ADS7128_ANALOG | 0 | x | x |
| ADS7128_INPUT | 1 | 0 | x |
| ADS7128_OUTPUT | 1 | 1 | 1 |
| ADS7128_OUTPUT_OPENDRAIN | 1 | 1 | 0 |

Use `_setBits()` and `_clearBits()` for atomic operations.

**`digitalWrite()` logic:**
- Use `_setBits(GPO_VALUE, 1 << channel)` for HIGH
- Use `_clearBits(GPO_VALUE, 1 << channel)` for LOW
- Atomic operations avoid read-modify-write race conditions

**`digitalRead()` logic:**
- Read `GPI_VALUE` register (0x0D)
- Extract bit for requested channel
- Return boolean state

---

## 11. Phase 2 API Design (ADC — Placeholder)

Phase 2 will add ADC functionality. Below is the proposed API:

```cpp
// Manual mode - single channel
uint16_t analogRead(uint8_t channel);

// Auto-sequence mode
bool setSequenceChannels(uint8_t channelMask);
bool startSequence();
bool stopSequence();
uint16_t readSequenceResult(); // reads next in sequence

// Oversampling
typedef enum {
  ADS7128_OSR_NONE = 0,
  ADS7128_OSR_2X = 1,
  ADS7128_OSR_4X = 2,
  ADS7128_OSR_8X = 3,
  ADS7128_OSR_16X = 4,
  ADS7128_OSR_32X = 5,
  ADS7128_OSR_64X = 6,
  ADS7128_OSR_128X = 7,
} ads7128_osr_t;

bool setOversampling(ads7128_osr_t osr);
ads7128_osr_t getOversampling();

// Statistics
bool enableStatistics(bool enable);
uint16_t getMax(uint8_t channel);
uint16_t getMin(uint8_t channel);
uint16_t getRecent(uint8_t channel);

// Digital window comparator
bool setThresholds(uint8_t channel, uint16_t low, uint16_t high);
bool setHysteresis(uint8_t channel, uint8_t hysteresis);
bool setEventCount(uint8_t channel, uint8_t count);
bool enableAlert(uint8_t channel, bool enable);
bool getEventFlags(uint8_t *flags); // Read EVENT_FLAG register

// RMS module
bool enableRMS(bool enable);
bool setRMSChannel(uint8_t channel);
uint16_t getRMS();
bool isRMSDone();
```

**Note:** Phase 2 implementation deferred until Phase 1 (GPIO + CRC) is tested and validated.

---

## 12. Hardware Test Rig

### 12.1 Test Setup

- **MCU:** Metro Mini (ATmega328P, arduino:avr:uno)
- **Serial:** /dev/ttyUSB0, 115200 baud
- **Supply:** 5V AVDD and DVDD (USB-powered)
- **ADS7128 Address:** 0x11 (ADDR pin to GND via 10kΩ)

### 12.2 Pin Connections

| ADS7128 Pin | Metro Mini Pin | Purpose |
|-------------|----------------|---------|
| SDA | A4 (SDA) | I2C data |
| SCL | A5 (SCL) | I2C clock |
| ADDR | — | 10kΩ to GND → address 0x11 |
| ALERT | D3 | Alert interrupt (optional) |
| AVDD | 5V | Analog supply |
| DVDD | 5V | Digital supply |
| GND | GND | Ground |

### 12.3 Test Topology: Resistor Chain

Channels 0–7 connected with 10kΩ resistors in series:

```
A0 ——10K—— A1 ——10K—— A2 ——10K—— A3 ——10K—— A4 ——10K—— A5 ——10K—— A6 ——10K—— A7
```

This creates a resistor divider network:
- Setting a channel as **GPO HIGH** creates measurable voltages on adjacent analog channels
- Setting a channel as **GPO LOW** pulls down adjacent channels
- Enables testing GPIO writes by reading adjacent analog channels

### 12.4 Example Test Scenario

**Test: GPIO Output → ADC Input**

1. Configure CH0 as analog input (default)
2. Configure CH1 as digital output (push-pull)
3. Set CH1 HIGH → measure voltage on CH0 (expect ~2.5V via divider)
4. Set CH1 LOW → measure voltage on CH0 (expect ~0V)
5. Verify CRC error-free communication

**Test: Digital Input Readback**

1. Configure CH2 as digital input
2. Connect external 5V/GND via jumper wire
3. Read GPI_VALUE register
4. Verify bit 2 reflects input state

### 12.5 Verification Steps

Phase 1 (GPIO + CRC):
- [ ] I2C scan finds device at 0x11
- [ ] begin() succeeds (reset, calibrate, clear BOR, enable CRC)
- [ ] pinMode() configures channels correctly
- [ ] digitalWrite() sets outputs (verify via LED or multimeter)
- [ ] digitalRead() reads inputs correctly
- [ ] CRC validation works (no CRC_ERR_IN flags)
- [ ] Resistor chain test: GPIO HIGH/LOW measurable on adjacent analog channels

Phase 2 (ADC):
- [ ] analogRead() returns valid 12-bit data
- [ ] Auto-sequence mode cycles through channels
- [ ] Oversampling averages correctly
- [ ] Statistics (min/max/recent) track values
- [ ] Digital window comparator triggers events

---

## 13. Implementation Checklist

### Phase 1: GPIO + CRC
- [ ] Create `Adafruit_ADS7128.h` and `.cpp`
- [ ] Implement register access methods with opcode protocol
- [ ] Implement CRC-8 calculation (lookup table or polynomial)
- [ ] Implement `begin()` with reset + calibrate + CRC enable
- [ ] Implement `pinMode()`, `digitalWrite()`, `digitalRead()`
- [ ] Implement CRC control methods
- [ ] Write basic example: `gpio_blink.ino`
- [ ] Write hw_test: GPIO output verification via resistor chain
- [ ] Test on hardware, verify CRC operation
- [ ] Document results

### Phase 2: ADC (Future)
- [ ] Implement `analogRead()` for manual mode
- [ ] Implement auto-sequence mode methods
- [ ] Implement oversampling configuration
- [ ] Implement statistics readback
- [ ] Implement digital window comparator setup
- [ ] Implement RMS module control
- [ ] Write examples: `analog_read.ino`, `auto_sequence.ino`
- [ ] Write hw_test: ADC accuracy and sequencer
- [ ] Test on hardware
- [ ] Document results

---

## 14. References

- **Datasheet:** ADS7128 (Texas Instruments SBAS868A)
- **I2C Specification:** NXP UM10204
- **CRC-8-CCITT:** Polynomial 0x07 (x^8 + x^2 + x + 1)
- **BusIO Library:** Adafruit_BusIO (I2CDevice only)
- **Arduino I2C:** Wire.h (TwoWire class)

---

**End of Document**

This design document provides complete information to implement the ADS7128 Arduino library without re-reading the datasheet. All register addresses, bit fields, opcodes, CRC details, and hardware test topology are included.
