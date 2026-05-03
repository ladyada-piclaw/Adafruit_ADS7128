/*!
 * @file 05_simple_alert_button.ino
 * @brief ADS7128 analog threshold alert example using DWC + ALERT pin
 *
 * Monitors CH0 as an analog input with a voltage threshold. When the
 * signal crosses the low threshold, the ALERT pin fires. This uses
 * manual mode — the host reads CH0 periodically and the DWC compares
 * against the programmed thresholds.
 *
 * Hardware:
 * - CH0: Analog signal (e.g., 10K pull-up to AVDD with button to GND)
 *
 * Written by Limor 'ladyada' Fried with assistance from Claude Code for
 * Adafruit Industries. MIT license.
 */

#include <Adafruit_ADS7128.h>

#define BUTTON_CH 0

Adafruit_ADS7128 ads;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  Serial.println(F("ADS7128 Analog Threshold Alert Example"));
  Serial.println(F("======================================="));

  // begin() defaults: address ADS7128_DEFAULT_ADDR (0x10), Wire bus &Wire
  if (!ads.begin(ADS7128_DEFAULT_ADDR, &Wire)) {
    Serial.println(F("Failed to find ADS7128!"));
    while (1)
      delay(100);
  }

  // CH0 defaults to analog input — that's what we want

  // Trigger on press (signal drops below 1000) and release (signal rises
  // above 3000). Set high threshold to 4095 to fire only on press.
  ads.setLowThreshold(BUTTON_CH, 1000);
  ads.setHighThreshold(BUTTON_CH, 3000);

  // Enable DWC and alert on CH0
  ads.enableDWC(true);
  ads.setAlertChannels(1 << BUTTON_CH);
  ads.configureAlert(true, 0); // Push-pull, active low

  // Clear any stale events from power-up
  ads.clearEventFlags();

  Serial.println(F("Monitoring CH0 analog input..."));
  Serial.println(F("Press fires below 1000, release fires above 3000."));
  Serial.println();
}

void loop() {
  // Read CH0 so the DWC can compare against thresholds
  uint16_t raw = ads.analogRead(BUTTON_CH);

  uint8_t lowFlags = ads.getEventLowFlags();
  uint8_t highFlags = ads.getEventHighFlags();
  uint8_t mask = (1 << BUTTON_CH);

  // Track last reported state so we only print on transitions. The DWC
  // event flag stays asserted as long as the signal is in the event region,
  // so clearing it just re-asserts on the next read.
  static bool wasPressed = false;

  if ((lowFlags & mask) && !wasPressed) {
    Serial.print(F("PRESSED!  CH0 raw = "));
    Serial.println(raw);
    wasPressed = true;
    ads.clearEventFlags();
  } else if ((highFlags & mask) && wasPressed) {
    Serial.print(F("RELEASED! CH0 raw = "));
    Serial.println(raw);
    wasPressed = false;
    ads.clearEventFlags();
  }

  delay(10);
}
