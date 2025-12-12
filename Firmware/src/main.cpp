#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_TSC2007.h>
#include <TimeLib.h>
#include <EEPROM.h>
#include "sensor_data.h"

// TFT Display pins (for Feather 32u4 with TFT FeatherWing)
#define TFT_CS   9
#define TFT_DC   10
#define TFT_RST  -1  // RST can be set to -1 if sharing Arduino reset pin

// Touch screen - V2 uses TSC2007 over I2C (not SPI like V1)
// V2 uses I2C with default address 0x48
// No CS pin needed for I2C

// Color definitions
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

// Screen states
enum Screen {
    SCREEN_HOME,
    SCREEN_TREMOR,
    SCREEN_DYSKINESIA,
    SCREEN_FOG_WARNING  // Freezing of Gait warning overlay
};

// // History filter options
// enum HistoryFilter {
//     FILTER_WEEK,
//     FILTER_MONTH,
//     FILTER_YEAR
// };

// Swipe sensitivity levels (user-friendly severity indicator)
enum SwipeSensitivity {
    SENSITIVITY_MILD,      // For users with minimal motor control issues
    SENSITIVITY_MODERATE,  // Default - balanced for most users
    SENSITIVITY_SEVERE     // For users with significant motor control difficulties
};

// EEPROM addresses for persistent storage
#define EEPROM_SENSITIVITY_ADDR 0

// Swipe threshold mapping (pixels)
// Mapped to sensitivity levels for user-friendly adjustment
#define SWIPE_THRESHOLD_MILD 40      // ~12% of screen - easier swipes
#define SWIPE_THRESHOLD_MODERATE 50  // ~16% of screen - balanced (lowered for easier swiping)
#define SWIPE_THRESHOLD_SEVERE 70    // ~22% of screen - very forgiving

#define TOUCH_DEBOUNCE_MS 200
#define LONG_PRESS_MS 1000  // Long press duration to open settings
#define LONG_PRESS_MOVEMENT_TOLERANCE 30  // Pixels - allows finger movement during long press
                                          // Important for users with tremors who can't hold perfectly still

#define MIN_TOUCH_DURATION_MS 50  // Minimum touch duration to be considered valid (ignore noise)
#define MIN_SWIPE_MOVEMENT 5  // Minimum pixel movement to consider it a swipe (not just a tap)

// Display and touch objects
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
Adafruit_TSC2007 ts = Adafruit_TSC2007();  // V2 uses TSC2007 over I2C

// Current screen state
Screen currentScreen = SCREEN_HOME;
//HistoryFilter currentFilter = FILTER_WEEK;
SwipeSensitivity currentSensitivity = SENSITIVITY_MODERATE;

// Function to get current swipe threshold based on sensitivity
int getSwipeThreshold() {
    switch (currentSensitivity) {
        case SENSITIVITY_MILD:
            return SWIPE_THRESHOLD_MILD;
        case SENSITIVITY_MODERATE:
            return SWIPE_THRESHOLD_MODERATE;
        case SENSITIVITY_SEVERE:
            return SWIPE_THRESHOLD_SEVERE;
        default:
            return SWIPE_THRESHOLD_MODERATE;
    }
}

// Touch tracking for swipe detection
int touchStartX = 0;
int touchStartY = 0;
int touchEndX = 0;
int touchEndY = 0;
unsigned long lastTouchTime = 0;
unsigned long touchStartTime = 0;
bool touchActive = false;
bool touchscreenAvailable = false;  // Track if touchscreen is working

// Current detection data (from P team)
// Defined in sensor_data.h, declared here for global access
DetectionData tremorData = {0.0, 0, false};
DetectionData dyskinesiaData = {0.0, 0, false};
DetectionData fogData = {0.0, 0, false};  // Freezing of Gait

// Flags to trigger UI updates when sensor data changes
bool tremorDataChanged = false;
bool dyskinesiaDataChanged = false;

// History tracking (circular buffer for last N detections)
#define MAX_HISTORY 100
struct HistoryEntry {
    unsigned long timestamp;
    float tremorIntensity;
    float dyskinesiaIntensity;
    bool tremorDetected;
    bool dyskinesiaDetected;
};

// HistoryEntry history[MAX_HISTORY];
// int historyIndex = 0;
// int historyCount = 0;

