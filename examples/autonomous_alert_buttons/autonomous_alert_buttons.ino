/*!
 * @file autonomous_alert_buttons.ino
 * @brief ADS7128 8-button interrupt example using autonomous mode + ALERT
 *
 * Detects button presses on all 8 channels without polling over I2C.
 * The ADS7128 continuously scans all channels in autonomous mode and
 * fires the ALERT pin when any button is pressed below the threshold.
 * The host MCU only needs to respond to the interrupt — no periodic
 * I2C reads required, saving bus bandwidth and CPU time.
 *
 * Hardware:
 * - CH0-CH7: Each with 10K pull-up to AVDD, button to GND
 * - ALERT pin connected to Arduino interrupt pin (D3 on Uno/Metro)
 * - Note: ADS7128 has no internal pull-ups, external 10K required
 *
 * Written by Limor 'ladyada' Fried with assistance from Claude Code for
 * Adafruit Industries. MIT license.
 */

#include <Adafruit_ADS7128.h>

#define ADS_VREF 3.3
#define ALERT_PIN 3
#define NUM_CHANNELS 8

Adafruit_ADS7128 ads;

volatile bool alertFired = false;

void alertISR() {
  alertFired = true;
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  Serial.println(F("ADS7128 Autonomous 8-Button Alert Example"));
  Serial.println(F("=========================================="));
  Serial.println(F("Detects button presses on CH0-CH7 via ALERT interrupt."));
  Serial.println(F("No I2C polling needed — ADS7128 monitors autonomously."));

  if (!ads.begin()) {
    Serial.println(F("Failed to find ADS7128!"));
    while (1)
      delay(100);
  }

  // All channels default to analog input — that's what we want

  // Set thresholds for all 8 channels:
  //   Low threshold at ~0.5V (button press pulls to GND)
  //   High threshold at max (don't trigger on high)
  uint16_t lowThresh = (uint16_t)(0.5 / ADS_VREF * 4095);
  for (uint8_t ch = 0; ch < NUM_CHANNELS; ch++) {
    ads.setLowThreshold(ch, lowThresh);
    ads.setHighThreshold(ch, 4095);
  }

  // Enable DWC and alert on all 8 channels
  ads.enableDWC(true);
  ads.setAlertChannels(0xFF);  // All channels
  ads.configureAlert(true, 0); // Push-pull, active low

  // Start autonomous sequence scanning all channels
  ads.setSequenceChannels(0xFF);
  ads.startSequence();

  // Clear any stale events
  ads.clearEventFlags();

  // Attach interrupt — ALERT is active low
  pinMode(ALERT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ALERT_PIN), alertISR, FALLING);

  Serial.println(F("\nWaiting for button presses on CH0-CH7..."));
  Serial.println(F("(10K pull-up to AVDD per channel, button to GND)"));
  Serial.println();
}

void loop() {
  if (alertFired) {
    alertFired = false;

    // Read which channels triggered the low event
    uint8_t lowFlags = ads.getEventLowFlags();

    for (uint8_t ch = 0; ch < NUM_CHANNELS; ch++) {
      if (lowFlags & (1 << ch)) {
        Serial.print(F("Button pressed on CH"));
        Serial.println(ch);
      }
    }

    ads.clearEventFlags();
  }
}
