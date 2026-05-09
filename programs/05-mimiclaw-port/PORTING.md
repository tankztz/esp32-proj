# MimiClaw ESP32-D0WDQ6 Port

This folder is a local port attempt from:

```text
https://github.com/memovai/mimiclaw
```

The upstream project targets ESP32-S3. This port targets the current board
documented in `../../HARDWARE.md`:

- ESP32-D0WDQ6
- 16MB flash
- 8MB SPI PSRAM
- CP210x UART on COM3

## First Port Changes

- Added `sdkconfig.defaults.esp32`.
- Uses DIO 40MHz flash instead of ESP32-S3 QIO.
- Uses ESP32 SPI PSRAM at 40MHz instead of ESP32-S3 Octal PSRAM.
- Uses UART console instead of ESP32-S3 USB Serial/JTAG.
- Relaxes the local component manifest to allow ESP-IDF v6.0.1 for this
  workspace build probe.
- Adds the Component Manager `espressif/cjson` dependency because ESP-IDF v6
  does not provide the old built-in `json` component name used upstream.
- Guards `WIFI_REASON_ASSOC_EXPIRE`, which is not present in this ESP-IDF
  v6.0.1 target header set.

## Build Probe

```powershell
. E:\Espressif\v6.0.1\esp-idf\export.ps1
cd E:\GithubDesktop\esp32-proj\programs\05-mimiclaw-port
idf.py set-target esp32
idf.py build
```

Do not add secrets to git. If needed, create `main/mimi_secrets.h` locally from
`main/mimi_secrets.h.example`.

## Verified Status

Built and flashed on COM3 with ESP-IDF v6.0.1.

Boot log confirms:

- Project boots as `mimiclaw`.
- ESP32-D0WDQ6 PSRAM is detected; 4MB is mapped into the allocator.
- SPIFFS mounts with about 11MB total space.
- Serial CLI starts on the CP210x UART.
- With no stored Wi-Fi credentials, onboarding starts an open AP similar to:

```text
MimiClaw-72F9
http://192.168.4.1
```

The upstream banner still says `ESP32-S3 AI Agent`; that is cosmetic in this
port and should be renamed later.
