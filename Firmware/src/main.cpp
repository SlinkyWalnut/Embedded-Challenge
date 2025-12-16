#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <ArduinoFFT.h>
#include <math.h>
#include "TFT_UI_Helper.h"
#include "sensorData.h"

/* ================= ADXL345 registers ================= */
#define ADXL345_REG_THRESH_ACT   0x24
#define ADXL345_REG_ACT_INACT    0x27
#define ADXL345_REG_INT_ENABLE   0x2E
#define ADXL345_REG_INT_MAP      0x2F
#define ADXL345_REG_INT_SOURCE   0x30

/* ================= Sampling config ================= */
#define SAMPLE_RATE_HZ     50
#define SAMPLE_PERIOD_MS  20
#define SAMPLE_COUNT      16   // Reduced to match FFT_SIZE (was 150)

/* ================= FFT config ================= */
#define FFT_SIZE               16   // Reduced from 32 to save RAM/flash (256 -> 128 bytes)
#define FFT_SAMPLING_FREQUENCY 50

#define ADXL_INT_PIN 1

/* ================= Function prototypes ================= */
void writeRegister(char reg, char value);
byte readRegister(char reg);
void isr_twitch();
void TakeSample();
float getMagnitude();
float getPeakFrequency(float vReal[], int bins, float fs);
void insertToBuffer(bool recent);
bool detectDiskinesiaFromFFT(float peakFreq);
bool detectTremorsFromFFT(float peakFreq);
bool Tremor();
void computeBandPercentagesFromFFT(float vReal[], int bins, float fs,
                                   float& tremor_perc, float& disc_perc);

/* ================= Globals ================= */
float vReal[FFT_SIZE];
float vImag[FFT_SIZE];
float sampleBuffer[SAMPLE_COUNT];  // Buffer to store samples during sampling

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

bool TremorBuffer[3] = {false, false, false};
int bufferPointer = 0;

// Global variables for UI (declared as extern in TFT_UI_Helper.h)
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
Adafruit_TSC2007 ts = Adafruit_TSC2007();
Screen currentScreen = SCREEN_HOME;
bool graphScreenDrawn = false;
unsigned long screenInactivityStart = 0;
bool lastTremorDetected = false;
bool lastDyskinesiaDetected = false;
unsigned long lastTremorTime = 0;
unsigned long lastDyskinesiaTime = 0;
int touchStartX = 0;
int touchStartY = 0;
int touchEndX = 0;
int touchEndY = 0;
unsigned long lastTouchTime = 0;
unsigned long touchStartTime = 0;
bool touchActive = false;
bool touchscreenAvailable = false;
bool touchJustEnded = false;
unsigned long lastTouchProcessTime = 0;
bool tremorDataChanged = false;
bool dyskinesiaDataChanged = false;
bool newDataAvailable = false;  // Flag for new sensor data available
SensorData sensorData = {0.0, false, false, 0};

/* ===================================================== */
void setup() {
    Serial.begin(115200);
    delay(500);  // Initial delay
    Serial.println("Starting embedded challenge firmware...");
    
    // Initialize UI (display and touchscreen)
    initializeDisplay();
    initializeTouch();
    drawHomeScreen();
    
    // Initialize ADXL345 accelerometer

    if (!accel.begin()) {
        // Don't halt - continue without sensor
    } else {
        accel.setRange(ADXL345_RANGE_2_G);
        accel.setDataRate(ADXL345_DATARATE_50_HZ);
        
        delay(500);  // Allow sensor to stabilize
        
        writeRegister(ADXL345_REG_THRESH_ACT, 30);
        writeRegister(ADXL345_REG_ACT_INACT, 0x70);
        writeRegister(ADXL345_REG_INT_MAP, 0x00);
        writeRegister(ADXL345_REG_INT_ENABLE, 0x10);
        
        pinMode(ADXL_INT_PIN, INPUT);
        attachInterrupt(digitalPinToInterrupt(ADXL_INT_PIN), isr_twitch, RISING);
        
        readRegister(ADXL345_REG_INT_SOURCE);
    }
}

