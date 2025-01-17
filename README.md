# ESPSmsA6Transceiver

A C++ library for the Arduino Core designed for the ESP8266 board that enables GSM transmission and reception using the AI-Thinker A6 GSM module.

This library is an adaptation of
[the A6lib library](https://github.com/skorokithakis/A6lib)

## Getting started

To use ESPSmsA6Transceiver, you need:

* An ESP8266
* An AI-Thinker A6 GSM module
* The Arduino IDE for ESP8266 (version 1.8.8 minimum)
* Basic knowledge of the Arduino environment (upload a sketch, import libraries, ...)

## Installing ESPSmsA6Transceiver

1. Download the latest master source code [.zip](https://github.com/gerald-guiony/ESPSmsA6Transceiver/archive/master.zip)
2. Load the `.zip` with **Sketch → Include Library → Add .ZIP Library**

## Dependency

To use this library you might need to have the latest version of [ESPCoreExtension library](https://github.com/gerald-guiony/ESPCoreExtension)

## A6 GSM module

* [Datasheet](https://github.com/gerald-guiony/ESPSmsA6Transceiver/blob/master/docs/A6_A7_A6C_datasheet-EN.pdf)
* [AT commands](https://github.com/gerald-guiony/ESPSmsA6Transceiver/blob/master/docs/A6_AT_commands.pdf)
* Lot of information [here](https://www.electrodragon.com/w/GSM_GPRS_A6_Module) and [here](https://lastminuteengineers.com/a6-gsm-gprs-module-arduino-tutorial/)
* [Firmware update](https://www.iot-experiments.com/ai-thinker-a6-module-firmware-update/)
* Pinout

![A6 GSM Pinout](https://lastminuteengineers.com/wp-content/uploads/arduino/A6-GSM-GPRS-Module-Pinout.png)
