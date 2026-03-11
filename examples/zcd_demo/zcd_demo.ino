/*!
 * @file zcd_demo.ino
 * @brief ADS7128 Zero-Crossing Detection demo
 *
 * Uses a fast PWM signal on Arduino pin 10 as a pseudo-AC source.
 * The ZCD module monitors CH0 and toggles CH1 (GPO) on threshold
 * crossings. Connect an LED to CH1 to see the ZCD output.
 *
 * Hardware:
 * - Arduino pin 10 -> ADS7128 CH0 (PWM signal source)
 * - LED + resistor on CH1 (ZCD output)
 * - ADS7128 at default address
 */

#include <Adafruit_ADS7128.h>
#include <Wire.h>

#define ADS_VREF 3.3
#define PWM_PIN 10

Adafruit_ADS7128 ads;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  Serial.println(F("ADS7128 Zero-Crossing Detection Demo"));
  Serial.println(F("====================================="));

  Wire.begin();

  if (!ads.begin()) {
    Serial.println(F("Failed to find ADS7128!"));
    while (1)
      delay(10);
  }

  // Start fast PWM on pin 10 (~31kHz)
  ::pinMode(PWM_PIN, OUTPUT);
  TCCR1A = _BV(COM1B1) | _BV(WGM10);
  TCCR1B = _BV(WGM12) | _BV(CS10);
  OCR1B = 128; // 50% duty

  // CH0 = analog input (default) — reads the PWM signal
  // CH1 = digital output for ZCD signal (connect LED here)
  ads.pinMode(1, ADS7128_OUTPUT);

  // Set ZCD threshold to VCC/2
  uint16_t threshold = 2048;
  ads.setHighThreshold(0, threshold);

  // Enable DWC (required for ZCD thresholds)
  ads.enableDWC(true);

  // Configure ZCD module
  ads.setZCDChannel(0);   // Monitor CH0
  ads.setZCDBlanking(0);  // No blanking (detect every crossing)
  ads.setZCDOutput(1, 2); // CH1 output = ZCD signal
  ads.enableZCDOutput(1, true);

  // Enable statistics to track recent values
  ads.enableStatistics(true);

  // Start autonomous conversion on CH0
  ads.setSequenceChannels(0x01);
  ads.startSequence();

  Serial.print(F("ZCD threshold: "));
  Serial.print(threshold);
  Serial.print(F(" ("));
  Serial.print(threshold * ADS_VREF / 4095.0, 2);
  Serial.println(F("V)"));
  Serial.println(F("CH0 = PWM input, CH1 = ZCD output (connect LED)"));
  Serial.println(F("\nReading CH0 + ZCD output state:"));
}

void loop() {
  // Use getRecent() — don't call analogRead() which disrupts auto-sequence
  uint16_t raw = ads.getRecent(0);
  bool zcdOut = ads.digitalRead(1);

  Serial.print(F("CH0: "));
  Serial.print(raw);
  Serial.print(F(" ("));
  Serial.print(raw * ADS_VREF / 4095.0, 2);
  Serial.print(F("V) ZCD_OUT: "));
  Serial.println(zcdOut ? F("HIGH") : F("LOW"));

  delay(200);
}
