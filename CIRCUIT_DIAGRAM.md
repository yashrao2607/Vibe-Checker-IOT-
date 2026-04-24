# 🔌 Circuit Connection Mapping

This document provides the detailed pin-to-pin wiring for the **Digital Crowd Mood Estimator**.

## 1. Main Controller (Arduino Uno)
| Pin | Component | Description |
| :--- | :--- | :--- |
| **A0** | Potentiometer (SIG) | Simulates Sound Sensor Input |
| **A4** | LCD (SDA) | I2C Data Line |
| **A5** | LCD (SCL) | I2C Clock Line |
| **8** | LED Green (Anode) | CALM indicator |
| **9** | LED Yellow (Anode) | ACTIVE indicator |
| **10** | LED Orange (Anode) | EXCITED indicator |
| **11** | LED Red (Anode) | CHAOTIC indicator |
| **12** | Buzzer (+) | Audible Siren Alert |
| **5V** | VCC Bus | Power supply for LCD & Pot |
| **GND** | Ground Bus | Common return for all components |

## 2. Power & Grounding Details
- **Power**: A common 5V line from the Arduino reaches the LCD VCC and Potentiometer VCC.
- **Ground**: A common GND line connects LCD GND, Potentiometer GND, all 4 LED Cathodes, and the Buzzer (-) terminal.
- **Resistance**: 220Ω resistors are connected in series with each LED to limit current.

## 3. LCD (I2C) Connections
- SDA -> A4
- SCL -> A5
- VCC -> 5V
- GND -> GND

---
**Prepared for IoT Semester Project.**