// Last detection timestamps
unsigned long lastTremorTime = 0;
unsigned long lastDyskinesiaTime = 0;
unsigned long lastFOGTime = 0;
bool fogWarningDismissed = false;  // Track if FOG warning was dismissed

// Store last displayed values to avoid unnecessary redraws
float lastDisplayedTremorPercent = -1.0;
float lastDisplayedDyskPercent = -1.0;

// Track if tremor/dyskinesia screens need initial draw
bool tremorScreenDrawn = false;
bool dyskinesiaScreenDrawn = false;
bool lastTremorDetected = false;
bool lastDyskinesiaDetected = false;

// Screen inactivity timer (return to home after inactivity)
unsigned long screenInactivityStart = 0;
#define INACTIVITY_TIMEOUT_MS 10000  // 10 seconds

// Function declarations
void initializeDisplay();
void initializeTouch();
void updateSensorData();  // Updates UI based on data from P team
void detectSwipe();
void handleTouch();
void drawHomeScreen();
void updateHomeScreenStats();
void drawTremorScreen();
void updateTremorScreen();
void drawDyskinesiaScreen();
void updateDyskinesiaScreen();
void drawWarningBox(const char* message, uint16_t color);
void drawExpandingTriangle(int screenType, int swipeProgress);  // screenType: 0=Tremor, 1=Dyskinesia
//void drawHistoryChart();
//void updateHistory();
//unsigned long getFilterStartTime(HistoryFilter filter);
void drawTime();
void drawLastDetections();
void drawFOGWarningScreen();

// Helper functions for P team to update detection data
void updateTremorData(float intensity, bool detected) {
    tremorData.intensity = intensity;
    tremorData.detected = detected;
    tremorData.timestamp = millis();
}

void updateDyskinesiaData(float intensity, bool detected) {
    dyskinesiaData.intensity = intensity;
    dyskinesiaData.detected = detected;
    dyskinesiaData.timestamp = millis();
}

void updateFOGData(float intensity, bool detected) {
    fogData.intensity = intensity;
    fogData.detected = detected;
    fogData.timestamp = millis();
}


void setup() {
    Serial1.begin(115200);
    delay(1000);  // Wait for serial port to be ready
    
    Serial.println("Starting UI System...");
    
    // Initialize display
    Serial.println("Initializing display...");
    initializeDisplay();
    Serial.println("Display initialized!");
    
    // Initialize touch screen
    Serial.println("Initializing touch screen...");
    initializeTouch();
    Serial.println("Touch screen initialized!");
    
    // Initialize time (you may want to set this from RTC or sync)
    setTime(12, 0, 0, 1, 1, 2025);
    
    // Draw initial home screen
    drawHomeScreen();
    
    Serial.println("UI System Initialized");
}

void loop() {
    // Update UI based on sensor data (provided by P team)
    updateSensorData();
    
    // Handle touch input
    handleTouch();
    
    // Detect swipe gestures
    detectSwipe();
    
    // Check for screen inactivity (auto-return to home)
    // Don't auto-return if FOG warning is showing
    if (currentScreen != SCREEN_HOME && currentScreen != SCREEN_FOG_WARNING) {
        if (millis() - screenInactivityStart > INACTIVITY_TIMEOUT_MS) {
            currentScreen = SCREEN_HOME;
            tremorScreenDrawn = false;  // Reset flags when returning home
            dyskinesiaScreenDrawn = false;
            drawHomeScreen();
        }
    }
    
    // Update current screen display (only when data changes)
    switch (currentScreen) {
        case SCREEN_HOME:
            // Only update stats if sensor data changed
            if (tremorDataChanged || dyskinesiaDataChanged) {
                updateHomeScreenStats();
                tremorDataChanged = false;
                dyskinesiaDataChanged = false;
            }
            break;
        case SCREEN_TREMOR:
            // Only update graph and warning, not full screen
            updateTremorScreen();
            break;
        case SCREEN_DYSKINESIA:
            // Only update graph and warning, not full screen
            updateDyskinesiaScreen();
            break;
        case SCREEN_FOG_WARNING:
            // FOG warning screen is static until dismissed
            break;
    }
    
    delay(50);  // Small delay to prevent excessive redraws
}

