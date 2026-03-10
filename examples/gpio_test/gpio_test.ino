/*!
 * @file gpio_test.ino
 *
 * GPIO test for ADS7128.
 * Configures channel 0 as push-pull output, channel 1 as digital input.
 * Toggles channel 0 and reads channel 1.
 *
 * With a 10K resistor chain between channels, channel 1 should reflect
 * the state of channel 0.
 *
 * Hardware:
 * - ADS7128 at address 0x10 (ADDR to GND via ~10kΩ)
 * - SDA/SCL to A4/A5 on Metro Mini
 * - 10K resistor between CH0 and CH1
 */

#include <Adafruit_ADS7128.h>
#include <Wire.h>

Adafruit_ADS7128 ads;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("\n=== ADS7128 GPIO Test ==="));

  // Initialize
  Wire.begin();

  if (!ads.begin(0x10, &Wire)) {
    Serial.println(F("Failed to find ADS7128!"));
    while (1) {
      delay(1000);
    }
  }
  Serial.println(F("ADS7128 initialized."));

  // Configure channel 0 as push-pull output
  Serial.print(F("Setting CH0 as push-pull output... "));
  if (ads.pinMode(0, ADS7128_OUTPUT)) {
    Serial.println(F("OK"));
  } else {
    Serial.println(F("FAILED"));
  }

  // Configure channel 1 as digital input
  Serial.print(F("Setting CH1 as digital input... "));
  if (ads.pinMode(1, ADS7128_INPUT)) {
    Serial.println(F("OK"));
  } else {
    Serial.println(F("FAILED"));
  }

  // Check CRC status
  if (ads.getCRCError()) {
    Serial.println(F("WARNING: CRC error detected!"));
    ads.clearCRCError();
  }

  Serial.println(F("\nStarting GPIO toggle test..."));
  Serial.println(F("CH0 (output) <-> CH1 (input)"));
  Serial.println(F("With 10K resistor chain, CH1 should reflect CH0.\n"));
}

void loop() {
  // Set CH0 HIGH
  Serial.print(F("CH0 -> HIGH, "));
  ads.digitalWrite(0, HIGH);
  delay(100); // Let it settle

  bool ch1_state = ads.digitalRead(1);
  Serial.print(F("CH1 reads: "));
  Serial.println(ch1_state ? F("HIGH") : F("LOW"));

  delay(500);

  // Set CH0 LOW
  Serial.print(F("CH0 -> LOW,  "));
  ads.digitalWrite(0, LOW);
  delay(100); // Let it settle

  ch1_state = ads.digitalRead(1);
  Serial.print(F("CH1 reads: "));
  Serial.println(ch1_state ? F("HIGH") : F("LOW"));

  // Check for CRC errors
  if (ads.getCRCError()) {
    Serial.println(F("*** CRC ERROR DETECTED ***"));
    ads.clearCRCError();
  }

  delay(1000);
  Serial.println();
}
