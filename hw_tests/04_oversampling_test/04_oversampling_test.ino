/*!
 * @file 04_oversampling_test.ino
 * @brief ADS7128 Oversampling Noise Reduction Test
 *
 * Hardware setup:
 * - ADS7128 at I2C address 0x10
 * - AVDD/DVDD = 5V
 * - Channels A0–A7 chained with 10K resistors:
 *   A0—10K—A1—10K—A2—10K—A3—10K—A4—10K—A5—10K—A6—10K—A7
 *
 * Test: Configure CH0=HIGH, CH7=LOW to create a stable ~2.85V on CH3.
 * For each oversampling setting (NONE through 128x), take 20 readings
 * and measure noise (range = max - min).
 *
 * Self-calibrating: Uses OSR_NONE baseline voltage for comparison.
 * Verifies that:
 *   1. All OSR mean voltages are within 0.3V of baseline
 *   2. OSR_128 range <= OSR_NONE range (proving noise reduction)
 */

#include <Adafruit_ADS7128.h>
#include <Wire.h>

Adafruit_ADS7128 ads;

#define ADS7128_ADDR 0x10
#define NUM_SAMPLES 20
#define VREF 5.0

// OSR settings array for iteration
const ads7128_osr_t osrSettings[] = {
    ADS7128_OSR_NONE, ADS7128_OSR_2,  ADS7128_OSR_4,  ADS7128_OSR_8,
    ADS7128_OSR_16,   ADS7128_OSR_32, ADS7128_OSR_64, ADS7128_OSR_128};

const char *osrNames[] = {"NONE", "2x  ", "4x  ", "8x  ",
                          "16x ", "32x ", "64x ", "128x"};

const uint8_t numOSR = 8;

uint8_t testsPassed = 0;
uint8_t totalTests = 2;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  Serial.println(F("ADS7128 Oversampling Noise Test"));
  Serial.println(F("--------------------------------"));

  Wire.begin();

  if (!ads.begin(ADS7128_ADDR)) {
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

  // Configure CH3 as analog input
  if (!ads.pinMode(3, ADS7128_ANALOG)) {
    Serial.println(F("ERROR: Failed to set CH3 as analog"));
  }

  delay(50); // Allow voltage to settle

  // Take reference reading at OSR_NONE
  ads.setOversampling(ADS7128_OSR_NONE);
  delay(5);
  uint32_t refSum = 0;
  for (uint8_t i = 0; i < NUM_SAMPLES; i++) {
    refSum += ads.analogRead(3);
    delay(2);
  }
  float refVoltage = ((float)refSum / NUM_SAMPLES) * VREF / 4095.0;
  Serial.print(F("Reference voltage on CH3: "));
  Serial.print(refVoltage, 2);
  Serial.println(F(" V"));
  Serial.println();

  // Store results for each OSR
  float means[8];
  uint16_t mins[8];
  uint16_t maxs[8];
  uint16_t ranges[8];

  Serial.println(F("OSR    Mean(V)  Min    Max    Range"));

  for (uint8_t osrIdx = 0; osrIdx < numOSR; osrIdx++) {
    ads.setOversampling(osrSettings[osrIdx]);
    delay(5); // Allow OSR setting to take effect

    uint32_t sum = 0;
    uint16_t minVal = 0xFFFF;
    uint16_t maxVal = 0;

    for (uint8_t i = 0; i < NUM_SAMPLES; i++) {
      uint16_t raw = ads.analogRead(3);
      sum += raw;
      if (raw < minVal)
        minVal = raw;
      if (raw > maxVal)
        maxVal = raw;
      delay(3); // Allow ADC to complete with higher OSR
    }

    float meanV = ((float)sum / NUM_SAMPLES) * VREF / 4095.0;
    uint16_t range = maxVal - minVal;

    means[osrIdx] = meanV;
    mins[osrIdx] = minVal;
    maxs[osrIdx] = maxVal;
    ranges[osrIdx] = range;

    Serial.print(osrNames[osrIdx]);
    Serial.print(F("   "));
    Serial.print(meanV, 2);
    Serial.print(F("     "));
    Serial.print(minVal);
    Serial.print(F("   "));
    Serial.print(maxVal);
    Serial.print(F("   "));
    Serial.println(range);
  }

  Serial.println();

  // Test 1: All means within 0.3V of reference
  bool meansOk = true;
  for (uint8_t i = 0; i < numOSR; i++) {
    float diff = means[i] - refVoltage;
    if (diff < 0)
      diff = -diff;
    if (diff > 0.3) {
      meansOk = false;
      break;
    }
  }
  Serial.print(F("Test 1: All means within 0.3V of reference: "));
  if (meansOk) {
    Serial.println(F("PASS"));
    testsPassed++;
  } else {
    Serial.println(F("FAIL"));
  }

  // Test 2: OSR_128 range <= OSR_NONE range
  Serial.print(F("Test 2: OSR_128 range <= OSR_NONE range: "));
  if (ranges[7] <= ranges[0]) {
    Serial.println(F("PASS"));
    testsPassed++;
  } else {
    Serial.print(F("FAIL ("));
    Serial.print(ranges[7]);
    Serial.print(F(" > "));
    Serial.print(ranges[0]);
    Serial.println(F(")"));
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
