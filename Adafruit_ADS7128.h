/*!
 * @file Adafruit_ADS7128.h
 *
 * Arduino library for TI ADS7128 8-Channel 12-Bit ADC with GPIOs and CRC
 *
 * Copyright 2026 Adafruit Industries (Limor 'ladyada' Fried with assistance
 * from Claude Code)
 *
 * MIT License
 */

#ifndef ADAFRUIT_ADS7128_H
#define ADAFRUIT_ADS7128_H

#include <Adafruit_I2CDevice.h>
#include <Arduino.h>

/** Default I2C address (ADDR pin to GND via ~10k ohm) */
#define ADS7128_DEFAULT_ADDR 0x10

/** Opcode for single register read */
#define ADS7128_OP_SINGLE_READ 0x10
/** Opcode for single register write */
#define ADS7128_OP_SINGLE_WRITE 0x08
/** Opcode for setting bits in a register */
#define ADS7128_OP_SET_BIT 0x18
/** Opcode for clearing bits in a register */
#define ADS7128_OP_CLEAR_BIT 0x20
/** Opcode for block read of consecutive registers */
#define ADS7128_OP_BLOCK_READ 0x30
/** Opcode for block write to consecutive registers */
#define ADS7128_OP_BLOCK_WRITE 0x28

/** System status register (reset, CRC errors, sequence status) */
#define ADS7128_REG_SYSTEM_STATUS 0x00
/** General configuration register (reset, cal, stats, CRC, RMS enables) */
#define ADS7128_REG_GENERAL_CFG 0x01
/** Data configuration register (channel ID append, data format) */
#define ADS7128_REG_DATA_CFG 0x02
/** Oversampling configuration register */
#define ADS7128_REG_OSR_CFG 0x03
/** Operating mode configuration register (manual vs autonomous) */
#define ADS7128_REG_OPMODE_CFG 0x04

/** Pin configuration register (analog vs digital per channel) */
#define ADS7128_REG_PIN_CFG 0x05
/** GPIO configuration register (input vs output direction) */
#define ADS7128_REG_GPIO_CFG 0x07
/** GPO drive configuration register (push-pull vs open-drain) */
#define ADS7128_REG_GPO_DRIVE_CFG 0x09
/** GPO output value register */
#define ADS7128_REG_GPO_VALUE 0x0B
/** GPI input value register */
#define ADS7128_REG_GPI_VALUE 0x0D

/** ZCD blanking configuration register (transient rejection) */
#define ADS7128_REG_ZCD_BLANKING_CFG 0x0F
/** Sequence configuration register (mode, start/stop) */
#define ADS7128_REG_SEQUENCE_CFG 0x10
/** Channel selection register for manual mode */
#define ADS7128_REG_CHANNEL_SEL 0x11
/** Auto-sequence channel selection bitmask register */
#define ADS7128_REG_AUTO_SEQ_CH_SEL 0x12

/** Alert channel selection register */
#define ADS7128_REG_ALERT_CH_SEL 0x14
/** Alert mapping register (event to alert pin routing) */
#define ADS7128_REG_ALERT_MAP 0x16
/** Alert pin configuration register (polarity, drive) */
#define ADS7128_REG_ALERT_PIN_CFG 0x17
/** Event flag register (combined high/low threshold events) */
#define ADS7128_REG_EVENT_FLAG 0x18
/** High threshold event flags register */
#define ADS7128_REG_EVENT_HIGH_FLAG 0x1A
/** Low threshold event flags register */
#define ADS7128_REG_EVENT_LOW_FLAG 0x1C
/** Event region register (inside/outside window tracking) */
#define ADS7128_REG_EVENT_RGN 0x1E

/** Hysteresis register base address for CH0 (add 4*ch for other channels) */
#define ADS7128_REG_HYSTERESIS_CH0 0x20
/** High threshold register base address for CH0 (add 4*ch for other channels)
 */
#define ADS7128_REG_HIGH_TH_CH0 0x21
/** Event count register base address for CH0 (add 4*ch for other channels) */
#define ADS7128_REG_EVENT_COUNT_CH0 0x22
/** Low threshold register base address for CH0 (add 4*ch for other channels) */
#define ADS7128_REG_LOW_TH_CH0 0x23

/** Statistics max value LSB register base address for CH0 */
#define ADS7128_REG_MAX_LSB_CH0 0x60
/** Statistics min value LSB register base address for CH0 */
#define ADS7128_REG_MIN_LSB_CH0 0x80
/** Recent conversion LSB register base address for CH0 */
#define ADS7128_REG_RECENT_LSB_CH0 0xA0

