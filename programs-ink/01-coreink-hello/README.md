# 01 CoreInk Hello

Minimal first program for M5Stack CoreInk.

## Goal

Bring up the e-ink screen and print a simple status page:

- project name
- boot count
- current mode
- button hint

This should be the first burn-in test before Wi-Fi, sleep, or API features.

## Recommended Stack

Use Arduino first for this device with M5Stack's current `M5Unified` and
`M5GFX` libraries. The older `M5Core-Ink` library exists, but M5Stack marks it
deprecated. After the hardware behavior is confirmed, a later project can move
to ESP-IDF if we want tighter control over sleep, Wi-Fi, or custom drivers.

## Folder Plan

- `arduino/CoreInkHello/CoreInkHello.ino` - minimal sketch
- later: `platformio.ini` if we standardize on PlatformIO
- later: `notes.md` for measured battery/sleep/current behavior

## Manual Build Path

Arduino IDE:

1. Install ESP32 board support.
2. Install `M5Unified` and `M5GFX`.
3. Open `arduino/CoreInkHello/CoreInkHello.ino`.
4. Select an ESP32 board profile compatible with M5Stack CoreInk.
5. Flash over USB.

## Next Checks

After first flash:

- Does the screen refresh?
- Is the text upright?
- Does the user button register?
- Does the board keep the last frame after power-off?
