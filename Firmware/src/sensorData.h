#define SENSOR_ADDRESS 0x1D

#ifndef ASENSOR_H
#define ASENSOR_H

void SetupSensor();

void UpdateSensor();

int GetMagnitude();
#endif