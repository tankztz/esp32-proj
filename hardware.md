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
- D/C: GPIO27
- Reset: GPIO33
- Backlight: GPIO32
- Tested MADCTL value for full-screen output: `0x08`
- Tested drawing size with `MADCTL=0x08`: 320x240
- Notes: ILI9341 native addressing behaves like 240x320, but this panel/module displays correctly when the app uses `MADCTL=0x08` and draws a 320x240 frame. Other common landscape values (`0x28`, `0x68`, `0xa8`, `0xe8`) produced rotation/mirroring issues or incomplete clearing during testing.

## USB Serial

- Normal recovered port after PnP re-enumeration: COM3
- Earlier working port: COM20
- If Windows lists a CP210x COM port but tools cannot open it with `FileNotFoundError`, see `docs/windows-serial-recovery.md`.

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
