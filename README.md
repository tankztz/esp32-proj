# ESP32 Project

Notes and future code for the connected ESP32 board.

## Current Board

- USB serial: Silicon Labs CP210x USB to UART Bridge
- Working port: COM20
- Chip: ESP32-D0WDQ6, revision v1.0
- Flash: 16MB
- PSRAM: 64Mbit / 8MB
- Crystal: 40MHz
- MAC: a8:03:2a:14:72:f8
- Existing firmware: ESP-IDF v4.2
- Existing app project name: C001_Project

## Useful Commands

```powershell
uv run python -m esptool --port COM20 chip-id
uv run python -m esptool --port COM20 flash-id
```

## Notes

Use `COM20` for now. Device Manager may show a different stale COM name after changing ports, but the working Windows device link was confirmed as `COM20`.
