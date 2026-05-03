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
 * - ALERT pin connected to Arduino interrupt pin (D3 on Uno/Metro)
 *
 * Written by Limor 'ladyada' Fried with assistance from Claude Code for
 * Adafruit Industries. MIT license.
 */

#include <Adafruit_ADS7128.h>

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

  Serial.println(F("ADS7128 Analog Threshold Alert Example"));
  Serial.println(F("======================================="));

  // begin() defaults: address ADS7128_DEFAULT_ADDR (0x10), Wire bus &Wire
  if (!ads.begin(ADS7128_DEFAULT_ADDR, &Wire)) {
    Serial.println(F("Failed to find ADS7128!"));
    while (1)
      delay(100);
  }

  // CH0 defaults to analog input — that's what we want

  // Set low threshold at ~AVDD/4
  ads.setLowThreshold(BUTTON_CH, 1000);
  ads.setHighThreshold(BUTTON_CH,
                       4095); // Change to 3000 to trigger on release as well!

  // Enable DWC and alert on CH0
  ads.enableDWC(true);
  ads.setAlertChannels(1 << BUTTON_CH);
  ads.configureAlert(true, 0); // Push-pull, active low

  // Clear any stale events
  ads.clearEventFlags();

  // Attach interrupt — ALERT is active low
  pinMode(ALERT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ALERT_PIN), alertISR, FALLING);

  Serial.println(F("Monitoring CH0 analog input..."));
  Serial.println(F("ALERT fires when signal drops below threshold."));
  Serial.println();
}

void loop() {
  // Read CH0 so the DWC can compare against thresholds
  uint16_t raw = ads.analogRead(BUTTON_CH);

  if (alertFired) {
    alertFired = false;

    Serial.print(F("ALERT! CH0 raw = "));
    Serial.println(raw);

    ads.clearEventFlags();
  }

  delay(10);
}
