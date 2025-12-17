# Embedded Challenge – Tremor & Dyskinesia Monitor

This project is an embedded system for detecting **tremor** and **dyskinesia** from accelerometer data and displaying the results in real time on a TFT touchscreen.

It was built for the **Embedded Challenge: “Shake, Rattle, and Roll”** using an Adafruit Feather, ADXL345 accelerometer, and an ILI9341 TFT display.

---

## Overview

- Uses a **single accelerometer (ADXL345)** to measure hand motion  
- Collects motion data and runs an **FFT** to analyze frequency content  
- Detects:
  - **Tremor** (3–5 Hz)
  - **Dyskinesia** (5–7 Hz)
- Displays status and a **live magnitude graph** on the TFT screen  
- Touchscreen UI with **Home** and **Graph** views

---

## Hardware

- Adafruit Feather (32u4)
- ADXL345 accelerometer (I2C)
- ILI9341 TFT display
- TSC2007 touchscreen controller

---

## Software Structure

### `main.cpp`
- Initializes hardware
- Handles sampling and FFT processing
- Detects tremor and dyskinesia
- Sends processed data to the UI layer

### `graphing.*`
- Real-time scrolling graph
- Auto-scaling based on signal magnitude
- Displays simple status feedback (“OK” / warning)

### `TFT_UI_Helper.*`
- Screen management (Home / Graph)
- Touch input handling
- UI drawing logic
- Shared `SensorData` structure between processing and UI

### `TFT_Helper.*`
- Lightweight wrapper around Adafruit ILI9341
- Basic drawing helpers (text, buttons, lines)

---

## Detection Logic (High Level)

1. Motion interrupt triggers sampling
2. ~3 seconds of data collected at 50 Hz
3. FFT applied to acceleration magnitude
4. Peak frequency extracted
5. Classification:
   - 3–5 Hz → Tremor
   - 5–7 Hz → Dyskinesia
6. Results displayed on screen in real time

---

## UI Behavior

- **Home Screen**
  - Shows overall status (`OK`, `Tremor!`, or `Dyskenisia!`)
- **Graph Screen**
  - Live magnitude graph
  - Auto-scaled Y-axis
  - Simple text warning indicator
- Touch buttons switch between screens

---


---


