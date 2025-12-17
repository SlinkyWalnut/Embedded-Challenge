#include "TFT_UI_Helper.h"
#include "graphing.h"
#include <SPI.h>
#include <Wire.h>
#include <Arduino.h>

// Static variables for tracking last displayed values
static float lastTremorIntensityDisplayed = -1.0;

void initializeDisplay() {
    // Initialize SPI bus (required for ILI9341)
    SPI.begin();
    SPI.setClockDivider(SPI_CLOCK_DIV2);
    tft.begin();
    tft.setRotation(3);
    tft.fillScreen(BLACK);
}

void drawHomeScreen() {
    tft.fillScreen(BLACK);
    
    // Title
    tft.setTextSize(3);
    tft.setTextColor(WHITE);
    tft.setCursor(10, 10);
    tft.print("Status");
    
    // Graph button at bottom right (simple text button)
    tft.fillRect(200, 200, 100, 30, BLUE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(215, 208);
    tft.print("Graph");
}

void updateHomeScreenStats() {
    // Only update the main status area (100, 80)
    tft.fillRect(50, 80, 220, 100, BLACK); // Clear status area

    tft.setTextSize(4);
    if (sensorData.tremorDetected || sensorData.dyskinesiaDetected) {
        tft.setTextColor(RED);
        tft.setCursor(100, 100);
        if (sensorData.tremorDetected) {
            tft.print("Tremor!");
        } else {
            tft.print("Dyskinesi!");
        }
    } else {
        tft.setTextColor(GREEN);
        tft.setCursor(100, 100);
        tft.print("OK");
    }

    // Reset flags after update
    tremorDataChanged = false;
    dyskinesiaDataChanged = false;
}

void drawGraphScreen() {
    tft.fillScreen(BLACK);
    
    // Title
    tft.setTextSize(3);
    tft.setTextColor(WHITE);
    tft.setCursor(10, 10);
    tft.print("Graph");
    
    // Home button at bottom left (simple text button)
    // Positioned below graph area (y_bottom is 200, so button at 210 to be safe)
    tft.fillRect(20, 210, 100, 30, BLUE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(35, 218);
    tft.print("Home");
    
    // Note: Graph area border will be drawn by startGraph() after clearing
}

void updateGraphScreen() {
    extern void updateGraph();
    extern void getComment();
    extern void startGraph();
    
    if (!graphScreenDrawn) {
        drawGraphScreen();  // Draw static elements (title, home button)
        startGraph();       // Clear graph area and draw axes
        lastTremorIntensityDisplayed = -1.0;
        graphScreenDrawn = true;
        return;  // Don't update graph on first draw, wait for next call
    }
    
    updateGraph();
    getComment();
    
    // Update magnitude display (positioned to not overlap with Home button)
    if (abs(sensorData.magnitude - lastTremorIntensityDisplayed) > 1.0) {
        tft.fillRect(150, 210, 80, 20, BLACK); // Clear mag area (above Home button)
        tft.setTextSize(2);
        tft.setTextColor(BLUE);
        tft.setCursor(150, 210);
        tft.print("M:");
        tft.print((int)sensorData.magnitude);
        lastTremorIntensityDisplayed = sensorData.magnitude;
    }
}

// Helper function for P team to update sensor data
void updateSensorData(float magnitude, bool tremorDetected, bool dyskinesiaDetected) {
    sensorData.magnitude = magnitude;
    sensorData.tremorDetected = tremorDetected;
    sensorData.dyskinesiaDetected = dyskinesiaDetected;
    sensorData.timestamp = millis();
}

// --- Touch/Navigation Logic (tap-only, no swipe) ---
void initializeTouch() {
    // Initialize I2C for TSC2007
    Wire.begin();
    delay(10);
    touchscreenAvailable = ts.begin();
}

void checkSensorDataChanges() {
    // Simplified logic: set flags immediately if state changes
    static bool lastTremorState = false;
    static bool lastDyskState = false;

    if (sensorData.tremorDetected != lastTremorState) {
        if (sensorData.tremorDetected) tremorDataChanged = true;
        lastTremorState = sensorData.tremorDetected;
    }
    if (sensorData.dyskinesiaDetected != lastDyskState) {
        if (sensorData.dyskinesiaDetected) dyskinesiaDataChanged = true;
        lastDyskState = sensorData.dyskinesiaDetected;
    }
}

void handleTouch() {
    if (!touchscreenAvailable) return;

    static unsigned long lastTouchRead = 0;
    unsigned long now = millis();

    // Simple cooldown to avoid processing the same touch multiple times
    if (now - lastTouchRead < TOUCH_COOLDOWN_MS) {
        return;
    }

    TS_Point p = ts.getPoint();

    // No touch detected: many TSC2007 boards report 0/0 or 4095/4095 when idle
    if ((p.x == 0 && p.y == 0) || (p.x == 4095 && p.y == 4095)) {
        return;
    }

    lastTouchRead = now;

    // Map raw touch to screen coordinates for rotation=3 (landscape)
    int x = map(p.y, 0, 4095, tft.width(), 0);
    int y = map(p.x, 0, 4095, 0, tft.height());

    if (currentScreen == SCREEN_HOME) {
        // Graph button rectangle: x 200-300, y 200-230
        if (x >= 200 && x <= 300 && y >= 200 && y <= 230) {
            currentScreen = SCREEN_GRAPH;
            graphScreenDrawn = false;  // Force graph screen to initialize on next update
        }
    } else if (currentScreen == SCREEN_GRAPH) {
        // Home button rectangle: x 20-120, y 210-239
        if (x >= 20 && x <= 120 && y >= 210 && y <= 239) {
            currentScreen = SCREEN_HOME;
            graphScreenDrawn = false;
            drawHomeScreen();
        }
    }
}

void detectSwipe() {
    // Swipe logic removed entirely
}
