/*!
 * @file 03_gpio_chase.ino
 * @brief ADS7128 GPIO LED chaser — pulse train across CH0-CH7
 *
 * Cycles a 100ms HIGH pulse through channels 0 to 7.
 * Connect LEDs (with resistors) to each channel to see the chase.
 */

#include <Adafruit_ADS7128.h>

Adafruit_ADS7128 ads;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("ADS7128 GPIO LED Chase"));
  Serial.println(F("======================"));

  // begin() defaults: address ADS7128_DEFAULT_ADDR (0x10), Wire bus &Wire
  if (!ads.begin(ADS7128_DEFAULT_ADDR, &Wire)) {
    Serial.println(F("Failed to find ADS7128!"));
    while (1) {
      delay(100);
    }
  }

  // Set all 8 channels as push-pull outputs
  for (uint8_t ch = 0; ch < 8; ch++) {
    ads.pinMode(ch, ADS7128_OUTPUT);
    ads.digitalWrite(ch, LOW);
  }

  Serial.println(F("All channels set as outputs"));
  Serial.println(F("Connect LEDs to CH0-CH7 to see the chase!"));
}

void loop() {
  for (uint8_t ch = 0; ch < 8; ch++) {
    ads.digitalWrite(ch, HIGH);
    delay(100);
    ads.digitalWrite(ch, LOW);
  }
}
