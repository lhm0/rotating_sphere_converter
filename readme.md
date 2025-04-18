# 🛠️ RS64 Conversion Tool

This repository contains the **conversion utility** used to create `.rs64` files for the  
[Rotating LED Sphere project](https://github.com/lhm0/rotating_sphere).  
These files are required to display **videos, animations, or static images** on the spherical POV LED display built with an RP2040 microcontroller.

---

## 🔄 What Is `.rs64`?

The `.rs64` format is a custom binary format designed specifically for the rotating LED sphere.  
It stores **preprocessed image data** optimized for high-speed POV rendering with interlaced LEDs, reduced color depth, and sub-pixel timing.

Each `.rs64` file encodes:
- 128 × 256 pixels (interlaced from 64 LEDs)
- 8 brightness levels via sub-pixel PWM
- Sequential frames for video playback
- Optional playlist control via metadata

---

## 🧰 What This Tool Does

This tool converts:
- Standard video formats (`.mp4`, `.mov`, etc.)
- Animated GIFs
- PNG or JPG image sequences  
⬇️  
…into `.rs64` files compatible with the display firmware.

The output can be saved directly to the SD card used in the LED sphere.

---

