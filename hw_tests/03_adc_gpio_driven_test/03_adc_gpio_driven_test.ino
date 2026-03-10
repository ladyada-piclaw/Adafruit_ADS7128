/*!
 * @file 03_adc_gpio_driven_test.ino
 * @brief ADS7128 ADC test with GPIO-driven voltage divider
 *
 * Hardware setup:
 * - ADS7128 at I2C address 0x10
 * - AVDD/DVDD = 5V
 * - Channels A0–A7 chained with 10K resistors:
 *   A0—10K—A1—10K—A2—10K—A3—10K—A4—10K—A5—10K—A6—10K—A7
 *
 * Test: Configure CH0 and CH7 as GPIO outputs, CH1-CH6 as analog inputs.
 * Drive CH0=HIGH, CH7=LOW creates a voltage divider.
 * Readings should be monotonically decreasing from CH1 to CH6.
 * Then flip to CH0=LOW, CH7=HIGH and readings should be monotonically
 * increasing.
 *
 * Self-calibrating: no hardcoded expected voltages, just monotonic checks
 * and boundary sanity checks.
 */

#include <Adafruit_ADS7128.h>
#include <Wire.h>

Adafruit_ADS7128 ads;

#define ADS7128_ADDR 0x10

uint8_t testsPassed = 0;
uint8_t totalTests = 6;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  Serial.println(F("ADS7128 ADC GPIO-Driven Voltage Divider Test"));
  Serial.println(F("----------------------------------------------"));
  Serial.println();

  Wire.begin();

  if (!ads.begin(ADS7128_ADDR)) {
    Serial.println(F("ERROR: Failed to initialize ADS7128!"));
    while (1) {
      delay(1000);
    }
  }

  // Configure CH0 as push-pull output
  if (!ads.pinMode(0, ADS7128_OUTPUT)) {
    Serial.println(F("ERROR: Failed to set CH0 as output"));
  }

  // Configure CH7 as push-pull output
  if (!ads.pinMode(7, ADS7128_OUTPUT)) {
    Serial.println(F("ERROR: Failed to set CH7 as output"));
  }

  // Configure CH1-CH6 as analog inputs
  for (uint8_t ch = 1; ch <= 6; ch++) {
    if (!ads.pinMode(ch, ADS7128_ANALOG)) {
      Serial.print(F("ERROR: Failed to set CH"));
      Serial.print(ch);
      Serial.println(F(" as analog"));
    }
  }

  // =========================================================================
  // Test 1: CH0=HIGH(5V), CH7=LOW(GND)
  // =========================================================================
  Serial.println(F("Test 1: CH0=HIGH(5V), CH7=LOW(GND)"));

  ads.digitalWrite(0, HIGH);
  ads.digitalWrite(7, LOW);
  delay(50); // Allow voltage to settle

  float voltages1[6];
  for (uint8_t i = 0; i < 6; i++) {
    uint8_t ch = i + 1;
    uint16_t raw = ads.analogRead(ch);
    voltages1[i] = ads.analogReadVoltage(ch, 5.0);
    Serial.print(F("  CH"));
    Serial.print(ch);
    Serial.print(F(": "));
    Serial.print(raw);
    Serial.print(F(" ("));
    Serial.print(voltages1[i], 2);
    Serial.println(F(" V)"));
  }

  // Check monotonically decreasing (CH1 > CH2 > CH3 > CH4 > CH5 > CH6)
  bool mono1 = true;
  for (uint8_t i = 0; i < 5; i++) {
    if (voltages1[i] <= voltages1[i + 1]) {
      mono1 = false;
      break;
    }
  }
  Serial.print(F("  Monotonically decreasing: "));
  if (mono1) {
    Serial.println(F("PASS"));
    testsPassed++;
  } else {
    Serial.println(F("FAIL"));
  }

  // Check CH1 > 3.0V
  Serial.print(F("  CH1 > 3.0V: "));
  if (voltages1[0] > 3.0) {
    Serial.println(F("PASS"));
    testsPassed++;
  } else {
    Serial.println(F("FAIL"));
  }

  // Check CH6 < 2.0V
  Serial.print(F("  CH6 < 2.0V: "));
  if (voltages1[5] < 2.0) {
    Serial.println(F("PASS"));
    testsPassed++;
  } else {
    Serial.println(F("FAIL"));
  }

  Serial.println();

  // =========================================================================
  // Test 2: CH0=LOW(GND), CH7=HIGH(5V)
  // =========================================================================
  Serial.println(F("Test 2: CH0=LOW(GND), CH7=HIGH(5V)"));

  ads.digitalWrite(0, LOW);
  ads.digitalWrite(7, HIGH);
  delay(50); // Allow voltage to settle

  float voltages2[6];
  for (uint8_t i = 0; i < 6; i++) {
    uint8_t ch = i + 1;
    uint16_t raw = ads.analogRead(ch);
    voltages2[i] = ads.analogReadVoltage(ch, 5.0);
    Serial.print(F("  CH"));
    Serial.print(ch);
    Serial.print(F(": "));
    Serial.print(raw);
    Serial.print(F(" ("));
    Serial.print(voltages2[i], 2);
    Serial.println(F(" V)"));
  }

  // Check monotonically increasing (CH1 < CH2 < CH3 < CH4 < CH5 < CH6)
  bool mono2 = true;
  for (uint8_t i = 0; i < 5; i++) {
    if (voltages2[i] >= voltages2[i + 1]) {
      mono2 = false;
      break;
    }
  }
  Serial.print(F("  Monotonically increasing: "));
  if (mono2) {
    Serial.println(F("PASS"));
    testsPassed++;
  } else {
    Serial.println(F("FAIL"));
  }

  // Check CH1 < 2.0V
  Serial.print(F("  CH1 < 2.0V: "));
  if (voltages2[0] < 2.0) {
    Serial.println(F("PASS"));
    testsPassed++;
  } else {
    Serial.println(F("FAIL"));
  }

  // Check CH6 > 3.0V
  Serial.print(F("  CH6 > 3.0V: "));
  if (voltages2[5] > 3.0) {
    Serial.println(F("PASS"));
    testsPassed++;
  } else {
    Serial.println(F("FAIL"));
  }

  Serial.println();
  Serial.println(F("================================"));
  Serial.print(F("RESULTS: "));
  Serial.print(testsPassed);
  Serial.print(F("/"));
  Serial.print(totalTests);
  Serial.println(F(" tests passed"));
  Serial.println(F("================================"));

  // Turn off outputs
  ads.digitalWrite(0, LOW);
  ads.digitalWrite(7, LOW);
}

void loop() {
  // Nothing to do in loop
}
