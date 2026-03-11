/*!
 * @file ads7128_fulltest.ino
 * @brief Full test sketch for ADS7128 8-Channel 12-Bit ADC
 *
 * Displays configuration and continuous ADC readings.
 * Twiddle the settings below to explore all features.
 *
 * Written by Limor 'ladyada' Fried with assistance from Claude Code for
 * Adafruit Industries. MIT license.
 */

#include <Adafruit_ADS7128.h>
#include <Wire.h>

// Set this to your AVDD voltage (3.3 or 5.0)
#define ADS_VREF 3.3

Adafruit_ADS7128 ads;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  Serial.println(F("ADS7128 Full Test"));
  Serial.println(F("================="));

  Wire.begin();

  if (!ads.begin()) {
    Serial.println(F("Couldn't find ADS7128!"));
    while (1)
      delay(10);
  }
  Serial.println(F("ADS7128 found!"));

  // === CRC Configuration ===
  Serial.println(F("\n--- CRC ---"));

  ads.enableCRC(false);
  // true  = CRC-8 integrity check on all I2C transfers (slower but safe)
  // false = No CRC (default, faster)
  Serial.print(F("CRC: "));
  Serial.println(ads.getCRCError() ? F("ERROR") : F("OK"));

  // === Oversampling ===
  Serial.println(F("\n--- Oversampling ---"));

  ads.setOversampling(ADS7128_OSR_NONE);
  // Options: ADS7128_OSR_NONE (no averaging, fastest)
  //          ADS7128_OSR_2    (2x averaging)
  //          ADS7128_OSR_4    (4x)
  //          ADS7128_OSR_8    (8x)
  //          ADS7128_OSR_16   (16x)
  //          ADS7128_OSR_32   (32x)
  //          ADS7128_OSR_64   (64x)
  //          ADS7128_OSR_128  (128x, lowest noise, slowest)
  Serial.print(F("OSR: "));
  switch (ads.getOversampling()) {
  case ADS7128_OSR_NONE:
    Serial.println(F("None"));
    break;
  case ADS7128_OSR_2:
    Serial.println(F("2x"));
    break;
  case ADS7128_OSR_4:
    Serial.println(F("4x"));
    break;
  case ADS7128_OSR_8:
    Serial.println(F("8x"));
    break;
  case ADS7128_OSR_16:
    Serial.println(F("16x"));
    break;
  case ADS7128_OSR_32:
    Serial.println(F("32x"));
    break;
  case ADS7128_OSR_64:
    Serial.println(F("64x"));
    break;
  case ADS7128_OSR_128:
    Serial.println(F("128x"));
    break;
  }

  // === Statistics ===
  Serial.println(F("\n--- Statistics ---"));

  ads.enableStatistics(true);
  // true  = Track min/max/recent for each channel
  // false = Disabled (default)
  Serial.println(F("Statistics: Enabled"));
  Serial.println(F("(Use getMin/getMax/getRecent after readings)"));

  // === Pin Configuration ===
  // All channels default to analog input. Configure GPIOs here.
  Serial.println(F("\n--- Pin Configuration ---"));
  Serial.println(F("All channels: analog input (default)"));
  // Uncomment to configure GPIOs:
  // ads.pinMode(0, ADS7128_OUTPUT);          // Push-pull output
  // ads.pinMode(1, ADS7128_INPUT);           // Digital input
  // ads.pinMode(2, ADS7128_OUTPUT_OPENDRAIN); // Open-drain output

  // === Autonomous Sequence (optional) ===
  // Uncomment to use auto-sequence instead of manual reads:
  // ads.setSequenceChannels(0xFF);  // All 8 channels
  // ads.startSequence();

  // === Digital Window Comparator (optional) ===
  // Uncomment to enable threshold alerts:
  // ads.setLowThreshold(0, 1000);   // Alert if CH0 < ~0.8V
  // ads.setHighThreshold(0, 3000);  // Alert if CH0 > ~2.4V
  // ads.enableDWC(true);
  // ads.setAlertChannels(0x01);     // CH0 only
  // ads.configureAlert(true, 0);    // Push-pull, active low

  // === Continuous ADC Readings ===
  Serial.println(F("\n--- ADC Readings ---"));
  Serial.println(F("Chan  Raw    Volts  Min    Max"));
  Serial.println(F("----  ----   -----  ----   ----"));
}

void loop() {
  for (uint8_t ch = 0; ch < 8; ch++) {
    uint16_t raw = ads.analogRead(ch);
    float voltage = ads.analogReadVoltage(ch, ADS_VREF);
    uint16_t minVal = ads.getMin(ch);
    uint16_t maxVal = ads.getMax(ch);

    Serial.print(F("  "));
    Serial.print(ch);
    Serial.print(F("   "));
    if (raw < 1000)
      Serial.print(F(" "));
    if (raw < 100)
      Serial.print(F(" "));
    if (raw < 10)
      Serial.print(F(" "));
    Serial.print(raw);
    Serial.print(F("   "));
    Serial.print(voltage, 2);
    Serial.print(F("  "));
    if (minVal < 1000)
      Serial.print(F(" "));
    if (minVal < 100)
      Serial.print(F(" "));
    if (minVal < 10)
      Serial.print(F(" "));
    Serial.print(minVal);
    Serial.print(F("   "));
    if (maxVal < 1000)
      Serial.print(F(" "));
    if (maxVal < 100)
      Serial.print(F(" "));
    if (maxVal < 10)
      Serial.print(F(" "));
    Serial.println(maxVal);
  }

  Serial.println();
  delay(500);
}
