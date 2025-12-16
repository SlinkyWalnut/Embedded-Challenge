#define SENSOR_ADDRESS 0x1D
#include <Adafruit_ADXL345_U.h>

#ifndef ASENSOR_H
#define ASENSOR_H

void writeRegister(char reg, char value);

void UpdateSensor();

int GetMagnitude();


#endif