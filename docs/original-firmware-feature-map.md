# Original Firmware Feature Map

This document summarizes what can be inferred from the backed-up firmware and
the MCube-1 MicroPython reference files.

## Sources

- `backups/C001_Project-factory-app.bin`
- `backups/esp32-full-flash-16mb.bin`
- `references/mcube-1-esp32-dev-board-mpy-code`

The original app image reports:

- Project name: `C001_Project`
- App version: `1`
- Compile time: `Feb 25 2021 15:55:36`
- ESP-IDF: `v4.2`
- Product string found in firmware: `Rubik Cube-1`

## Hardware Map

Confirmed or strongly supported by firmware strings and reference code:

- Display: ILI9341 SPI TFT
- Display resolution used by app: 320x240
- Display SPI:
  - SCLK: GPIO18
  - MOSI: GPIO23
  - MISO: GPIO19 in MicroPython reference, unused in current C display output
  - CS: GPIO14
  - D/C: GPIO27
  - Reset: GPIO33
  - Backlight: GPIO32
- Touch: FT6X36-style capacitive controller
  - I2C SDA: GPIO21
  - I2C SCL: GPIO22
  - Address in MicroPython reference: `0x38`
  - IRQ pin in MicroPython reference: GPIO37
- SD card:
  - SPI SCLK: GPIO18
  - SPI MOSI: GPIO23
  - SPI MISO: GPIO19
  - CS: GPIO4
- IMU:
  - MPU6886 over I2C on GPIO21/GPIO22
- RGB LED / buzzer:
  - Reference examples use GPIO5 for both `neopixel` and PWM speaker tests.
  - Treat GPIO5 as needing hardware probing before assuming both features exist.

## Original UI Structure

Strings in the original firmware show an LVGL interface with these screens:

- `main page`
- `software page`
- `main menu`
- `software menu`
- `setting`
- `theme`
- `SD file`
- `Time setting`
- `WIFI-scan`
- `WIFI-AP`
- `WIFI-SmartConfig`

Menu descriptions found in the image:

- Main menu: beginning of the whole interface; pressing the middle touch button
  from any interface returns there.
- Software menu: host-only functions, no external modules required.
- SD file: after a TF card is inserted, files on the card can be managed.
- Theme: press the button to switch light/dark style.
- Settings: configure time, Wi-Fi, volume, and other Rubik Cube-1 core features.
- Calendar: view calendar information after setting the time first.

## Functional Areas To Recreate

Recommended rebuild order:

1. Display and UI shell
   - Landscape 320x240 UI.
   - Home screen with tiles/cards for the main features.
   - Light/dark theme toggle.
2. Touch input
   - Read FT6X36 at address `0x38`.
   - Map raw touch coordinates to screen coordinates.
   - Support a middle/home touch action.
3. Settings
   - Backlight brightness.
   - Time setting.
   - Wi-Fi mode selection.
4. Wi-Fi
   - Off.
   - STA scan.
   - SoftAP.
   - ESPTouch/SmartConfig.
5. SD/TF file browser
   - Mount SPI SD at CS GPIO4.
   - List root directory.
   - Open simple text files.
6. IMU demo
   - Read MPU6886 acceleration, gyro, and temperature.
   - Show values on screen.
7. GPIO5 accessory demo
   - Probe whether GPIO5 is connected to a WS2812 RGB LED, buzzer, or both.

## Reference Repository Contents

Useful files from `references/mcube-1-esp32-dev-board-mpy-code`:

- `boot.py`: MicroPython ILI9341 display class and touch demo.
- `Touch.py`: FT6X36-like touch read helper.
- `ws2812.py`: NeoPixel example on GPIO5.
- `speaker/speaker.py`: PWM speaker sweep example on GPIO5.
- `sdcard/sdcard.py`: SPI SD card driver using GPIO18/23/19 and CS GPIO4.
- `sdcard/test.py`: FAT mount and read/write test.
- `micropython-mpu6886-master/.../sample.py`: MPU6886 sample over GPIO21/22.
- `cube1python-lvgl/*`: LVGL/MicroPython GUI examples.

## C / ESP-IDF Reimplementation Notes

- Use ESP-IDF v6.0.1 for new programs.
- Keep each recreated feature in its own `programs/NN-*` folder until the
  hardware behavior is proven.
- Current display output is proven with `MADCTL=0x08` and 320x240 drawing.
- Prefer ESP-IDF native components for Wi-Fi, SD/FAT, LEDC PWM, and I2C.
- Consider LVGL for the final combined app once display and touch are stable.
