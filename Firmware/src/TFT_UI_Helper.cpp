#include "TFT_UI_Helper.h"
#include "graphing.h"  // Include graphing functions
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_TSC2007.h>
#include <Arduino.h>
// Removed string.h - no longer needed after removing Graph label loop
// Removed math.h - no longer needed after simplifying smiley face

// Static variables for tracking last displayed values
static float lastTremorIntensityDisplayed = -1.0;

void initializeDisplay() {
    // Initialize SPI bus (required for ILI9341)
    SPI.begin();
    SPI.setClockDivider(SPI_CLOCK_DIV2);
    delay(10);
    tft.begin();
    delay(50);  // Allow display to initialize
    tft.setRotation(3);
    delay(20);  // Allow rotation to take effect
    tft.fillScreen(BLACK);
    delay(20);  // Ensure screen is cleared
}

void drawHomeScreen() {
    tft.fillScreen(BLACK);
    delay(50);  // Ensure screen is cleared
    
    // Display warning boxes if tremor or dyskinesia detected
    if (sensorData.tremorDetected || sensorData.dyskinesiaDetected) {
        int warningY = 80;
        int warningHeight = 30;
        int warningWidth = 280;
        int warningX = (320 - warningWidth) / 2;
        
        if (sensorData.tremorDetected) {
            tft.fillRect(warningX, warningY, warningWidth, warningHeight, RED);
            tft.setTextSize(2);
            tft.setTextColor(WHITE);
            tft.setCursor(warningX + 100, warningY + 8);
            tft.print("T!");  // Shortened to save flash
            warningY += warningHeight + 10;
        }
        
        if (sensorData.dyskinesiaDetected) {
            tft.fillRect(warningX, warningY, warningWidth, warningHeight, RED);
            tft.setTextSize(2);
            tft.setTextColor(WHITE);
            tft.setCursor(warningX + 100, warningY + 8);
            tft.print("D!");  // Shortened to save flash
        }
    }
    
    // Navigation label at bottom - simpler than triangle, saves flash
    tft.setTextSize(2);
    tft.setTextColor(BLUE);
    tft.setCursor(180, 220);  // Bottom center-right area
    tft.print("Graph -->");
    
}

void updateHomeScreenStats() {
    static bool lastTremorState = false;
    static bool lastDyskinesiaState = false;
    static bool stateInitialized = false;
    
    if (!stateInitialized) {
        stateInitialized = true;
        lastTremorState = sensorData.tremorDetected;
        lastDyskinesiaState = sensorData.dyskinesiaDetected;
        return;
    }
    
    // Only refresh when detection becomes TRUE (detected), not when it becomes false
    bool tremorJustDetected = sensorData.tremorDetected && !lastTremorState;
    bool dyskJustDetected = sensorData.dyskinesiaDetected && !lastDyskinesiaState;
    
    // If nothing new detected, don't refresh
    if (!tremorJustDetected && !dyskJustDetected) {
        // Update last states but don't refresh screen
        lastTremorState = sensorData.tremorDetected;
        lastDyskinesiaState = sensorData.dyskinesiaDetected;
        return;
    }
    
    // Debug output for detection
    if (tremorJustDetected) {
        Serial.println("Tremor detected!");
    }
    if (dyskJustDetected) {
        Serial.println("Dyskinesia detected!");
    }
    
    // Clear only warning area (280x70 max if both warnings)
    tft.fillRect(20, 80, 280, 70, BLACK);
    int wy = 80, h = 30, w = 280, wx = 20;
    
    if (sensorData.tremorDetected) {
        tft.fillRect(wx, wy, w, h, RED);
        tft.setTextSize(2);
        tft.setTextColor(WHITE);
        tft.setCursor(wx + 100, wy + 8);
        tft.print("T!");
        wy += h + 10;
    }
    if (sensorData.dyskinesiaDetected) {
        tft.fillRect(wx, wy, w, h, RED);
        tft.setTextSize(2);
        tft.setTextColor(WHITE);
        tft.setCursor(wx + 100, wy + 8);
        tft.print("D!");
    }
    
    lastTremorState = sensorData.tremorDetected;
    lastDyskinesiaState = sensorData.dyskinesiaDetected;
}

