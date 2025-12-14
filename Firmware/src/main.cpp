#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <ArduinoFFT.h>
#include <math.h>

/* ================= ADXL345 registers ================= */
#define ADXL345_REG_THRESH_ACT   0x24
#define ADXL345_REG_ACT_INACT    0x27
#define ADXL345_REG_INT_ENABLE   0x2E
#define ADXL345_REG_INT_MAP      0x2F
#define ADXL345_REG_INT_SOURCE   0x30

/* ================= Sampling config ================= */
#define SAMPLE_RATE_HZ     50
#define SAMPLE_PERIOD_MS  20
#define SAMPLE_COUNT      150   // 3 seconds

/* ================= FFT config ================= */
#define FFT_SIZE               128
#define FFT_SAMPLING_FREQUENCY 50

#define ADXL_INT_PIN 1   // ⚠️ 保持你原来能工作的设置

/* ================= Function prototypes ================= */
void writeRegister(char reg, char value);
byte readRegister(char reg);
void isr_twitch();
void TakeSample();

/* Feature functions */
bool detectDiskinesiaFromFFT(float vReal[], int bins, float fs, float& peakFreq);
void computeBandPercentagesFromFFT(float vReal[], int bins, float fs,
                                   float& tremor_perc, float& disc_perc);

/* ================= Globals ================= */
float magnitude[SAMPLE_COUNT];

float vReal[FFT_SIZE];
float vImag[FFT_SIZE];

ArduinoFFT<float> FFT(vReal, vImag, FFT_SIZE, FFT_SAMPLING_FREQUENCY);

Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

volatile bool motionDetected = false;
bool sampling = false;

/* Output features */
bool  diskinesia  = false;
float peak_freq   = 0.0f;
float tremor_perc = 0.0f;
float disc_perc   = 0.0f;

unsigned long lastSampleTime = 0;
int sampleIndex = 0;
sensors_event_t event;

/* ===================================================== */
void setup() {

#ifndef ESP8266
  while (!Serial);
#endif

  Serial.begin(9600);
  Serial.println("ADXL345 FFT feature extraction (50 Hz)");

  if (!accel.begin()) {
    Serial.println("No ADXL345 detected");
    while (1);
  }

  accel.setRange(ADXL345_RANGE_2_G);
  accel.setDataRate(ADXL345_DATARATE_50_HZ);

  delay(500);

  writeRegister(ADXL345_REG_THRESH_ACT, 30);
  writeRegister(ADXL345_REG_ACT_INACT, 0x70);
  writeRegister(ADXL345_REG_INT_MAP, 0x00);
  writeRegister(ADXL345_REG_INT_ENABLE, 0x10);

  pinMode(ADXL_INT_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(ADXL_INT_PIN), isr_twitch, RISING);

  readRegister(ADXL345_REG_INT_SOURCE);

  Serial.println("Waiting for trigger...\n");
}

/* ===================================================== */
void loop() {

  if (motionDetected && !sampling) {
    motionDetected = false;   // 消费 trigger（防抖）

    readRegister(ADXL345_REG_INT_SOURCE);

    sampling = true;
    sampleIndex = 0;
    lastSampleTime = millis();

    Serial.println(">> START 3s CAPTURE <<");
  }

  if (sampling) {
    TakeSample();
  }
}

/* ===================================================== */
void TakeSample() {

  unsigned long now = millis();
  if (now - lastSampleTime < SAMPLE_PERIOD_MS) return;
  lastSampleTime += SAMPLE_PERIOD_MS;

  accel.getEvent(&event);

  float x = event.acceleration.x;
  float y = event.acceleration.y;
  float z = event.acceleration.z;

  magnitude[sampleIndex++] = sqrt(x*x + y*y + z*z) - 9.80665f;

  if (sampleIndex >= SAMPLE_COUNT) {
    sampling = false;

    /* ---------- Prepare FFT input ---------- */
    for (int i = 0; i < FFT_SIZE; i++) {
      if (i < SAMPLE_COUNT) vReal[i] = magnitude[i];
      else vReal[i] = 0.0f;
      vImag[i] = 0.0f;
    }

    /* ---------- FFT ---------- */
    FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.compute(FFT_FORWARD);
    FFT.complexToMagnitude();

    /* ---------- Feature extraction ---------- */
    diskinesia = detectDiskinesiaFromFFT(vReal, FFT_SIZE / 2,
                                         FFT_SAMPLING_FREQUENCY, peak_freq);

    computeBandPercentagesFromFFT(vReal, FFT_SIZE / 2,
                                  FFT_SAMPLING_FREQUENCY,
                                  tremor_perc, disc_perc);

    /* ---------- Output ---------- */
    Serial.print("Peak freq: ");
    Serial.print(peak_freq, 2);
    Serial.print(" Hz | Diskinesia: ");
    Serial.println(diskinesia ? "TRUE" : "FALSE");

    Serial.print("Tremor % (3–5 Hz): ");
    Serial.print(tremor_perc, 1);
    Serial.print(" | Disc % (5–7 Hz): ");
    Serial.println(disc_perc, 1);

    Serial.println(">> READY FOR NEXT TRIGGER <<\n");
  }
}

/* ================= Feature functions ================= */

bool detectDiskinesiaFromFFT(float vReal[], int bins, float fs, float& peakFreq) {

  float maxAmp = 0.0f;
  peakFreq = 0.0f;

  for (int i = 1; i < bins; i++) {
    float freq = (i * fs) / FFT_SIZE;
    if (freq > 10.0f) break;

    if (vReal[i] > maxAmp) {
      maxAmp = vReal[i];
      peakFreq = freq;
    }
  }

  return (peakFreq >= 5.0f && peakFreq <= 7.0f);
}

void computeBandPercentagesFromFFT(float vReal[], int bins, float fs,
                                   float& tremor_perc, float& disc_perc) {

  float tremor_energy = 0.0f;
  float disc_energy   = 0.0f;
  float total_energy  = 0.0f;

  for (int i = 1; i < bins; i++) {
    float freq = (i * fs) / FFT_SIZE;
    float energy = vReal[i] * vReal[i];

    if (freq >= 3.0f && freq < 5.0f) {
      tremor_energy += energy;
      total_energy  += energy;
    } else if (freq >= 5.0f && freq <= 7.0f) {
      disc_energy  += energy;
      total_energy += energy;
    } else if (freq > 7.0f) {
      break;
    }
  }

  if (total_energy > 0.0f) {
    tremor_perc = (tremor_energy / total_energy) * 100.0f;
    disc_perc   = (disc_energy   / total_energy) * 100.0f;
  } else {
    tremor_perc = 0.0f;
    disc_perc   = 0.0f;
  }
}

/* ================= I2C helpers ================= */
void writeRegister(char reg, char value) {
  Wire.beginTransmission(0x53);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

byte readRegister(char reg) {
  Wire.beginTransmission(0x53);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(0x53, 1);
  return Wire.read();
}

/* ================= ISR ================= */
void isr_twitch() {
  motionDetected = true;
}

