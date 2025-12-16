#include "graphing.h"
#include "TFT_UI_Helper.h"
#include <Arduino.h>

// Graph area coordinates
#define x_end_point 310
#define x_start_point 50
#define y_top 50
#define y_bottom 200

#define DYSKINESIA_THRESHOLD 30  // Kept

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

// Simplified autoscaling - kept simple float math
void autoscale_upd(){
    extern SensorData sensorData;
    float mag = sensorData.magnitude;
    if(mag > max_mag_val){
        max_scale_detect = mag + 10;
    }
    if (mag < min_mag_val){
        min_scale_detect = mag - 10;
        if (min_scale_detect < 0) min_scale_detect = 0;
    }
}

void autoscale(){
    if(max_scale_detect > max_mag_val) max_mag_val = max_scale_detect;
    if(min_scale_detect < min_mag_val) min_mag_val = min_scale_detect;
    if(max_mag_val - min_mag_val < 50) max_mag_val = min_mag_val + 50;
}

void initialize_g_screen (){
    tft.fillRect(x_start_point, y_top, x_end_point - x_start_point, y_bottom - y_top, BLACK);
}

void initialize_graph_setup (){
    draw_graph_axis();
}

void draw_graph_axis (){
    // Draw Y-axis
    tft.drawLine(x_start_point, y_top, x_start_point, y_bottom, YELLOW);
    // Draw X-axis
    tft.drawLine(x_start_point, y_bottom, x_end_point, y_bottom, YELLOW);
}

void startGraph() {
    graphActive = true;
    lastCommentInitialized = false;
    current_xval = x_start_point;
    last_xval = -1;
    initialize_g_screen();
    draw_graph_axis();
}

void updateGraph(){
    if (!graphActive) return;
    extern SensorData sensorData;
    
    autoscale_upd();
    autoscale();
    
    float mag = sensorData.magnitude;
    if (mag == 0.0) {
        mag = random(10, 50);
    }
    
    int y = map((int)mag, (int)min_mag_val, (int)max_mag_val, y_bottom, y_top);
    
    if (current_xval >= x_end_point) {
        initialize_g_screen();
        initialize_graph_setup();
        current_xval = x_start_point;
        last_xval = -1;
    }
    
    if (last_xval == -1) {
        last_yval = y;
        last_xval = current_xval;
        current_xval += stepx;
    } else {
        tft.drawLine(last_xval, last_yval, current_xval, y, GREEN);
        last_yval = y;
        last_xval = current_xval;
        current_xval += stepx;
    }
}

void getComment(){
    if (!graphActive) return;
    extern SensorData sensorData;
    bool currentHigh = sensorData.dyskinesiaDetected || sensorData.tremorDetected;
    if (lastCommentInitialized && currentHigh == lastCommentHigh) return;
    
    // Clear comment area (270x40 area below graph)
    tft.fillRect(50, 200, 270, 40, BLACK); 
    tft.setTextSize(2);
    
    // Simplified printing to save flash
    if (currentHigh) {
        tft.setTextColor(RED);
        tft.setCursor(100, 210);
        tft.print("WARNING!"); 
    } else {
        tft.setTextColor(GREEN);
        tft.setCursor(120, 210);
        tft.print("Status OK"); 
    }
    
    lastCommentHigh = currentHigh;
    lastCommentInitialized = true;
}