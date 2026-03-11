/*!
 * @file autonomous_mode.ino
 * @brief Auto-sequence mode with statistics
 *
 * Enables auto-sequence on all 8 channels, reads continuously,
 * and displays min/max statistics.
 */

#include <Adafruit_ADS7128.h>
#include <Wire.h>

Adafruit_ADS7128 adc;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("ADS7128 Autonomous Mode Test"));
  Serial.println(F("============================"));

  Wire.begin();

  if (!adc.begin()) {
    Serial.println(F("Failed to find ADS7128!"));
    while (1) {
      delay(100);
    }
  }

  Serial.println(F("ADS7128 initialized!"));

  // Enable statistics tracking
  adc.enableStatistics(true);

  // Enable all 8 channels in sequence
  adc.setSequenceChannels(0xFF);

  // Start autonomous sequencing
  if (!adc.startSequence()) {
    Serial.println(F("Failed to start sequence!"));
  }

  Serial.println(F("Sequence started on all channels"));
  Serial.println();
}

void loop() {
  // Let some conversions happen
  delay(100);

  // Read statistics for all channels
  Serial.println(F("Ch  Recent   Min     Max"));
  Serial.println(F("--  ------  ------  ------"));

  for (uint8_t ch = 0; ch < 8; ch++) {
    uint16_t recent = adc.getRecent(ch);
    uint16_t minVal = adc.getMin(ch);
    uint16_t maxVal = adc.getMax(ch);

    Serial.print(ch);
    Serial.print(F("   "));
    printPadded(recent);
    Serial.print(F("  "));
    printPadded(minVal);
    Serial.print(F("  "));
    printPadded(maxVal);
    Serial.println();
  }

  Serial.println();
  delay(500);
}

void printPadded(uint16_t val) {
  if (val < 1000) {
    Serial.print(F(" "));
  }
  if (val < 100) {
    Serial.print(F(" "));
  }
  if (val < 10) {
    Serial.print(F(" "));
  }
  Serial.print(val);
}
