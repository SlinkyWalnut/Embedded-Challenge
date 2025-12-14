#include <Arduino.h>
#include "sensorData.h"
#include "Wire.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <math.h>

#define ADXL345_REG_THRESH_ACT   0x24
#define ADXL345_REG_ACT_INACT    0x27
#define ADXL345_REG_INT_ENABLE   0x2E
#define ADXL345_REG_INT_MAP      0x2F
#define ADXL345_REG_INT_SOURCE   0x30

#define SAMPLE_RATE_HZ     100
#define SAMPLE_PERIOD_MS  10
#define SAMPLE_COUNT      300   // 3 seconds * 100 Hz
#define ADXL_INT_PIN 1

// -------- function prototypes --------
void writeRegister(char reg, char value);
byte readRegister(char reg);
void isr_twitch();

// -------- globals --------
float magnitude[SAMPLE_COUNT];

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
  Serial.println("Accelerometer magnitude capture\n");

  if (!accel.begin()) {
    Serial.println("No ADXL345 detected");
    while (1);
  }

  accel.setRange(ADXL345_RANGE_2_G);
  accel.setDataRate(ADXL345_DATARATE_50_HZ);

  delay(500);

  // Activity detection
  writeRegister(ADXL345_REG_THRESH_ACT, 75);
  writeRegister(ADXL345_REG_ACT_INACT, 0x70);

  // Map interrupts to INT1
  writeRegister(ADXL345_REG_INT_MAP, 0x00);

  // Enable Activity interrupt
  writeRegister(ADXL345_REG_INT_ENABLE, 0x10);

  pinMode(ADXL_INT_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(ADXL_INT_PIN), isr_twitch, RISING);

  // Clear pending interrupts
  readRegister(ADXL345_REG_INT_SOURCE);

  Serial.println("Waiting for motion...\n");
}

void loop() {

  /* ---------- interrupt trigger ---------- */
  if (motionDetected && !sampling) {

    Serial.println(">> ACTIVITY detected, start 3s capture <<");

    readRegister(ADXL345_REG_INT_SOURCE);
    motionDetected = false;

    sampling = true;
    sampleIndex = 0;
    lastSampleTime = millis();
  }

  /* ---------- sampling ---------- */
  if (sampling) {
    unsigned long now = millis();

    if (now - lastSampleTime >= SAMPLE_PERIOD_MS) {
      lastSampleTime += SAMPLE_PERIOD_MS;

      accel.getEvent(&event);

      float x = event.acceleration.x;
      float y = event.acceleration.y;
      float z = event.acceleration.z;

      // magnitude = sqrt(x^2 + y^2 + z^2)
      magnitude[sampleIndex] = sqrt(x * x + y * y + z * z)-9.802;

      sampleIndex++;

      if (sampleIndex >= SAMPLE_COUNT) {
        sampling = false;

        Serial.println(">> Capture finished (magnitude) <<");

        // sample:print all
        for (int i = 0; i < SAMPLE_COUNT; i++) {
          Serial.println(magnitude[i], 4);
        }

        Serial.println(">> Ready for next trigger <<\n");
      }
    }
  }
}

// -------- low-level I2C helpers --------

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

// -------- ISR --------

void isr_twitch() {
  motionDetected = true;
}
