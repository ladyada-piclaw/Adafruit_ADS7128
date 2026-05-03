/*!
 * @file 06_autonomous_alert_buttons.ino
 * @brief ADS7128 8-button autonomous example using digital inputs + DWC
 *
 * Detects button press and release events on all 8 channels using the
 * digital window comparator (DWC). Channels are configured as digital
 * inputs and the chip auto-scans them in autonomous mode — the host just
 * polls the event flags over I2C. No edge interrupt needed.
 *
 * Hardware:
 * - CH0-CH7: Each with 10K pull-up to AVDD, button to GND
 * - Note: ADS7128 has no internal pull-ups, external 10K required
 *
 * Written by Limor 'ladyada' Fried with assistance from Claude Code for
 * Adafruit Industries. MIT license.
 */

#include <Adafruit_ADS7128.h>

#define NUM_CHANNELS 8

Adafruit_ADS7128 ads;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  Serial.println(F("ADS7128 8-Button Digital Input Alert Example"));
  Serial.println(F("============================================="));
  Serial.println(F("Detects press/release on CH0-CH7 via DWC event flags."));
  Serial.println(F("Chip auto-scans inputs; host just polls flags over I2C."));

  // begin() defaults: address ADS7128_DEFAULT_ADDR (0x10), Wire bus &Wire
  if (!ads.begin(ADS7128_DEFAULT_ADDR, &Wire)) {
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

  // Clear any stale events from power-up
  ads.clearEventFlags();

  Serial.println(F("\nWaiting for button presses on CH0-CH7..."));
  Serial.println(F("(10K pull-up to AVDD per channel, button to GND)"));
  Serial.println();
}

void loop() {
  uint8_t lowFlags = ads.getEventLowFlags();
  uint8_t highFlags = ads.getEventHighFlags();

  // Track each channel's last reported state. The DWC flag stays asserted
  // while the input is in the event region, so we only print transitions.
  static uint8_t pressed = 0;

  for (uint8_t ch = 0; ch < NUM_CHANNELS; ch++) {
    uint8_t mask = 1 << ch;
    if ((lowFlags & mask) && !(pressed & mask)) {
      Serial.print(F("PRESSED  CH"));
      Serial.println(ch);
      pressed |= mask;
    } else if ((highFlags & mask) && (pressed & mask)) {
      Serial.print(F("RELEASED CH"));
      Serial.println(ch);
      pressed &= ~mask;
    }
  }

  if (lowFlags || highFlags) {
    ads.clearEventFlags();
  }

  delay(10);
}