void initializeDisplay() {
    // Initialize SPI bus (required for ILI9341)
    SPI.begin();
    // Set SPI mode for ILI9341 (mode 0, MSB first)
    SPI.setClockDivider(SPI_CLOCK_DIV2);  // Faster SPI speed
    delay(10);
    
    // Initialize display
    Serial.println("Initializing TFT display...");
    tft.begin();
    Serial.println("TFT display begin() called");
    
    delay(100);  // Give display time to stabilize
    
    tft.setRotation(3);  // Landscape orientation
    delay(50);
    
    // Quick test - fill screen white then black to verify it's working
    Serial.println("Testing display...");
    tft.fillScreen(WHITE);
    delay(200);
    tft.fillScreen(BLACK);
    delay(100);
    
    // // Test text rendering
    // tft.setTextSize(2);
    // tft.setTextColor(WHITE);
    // tft.setCursor(50, 100);
    // tft.print("Init OK");
    // delay(1000);  // Show test message longer
    
    // // Clear test message before home screen is drawn
    // tft.fillScreen(BLACK);
    // delay(100);
    
    Serial.println("Display ready!");
}

void initializeTouch() {
    Serial.println("Attempting to initialize TSC2007 touchscreen (V2)...");
    Serial.println("V2 uses I2C interface (not SPI like V1)");

    // Initialize I2C for TSC2007 (V2 uses I2C, not SPI)
    Wire.begin();
    delay(10);
    
    // Try to initialize with retry
    // TSC2007 uses I2C with default address 0x48
    bool touchInitSuccess = false;
    for (int i = 0; i < 3; i++) {
        Serial.print("Touchscreen init Attempt ");
        Serial.print(i + 1);
        Serial.print(": Calling ts.begin()... ");

        if (ts.begin()) {
            Serial.println("SUCCESS!");
            touchInitSuccess = true;
            break;
        } else {
            Serial.println("FAILED");
            Serial.println("  -> TSC2007 chip did not respond on I2C");
        }
        delay(200);
    }
    
    if (!touchInitSuccess) {
        Serial.println("ERROR: Couldn't start touchscreen controller after 3 attempts");
        Serial.println("Hardware troubleshooting for V2:");
        Serial.println("1. Verify I2C connections (SDA, SCL)");
        Serial.println("2. Check I2C address (default 0x48)");
        Serial.println("3. Check power supply (touchscreen needs 3.3V)");
        Serial.println("4. Verify TFT FeatherWing V2 is properly seated");
        Serial.println("5. Try reseating the FeatherWing on the Feather 32u4");
        Serial.println("Display will work, but touch/swipe features disabled");
        touchscreenAvailable = false;
    } else {
        Serial.println("Touchscreen initialized successfully!");
        touchscreenAvailable = true;
    }
}

void updateSensorData() {
    // Check if tremor data changed (trigger update)
    static float lastTremorIntensity = 0.0;
    static bool tremorInitialized = false;
    
    if (!tremorInitialized) {
        // First time: initialize and trigger update to show initial value
        lastTremorIntensity = tremorData.intensity;
        tremorDataChanged = true;
        tremorInitialized = true;
    } else if (abs(tremorData.intensity - lastTremorIntensity) > 0.01) {
        // Value changed: trigger update
        tremorDataChanged = true;
        lastTremorIntensity = tremorData.intensity;
    }
    
    // Check if dyskinesia data changed (trigger update)
    static float lastDyskIntensity = 0.0;
    static bool dyskInitialized = false;
    
    if (!dyskInitialized) {
        // First time: initialize and trigger update to show initial value
        lastDyskIntensity = dyskinesiaData.intensity;
        dyskinesiaDataChanged = true;
        dyskInitialized = true;
    } else if (abs(dyskinesiaData.intensity - lastDyskIntensity) > 0.01) {
        // Value changed: trigger update
        dyskinesiaDataChanged = true;
        lastDyskIntensity = dyskinesiaData.intensity;
    }
    
    // Update history when new detections occur (data provided by P team)
    // if (tremorData.detected && tremorData.timestamp != lastTremorTime) {
    //     lastTremorTime = tremorData.timestamp;
    //     updateHistory();
    // }
    // if (dyskinesiaData.detected && dyskinesiaData.timestamp != lastDyskinesiaTime) {
    //     lastDyskinesiaTime = dyskinesiaData.timestamp;
    //     updateHistory();
    // }
    
    // Check for Freezing of Gait detection - show warning overlay
    if (fogData.detected && fogData.timestamp != lastFOGTime && !fogWarningDismissed) {
        lastFOGTime = fogData.timestamp;
        currentScreen = SCREEN_FOG_WARNING;
        drawFOGWarningScreen();
    }
    
    // Reset dismissed flag when FOG is no longer detected
    if (!fogData.detected && fogWarningDismissed) {
        fogWarningDismissed = false;
    }
}

