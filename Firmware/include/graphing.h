

#ifndef GRAPHING_H
#define GRAPHING_H
#include <Adafruit_ILI9341.h>

extern Adafruit_ILI9341 tft;
// extern Adafruit_TSC2007 ts;

void updateGraph();

void getComment();

void back_to_home();

void initialize_g_screen ();

void draw_graph_axis ();

void initialize_graph_setup ();

bool Home_pressed();

#endif