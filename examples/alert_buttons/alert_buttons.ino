/*!
 * @file alert_buttons.ino
 * @brief ADS7128 button interrupt example using DWC + ALERT pin
 *
 * Detects button presses on any of the 8 channels using the digital
 * window comparator. When a button pulls a channel below the threshold,
 * the ALERT pin fires an interrupt.
 *
 * Hardware:
 * - Each channel: 10K pull-up to AVDD, button to GND
 * - ALERT pin connected to Arduino interrupt pin (D3 on Uno/Metro)
 * - Note: ADS7128 has no internal pull-ups, external 10K required
 */

#include <Adafruit_ADS7128.h>
#include <Wire.h>

#define ADS_VREF 3.3
#define ALERT_PIN 3

Adafruit_ADS7128 ads;

volatile bool alertFired = false;

void alertISR() { alertFired = true; }

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

  // All channels default to analog input — that's what we want

  // Set low threshold on all channels: ~0.5V (button press = GND)
  uint16_t threshold = (uint16_t)(0.5 / ADS_VREF * 4095);
  for (uint8_t ch = 0; ch < 8; ch++) {
    ads.setLowThreshold(ch, threshold);
    ads.setHighThreshold(ch, 4095); // don't trigger on high
  }

  // Enable DWC and alert on all channels
  ads.enableDWC(true);
  ads.setAlertChannels(0xFF);
  ads.configureAlert(true, 0); // push-pull, active low

  // Start autonomous sequence on all channels
  ads.setSequenceChannels(0xFF);
  ads.startSequence();

  // Clear any stale events
  ads.clearEventFlags();

  // Attach interrupt
  ::pinMode(ALERT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ALERT_PIN), alertISR, FALLING);

  Serial.println(F("Waiting for button presses..."));
  Serial.println(F("(10K pull-up + button to GND on each channel)"));
  Serial.println();
}

void loop() {
  if (alertFired) {
    alertFired = false;

    uint8_t lowFlags = ads.getEventLowFlags();

    for (uint8_t ch = 0; ch < 8; ch++) {
      if (lowFlags & (1 << ch)) {
        Serial.print(F("Button press on CH"));
        Serial.println(ch);
      }
    }

    ads.clearEventFlags();
  }
}
