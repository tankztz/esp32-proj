# 01 Minimal Log

Smallest ESP-IDF app for this board.

It starts, prints a boot message, then logs a counter once per second over UART.

## Build And Flash

```powershell
cd programs\01-minimal-log
idf.py set-target esp32
idf.py build
idf.py -p COM20 flash monitor
```

Use `Ctrl+]` to leave `idf.py monitor`.
