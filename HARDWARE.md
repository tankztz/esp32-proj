# Hardware Notes

These notes were captured from `esptool`, serial boot logs, reference code, and
visual inspection of the board photo.

## Board

- Front silkscreen: `2009 UP Ahead`
- Form factor: small enclosed ESP32 touchscreen board with side expansion header
- Likely relation: M5Stack Core2-style compatible/mimic board, not a direct
  M5Stack Core2 clone. The side bus layout and major peripherals are similar,
  but several LCD/SPI pins differ from official Core2 documentation.
- Visible connectors:
  - USB-C port
  - TF/microSD card slot
  - Small top-edge card/socket slot
  - 4-pin Grove-style side connector
  - 2-pin battery connector
  - Side expansion socket/header
- Visible controls:
  - Front push button near the printed pin map
  - Side red push button
- Visible printed pin map includes:
  - `G35`, `G36`, `G39` as ADC inputs with adjacent GND labels
  - `G25` DAC/SPK
  - `G26` DAC
  - `G5`, `G2` GPIO
  - `G22` SCL
  - `G21` SDA
  - UART labels: `RXD0/TXD0`, `RXD2/TXD2`
  - SPI labels: `MISO G19`, `MOSI G23`, `SCK G18`
  - Power labels: `5V`, `BAT`, `3.3V`, `GND`

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

## Expansion / Board Pin Labels

The board photo shows a printed pinout next to the expansion connector. Known
labels:

| Board label | Printed function |
| --- | --- |
| G35 | ADC |
| G36 | ADC |
| G39 | ADC |
| G25 | DAC/SPK |
| G26 | DAC |
| G5 | GPIO |
| G2 | GPIO |
| G22 | SCL |
| G21 | SDA |
| G3 | RXD0 |
| G1 | TXD0 |
| G16 | RXD2 |
| G17 | TXD2 |
| G12 | HS |
| G34 | HS |
| G13 | TXD1 |
| G15 | RXD1 |
| G19 | MISO |
| G23 | MOSI |
| G18 | SCK |
| EN | Reset/enable |

## M5Stack Core2 Comparison

Online M5Stack Core2 documentation is a useful reference because this board
appears to follow the same general product idea: ESP32-D0WDQ6, 320x240
touchscreen, microSD, MPU6886-class IMU, CP210x/USB serial, and an M-Bus-style
side connector.

Important differences from official Core2 pin maps:

| Function | This board / tested | Official M5Stack Core2 reference |
| --- | --- | --- |
| LCD controller | ILI9341-compatible behavior | ILI9342C |
| LCD CS | GPIO14 | GPIO5 |
| LCD D/C | GPIO27 | GPIO15 |
| LCD RST | GPIO33 | PMIC/AXP-controlled |
| LCD backlight | GPIO32 | PMIC/AXP-controlled |
| LCD MOSI/SCK | GPIO23 / GPIO18 | GPIO23 / GPIO18 |
| LCD/SD MISO | GPIO19 from board silkscreen/reference code | GPIO38 |
| microSD CS | GPIO4 | GPIO4 |
| Touch I2C | GPIO21/GPIO22, address `0x38` | GPIO21/GPIO22, FT6336U `0x38` |
| Touch INT | GPIO37 in MicroPython reference | GPIO39 |
| IMU | MPU6886 on GPIO21/GPIO22 | MPU6886 on GPIO21/GPIO22 |

Treat M5Stack Core2 documentation as a close reference for architecture and
bus layout, but keep this board's tested pin map as the source of truth.

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
