# ESP32 Project

Notes and future code for the connected ESP32 board.

## Programs

Each program lives in its own folder under `programs/`.

- `programs/01-minimal-log` - smallest ESP-IDF app for checking build, flash, and UART logging.
- `programs/02-sd-card-test` - SPI SD/TF card mount test using the likely board pins.
- `programs/03-display-hello` - direct ILI9341 SPI display test with simple on-screen text.
- `programs/04-touch-ui-shell` - first rebuild UI shell with ILI9341 output and FT6X36 touch probing.

## Docs

- `docs/windows-serial-recovery.md` - Windows CP210x COM port recovery notes.
- `docs/original-firmware-feature-map.md` - extracted feature map for recreating the original Rubik Cube-1 firmware.
- `docs/hardware-reference.html` - visual hardware reference page for the board.

## ESP-IDF Setup

ESP-IDF v6.0.1 is installed at:

```text
E:\Espressif\v6.0.1\esp-idf
```

Activate it in each new PowerShell session before building:

```powershell
. E:\Espressif\v6.0.1\esp-idf\export.ps1
```

## Current Board

See `HARDWARE.md` for the detailed board notes.

- USB serial: Silicon Labs CP210x USB to UART Bridge
- Current recovered USB serial port: COM3
- Earlier working USB serial port: COM20
- Chip: ESP32-D0WDQ6, revision v1.0
- Flash: 16MB
- PSRAM: 64Mbit / 8MB
- Crystal: 40MHz
- MAC: a8:03:2a:14:72:f8
- Existing firmware: ESP-IDF v4.2
- Existing app project name: C001_Project

## Useful Commands

```powershell
[System.IO.Ports.SerialPort]::GetPortNames()
uv run python -m esptool --port COM3 chip-id
uv run python -m esptool --port COM3 flash-id
```

From inside an ESP-IDF program folder:

```powershell
. E:\Espressif\v6.0.1\esp-idf\export.ps1
idf.py set-target esp32
idf.py build
idf.py -p COM3 flash monitor
```

## Notes

Use the current port from `[System.IO.Ports.SerialPort]::GetPortNames()`. If Windows lists the CP210x device but Python or ESP-IDF cannot open the COM port, follow `docs/windows-serial-recovery.md`.
