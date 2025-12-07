#include <Arduino.h>
#include "sensorData.h"
#include "Wire.h"

// put function declarations here:

void setup() {
  // put your setup code here, to run once:
  //Wire.begin();
  Serial.begin(9600);
  DDRC = 0;
  DDRC |= (1 << PC7);
  PORTC &= ~(1<<PC7);
  Serial.println("Hello There");
  Wire.begin();
  SetupSensor();
  

  
}

void loop() {
  // put your main code here, to run repeatedly:
  UpdateSensor();
  Serial.println(GetMagnitude());
}