// Flag to indicate touch just ended (for swipe detection)
bool touchJustEnded = false;
unsigned long lastTouchProcessTime = 0;
#define TOUCH_COOLDOWN_MS 300  // Cooldown period after processing a touch

void handleTouch() {
    // Skip if touchscreen is not available
    if (!touchscreenAvailable) {
        return;
    }

    // Cooldown period - ignore touches for a short time after processing one
    if (millis() - lastTouchProcessTime < TOUCH_COOLDOWN_MS && !touchActive) {
        return;
    }

    // TSC2007 (V2) API - getPoint() takes NO arguments and returns TS_Point
    // TS_Point contains x, y, z (pressure) values
    TS_Point p = ts.getPoint();

    // Check if touch is valid (x and y not 4095, which indicates no touch)
    // Also check if not (0,0,0) which indicates error/no touch
    if (p.x != 4095 && p.y != 4095 && (p.x != 0 || p.y != 0)) {
        // Valid touch detected - convert coordinates to screen coordinates
        // TSC2007 returns values in range 0-4095
        // NOTE: Touchscreen axes are rotated 90 degrees relative to display
        // Physical left/right swipes on screen are detected as Y movement by touchscreen
        // Physical up/down swipes on screen are detected as X movement by touchscreen
        // So we swap the axes: TSC2007 X -> screen Y, TSC2007 Y -> screen X
        int x = map(p.y, 0, 4095, tft.width(), 0);   // TSC2007 Y -> screen X (physical left/right)
        int y = map(p.x, 0, 4095, 0, tft.height());  // TSC2007 X -> screen Y (physical up/down)
        
        unsigned long currentTime = millis();
        
        if (!touchActive) {
            // Touch start
            touchStartX = x;
            touchStartY = y;
            touchEndX = x;  // Initialize end position
            touchEndY = y;
            touchActive = true;
            lastTouchTime = currentTime;
            touchStartTime = currentTime;
        } else {
            // Touch continues - update end position
            touchEndX = x;
            touchEndY = y;
        }
    } else {
        // No valid touch detected
        if (touchActive) {
            // Touch just ended - set flag for swipe detection
            touchJustEnded = true;
            touchActive = false;
        }
    }
}

