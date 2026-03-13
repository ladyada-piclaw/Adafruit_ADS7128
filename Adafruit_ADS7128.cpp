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

/**
 * @brief Initialize the ADS7128
 * @param addr I2C address (default 0x10)
 * @param wire Pointer to TwoWire instance
 * @return true on success, false on failure
 */
bool Adafruit_ADS7128::begin(uint8_t addr, TwoWire* wire) {
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

/**
 * @brief Configure a channel as analog input, digital input, or digital
 * output
 * @param channel Channel number (0-7)
 * @param mode Pin mode (see ads7128_pin_mode_t)
 * @return true on success, false on I2C error or invalid channel
 */
bool Adafruit_ADS7128::pinMode(uint8_t channel, ads7128_pin_mode_t mode) {
  if (channel > 7) {
    return false;
  }

  uint8_t mask = 1 << channel;

  switch (mode) {
    case ADS7128_ANALOG:
      if (!_clearBits(ADS7128_REG_PIN_CFG, mask))
        return false;
      break;

    case ADS7128_INPUT:
      if (!_setBits(ADS7128_REG_PIN_CFG, mask) ||
          !_clearBits(ADS7128_REG_GPIO_CFG, mask))
        return false;
      break;

    case ADS7128_OUTPUT:
      if (!_setBits(ADS7128_REG_PIN_CFG, mask) ||
          !_setBits(ADS7128_REG_GPIO_CFG, mask) ||
          !_setBits(ADS7128_REG_GPO_DRIVE_CFG, mask))
        return false;
      break;

    case ADS7128_OUTPUT_OPENDRAIN:
      if (!_setBits(ADS7128_REG_PIN_CFG, mask) ||
          !_setBits(ADS7128_REG_GPIO_CFG, mask) ||
          !_clearBits(ADS7128_REG_GPO_DRIVE_CFG, mask))
        return false;
      break;

    default:
      return false;
  }

  return true;
}

/**
 * @brief Get the current pin mode for a channel
 * @param channel Channel number (0-7)
 * @return Current pin mode (ADS7128_ANALOG, ADS7128_INPUT, ADS7128_OUTPUT,
 *         or ADS7128_OUTPUT_OPENDRAIN). Returns ADS7128_ANALOG on error.
 */
ads7128_pin_mode_t Adafruit_ADS7128::getPinMode(uint8_t channel) {
  if (channel > 7)
    return ADS7128_ANALOG;
  uint8_t mask = 1 << channel;
  bool isDigital = (_readRegister(ADS7128_REG_PIN_CFG) & mask) != 0;
  if (!isDigital)
    return ADS7128_ANALOG;
  bool isOutput = (_readRegister(ADS7128_REG_GPIO_CFG) & mask) != 0;
  if (!isOutput)
    return ADS7128_INPUT;
  bool isPushPull = (_readRegister(ADS7128_REG_GPO_DRIVE_CFG) & mask) != 0;
  return isPushPull ? ADS7128_OUTPUT : ADS7128_OUTPUT_OPENDRAIN;
}

/**
 * @brief Set digital output level
 * @param channel Channel number (0-7)
 * @param value true=HIGH, false=LOW
 * @return true on success, false on I2C error
 */
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

/**
 * @brief Read digital input level
 * @param channel Channel number (0-7)
 * @return Input state (true=HIGH, false=LOW)
 */
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

/**
 * @brief Enable or disable CRC on I2C interface
 * @param enable true=enable CRC, false=disable
 * @return true on success, false on I2C error
 */
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

/**
 * @brief Check if CRC is enabled
 * @return true if CRC checking is active
 */
bool Adafruit_ADS7128::getCRCEnabled() {
  return (_readRegister(ADS7128_REG_GENERAL_CFG) & ADS7128_BIT_CRC_EN) != 0;
}

/**
 * @brief Check if CRC error detected
 * @return true if CRC_ERR_IN flag is set
 */
bool Adafruit_ADS7128::getCRCError() {
  uint8_t status = _readRegister(ADS7128_REG_SYSTEM_STATUS);
  return (status & ADS7128_BIT_CRC_ERR_IN) != 0;
}

/**
 * @brief Clear CRC error flag
 * @return true on success, false on I2C error
 */
bool Adafruit_ADS7128::clearCRCError() {
  _crc_error = false;
  return _writeRegister(ADS7128_REG_SYSTEM_STATUS, ADS7128_BIT_CRC_ERR_IN);
}

// ---------------------------------------------------------------------------
// ADC Functions - Manual Mode
// ---------------------------------------------------------------------------

/**
 * @brief Read ADC value from a channel in manual mode
 * @param channel Channel number (0-7)
 * @return 12-bit ADC value (0-4095), or 0xFFFF on error
 */
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

/**
 * @brief Read ADC value and convert to voltage
 * @param channel Channel number (0-7)
 * @param vref Reference voltage (default 5.0V)
 * @return Voltage in volts, or -1.0 on error
 */
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

/**
 * @brief Set channels to include in auto-sequence
 * @param channelMask Bitmask of channels (bit 0 = CH0, etc.)
 * @return true on success, false on I2C error
 */
bool Adafruit_ADS7128::setSequenceChannels(uint8_t channelMask) {
  return _writeRegister(ADS7128_REG_AUTO_SEQ_CH_SEL, channelMask);
}

/**
 * @brief Start auto-sequence mode
 * @return true on success, false on I2C error
 */
bool Adafruit_ADS7128::startSequence() {
  // Enable statistics, channel ID append, and autonomous mode
  if (!_setBits(ADS7128_REG_GENERAL_CFG, ADS7128_BIT_STATS_EN) ||
      !_setBits(ADS7128_REG_DATA_CFG, ADS7128_APPEND_CHID) ||
      !_setBits(ADS7128_REG_OPMODE_CFG, ADS7128_BIT_CONV_MODE))
    return false;

  // Set SEQ_MODE=1 and SEQ_START=1 in SEQUENCE_CFG
  return _writeRegister(ADS7128_REG_SEQUENCE_CFG,
                        ADS7128_BIT_SEQ_MODE | ADS7128_BIT_SEQ_START);
}

/**
 * @brief Stop auto-sequence mode
 * @return true on success, false on I2C error
 */
bool Adafruit_ADS7128::stopSequence() {
  // Clear SEQ_START bit
  return _clearBits(ADS7128_REG_SEQUENCE_CFG, ADS7128_BIT_SEQ_START);
}

/**
 * @brief Read next conversion result from sequence
 * @param channel Pointer to store channel ID (optional, can be nullptr)
 * @return 12-bit ADC value (0-4095), or 0xFFFF on error
 */
uint16_t Adafruit_ADS7128::readSequenceResult(uint8_t* channel) {
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

/**
 * @brief Set oversampling ratio
 * @param osr Oversampling ratio (see ads7128_osr_t)
 * @return true on success, false on I2C error
 */
bool Adafruit_ADS7128::setOversampling(ads7128_osr_t osr) {
  return _writeRegister(ADS7128_REG_OSR_CFG, (uint8_t)osr & 0x07);
}

/**
 * @brief Get current oversampling ratio
 * @return Current oversampling ratio setting
 */
ads7128_osr_t Adafruit_ADS7128::getOversampling() {
  uint8_t val = _readRegister(ADS7128_REG_OSR_CFG);
  return (ads7128_osr_t)(val & 0x07);
}

// ---------------------------------------------------------------------------
// Statistics Functions
// ---------------------------------------------------------------------------

/**
 * @brief Enable or disable statistics module (min/max/recent tracking)
 * @param enable true to enable, false to disable
 * @return true on success, false on I2C error
 */
bool Adafruit_ADS7128::enableStatistics(bool enable) {
  if (enable) {
    return _setBits(ADS7128_REG_GENERAL_CFG, ADS7128_BIT_STATS_EN);
  } else {
    return _clearBits(ADS7128_REG_GENERAL_CFG, ADS7128_BIT_STATS_EN);
  }
}

/**
 * @brief Check if statistics tracking is enabled
 * @return true if statistics (min/max/recent) tracking is active
 */
bool Adafruit_ADS7128::getStatisticsEnabled() {
  return (_readRegister(ADS7128_REG_GENERAL_CFG) & ADS7128_BIT_STATS_EN) != 0;
}

/**
 * @brief Get maximum recorded value for a channel
 * @param channel Channel number (0-7)
 * @return 12-bit max value, or 0xFFFF on error
 */
uint16_t Adafruit_ADS7128::getMax(uint8_t channel) {
  if (channel > 7) {
    return 0xFFFF;
  }
  uint8_t lsbReg = ADS7128_REG_MAX_LSB_CH0 + (channel * 2);
  return _read12BitValue(lsbReg);
}

/**
 * @brief Get minimum recorded value for a channel
 * @param channel Channel number (0-7)
 * @return 12-bit min value, or 0xFFFF on error
 */
uint16_t Adafruit_ADS7128::getMin(uint8_t channel) {
  if (channel > 7) {
    return 0xFFFF;
  }
  uint8_t lsbReg = ADS7128_REG_MIN_LSB_CH0 + (channel * 2);
  return _read12BitValue(lsbReg);
}

/**
 * @brief Get most recent conversion value for a channel
 * @param channel Channel number (0-7)
 * @return 12-bit recent value, or 0xFFFF on error
 */
uint16_t Adafruit_ADS7128::getRecent(uint8_t channel) {
  if (channel > 7) {
    return 0xFFFF;
  }
  uint8_t lsbReg = ADS7128_REG_RECENT_LSB_CH0 + (channel * 2);
  return _read12BitValue(lsbReg);
}

/**
 * @brief Reset all statistics (min/max/recent)
 * @return true on success, false on I2C error
 */
bool Adafruit_ADS7128::resetStatistics() {
  // Toggle STATS_EN: writing 1 clears statistics and restarts recording
  if (!_clearBits(ADS7128_REG_GENERAL_CFG, ADS7128_BIT_STATS_EN)) {
    return false;
  }
  return _setBits(ADS7128_REG_GENERAL_CFG, ADS7128_BIT_STATS_EN);
}

// ---------------------------------------------------------------------------
// Digital Window Comparator
// ---------------------------------------------------------------------------

/**
 * @brief Enable or disable digital window comparator
 * @param enable true to enable, false to disable
 * @return true on success, false on I2C error
 */
bool Adafruit_ADS7128::enableDWC(bool enable) {
  if (enable) {
    return _setBits(ADS7128_REG_GENERAL_CFG, ADS7128_BIT_DWC_EN);
  } else {
    return _clearBits(ADS7128_REG_GENERAL_CFG, ADS7128_BIT_DWC_EN);
  }
}

/**
 * @brief Check if the digital window comparator is enabled
 * @return true if DWC is active
 */
bool Adafruit_ADS7128::getDWCEnabled() {
  return (_readRegister(ADS7128_REG_GENERAL_CFG) & ADS7128_BIT_DWC_EN) != 0;
}

/**
 * @brief Set high threshold for a channel
 * @param channel Channel number (0-7)
 * @param threshold 12-bit threshold value
 * @return true on success, false on I2C error
 */
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

/**
 * @brief Get high threshold for a channel
 * @param channel Channel number (0-7)
 * @return 12-bit threshold value, or 0xFFFF on error
 */
uint16_t Adafruit_ADS7128::getHighThreshold(uint8_t channel) {
  if (channel > 7)
    return 0xFFFF;
  uint8_t baseAddr = ADS7128_REG_HYSTERESIS_CH0 + (channel * 4);
  uint8_t msb = _readRegister(baseAddr + 1);
  uint8_t hystReg = _readRegister(baseAddr);
  return ((uint16_t)msb << 4) | ((hystReg >> 4) & 0x0F);
}

/**
 * @brief Set low threshold for a channel
 * @param channel Channel number (0-7)
 * @param threshold 12-bit threshold value
 * @return true on success, false on I2C error
 */
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

/**
 * @brief Get low threshold for a channel
 * @param channel Channel number (0-7)
 * @return 12-bit threshold value, or 0xFFFF on error
 */
uint16_t Adafruit_ADS7128::getLowThreshold(uint8_t channel) {
  if (channel > 7)
    return 0xFFFF;
  uint8_t baseAddr = ADS7128_REG_HYSTERESIS_CH0 + (channel * 4);
  uint8_t msb = _readRegister(baseAddr + 3);
  uint8_t evtReg = _readRegister(baseAddr + 2);
  return ((uint16_t)msb << 4) | ((evtReg >> 4) & 0x0F);
}

/**
 * @brief Set hysteresis for a channel
 * @param channel Channel number (0-7)
 * @param hysteresis 4-bit hysteresis value (0-15)
 * @return true on success, false on I2C error
 */
bool Adafruit_ADS7128::setHysteresis(uint8_t channel, uint8_t hysteresis) {
  if (channel > 7 || hysteresis > 15) {
    return false;
  }

  uint8_t baseAddr = ADS7128_REG_HYSTERESIS_CH0 + (channel * 4);
  uint8_t hystReg = _readRegister(baseAddr);
  hystReg = (hystReg & 0xF0) | (hysteresis & 0x0F);
  return _writeRegister(baseAddr, hystReg);
}

/**
 * @brief Get hysteresis for a channel
 * @param channel Channel number (0-7)
 * @return 4-bit hysteresis value (0-15), or 0xFF on error
 */
uint8_t Adafruit_ADS7128::getHysteresis(uint8_t channel) {
  if (channel > 7)
    return 0xFF;
  uint8_t baseAddr = ADS7128_REG_HYSTERESIS_CH0 + (channel * 4);
  return _readRegister(baseAddr) & 0x0F;
}

/**
 * @brief Set event region mode for a channel
 * @param channel Channel number (0-7)
 * @param inBand true = in-band mode (digital: logic 0 sets low flag),
 *               false = out-of-window mode (digital: logic 1 sets high flag)
 * @return true on success, false on I2C error
 */
bool Adafruit_ADS7128::setEventRegion(uint8_t channel, bool inBand) {
  if (channel > 7)
    return false;
  uint8_t mask = 1 << channel;
  if (inBand)
    return _setBits(ADS7128_REG_EVENT_RGN, mask);
  else
    return _clearBits(ADS7128_REG_EVENT_RGN, mask);
}

/**
 * @brief Get event region mode for a channel
 * @param channel Channel number (0-7)
 * @return true if in-band mode, false if out-of-window mode
 */
bool Adafruit_ADS7128::getEventRegion(uint8_t channel) {
  if (channel > 7)
    return false;
  return (_readRegister(ADS7128_REG_EVENT_RGN) & (1 << channel)) != 0;
}

/**
 * @brief Set event count for a channel (consecutive samples before alert)
 * @param channel Channel number (0-7)
 * @param count 4-bit event count (0-15, alert after count+1 samples)
 * @return true on success, false on I2C error
 */
bool Adafruit_ADS7128::setEventCount(uint8_t channel, uint8_t count) {
  if (channel > 7 || count > 15) {
    return false;
  }

  uint8_t baseAddr = ADS7128_REG_HYSTERESIS_CH0 + (channel * 4);
  uint8_t evtReg = _readRegister(baseAddr + 2);
  evtReg = (evtReg & 0xF0) | (count & 0x0F);
  return _writeRegister(baseAddr + 2, evtReg);
}

/**
 * @brief Get event flags for all channels
 * @return 8-bit event flags (bit per channel)
 */
uint8_t Adafruit_ADS7128::getEventFlags() {
  return _readRegister(ADS7128_REG_EVENT_FLAG);
}

/**
 * @brief Get high threshold event flags
 * @return 8-bit event flags (bit per channel)
 */
uint8_t Adafruit_ADS7128::getEventHighFlags() {
  return _readRegister(ADS7128_REG_EVENT_HIGH_FLAG);
}

/**
 * @brief Get low threshold event flags
 * @return 8-bit event flags (bit per channel)
 */
uint8_t Adafruit_ADS7128::getEventLowFlags() {
  return _readRegister(ADS7128_REG_EVENT_LOW_FLAG);
}

/**
 * @brief Clear all event flags
 * @return true on success, false on I2C error
 */
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

/**
 * @brief Configure ALERT pin output mode and logic
 * @param pushPull true for push-pull, false for open-drain
 * @param logic 0=active low, 1=active high, 2=pulsed low, 3=pulsed high
 * @return true on success, false on I2C error
 */
bool Adafruit_ADS7128::configureAlert(bool pushPull, uint8_t logic) {
  uint8_t cfg = (logic & 0x03);
  if (pushPull) {
    cfg |= 0x04; // ALERT_DRIVE bit
  }
  return _writeRegister(ADS7128_REG_ALERT_PIN_CFG, cfg);
}

/**
 * @brief Check if ALERT pin is configured as push-pull
 * @return true if push-pull, false if open-drain
 */
bool Adafruit_ADS7128::getAlertPushPull() {
  return (_readRegister(ADS7128_REG_ALERT_PIN_CFG) & 0x04) != 0;
}

/**
 * @brief Get ALERT pin logic configuration
 * @return Logic value (0-3): 0=active low, 1=active high, etc.
 */
uint8_t Adafruit_ADS7128::getAlertLogic() {
  return _readRegister(ADS7128_REG_ALERT_PIN_CFG) & 0x03;
}

/**
 * @brief Set which channels trigger ALERT pin
 * @param channelMask Bitmask of channels (bit 0 = CH0, etc.)
 * @return true on success, false on I2C error
 */
bool Adafruit_ADS7128::setAlertChannels(uint8_t channelMask) {
  return _writeRegister(ADS7128_REG_ALERT_CH_SEL, channelMask);
}

/**
 * @brief Get which channels trigger ALERT pin
 * @return Bitmask of channels (bit 0 = CH0, etc.)
 */
uint8_t Adafruit_ADS7128::getAlertChannels() {
  return _readRegister(ADS7128_REG_ALERT_CH_SEL);
}

// ---------------------------------------------------------------------------
// Sampling Rate
// ---------------------------------------------------------------------------

/**
 * @brief Set sampling rate for autonomous mode
 * @param slowOsc true for low-power oscillator, false for high-speed
 * @param divider Clock divider (0-7)
 * @return true on success, false on I2C error
 */
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
// Zero-Crossing Detection (ZCD)
// ---------------------------------------------------------------------------

/**
 * @brief Set which analog channel the ZCD module monitors
 * @param channel Channel number (0-7)
 * @return true on success, false on I2C error or invalid channel
 */
bool Adafruit_ADS7128::setZCDChannel(uint8_t channel) {
  if (channel > 7)
    return false;
  // ZCD_CHID is upper nibble of CHANNEL_SEL (0x11)
  uint8_t reg = _readRegister(ADS7128_REG_CHANNEL_SEL);
  reg = (reg & 0x0F) | (channel << 4);
  return _writeRegister(ADS7128_REG_CHANNEL_SEL, reg);
}

/**
 * @brief Configure ZCD blanking (transient rejection)
 * @param count Blanking count (0-127, conversions to skip after ZCD event)
 * @param multiply true = count x8, false = count x1
 * @return true on success, false on I2C error
 */
bool Adafruit_ADS7128::setZCDBlanking(uint8_t count, bool multiply) {
  uint8_t val = (count & 0x7F);
  if (multiply) {
    val |= 0x80; // MULT_EN bit
  }
  return _writeRegister(ADS7128_REG_ZCD_BLANKING_CFG, val);
}

/**
 * @brief Map ZCD output to a GPO pin
 * @param gpoChannel GPO channel (0-7, must be configured as digital output)
 * @param mode 0=low, 1=high, 2=ZCD signal, 3=inverted ZCD
 * @return true on success, false on I2C error or invalid channel
 */
bool Adafruit_ADS7128::setZCDOutput(uint8_t gpoChannel, uint8_t mode) {
  if (gpoChannel > 7 || mode > 3)
    return false;
  // CH0-CH3 in register 0xE3, CH4-CH7 in register 0xE4
  // Each channel gets 2 bits
  uint8_t reg;
  uint8_t shift;
  if (gpoChannel < 4) {
    reg = ADS7128_REG_GPO_VALUE_ZCD_CFG_CH0_CH3;
    shift = gpoChannel * 2;
  } else {
    reg = ADS7128_REG_GPO_VALUE_ZCD_CFG_CH4_CH7;
    shift = (gpoChannel - 4) * 2;
  }
  uint8_t val = _readRegister(reg);
  val &= ~(0x03 << shift);
  val |= (mode << shift);
  return _writeRegister(reg, val);
}

/**
 * @brief Enable or disable ZCD-to-GPO updates for a channel
 * @param gpoChannel GPO channel (0-7)
 * @param enable true to enable ZCD updates on this GPO
 * @return true on success, false on I2C error
 */
bool Adafruit_ADS7128::enableZCDOutput(uint8_t gpoChannel, bool enable) {
  if (gpoChannel > 7)
    return false;
  uint8_t mask = (1 << gpoChannel);
  if (enable) {
    return _setBits(ADS7128_REG_GPO_ZCD_UPDATE_EN, mask);
  } else {
    return _clearBits(ADS7128_REG_GPO_ZCD_UPDATE_EN, mask);
  }
}

/**
 * @brief Check if ZCD output is enabled for a specific GPO channel
 * @param gpoChannel GPO channel number (0-7)
 * @return true if ZCD output is enabled for that channel
 */
bool Adafruit_ADS7128::getZCDOutputEnabled(uint8_t gpoChannel) {
  if (gpoChannel > 7)
    return false;
  uint8_t mask = (1 << gpoChannel);
  return (_readRegister(ADS7128_REG_GPO_ZCD_UPDATE_EN) & mask) != 0;
}

// ---------------------------------------------------------------------------
// RMS Functions
// ---------------------------------------------------------------------------

/**
 * @brief Enable or disable the RMS computation module
 *
 * When enabled, the RMS module clears any previous result and begins
 * a new computation using samples from autonomous conversion mode.
 *
 * @param enable true to enable RMS (starts new computation), false to disable
 * @return true on success, false on I2C error
 */
bool Adafruit_ADS7128::enableRMS(bool enable) {
  if (enable) {
    return _setBits(ADS7128_REG_GENERAL_CFG, ADS7128_BIT_RMS_EN);
  } else {
    return _clearBits(ADS7128_REG_GENERAL_CFG, ADS7128_BIT_RMS_EN);
  }
}

/**
 * @brief Check if RMS computation is enabled
 * @return true if RMS module is active
 */
bool Adafruit_ADS7128::getRMSEnabled() {
  return (_readRegister(ADS7128_REG_GENERAL_CFG) & ADS7128_BIT_RMS_EN) != 0;
}

/**
 * @brief Set which analog channel the RMS module monitors
 *
 * The RMS module computes the root-mean-square value of samples from
 * the specified channel during autonomous conversion mode.
 *
 * @param channel Channel number (0-7)
 * @return true on success, false on I2C error or invalid channel
 */
bool Adafruit_ADS7128::setRMSChannel(uint8_t channel) {
  if (channel > 7)
    return false;
  uint8_t reg = _readRegister(ADS7128_REG_RMS_CFG);
  reg = (reg & 0x0F) | (channel << 4);
  return _writeRegister(ADS7128_REG_RMS_CFG, reg);
}

/**
 * @brief Set number of samples for RMS computation
 *
 * More samples give higher accuracy but take longer to compute.
 * The RMS module must accumulate the specified number of samples
 * before the result is valid.
 *
 * @param setting Sample count: 0=1024, 1=4096, 2=16384, 3=65536 samples
 * @return true on success, false on I2C error or invalid setting
 */
bool Adafruit_ADS7128::setRMSSamples(uint8_t setting) {
  if (setting > 3)
    return false;
  uint8_t reg = _readRegister(ADS7128_REG_RMS_CFG);
  reg = (reg & 0xFC) | (setting & 0x03);
  return _writeRegister(ADS7128_REG_RMS_CFG, reg);
}

/**
 * @brief Enable or disable DC subtraction for RMS calculation
 *
 * When enabled, the DC component (average) is subtracted from samples
 * before RMS calculation, giving the AC RMS value.
 *
 * @param enable true to subtract DC component, false for total RMS
 * @return true on success, false on I2C error
 */
bool Adafruit_ADS7128::setRMSDCSub(bool enable) {
  if (enable) {
    return _setBits(ADS7128_REG_RMS_CFG, 0x04);
  } else {
    return _clearBits(ADS7128_REG_RMS_CFG, 0x04);
  }
}

/**
 * @brief Read the 16-bit RMS result
 *
 * Returns the RMS value computed over the configured number of samples.
 * This is a full 16-bit value (not 12-bit like ADC readings).
 * Check isRMSDone() before reading to ensure computation is complete.
 *
 * @return 16-bit RMS value, or 0xFFFF on I2C error
 */
uint16_t Adafruit_ADS7128::getRMS() {
  uint8_t lsb = _readRegister(ADS7128_REG_RMS_LSB);
  uint8_t msb = _readRegister(ADS7128_REG_RMS_MSB);
  return ((uint16_t)msb << 8) | lsb;
}

/**
 * @brief Check if RMS computation is complete
 *
 * Checks the RMS_DONE flag in SYSTEM_STATUS. If set, the flag is
 * automatically cleared by writing 1 to it (W1C behavior).
 *
 * @return true if RMS computation is done, false if not done or error
 */
bool Adafruit_ADS7128::isRMSDone() {
  uint8_t status = _readRegister(ADS7128_REG_SYSTEM_STATUS);
  if (status & ADS7128_BIT_RMS_DONE) {
    // Clear the flag by writing 1 to it
    _setBits(ADS7128_REG_SYSTEM_STATUS, ADS7128_BIT_RMS_DONE);
    return true;
  }
  return false;
}

// ---------------------------------------------------------------------------
// Private Methods - Low-level register access
// ---------------------------------------------------------------------------

uint8_t Adafruit_ADS7128::_readRegister(uint8_t reg) {
  if (_crc_enabled) {
    // With CRC: setup sends [opcode][CRC][reg][CRC], read returns [data][CRC]
    uint8_t cmd[4] = {ADS7128_OP_SINGLE_READ, _crc8(ADS7128_OP_SINGLE_READ),
                      reg, _crc8(reg)};
    uint8_t response[2];
    if (!_i2c->write_then_read(cmd, 4, response, 2)) {
      return 0;
    }
    if (response[1] != _crc8(response[0])) {
      _crc_error = true;
    }
    return response[0];
  } else {
    uint8_t cmd[2] = {ADS7128_OP_SINGLE_READ, reg};
    uint8_t response;
    if (!_i2c->write_then_read(cmd, 2, &response, 1)) {
      return 0;
    }
    return response;
  }
}

bool Adafruit_ADS7128::_writeRegister(uint8_t reg, uint8_t data) {
  if (_crc_enabled) {
    // With CRC: [opcode][CRC][reg][CRC][data][CRC]
    uint8_t cmd[6] = {ADS7128_OP_SINGLE_WRITE,
                      _crc8(ADS7128_OP_SINGLE_WRITE),
                      reg,
                      _crc8(reg),
                      data,
                      _crc8(data)};
    return _i2c->write(cmd, 6);
  } else {
    uint8_t cmd[3] = {ADS7128_OP_SINGLE_WRITE, reg, data};
    return _i2c->write(cmd, 3);
  }
}

bool Adafruit_ADS7128::_setBits(uint8_t reg, uint8_t mask) {
  if (_crc_enabled) {
    uint8_t cmd[6] = {
        ADS7128_OP_SET_BIT, _crc8(ADS7128_OP_SET_BIT), reg, _crc8(reg), mask,
        _crc8(mask)};
    return _i2c->write(cmd, 6);
  } else {
    uint8_t cmd[3] = {ADS7128_OP_SET_BIT, reg, mask};
    return _i2c->write(cmd, 3);
  }
}

bool Adafruit_ADS7128::_clearBits(uint8_t reg, uint8_t mask) {
  if (_crc_enabled) {
    uint8_t cmd[6] = {ADS7128_OP_CLEAR_BIT,
                      _crc8(ADS7128_OP_CLEAR_BIT),
                      reg,
                      _crc8(reg),
                      mask,
                      _crc8(mask)};
    return _i2c->write(cmd, 6);
  } else {
    uint8_t cmd[3] = {ADS7128_OP_CLEAR_BIT, reg, mask};
    return _i2c->write(cmd, 3);
  }
}

bool Adafruit_ADS7128::_readBlock(uint8_t startReg, uint8_t* buf, uint8_t len) {
  if (_crc_enabled) {
    // Setup: [opcode][CRC][reg][CRC]
    uint8_t cmd[4] = {ADS7128_OP_BLOCK_READ, _crc8(ADS7128_OP_BLOCK_READ),
                      startReg, _crc8(startReg)};
    // Response: [data0][CRC0][data1][CRC1]... = 2*len bytes
    uint8_t rawLen = len * 2;
    uint8_t raw[16]; // max 8 registers * 2
    if (rawLen > sizeof(raw))
      return false;
    if (!_i2c->write_then_read(cmd, 4, raw, rawLen)) {
      return false;
    }
    for (uint8_t i = 0; i < len; i++) {
      buf[i] = raw[i * 2];
      if (raw[i * 2 + 1] != _crc8(raw[i * 2])) {
        _crc_error = true;
      }
    }
    return true;
  } else {
    uint8_t cmd[2] = {ADS7128_OP_BLOCK_READ, startReg};
    return _i2c->write_then_read(cmd, 2, buf, len);
  }
}

uint16_t Adafruit_ADS7128::_read12BitValue(uint8_t lsbReg) {
  uint8_t lsb = _readRegister(lsbReg);
  uint8_t msb = _readRegister(lsbReg + 1);
  // 12-bit value is MSB-aligned in 16-bit register pair:
  //   MSB register = [D11:D4], LSB register = [D3:D0, 0, 0, 0, 0]
  return ((uint16_t)msb << 4) | (lsb >> 4);
}

uint8_t Adafruit_ADS7128::_crc8(uint8_t byte) {
  uint8_t crc = byte;
  for (uint8_t i = 0; i < 8; i++) {
    if (crc & 0x80) {
      crc = (crc << 1) ^ 0x07;
    } else {
      crc <<= 1;
    }
  }
  return crc;
}
