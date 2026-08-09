# Task 2 – RFID-based WhatsApp Attendance System

PIC16F877A-based RFID attendance system using EM18 RFID reader.

## Components
- PIC16F877A
- EM18 RFID Reader
- 16x2 LCD
- Green LED
- Red LED
- Buzzer
- UART Virtual Terminal

## Function
The system reads a 12-digit RFID tag through UART and checks it against authorized tags.

- Authorized RFID → Access Granted + Green LED
- Unauthorized RFID → Access Denied + Red LED + Buzzer
