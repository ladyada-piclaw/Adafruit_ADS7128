/*!
 * @file 07_dwc_alert_test.ino
 * @brief Hardware test for ADS7128 Digital Window Comparator + ALERT pin
 *
 * Tests DWC high/low threshold alerts with interrupt detection on Metro D3.
 *
 * Hardware setup:
 * - ADS7128 at 0x11, AVDD/DVDD = 5V
 * - CH0 configured as push-pull output
 * - CH1 configured as analog input
 * - A0—10K—A1—10K—...—A7 resistor chain
 * - ALERT pin connected to Metro Mini D3
 */

#include <Adafruit_ADS7128.h>
#include <Wire.h>

#define ADS7128_ADDR 0x11
#define ALERT_PIN 3

Adafruit_ADS7128 ads;

volatile bool alertFired = false;

void alertISR() { alertFired = true; }

void setup() {
  Serial.begin(115200);

  Serial.println(F("ADS7128 Digital Window Comparator + ALERT Test"));
  Serial.println(F("-------------------------------------------------"));
  Serial.println(F("ALERT pin on Metro D3"));
  Serial.println();

  // Configure ALERT pin with pullup and falling edge interrupt
  ::pinMode(ALERT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ALERT_PIN), alertISR, FALLING);

  Wire.begin();

  if (!ads.begin(ADS7128_ADDR)) {
    Serial.println(F("ERROR: ADS7128 not found at 0x11!"));
    while (1)
      delay(10);
  }

  // Configure CH0 as push-pull output, CH1 as analog input
  ads.pinMode(0, ADS7128_OUTPUT);
  ads.pinMode(1, ADS7128_ANALOG);

  int passed = 0;
  int total = 3;

  // =========================================================================
  // Test 1: High threshold alert
  // =========================================================================
  Serial.println(F("Test 1: High threshold alert"));

  // Drive CH0 LOW first so CH1 reads ~0V
  ads.digitalWrite(0, false);
  delay(10);

  // Take baseline reading of CH1
  uint16_t baseline = ads.analogRead(1);
  float baselineV = baseline * 5.0 / 4095.0;
  Serial.print(F("  Baseline CH1: "));
  Serial.print(baseline);
  Serial.print(F(" ("));
  Serial.print(baselineV, 2);
  Serial.println(F(" V)"));

  // Set high threshold to baseline + 500 counts
  uint16_t highThresh = baseline + 500;
  if (highThresh > 4095)
    highThresh = 4095;
  Serial.print(F("  High threshold: "));
  Serial.println(highThresh);

  // Set low threshold to 0
  ads.setLowThreshold(1, 0);
  ads.setHighThreshold(1, highThresh);

  // Enable DWC
  ads.enableDWC(true);

  // Set alert channels to CH1 (bit 1 = 0x02)
  ads.setAlertChannels(0x02);

  // Configure alert pin: push-pull, active low (logic=0)
  ads.configureAlert(true, 0);

  // Clear any existing event flags
  ads.clearEventFlags();

  // Clear interrupt flag
  alertFired = false;

  // Now drive CH0 HIGH — CH1 will jump to ~4.3V
  ads.digitalWrite(0, true);
  delay(10);

  // Take a few reads to trigger comparison
  for (int i = 0; i < 5; i++) {
    ads.analogRead(1);
    delay(5);
  }

  uint16_t afterHigh = ads.analogRead(1);
  float afterHighV = afterHigh * 5.0 / 4095.0;
  Serial.print(F("  CH0=HIGH, CH1 now: "));
  Serial.print(afterHigh);
  Serial.print(F(" ("));
  Serial.print(afterHighV, 2);
  Serial.println(F(" V)"));

  // Wait up to 500ms for interrupt
  unsigned long start = millis();
  while (!alertFired && (millis() - start < 500)) {
    ads.analogRead(1); // Keep triggering comparisons
    delay(10);
  }

  Serial.print(F("  Interrupt fired: "));
  Serial.println(alertFired ? F("YES") : F("NO"));

  uint8_t eventHigh = ads.getEventHighFlags();
  Serial.print(F("  Event high flags: 0x"));
  if (eventHigh < 0x10)
    Serial.print(F("0"));
  Serial.print(eventHigh, HEX);
  Serial.print(F(" (bit 1 "));
  Serial.print((eventHigh & 0x02) ? F("set") : F("NOT set"));
  Serial.println(F(")"));

  bool test1Pass = alertFired && (eventHigh & 0x02);
  Serial.println(test1Pass ? F("  PASS") : F("  FAIL"));
  if (test1Pass)
    passed++;
  Serial.println();

  // =========================================================================
  // Test 2: Low threshold alert
  // =========================================================================
  Serial.println(F("Test 2: Low threshold alert"));

  // Clear event flags and interrupt flag
  ads.clearEventFlags();
  alertFired = false;

  // Set low threshold to ~3.0V (about 2457 counts at 5V ref)
  uint16_t lowThresh = 2457;
  Serial.print(F("  Low threshold: "));
  Serial.println(lowThresh);

  // Set high threshold to max (no trigger)
  ads.setHighThreshold(1, 4095);
  ads.setLowThreshold(1, lowThresh);

  // CH1 is currently ~4.3V (above low threshold — no alert yet)
  // Drive CH0 LOW — CH1 drops to ~0V, below the low threshold
  ads.digitalWrite(0, false);
  delay(10);

  // Take a few reads to trigger comparison
  for (int i = 0; i < 5; i++) {
    ads.analogRead(1);
    delay(5);
  }

  uint16_t afterLow = ads.analogRead(1);
  float afterLowV = afterLow * 5.0 / 4095.0;
  Serial.print(F("  CH0=LOW, CH1 now: "));
  Serial.print(afterLow);
  Serial.print(F(" ("));
  Serial.print(afterLowV, 2);
  Serial.println(F(" V)"));

  // Wait up to 500ms for interrupt
  start = millis();
  while (!alertFired && (millis() - start < 500)) {
    ads.analogRead(1); // Keep triggering comparisons
    delay(10);
  }

  Serial.print(F("  Interrupt fired: "));
  Serial.println(alertFired ? F("YES") : F("NO"));

  uint8_t eventLow = ads.getEventLowFlags();
  Serial.print(F("  Event low flags: 0x"));
  if (eventLow < 0x10)
    Serial.print(F("0"));
  Serial.print(eventLow, HEX);
  Serial.print(F(" (bit 1 "));
  Serial.print((eventLow & 0x02) ? F("set") : F("NOT set"));
  Serial.println(F(")"));

  bool test2Pass = alertFired && (eventLow & 0x02);
  Serial.println(test2Pass ? F("  PASS") : F("  FAIL"));
  if (test2Pass)
    passed++;
  Serial.println();

  // =========================================================================
  // Test 3: clearEventFlags works
  // =========================================================================
  Serial.println(F("Test 3: clearEventFlags"));

  // Verify event flags are currently set
  uint8_t beforeClear = ads.getEventFlags();
  Serial.print(F("  Before clear: 0x"));
  if (beforeClear < 0x10)
    Serial.print(F("0"));
  Serial.println(beforeClear, HEX);

  // Clear all event flags
  ads.clearEventFlags();

  // Read flags again — should be 0
  uint8_t afterClear = ads.getEventFlags();
  Serial.print(F("  After clear: 0x"));
  if (afterClear < 0x10)
    Serial.print(F("0"));
  Serial.println(afterClear, HEX);

  bool test3Pass = (afterClear == 0);
  Serial.println(test3Pass ? F("  PASS") : F("  FAIL"));
  if (test3Pass)
    passed++;
  Serial.println();

  // =========================================================================
  // Results
  // =========================================================================
  Serial.println(F("================================"));
  Serial.print(F("RESULTS: "));
  Serial.print(passed);
  Serial.print(F("/"));
  Serial.print(total);
  Serial.println(F(" tests passed"));
  Serial.println(F("================================"));

  // Disable DWC when done
  ads.enableDWC(false);
}

void loop() {
  // Nothing to do
}
