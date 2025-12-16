#ifndef TFT_HELPER_H
#define TFT_HELPER_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// Define pins (update if needed)
#define TFT_CS   9
#define TFT_DC   10
#define TFT_RST  -1  // optional, -1 if not connected

class TFT_Helper {
public:
    TFT_Helper(); // constructor

    void begin();                 // initialize TFT
    void clearScreen(uint16_t color = ILI9341_BLACK);  // fill screen
    void printText(const String &text, int16_t x, int16_t y, uint16_t color = ILI9341_WHITE, uint8_t size = 2);
    void drawButton(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t fillColor, uint16_t borderColor, const String &label, uint16_t textColor = ILI9341_WHITE, uint8_t textSize = 2);
    void drawCircle(int16_t x, int16_t y, int16_t r, uint16_t color = ILI9341_WHITE, bool filled = true);
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color = ILI9341_WHITE);
    
    // Access to the TFT object if needed
    Adafruit_ILI9341* getTFT();

private:
    Adafruit_ILI9341 tft;
};

extern TFT_Helper tftHelper;

#endif