/** RMS configuration register (channel, samples, DC subtraction) */
#define ADS7128_REG_RMS_CFG 0xC0
/** RMS result LSB register */
#define ADS7128_REG_RMS_LSB 0xC1
/** RMS result MSB register */
#define ADS7128_REG_RMS_MSB 0xC2

/** GPO0 trigger event selection register base address */
#define ADS7128_REG_GPO0_TRIG_EVENT_SEL 0xC3

/** ZCD output mapping register for channels 0-3 */
#define ADS7128_REG_GPO_VALUE_ZCD_CFG_CH0_CH3 0xE3
/** ZCD output mapping register for channels 4-7 */
#define ADS7128_REG_GPO_VALUE_ZCD_CFG_CH4_CH7 0xE4
/** ZCD to GPO update enable register */
#define ADS7128_REG_GPO_ZCD_UPDATE_EN 0xE7
/** GPO trigger configuration register */
#define ADS7128_REG_GPO_TRIGGER_CFG 0xE9
/** GPO trigger output value register */
#define ADS7128_REG_GPO_VALUE_TRIG 0xEB

/** Brown-out reset detected flag bit */
#define ADS7128_BIT_BOR 0x01
/** CRC error on input data flag bit */
#define ADS7128_BIT_CRC_ERR_IN 0x02
/** CRC error on fuse data flag bit */
#define ADS7128_BIT_CRC_ERR_FUSE 0x04
/** Oversampling computation done flag bit */
#define ADS7128_BIT_OSR_DONE 0x08
/** RMS computation done flag bit */
#define ADS7128_BIT_RMS_DONE 0x10
/** I2C high-speed mode status bit */
#define ADS7128_BIT_I2C_SPEED 0x20
/** Sequence active status bit */
#define ADS7128_BIT_SEQ_STATUS 0x40

/** Software reset bit */
#define ADS7128_BIT_RST 0x01
/** Calibration trigger bit */
#define ADS7128_BIT_CAL 0x02
/** Channel statistics reset bit */
#define ADS7128_BIT_CH_RST 0x04
/** Conversion start trigger bit */
#define ADS7128_BIT_CNVST 0x08
/** Digital window comparator enable bit */
#define ADS7128_BIT_DWC_EN 0x10
/** Statistics module enable bit */
#define ADS7128_BIT_STATS_EN 0x20
/** CRC enable bit */
#define ADS7128_BIT_CRC_EN 0x40
/** RMS module enable bit */
#define ADS7128_BIT_RMS_EN 0x80

/** Sequence start bit */
#define ADS7128_BIT_SEQ_START 0x10
/** Sequence mode bit (0=manual, 1=auto) */
#define ADS7128_BIT_SEQ_MODE 0x01

/** Conversion mode bit (0=manual, 1=autonomous) */
#define ADS7128_BIT_CONV_MODE 0x20

/** Append channel ID to conversion data bit */
#define ADS7128_APPEND_CHID 0x10

/**
 * Pin mode configuration for ADS7128 channels
 */
typedef enum {
  ADS7128_ANALOG = 0,       ///< Analog input (default)
  ADS7128_INPUT,            ///< Digital input (GPIO input)
  ADS7128_OUTPUT,           ///< Digital output, push-pull
  ADS7128_OUTPUT_OPENDRAIN, ///< Digital output, open-drain
} ads7128_pin_mode_t;

/**
 * Oversampling ratio configuration
 */
typedef enum {
  ADS7128_OSR_NONE = 0, ///< No oversampling (12-bit)
  ADS7128_OSR_2 = 1,    ///< 2x oversampling (13-bit)
  ADS7128_OSR_4 = 2,    ///< 4x oversampling (14-bit)
  ADS7128_OSR_8 = 3,    ///< 8x oversampling (15-bit)
  ADS7128_OSR_16 = 4,   ///< 16x oversampling (16-bit)
  ADS7128_OSR_32 = 5,   ///< 32x oversampling (16-bit)
  ADS7128_OSR_64 = 6,   ///< 64x oversampling (16-bit)
  ADS7128_OSR_128 = 7,  ///< 128x oversampling (16-bit)
} ads7128_osr_t;

/**
 * @brief Class for interfacing with TI ADS7128 8-channel 12-bit ADC
 *
 * The ADS7128 uses an opcode-based I2C protocol (not standard register
 * access). Each channel can be independently configured as analog input,
 * digital input, or digital output.
 */
class Adafruit_ADS7128 {
 public:
  Adafruit_ADS7128();
  ~Adafruit_ADS7128();

  // -------------------------------------------------------------------------
  // Initialization
  // -------------------------------------------------------------------------

  bool begin(uint8_t addr = ADS7128_DEFAULT_ADDR, TwoWire* wire = &Wire);

