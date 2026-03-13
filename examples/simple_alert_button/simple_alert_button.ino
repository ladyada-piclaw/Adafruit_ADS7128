/*!
 * @file simple_alert_button.ino
 * @brief ADS7128 single button example using DWC + ALERT pin
 *
 * Monitors one channel for a button press using the digital window
 * comparator. When pressed, the ALERT pin goes low. This example
 * uses manual (non-autonomous) mode, reading the channel on demand.
 *
 * Hardware:
 * - CH0: 10K pull-up to AVDD, button to GND
 * - ALERT pin connected to Arduino interrupt pin (D3 on Uno/Metro)
 * - Note: ADS7128 has no internal pull-ups, external 10K required
 *
 * Written by Limor 'ladyada' Fried with assistance from Claude Code for
 * Adafruit Industries. MIT license.
 */

#include <Adafruit_ADS7128.h>

#define ADS_VREF 3.3
#define ALERT_PIN 3
#define BUTTON_CH 0

Adafruit_ADS7128 ads;

volatile bool alertFired = false;

void alertISR() {
  alertFired = true;
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  Serial.println(F("ADS7128 Simple Button Alert Example"));
  Serial.println(F("===================================="));

  if (!ads.begin()) {
    Serial.println(F("Failed to find ADS7128!"));
    while (1)
      delay(100);
  }

  // CH0 defaults to analog input — that's what we want

  // Set low threshold: ~0.5V (button press = GND)
  uint16_t threshold = (uint16_t)(0.5 / ADS_VREF * 4095);
  ads.setLowThreshold(BUTTON_CH, threshold);
  ads.setHighThreshold(BUTTON_CH, 4095); // Don't trigger on high

  // Enable DWC and alert on CH0
  ads.enableDWC(true);
  ads.setAlertChannels(1 << BUTTON_CH);
  ads.configureAlert(true, 0); // Push-pull, active low

  // Clear any stale events
  ads.clearEventFlags();

  // Attach interrupt — ALERT is active low
  pinMode(ALERT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ALERT_PIN), alertISR, FALLING);

  Serial.println(F("Waiting for button press on CH0..."));
  Serial.println(F("(10K pull-up to AVDD, button to GND)"));
  Serial.println();
}

void loop() {
  // Periodically read CH0 so the DWC can compare against thresholds
  uint16_t raw = ads.analogRead(BUTTON_CH);

  if (alertFired) {
    alertFired = false;

    Serial.print(F("Button pressed! CH0 = "));
    Serial.print(raw);
    Serial.print(F(" ("));
    Serial.print(raw * ADS_VREF / 4095.0, 2);
    Serial.println(F("V)"));

    ads.clearEventFlags();
  }

  delay(10);
}
