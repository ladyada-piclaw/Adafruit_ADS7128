/*!
 * @file Adafruit_ADS7128.cpp
 *
 * Arduino library for TI ADS7128 8-Channel 12-Bit ADC with GPIOs and CRC
 *
 * Copyright 2026 Adafruit Industries (Limor 'ladyada' Fried with assistance
 * from Claude Code)
 *
 * MIT License
 */

#include "Adafruit_ADS7128.h"

Adafruit_ADS7128::Adafruit_ADS7128() {
  _i2c = nullptr;
  _crc_enabled = false;
  _crc_error = false;
}

Adafruit_ADS7128::~Adafruit_ADS7128() {
  if (_i2c) {
    delete _i2c;
  }
}

bool Adafruit_ADS7128::begin(uint8_t addr, TwoWire *wire) {
  if (_i2c) {
    delete _i2c;
  }

  // General call reset to clear any lingering CRC state
  wire->beginTransmission(0x00);
  wire->write(0x06);
  wire->endTransmission();
  delay(10);

  _i2c = new Adafruit_I2CDevice(addr, wire);
  if (!_i2c->begin()) {
    return false;
  }

  _crc_enabled = false;

  // Software reset
  if (!_writeRegister(ADS7128_REG_GENERAL_CFG, ADS7128_BIT_RST)) {
    return false;
  }
  delay(5);

  // ADC calibration
  if (!_writeRegister(ADS7128_REG_GENERAL_CFG, ADS7128_BIT_CAL)) {
    return false;
  }

  uint16_t timeout = 1000;
  while (timeout--) {
    uint8_t cfg = _readRegister(ADS7128_REG_GENERAL_CFG);
    if ((cfg & ADS7128_BIT_CAL) == 0) {
      break;
    }
    delay(1);
  }
  if (timeout == 0) {
    return false;
  }

  // Clear BOR flag
  if (!_writeRegister(ADS7128_REG_SYSTEM_STATUS, ADS7128_BIT_BOR)) {
    return false;
  }

  return true;
}

// ---------------------------------------------------------------------------
// GPIO Functions
// ---------------------------------------------------------------------------

bool Adafruit_ADS7128::pinMode(uint8_t channel, ads7128_pin_mode_t mode) {
  if (channel > 7) {
    return false;
  }

  uint8_t mask = 1 << channel;

  switch (mode) {
  case ADS7128_ANALOG:
    if (!_clearBits(ADS7128_REG_PIN_CFG, mask)) {
      return false;
    }
    break;

  case ADS7128_INPUT:
    if (!_setBits(ADS7128_REG_PIN_CFG, mask)) {
      return false;
    }
    if (!_clearBits(ADS7128_REG_GPIO_CFG, mask)) {
      return false;
    }
    break;

  case ADS7128_OUTPUT:
    if (!_setBits(ADS7128_REG_PIN_CFG, mask)) {
      return false;
    }
    if (!_setBits(ADS7128_REG_GPIO_CFG, mask)) {
      return false;
    }
    if (!_setBits(ADS7128_REG_GPO_DRIVE_CFG, mask)) {
      return false;
    }
    break;

  case ADS7128_OUTPUT_OPENDRAIN:
    if (!_setBits(ADS7128_REG_PIN_CFG, mask)) {
      return false;
    }
    if (!_setBits(ADS7128_REG_GPIO_CFG, mask)) {
      return false;
    }
    if (!_clearBits(ADS7128_REG_GPO_DRIVE_CFG, mask)) {
      return false;
    }
    break;

  default:
    return false;
  }

  return true;
}

bool Adafruit_ADS7128::digitalWrite(uint8_t channel, bool value) {
  if (channel > 7) {
    return false;
  }

  uint8_t mask = 1 << channel;
  if (value) {
    return _setBits(ADS7128_REG_GPO_VALUE, mask);
  } else {
    return _clearBits(ADS7128_REG_GPO_VALUE, mask);
  }
}

