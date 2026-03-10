/*!
 * @file 05_statistics_test.ino
 * @brief ADS7128 statistics module test (min/max/recent tracking)
 *
 * Hardware setup:
 * - ADS7128 at I2C address 0x10
 * - AVDD/DVDD = 5V
 * - Channels A0–A7 chained with 10K resistors:
 *   A0—10K—A1—10K—A2—10K—A3—10K—A4—10K—A5—10K—A6—10K—A7
 *
 * Test: Verify statistics module tracks min/max/recent values correctly
 * by driving CH0 as GPIO output and reading CH1 as analog input.
 * Changing CH0 from HIGH to LOW changes the voltage on CH1.
 *
 * Test 1: Min/Max tracking across voltage changes
 * Test 2: resetStatistics clears min/max tracking
 * Test 3: getRecent matches analogRead
 */

#include <Adafruit_ADS7128.h>
#include <Wire.h>

Adafruit_ADS7128 ads;

#define ADS7128_ADDR 0x10

uint8_t testsPassed = 0;
uint8_t totalTests = 3;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  Serial.println(F("ADS7128 Statistics Test"));
  Serial.println(F("------------------------"));
  Serial.println();

  Wire.begin();

  if (!ads.begin(ADS7128_ADDR)) {
    Serial.println(F("ERROR: Failed to initialize ADS7128!"));
    while (1) {
      delay(1000);
    }
  }

  // Configure CH0 as push-pull output, CH1 as analog input
  if (!ads.pinMode(0, ADS7128_OUTPUT)) {
    Serial.println(F("ERROR: Failed to set CH0 as output"));
  }
  if (!ads.pinMode(1, ADS7128_ANALOG)) {
    Serial.println(F("ERROR: Failed to set CH1 as analog"));
  }

  // Enable statistics module
  if (!ads.enableStatistics(true)) {
    Serial.println(F("ERROR: Failed to enable statistics"));
  }

  // =========================================================================
  // Test 1: Min/Max tracking across voltage change
  // =========================================================================
  Serial.println(F("Test 1: Min/Max tracking across voltage change"));

  // Reset statistics to start fresh
  ads.resetStatistics();

  // Drive CH0 HIGH
  ads.digitalWrite(0, HIGH);
  delay(10);

  // Take several readings to populate statistics
  uint16_t highReading = 0;
  for (uint8_t i = 0; i < 5; i++) {
    highReading = ads.analogRead(1);
    delay(5);
  }

  float highVoltage = (highReading * 5.0) / 4095.0;
  Serial.print(F("  CH0=HIGH: CH1 reads "));
  Serial.print(highVoltage, 2);
  Serial.println(F(" V"));

  uint16_t maxAfterHigh = ads.getMax(1);
  uint16_t minAfterHigh = ads.getMin(1);
  Serial.print(F("  Max="));
  Serial.print(maxAfterHigh);
  Serial.print(F(" Min="));
  Serial.print(minAfterHigh);
  Serial.println(F(" (should be similar)"));

  // Now drive CH0 LOW
  ads.digitalWrite(0, LOW);
  delay(10);

  // Take several readings to update statistics
  uint16_t lowReading = 0;
  for (uint8_t i = 0; i < 5; i++) {
    lowReading = ads.analogRead(1);
    delay(5);
  }

  float lowVoltage = (lowReading * 5.0) / 4095.0;
  Serial.print(F("  CH0=LOW: CH1 reads "));
  Serial.print(lowVoltage, 2);
  Serial.println(F(" V"));

  uint16_t maxAfterLow = ads.getMax(1);
  uint16_t minAfterLow = ads.getMin(1);
  Serial.print(F("  Max="));
  Serial.print(maxAfterLow);
  Serial.print(F(" Min="));
  Serial.println(minAfterLow);

  int32_t diff1 = (int32_t)maxAfterLow - (int32_t)minAfterLow;
  Serial.print(F("  Max - Min = "));
  Serial.print(diff1);
  Serial.println(F(" counts"));

  Serial.print(F("  "));
  if (diff1 > 1000) {
    Serial.println(F("PASS: Max > Min by >1000 counts"));
    testsPassed++;
  } else {
    Serial.print(F("FAIL: Max - Min = "));
    Serial.print(diff1);
    Serial.println(F(" (expected >1000)"));
  }

  Serial.println();

  // =========================================================================
  // Test 2: resetStatistics clears min/max
  // =========================================================================
  Serial.println(F("Test 2: resetStatistics clears min/max"));

  // Reset statistics
  ads.resetStatistics();

  // Drive CH0 HIGH and take readings (stable high voltage)
  ads.digitalWrite(0, HIGH);
  delay(10);

  for (uint8_t i = 0; i < 5; i++) {
    ads.analogRead(1);
    delay(5);
  }

  uint16_t maxAfterReset = ads.getMax(1);
  uint16_t minAfterReset = ads.getMin(1);
  Serial.println(F("  After reset + CH0=HIGH readings:"));
  Serial.print(F("  Max="));
  Serial.print(maxAfterReset);
  Serial.print(F(" Min="));
  Serial.println(minAfterReset);

  int32_t diff2 = (int32_t)maxAfterReset - (int32_t)minAfterReset;
  Serial.print(F("  Max - Min = "));
  Serial.print(diff2);
  Serial.println(F(" counts"));

  Serial.print(F("  "));
  if (diff2 < 100) {
    Serial.println(F("PASS: Max - Min < 100"));
    testsPassed++;
  } else {
    Serial.print(F("FAIL: Max - Min = "));
    Serial.print(diff2);
    Serial.println(F(" (expected <100)"));
  }

  Serial.println();

  // =========================================================================
  // Test 3: getRecent matches analogRead
  // =========================================================================
  Serial.println(F("Test 3: getRecent matches analogRead"));

  // Take a fresh reading
  uint16_t analogVal = ads.analogRead(1);
  uint16_t recentVal = ads.getRecent(1);

  Serial.print(F("  analogRead="));
  Serial.print(analogVal);
  Serial.print(F(" getRecent="));
  Serial.print(recentVal);

  int32_t diff3 = (int32_t)analogVal - (int32_t)recentVal;
  if (diff3 < 0) {
    diff3 = -diff3;
  }
  Serial.print(F(" diff="));
  Serial.println(diff3);

  Serial.print(F("  "));
  if (diff3 < 20) {
    Serial.println(F("PASS: difference < 20"));
    testsPassed++;
  } else {
    Serial.print(F("FAIL: difference = "));
    Serial.print(diff3);
    Serial.println(F(" (expected <20)"));
  }

  Serial.println();
  Serial.println(F("================================"));
  Serial.print(F("RESULTS: "));
  Serial.print(testsPassed);
  Serial.print(F("/"));
  Serial.print(totalTests);
  Serial.println(F(" tests passed"));
  Serial.println(F("================================"));

  // Clean up: drive CH0 LOW
  ads.digitalWrite(0, LOW);
}

void loop() {
  // Nothing to do in loop
}
