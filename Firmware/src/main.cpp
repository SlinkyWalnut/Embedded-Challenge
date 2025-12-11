#include <Arduino.h>
#include "sensorData.h"
#include "Wire.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

// put function declarations here:
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
void setup() {
  
  #ifndef ESP8266
  while (!Serial); // for Leonardo/Micro/Zero
  #endif
  Serial.begin(9600);
  Serial.println("Accelerometer Test"); Serial.println("");
  
  /* Initialise the sensor */
  if(!accel.begin())
  {
    /* There was a problem detecting the ADXL345 ... check your connections */
    Serial.println("Ooops, no ADXL345 detected ... Check your wiring!");
    while(1);
  }
  accel.setRange(ADXL345_RANGE_4_G);
  accel.setDataRate(ADXL345_DATARATE_100_HZ);
  sensor_t sensor;
  accel.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" m/s^2");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" m/s^2");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" m/s^2");  
  Serial.println("------------------------------------");
  Serial.println("");
  delay(500);

  
}
sensors_event_t event; 
void loop(void) {
  
  
  accel.getEvent(&event);
 
  /* Display the results (acceleration is measured in m/s^2) */
  Serial.print("X: "); Serial.print(accel.getX()); Serial.print("  ");
  Serial.print("Y: "); Serial.print(accel.getY()); Serial.print("  ");
  Serial.print("Z: "); Serial.print(event.acceleration.z); Serial.print("  ");Serial.println("m/s^2 ");
  delay(100);
}