/*!
 * @file autonomous_alert_buttons.ino
 * @brief ADS7128 8-button interrupt example using digital inputs + ALERT
 *
 * Detects button presses on all 8 channels without polling over I2C.
 * Channels are configured as digital inputs with the digital window
 * comparator (DWC) monitoring their logic state. When any button pulls
 * a channel low, the ALERT pin fires. The host only needs to respond
 * to the interrupt — no periodic I2C reads required.
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

  Serial.println(F("ADS7128 8-Button Digital Input Alert Example"));
  Serial.println(F("============================================="));
  Serial.println(F("Detects button presses on CH0-CH7 via ALERT interrupt."));
  Serial.println(F("No I2C polling needed — DWC monitors digital inputs."));

  if (!ads.begin()) {
    Serial.println(F("Failed to find ADS7128!"));
    while (1)
      delay(100);
  }

  // Configure all channels as digital inputs
  for (uint8_t ch = 0; ch < NUM_CHANNELS; ch++) {
    ads.pinMode(ch, ADS7128_INPUT);
  }

  // Enable DWC and alert on all 8 channels
  // EVENT_RGN=1 (in-band) so EVENT_LOW_FLAG fires on logic 0 (button press)
  ads.enableDWC(true);
  ads.setAlertChannels(0xFF);  // All channels
  ads.configureAlert(true, 0); // Push-pull, active low
  for (uint8_t ch = 0; ch < NUM_CHANNELS; ch++) {
    ads.setEventRegion(ch, true); // In-band mode for press detection
  }

  // Start autonomous scanning so DWC evaluates digital inputs
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