/* ===================================================== */
void loop() {
    // Handle sensor sampling and FFT processing
    if (motionDetected && !sampling) {
        motionDetected = false;   // Consume trigger (debounce)
        
        readRegister(ADXL345_REG_INT_SOURCE);
        
        sampling = true;
        sampleIndex = 0;
        lastSampleTime = millis();
    }
    
    if (sampling) {
        // Collect sample at regular intervals
        unsigned long now = millis();
        if (now - lastSampleTime >= SAMPLE_PERIOD_MS) {
            sampleBuffer[sampleIndex] = getMagnitude();
            sampleIndex++;
            lastSampleTime = now;
            
            if (sampleIndex >= SAMPLE_COUNT) {
                // All samples collected, process FFT
                TakeSample();
            }
        }
    }
    
    // Update sensor data for UI (convert FFT results to sensorData format)
    // Only update after a complete sample has been processed (not during sampling)
    if (!sampling && sampleIndex > 0) {
        // Update sensor data with FFT results
        bool tremorDetected = Tremor();
        bool dyskinesiaDetected = diskinesia;
        
        // Convert percentages to magnitude (0-10 scale)
        // Use the larger of tremor or dyskinesia percentage, scaled appropriately
        float maxPercent = (tremor_perc > disc_perc) ? tremor_perc : disc_perc;
        float combinedMagnitude = maxPercent / 10.0f;  // Scale to 0-10
        if (combinedMagnitude > 10.0f) combinedMagnitude = 10.0f;
        
        updateSensorData(combinedMagnitude, tremorDetected, dyskinesiaDetected);
        newDataAvailable = true;
        sampleIndex = 0;  // Reset after processing
        
        // Debug output
        Serial.print("Magnitude: ");
        Serial.print(combinedMagnitude);
        Serial.print(" | Tremor: ");
        Serial.print(tremorDetected ? "YES" : "NO");
        Serial.print(" | Dyskinesia: ");
        Serial.print(dyskinesiaDetected ? "YES" : "NO");
        Serial.print(" | Tremor%: ");
        Serial.print(tremor_perc);
        Serial.print(" | Dysk%: ");
        Serial.println(disc_perc);
    }
    
    // UI handling (from TFT_UI_Helper)
    // Only check for changes when new data is available to prevent unnecessary checks
    if (newDataAvailable) {
        checkSensorDataChanges();
        newDataAvailable = false;  // Clear after checking
    }
    handleTouch();
    detectSwipe();
    
    // Inactivity timeout removed
    
    // Update current screen display (only when data changes)
    // Don't update during touch interactions to prevent interrupting swipes
    switch (currentScreen) {
        case SCREEN_HOME:
            // Only update stats if detection states changed AND not touching
            if (!touchActive && (tremorDataChanged || dyskinesiaDataChanged)) {
                tremorDataChanged = false;
                dyskinesiaDataChanged = false;
                updateHomeScreenStats();  // This only updates warning area
            }
            break;
        case SCREEN_GRAPH:
            // Update graph when new sensor data is available
            // Also ensure graph screen is drawn initially
            if (!graphScreenDrawn) {
                updateGraphScreen();  // This will initialize the graph
            } else if (newDataAvailable) {
                updateGraphScreen();
                newDataAvailable = false;  // Clear flag after updating
            } else {
                // For testing: update graph periodically even without new data
                // This allows testing with random values
                static unsigned long lastGraphUpdate = 0;
                if (millis() - lastGraphUpdate > 100) {  // Update every 100ms for testing
                    updateGraphScreen();
                    lastGraphUpdate = millis();
                }
            }
            break;
    }
    
    delay(50);  // Small delay to prevent excessive redraws
}

/* ===================================================== */
void TakeSample() {
    sampling = false;
    
    /* ---------- Prepare FFT input ---------- */
    for (int i = 0; i < FFT_SIZE; i++) {
        if (i < SAMPLE_COUNT) vReal[i] = sampleBuffer[i];
        else vReal[i] = 0.0f;
    }
    
    /* ---------- FFT ---------- */
    FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.compute(FFT_FORWARD);
    FFT.complexToMagnitude();
    
    /* ---------- Feature extraction ---------- */
    peak_freq = getPeakFrequency(vReal, FFT_SIZE / 2,
                                 FFT_SAMPLING_FREQUENCY);
    diskinesia = detectDiskinesiaFromFFT(peak_freq);
    insertToBuffer(detectTremorsFromFFT(peak_freq));
    
    computeBandPercentagesFromFFT(vReal, FFT_SIZE / 2,
                                  FFT_SAMPLING_FREQUENCY,
                                  tremor_perc, disc_perc);
    
    /* ---------- Output removed to save flash memory ---------- */
}

/* ================= Feature functions ================= */

float getPeakFrequency(float vReal[], int bins, float fs){
    float maxAmp = 0.0f;
    float peakFreq = 0.0f;
    
    for (int i = 1; i < bins; i++) {
        float freq = (i * fs) / FFT_SIZE;
        
        if (vReal[i] > maxAmp) {
            maxAmp = vReal[i];
            peakFreq = freq;
        }
    }
    return peakFreq;
}

bool detectDiskinesiaFromFFT(float peakFreq) {
    return (peakFreq >= 5.0f && peakFreq <= 7.0f);
}

bool detectTremorsFromFFT(float peakFreq){
    return (peakFreq >= 3.0f && peakFreq <= 5.0f);
}

void insertToBuffer(bool recent){
    TremorBuffer[bufferPointer] = recent;
    bufferPointer++;
    bufferPointer %= 3;
}

bool Tremor(){
    for(int i = 0; i < 3; i++){
        if(TremorBuffer[i] == false){
            return false;
        }
    }
    return true;
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

float getMagnitude(){
    accel.getEvent(&event);
    
    float x = event.acceleration.x;
    float y = event.acceleration.y;
    float z = event.acceleration.z;
    return sqrt(x*x + y*y + z*z) - 9.802f;
}

/* ================= ISR ================= */
void isr_twitch() {
    motionDetected = true;
}
