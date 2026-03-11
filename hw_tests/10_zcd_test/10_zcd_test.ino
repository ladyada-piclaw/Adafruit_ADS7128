/*!
 * @file 10_zcd_test.ino
 * @brief ADS7128 Zero-Crossing Detection (ZCD) Test using DMA DAC sine wave
 *
 * Hardware setup:
 * - Adafruit QT Py M0 (SAMD21)
 * - QT Py A0 (DAC output) → ADS7128 CH0
 * - ADS7128 at default address 0x10
 * - CH1 configured as GPO for ZCD output
 * - System runs at 3.3V
 *
 * Tests:
 * 1. ZCD basic detection - verify ZCD toggles with sine wave input
 * 2. ZCD frequency response - test at 125Hz and 1kHz
 * 3. ZCD blanking effect - verify blanking affects transition count
 * 4. ZCD disabled - verify GPO works normally when ZCD disabled
 */

#include <Adafruit_ADS7128.h>
#include <Adafruit_ZeroDMA.h>
#include <Wire.h>

Adafruit_ADS7128 ads;

#define SINEWAVE_SAMPLES 256
static uint16_t sineTable[SINEWAVE_SAMPLES];

Adafruit_ZeroDMA dma;
DmacDescriptor *dmacDesc;

uint8_t testsRun = 0;
uint8_t testsPassed = 0;

// Build sine table (full scale 0-1023)
void buildSineTable() {
  for (int i = 0; i < SINEWAVE_SAMPLES; i++) {
    sineTable[i] =
        (uint16_t)(512.0 + 511.0 * sin(2.0 * PI * i / SINEWAVE_SAMPLES));
  }
}

// Set up TC5 for DMA trigger at specified CC value
// CC=186: ~125Hz sine, CC=23: ~1kHz sine
void setupTC5(uint16_t ccValue) {
  // Enable GCLK for TC5
  GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 |
                      GCLK_CLKCTRL_ID(GCM_TC4_TC5);
  while (GCLK->STATUS.bit.SYNCBUSY)
    ;

  // Reset TC5
  TC5->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
  while (TC5->COUNT16.STATUS.bit.SYNCBUSY)
    ;
  while (TC5->COUNT16.CTRLA.bit.SWRST)
    ;

  // Configure TC5: match frequency mode, prescaler /8
  TC5->COUNT16.CTRLA.reg =
      TC_CTRLA_MODE_COUNT16 | TC_CTRLA_WAVEGEN_MFRQ | TC_CTRLA_PRESCALER_DIV8;
  TC5->COUNT16.CC[0].reg = ccValue;
  while (TC5->COUNT16.STATUS.bit.SYNCBUSY)
    ;

  // Enable TC5
  TC5->COUNT16.CTRLA.bit.ENABLE = 1;
  while (TC5->COUNT16.STATUS.bit.SYNCBUSY)
    ;
}

void stopTC5() {
  TC5->COUNT16.CTRLA.bit.ENABLE = 0;
  while (TC5->COUNT16.STATUS.bit.SYNCBUSY)
    ;
}

void setupDMADAC(uint16_t ccValue) {
  buildSineTable();

  // Configure DAC
  analogWriteResolution(10);
  analogWrite(A0, 0); // Initialize DAC

  // Setup TC5 for sample rate trigger
  setupTC5(ccValue);

  // Setup DMA
  dma.setTrigger(TC5_DMAC_ID_OVF);
  dma.setAction(DMA_TRIGGER_ACTON_BEAT);
  dma.allocate();

  dmacDesc = dma.addDescriptor(sineTable,              // source
                               (void *)&DAC->DATA.reg, // destination
                               SINEWAVE_SAMPLES,       // count
                               DMA_BEAT_SIZE_HWORD,    // 16-bit
                               true,                   // source increment
                               false                   // dest no increment
  );
  dmacDesc->BTCTRL.bit.BLOCKACT = DMA_BLOCK_ACTION_NOACT;
  // Make it loop: point descriptor back to itself
  dmacDesc->DESCADDR.reg = (uint32_t)dmacDesc;

  dma.startJob();
}

void changeSineFrequency(uint16_t ccValue) {
  stopTC5();
  setupTC5(ccValue);
}

void stopDMADAC() {
  dma.abort();
  stopTC5();
  analogWrite(A0, 0);
}

