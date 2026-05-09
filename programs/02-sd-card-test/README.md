# 02 SD Card Test

SPI SD/TF card probe for the board.

Known bus pins:

- SCLK: GPIO18
- MOSI: GPIO23
- MISO: GPIO19

The app probes likely SD card CS pins at low SPI speed:

```text
4, 13, 15, 2, 5, 14, 25, 26, 27
```

The app mounts the card read-only in practice: it does not format, create, delete, or rename files. It prints card info and lists the root directory.

## Build And Flash

```powershell
. E:\Espressif\v6.0.1\esp-idf\export.ps1
cd E:\GithubDesktop\esp32-proj\programs\02-sd-card-test
idf.py set-target esp32
idf.py -p COM20 flash monitor
```
