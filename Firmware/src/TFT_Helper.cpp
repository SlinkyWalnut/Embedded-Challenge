#include "TFT_Helper.h"

// Constructor: create TFT object
TFT_Helper::TFT_Helper() : tft(TFT_CS, TFT_DC, TFT_RST) {}

// Initialize TFT
void TFT_Helper::begin() {
    tft.begin();
    tft.setRotation(3); // optional: landscape
    clearScreen();
}

// Clear screen with optional color
void TFT_Helper::clearScreen(uint16_t color) {
    tft.fillScreen(color);
}

// Print text at (x,y)
void TFT_Helper::printText(const String &text, int16_t x, int16_t y, uint16_t color, uint8_t size) {
    tft.setCursor(x, y);
    tft.setTextColor(color);
    tft.setTextSize(size);
    tft.println(text);
}

// Draw a rectangle button with label
void TFT_Helper::drawButton(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t fillColor, uint16_t borderColor, const String &label, uint16_t textColor, uint8_t textSize) {
    tft.fillRect(x, y, w, h, fillColor);
    tft.drawRect(x, y, w, h, borderColor);

    // Center the text
    int16_t xText = x + (w - (label.length() * 6 * textSize)) / 2; // approximate width per char = 6
    int16_t yText = y + (h - (8 * textSize)) / 2;                 // approximate height = 8
    printText(label, xText, yText, textColor, textSize);
}

// Draw circle
void TFT_Helper::drawCircle(int16_t x, int16_t y, int16_t r, uint16_t color, bool filled) {
    if(filled) {
        tft.fillCircle(x, y, r, color);
    } else {
        tft.drawCircle(x, y, r, color);
    }
}

// Draw line
void TFT_Helper::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    tft.drawLine(x0, y0, x1, y1, color);
}

// Return pointer to TFT object
Adafruit_ILI9341* TFT_Helper::getTFT() {
    return &tft;
}
