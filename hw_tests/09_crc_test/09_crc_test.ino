/*!
 * @file 09_crc_test.ino
 * @brief ADS7128 CRC Test - Verifies CRC works for reads/writes via library API
 *
 * Hardware setup:
 * - ADS7128 at default address 0x10
 * - AVDD/DVDD = 5V
 * - Channels A0-A7 chained with 10K resistors: A0—10K—A1—10K—...—A7
 *
 * Tests:
 * 1. GPIO round-trip with CRC enabled
 * 2. ADC read with CRC enabled
 * 3. Bulk reads with CRC (20 reads)
 * 4. CRC disable/re-enable transitions
 */

#include <Adafruit_ADS7128.h>
#include <Wire.h>

Adafruit_ADS7128 ads;

uint8_t testsRun = 0;
uint8_t testsPassed = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println(F("ADS7128 CRC Test"));
  Serial.println(F("------------------"));
  Serial.println();

  Wire.begin();

  if (!ads.begin()) {
    Serial.println(F("ERROR: ADS7128 not found!"));
    while (1)
      ;
  }

  // Test 1: GPIO round-trip with CRC
  Serial.println(F("Test 1: GPIO round-trip with CRC"));
  testsRun++;

  ads.enableCRC(true);
  ads.clearCRCError();

  // Configure CH0 as output, CH1 as input
  ads.pinMode(0, ADS7128_OUTPUT);
  ads.pinMode(1, ADS7128_INPUT);

  // Test HIGH
  ads.digitalWrite(0, true);
  delay(1);
  bool val = ads.digitalRead(1);
  Serial.print(F("  CH0=HIGH, CH1="));
  Serial.print(val ? F("HIGH") : F("LOW"));
  if (val) {
    Serial.println(F(" ... PASS"));
    testsPassed++;
  } else {
    Serial.println(F(" ... FAIL"));
  }

  // Test LOW
  testsRun++;
  ads.digitalWrite(0, false);
  delay(1);
  val = ads.digitalRead(1);
  Serial.print(F("  CH0=LOW, CH1="));
  Serial.print(val ? F("HIGH") : F("LOW"));
  if (!val) {
    Serial.println(F(" ... PASS"));
    testsPassed++;
  } else {
    Serial.println(F(" ... FAIL"));
  }
  Serial.println();

  // Test 2: ADC read with CRC
  Serial.println(F("Test 2: ADC read with CRC"));

  // Set CH0 as output HIGH, CH7 as output LOW for voltage divider
  ads.pinMode(0, ADS7128_OUTPUT);
  ads.pinMode(7, ADS7128_OUTPUT);
  ads.digitalWrite(0, true);
  ads.digitalWrite(7, false);
  delay(10); // Let voltage settle

  // CH3 should be mid-divider
  ads.pinMode(3, ADS7128_ANALOG);
  testsRun++;
  float voltage = ads.analogReadVoltage(3);
  Serial.print(F("  CH3: "));
  uint16_t rawVal = ads.analogRead(3);
  Serial.print(rawVal);
  Serial.print(F(" ("));
  Serial.print(voltage, 2);
  Serial.print(F(" V)"));
  if (voltage >= 1.0 && voltage <= 4.0) {
    Serial.println(F(" ... PASS"));
    testsPassed++;
  } else {
    Serial.println(F(" ... FAIL"));
  }

  testsRun++;
  bool crcErr = ads.getCRCError();
  Serial.print(F("  CRC errors: "));
  Serial.print(crcErr ? F("YES") : F("none"));
  if (!crcErr) {
    Serial.println(F(" ... PASS"));
    testsPassed++;
  } else {
    Serial.println(F(" ... FAIL"));
  }
  Serial.println();

  // Test 3: Bulk reads with CRC
  Serial.println(F("Test 3: Bulk reads with CRC (20 reads)"));
  testsRun++;

  ads.clearCRCError();

  // Set all channels to analog for bulk read
  for (uint8_t ch = 0; ch < 8; ch++) {
    ads.pinMode(ch, ADS7128_ANALOG);
  }

  // Do 20 sequential analog reads
  for (uint8_t i = 0; i < 20; i++) {
    ads.analogRead(i % 8);
  }

  crcErr = ads.getCRCError();
  Serial.print(F("  CRC errors after bulk: "));
  Serial.print(crcErr ? F("YES") : F("none"));
  if (!crcErr) {
    Serial.println(F(" ... PASS"));
    testsPassed++;
  } else {
    Serial.println(F(" ... FAIL"));
  }
  Serial.println();

  // Test 4: CRC disable/re-enable transition
  Serial.println(F("Test 4: CRC disable/re-enable"));

  // Disable CRC
  ads.enableCRC(false);

  // Configure GPIO
  ads.pinMode(0, ADS7128_OUTPUT);
  ads.pinMode(1, ADS7128_INPUT);

  testsRun++;
  ads.digitalWrite(0, true);
  delay(1);
  val = ads.digitalRead(1);
  Serial.print(F("  Without CRC: CH0=HIGH, CH1="));
  Serial.print(val ? F("HIGH") : F("LOW"));
  if (val) {
    Serial.println(F(" ... PASS"));
    testsPassed++;
  } else {
    Serial.println(F(" ... FAIL"));
  }

  // Re-enable CRC
  ads.enableCRC(true);
  ads.clearCRCError();

  testsRun++;
  ads.digitalWrite(0, false);
  delay(1);
  val = ads.digitalRead(1);
  Serial.print(F("  Re-enabled CRC: CH0=LOW, CH1="));
  Serial.print(val ? F("HIGH") : F("LOW"));
  if (!val) {
    Serial.println(F(" ... PASS"));
    testsPassed++;
  } else {
    Serial.println(F(" ... FAIL"));
  }

  // Disable CRC to leave device clean
  ads.enableCRC(false);

  Serial.println();
  Serial.println(F("================================"));
  Serial.print(F("RESULTS: "));
  Serial.print(testsPassed);
  Serial.print(F("/"));
  Serial.print(testsRun);
  Serial.println(F(" tests passed"));
  Serial.println(F("================================"));
}

void loop() {
  // Nothing to do
}
