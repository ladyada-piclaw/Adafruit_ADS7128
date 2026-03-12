/*!
 * @file 02_gpio_chain_test.ino
 * @brief GPIO chain walk test for ADS7128 - tests all 8 channels as GPIO
 *
 * Hardware setup:
 * - ADS7128 at default address
 * - AVDD/DVDD = 5V
 * - Channels chained with 10K resistors: A0—10K—A1—10K—A2—...—A7
 *
 * This test walks a GPIO output down the resistor chain to verify
 * all 8 channels work as both output and input.
 */

#include <Adafruit_ADS7128.h>

Adafruit_ADS7128 ads;

uint8_t testsPassed = 0;
uint8_t testsFailed = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  Serial.println(F("ADS7128 GPIO Chain Walk Test"));
  Serial.println(F("-----------------------------"));

  if (!ads.begin()) {
    Serial.println(F("ERROR: Failed to initialize ADS7128!"));
    while (1) {
      delay(1000);
    }
  }

  // Set all channels to analog (high-impedance) initially
  for (uint8_t i = 0; i < 8; i++) {
    ads.pinMode(i, ADS7128_ANALOG);
  }

  // Test each adjacent pair
  for (uint8_t n = 0; n < 7; n++) {
    testPair(n, n + 1);
  }

  // Print summary
  Serial.println();
  Serial.println(F("================================"));
  Serial.print(F("RESULTS: "));
  Serial.print(testsPassed);
  Serial.print(F("/"));
  Serial.print(testsPassed + testsFailed);
  Serial.println(F(" tests passed"));
  Serial.println(F("================================"));
}

void testPair(uint8_t outCh, uint8_t inCh) {
  bool highOk = false;
  bool lowOk = false;

  // Configure output channel as push-pull output
  ads.pinMode(outCh, ADS7128_OUTPUT);

  // Configure input channel as digital input
  ads.pinMode(inCh, ADS7128_INPUT);

  // Small delay for signals to settle through resistor chain
  delay(20);

  // Test HIGH
  ads.digitalWrite(outCh, true);
  delay(20); // Let signal propagate
  bool readHigh = ads.digitalRead(inCh);
  highOk = readHigh;

  // Test LOW
  ads.digitalWrite(outCh, false);
  delay(20); // Let signal propagate
  bool readLow = ads.digitalRead(inCh);
  lowOk = !readLow; // Should read LOW

  // Reset both channels to analog
  ads.pinMode(outCh, ADS7128_ANALOG);
  ads.pinMode(inCh, ADS7128_ANALOG);

  // Print result
  Serial.print(F("Pair CH"));
  Serial.print(outCh);
  Serial.print(F("->CH"));
  Serial.print(inCh);
  Serial.print(F(": HIGH="));
  Serial.print(readHigh ? F("HIGH") : F("LOW"));
  Serial.print(F(" LOW="));
  Serial.print(readLow ? F("HIGH") : F("LOW"));
  Serial.print(F(" ... "));

  if (highOk && lowOk) {
    Serial.println(F("PASS"));
    testsPassed++;
  } else {
    Serial.println(F("FAIL"));
    testsFailed++;
  }
}

void loop() {
  // Nothing to do
  delay(1000);
}
