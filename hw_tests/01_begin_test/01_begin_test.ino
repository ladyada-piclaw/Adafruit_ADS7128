/*!
 * @file 01_begin_test.ino
 * @brief Hardware test: ADS7128 begin() and I2C communication verification
 *
 * Tests:
 * 1. I2C scan for device at 0x10
 * 2. begin() initialization
 * 3. GPIO round-trip (CH0 output -> CH1 input through 10K resistor)
 *
 * Hardware: ADS7128 at 0x10, channels chained with 10K resistors
 */

#include <Adafruit_ADS7128.h>
#include <Wire.h>

Adafruit_ADS7128 ads;

uint8_t tests_passed = 0;
uint8_t tests_total = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  Serial.println(F(""));
  Serial.println(F("ADS7128 Begin + Communication Test"));
  Serial.println(F("-----------------------------------"));

  Wire.begin();

  // Test 1: I2C scan for device
  tests_total++;
  Serial.println(F(""));
  Serial.println(F("Test 1: I2C device present at 0x10"));
  Wire.beginTransmission(0x10);
  uint8_t error = Wire.endTransmission();
  if (error == 0) {
    Serial.println(F("  PASS: Device found"));
    tests_passed++;
  } else {
    Serial.print(F("  FAIL: Device not found (error="));
    Serial.print(error);
    Serial.println(F(")"));
  }

  // Test 2: begin() succeeds
  tests_total++;
  Serial.println(F(""));
  Serial.println(F("Test 2: begin() succeeds"));
  bool beginResult = ads.begin();
  if (beginResult) {
    Serial.println(F("  PASS: begin() returned true"));
    tests_passed++;
  } else {
    Serial.println(F("  FAIL: begin() returned false"));
  }

  // Test 3: GPIO round-trip (CH0 out -> CH1 in)
  Serial.println(F(""));
  Serial.println(F("Test 3: GPIO round-trip (CH0 out -> CH1 in)"));

  // Configure CH0 as push-pull output, CH1 as digital input
  ads.pinMode(0, ADS7128_OUTPUT);
  ads.pinMode(1, ADS7128_INPUT);

  // Test HIGH
  tests_total++;
  ads.digitalWrite(0, true);
  delay(10); // Allow signal to settle through resistor
  bool ch1_high = ads.digitalRead(1);
  Serial.print(F("  CH0=HIGH, CH1="));
  Serial.print(ch1_high ? F("HIGH") : F("LOW"));
  Serial.print(F(" ... "));
  if (ch1_high) {
    Serial.println(F("PASS"));
    tests_passed++;
  } else {
    Serial.println(F("FAIL"));
  }

  // Test LOW
  tests_total++;
  ads.digitalWrite(0, false);
  delay(10); // Allow signal to settle
  bool ch1_low = ads.digitalRead(1);
  Serial.print(F("  CH0=LOW, CH1="));
  Serial.print(ch1_low ? F("HIGH") : F("LOW"));
  Serial.print(F(" ... "));
  if (!ch1_low) {
    Serial.println(F("PASS"));
    tests_passed++;
  } else {
    Serial.println(F("FAIL"));
  }

  // Summary
  Serial.println(F(""));
  Serial.println(F("================================"));
  Serial.print(F("RESULTS: "));
  Serial.print(tests_passed);
  Serial.print(F("/"));
  Serial.print(tests_total);
  Serial.println(F(" tests passed"));
  Serial.println(F("================================"));
}

void loop() {
  // Nothing to do
}
