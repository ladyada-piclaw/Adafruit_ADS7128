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

// Default I2C address (ADDR pin to GND via ~10kΩ)
#define ADS7128_DEFAULT_ADDR 0x10

// Opcodes (NOT standard I2C register access!)
#define ADS7128_OP_SINGLE_READ 0x10
#define ADS7128_OP_SINGLE_WRITE 0x08
#define ADS7128_OP_SET_BIT 0x18
#define ADS7128_OP_CLEAR_BIT 0x20
#define ADS7128_OP_BLOCK_READ 0x30
#define ADS7128_OP_BLOCK_WRITE 0x28

// System and Configuration Registers
#define ADS7128_REG_SYSTEM_STATUS 0x00
#define ADS7128_REG_GENERAL_CFG 0x01
#define ADS7128_REG_DATA_CFG 0x02
#define ADS7128_REG_OSR_CFG 0x03
#define ADS7128_REG_OPMODE_CFG 0x04

// GPIO Configuration Registers
#define ADS7128_REG_PIN_CFG 0x05
#define ADS7128_REG_GPIO_CFG 0x07
#define ADS7128_REG_GPO_DRIVE_CFG 0x09
#define ADS7128_REG_GPO_VALUE 0x0B
#define ADS7128_REG_GPI_VALUE 0x0D

// Sequencer and Channel Selection
#define ADS7128_REG_ZCD_BLANKING_CFG 0x0F
#define ADS7128_REG_SEQUENCE_CFG 0x10
#define ADS7128_REG_CHANNEL_SEL 0x11
#define ADS7128_REG_AUTO_SEQ_CH_SEL 0x12

// Alert and Event Registers
#define ADS7128_REG_ALERT_CH_SEL 0x14
#define ADS7128_REG_ALERT_MAP 0x16
#define ADS7128_REG_ALERT_PIN_CFG 0x17
#define ADS7128_REG_EVENT_FLAG 0x18
#define ADS7128_REG_EVENT_HIGH_FLAG 0x1A
#define ADS7128_REG_EVENT_LOW_FLAG 0x1C
#define ADS7128_REG_EVENT_RGN 0x1E

// Per-Channel Threshold Registers (base addresses)
#define ADS7128_REG_HYSTERESIS_CH0 0x20
#define ADS7128_REG_HIGH_TH_CH0 0x21
#define ADS7128_REG_EVENT_COUNT_CH0 0x22
#define ADS7128_REG_LOW_TH_CH0 0x23

// Statistics Registers (base addresses)
#define ADS7128_REG_MAX_LSB_CH0 0x60
#define ADS7128_REG_MIN_LSB_CH0 0x80
#define ADS7128_REG_RECENT_LSB_CH0 0xA0

// RMS Registers
#define ADS7128_REG_RMS_CFG 0xC0
#define ADS7128_REG_RMS_LSB 0xC1
#define ADS7128_REG_RMS_MSB 0xC2

// GPO Trigger Event Registers (base address)
#define ADS7128_REG_GPO0_TRIG_EVENT_SEL 0xC3

// ZCD and Trigger Registers
#define ADS7128_REG_GPO_VALUE_ZCD_CFG_CH0_CH3 0xE3
#define ADS7128_REG_GPO_VALUE_ZCD_CFG_CH4_CH7 0xE4
#define ADS7128_REG_GPO_ZCD_UPDATE_EN 0xE7
#define ADS7128_REG_GPO_TRIGGER_CFG 0xE9
#define ADS7128_REG_GPO_VALUE_TRIG 0xEB

// SYSTEM_STATUS bits
#define ADS7128_BIT_BOR 0x01
#define ADS7128_BIT_CRC_ERR_IN 0x02
#define ADS7128_BIT_CRC_ERR_FUSE 0x04
#define ADS7128_BIT_OSR_DONE 0x08
#define ADS7128_BIT_RMS_DONE 0x10
#define ADS7128_BIT_I2C_SPEED 0x20
#define ADS7128_BIT_SEQ_STATUS 0x40

// GENERAL_CFG bits
#define ADS7128_BIT_RST 0x01
#define ADS7128_BIT_CAL 0x02
#define ADS7128_BIT_CH_RST 0x04
#define ADS7128_BIT_CNVST 0x08
#define ADS7128_BIT_DWC_EN 0x10
#define ADS7128_BIT_STATS_EN 0x20
#define ADS7128_BIT_CRC_EN 0x40
#define ADS7128_BIT_RMS_EN 0x80

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

  /**
   * @brief Initialize the ADS7128
   * @param addr I2C address (default 0x11)
   * @param wire Pointer to TwoWire instance
   * @return true on success, false on failure
   */
  bool begin(uint8_t addr = ADS7128_DEFAULT_ADDR, TwoWire *wire = &Wire);

  /**
   * @brief Configure a channel as analog input, digital input, or digital
   * output
   * @param channel Channel number (0-7)
   * @param mode Pin mode (see ads7128_pin_mode_t)
   * @return true on success, false on I2C error or invalid channel
   */
  bool pinMode(uint8_t channel, ads7128_pin_mode_t mode);

  /**
   * @brief Set digital output level
   * @param channel Channel number (0-7)
   * @param value true=HIGH, false=LOW
   * @return true on success, false on I2C error
   */
  bool digitalWrite(uint8_t channel, bool value);

  /**
   * @brief Read digital input level
   * @param channel Channel number (0-7)
   * @return Input state (true=HIGH, false=LOW)
   */
  bool digitalRead(uint8_t channel);

  /**
   * @brief Enable or disable CRC on I2C interface
   * @param enable true=enable CRC, false=disable
   * @return true on success, false on I2C error
   */
  bool enableCRC(bool enable);

  /**
   * @brief Check if CRC error detected
   * @return true if CRC_ERR_IN flag is set
   */
  bool getCRCError();

  /**
   * @brief Clear CRC error flag
   * @return true on success, false on I2C error
   */
  bool clearCRCError();

private:
  Adafruit_I2CDevice *_i2c;
  bool _crc_enabled;
  bool _crc_error;

  // Low-level register access (handle opcode protocol + CRC)
  uint8_t _readRegister(uint8_t reg);
  bool _writeRegister(uint8_t reg, uint8_t data);
  bool _setBits(uint8_t reg, uint8_t mask);
  bool _clearBits(uint8_t reg, uint8_t mask);

  // CRC helper - CRC-8-CCITT polynomial 0x07
  uint8_t _crc8(uint8_t *data, uint8_t len);
};

#endif // ADAFRUIT_ADS7128_H
