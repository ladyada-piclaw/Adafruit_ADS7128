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

// SEQUENCE_CFG bits
#define ADS7128_BIT_SEQ_START 0x10
#define ADS7128_BIT_SEQ_MODE 0x01

// OPMODE_CFG bits
#define ADS7128_BIT_CONV_MODE 0x20

// DATA_CFG bits
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

  /**
   * @brief Initialize the ADS7128
   * @param addr I2C address (default 0x10)
   * @param wire Pointer to TwoWire instance
   * @return true on success, false on failure
   */
  bool begin(uint8_t addr = ADS7128_DEFAULT_ADDR, TwoWire *wire = &Wire);

  // -------------------------------------------------------------------------
  // GPIO Functions (Phase 1)
  // -------------------------------------------------------------------------

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

  // -------------------------------------------------------------------------
  // CRC Functions
  // -------------------------------------------------------------------------

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

  // -------------------------------------------------------------------------
  // ADC Functions - Manual Mode (Phase 2)
  // -------------------------------------------------------------------------

  /**
   * @brief Read ADC value from a channel in manual mode
   * @param channel Channel number (0-7)
   * @return 12-bit ADC value (0-4095), or 0xFFFF on error
   */
  uint16_t analogRead(uint8_t channel);

  /**
   * @brief Read ADC value and convert to voltage
   * @param channel Channel number (0-7)
   * @param vref Reference voltage (default 5.0V)
   * @return Voltage in volts, or -1.0 on error
   */
  float analogReadVoltage(uint8_t channel, float vref = 5.0);

  // -------------------------------------------------------------------------
  // ADC Functions - Auto-Sequence Mode (Phase 2)
  // -------------------------------------------------------------------------

  /**
   * @brief Set channels to include in auto-sequence
   * @param channelMask Bitmask of channels (bit 0 = CH0, etc.)
   * @return true on success, false on I2C error
   */
  bool setSequenceChannels(uint8_t channelMask);

  /**
   * @brief Start auto-sequence mode
   * @return true on success, false on I2C error
   */
  bool startSequence();

  /**
   * @brief Stop auto-sequence mode
   * @return true on success, false on I2C error
   */
  bool stopSequence();

  /**
   * @brief Read next conversion result from sequence
   * @param channel Pointer to store channel ID (optional, can be nullptr)
   * @return 12-bit ADC value (0-4095), or 0xFFFF on error
   */
  uint16_t readSequenceResult(uint8_t *channel = nullptr);

  // -------------------------------------------------------------------------
  // Oversampling Configuration (Phase 2)
  // -------------------------------------------------------------------------

  /**
   * @brief Set oversampling ratio
   * @param osr Oversampling ratio (see ads7128_osr_t)
   * @return true on success, false on I2C error
   */
  bool setOversampling(ads7128_osr_t osr);

  /**
   * @brief Get current oversampling ratio
   * @return Current oversampling ratio setting
   */
  ads7128_osr_t getOversampling();

  // -------------------------------------------------------------------------
  // Statistics Functions (Phase 2)
  // -------------------------------------------------------------------------

  /**
   * @brief Enable or disable statistics module (min/max/recent tracking)
   * @param enable true to enable, false to disable
   * @return true on success, false on I2C error
   */
  bool enableStatistics(bool enable);

  /**
   * @brief Get maximum recorded value for a channel
   * @param channel Channel number (0-7)
   * @return 12-bit max value, or 0xFFFF on error
   */
  uint16_t getMax(uint8_t channel);

  /**
   * @brief Get minimum recorded value for a channel
   * @param channel Channel number (0-7)
   * @return 12-bit min value, or 0xFFFF on error
   */
  uint16_t getMin(uint8_t channel);

  /**
   * @brief Get most recent conversion value for a channel
   * @param channel Channel number (0-7)
   * @return 12-bit recent value, or 0xFFFF on error
   */
  uint16_t getRecent(uint8_t channel);

  /**
   * @brief Reset all statistics (min/max/recent)
   * @return true on success, false on I2C error
   */
  bool resetStatistics();

  // -------------------------------------------------------------------------
  // Digital Window Comparator (Phase 2)
  // -------------------------------------------------------------------------

  /**
   * @brief Enable or disable digital window comparator
   * @param enable true to enable, false to disable
   * @return true on success, false on I2C error
   */
  bool enableDWC(bool enable);

  /**
   * @brief Set high threshold for a channel
   * @param channel Channel number (0-7)
   * @param threshold 12-bit threshold value
   * @return true on success, false on I2C error
   */
  bool setHighThreshold(uint8_t channel, uint16_t threshold);

  /**
   * @brief Set low threshold for a channel
   * @param channel Channel number (0-7)
   * @param threshold 12-bit threshold value
   * @return true on success, false on I2C error
   */
  bool setLowThreshold(uint8_t channel, uint16_t threshold);

  /**
   * @brief Set hysteresis for a channel
   * @param channel Channel number (0-7)
   * @param hysteresis 4-bit hysteresis value (0-15)
   * @return true on success, false on I2C error
   */
  bool setHysteresis(uint8_t channel, uint8_t hysteresis);

  /**
   * @brief Set event count for a channel (consecutive samples before alert)
   * @param channel Channel number (0-7)
   * @param count 4-bit event count (0-15, alert after count+1 samples)
   * @return true on success, false on I2C error
   */
  bool setEventCount(uint8_t channel, uint8_t count);

  /**
   * @brief Get event flags for all channels
   * @return 8-bit event flags (bit per channel)
   */
  uint8_t getEventFlags();

  /**
   * @brief Get high threshold event flags
   * @return 8-bit event flags (bit per channel)
   */
  uint8_t getEventHighFlags();

  /**
   * @brief Get low threshold event flags
   * @return 8-bit event flags (bit per channel)
   */
  uint8_t getEventLowFlags();

  /**
   * @brief Clear all event flags
   * @return true on success, false on I2C error
   */
  bool clearEventFlags();

  // -------------------------------------------------------------------------
  // ALERT Pin Configuration (Phase 2)
  // -------------------------------------------------------------------------

  /**
   * @brief Configure ALERT pin output mode and logic
   * @param pushPull true for push-pull, false for open-drain
   * @param logic 0=active low, 1=active high, 2=pulsed low, 3=pulsed high
   * @return true on success, false on I2C error
   */
  bool configureAlert(bool pushPull, uint8_t logic);

  /**
   * @brief Set which channels trigger ALERT pin
   * @param channelMask Bitmask of channels (bit 0 = CH0, etc.)
   * @return true on success, false on I2C error
   */
  bool setAlertChannels(uint8_t channelMask);

  // -------------------------------------------------------------------------
  // Sampling Rate (Phase 2)
  // -------------------------------------------------------------------------

  /**
   * @brief Set sampling rate for autonomous mode
   * @param slowOsc true for low-power oscillator, false for high-speed
   * @param divider Clock divider (0-7)
   * @return true on success, false on I2C error
   */
  bool setSamplingRate(bool slowOsc, uint8_t divider);

  // -------------------------------------------------------------------------
  // Zero-Crossing Detection (ZCD)
  // -------------------------------------------------------------------------

  /**
   * @brief Set which analog channel the ZCD module monitors
   * @param channel Channel number (0-7)
   * @return true on success, false on I2C error or invalid channel
   */
  bool setZCDChannel(uint8_t channel);

  /**
   * @brief Configure ZCD blanking (transient rejection)
   * @param count Blanking count (0-127, conversions to skip after ZCD event)
   * @param multiply true = count x8, false = count x1
   * @return true on success, false on I2C error
   */
  bool setZCDBlanking(uint8_t count, bool multiply = false);

  /**
   * @brief Map ZCD output to a GPO pin
   * @param gpoChannel GPO channel (0-7, must be configured as digital output)
   * @param mode 0=low, 1=high, 2=ZCD signal, 3=inverted ZCD
   * @return true on success, false on I2C error or invalid channel
   */
  bool setZCDOutput(uint8_t gpoChannel, uint8_t mode);

  /**
   * @brief Enable or disable ZCD-to-GPO updates for a channel
   * @param gpoChannel GPO channel (0-7)
   * @param enable true to enable ZCD updates on this GPO
   * @return true on success, false on I2C error
   */
  bool enableZCDOutput(uint8_t gpoChannel, bool enable);

private:
  Adafruit_I2CDevice *_i2c;
  bool _crc_enabled;
  bool _crc_error;

  // Low-level register access (handle opcode protocol + CRC)
  uint8_t _readRegister(uint8_t reg);
  bool _writeRegister(uint8_t reg, uint8_t data);
  bool _setBits(uint8_t reg, uint8_t mask);
  bool _clearBits(uint8_t reg, uint8_t mask);
  bool _readBlock(uint8_t startReg, uint8_t *buf, uint8_t len);

  // Helper to read 12-bit value from LSB/MSB register pair
  uint16_t _read12BitValue(uint8_t lsbReg);

  // CRC helper - CRC-8-CCITT polynomial 0x07 over a single byte
  uint8_t _crc8(uint8_t byte);
};

#endif // ADAFRUIT_ADS7128_H