void drawGraphScreen() {
    // Draw static elements only once
    tft.fillScreen(BLACK);
    
    // Title - adjusted to prevent cutoff
    tft.setTextSize(2);
    tft.setTextColor(BLUE);
    tft.setCursor(50, 10);
    tft.print("G");  // Shortened to save flash
    
    // Graph area border (axes will be drawn by draw_graph_axis() in updateGraphScreen)
    tft.drawRect(50, 50, 260, 150, WHITE);

    // H triangle on left edge (for Home)
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
    
    // Note: graphScreenDrawn is set in updateGraphScreen() after startGraph() is called
}

void updateGraphScreen() {
    extern void updateGraph();
    extern void getComment();
    extern void startGraph();
    extern void draw_graph_axis();
    
    if (!graphScreenDrawn) {
        drawGraphScreen();
        startGraph();  // This resets graph state, clears graph area, and draws axes
        lastTremorIntensityDisplayed = -1.0;
        graphScreenDrawn = true;  // Set after initialization is complete
    }
    
    updateGraph();
    getComment();
    
    // Update magnitude display less frequently to save flash
    if (abs(sensorData.magnitude - lastTremorIntensityDisplayed) > 1.0) {
        tft.fillRect(50, 210, 200, 20, BLACK);
        tft.setTextSize(2);
        tft.setTextColor(BLUE);
        tft.setCursor(50, 210);
        tft.print("M:");
        tft.print((int)sensorData.magnitude);  // Integer display saves flash
        lastTremorIntensityDisplayed = sensorData.magnitude;
    }
}


// Removed unused functions (drawWarningBox, drawExpandingTriangle, drawTime, drawLastDetections) to save flash memory

// Helper function for P team to update sensor data
void updateSensorData(float magnitude, bool tremorDetected, bool dyskinesiaDetected) {
    sensorData.magnitude = magnitude;
    sensorData.tremorDetected = tremorDetected;
    sensorData.dyskinesiaDetected = dyskinesiaDetected;
    sensorData.timestamp = millis();
}

void initializeTouch() {
    // Initialize I2C for TSC2007 (V2 uses I2C, not SPI)
    Wire.begin();
    delay(10);
    
    // Simplified touch init - single attempt to save flash
    touchscreenAvailable = ts.begin();
    if (touchscreenAvailable) {
        Serial.println("Touchscreen initialized successfully");
    } else {
        Serial.println("Touchscreen initialization failed!");
    }
}

void checkSensorDataChanges() {
    static float lastMagnitude = -1.0;
    static bool magnitudeInitialized = false;
    static bool lastTremorState = false;
    static bool lastDyskState = false;
    static bool statesInit = false;
    static bool pendingTremor = false;
    static bool pendingDysk = false;
    static unsigned long tremorTime = 0;
    static unsigned long dyskTime = 0;
    static const unsigned long DEBOUNCE_MS = 200;
    
    if (!statesInit) {
        lastTremorState = sensorData.tremorDetected;
        lastDyskState = sensorData.dyskinesiaDetected;
        pendingTremor = sensorData.tremorDetected;
        pendingDysk = sensorData.dyskinesiaDetected;
        statesInit = true;
    }
    
    if (!magnitudeInitialized && sensorData.magnitude > 0.0) {
        lastMagnitude = sensorData.magnitude;
        magnitudeInitialized = true;
    } else if (magnitudeInitialized) {
        float cp = (sensorData.magnitude / 10.0) * 100.0;
        float lp = (lastMagnitude / 10.0) * 100.0;
        if (abs(cp - lp) > 0.1) lastMagnitude = sensorData.magnitude;
    }
    
    // Only check for home screen updates when actually on home screen
    // This prevents setting flags when on graph screen
    if (currentScreen != SCREEN_HOME) {
        return;  // Don't set flags if not on home screen
    }
    
    unsigned long now = millis();
    
    // Only trigger updates when detection becomes TRUE (detected), not when it becomes false
    if (sensorData.tremorDetected != pendingTremor) {
        pendingTremor = sensorData.tremorDetected;
        tremorTime = now;
    } else if (pendingTremor != lastTremorState && now - tremorTime >= DEBOUNCE_MS) {
        // Only set flag if detection became TRUE (not false)
        if (pendingTremor) {
            tremorDataChanged = true;
        }
        lastTremorState = pendingTremor;
    }
    
    if (sensorData.dyskinesiaDetected != pendingDysk) {
        pendingDysk = sensorData.dyskinesiaDetected;
        dyskTime = now;
    } else if (pendingDysk != lastDyskState && now - dyskTime >= DEBOUNCE_MS) {
        // Only set flag if detection became TRUE (not false)
        if (pendingDysk) {
            dyskinesiaDataChanged = true;
        }
        lastDyskState = pendingDysk;
    }
}

