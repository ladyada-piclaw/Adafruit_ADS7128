/*!
 * @file 11_rms_test.ino
 * @brief ADS7128 RMS Module Test
 *
 * Hardware setup:
 * - ADS7128 at default address 0x10
 * - Arduino pin 10 → ADS7128 CH0 (fast PWM ~31kHz source)
 * - AVDD = 5V
 *
 * Tests:
 * 1. RMS basic computation - verify RMS value in expected range for 50% PWM
 * 2. RMS with DC subtraction - verify DC-sub RMS < plain RMS
 * 3. RMS different duty cycles - verify 25% < 75% duty RMS
 */

#include <Adafruit_ADS7128.h>
#include <Wire.h>

Adafruit_ADS7128 ads;

#define PWM_PIN 10
#define RMS_TIMEOUT_MS 5000

uint8_t testsRun = 0;
uint8_t testsPassed = 0;

void setupFastPWM(uint8_t duty) {
  // Configure Timer1 for fast PWM ~31kHz on pin 10 (OC1B)
  ::pinMode(PWM_PIN, OUTPUT);
  TCCR1A = _BV(COM1B1) | _BV(WGM10); // Fast PWM 8-bit, clear OC1B on match
  TCCR1B = _BV(WGM12) | _BV(CS10);   // Fast PWM, no prescaler
  OCR1B = duty;
}

void stopPWM() {
  TCCR1A = 0;
  TCCR1B = 0;
  ::pinMode(PWM_PIN, INPUT);
}

// Helper to run RMS computation and wait for result
uint16_t runRMSComputation(bool dcSub) {
  // Disable RMS first to reset
  ads.enableRMS(false);
  ads.stopSequence();
  delay(5);

  // Configure RMS
  ads.setRMSChannel(0);
  ads.setRMSSamples(0); // 1024 samples (fastest)
  ads.setRMSDCSub(dcSub);

  // Enable statistics and start sequence
  ads.enableStatistics(true);
  ads.setSequenceChannels(0x01); // CH0 only
  ads.startSequence();
  delay(1);

  // Enable RMS (starts computation)
  ads.enableRMS(true);

  // Poll for completion
  unsigned long start = millis();
  while (!ads.isRMSDone()) {
    if (millis() - start > RMS_TIMEOUT_MS) {
      Serial.println(F("  ERROR: RMS timeout!"));
      return 0xFFFF;
    }
    delay(1);
  }

  return ads.getRMS();
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println(F("ADS7128 RMS Test"));
  Serial.println(F("------------------"));
  Serial.println();

  Wire.begin();

  if (!ads.begin()) {
    Serial.println(F("ERROR: ADS7128 not found!"));
    while (1)
      ;
  }

  // Configure CH0 as analog input
  ads.pinMode(0, ADS7128_ANALOG);

  // Start 50% PWM on pin 10
  setupFastPWM(128);
  delay(10);

  // ==========================================================================
  // Test 1: RMS basic computation
  // ==========================================================================
  Serial.println(F("Test 1: RMS basic computation (50% duty)"));
  testsRun++;

  uint16_t rms50 = runRMSComputation(false);

  Serial.print(F("  RMS value: "));
  Serial.println(rms50);

  // For 50% duty square wave, RMS ≈ 0.707 * Vpeak
  // 16-bit scale: expected ~46340, accept 30000-60000
  Serial.print(F("  Expected range 30000-60000: "));
  if (rms50 >= 30000 && rms50 <= 60000) {
    Serial.println(F("yes ... PASS"));
    testsPassed++;
  } else {
    Serial.println(F("no ... FAIL"));
  }
  Serial.println();

  // ==========================================================================
  // Test 2: RMS with DC subtraction
  // ==========================================================================
  Serial.println(F("Test 2: RMS with DC subtraction"));
  testsRun++;

  // Get plain RMS
  uint16_t rmsPlain = runRMSComputation(false);
  Serial.print(F("  Plain RMS: "));
  Serial.println(rmsPlain);

  // Get DC-subtracted RMS
  uint16_t rmsDCSub = runRMSComputation(true);
  Serial.print(F("  DC-sub RMS: "));
  Serial.println(rmsDCSub);

  // DC-subtracted should be less (50% PWM has large DC component)
  Serial.print(F("  DC-sub < plain: "));
  if (rmsDCSub < rmsPlain) {
    Serial.println(F("yes ... PASS"));
    testsPassed++;
  } else {
    Serial.println(F("no ... FAIL"));
  }
  Serial.println();

  // ==========================================================================
  // Test 3: RMS different duty cycles
  // ==========================================================================
  Serial.println(F("Test 3: RMS different duty cycles"));
  testsRun++;

  // 25% duty cycle
  setupFastPWM(64);
  delay(10);
  uint16_t rms25 = runRMSComputation(false);
  Serial.print(F("  25% duty RMS: "));
  Serial.println(rms25);

  // 75% duty cycle
  setupFastPWM(192);
  delay(10);
  uint16_t rms75 = runRMSComputation(false);
  Serial.print(F("  75% duty RMS: "));
  Serial.println(rms75);

  // 75% should have higher RMS than 25%
  Serial.print(F("  25% RMS < 75% RMS: "));
  if (rms25 < rms75) {
    Serial.println(F("yes ... PASS"));
    testsPassed++;
  } else {
    Serial.println(F("no ... FAIL"));
  }

  // Cleanup
  stopPWM();
  ads.enableRMS(false);
  ads.stopSequence();

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
