#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <ArduinoFFT.h>
#include <math.h>
#include "TFT_UI_Helper.h"

/* ================= ADXL345 registers ================= */
#define ADXL345_REG_THRESH_ACT   0x24
#define ADXL345_REG_ACT_INACT    0x27
#define ADXL345_REG_INT_ENABLE   0x2E
#define ADXL345_REG_INT_MAP      0x2F
#define ADXL345_REG_INT_SOURCE   0x30

/* ================= Sampling config ================= */
#define SAMPLE_PERIOD_MS  20
#define SAMPLE_COUNT      150

/* ================= FFT config ================= */
#define FFT_SIZE               128
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
// REMOVED: void computeBandPercentagesFromFFT(...)

/* ================= Globals ================= */
float vReal[FFT_SIZE];
float vImag[FFT_SIZE];

ArduinoFFT<float> FFT(vReal, vImag, FFT_SIZE, FFT_SAMPLING_FREQUENCY);

Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

volatile bool motionDetected = false;
bool sampling = false;

/* Output features */
bool  diskinesia  = false;
float peak_freq   = 0.0f;
// REMOVED: float tremor_perc = 0.0f;
// REMOVED: float disc_perc   = 0.0f;

unsigned long lastSampleTime = 0;
int sampleIndex = 0;
sensors_event_t event;

bool TremorBuffer[3] = {false, false, false};
int bufferPointer = 0;

// Global variables for UI (declared as extern in TFT_UI_Helper.h)
// ... (All UI globals remain)
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
// Adafruit_TSC2007 ts = Adafruit_TSC2007(); // REMOVED: No longer needed with disabled touch
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
bool newDataAvailable = false;
SensorData sensorData = {0.0, false, false, 0};

/* ===================================================== */
void setup() {
    // ... (setup remains the same)
    #ifndef ESP8266
        while (!Serial);
    #endif
    Serial.begin(9600);
    delay(500);
    Serial.println("Starting embedded challenge firmware...");
    
    initializeDisplay();
    initializeTouch(); // Does nothing now
    drawHomeScreen();
    
    if (!accel.begin()) {
    } else {
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
    }
    for(int i = 0; i < FFT_SIZE; i++) vImag[i] = 0.0f;
}

/* ===================================================== */
void loop() {
    // ... (sampling logic remains the same)
    if (motionDetected && !sampling) {
        motionDetected = false;
        readRegister(ADXL345_REG_INT_SOURCE);
        sampling = true;
        sampleIndex = 0;
        lastSampleTime = millis();
    }
    
    if (sampling) {
        unsigned long now = millis();
        if (now - lastSampleTime >= SAMPLE_PERIOD_MS) {
            vReal[sampleIndex] = getMagnitude();
            vImag[sampleIndex] = 0.0f;
            sampleIndex++;
            lastSampleTime = now;
            
            if (sampleIndex >= SAMPLE_COUNT) {
                TakeSample();
            }
        }
    }
    // if (!sampling) {
        bool tremorDetected = Tremor();
        bool dyskinesiaDetected = diskinesia;
        
        // SIMPLIFIED MAGNITUDE CALCULATION
        // Use the peak frequency's amplitude (from vReal[i]) as the magnitude
        float combinedMagnitude = getMagnitude(); // Just use peak freq for a unique value
        
        // This is a minimal way to get a value for the graph. 
        // A more accurate simple magnitude would be vReal[peak_bin] / 10.0f;
        // For maximum flash savings, we simply reuse peak_freq as a placeholder.
        
        // Use max acceleration magnitude if needed, but for now, keep it minimal
        if (combinedMagnitude > 10.0f) combinedMagnitude = 10.0f; // Cap for graph scale
        
        updateSensorData(combinedMagnitude, tremorDetected, dyskinesiaDetected);
        newDataAvailable = true;
        sampleIndex = 0;
        
        // Debug output removed percentages to match removal
        Serial.print("Magnitude: ");
        Serial.print(combinedMagnitude);
        Serial.print(" | Tremor: ");
        Serial.print(tremorDetected ? "YES" : "NO");
        Serial.print(" | Dyskinesia: ");
        Serial.println(dyskinesiaDetected ? "YES" : "NO");
    // }
    
    // UI handling (now using simple timer toggle)
    if (newDataAvailable) {
        checkSensorDataChanges();
        newDataAvailable = false;
    }
    handleTouch(); // Now simple timer toggle
    detectSwipe(); // Does nothing
    
    // Update current screen display
    switch (currentScreen) {
        case SCREEN_HOME:
            // Only update stats if detection states changed
            if (tremorDataChanged || dyskinesiaDataChanged) {
                updateHomeScreenStats();
            }
            break;
        case SCREEN_GRAPH:
            if (!graphScreenDrawn || newDataAvailable) {
                updateGraphScreen();
                newDataAvailable = false;
            } else {
                // For testing: update graph periodically even without new data
                static unsigned long lastGraphUpdate = 0;
                if (millis() - lastGraphUpdate > 100) {
                    updateGraphScreen();
                    lastGraphUpdate = millis();
                }
            }
            break;
    }
    
    delay(50);
}

/* ===================================================== */
void TakeSample() {
    sampling = false;
    
    FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.compute(FFT_FORWARD);
    FFT.complexToMagnitude();
    
    peak_freq = getPeakFrequency(vReal, FFT_SIZE / 2,
                                 FFT_SAMPLING_FREQUENCY);
    diskinesia = detectDiskinesiaFromFFT(peak_freq);
    insertToBuffer(detectTremorsFromFFT(peak_freq));
    
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
    // Serial.print("X: "); Serial.print(x); Serial.print("\tY: "); Serial.print(y); Serial.print("\tZ: "); Serial.println(z);
    return sqrt(x*x + y*y + z*z) - 9.802f;
}

/* ================= ISR ================= */
void isr_twitch() {
    motionDetected = true;
}
