#ifndef TFT_UI_HELPER_H
#define TFT_UI_HELPER_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_TSC2007.h>

// Data structure for sharing sensor detection data
// between P team (detection algorithms) and U team (UI)
struct SensorData {
    float magnitude;           // Single magnitude value from sensor
    bool tremorDetected;       // Whether tremor is detected
    bool dyskinesiaDetected;   // Whether dyskinesia is detected
    unsigned long timestamp;   // Timestamp of detection
};

// TFT Display pins (for Feather 32u4 with TFT FeatherWing)
#define TFT_CS   9
#define TFT_DC   10
#define TFT_RST  -1  // RST can be set to -1 if sharing Arduino reset pin

// Screen states enum
enum Screen {
    SCREEN_HOME,
    SCREEN_GRAPH,
};

// UI-related constants
#define INACTIVITY_TIMEOUT_MS 10000  // 10 seconds - auto-return to home after inactivity
#define TOUCH_COOLDOWN_MS 300  // Cooldown period after processing a touch
#define MIN_TOUCH_DURATION_MS 50  // Minimum touch duration to be considered valid (ignore noise)
#define MIN_SWIPE_MOVEMENT 5  // Minimum pixel movement to consider it a swipe (not just a tap)

// Extern declarations for global variables accessed by UI functions
extern Adafruit_ILI9341 tft;
extern Adafruit_TSC2007 ts;
extern SensorData sensorData;
extern Screen currentScreen;
extern bool graphScreenDrawn;
extern unsigned long screenInactivityStart;
extern bool lastTremorDetected;
extern bool lastDyskinesiaDetected;
extern unsigned long lastTremorTime;
extern unsigned long lastDyskinesiaTime;

// Touch tracking variables
extern int touchStartX;
extern int touchStartY;
extern int touchEndX;
extern int touchEndY;
extern unsigned long lastTouchTime;
extern unsigned long touchStartTime;
extern bool touchActive;
extern bool touchscreenAvailable;
extern bool touchJustEnded;
extern unsigned long lastTouchProcessTime;
extern bool tremorDataChanged;
extern bool dyskinesiaDataChanged;

// Color definitions (needed for UI functions)
#define BLACK    0x0000
#define BLUE     0x001F
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0
#define WHITE    0xFFFF
#define ORANGE   0xFDA0
#define DARKGRAY 0x4208
#define LIGHTGRAY 0xC618

// Function declarations
void initializeDisplay();
void initializeTouch();
void checkSensorDataChanges();
void handleTouch();
void detectSwipe();
void drawHomeScreen();
void updateHomeScreenStats();
void drawGraphScreen();
void updateGraphScreen();

// Helper function for P team to update sensor data
void updateSensorData(float magnitude, bool tremorDetected, bool dyskinesiaDetected);

#endif