void detectSwipe() {
    // Skip if touchscreen is not available
    if (!touchscreenAvailable) {
        return;
    }
    
    if (touchJustEnded) {
        touchJustEnded = false;  // Clear flag

        // Calculate touch duration
        unsigned long touchDuration = millis() - touchStartTime;
        
        // Ignore very brief touches (likely noise)
        if (touchDuration < MIN_TOUCH_DURATION_MS) {
            // Reset touch tracking and ignore
            touchStartX = 0;
            touchStartY = 0;
            touchEndX = 0;
            touchEndY = 0;
            return;
        }

        int deltaX = touchEndX - touchStartX;
        int deltaY = touchEndY - touchStartY;
        int threshold = getSwipeThreshold();

        // Calculate total movement distance
        int totalMovement = abs(deltaX) + abs(deltaY);
        
        // Ignore touches with very little movement (taps/noise)
        if (totalMovement < MIN_SWIPE_MOVEMENT) {
            // Reset touch tracking and ignore
            touchStartX = 0;
            touchStartY = 0;
            touchEndX = 0;
            touchEndY = 0;
            return;
        }

        // Debug output (only for meaningful touches)
        Serial.print("Touch ended - deltaX: ");
        Serial.print(deltaX);
        Serial.print("(X0: ");
        Serial.print(touchStartX);
        Serial.print(", X1: ");
        Serial.print(touchEndX);
        Serial.print("), deltaY: ");
        Serial.print(deltaY);
        Serial.print("(Y0: ");
        Serial.print(touchStartY);
        Serial.print(", Y1: ");
        Serial.print(touchEndY);
        Serial.print("), threshold: ");
        Serial.print(threshold);
        Serial.print(", duration: ");
        Serial.print(touchDuration);
        Serial.println("ms");

        // Handle FOG warning screen - Dismiss button (check before swipe)
        if (currentScreen == SCREEN_FOG_WARNING) {
            // Dismiss button is at bottom center of screen
            // Button area: x: 100-220, y: 180-220
            if (touchEndY > 180 && touchEndY < 220 && touchEndX > 100 && touchEndX < 220) {
                fogWarningDismissed = true;
                currentScreen = SCREEN_HOME;
                drawHomeScreen();
                return;  // Don't process swipe if button was pressed
            }
        }
        
        // Check if horizontal swipe
        // For Dyskinesia screen: Only swipe from right edge (H triangle) to left -> Home
        // For Tremor screen: Only swipe from left edge (H triangle) to right -> Home
        // For Home screen: Swipe left -> Dyskinesia, Swipe right -> Tremor
        // Make swipe detection more forgiving - horizontal movement should be dominant
        // Use a 1.5:1 ratio (horizontal must be 1.5x vertical) instead of strict >
        if (abs(deltaX) > threshold && abs(deltaX) > (abs(deltaY) * 1.5)) {
            if (currentScreen == SCREEN_DYSKINESIA) {
                // Dyskinesia: Only swipe from right edge (near H triangle) to left -> Home
                // Check if swipe started near right edge (within 100 pixels) and in triangle Y range (20-220)
                if (touchStartX > 135 && touchStartY >= 20 && touchStartY <= 220 && deltaX < 0) {
                    Serial.println("Swipe LEFT from Dyskinesia H triangle -> Home screen");
                    currentScreen = SCREEN_HOME;
                    drawHomeScreen();
                }
                // Ignore all other swipes on Dyskinesia screen
            } else if (currentScreen == SCREEN_TREMOR) {
                // Tremor: Only swipe from left edge (near H triangle) to right -> Home
                // Check if swipe started near left edge (within 50 pixels) and in triangle Y range (20-220)
                if (touchStartX < 33 && touchStartY >= 25 && touchStartY <= 210 && deltaX > 0) {
                    Serial.println("Swipe RIGHT from Tremor H triangle -> Home screen");
                    currentScreen = SCREEN_HOME;
                    drawHomeScreen();
                }
                // Ignore all other swipes on Tremor screen
            } else if (currentScreen == SCREEN_HOME) {
                // Home screen: Normal navigation
                if (deltaX < 0) {
                    // Swipe right -> Tremor
                    Serial.println("Swipe RIGHT -> Tremor screen");
                    currentScreen = SCREEN_TREMOR;
                    tremorScreenDrawn = false;
                    screenInactivityStart = millis();
                    drawTremorScreen();
                } else {
                    // Swipe left -> Dyskinesia
                    Serial.println("Swipe LEFT -> Dyskinesia screen");
                    currentScreen = SCREEN_DYSKINESIA;
                    dyskinesiaScreenDrawn = false;
                    screenInactivityStart = millis();
                    drawDyskinesiaScreen();
                }
            }
        }

        // Reset touch tracking
        touchStartX = 0;
        touchStartY = 0;
        touchEndX = 0;
        touchEndY = 0;

        // Set cooldown to prevent rapid re-detection
        lastTouchProcessTime = millis();
    }
}

