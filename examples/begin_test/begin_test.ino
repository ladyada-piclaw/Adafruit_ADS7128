/*!
 * @file begin_test.ino
 *
 * Basic initialization test for ADS7128.
 * Tests begin() and reads SYSTEM_STATUS register.
 *
 * Hardware:
 * - ADS7128 at address 0x11 (ADDR to GND via ~10kΩ)
 * - SDA/SCL to A4/A5 on Metro Mini
 */

#include <Adafruit_ADS7128.h>
#include <Wire.h>

Adafruit_ADS7128 ads;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("\n=== ADS7128 Begin Test ==="));

  // Initialize I2C
  Wire.begin();

  // Scan for device first
  Serial.print(F("Scanning for device at 0x11... "));
  Wire.beginTransmission(0x11);
  if (Wire.endTransmission() == 0) {
    Serial.println(F("FOUND"));
  } else {
    Serial.println(F("NOT FOUND!"));
    Serial.println(F("Check wiring and I2C address."));
    while (1) {
      delay(1000);
    }
  }

  // Initialize ADS7128
  Serial.print(F("Calling begin()... "));
  if (ads.begin(0x11, &Wire)) {
    Serial.println(F("SUCCESS"));
  } else {
    Serial.println(F("FAILED"));
    Serial.println(F("begin() returned false - check connections."));
    while (1) {
      delay(1000);
    }
  }

  // Check CRC status
  Serial.print(F("CRC error flag: "));
  if (ads.getCRCError()) {
    Serial.println(F("SET (error detected)"));
    ads.clearCRCError();
  } else {
    Serial.println(F("clear"));
  }

  Serial.println(F("\n=== Initialization Complete ==="));
  Serial.println(F("ADS7128 is ready for use."));
}

void loop() {
  // Nothing to do in loop - this is just an init test
  delay(1000);
}
