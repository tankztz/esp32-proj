# 03 Display Hello

Minimal ILI9341 SPI display test.

Assumed pins from the old firmware logs and common ESP32 TFT boards:

- SCLK: GPIO18
- MOSI: GPIO23
- MISO: unused
- CS: GPIO14
- DC: GPIO27
- RST: GPIO33
- Backlight: GPIO32

The app initializes the screen, fills the background, draws a few colored bands, and renders text using a tiny built-in bitmap font.

## Build And Flash

```powershell
. E:\Espressif\v6.0.1\esp-idf\export.ps1
cd E:\GithubDesktop\esp32-proj\programs\03-display-hello
idf.py set-target esp32
idf.py -p COM20 flash monitor
```
