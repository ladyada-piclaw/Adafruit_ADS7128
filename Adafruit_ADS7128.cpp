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

/**
 * @brief Construct a new Adafruit_ADS7128 object
 */
Adafruit_ADS7128::Adafruit_ADS7128() {
  _i2c = nullptr;
  _crc_enabled = false;
  _crc_error = false;
}

/**
 * @brief Destroy the Adafruit_ADS7128 object
 */
Adafruit_ADS7128::~Adafruit_ADS7128() {
  if (_i2c) {
    delete _i2c;
  }
}

/**
 * @brief Initialize the ADS7128
 *
 * Performs:
 * 1. I2C device initialization
 * 2. Software reset (write RST=1b to GENERAL_CFG)
 * 3. Wait for reset completion
 * 4. ADC calibration (write CAL=1b, wait for auto-clear)
 * 5. Clear BOR flag (write 1b to BOR)
 * 6. Enable CRC (write CRC_EN=1b to GENERAL_CFG)
 *
 * @param addr I2C address (default 0x11)
 * @param wire Pointer to TwoWire instance
 * @return true on success, false on failure
 */
bool Adafruit_ADS7128::begin(uint8_t addr, TwoWire *wire) {
  if (_i2c) {
    delete _i2c;
  }

  // General call reset first to clear any lingering CRC state
  // This resets ALL I2C devices that support it, but ensures clean state
  wire->beginTransmission(0x00);
  wire->write(0x06);
  wire->endTransmission();
  delay(10);

  _i2c = new Adafruit_I2CDevice(addr, wire);
  if (!_i2c->begin()) {
    return false;
  }

  // CRC is disabled at this point - reset clears it
  _crc_enabled = false;

  // Software reset: write RST=1b to GENERAL_CFG
  if (!_writeRegister(ADS7128_REG_GENERAL_CFG, ADS7128_BIT_RST)) {
    return false;
  }

  // Wait for reset to complete (~1ms)
  delay(5);

  // ADC calibration: write CAL=1b, then poll until it auto-clears
  if (!_writeRegister(ADS7128_REG_GENERAL_CFG, ADS7128_BIT_CAL)) {
    return false;
  }

  // Poll for calibration complete (CAL bit clears when done)
  uint16_t timeout = 1000;
  while (timeout--) {
    uint8_t cfg = _readRegister(ADS7128_REG_GENERAL_CFG);
    if ((cfg & ADS7128_BIT_CAL) == 0) {
      break;
    }
    delay(1);
  }
  if (timeout == 0) {
    return false; // Calibration timed out
  }

  // Clear BOR flag by writing 1b to it
  if (!_writeRegister(ADS7128_REG_SYSTEM_STATUS, ADS7128_BIT_BOR)) {
    return false;
  }

  // CRC disabled by default - the CRC byte format for writes is not yet
  // verified on hardware. Call enableCRC(true) after begin() to enable.
  _crc_enabled = false;

  return true;
}

/**
 * @brief Configure a channel as analog input, digital input, or digital output
 *
 * | Mode                    | PIN_CFG | GPIO_CFG | GPO_DRIVE_CFG |
 * |-------------------------|---------|----------|---------------|
 * | ADS7128_ANALOG          | 0       | x        | x             |
 * | ADS7128_INPUT           | 1       | 0        | x             |
 * | ADS7128_OUTPUT          | 1       | 1        | 1             |
 * | ADS7128_OUTPUT_OPENDRAIN| 1       | 1        | 0             |
 *
 * @param channel Channel number (0-7)
 * @param mode Pin mode
 * @return true on success, false on I2C error or invalid channel
 */