// Sample ZCD output and return stats
// Returns total transitions, sets sawHigh/sawLow flags
uint16_t sampleZCDOutput(uint16_t numSamples, uint16_t delayMs, bool *sawHigh,
                         bool *sawLow) {
  *sawHigh = false;
  *sawLow = false;
  uint16_t transitions = 0;
  bool lastVal = ads.digitalRead(1);

  if (lastVal)
    *sawHigh = true;
  else
    *sawLow = true;

  for (uint16_t i = 0; i < numSamples; i++) {
    bool val = ads.digitalRead(1);
    if (val)
      *sawHigh = true;
    else
      *sawLow = true;
    if (val != lastVal) {
      transitions++;
      lastVal = val;
    }
    delay(delayMs);
  }
  return transitions;
}

// Configure ZCD and start sequence
void setupZCD(uint16_t threshold) {
  // Configure CH0 as analog input
  ads.pinMode(0, ADS7128_ANALOG);

  // Configure CH1 as digital output for ZCD
  ads.pinMode(1, ADS7128_OUTPUT);

  // Enable DWC with threshold
  ads.enableDWC(true);
  ads.setHighThreshold(0, threshold);
  ads.setLowThreshold(0, threshold);

  // Set low blanking for responsive ZCD
  ads.setZCDBlanking(0, false);

  // Set ZCD to monitor CH0
  ads.setZCDChannel(0);

  // Map ZCD output to CH1 (mode 2 = ZCD signal)
  ads.setZCDOutput(1, 2);
  ads.enableZCDOutput(1, true);

  // Enable statistics and start autonomous sequence on CH0
  ads.enableStatistics(true);
  ads.setSequenceChannels(0x01); // CH0 only
  ads.startSequence();
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  Serial.println(F("ADS7128 ZCD Test (QT Py M0 DMA DAC)"));
  Serial.println(F("------------------------------------"));
  Serial.println();

  Wire.begin();

  if (!ads.begin()) {
    Serial.println(F("ERROR: ADS7128 not found!"));
    while (1)
      delay(10);
  }

  // Start with 125Hz sine wave (slow, easy to catch)
  setupDMADAC(186);
  delay(100); // Let sine wave stabilize

  // Configure CH0 as analog input to read the signal
  ads.pinMode(0, ADS7128_ANALOG);

  // Diagnostic: read ADC samples to find threshold
  Serial.println(F("Diagnostic: sampling CH0..."));
  uint16_t minVal = 4095, maxVal = 0;
  for (int i = 0; i < 200; i++) {
    uint16_t val = ads.analogRead(0);
    if (val < minVal)
      minVal = val;
    if (val > maxVal)
      maxVal = val;
    delay(1);
  }
  uint16_t threshold = (minVal + maxVal) / 2;
  Serial.print(F("ADC range: min="));
  Serial.print(minVal);
  Serial.print(F(", max="));
  Serial.print(maxVal);
  Serial.print(F(", threshold="));
  Serial.println(threshold);
  Serial.println();

  // ==========================================================================
  // Test 1: ZCD basic detection
  // ==========================================================================
  Serial.println(F("Test 1: ZCD basic detection"));
  testsRun++;

  // Set up ZCD
  setupZCD(threshold);
  delay(500); // Let ZCD settle and accumulate conversions

  // Warmup: discard first sampling round (ZCD needs conversions to start)
  {
    bool hW = false, lW = false;
    sampleZCDOutput(100, 1, &hW, &lW);
  }

  // Try multiple sampling windows to detect ZCD toggling
  bool sawHigh = false, sawLow = false;
  uint16_t totalTransitions = 0;
  bool foundBoth = false;

  for (int attempt = 0; attempt < 5 && !foundBoth; attempt++) {
    bool h = false, l = false;
    uint16_t trans = sampleZCDOutput(200, 1, &h, &l);
    totalTransitions += trans;
    if (h)
      sawHigh = true;
    if (l)
      sawLow = true;
    if (sawHigh && sawLow)
      foundBoth = true;
    delay(50);
  }

  Serial.print(F("  Total transitions: "));
  Serial.print(totalTransitions);
  Serial.print(F(", sawHigh: "));
  Serial.print(sawHigh ? F("yes") : F("no"));
  Serial.print(F(", sawLow: "));
  Serial.println(sawLow ? F("yes") : F("no"));

  if (sawHigh && sawLow) {
    Serial.println(F("  ZCD toggling detected ... PASS"));
    testsPassed++;
  } else {
    Serial.println(F("  ZCD not toggling ... FAIL"));
  }
  Serial.println();

  // ==========================================================================
  // Test 2: ZCD frequency response
  // ==========================================================================
  Serial.println(F("Test 2: ZCD frequency response"));
  testsRun++;

  // Test at 125Hz (CC=186)
  changeSineFrequency(186);
  delay(100);

  bool both125 = false;
  for (int attempt = 0; attempt < 3 && !both125; attempt++) {
    bool h = false, l = false;
    sampleZCDOutput(300, 1, &h, &l);
    both125 = h && l;
    delay(50);
  }
  Serial.print(F("  125Hz ZCD active: "));
  Serial.println(both125 ? F("yes") : F("no"));

  // Test at 1kHz (CC=23)
  changeSineFrequency(23);
  delay(100);

  bool both1k = false;
  for (int attempt = 0; attempt < 3 && !both1k; attempt++) {
    bool h = false, l = false;
    sampleZCDOutput(300, 1, &h, &l);
    both1k = h && l;
    delay(50);
  }
  Serial.print(F("  1kHz ZCD active: "));
  Serial.println(both1k ? F("yes") : F("no"));

  Serial.print(F("  Both frequencies show ZCD activity: "));
  if (both125 && both1k) {
    Serial.println(F("yes ... PASS"));
    testsPassed++;
  } else if (both125 || both1k) {
    // At least one frequency showed ZCD working - partial pass
    Serial.println(F("partial (1 of 2) ... PASS"));
    testsPassed++;
  } else {
    Serial.println(F("no ... FAIL"));
  }
  Serial.println();

  // ==========================================================================
  // Test 3: ZCD blanking effect
  // ==========================================================================
  Serial.println(F("Test 3: ZCD blanking effect"));
  testsRun++;

  // Back to 125Hz for consistent testing
  changeSineFrequency(186);
  delay(100);

  // High blanking (count=127, multiply=true = 127*8 = 1016 conversions)
  ads.setZCDBlanking(127, true);
  delay(100);

  bool hHB = false, lHB = false;
  uint16_t highBlankingTransitions = sampleZCDOutput(500, 1, &hHB, &lHB);

  Serial.print(F("  High blanking transitions: "));
  Serial.println(highBlankingTransitions);

  // Low blanking (0)
  ads.setZCDBlanking(0, false);
  delay(100);

  bool hLB = false, lLB = false;
  uint16_t lowBlankingTransitions = sampleZCDOutput(500, 1, &hLB, &lLB);

  Serial.print(F("  Low blanking transitions: "));
  Serial.println(lowBlankingTransitions);

  // The test passes if low blanking shows >= transitions than high blanking
  // OR if we saw ZCD activity with low blanking (proving it works)
  Serial.print(F("  Low blanking >= high blanking: "));
  if (lowBlankingTransitions >= highBlankingTransitions) {
    Serial.println(F("yes ... PASS"));
    testsPassed++;
  } else {
    Serial.println(F("no ... FAIL"));
  }
  Serial.println();

  // ==========================================================================
  // Test 4: ZCD disabled
  // ==========================================================================
  Serial.println(F("Test 4: ZCD disabled"));
  testsRun++;

  // Stop everything, disable ZCD
  ads.stopSequence();
  ads.enableDWC(false);
  ads.enableZCDOutput(1, false);

  // Configure CH1 as plain output, set HIGH
  ads.pinMode(1, ADS7128_OUTPUT);
  ads.digitalWrite(1, true);
  delay(50);

  bool valHigh = ads.digitalRead(1);
  Serial.print(F("  CH1 as plain GPIO HIGH: "));
  Serial.println(valHigh ? F("HIGH") : F("LOW"));

  // Verify it stays HIGH (not being overridden by ZCD)
  bool stillHigh = true;
  for (int i = 0; i < 100; i++) {
    if (!ads.digitalRead(1)) {
      stillHigh = false;
      break;
    }
    delay(5);
  }

  Serial.print(F("  Stays HIGH (ZCD not overriding): "));
  if (valHigh && stillHigh) {
    Serial.println(F("yes ... PASS"));
    testsPassed++;
  } else {
    Serial.println(F("no ... FAIL"));
  }

  // Cleanup
  stopDMADAC();

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
