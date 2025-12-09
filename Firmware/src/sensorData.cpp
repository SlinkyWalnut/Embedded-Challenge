#include "Wire.h"
#include "sensorData.h"
#include <Arduino.h>

int x, y, z;

void SetupSensor(){
    Wire.beginTransmission(SENSOR_ADDRESS);
    Wire.write(0x2D); // Power Control Register
    Wire.write(0x08); // Measure mode (bit 3 set to 1)
    Wire.endTransmission();

    Wire.beginTransmission(SENSOR_ADDRESS);
    Wire.write(0x31); // Data Format Register
    Wire.write(0x08); // Full resolution, +/-16g (adjust as needed)
    Wire.endTransmission();
}

void UpdateSensor(){
    // Initialize ADXL345
  Wire.beginTransmission(SENSOR_ADDRESS);
  Wire.write(0x32); // Start from DATAX0
  Wire.endTransmission();
  Wire.requestFrom(SENSOR_ADDRESS, 2);
  if (Wire.available() == 2) {
    int x_lsb = Wire.read();
    int x_msb = Wire.read();
    x = (x_msb << 8) | x_lsb; // Combine into 16-bit value
  }

  // Read Y-axis (2 bytes)
  Wire.beginTransmission(SENSOR_ADDRESS);
  Wire.write(0x34); // Start from DATAY0
  Wire.endTransmission();
  Wire.requestFrom(SENSOR_ADDRESS, 2);
  if (Wire.available() == 2) {
    int y_lsb = Wire.read();
    int y_msb = Wire.read();
    y = (y_msb << 8) | y_lsb;
  }

  // Read Z-axis (2 bytes)
  Wire.beginTransmission(SENSOR_ADDRESS);
  Wire.write(036); // Start from DATAZ0
  Wire.endTransmission();
  Wire.requestFrom(SENSOR_ADDRESS, 2);
  if (Wire.available() == 2) {
    int z_lsb = Wire.read();
    int z_msb = Wire.read();
    z = (z_msb << 8) | z_lsb;
  }
}

int GetMagnitude(){
    int x2 = x * x;
    int y2 = y * y;
    int z2 = z*z;
    return sqrt(x2 + y2 + z2);
}