void drawHomeScreen() {
    // Clear entire screen first
    tft.fillScreen(BLACK);
    delay(50);  // Ensure screen is cleared
    
    // Title - centered
    tft.setTextSize(3);
    tft.setTextColor(WHITE);
    tft.setCursor(60, 10);
    tft.print("Your Stats:");
    
    // Calculate percentages
    float tremorPercent = (tremorData.intensity / 10.0) * 100.0;
    if (tremorPercent > 100.0) tremorPercent = 100.0;
    
    float dyskPercent = (dyskinesiaData.intensity / 10.0) * 100.0;
    if (dyskPercent > 100.0) dyskPercent = 100.0;
    
    // Tremor percentage - centered
    tft.setTextSize(2);
    tft.setTextColor(BLUE);
    String tremorLabel = "Tremor: ";
    int tremorLabelWidth = tremorLabel.length() * 6 * 2;  // Approximate width
    tft.setCursor((320 - tremorLabelWidth) / 2, 60);
    tft.print(tremorLabel);
    
    // Tremor percentage value - centered below label
    tft.setTextSize(3);
    tft.setTextColor(WHITE);
    String tremorValue = String(tremorPercent, 1) + " %";
    int tremorValueWidth = tremorValue.length() * 6 * 3;  // Approximate width
    tft.setCursor((320 - tremorValueWidth) / 2, 85);
    tft.print(tremorValue);
    
    // Dyskinesia percentage - centered
    tft.setTextSize(2);
    tft.setTextColor(ORANGE);
    String dyskLabel = "Dyskinesia: ";
    int dyskLabelWidth = dyskLabel.length() * 6 * 2;  // Approximate width
    tft.setCursor((320 - dyskLabelWidth) / 2, 130);
    tft.print(dyskLabel);
    
    // Dyskinesia percentage value - centered below label
    tft.setTextSize(3);
    tft.setTextColor(WHITE);
    String dyskValue = String(dyskPercent, 1) + " %";
    int dyskValueWidth = dyskValue.length() * 6 * 3;  // Approximate width
    tft.setCursor((320 - dyskValueWidth) / 2, 155);
    tft.print(dyskValue);
    
    // Triangular indicators on sides (per Final Planning)
    // Large triangles spanning most of the vertical edge
    // Left triangle with "D" for Dyskinesia
    int triangleWidth = 40;  // Width of triangle (how far it extends into screen)
    int triangleTop = 20;    // Top of triangle
    int triangleBottom = 220; // Bottom of triangle (spans most of 240px height)
    int leftTriangleX = 0;
    
    // Left triangle pointing right (base on left edge)
    tft.fillTriangle(leftTriangleX, triangleTop,
                     leftTriangleX, triangleBottom,
                     leftTriangleX + triangleWidth, (triangleTop + triangleBottom) / 2,
                     ORANGE);
    // "D" text in center of triangle
    tft.setTextSize(3);
    tft.setTextColor(BLACK);
    tft.setCursor(leftTriangleX + 10, (triangleTop + triangleBottom) / 2 - 10);
    tft.print("D");
    
    // Right triangle with "T" for Tremor
    int rightTriangleX = 320;
    
    // Right triangle pointing left (base on right edge)
    tft.fillTriangle(rightTriangleX, triangleTop,
                     rightTriangleX, triangleBottom,
                     rightTriangleX - triangleWidth, (triangleTop + triangleBottom) / 2,
                     BLUE);
    // "T" text in center of triangle
    tft.setTextColor(BLACK);
    tft.setCursor(rightTriangleX - triangleWidth + 10, (triangleTop + triangleBottom) / 2 - 10);
    tft.print("T");
    
    // Reset last displayed values so they update on first loop
    lastDisplayedTremorPercent = -1.0;
    lastDisplayedDyskPercent = -1.0;
}

void updateHomeScreenStats() {
    // Calculate percentages
    float tremorPercent = (tremorData.intensity / 10.0) * 100.0;
    if (tremorPercent > 100.0) tremorPercent = 100.0;
    
    float dyskPercent = (dyskinesiaData.intensity / 10.0) * 100.0;
    if (dyskPercent > 100.0) dyskPercent = 100.0;
    
    // Only update if values have changed (to prevent flickering)
    if (abs(tremorPercent - lastDisplayedTremorPercent) > 0.1) {
        // Clear the percentage area for Tremor (centered, wider area to clear)
        tft.fillRect(50, 85, 220, 25, BLACK);
        
        // Display Tremor percentage - centered
        tft.setTextSize(3);
        tft.setTextColor(WHITE);
        String tremorValue = String(tremorPercent, 1) + " %";
        int tremorValueWidth = tremorValue.length() * 6 * 3;  // Approximate width
        tft.setCursor((320 - tremorValueWidth) / 2, 85);
        tft.print(tremorValue);
        
        lastDisplayedTremorPercent = tremorPercent;
    }
    
    // Only update if values have changed
    if (abs(dyskPercent - lastDisplayedDyskPercent) > 0.1) {
        // Clear the percentage area for Dyskinesia (centered, wider area to clear)
        tft.fillRect(50, 155, 220, 25, BLACK);
        
        // Display Dyskinesia percentage - centered
        tft.setTextSize(3);
        tft.setTextColor(WHITE);
        String dyskValue = String(dyskPercent, 1) + " %";
        int dyskValueWidth = dyskValue.length() * 6 * 3;  // Approximate width
        tft.setCursor((320 - dyskValueWidth) / 2, 155);
        tft.print(dyskValue);
        
        lastDisplayedDyskPercent = dyskPercent;
    }
}

