/*!
 * @file gpio_test.ino
 * @brief ADS7128 GPIO output/input test
 *
 * Configures channel 0 as push-pull output, channel 1 as digital input.
 * Toggles channel 0 and reads channel 1.
 *
 * Hardware options:
 * - Connect a wire between CH0 and CH1 (CH1 reads back CH0)
 * - Or connect an LED to CH0 and a button to CH1
 */

#include <Adafruit_ADS7128.h>

Adafruit_ADS7128 ads;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("ADS7128 GPIO Test"));
  Serial.println(F("================="));

  if (!ads.begin()) {
    Serial.println(F("Failed to find ADS7128!"));
    while (1) {
      delay(100);
    }
  }
  Serial.println(F("ADS7128 initialized."));

  ads.pinMode(0, ADS7128_OUTPUT);
  ads.pinMode(1, ADS7128_INPUT);

  Serial.println(F("CH0 = output, CH1 = input"));
  Serial.println(F("Wire CH0 to CH1, or LED on CH0 + button on CH1"));
  Serial.println();
}

void loop() {
  ads.digitalWrite(0, HIGH);
  delay(100);
  bool val = ads.digitalRead(1);
  Serial.print(F("CH0=HIGH, CH1="));
  Serial.println(val ? F("HIGH") : F("LOW"));

  ads.digitalWrite(0, LOW);
  delay(100);
  val = ads.digitalRead(1);
  Serial.print(F("CH0=LOW,  CH1="));
  Serial.println(val ? F("HIGH") : F("LOW"));

  Serial.println();
  delay(500);
}
