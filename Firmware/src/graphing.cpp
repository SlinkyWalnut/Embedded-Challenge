#include "graphing.h"
#include "TFT_UI_Helper.h"  // Use shared color definitions and tft object
#include <Arduino.h>

// Graph area coordinates (adjusted for current UI layout with left H triangle)
#define x_end_point 310      // Right edge of graph area
#define x_start_point 50     // Left edge (after 40px triangle + 10px margin)
#define y_top 50             // Top of graph area
#define y_bottom 200         // Bottom of graph area (before warning box)

#define DYSKINESIA_THRESHOLD 30  // Changed to #define to save RAM

int stepx = 1;
int last_xval = -1; 
int last_yval = -1;
int current_xval = x_start_point;
float min_mag_val = 0; 
float max_mag_val = 100;
float min_scale_detect = 0; 
float max_scale_detect = 100;

bool lastCommentHigh = false; 
bool lastCommentInitialized = false;
bool graphActive = true;

// Simplified autoscaling - removed complex float math to save flash
void autoscale_upd(){
    extern SensorData sensorData;
    float mag = sensorData.magnitude;
    if(mag > max_mag_val){
        max_scale_detect = mag + 10;  // Simple margin
    }
    if (mag < min_mag_val){
        min_scale_detect = mag - 10;
        if (min_scale_detect < 0) min_scale_detect = 0;
    }
}

void autoscale(){
    // Simplified - direct update instead of interpolation
    if(max_scale_detect > max_mag_val) max_mag_val = max_scale_detect;
    if(min_scale_detect < min_mag_val) min_mag_val = min_scale_detect;
    if(max_mag_val - min_mag_val < 50) max_mag_val = min_mag_val + 50;
}

void initialize_g_screen (){
    // Clear only the graph area, not the whole screen
    tft.fillRect(x_start_point, y_top, x_end_point - x_start_point, y_bottom - y_top, BLACK);
}

void initialize_graph_setup (){
    draw_graph_axis();  // Draw both X and Y axes
}

void draw_graph_axis (){
    // Draw Y-axis (vertical line on left)
    tft.drawLine(x_start_point, y_top, x_start_point, y_bottom, YELLOW);
    // Draw X-axis (horizontal line on bottom)
    tft.drawLine(x_start_point, y_bottom, x_end_point, y_bottom, YELLOW);
}

void startGraph() {
    graphActive = true;
    lastCommentInitialized = false;
    current_xval = x_start_point;
    last_xval = -1;
    initialize_g_screen();  // Clear graph area
    draw_graph_axis();  // Draw axes after clearing
}

void updateGraph(){
    if (!graphActive) return;
    extern SensorData sensorData;
    
    autoscale_upd();
    autoscale();
    
    // Use sensorData.magnitude for actual data
    // For testing: if magnitude is 0, use random value
    float mag = sensorData.magnitude;
    if (mag == 0.0) {
        mag = random(10, 50);  // Random test value between 10-50
    }
    
    int y = map((int)mag, (int)min_mag_val, (int)max_mag_val, y_bottom, y_top);
    
    if (current_xval >= x_end_point) {
        initialize_g_screen();
        initialize_graph_setup();
        current_xval = x_start_point;
        last_xval = -1;
    }
    
    // Always draw the first point, then connect subsequent points
    if (last_xval == -1) {
        // First point - just set position, don't draw line yet
        last_yval = y;
        last_xval = current_xval;
        current_xval += stepx;
    } else {
        // Draw line from previous point to current point
        tft.drawLine(last_xval, last_yval, current_xval, y, GREEN);
        last_yval = y;
        last_xval = current_xval;
        current_xval += stepx;
    }
}

// Simplified comment function - removed redundant checks
void getComment(){
    if (!graphActive) return;
    extern SensorData sensorData;
    bool currentHigh = sensorData.dyskinesiaDetected || sensorData.tremorDetected;
    if (lastCommentInitialized && currentHigh == lastCommentHigh) return;
    
    tft.fillRect(50, 200, 270, 40, BLACK);
    tft.setTextSize(2);
    if (currentHigh) {
        tft.setTextColor(RED);
        tft.setCursor(100, 210);
        tft.print("W!");  // Shortened to save flash
    } else {
        tft.setTextColor(GREEN);
        tft.setCursor(120, 210);
        tft.print("OK");  // Shortened to save flash
    }
    
    lastCommentHigh = currentHigh;
    lastCommentInitialized = true;
}

// back_to_home() and Home_pressed() removed - navigation handled by swipe detection in TFT_UI_Helper.cpp
// graph_page() removed - graph updates handled by updateGraphScreen() in TFT_UI_Helper.cpp