void drawTremorScreen() {
    // Draw static elements only once
    tft.fillScreen(BLACK);
    
    // Title
    tft.setTextSize(2);
    tft.setTextColor(BLUE);
    tft.setCursor(100, 10);
    tft.print("TREMOR");
    
    // Graph label
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    tft.setCursor(50, 35);
    tft.print("Tremor Intensity");
    
    // Draw graph area border - adjusted to avoid left H triangle (triangle extends 40px from left edge)
    tft.drawRect(50, 50, 260, 150, WHITE);
    
    // Draw axes
    int graphX = 50;  // Start after triangle (40px triangle + 10px margin)
    int graphY = 50;
    int graphWidth = 260; // Reduced width to fit after triangle
    int graphHeight = 150;
    tft.drawLine(graphX, graphY + graphHeight, graphX + graphWidth, graphY + graphHeight, WHITE);
    tft.drawLine(graphX, graphY, graphX, graphY + graphHeight, WHITE);

    // H triangle on left edge (for History)
    int triangleWidth = 40;
    int triangleTop = 20;
    int triangleBottom = 220;
    int leftTriangleX = 0;
    tft.fillTriangle(leftTriangleX, triangleTop,
                     leftTriangleX, triangleBottom,
                     leftTriangleX + triangleWidth, (triangleTop + triangleBottom) / 2,
                     BLUE);
    tft.setTextSize(3);
    tft.setTextColor(BLACK);
    tft.setCursor(leftTriangleX + 10, (triangleTop + triangleBottom) / 2 - 10);
    tft.print("H");
    
    tremorScreenDrawn = true;
    screenInactivityStart = millis();
}

// Track last displayed values for tremor/dyskinesia screens
static float lastTremorIntensityDisplayed = -1.0;
static float lastDyskIntensityDisplayed = -1.0;

void updateTremorScreen() {
    // Only update dynamic elements: graph area and warning box
    if (!tremorScreenDrawn) {
        drawTremorScreen();
        lastTremorIntensityDisplayed = -1.0;  // Force update on first draw
    }
    
    // Update intensity value only if it changed
    if (abs(tremorData.intensity - lastTremorIntensityDisplayed) > 0.01) {
        tft.fillRect(50, 210, 200, 20, BLACK);
        tft.setTextSize(2);
        tft.setTextColor(BLUE);
        tft.setCursor(50, 210);
        tft.print("Intensity: ");
        tft.print(tremorData.intensity, 2);
        lastTremorIntensityDisplayed = tremorData.intensity;
    }
    
    // this is just to test out the warning box
    tremorData.detected = true;
    // Update warning box only if detection state changed
    if (tremorData.detected != lastTremorDetected) {
        if (tremorData.detected) {
            drawWarningBox("Tremor detected!", RED);
        } else {
            // Clear warning box area
            tft.fillRect(0, 200, 320, 40, BLACK);
        }
        lastTremorDetected = tremorData.detected;
    }
    
    // TODO: Update graph visualization here when FFT data is available from P team
    // Graph updates should only happen when new FFT data arrives
}

void drawDyskinesiaScreen() {
    // Draw static elements only once
    tft.fillScreen(BLACK);
    
    // Title
    tft.setTextSize(2);
    tft.setTextColor(ORANGE);
    tft.setCursor(70, 10);
    tft.print("DYSKINESIA");
    
    // Graph label
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    tft.setCursor(10, 35);
    tft.print("Dyskinesia Intensity");
    
    // Draw graph area border - adjusted to avoid right H triangle (triangle extends 40px from right edge)
    tft.drawRect(10, 50, 260, 150, WHITE);
    
    // Draw axes
    int graphX = 10;
    int graphY = 50;
    int graphWidth = 260;  // Reduced width to fit before triangle (320 - 40 - 10 margin)
    int graphHeight = 150;
    tft.drawLine(graphX, graphY + graphHeight, graphX + graphWidth, graphY + graphHeight, WHITE);
    tft.drawLine(graphX, graphY, graphX, graphY + graphHeight, WHITE);

    // H triangle on right edge (for History)
    int triangleWidth = 40;
    int triangleTop = 20;
    int triangleBottom = 220;
    int rightTriangleX = 320;
    tft.fillTriangle(rightTriangleX, triangleTop,
                     rightTriangleX, triangleBottom,
                     rightTriangleX - triangleWidth, (triangleTop + triangleBottom) / 2,
                     ORANGE);
    tft.setTextSize(3);
    tft.setTextColor(BLACK);
    tft.setCursor(rightTriangleX - triangleWidth + 10, (triangleTop + triangleBottom) / 2 - 10);
    tft.print("H");
    
    dyskinesiaScreenDrawn = true;
    screenInactivityStart = millis();
}