bool Adafruit_ADS7128::digitalRead(uint8_t channel) {
  if (channel > 7) {
    return false;
  }

  uint8_t val = _readRegister(ADS7128_REG_GPI_VALUE);
  return (val & (1 << channel)) != 0;
}

// ---------------------------------------------------------------------------
// CRC Functions
// ---------------------------------------------------------------------------

bool Adafruit_ADS7128::enableCRC(bool enable) {
  bool result;
  if (enable) {
    result = _setBits(ADS7128_REG_GENERAL_CFG, ADS7128_BIT_CRC_EN);
  } else {
    result = _clearBits(ADS7128_REG_GENERAL_CFG, ADS7128_BIT_CRC_EN);
  }

  if (result) {
    _crc_enabled = enable;
  }
  return result;
}

bool Adafruit_ADS7128::getCRCError() {
  uint8_t status = _readRegister(ADS7128_REG_SYSTEM_STATUS);
  return (status & ADS7128_BIT_CRC_ERR_IN) != 0;
}

bool Adafruit_ADS7128::clearCRCError() {
  _crc_error = false;
  return _writeRegister(ADS7128_REG_SYSTEM_STATUS, ADS7128_BIT_CRC_ERR_IN);
}

// ---------------------------------------------------------------------------
// ADC Functions - Manual Mode
// ---------------------------------------------------------------------------

uint16_t Adafruit_ADS7128::analogRead(uint8_t channel) {
  if (channel > 7) {
    return 0xFFFF;
  }

  // Enable statistics (required for RECENT registers)
  if (!_setBits(ADS7128_REG_GENERAL_CFG, ADS7128_BIT_STATS_EN)) {
    return 0xFFFF;
  }

  // Select channel: write to CHANNEL_SEL[3:0], preserve ZCD_CHID[7:4]
  uint8_t chanSel = _readRegister(ADS7128_REG_CHANNEL_SEL);
  chanSel = (chanSel & 0xF0) | (channel & 0x0F);
  if (!_writeRegister(ADS7128_REG_CHANNEL_SEL, chanSel)) {
    return 0xFFFF;
  }

  // Trigger conversion: set CNVST bit
  if (!_setBits(ADS7128_REG_GENERAL_CFG, ADS7128_BIT_CNVST)) {
    return 0xFFFF;
  }

  // Wait for conversion (device stretches SCL, but small delay is safer)
  delayMicroseconds(10);

  // Read RECENT_CHn_LSB and MSB
  uint8_t lsbReg = ADS7128_REG_RECENT_LSB_CH0 + (channel * 2);
  return _read12BitValue(lsbReg);
}

float Adafruit_ADS7128::analogReadVoltage(uint8_t channel, float vref) {
  uint16_t raw = analogRead(channel);
  if (raw == 0xFFFF) {
    return -1.0;
  }
  return (raw / 4096.0) * vref;
}

// ---------------------------------------------------------------------------
// ADC Functions - Auto-Sequence Mode
// ---------------------------------------------------------------------------

bool Adafruit_ADS7128::setSequenceChannels(uint8_t channelMask) {
  return _writeRegister(ADS7128_REG_AUTO_SEQ_CH_SEL, channelMask);
}

bool Adafruit_ADS7128::startSequence() {
  // Enable statistics
  if (!_setBits(ADS7128_REG_GENERAL_CFG, ADS7128_BIT_STATS_EN)) {
    return false;
  }

  // Enable channel ID appending to data
  if (!_setBits(ADS7128_REG_DATA_CFG, ADS7128_APPEND_CHID)) {
    return false;
  }

  // Set CONV_MODE=1 (autonomous) in OPMODE_CFG
  if (!_setBits(ADS7128_REG_OPMODE_CFG, ADS7128_BIT_CONV_MODE)) {
    return false;
  }

  // Set SEQ_MODE=1 and SEQ_START=1 in SEQUENCE_CFG
  return _writeRegister(ADS7128_REG_SEQUENCE_CFG,
                        ADS7128_BIT_SEQ_MODE | ADS7128_BIT_SEQ_START);
}

