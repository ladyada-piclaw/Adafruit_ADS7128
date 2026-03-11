/*!
 * @file 06_autonomous_test.ino
 * @brief ADS7128 autonomous (auto-sequence) mode test
 *
 * Hardware setup:
 * - ADS7128 at default I2C address
 * - AVDD/DVDD = 5V
 * - Channels A0–A7 chained with 10K resistors:
 *   A0—10K—A1—10K—A2—10K—A3—10K—A4—10K—A5—10K—A6—10K—A7
 *
 * Test: Verify autonomous mode (internal sequencer) works by running
 * the sequencer across multiple channels and reading results.
 *
 * Setup: CH0 = push-pull HIGH, CH7 = push-pull LOW
 * This creates a voltage ramp on CH1-CH6 via resistor divider.
 *
 * Test 1: Auto-sequence reads all analog channels CH1-CH6
 * Test 2: Sequence results match manual reads
 * Test 3: Changing sequence channels (CH2 + CH5 only)
 */

#include <Adafruit_ADS7128.h>
#include <Wire.h>

Adafruit_ADS7128 ads;


uint8_t testsPassed = 0;
uint8_t totalTests = 3;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  Serial.println(F("ADS7128 Autonomous Sequence Test"));
  Serial.println(F("----------------------------------"));

  Wire.begin();

  if (!ads.begin()) {
    Serial.println(F("ERROR: Failed to initialize ADS7128!"));
    while (1) {
      delay(1000);
    }
  }

  // Configure CH0 as push-pull output HIGH
  if (!ads.pinMode(0, ADS7128_OUTPUT)) {
    Serial.println(F("ERROR: Failed to set CH0 as output"));
  }
  ads.digitalWrite(0, HIGH);

  // Configure CH7 as push-pull output LOW
  if (!ads.pinMode(7, ADS7128_OUTPUT)) {
    Serial.println(F("ERROR: Failed to set CH7 as output"));
  }
  ads.digitalWrite(7, LOW);

  // Configure CH1-CH6 as analog inputs
  for (uint8_t ch = 1; ch <= 6; ch++) {
    if (!ads.pinMode(ch, ADS7128_ANALOG)) {
      Serial.print(F("ERROR: Failed to set CH"));
      Serial.print(ch);
      Serial.println(F(" as analog"));
    }
  }

  // Enable statistics module (needed for getRecent)
  if (!ads.enableStatistics(true)) {
    Serial.println(F("ERROR: Failed to enable statistics"));
  }

  Serial.println(F("CH0=HIGH, CH7=LOW (voltage divider on CH1-CH6)"));
  Serial.println();

  // Allow voltages to settle
  delay(50);

  // =========================================================================
  // Test 1: Auto-sequence reads all analog channels CH1-CH6
  // =========================================================================
  Serial.println(F("Test 1: Auto-sequence CH1-CH6"));

  // Set sequence to CH1-CH6 (bits 1-6 = 0x7E)
  if (!ads.setSequenceChannels(0x7E)) {
    Serial.println(F("  ERROR: setSequenceChannels failed"));
  }

  // Start sequence
  if (!ads.startSequence()) {
    Serial.println(F("  ERROR: startSequence failed"));
  }

  // Wait for conversions to complete
  delay(100);

  // Stop sequence
  if (!ads.stopSequence()) {
    Serial.println(F("  ERROR: stopSequence failed"));
  }

  // Read getRecent() for each channel CH1-CH6
  uint16_t seqValues[6];
  float seqVoltages[6];
  for (uint8_t i = 0; i < 6; i++) {
    seqValues[i] = ads.getRecent(i + 1);
    seqVoltages[i] = (seqValues[i] * 5.0) / 4095.0;
    Serial.print(F("  CH"));
    Serial.print(i + 1);
    Serial.print(F(": "));
    Serial.print(seqVoltages[i], 2);
    Serial.println(F(" V"));
  }

  // Check monotonically decreasing
  bool monotonic = true;
  for (uint8_t i = 0; i < 5; i++) {
    if (seqValues[i] <= seqValues[i + 1]) {
      monotonic = false;
      break;
    }
  }
  Serial.print(F("  Monotonically decreasing: "));
  Serial.println(monotonic ? F("PASS") : F("FAIL"));

  // Check CH1 > 3.0V
  bool ch1High = (seqVoltages[0] > 3.0);
  Serial.print(F("  CH1 > 3.0V: "));
  Serial.println(ch1High ? F("PASS") : F("FAIL"));

  // Check CH6 < 2.0V
  bool ch6Low = (seqVoltages[5] < 2.0);
  Serial.print(F("  CH6 < 2.0V: "));
  Serial.println(ch6Low ? F("PASS") : F("FAIL"));

  if (monotonic && ch1High && ch6Low) {
    testsPassed++;
  }

  Serial.println();

  // =========================================================================
  // Test 2: Sequence results match manual reads
  // =========================================================================
  Serial.println(F("Test 2: Sequence results match manual reads"));

  // Read each channel manually with analogRead()
  uint16_t manualValues[6];
  bool allWithinTolerance = true;

  for (uint8_t i = 0; i < 6; i++) {
    manualValues[i] = ads.analogRead(i + 1);
    int32_t diff = (int32_t)seqValues[i] - (int32_t)manualValues[i];
    if (diff < 0) {
      diff = -diff;
    }

    Serial.print(F("  CH"));
    Serial.print(i + 1);
    Serial.print(F(": seq="));
    Serial.print(seqValues[i]);
    Serial.print(F(" manual="));
    Serial.print(manualValues[i]);
    Serial.print(F(" diff="));
    Serial.print(diff);
    Serial.print(F(" ... "));

    if (diff <= 100) {
      Serial.println(F("PASS"));
    } else {
      Serial.println(F("FAIL"));
      allWithinTolerance = false;
    }
  }

  Serial.print(F("  All within tolerance: "));
  Serial.println(allWithinTolerance ? F("PASS") : F("FAIL"));

  if (allWithinTolerance) {
    testsPassed++;
  }

  Serial.println();

  // =========================================================================
  // Test 3: Changing sequence channels (CH2 + CH5 only)
  // =========================================================================
  Serial.println(F("Test 3: Selective sequence (CH2 + CH5 only)"));

  // Set sequence to only CH2 and CH5: bits 2 and 5 = 0x24
  if (!ads.setSequenceChannels(0x24)) {
    Serial.println(F("  ERROR: setSequenceChannels failed"));
  }

  // Start sequence, wait, stop
  if (!ads.startSequence()) {
    Serial.println(F("  ERROR: startSequence failed"));
  }
  delay(50);
  if (!ads.stopSequence()) {
    Serial.println(F("  ERROR: stopSequence failed"));
  }

  // Get recent values for CH2 and CH5
  uint16_t ch2Val = ads.getRecent(2);
  uint16_t ch5Val = ads.getRecent(5);
  float ch2V = (ch2Val * 5.0) / 4095.0;
  float ch5V = (ch5Val * 5.0) / 4095.0;

  Serial.print(F("  CH2: "));
  Serial.print(ch2V, 2);
  Serial.print(F(" V  CH5: "));
  Serial.print(ch5V, 2);
  Serial.println(F(" V"));

  // Verify both are non-zero and CH2 > CH5
  bool test3Pass = (ch2Val > 0) && (ch5Val > 0) && (ch2Val > ch5Val);
  Serial.print(F("  CH2 > CH5: "));
  Serial.println(test3Pass ? F("PASS") : F("FAIL"));

  if (test3Pass) {
    testsPassed++;
  }

  Serial.println();
  Serial.println(F("================================"));
  Serial.print(F("RESULTS: "));
  Serial.print(testsPassed);
  Serial.print(F("/"));
  Serial.print(totalTests);
  Serial.println(F(" tests passed"));
  Serial.println(F("================================"));

  // Clean up: restore all to analog and drive GPIOs LOW
  ads.stopSequence();
  ads.digitalWrite(0, LOW);
  ads.digitalWrite(7, LOW);
}

void loop() {
  // Nothing to do in loop
}
