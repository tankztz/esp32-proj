# 04 Touch UI Shell

First C rebuild step for the original Rubik Cube-1 style firmware.

This program keeps the proven ILI9341 display setup from `03-display-hello` and
adds a basic FT6X36-style touch probe using the manufacturer MicroPython
reference code.

## Hardware

- LCD SPI: SCLK GPIO18, MOSI GPIO23, CS GPIO14, D/C GPIO27, RST GPIO33
- Backlight: GPIO32
- Touch I2C: SDA GPIO21, SCL GPIO22
- Touch address: `0x38`

## Expected Behavior

- Full 320x240 landscape UI shell.
- Three menu tiles: `APPS`, `SET`, `WIFI`.
- Live touch coordinate area at the bottom.
- Pressing the screen updates the coordinate readout and draws a small marker.

## Build And Flash

```powershell
. E:\Espressif\v6.0.1\esp-idf\export.ps1
idf.py set-target esp32
idf.py build
idf.py -p COM3 flash monitor
```
