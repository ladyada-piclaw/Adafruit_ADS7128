/*!
 * @file 07_zcd_demo.ino
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

#define ADS_VREF 3.3
#define PWM_PIN 10

Adafruit_ADS7128 ads;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  Serial.println(F("ADS7128 Zero-Crossing Detection Demo"));
  Serial.println(F("====================================="));

  // begin() defaults: address ADS7128_DEFAULT_ADDR (0x10), Wire bus &Wire
  if (!ads.begin(ADS7128_DEFAULT_ADDR, &Wire)) {
    Serial.println(F("Failed to find ADS7128!"));
    while (1)
      delay(10);
  }

  // Start PWM signal on pin 10 as a pseudo-AC source
  pinMode(PWM_PIN, OUTPUT);
  analogWrite(PWM_PIN, 128); // 50% duty

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
  Serial.println(F("\nCounting upward zero crossings on CH0:"));
}

void loop() {
  static uint32_t crossings = 0;
  static uint32_t lastReport = 0;

  // Poll the high-threshold event flag and count each upward crossing.
  // Reading getEventHighFlags() doesn't clear it — we have to clear flags
  // ourselves so the next crossing can latch.
  if (ads.getEventHighFlags() & 0x01) {
    crossings++;
    ads.clearEventFlags();
  }

  uint32_t now = millis();
  if (now - lastReport >= 1000) {
    Serial.print(F("Crossings/sec: "));
    Serial.print(crossings);
    Serial.print(F("   CH0: "));
    uint16_t raw = ads.getRecent(0);
    Serial.print(raw);
    Serial.print(F(" ("));
    Serial.print(raw * ADS_VREF / 4095.0, 2);
    Serial.println(F("V)"));
    crossings = 0;
    lastReport = now;
  }
}
