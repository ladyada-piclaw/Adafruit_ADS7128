/*!
 * @file 04_oversampling_test.ino
 * @brief ADS7128 Oversampling Test with PWM Signal
 *
 * Hardware setup:
 * - ADS7128 at default I2C address
 * - AVDD/DVDD = 5V
 * - Metro Mini pin 10 (Timer1 PWM, OC1B) connected to ADS7128 CH0
 *
 * Test concept:
 * Metro Mini outputs ~31kHz fast PWM at 50% duty cycle on pin 10.
 * Without oversampling, ADC catches random points of the square wave,
 * resulting in readings scattered between 0V and 5V (large range).
 * With high oversampling, ADC averages many samples, readings converge
 * toward ~2.5V (small range).
 */

#include <Adafruit_ADS7128.h>

#define PWM_PIN 10
#define NUM_SAMPLES 30
#define VREF 5.0

Adafruit_ADS7128 ads;

// OSR settings array for iteration
const ads7128_osr_t osrSettings[] = {
    ADS7128_OSR_NONE, ADS7128_OSR_2,  ADS7128_OSR_4,  ADS7128_OSR_8,
    ADS7128_OSR_16,   ADS7128_OSR_32, ADS7128_OSR_64, ADS7128_OSR_128};

const char* osrNames[] = {"NONE", "2x  ", "4x  ", "8x  ",
                          "16x ", "32x ", "64x ", "128x"};

const uint8_t numOSR = 8;

// Store results
float means[8];
uint16_t mins[8];
uint16_t maxs[8];
uint16_t ranges[8];

uint8_t testsPassed = 0;
const uint8_t totalTests = 3;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("ADS7128 Oversampling Test (PWM Signal)"));
  Serial.println(F("---------------------------------------"));
  Serial.println(F("Fast PWM (~31kHz) on Metro pin 10 -> ADS7128 CH0"));
  Serial.println(F("~31kHz, 50% duty cycle"));
  Serial.println();

  // Start fast PWM on pin 10 (~31kHz) using Timer1
  // Pin 10 = OC1B on ATmega328
  pinMode(PWM_PIN, OUTPUT);
  // Timer1: Fast PWM 8-bit (mode 5), no prescaler (CS10=1)
  // COM1B1=1 for non-inverting PWM on OC1B (pin 10)
  TCCR1A = _BV(COM1B1) | _BV(WGM10);
  TCCR1B = _BV(WGM12) | _BV(CS10);
  OCR1B = 128; // 50% duty

  if (!ads.begin()) {
    Serial.println(F("ERROR: Failed to initialize ADS7128!"));
    while (1) {
      delay(1000);
    }
  }

  // CH0 is analog input by default after begin()
  // Small delay for PWM to stabilize
  delay(10);

  Serial.println(F("OSR    Mean(V)  Min    Max    Range"));

  for (uint8_t osrIdx = 0; osrIdx < numOSR; osrIdx++) {
    ads.setOversampling(osrSettings[osrIdx]);
    delay(5); // Allow OSR setting to take effect

    uint32_t sum = 0;
    uint16_t minVal = 0xFFFF;
    uint16_t maxVal = 0;

    for (uint8_t i = 0; i < NUM_SAMPLES; i++) {
      uint16_t raw = ads.analogRead(0);
      sum += raw;
      if (raw < minVal)
        minVal = raw;
      if (raw > maxVal)
        maxVal = raw;
      delay(5); // Give ADC time with higher OSR
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

  // Test 1: OSR_NONE should have large range (>500 counts)
  // Catching both high and low of the square wave
  Serial.print(F("Test 1: OSR_NONE range > 500 counts: "));
  if (ranges[0] > 500) {
    Serial.println(F("PASS"));
    testsPassed++;
  } else {
    Serial.print(F("FAIL (range="));
    Serial.print(ranges[0]);
    Serial.println(F(")"));
  }

  // Test 2: OSR_128 range should be smaller than OSR_NONE range
  Serial.print(F("Test 2: OSR_128 range < OSR_NONE range: "));
  if (ranges[7] < ranges[0]) {
    Serial.println(F("PASS"));
    testsPassed++;
  } else {
    Serial.print(F("FAIL ("));
    Serial.print(ranges[7]);
    Serial.print(F(" >= "));
    Serial.print(ranges[0]);
    Serial.println(F(")"));
  }

  // Test 3: OSR_128 mean should be approximately 2.5V (within 1.0V)
  float diff = means[7] - 2.5;
  if (diff < 0)
    diff = -diff;
  Serial.print(F("Test 3: OSR_128 mean within 1.0V of 2.5V: "));
  if (diff <= 1.0) {
    Serial.println(F("PASS"));
    testsPassed++;
  } else {
    Serial.print(F("FAIL (mean="));
    Serial.print(means[7], 2);
    Serial.println(F("V)"));
  }

  Serial.println();
  Serial.println(F("================================"));
  Serial.print(F("RESULTS: "));
  Serial.print(testsPassed);
  Serial.print(F("/"));
  Serial.print(totalTests);
  Serial.println(F(" tests passed"));
  Serial.println(F("================================"));

  // Stop PWM (set pin LOW)
  TCCR1A = 0; // Stop Timer1 PWM
  TCCR1B = 0;
  digitalWrite(PWM_PIN, LOW);
}

void loop() {
  // Nothing to do in loop
}
