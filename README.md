# 23COA202 Embedded Systems Programming Coursework

This repository contains my coursework submission for **23COA202 Embedded Systems Programming** at **Loughborough University**, Completed in Semester 1, Year 1.

**Final mark:** 86/100

The project is an Arduino-based **parking management system**. It uses an Arduino Uno with an RGB LCD shield to receive vehicle information over the serial interface, store and update vehicle records, and display parking information on the LCD screen.

## Project Summary

The system reads custom serial commands and uses them to manage vehicles in a car park. Each vehicle has a registration number, vehicle type, parking location, entry time, payment status, and exit/payment time.

The LCD displays one vehicle at a time and allows the user to scroll through vehicles using the shield buttons. The display colour changes depending on whether the currently shown vehicle has paid or not.

## Main Features

- Add new vehicles using serial commands
- Update payment status
- Change vehicle type
- Change vehicle location
- Remove vehicles after payment
- Validate serial input and display error messages
- Show vehicle information on a 16x2 LCD screen
- Navigate records using the UP and DOWN buttons
- Display student ID and free SRAM when SELECT is held
- Store vehicle data in EEPROM
- Filter vehicles by paid or unpaid status
- Scroll long parking location names
- Use custom LCD arrow characters

## Skills Demonstrated

This coursework helped develop skills in:

- Arduino C/C++ programming
- Embedded systems design
- Serial communication
- Finite State Machines
- LCD display control
- Button input handling
- EEPROM data storage
- Memory-conscious programming
- Input validation and debugging

## Files

| File | Description |
|---|---|
| `F311453.ino` | Main Arduino source code |

## Hardware

- Arduino Uno
- Adafruit RGB LCD Shield
- USB serial connection
- Arduino IDE

## Notes

The program was written as a single `.ino` file as required by the coursework. It is designed to work with the Arduino Uno and RGB LCD shield provided for the module.

The implementation follows the coursework protocol and includes both the basic requirements and several extension features.