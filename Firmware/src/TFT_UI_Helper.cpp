#include "TFT_UI_Helper.h"
#include "graphing.h"
#include <SPI.h>
#include <Wire.h>
#include <Arduino.h>

// Removed Adafruit_TSC2007 include and associated variables

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
            tft.print("T!");
        } else {
            tft.print("D!");
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
    tft.fillRect(20, 200, 100, 30, BLUE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(35, 208);
    tft.print("Home");
    
    // Graph area border
    tft.drawRect(50, 50, 260, 150, WHITE);
}

void updateGraphScreen() {
    extern void updateGraph();
    extern void getComment();
    extern void startGraph();
    
    if (!graphScreenDrawn) {
        drawGraphScreen();
        startGraph();
        lastTremorIntensityDisplayed = -1.0;
        graphScreenDrawn = true;
    }
    
    updateGraph();
    getComment();
    
    // Update magnitude display
    if (abs(sensorData.magnitude - lastTremorIntensityDisplayed) > 1.0) {
        tft.fillRect(150, 210, 80, 20, BLACK); // Clear mag area
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

// --- Simplified Touch/Navigation Logic ---
// Initialize touch now does nothing.
void initializeTouch() {
    // Disabled TSC2007 library initialization to save Flash
    touchscreenAvailable = false;
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
    // Disabled all complex touch and swipe logic to save Flash
    // We only process a simplified check for a button press (simulated)
    // To implement a real button, you would need a physical button connected
    
    // For now, we use a simple timer to toggle screens for testing
    static unsigned long lastToggleTime = 0;
    if (millis() - lastToggleTime > 5000) { // Toggle screen every 5 seconds for testing
        if (currentScreen == SCREEN_HOME) {
            currentScreen = SCREEN_GRAPH;
            drawGraphScreen();
            graphScreenDrawn = false;
        } else {
            currentScreen = SCREEN_HOME;
            drawHomeScreen();
        }
        lastToggleTime = millis();
    }
}

void detectSwipe() {
    // Swipe logic removed entirely
}