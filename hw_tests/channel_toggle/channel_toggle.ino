/*!
 * @file channel_toggle.ino
 * @brief Toggle a single ADS7128 channel for hardware probing
 *
 * Change TEST_CHANNEL to select which channel toggles.
 * 10ms HIGH / 10ms LOW loop — probe with scope or LED.
 */

#include <Adafruit_ADS7128.h>
#include <Wire.h>

Adafruit_ADS7128 ads;

#define ADS7128_ADDR 0x11
#define TEST_CHANNEL 0

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.print(F("Toggling CH"));
  Serial.print(TEST_CHANNEL);
  Serial.println(F(" at ~50Hz (10ms HIGH / 10ms LOW)"));

  Wire.begin();
  if (!ads.begin(ADS7128_ADDR)) {
    Serial.println(F("FAIL: begin"));
    while (1) delay(10);
  }

  ads.pinMode(TEST_CHANNEL, ADS7128_OUTPUT);
}

void loop() {
  ads.digitalWrite(TEST_CHANNEL, true);
  delay(10);
  ads.digitalWrite(TEST_CHANNEL, false);
  delay(10);
}
