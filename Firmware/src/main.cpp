#include <Arduino.h>
#include "sensorData.h"
#include "Wire.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

#define ADXL345_REG_THRESH_ACT   0x24
#define ADXL345_REG_ACT_INACT    0x27
#define ADXL345_REG_INT_ENABLE   0x2E
#define ADXL345_REG_INT_MAP      0x2F
#define ADXL345_REG_INT_SOURCE   0x30
// put function declarations here:
uint16_t buffer[300];
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
volatile bool motionDetected = false;
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
  writeRegister(ADXL345_REG_THRESH_ACT, 20);
  writeRegister(ADXL345_REG_ACT_INACT, 0x70);
  writeRegister(ADXL345_REG_ACT_INACT, 0x70); 
  
  // Map all interrupts to the INT1 pin
  writeRegister(ADXL345_REG_INT_MAP, 0x00); 

  // Enable the "Activity" interrupt function
  writeRegister(ADXL345_REG_INT_ENABLE, 0x10);
  pinMode(2, INPUT);
  attachInterrupt(digitalPinToInterrupt(2), isr_twitch, RISING);
  
  // Clear any old interrupts by reading the source register once
  readRegister(ADXL345_REG_INT_SOURCE);
}
sensors_event_t event; 
void loop(void) {
  
  
  // accel.getEvent(&event);
 
  // /* Display the results (acceleration is measured in m/s^2) */
  // Serial.print("X: "); Serial.print(accel.getX()); Serial.print("  ");
  // Serial.print("Y: "); Serial.print(accel.getY()); Serial.print("  ");
  // Serial.print("Z: "); Serial.print(event.acceleration.z); Serial.print("  ");Serial.println("m/s^2 ");
  if (motionDetected) {
    Serial.println(">> TWITCH DETECTED! <<");
    
    // Read the INT_SOURCE register to clear the interrupt
    // If you don't do this, the pin stays HIGH and won't fire again.
    byte interrupts = readRegister(ADXL345_REG_INT_SOURCE);
    
    // Reset our flag so we can wait for the next one
    motionDetected = false;
  }
  //delay(100);
}

void writeRegister(char reg, char value){
  Wire.beginTransmission(0x53); // Default I2C address
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

byte readRegister(char reg) {
  Wire.beginTransmission(0x53);
  Wire.write(reg);
  Wire.endTransmission();
  
  Wire.requestFrom(0x53, 1);
  return Wire.read();
}

void isr_twitch() {
  motionDetected = true;
}