bool Adafruit_ADS7128::stopSequence() {
  // Clear SEQ_START bit
  return _clearBits(ADS7128_REG_SEQUENCE_CFG, ADS7128_BIT_SEQ_START);
}

uint16_t Adafruit_ADS7128::readSequenceResult(uint8_t *channel) {
  // Read 2 bytes of conversion data from device
  uint8_t cmd[2] = {ADS7128_OP_SINGLE_READ, 0x00};
  uint8_t response[2];

  if (!_i2c->write_then_read(cmd, 2, response, 2)) {
    return 0xFFFF;
  }

  // Data format with APPEND_CHID: [D11-D4], [D3-D0, CHID3-CHID0]
  uint16_t raw = ((uint16_t)response[0] << 4) | ((response[1] >> 4) & 0x0F);

  if (channel != nullptr) {
    *channel = response[1] & 0x0F;
  }

  return raw;
}

// ---------------------------------------------------------------------------
// Oversampling Configuration
// ---------------------------------------------------------------------------

bool Adafruit_ADS7128::setOversampling(ads7128_osr_t osr) {
  return _writeRegister(ADS7128_REG_OSR_CFG, (uint8_t)osr & 0x07);
}

ads7128_osr_t Adafruit_ADS7128::getOversampling() {
  uint8_t val = _readRegister(ADS7128_REG_OSR_CFG);
  return (ads7128_osr_t)(val & 0x07);
}

// ---------------------------------------------------------------------------
// Statistics Functions
// ---------------------------------------------------------------------------

bool Adafruit_ADS7128::enableStatistics(bool enable) {
  if (enable) {
    return _setBits(ADS7128_REG_GENERAL_CFG, ADS7128_BIT_STATS_EN);
  } else {
    return _clearBits(ADS7128_REG_GENERAL_CFG, ADS7128_BIT_STATS_EN);
  }
}

uint16_t Adafruit_ADS7128::getMax(uint8_t channel) {
  if (channel > 7) {
    return 0xFFFF;
  }
  uint8_t lsbReg = ADS7128_REG_MAX_LSB_CH0 + (channel * 2);
  return _read12BitValue(lsbReg);
}

uint16_t Adafruit_ADS7128::getMin(uint8_t channel) {
  if (channel > 7) {
    return 0xFFFF;
  }
  uint8_t lsbReg = ADS7128_REG_MIN_LSB_CH0 + (channel * 2);
  return _read12BitValue(lsbReg);
}

uint16_t Adafruit_ADS7128::getRecent(uint8_t channel) {
  if (channel > 7) {
    return 0xFFFF;
  }
  uint8_t lsbReg = ADS7128_REG_RECENT_LSB_CH0 + (channel * 2);
  return _read12BitValue(lsbReg);
}

bool Adafruit_ADS7128::resetStatistics() {
  // Set CH_RST bit to reset statistics
  return _setBits(ADS7128_REG_GENERAL_CFG, ADS7128_BIT_CH_RST);
}

// ---------------------------------------------------------------------------
// Digital Window Comparator
// ---------------------------------------------------------------------------

bool Adafruit_ADS7128::enableDWC(bool enable) {
  if (enable) {
    return _setBits(ADS7128_REG_GENERAL_CFG, ADS7128_BIT_DWC_EN);
  } else {
    return _clearBits(ADS7128_REG_GENERAL_CFG, ADS7128_BIT_DWC_EN);
  }
}

