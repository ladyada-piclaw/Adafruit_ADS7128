/*!
 * @file alert_buttons.ino
 * @brief ADS7128 button interrupt example using DWC + ALERT pin
 *
 * Monitors CH0 as an analog input with a low-voltage threshold.
 * When a button pulls CH0 below the threshold, the ALERT pin fires.
 *
 * Hardware:
 * - CH0: 10K pull-up to AVDD, button to GND
 * - ALERT pin connected to Arduino interrupt pin (D3 on Uno/Metro)
 * - Note: ADS7128 has no internal pull-ups, external 10K required
 */

#include <Adafruit_ADS7128.h>
#include <Wire.h>

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
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("ADS7128 Button Interrupt Example"));
  Serial.println(F("================================"));

  Wire.begin();

  if (!ads.begin()) {
    Serial.println(F("Failed to find ADS7128!"));
    while (1) {
      delay(100);
    }
  }

  // CH0 defaults to analog input — that's what we want

  // Set low threshold on CH0: ~0.5V (button press = GND)
  uint16_t threshold = (uint16_t)(0.5 / ADS_VREF * 4095);
  ads.setLowThreshold(BUTTON_CH, threshold);
  ads.setHighThreshold(BUTTON_CH, 4095); // don't trigger on high

  // Enable DWC and alert on CH0 only
  ads.enableDWC(true);
  ads.setAlertChannels(1 << BUTTON_CH);
  ads.configureAlert(true, 0); // push-pull, active low

  // Start autonomous sequence on CH0
  ads.setSequenceChannels(1 << BUTTON_CH);
  ads.startSequence();

  // Clear any stale events
  ads.clearEventFlags();

  // Attach interrupt
  ::pinMode(ALERT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ALERT_PIN), alertISR, FALLING);

  Serial.println(F("Waiting for button press on CH0..."));
  Serial.println(F("(10K pull-up to AVDD, button to GND)"));
  Serial.println();
}

void loop() {
  if (alertFired) {
    alertFired = false;

    uint8_t lowFlags = ads.getEventLowFlags();
    if (lowFlags & (1 << BUTTON_CH)) {
      Serial.println(F("Button pressed!"));
    }

    ads.clearEventFlags();
  }
}
