/*!
 * @file adc_simpletest.ino
 * @brief Read all 8 ADC channels in manual mode
 *
 * Reads all 8 channels and prints raw values and voltages.
 * Hardware: ADS7128 with 5V reference
 */

#include <Adafruit_ADS7128.h>
#include <Wire.h>

Adafruit_ADS7128 adc;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("ADS7128 ADC Simple Test"));
  Serial.println(F("======================="));

  Wire.begin();

  if (!adc.begin(0x10)) {
    Serial.println(F("Failed to find ADS7128!"));
    while (1) {
      delay(100);
    }
  }

  Serial.println(F("ADS7128 initialized!"));
  Serial.println();
}

void loop() {
  Serial.println(F("Channel  Raw    Voltage"));
  Serial.println(F("-------  ----   -------"));

  for (uint8_t ch = 0; ch < 8; ch++) {
    uint16_t raw = adc.analogRead(ch);
    float voltage = adc.analogReadVoltage(ch, 5.0);

    Serial.print(F("   "));
    Serial.print(ch);
    Serial.print(F("     "));
    if (raw < 1000) {
      Serial.print(F(" "));
    }
    if (raw < 100) {
      Serial.print(F(" "));
    }
    if (raw < 10) {
      Serial.print(F(" "));
    }
    Serial.print(raw);
    Serial.print(F("   "));
    Serial.print(voltage, 3);
    Serial.println(F(" V"));
  }

  Serial.println();
  delay(2000);
}