bool Adafruit_ADS7128::setHighThreshold(uint8_t channel, uint16_t threshold) {
  if (channel > 7 || threshold > 0x0FFF) {
    return false;
  }

  uint8_t baseAddr = ADS7128_REG_HYSTERESIS_CH0 + (channel * 4);

  // HIGH_TH_MSB goes in register baseAddr+1
  uint8_t msb = (threshold >> 4) & 0xFF;
  if (!_writeRegister(baseAddr + 1, msb)) {
    return false;
  }

  // HIGH_TH_LSB[3:0] goes in upper nibble of HYSTERESIS register (baseAddr)
  uint8_t hystReg = _readRegister(baseAddr);
  hystReg = (hystReg & 0x0F) | ((threshold & 0x0F) << 4);
  return _writeRegister(baseAddr, hystReg);
}

bool Adafruit_ADS7128::setLowThreshold(uint8_t channel, uint16_t threshold) {
  if (channel > 7 || threshold > 0x0FFF) {
    return false;
  }

  uint8_t baseAddr = ADS7128_REG_HYSTERESIS_CH0 + (channel * 4);

  // LOW_TH_MSB goes in register baseAddr+3
  uint8_t msb = (threshold >> 4) & 0xFF;
  if (!_writeRegister(baseAddr + 3, msb)) {
    return false;
  }

  // LOW_TH_LSB[3:0] goes in upper nibble of EVENT_COUNT register (baseAddr+2)
  uint8_t evtReg = _readRegister(baseAddr + 2);
  evtReg = (evtReg & 0x0F) | ((threshold & 0x0F) << 4);
  return _writeRegister(baseAddr + 2, evtReg);
}

bool Adafruit_ADS7128::setHysteresis(uint8_t channel, uint8_t hysteresis) {
  if (channel > 7 || hysteresis > 15) {
    return false;
  }

  uint8_t baseAddr = ADS7128_REG_HYSTERESIS_CH0 + (channel * 4);
  uint8_t hystReg = _readRegister(baseAddr);
  hystReg = (hystReg & 0xF0) | (hysteresis & 0x0F);
  return _writeRegister(baseAddr, hystReg);
}

bool Adafruit_ADS7128::setEventCount(uint8_t channel, uint8_t count) {
  if (channel > 7 || count > 15) {
    return false;
  }

  uint8_t baseAddr = ADS7128_REG_HYSTERESIS_CH0 + (channel * 4);
  uint8_t evtReg = _readRegister(baseAddr + 2);
  evtReg = (evtReg & 0xF0) | (count & 0x0F);
  return _writeRegister(baseAddr + 2, evtReg);
}

uint8_t Adafruit_ADS7128::getEventFlags() {
  return _readRegister(ADS7128_REG_EVENT_FLAG);
}

uint8_t Adafruit_ADS7128::getEventHighFlags() {
  return _readRegister(ADS7128_REG_EVENT_HIGH_FLAG);
}

uint8_t Adafruit_ADS7128::getEventLowFlags() {
  return _readRegister(ADS7128_REG_EVENT_LOW_FLAG);
}

bool Adafruit_ADS7128::clearEventFlags() {
  // Write 0xFF to clear all flags (W1C registers)
  if (!_writeRegister(ADS7128_REG_EVENT_HIGH_FLAG, 0xFF)) {
    return false;
  }
  return _writeRegister(ADS7128_REG_EVENT_LOW_FLAG, 0xFF);
}

// ---------------------------------------------------------------------------
// ALERT Pin Configuration
// ---------------------------------------------------------------------------

bool Adafruit_ADS7128::configureAlert(bool pushPull, uint8_t logic) {
  uint8_t cfg = (logic & 0x03);
  if (pushPull) {
    cfg |= 0x04; // ALERT_DRIVE bit
  }
  return _writeRegister(ADS7128_REG_ALERT_PIN_CFG, cfg);
}

bool Adafruit_ADS7128::setAlertChannels(uint8_t channelMask) {
  return _writeRegister(ADS7128_REG_ALERT_CH_SEL, channelMask);
}

// ---------------------------------------------------------------------------
// Sampling Rate
// ---------------------------------------------------------------------------

