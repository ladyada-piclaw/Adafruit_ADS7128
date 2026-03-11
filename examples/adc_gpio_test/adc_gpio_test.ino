/*!
 * @file adc_gpio_test.ino
 * @brief Test GPIO output affecting adjacent analog channels
 *
 * Hardware setup: Resistor chain A0-10K-A1-10K-A2-...-A7
 * Sets CH0 as GPO, reads adjacent channels as analog.
 * Toggling CH0 creates measurable voltage changes via divider.
 */

#include <Adafruit_ADS7128.h>
#include <Wire.h>

Adafruit_ADS7128 adc;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("ADS7128 GPIO + ADC Test"));
  Serial.println(F("======================="));

  Wire.begin();

  if (!adc.begin()) {
    Serial.println(F("Failed to find ADS7128!"));
    while (1) {
      delay(100);
    }
  }

  Serial.println(F("ADS7128 initialized!"));

  // Configure CH0 as push-pull output
  if (!adc.pinMode(0, ADS7128_OUTPUT)) {
    Serial.println(F("Failed to set CH0 as output!"));
  }

  // CH1-CH7 remain as analog inputs (default)
  Serial.println(F("CH0 = GPO output, CH1-CH7 = analog inputs"));
  Serial.println();
}

void loop() {
  // Set CH0 HIGH
  Serial.println(F("CH0 = HIGH"));
  adc.digitalWrite(0, true);
  delay(100); // Let voltage settle

  Serial.println(F("Adjacent channel readings:"));
  for (uint8_t ch = 1; ch <= 3; ch++) {
    float v = adc.analogReadVoltage(ch, 5.0);
    Serial.print(F("  CH"));
    Serial.print(ch);
    Serial.print(F(": "));
    Serial.print(v, 3);
    Serial.println(F(" V"));
  }

  delay(1000);

  // Set CH0 LOW
  Serial.println(F("\nCH0 = LOW"));
  adc.digitalWrite(0, false);
  delay(100);

  Serial.println(F("Adjacent channel readings:"));
  for (uint8_t ch = 1; ch <= 3; ch++) {
    float v = adc.analogReadVoltage(ch, 5.0);
    Serial.print(F("  CH"));
    Serial.print(ch);
    Serial.print(F(": "));
    Serial.print(v, 3);
    Serial.println(F(" V"));
  }

  Serial.println(F("\n-------------------\n"));
  delay(500);
}