bool Adafruit_ADS7128::pinMode(uint8_t channel, ads7128_pin_mode_t mode) {
  if (channel > 7) {
    return false;
  }

  uint8_t mask = 1 << channel;

  switch (mode) {
  case ADS7128_ANALOG:
    // PIN_CFG = 0 (analog), GPIO_CFG don't care
    if (!_clearBits(ADS7128_REG_PIN_CFG, mask)) {
      return false;
    }
    break;

  case ADS7128_INPUT:
    // PIN_CFG = 1 (GPIO), GPIO_CFG = 0 (input)
    if (!_setBits(ADS7128_REG_PIN_CFG, mask)) {
      return false;
    }
    if (!_clearBits(ADS7128_REG_GPIO_CFG, mask)) {
      return false;
    }
    break;

  case ADS7128_OUTPUT:
    // PIN_CFG = 1 (GPIO), GPIO_CFG = 1 (output), GPO_DRIVE_CFG = 1 (push-pull)
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
    // PIN_CFG = 1 (GPIO), GPIO_CFG = 1 (output), GPO_DRIVE_CFG = 0 (open-drain)
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

/**
 * @brief Set digital output level using atomic set/clear operations
 *
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
 * @brief Read digital input level from GPI_VALUE register
 *
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

/**
 * @brief Enable or disable CRC on I2C interface
 *
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
 * @brief Check if CRC error detected
 *
 * @return true if CRC_ERR_IN flag is set
 */
bool Adafruit_ADS7128::getCRCError() {
  uint8_t status = _readRegister(ADS7128_REG_SYSTEM_STATUS);
  return (status & ADS7128_BIT_CRC_ERR_IN) != 0;
}

/**
 * @brief Clear CRC error flag by writing 1b to CRC_ERR_IN
 *
 * @return true on success, false on I2C error
 */
bool Adafruit_ADS7128::clearCRCError() {
  _crc_error = false;
  return _writeRegister(ADS7128_REG_SYSTEM_STATUS, ADS7128_BIT_CRC_ERR_IN);
}

// ---------------------------------------------------------------------------
// Private Methods - Low-level register access with opcode protocol
// ---------------------------------------------------------------------------

/**
 * @brief Read a single register using opcode 0x10
 *
 * Frame: [opcode 0x10, reg_addr] then read [data] (or [data, crc] if CRC
 * enabled)
 *
 * @param reg Register address
 * @return Register value (0 on error)
 */
uint8_t Adafruit_ADS7128::_readRegister(uint8_t reg) {
  uint8_t cmd[2] = {ADS7128_OP_SINGLE_READ, reg};

  if (_crc_enabled) {
    // Write command, then read data + CRC
    uint8_t response[2];
    if (!_i2c->write_then_read(cmd, 2, response, 2)) {
      return 0;
    }

    // Validate CRC - CRC covers only the data byte for reads
    uint8_t expected_crc = _crc8(&response[0], 1);
    if (response[1] != expected_crc) {
      _crc_error = true;
    }

    return response[0];
  } else {
    // Write command, then read data
    uint8_t response;
    if (!_i2c->write_then_read(cmd, 2, &response, 1)) {
      return 0;
    }
    return response;
  }
}

/**
 * @brief Write a single register using opcode 0x08
 *
 * Frame: [opcode 0x08, reg_addr, data] (append CRC if enabled)
 *
 * @param reg Register address
 * @param data Data to write
 * @return true on success, false on I2C error
 */
bool Adafruit_ADS7128::_writeRegister(uint8_t reg, uint8_t data) {
  if (_crc_enabled) {
    // CRC covers all bytes: opcode, reg_addr, data
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

/**
 * @brief Set bits in a register using opcode 0x18 (atomic OR)
 *
 * Frame: [opcode 0x18, reg_addr, mask] (append CRC if enabled)
 * Bits with value 1 in mask are SET, bits with value 0 are unchanged.
 *
 * @param reg Register address
 * @param mask Bit mask (1 = set that bit)
 * @return true on success, false on I2C error
 */
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

/**
 * @brief Clear bits in a register using opcode 0x20 (atomic AND-NOT)
 *
 * Frame: [opcode 0x20, reg_addr, mask] (append CRC if enabled)
 * Bits with value 1 in mask are CLEARED, bits with value 0 are unchanged.
 *
 * @param reg Register address
 * @param mask Bit mask (1 = clear that bit)
 * @return true on success, false on I2C error
 */
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

/**
 * @brief Calculate CRC-8-CCITT
 *
 * Polynomial: x^8 + x^2 + x + 1 (0x07)
 * Uses bitwise implementation to save flash on AVR.
 *
 * @param data Pointer to data buffer
 * @param len Number of bytes
 * @return 8-bit CRC value
 */
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
