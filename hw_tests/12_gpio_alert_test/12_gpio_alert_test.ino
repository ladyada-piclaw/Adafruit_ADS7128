/*!
 * @file 12_gpio_alert_test.ino
 * @brief Hardware test for ADS7128 GPIO digital input alert via DWC
 *
 * Tests that digital inputs trigger EVENT_HIGH_FLAG and EVENT_LOW_FLAG
 * through the DWC, and that the ALERT pin fires for press/release events.
 *
 * For digital inputs, EVENT_RGN controls flag behavior:
 *   EVENT_RGN=0: logic 1 sets EVENT_HIGH_FLAG (out-of-window mode)
 *   EVENT_RGN=1: logic 0 sets EVENT_LOW_FLAG (in-band mode)
 *
 * Hardware setup:
 * - ADS7128 at 0x10, AVDD/DVDD = 5V
 * - CH0 configured as push-pull output (simulates button)
 * - CH1 configured as digital input (monitored channel)
 * - A0 wired to A1 via resistor chain
 * - ALERT pin connected to Metro Mini D3
 */

#include <Adafruit_ADS7128.h>

#define ALERT_PIN 3

Adafruit_ADS7128 ads;

volatile bool alertFired = false;

void alertISR() {
  alertFired = true;
}

void setup() {
  Serial.begin(115200);

  Serial.println(F("ADS7128 GPIO Digital Input Alert Test"));
  Serial.println(F("--------------------------------------"));
  Serial.println();

  pinMode(ALERT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ALERT_PIN), alertISR, FALLING);

  if (!ads.begin()) {
    Serial.println(F("ERROR: ADS7128 not found at 0x10!"));
    while (1)
      delay(10);
  }

  // CH0 = push-pull output (drives the signal)
  // CH1 = digital input (monitored by DWC)
  ads.pinMode(0, ADS7128_OUTPUT);
  ads.pinMode(1, ADS7128_INPUT);

  // Enable DWC and alert on CH1
  ads.enableDWC(true);
  ads.setAlertChannels(0x02);  // CH1 only
  ads.configureAlert(true, 0); // Push-pull, active low

  // Autonomous mode required for DWC to evaluate digital inputs
  ads.setSequenceChannels(0x02);
  ads.startSequence();
  delay(50);

  int passed = 0;
  int total = 4;

  // =========================================================================
  // Test 1: Digital HIGH triggers EVENT_HIGH_FLAG (EVENT_RGN=0, default)
  // =========================================================================
  Serial.println(F("Test 1: HIGH triggers EVENT_HIGH_FLAG (EVENT_RGN=0)"));

  ads.digitalWrite(0, false); // Start LOW
  delay(100);
  ads.clearEventFlags();
  alertFired = false;

  ads.digitalWrite(0, true); // Drive HIGH
  delay(100);

  uint8_t highFlags = ads.getEventHighFlags();
  Serial.print(F("  CH1 digital: "));
  Serial.println(ads.digitalRead(1) ? F("HIGH") : F("LOW"));
  Serial.print(F("  EVENT_HIGH_FLAG bit 1: "));
  Serial.println((highFlags & 0x02) ? F("set") : F("NOT set"));
  Serial.print(F("  ALERT fired: "));
  Serial.println(alertFired ? F("yes") : F("no"));

  bool test1 = (highFlags & 0x02) && alertFired;
  Serial.println(test1 ? F("  ... PASS") : F("  ... FAIL"));
  if (test1)
    passed++;
  Serial.println();

  // =========================================================================
  // Test 2: Digital LOW triggers EVENT_LOW_FLAG (EVENT_RGN=1)
  // =========================================================================
  Serial.println(F("Test 2: LOW triggers EVENT_LOW_FLAG (EVENT_RGN=1)"));

  // Set EVENT_RGN bit for CH1 to enable in-band mode
  ads.setEventRegion(1, true);

  ads.clearEventFlags();
  alertFired = false;

  ads.digitalWrite(0, false); // Drive LOW
  delay(100);

  uint8_t lowFlags = ads.getEventLowFlags();
  Serial.print(F("  CH1 digital: "));
  Serial.println(ads.digitalRead(1) ? F("HIGH") : F("LOW"));
  Serial.print(F("  EVENT_LOW_FLAG bit 1: "));
  Serial.println((lowFlags & 0x02) ? F("set") : F("NOT set"));
  Serial.print(F("  ALERT fired: "));
  Serial.println(alertFired ? F("yes") : F("no"));

  bool test2 = (lowFlags & 0x02) && alertFired;
  Serial.println(test2 ? F("  ... PASS") : F("  ... FAIL"));
  if (test2)
    passed++;
  Serial.println();

  // =========================================================================
  // Test 3: Press/release cycle with EVENT_RGN=1 (detects both)
  // =========================================================================
  Serial.println(F("Test 3: Press/release cycle (flip EVENT_RGN)"));

  // Start HIGH (released), EVENT_RGN=1 to detect LOW (press)
  ads.setEventRegion(1, true);
  ads.digitalWrite(0, true);
  ads.clearEventFlags();
  alertFired = false;
  delay(100);

  // Press: drive LOW
  ads.digitalWrite(0, false);
  delay(100);

  uint8_t pressLow = ads.getEventLowFlags();
  bool pressDetected = (pressLow & 0x02) != 0;
  Serial.print(F("  Press detected (LOW flag): "));
  Serial.println(pressDetected ? F("yes") : F("no"));

  // Switch EVENT_RGN=0 to detect HIGH (release)
  ads.setEventRegion(1, false);
  ads.clearEventFlags();
  alertFired = false;

  ads.digitalWrite(0, true);
  delay(100);

  uint8_t releaseHigh = ads.getEventHighFlags();
  bool releaseDetected = (releaseHigh & 0x02) != 0;
  Serial.print(F("  Release detected (HIGH flag): "));
  Serial.println(releaseDetected ? F("yes") : F("no"));

  bool test3 = pressDetected && releaseDetected;
  Serial.println(test3 ? F("  ... PASS") : F("  ... FAIL"));
  if (test3)
    passed++;
  Serial.println();

  // =========================================================================
  // Test 4: Alert disabled channel does NOT fire
  // =========================================================================
  Serial.println(F("Test 4: Disabled alert channel does not fire"));

  ads.setAlertChannels(0x00); // Disable alert on all channels
  ads.clearEventFlags();
  alertFired = false;

  ads.digitalWrite(0, false);
  delay(100);

  Serial.print(F("  ALERT fired with no channels enabled: "));
  Serial.println(alertFired ? F("yes (BAD)") : F("no (good)"));

  bool test4 = !alertFired;
  Serial.println(test4 ? F("  ... PASS") : F("  ... FAIL"));
  if (test4)
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

  ads.stopSequence();
  ads.enableDWC(false);
}

void loop() {}
