/*!
 * @file 10_zcd_test.ino
 * @brief ADS7128 Zero-Crossing Detection (ZCD) Test
 *
 * Hardware setup:
 * - ADS7128 at default address 0x10
 * - Arduino pin 10 → ADS7128 CH0 (fast PWM ~31kHz source)
 * - CH1 configured as GPO for ZCD output
 * - AVDD = 5V
 *
 * Tests:
 * 1. ZCD basic detection - verify ZCD toggles with PWM input
 * 2. ZCD blanking - verify high blanking reduces transitions
 * 3. ZCD disabled - verify GPO works normally when ZCD disabled
 */

#include <Adafruit_ADS7128.h>
#include <Wire.h>

Adafruit_ADS7128 ads;

#define PWM_PIN 10

uint8_t testsRun = 0;
uint8_t testsPassed = 0;

void setupFastPWM() {
  // Configure Timer1 for fast PWM ~31kHz on pin 10 (OC1B)
  ::pinMode(PWM_PIN, OUTPUT);
  TCCR1A = _BV(COM1B1) | _BV(WGM10); // Fast PWM 8-bit, clear OC1B on match
  TCCR1B = _BV(WGM12) | _BV(CS10);   // Fast PWM, no prescaler
  OCR1B = 128;                       // 50% duty cycle
}

void stopPWM() {
  TCCR1A = 0;
  TCCR1B = 0;
  ::pinMode(PWM_PIN, INPUT);
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println(F("ADS7128 ZCD Test"));
  Serial.println(F("------------------"));
  Serial.println();

  Wire.begin();

  if (!ads.begin()) {
    Serial.println(F("ERROR: ADS7128 not found!"));
    while (1)
      ;
  }

  // Start PWM on pin 10
  setupFastPWM();
  delay(10);

  // ==========================================================================
  // Test 1: ZCD basic detection
  // ==========================================================================
  Serial.println(F("Test 1: ZCD basic detection"));
  testsRun++;

  // Configure CH0 as analog input for ZCD
  ads.pinMode(0, ADS7128_ANALOG);

  // Configure CH1 as digital output for ZCD output
  ads.pinMode(1, ADS7128_OUTPUT);

  // Set up ZCD: threshold at mid-scale for 50% PWM
  ads.enableDWC(true);
  ads.setHighThreshold(0, 2048); // ZCD triggers on this threshold
  ads.setLowThreshold(0, 2048);

  // Set ZCD to monitor CH0
  ads.setZCDChannel(0);

  // Map ZCD output to CH1 (mode 2 = ZCD signal)
  ads.setZCDOutput(1, 2);
  ads.enableZCDOutput(1, true);

  // Enable statistics and start autonomous sequence on CH0
  ads.enableStatistics(true);
  ads.setSequenceChannels(0x01); // CH0 only
  ads.startSequence();

  delay(10);

  // Read ZCD output 100 times, expect both HIGH and LOW
  bool sawHigh = false;
  bool sawLow = false;
  for (uint8_t i = 0; i < 100; i++) {
    bool val = ads.digitalRead(1);
    if (val)
      sawHigh = true;
    else
      sawLow = true;
    delayMicroseconds(100);
  }

  Serial.print(F("  Saw HIGH: "));
  Serial.print(sawHigh ? F("yes") : F("no"));
  Serial.print(F(", LOW: "));
  Serial.print(sawLow ? F("yes") : F("no"));

  if (sawHigh && sawLow) {
    Serial.println(F(" ... PASS"));
    testsPassed++;
  } else {
    Serial.println(F(" ... FAIL"));
  }
  Serial.println();

  // ==========================================================================
  // Test 2: ZCD blanking
  // ==========================================================================
  Serial.println(F("Test 2: ZCD blanking"));
  testsRun++;

  // First, high blanking (127 * 8 = 1016 conversions)
  ads.setZCDBlanking(127, true);
  delay(10);

  uint8_t highBlankingTransitions = 0;
  bool lastVal = ads.digitalRead(1);
  for (uint8_t i = 0; i < 100; i++) {
    bool val = ads.digitalRead(1);
    if (val != lastVal) {
      highBlankingTransitions++;
      lastVal = val;
    }
    delayMicroseconds(100);
  }

  Serial.print(F("  High blanking transitions: "));
  Serial.println(highBlankingTransitions);

  // Now low blanking (0)
  ads.setZCDBlanking(0, false);
  delay(10);

  uint8_t lowBlankingTransitions = 0;
  lastVal = ads.digitalRead(1);
  for (uint8_t i = 0; i < 100; i++) {
    bool val = ads.digitalRead(1);
    if (val != lastVal) {
      lowBlankingTransitions++;
      lastVal = val;
    }
    delayMicroseconds(100);
  }

  Serial.print(F("  Low blanking transitions: "));
  Serial.println(lowBlankingTransitions);

  // High blanking should have fewer or equal transitions
  Serial.print(F("  High blanking <= low blanking: "));
  if (highBlankingTransitions <= lowBlankingTransitions) {
    Serial.println(F("yes ... PASS"));
    testsPassed++;
  } else {
    Serial.println(F("no ... FAIL"));
  }
  Serial.println();

  // ==========================================================================
  // Test 3: ZCD disabled
  // ==========================================================================
  Serial.println(F("Test 3: ZCD disabled"));
  testsRun++;

  // Stop sequence, disable DWC and ZCD output
  ads.stopSequence();
  ads.enableDWC(false);
  ads.enableZCDOutput(1, false);

  // Configure CH1 as plain output, set HIGH
  ads.pinMode(1, ADS7128_OUTPUT);
  ads.digitalWrite(1, true);
  delay(1);

  bool val = ads.digitalRead(1);
  Serial.print(F("  CH1 as plain GPIO HIGH: "));
  Serial.print(val ? F("HIGH") : F("LOW"));

  if (val) {
    Serial.println(F(" ... PASS"));
    testsPassed++;
  } else {
    Serial.println(F(" ... FAIL"));
  }

  // Verify it stays HIGH (not being overridden by ZCD)
  testsRun++;
  delay(10);
  bool stillHigh = true;
  for (uint8_t i = 0; i < 50; i++) {
    if (!ads.digitalRead(1)) {
      stillHigh = false;
      break;
    }
    delayMicroseconds(100);
  }

  Serial.print(F("  Stays HIGH (ZCD not overriding): "));
  if (stillHigh) {
    Serial.println(F("yes ... PASS"));
    testsPassed++;
  } else {
    Serial.println(F("no ... FAIL"));
  }

  // Cleanup
  stopPWM();

  Serial.println();
  Serial.println(F("================================"));
  Serial.print(F("RESULTS: "));
  Serial.print(testsPassed);
  Serial.print(F("/"));
  Serial.print(testsRun);
  Serial.println(F(" tests passed"));
  Serial.println(F("================================"));
}

void loop() {
  // Nothing to do
}