bool Adafruit_ADS7128::setSamplingRate(bool slowOsc, uint8_t divider) {
  uint8_t cfg = _readRegister(ADS7128_REG_OPMODE_CFG);
  cfg = (cfg & 0xE0); // Preserve CONV_ON_ERR and CONV_MODE
  cfg |= (divider & 0x0F);
  if (slowOsc) {
    cfg |= 0x10; // OSC_SEL bit
  }
  return _writeRegister(ADS7128_REG_OPMODE_CFG, cfg);
}

// ---------------------------------------------------------------------------
// Private Methods - Low-level register access
// ---------------------------------------------------------------------------

uint8_t Adafruit_ADS7128::_readRegister(uint8_t reg) {
  uint8_t cmd[2] = {ADS7128_OP_SINGLE_READ, reg};

  if (_crc_enabled) {
    uint8_t response[2];
    if (!_i2c->write_then_read(cmd, 2, response, 2)) {
      return 0;
    }
    uint8_t expected_crc = _crc8(&response[0], 1);
    if (response[1] != expected_crc) {
      _crc_error = true;
    }
    return response[0];
  } else {
    uint8_t response;
    if (!_i2c->write_then_read(cmd, 2, &response, 1)) {
      return 0;
    }
    return response;
  }
}

bool Adafruit_ADS7128::_writeRegister(uint8_t reg, uint8_t data) {
  if (_crc_enabled) {
    uint8_t cmd[4];
    cmd[0] = ADS7128_OP_SINGLE_WRITE;
    cmd[1] = reg;
    cmd[2] = data;
    cmd[3] = _crc8(cmd, 3);
    return _i2c->write(cmd, 4);
  } else {
    uint8_t cmd[3] = {ADS7128_OP_SINGLE_WRITE, reg, data};
    return _i2c->write(cmd, 3);
  }
}

bool Adafruit_ADS7128::_setBits(uint8_t reg, uint8_t mask) {
  if (_crc_enabled) {
    uint8_t cmd[4];
    cmd[0] = ADS7128_OP_SET_BIT;
    cmd[1] = reg;
    cmd[2] = mask;
    cmd[3] = _crc8(cmd, 3);
    return _i2c->write(cmd, 4);
  } else {
    uint8_t cmd[3] = {ADS7128_OP_SET_BIT, reg, mask};
    return _i2c->write(cmd, 3);
  }
}

bool Adafruit_ADS7128::_clearBits(uint8_t reg, uint8_t mask) {
  if (_crc_enabled) {
    uint8_t cmd[4];
    cmd[0] = ADS7128_OP_CLEAR_BIT;
    cmd[1] = reg;
    cmd[2] = mask;
    cmd[3] = _crc8(cmd, 3);
    return _i2c->write(cmd, 4);
  } else {
    uint8_t cmd[3] = {ADS7128_OP_CLEAR_BIT, reg, mask};
    return _i2c->write(cmd, 3);
  }
}

bool Adafruit_ADS7128::_readBlock(uint8_t startReg, uint8_t *buf, uint8_t len) {
  uint8_t cmd[2] = {ADS7128_OP_BLOCK_READ, startReg};
  return _i2c->write_then_read(cmd, 2, buf, len);
}

uint16_t Adafruit_ADS7128::_read12BitValue(uint8_t lsbReg) {
  uint8_t lsb = _readRegister(lsbReg);
  uint8_t msb = _readRegister(lsbReg + 1);
  // 12-bit value is MSB-aligned in 16-bit register pair:
  //   MSB register = [D11:D4], LSB register = [D3:D0, 0, 0, 0, 0]
  return ((uint16_t)msb << 4) | (lsb >> 4);
}

uint8_t Adafruit_ADS7128::_crc8(uint8_t *data, uint8_t len) {
  uint8_t crc = 0x00;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x80) {
        crc = (crc << 1) ^ 0x07;
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}
