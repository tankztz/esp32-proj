# M5Stack Ink Programs

This folder is for M5Stack e-ink devices, starting with CoreInk-style hardware.

CoreInk is a better fit for slow, persistent information than for animated UI:
the display keeps the last frame without power, refreshes slowly, and is easy to
read for text.

## Best Project Ideas

1. Desk status card
   - Shows time, Wi-Fi, battery, current task, and one short message.
   - Refresh every few minutes or on button press.

2. AI notification pager
   - Pulls one summary from a local server or Telegram bridge.
   - Displays only high-signal text on the e-ink screen.

3. Daily dashboard
   - Weather, calendar, to-do count, commute note.
   - Deep sleep between updates.

4. QR identity card
   - Static QR codes for Wi-Fi, GitHub, contact, lab notes, or device setup.
   - Good first real app because e-ink holds the code even after power loss.

5. Sensor label
   - Pair with ENV/temperature units and show last reading.
   - Useful because values can update slowly.

6. Tiny field notebook
   - Button cycles through saved notes stored in flash.
   - No SD card required.

## First Target

Start with `01-coreink-hello`: a minimal CoreInk display test. The goal is not
feature depth; it is to prove the board, screen orientation, buttons, and build
toolchain.

## Hardware Notes

Expected CoreInk-class hardware:

- MCU: ESP32-PICO-D4
- Display: 1.54 inch 200x200 black/white e-ink
- E-ink SPI pins: BUSY G4, RST G0, D/C G15, CS G9, SCK G18, MOSI G23
- Buttons/dial: G37, G38, G39, user button G5
- RTC: BM8563 on I2C G21/G22
- Power hold: G12

If the connected board is M5Paper or another e-ink model, do not reuse these
pins blindly; create a separate folder under `programs-ink`.