  // -------------------------------------------------------------------------
  // GPIO Functions (Phase 1)
  // -------------------------------------------------------------------------

  bool pinMode(uint8_t channel, ads7128_pin_mode_t mode);
  bool digitalWrite(uint8_t channel, bool value);
  bool digitalRead(uint8_t channel);

  // -------------------------------------------------------------------------
  // CRC Functions
  // -------------------------------------------------------------------------

  bool enableCRC(bool enable);
  bool getCRCEnabled();
  bool getCRCError();
  bool clearCRCError();

  // -------------------------------------------------------------------------
  // ADC Functions - Manual Mode (Phase 2)
  // -------------------------------------------------------------------------

  uint16_t analogRead(uint8_t channel);
  float analogReadVoltage(uint8_t channel, float vref = 5.0);

  // -------------------------------------------------------------------------
  // ADC Functions - Auto-Sequence Mode (Phase 2)
  // -------------------------------------------------------------------------

  bool setSequenceChannels(uint8_t channelMask);
  bool startSequence();
  bool stopSequence();
  uint16_t readSequenceResult(uint8_t* channel = nullptr);

  // -------------------------------------------------------------------------
  // Oversampling Configuration (Phase 2)
  // -------------------------------------------------------------------------

  bool setOversampling(ads7128_osr_t osr);
  ads7128_osr_t getOversampling();

  // -------------------------------------------------------------------------
  // Statistics Functions (Phase 2)
  // -------------------------------------------------------------------------

  bool enableStatistics(bool enable);
  bool getStatisticsEnabled();
  uint16_t getMax(uint8_t channel);
  uint16_t getMin(uint8_t channel);
  uint16_t getRecent(uint8_t channel);
  bool resetStatistics();

  // -------------------------------------------------------------------------
  // Digital Window Comparator (Phase 2)
  // -------------------------------------------------------------------------

  bool enableDWC(bool enable);
  bool getDWCEnabled();
  bool setHighThreshold(uint8_t channel, uint16_t threshold);
  uint16_t getHighThreshold(uint8_t channel);
  bool setLowThreshold(uint8_t channel, uint16_t threshold);
  uint16_t getLowThreshold(uint8_t channel);
  bool setHysteresis(uint8_t channel, uint8_t hysteresis);
  uint8_t getHysteresis(uint8_t channel);
  bool setEventCount(uint8_t channel, uint8_t count);
  uint8_t getEventFlags();
  uint8_t getEventHighFlags();
  uint8_t getEventLowFlags();
  bool clearEventFlags();

  // -------------------------------------------------------------------------
  // ALERT Pin Configuration (Phase 2)
  // -------------------------------------------------------------------------

  bool configureAlert(bool pushPull, uint8_t logic);
  bool getAlertPushPull();
  uint8_t getAlertLogic();
  bool setAlertChannels(uint8_t channelMask);
  uint8_t getAlertChannels();

  // -------------------------------------------------------------------------
  // Sampling Rate (Phase 2)
  // -------------------------------------------------------------------------

  bool setSamplingRate(bool slowOsc, uint8_t divider);

  // -------------------------------------------------------------------------
  // Zero-Crossing Detection (ZCD)
  // -------------------------------------------------------------------------

  bool setZCDChannel(uint8_t channel);
  bool setZCDBlanking(uint8_t count, bool multiply = false);
  bool setZCDOutput(uint8_t gpoChannel, uint8_t mode);
  bool enableZCDOutput(uint8_t gpoChannel, bool enable);
  bool getZCDOutputEnabled(uint8_t gpoChannel);

  // -------------------------------------------------------------------------
  // RMS Functions
  // -------------------------------------------------------------------------

  bool enableRMS(bool enable);
  bool getRMSEnabled();
  bool setRMSChannel(uint8_t channel);
  bool setRMSSamples(uint8_t setting);
  bool setRMSDCSub(bool enable);
  uint16_t getRMS();
  bool isRMSDone();

 private:
  Adafruit_I2CDevice* _i2c;
  bool _crc_enabled;
  bool _crc_error;

  // Low-level register access (handle opcode protocol + CRC)
  uint8_t _readRegister(uint8_t reg);
  bool _writeRegister(uint8_t reg, uint8_t data);
  bool _setBits(uint8_t reg, uint8_t mask);
  bool _clearBits(uint8_t reg, uint8_t mask);
  bool _readBlock(uint8_t startReg, uint8_t* buf, uint8_t len);

  // Helper to read 12-bit value from LSB/MSB register pair
  uint16_t _read12BitValue(uint8_t lsbReg);

  // CRC helper - CRC-8-CCITT polynomial 0x07 over a single byte
  uint8_t _crc8(uint8_t byte);
};

#endif // ADAFRUIT_ADS7128_H
