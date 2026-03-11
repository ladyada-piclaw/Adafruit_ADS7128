/*!
 * @file 08_opendrain_test.ino
 * @brief Open-drain GPIO output test for ADS7128
 *
 * Hardware setup:
 * - ADS7128 at default address 0x10
 * - AVDD/DVDD = 5V
 * - Channels chained with 10K resistors: A0—10K—A1—10K—A2—...—A7
 *
 * This test verifies open-drain GPIO output mode works correctly.
 * Open-drain can only sink (pull LOW) or float (high-Z).
 * With the 10K resistor chain, floating pins get pulled to intermediate
 * voltages by the network.
 */

#include <Adafruit_ADS7128.h>
#include <Wire.h>

Adafruit_ADS7128 ads;

uint8_t testsPassed = 0;
uint8_t testsFailed = 0;

float odHighVoltage = 0; // Store for test 3 comparison

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  Serial.println(F("ADS7128 Open-Drain GPIO Test"));
  Serial.println(F("-------------------------------"));

  Wire.begin();

  if (!ads.begin()) {
    Serial.println(F("ERROR: Failed to initialize ADS7128!"));
    while (1) {
      delay(1000);
    }
  }

  // Setup: CH0 push-pull HIGH (pull-up source), CH7 push-pull LOW (ground ref)
  ads.pinMode(0, ADS7128_OUTPUT);
  ads.digitalWrite(0, true);
  ads.pinMode(7, ADS7128_OUTPUT);
  ads.digitalWrite(7, false);
  Serial.println(F("CH0=push-pull HIGH, CH7=push-pull LOW"));
  Serial.println();

  delay(20); // Let signals settle

  // Run tests
  test1_odLow();
  test2_odHigh();
  test3_ppComparison();

  // Print summary
  Serial.println();
  Serial.println(F("================================"));
  Serial.print(F("RESULTS: "));
  Serial.print(testsPassed);
  Serial.println(F("/3 tests passed"));
  Serial.println(F("================================"));
}

void test1_odLow() {
  Serial.println(F("Test 1: Open-drain LOW sinks"));

  // Set CH1 as open-drain output, drive LOW (sinking)
  ads.pinMode(1, ADS7128_OUTPUT_OPENDRAIN);
  ads.digitalWrite(1, false);
  delay(20);

  // Read CH2 as analog - should be pulled low by CH1 sinking
  ads.pinMode(2, ADS7128_ANALOG);
  float voltage = ads.analogReadVoltage(2);

  Serial.print(F("  CH1=OD LOW, CH2 reads: "));
  Serial.print(voltage, 2);
  Serial.println(F(" V"));

  if (voltage < 2.0) {
    Serial.println(F("  PASS: CH2 < 2.0V"));
    testsPassed++;
  } else {
    Serial.println(F("  FAIL: CH2 >= 2.0V"));
    testsFailed++;
  }
  Serial.println();

  // Reset CH1 to analog
  ads.pinMode(1, ADS7128_ANALOG);
}

void test2_odHigh() {
  Serial.println(F("Test 2: Open-drain HIGH floats"));

  // Set CH1 as open-drain output, drive HIGH (floating, high-Z)
  ads.pinMode(1, ADS7128_OUTPUT_OPENDRAIN);
  ads.digitalWrite(1, true);
  delay(20);

  // Read CH2 as analog - with CH1 floating, CH2 is determined by resistor
  // divider
  ads.pinMode(2, ADS7128_ANALOG);
  float voltage = ads.analogReadVoltage(2);
  odHighVoltage = voltage; // Save for test 3

  Serial.print(F("  CH1=OD HIGH (float), CH2 reads: "));
  Serial.print(voltage, 2);
  Serial.println(F(" V"));

  if (voltage > 2.5) {
    Serial.println(F("  PASS: CH2 > 2.5V"));
    testsPassed++;
  } else {
    Serial.println(F("  FAIL: CH2 <= 2.5V"));
    testsFailed++;
  }
  Serial.println();

  // Reset CH1 to analog
  ads.pinMode(1, ADS7128_ANALOG);
}

void test3_ppComparison() {
  Serial.println(F("Test 3: Push-pull vs open-drain"));

  // Set CH1 as push-pull output, drive LOW
  ads.pinMode(1, ADS7128_OUTPUT);
  ads.digitalWrite(1, false);
  delay(20);

  ads.pinMode(2, ADS7128_ANALOG);
  float ppLowVoltage = ads.analogReadVoltage(2);

  Serial.print(F("  CH1=PP LOW,  CH2: "));
  Serial.print(ppLowVoltage, 2);
  Serial.println(F(" V"));

  // Set CH1 as push-pull output, drive HIGH
  ads.digitalWrite(1, true);
  delay(20);

  float ppHighVoltage = ads.analogReadVoltage(2);

  Serial.print(F("  CH1=PP HIGH, CH2: "));
  Serial.print(ppHighVoltage, 2);
  Serial.println(F(" V"));

  Serial.print(F("  CH1=OD HIGH, CH2: "));
  Serial.print(odHighVoltage, 2);
  Serial.println(F(" V (from test 2)"));

  // Push-pull HIGH should drive higher than open-drain HIGH (which floats)
  if (ppHighVoltage > odHighVoltage) {
    Serial.println(F("  PP HIGH > OD HIGH: PASS"));
    testsPassed++;
  } else {
    Serial.println(F("  PP HIGH > OD HIGH: FAIL"));
    testsFailed++;
  }

  // Reset CH1 to analog
  ads.pinMode(1, ADS7128_ANALOG);
}

void loop() {
  // Nothing to do
  delay(1000);
}