void handleTouch() {
    if (!touchscreenAvailable) return;
    if (millis() - lastTouchProcessTime < TOUCH_COOLDOWN_MS && !touchActive) return;

    TS_Point p = ts.getPoint();
    if (p.x != 4095 && p.y != 4095 && (p.x != 0 || p.y != 0)) {
        int x = map(p.y, 0, 4095, tft.width(), 0);
        int y = map(p.x, 0, 4095, 0, tft.height());
        unsigned long now = millis();
        
        if (!touchActive) {
            touchStartX = touchEndX = x;
            touchStartY = touchEndY = y;
            touchActive = true;
            lastTouchTime = touchStartTime = now;
        } else {
            touchEndX = x;
            touchEndY = y;
        }
    } else if (touchActive) {
        touchJustEnded = true;
        touchActive = false;
    }
}

void detectSwipe() {
    if (!touchscreenAvailable || !touchJustEnded) return;
    touchJustEnded = false;

    unsigned long touchDuration = millis() - touchStartTime;
    if (touchDuration < MIN_TOUCH_DURATION_MS) {
        touchStartX = touchStartY = touchEndX = touchEndY = 0;
        return;
    }

    int deltaX = touchEndX - touchStartX;
    int deltaY = touchEndY - touchStartY;
    int totalMovement = abs(deltaX) + abs(deltaY);
    
    if (totalMovement < MIN_SWIPE_MOVEMENT) {
        touchStartX = touchStartY = touchEndX = touchEndY = 0;
        return;
    }
    
    int threshold = 20;  // Reduced to make swipes easier to trigger

    if (abs(deltaX) > threshold && abs(deltaX) > (abs(deltaY) * 1.2)) {  // Reduced vertical ratio from 1.5 to 1.2
        if (currentScreen == SCREEN_GRAPH) {
            // Swipe from left edge (H triangle area) to right to go to home screen
            if (touchStartX < 50 && touchStartY >= 20 && touchStartY <= 220 && deltaX > MIN_SWIPE_MOVEMENT) {
                Serial.println("Swipe detected: GRAPH -> HOME");
                currentScreen = SCREEN_HOME;
                graphScreenDrawn = false;  // Reset so graph restarts when returning
                drawHomeScreen();
            }
        } else if (currentScreen == SCREEN_HOME) {
            // Swipe from right side (where "Graph -->" label is) to left to go to graph screen

            if (touchStartX >= 40 && touchStartX <= 320 && touchStartY >= 10 && touchStartY <= 230 && deltaX < -MIN_SWIPE_MOVEMENT) {
                Serial.println("Swipe detected: HOME -> GRAPH");
                currentScreen = SCREEN_GRAPH;
                graphScreenDrawn = false;  // Reset so graph starts fresh from beginning
                // Don't call drawGraphScreen() here - updateGraphScreen() will handle it to avoid double draw
            }
        }
    }

    touchStartX = touchStartY = touchEndX = touchEndY = 0;
    lastTouchProcessTime = millis();
}

