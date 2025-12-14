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
void writeRegister(char reg, char value);
byte readRegister(char reg);
void isr_twitch();

#define SAMPLE_RATE_HZ   100
#define SAMPLE_PERIOD_MS 10
#define SAMPLE_COUNT     300   // 3 seconds * 100 Hz

uint16_t buffer[SAMPLE_COUNT];

Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

volatile bool motionDetected = false;
bool sampling = false;

unsigned long lastSampleTime = 0;
int sampleIndex = 0;

sensors_event_t event;

void setup() {

#ifndef ESP8266
  while (!Serial);
#endif

  Serial.begin(9600);
  Serial.println("Accelerometer Test\n");

  if (!accel.begin()) {
    Serial.println("Ooops, no ADXL345 detected ... Check your wiring!");
    while (1);
  }

  accel.setRange(ADXL345_RANGE_4_G);
  accel.setDataRate(ADXL345_DATARATE_100_HZ);

  delay(500);

  // Activity detection config
  writeRegister(ADXL345_REG_THRESH_ACT, 20);   // threshold
  writeRegister(ADXL345_REG_ACT_INACT, 0x70);  // enable XYZ activity

  // Map all interrupts to INT1
  writeRegister(ADXL345_REG_INT_MAP, 0x00);

  // Enable Activity interrupt only
  writeRegister(ADXL345_REG_INT_ENABLE, 0x10);

  pinMode(2, INPUT);
  attachInterrupt(digitalPinToInterrupt(2), isr_twitch, RISING);

  // Clear pending interrupts
  readRegister(ADXL345_REG_INT_SOURCE);

  Serial.println("Setup done. Waiting for motion...\n");
}

void loop() {

  /* ---------- interrupt triggered ---------- */
  if (motionDetected && !sampling) {

    Serial.println(">> TWITCH DETECTED! Start 3s capture <<");

    // Clear interrupt source
    readRegister(ADXL345_REG_INT_SOURCE);

    motionDetected = false;

    // Init sampling state
    sampling = true;
    sampleIndex = 0;
    lastSampleTime = millis();
  }

  /* ---------- sampling state ---------- */
  if (sampling) {
    unsigned long now = millis();

    if (now - lastSampleTime >= SAMPLE_PERIOD_MS) {
      lastSampleTime += SAMPLE_PERIOD_MS;

      accel.getEvent(&event);

      // 这里只存一个轴，示例用 X
      // 你可以改成 struct / 三个 buffer
      buffer[sampleIndex] = (int16_t)(event.acceleration.x * 1000); // m/s^2 → mg 量级

      sampleIndex++;

      if (sampleIndex >= SAMPLE_COUNT) {
        sampling = false;

        Serial.println(">> Capture finished <<");
        Serial.println("Dump data:");

        for (int i = 0; i < SAMPLE_COUNT; i++) {
          Serial.println(buffer[i]);
        }

        Serial.println(">> Ready for next trigger <<\n");
      }
    }
  }
}

/* ---------- low-level I2C helpers ---------- */

void writeRegister(char reg, char value) {
  Wire.beginTransmission(0x53);
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

/* ---------- ISR ---------- */

void isr_twitch() {
  motionDetected = true;
}
