/*!
 * @file 11_rms_test.ino
 * @brief ADS7128 RMS Module Test using DMA DAC sine wave
 *
 * Hardware setup:
 * - Adafruit QT Py M0 (SAMD21)
 * - QT Py A0 (DAC output) → ADS7128 CH0
 * - ADS7128 at default address 0x10
 * - System runs at 3.3V
 *
 * Tests:
 * 1. RMS of sine wave - verify RMS value in expected range
 * 2. RMS with DC subtraction - verify DC-sub RMS < plain RMS
 * 3. RMS amplitude scaling - verify half-amplitude RMS < full-amplitude RMS
 */

#include <Adafruit_ADS7128.h>
#include <Adafruit_ZeroDMA.h>
#include <Wire.h>

Adafruit_ADS7128 ads;

#define SINEWAVE_SAMPLES 256
#define RMS_TIMEOUT_MS 5000
static uint16_t sineTable[SINEWAVE_SAMPLES];

Adafruit_ZeroDMA dma;
DmacDescriptor *dmacDesc;
bool dmaInitialized = false;

uint8_t testsRun = 0;
uint8_t testsPassed = 0;

// Build sine table with configurable amplitude
// fullScale: true = 0-1023, false = 256-768 (half amplitude)
void buildSineTable(bool fullScale) {
  for (int i = 0; i < SINEWAVE_SAMPLES; i++) {
    if (fullScale) {
      sineTable[i] =
          (uint16_t)(512.0 + 511.0 * sin(2.0 * PI * i / SINEWAVE_SAMPLES));
    } else {
      sineTable[i] =
          (uint16_t)(512.0 + 255.0 * sin(2.0 * PI * i / SINEWAVE_SAMPLES));
    }
  }
}

// Set up TC5 for DMA trigger at specified CC value
// CC=23: ~1kHz sine
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

void setupDMADAC(uint16_t ccValue, bool fullScale) {
  buildSineTable(fullScale);

  // Configure DAC
  analogWriteResolution(10);
  analogWrite(A0, 0); // Initialize DAC

  // Setup TC5 for sample rate trigger
  setupTC5(ccValue);

  if (!dmaInitialized) {
    // Setup DMA (only once)
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

    dmaInitialized = true;
  }

  dma.startJob();
}

void restartDMAWithNewTable(bool fullScale) {
  // Stop DMA and timer
  dma.abort();
  stopTC5();

  // Rebuild table with new amplitude
  buildSineTable(fullScale);

  // Restart timer and DMA
  setupTC5(23); // 1kHz
  dma.startJob();
}

void stopDMADAC() {
  dma.abort();
  stopTC5();
  analogWrite(A0, 0);
}

// Helper to run RMS computation and wait for result
uint16_t runRMSComputation(bool dcSub) {
  // Disable RMS first to reset
  ads.enableRMS(false);
  ads.stopSequence();
  delay(10);

  // Configure RMS (must follow this order)
  ads.setRMSChannel(0);
  ads.setRMSSamples(0); // 1024 samples
  ads.setRMSDCSub(dcSub);

  // Enable statistics and start sequence
  ads.enableStatistics(true);
  ads.setSequenceChannels(0x01); // CH0 only
  ads.startSequence();
  delay(5);

  // Enable RMS (starts computation)
  ads.enableRMS(true);

  // Poll for completion
  unsigned long start = millis();
  while (!ads.isRMSDone()) {
    if (millis() - start > RMS_TIMEOUT_MS) {
      Serial.println(F("  ERROR: RMS timeout!"));
      return 0xFFFF;
    }
    delay(1);
  }

  return ads.getRMS();
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  Serial.println(F("ADS7128 RMS Test (QT Py M0 DMA DAC)"));
  Serial.println(F("------------------------------------"));
  Serial.println();

  Wire.begin();

  if (!ads.begin()) {
    Serial.println(F("ERROR: ADS7128 not found!"));
    while (1)
      delay(10);
  }

  // Configure CH0 as analog input
  ads.pinMode(0, ADS7128_ANALOG);

  // Start 1kHz sine wave (CC=23), full scale
  setupDMADAC(23, true);
  delay(50);

  // ==========================================================================
  // Test 1: RMS of sine wave
  // ==========================================================================
  Serial.println(F("Test 1: RMS of sine wave"));
  testsRun++;

  uint16_t rmsSine = runRMSComputation(false);

  Serial.print(F("  RMS value: "));
  Serial.println(rmsSine);

  // For a full-scale sine wave centered at mid-rail, RMS should be in a
  // reasonable range. The exact value depends on ADC scaling.
  // Accept 25000-60000 as a wide tolerance.
  Serial.print(F("  Expected range 25000-60000: "));
  if (rmsSine >= 25000 && rmsSine <= 60000) {
    Serial.println(F("yes ... PASS"));
    testsPassed++;
  } else {
    Serial.println(F("no ... FAIL"));
  }
  Serial.println();

  // ==========================================================================
  // Test 2: RMS with DC subtraction
  // ==========================================================================
  Serial.println(F("Test 2: RMS with DC subtraction"));
  testsRun++;

  // Get plain RMS
  uint16_t rmsPlain = runRMSComputation(false);
  Serial.print(F("  Plain RMS: "));
  Serial.println(rmsPlain);

  // Get DC-subtracted RMS
  uint16_t rmsDCSub = runRMSComputation(true);
  Serial.print(F("  DC-sub RMS: "));
  Serial.println(rmsDCSub);

  // DC-subtracted should be less (sine centered at VCC/2 has DC component)
  Serial.print(F("  DC-sub < plain: "));
  if (rmsDCSub < rmsPlain) {
    Serial.println(F("yes ... PASS"));
    testsPassed++;
  } else {
    Serial.println(F("no ... FAIL"));
  }
  Serial.println();

  // ==========================================================================
  // Test 3: RMS amplitude scaling
  // ==========================================================================
  Serial.println(F("Test 3: RMS amplitude scaling"));
  testsRun++;

  // Get full-amplitude RMS with DC subtraction (AC component only)
  uint16_t rmsFull = runRMSComputation(true);
  Serial.print(F("  Full amplitude RMS (DC-sub): "));
  Serial.println(rmsFull);

  // Switch to half-amplitude sine (256-768 instead of 0-1023)
  restartDMAWithNewTable(false);
  delay(50);

  // Get half-amplitude RMS with DC subtraction (AC component only)
  uint16_t rmsHalf = runRMSComputation(true);
  Serial.print(F("  Half amplitude RMS (DC-sub): "));
  Serial.println(rmsHalf);

  // Half amplitude AC should have lower RMS (at least 20% difference)
  uint16_t threshold = (uint16_t)(rmsFull * 0.8);
  Serial.print(F("  Half RMS < Full RMS (with 20% margin): "));
  if (rmsHalf < threshold) {
    Serial.println(F("yes ... PASS"));
    testsPassed++;
  } else {
    Serial.println(F("no ... FAIL"));
  }

  // Cleanup
  stopDMADAC();
  ads.enableRMS(false);
  ads.stopSequence();

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
