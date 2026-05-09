# Hardware Notes

These notes were captured from `esptool` and serial boot logs.

## Core

- MCU: ESP32-D0WDQ6, revision v1.0
- Features: Wi-Fi, Bluetooth, dual core, 240MHz
- External PSRAM: 64Mbit / 8MB
- Flash: 16MB, manufacturer `c8`, device `4018`
- USB-UART: Silicon Labs CP210x

## Display

The running firmware initializes an LVGL display stack.

- Resolution: 320x240
- Display driver: ILI9341
- Orientation: landscape
- SPI host: VSPI
- SPI clock: 40MHz
- MOSI: GPIO23
- SCLK: GPIO18
- MISO: unused
- CS: GPIO14

## Touch

- Touch controller: FT6X36 capacitive touch
- I2C port: 0
- SDA: GPIO21
- SCL: GPIO22
- I2C speed: 100kHz

## Existing Firmware

- Framework: ESP-IDF v4.2
- Project name: C001_Project
- App version: 1
- Compile time: Feb 25 2021 15:55:36

## Observed Runtime

- Wi-Fi driver starts during boot.
- LVGL display helper initializes ILI9341 over SPI.
- FT6X36 touch controller is detected with chip ID `0x64`.