void updateDyskinesiaScreen() {
    // Only update dynamic elements: graph area and warning box
    if (!dyskinesiaScreenDrawn) {
        drawDyskinesiaScreen();
        lastDyskIntensityDisplayed = -1.0;  // Force update on first draw
    }
    
    // Update intensity value only if it changed
    if (abs(dyskinesiaData.intensity - lastDyskIntensityDisplayed) > 0.01) {
        tft.fillRect(10, 210, 200, 20, BLACK);
        tft.setTextSize(2);
        tft.setTextColor(ORANGE);
        tft.setCursor(10, 210);
        tft.print("Intensity: ");
        tft.print(dyskinesiaData.intensity, 2);
        lastDyskIntensityDisplayed = dyskinesiaData.intensity;
    }
    
    // this is just to test out the warning box
    dyskinesiaData.detected = true;
    // Update warning box only if detection state changed
    if (dyskinesiaData.detected != lastDyskinesiaDetected) {
        if (dyskinesiaData.detected) {
            drawWarningBox("Dyskinesia detected!", RED);
        } else {
            // Clear warning box area
            tft.fillRect(0, 200, 320, 40, BLACK);
        }
        lastDyskinesiaDetected = dyskinesiaData.detected;
    }
    
    // TODO: Update graph visualization here when FFT data is available from P team
    // Graph updates should only happen when new FFT data arrives
}

void drawFOGWarningScreen() {
    // Full screen warning overlay for Freezing of Gait detection
    // Per Final Planning: "Warning! Freezing of Gait detected" with Dismiss button
    tft.fillScreen(RED);
    
    // Warning text
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(40, 60);
    tft.print("Warning!");
    
    tft.setTextSize(2);
    tft.setCursor(20, 100);
    tft.print("Freezing of");
    tft.setCursor(20, 130);
    tft.print("Gait detected");
    
    // Dismiss button
    tft.fillRoundRect(100, 180, 120, 40, 5, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(RED);
    tft.setCursor(130, 192);
    tft.print("Dismiss");
}

void drawWarningBox(const char* message, uint16_t color) {
    // Draw warning box at bottom of screen
    tft.fillRect(0, 200, 320, 40, color);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(40, 215);
    tft.print(message);
}

void drawTime() {
    // Draw current time
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    tft.fillRect(10, 30, 150, 15, BLACK);
    tft.setCursor(10, 30);
    tft.print("Time: ");
    
    char timeStr[20];
    sprintf(timeStr, "%02d:%02d:%02d", hour(), minute(), second());
    tft.print(timeStr);
}

void drawLastDetections() {
    // Draw last tremor detection time
    tft.setTextSize(1);
    tft.fillRect(10, 50, 300, 40, BLACK);
    
    tft.setTextColor(BLUE);
    tft.setCursor(10, 50);
    tft.print("Last Tremor: ");
    if (lastTremorTime > 0) {
        unsigned long secondsAgo = (millis() - lastTremorTime) / 1000;
        if (secondsAgo < 60) {
            tft.print(secondsAgo);
            tft.print("s ago");
        } else if (secondsAgo < 3600) {
            tft.print(secondsAgo / 60);
            tft.print("m ago");
        } else {
            tft.print(secondsAgo / 3600);
            tft.print("h ago");
        }
    } else {
        tft.print("Never");
    }
    
    // Draw last dyskinesia detection time
    tft.setTextColor(ORANGE);
    tft.setCursor(10, 65);
    tft.print("Last Dyskinesia: ");
    if (lastDyskinesiaTime > 0) {
        unsigned long secondsAgo = (millis() - lastDyskinesiaTime) / 1000;
        if (secondsAgo < 60) {
            tft.print(secondsAgo);
            tft.print("s ago");
        } else if (secondsAgo < 3600) {
            tft.print(secondsAgo / 60);
            tft.print("m ago");
        } else {
            tft.print(secondsAgo / 3600);
            tft.print("h ago");
        }
    } else {
        tft.print("Never");
    }
}
