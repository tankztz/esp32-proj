# References

This directory contains optional reference material for reverse engineering and
recreating the original board behavior.

## Submodules

- `mcube-1-esp32-dev-board-mpy-code`
  - Optional reference submodule.
  - Intended for AI-assisted learning, hardware mapping, and comparing against
    original manufacturer information.
  - It is not required to build the ESP-IDF programs in `programs/`.
  - Mirrored under GitHub so GitHub users do not need to depend on Gitee.
  - Current status: treat this as archived reference material. The original
    company/manufacturer support appears inactive, the Gitee source can be slow
    or difficult to access, and no newer official reference has been found.
- `M5-ProductExampleCodes`
  - Optional M5Stack product example submodule.
  - Source: `https://github.com/m5stack/M5-ProductExampleCodes`
  - License: MIT.
  - Useful path: `Core/M5Core2/Arduino/Core2_Factory_test`.
  - Used as a behavior reference for Core2-style factory test flow, app zones,
    Wi-Fi scan UI, startup checks, settings, timer, SD card, touch, and IMU demo
    organization. This is not original Rubik Cube-1 firmware and the hardware
    pins are not copied directly.

Clone with references:

```powershell
git clone --recurse-submodules <repo-url>
```

If the repository was already cloned:

```powershell
git submodule update --init --recursive
```

The original upstream source is:

```text
https://gitee.com/qq5541/mcube-1-esp32-dev-board-mpy-code
```

The M5Stack Core2 factory-test reference source is:

```text
https://github.com/m5stack/M5-ProductExampleCodes/tree/master/Core/M5Core2/Arduino/Core2_Factory_test
```
