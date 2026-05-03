# ATtiny85 DRV8833 Motor Driver

Minimal potentiometer-controlled DC motor firmware for the Digispark (ATtiny85). Turn the knob, motor spins. Click the pot off, power cuts. No app, no cloud, no sleep state to manage.

[![PlatformIO](https://img.shields.io/badge/PlatformIO-arduino-orange.svg)](https://platformio.org/)
[![MCU](https://img.shields.io/badge/MCU-ATtiny85-blue.svg)](https://www.microchip.com/en-us/product/attiny85)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

---

## Hardware

| Component | Part |
|-----------|------|
| MCU | [Digispark](http://digistump.com/products/1) (ATtiny85 @ 8 MHz internal) |
| Driver | DRV8833 breakout, single channel |
| Motor | 3 V, 1 A DC with flyback diode |
| Control | 10 kΩ linear pot with integral SPST power switch |
| Power | Single Li-ion cell (3.0–4.2 V) |

### Pinout

| Digispark Pin | ATtiny85 | Connected To | Notes |
|---------------|----------|--------------|-------|
| P0 | PB0 / OC0A | DRV8833 AIN1 | PWM output |
| P1 | PB1 | DRV8833 AIN2 | Held LOW (forward / coast) |
| P2 | PB2 / ADC1 | Pot wiper | Add 100 nF cap wiper-to-GND |

### DRV8833 Truth Table (one channel)

| AIN1 | AIN2 | Output |
|------|------|--------|
| PWM | LOW | Forward PWM (fast decay) |
| LOW | LOW | Coast — used as the off state |
| HIGH | HIGH | Brake — not used |

---

## How It Works

The potentiometer drives the ADC. A small deadzone at the bottom (`POT_DEAD_LOW`) ensures the motor stops cleanly before the integral switch clicks off. A matching deadzone at the top (`POT_DEAD_HIGH`) ensures full speed is reachable even if the pot doesn't hit a perfect 1023. Between those bounds, ADC counts map linearly to PWM duty cycle 1–254.

PWM runs at **31.25 kHz** (Timer0, Fast PWM, prescaler = 1 on 8 MHz clock) — above audible range, no motor whine.

There is no sleep logic. The pot switch cuts the entire supply; firmware only runs when the user has turned the pot on.

---

## Wiring

```
Li-ion (+) ──┬──────────────────── DRV8833 VM
             │
         Digispark 5V pin   ← not VIN — bypasses the regulator
             │
         Digispark GND ───── DRV8833 GND ───── Li-ion (–)

Potentiometer:
  CCW end  → GND
  Wiper    → P2  (+ 100 nF cap to GND)
  CW end   → 5V
  Switch   → in series with Li-ion (+) line

DRV8833:
  AIN1 → P0
  AIN2 → P1
  AOUT1/AOUT2 → Motor (observe polarity for forward direction)
```

> **Important:** Connect Li-ion (+) to the Digispark **5V pin**, not VIN. The VIN pin goes through the onboard regulator, which wastes power and drops voltage at the motor.

> **Important:** Desolder or disable the Digispark power LED. It draws 3–5 mA continuously from the battery regardless of what the firmware does.

---

## Quick Start

### Prerequisites

- [VS Code](https://code.visualstudio.com/) + [PlatformIO IDE](https://platformio.org/install/ide?install=vscode)
- [Git](https://git-scm.com/)

### Clone & Build

```bash
git clone <this-repo>
cd <this-repo>

# Build and flash (plug in Digispark when PlatformIO prompts)
pio run -t upload
```

The Digispark bootloader requires you to plug in the USB **after** the upload process starts. PlatformIO will prompt you.

### Tuning

All tunables are `#define`s at the top of `src/main.cpp`:

| Define | Default | Description |
|--------|---------|-------------|
| `POT_DEAD_LOW` | `12` | ADC counts below this = motor off. Raise if motor creeps at minimum. |
| `POT_DEAD_HIGH` | `1011` | ADC counts above this = full speed. Lower if pot can't quite reach max. |
| `LOOP_MS` | `20` | Control loop period in ms (50 Hz). |

---

## Project Structure

```
├── src/
│   └── main.cpp          # All firmware — ~80 lines
├── platformio.ini
└── README.md
```

No library dependencies. Timer0 and ADC are configured directly via registers — smaller flash footprint and no framework overhead.

---

## Resources

- [ATtiny85 Datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-2586-AVR-8-bit-Microcontroller-ATtiny25-ATtiny45-ATtiny85_Datasheet.pdf)
- [DRV8833 Datasheet](https://www.ti.com/lit/ds/symlink/drv8833.pdf)
- [Digispark Wiki](http://digistump.com/wiki/digispark)
- [PlatformIO ATtiny85 docs](https://docs.platformio.org/en/latest/boards/atmelavr/digispark-tiny.html)

---

## License

MIT.
