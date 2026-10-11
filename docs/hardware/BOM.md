# Bill of materials

Everything is off-the-shelf and breadboard-friendly. Prices are rough 2026
numbers for single units.

## Required (v0.1, USB powered)

| Qty | Part | Role | Notes | ~Price |
| --- | --- | --- | --- | --- |
| 1 | **ESP32 DevKit V1** (ESP32-WROOM-32, 30 or 38 pins) | Brain: reads the sensor, runs the state machine, drives buzzer/LED, Wi-Fi later | Any ESP32 dev board with an ADC1 pin exposed works; adjust `BoardPins.h` | US$ 4–8 |
| 1 | **TEPT4400** phototransistor (Vishay, 3 mm, ambient-light type) | Sees the coffee machine's green LED | Peak sensitivity ≈ 570 nm, which matches green LEDs well. A TEPT5600, PT333-3C or any NPN phototransistor also works after calibration | US$ 0.5 |
| 1 | **10 kΩ resistor**, ¼ W | Load resistor of the phototransistor (converts current to voltage for the ADC) | Raise to 47–100 kΩ if the LED is dim and readings stay low; lower to 4.7 kΩ if they saturate at 4095 | US$ 0.05 |
| 1 | **Active buzzer**, 3.3–5 V | Local "coffee is ready" beep | *Active* = beeps when powered; a passive buzzer needs a tone driver (not in v0.1). Prefer a ≤ 12 mA model if driven straight from the GPIO | US$ 0.5 |
| 1 | **Momentary push button** (6×6 mm tactile or a panel button) | "START COFFEE": opens a brew session | Wired between GPIO33 and GND; the ESP32's internal pull-up is used, so **no resistor is needed** | US$ 0.2 |
| 1 | Breadboard (400 points) | Prototyping | | US$ 2 |
| — | Jumper wires (M-M, M-F) | Connections | A few female jumpers help reach the LED on the machine | US$ 2 |
| 1 | USB cable (micro-USB or USB-C, **data** capable) | Flashing + power + serial monitor | Charge-only cables will not flash | US$ 2 |
| 1 | USB 5 V power supply, ≥ 500 mA | Power at the final location | A phone charger is fine | US$ 3 |

## Optional

| Qty | Part | Role | Notes |
| --- | --- | --- | --- |
| 1 | NPN transistor (2N2222, BC547, S8050) + 1 kΩ base resistor | Buzzer driver | Required for buzzers above ~12 mA; keeps the GPIO within its safe current |
| 1 | 10 kΩ pull-up + 100 nF across the button | Button hardening | Only for long button leads in a noisy spot; the internal pull-up and the firmware debounce are enough on a breadboard |
| 1 | External LED + 220 Ω resistor | Visible status LED | The DevKit's on-board LED (GPIO2) is used by default |
| 1 | Heat-shrink tube or small 3D-printed cap | Light shield around the phototransistor | Blocks room light so only the machine's LED reaches the sensor. Makes calibration much easier |
| 1 | Double-sided tape / hook-and-loop | Mounting on the machine | Reversible; nothing is drilled or glued permanently |
| 1 | LiPo/Li-ion cell, 3.7 V, 1000–2500 mAh | Battery operation ([battery.md](battery.md)) | Roadmap, after the web platform |
| 1 | Charger/protection module (TP4056 + DW01) or a board with built-in charging (e.g. a FireBeetle, TinyPICO, Lolin32) | Charging and cell protection | Roadmap, after the web platform |
| 1 | Slide switch | Power on/off | With battery operation |
| 1 | 3D-printed enclosure | Tidy mount + light shield | Roadmap |

## Not needed

* No relay, no mains components, no connection to the coffee machine's wiring. See [safety.md](safety.md).
* No external ADC: the ESP32's 12-bit ADC is enough once the thresholds are calibrated.
