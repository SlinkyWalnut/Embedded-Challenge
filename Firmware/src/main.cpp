#include <Arduino.h>
#include "sensorData.h"

// put function declarations here:

void setup() {
  // put your setup code here, to run once:
  //Wire.begin();
  Serial.begin(9600);
  SetupSensor();

  DDRC = 0;
  DDRC |= (1 << PC7);
  PORTC &= ~(1<<PC7);
}

void loop() {
  // put your main code here, to run repeatedly:
  UpdateSensor();
  Serial.print(GetMagnitude());
}
