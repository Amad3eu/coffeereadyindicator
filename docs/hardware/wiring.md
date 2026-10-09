# Wiring

Everything runs at 3.3 V. Read [safety.md](safety.md) first.

![Hardware flow](../images/diagrams/hardware-flow.svg)

## Pin map (ESP32 DevKit V1)

| Signal | GPIO | Why this pin | Defined in |
| --- | --- | --- | --- |
| Light sensor (analog in) | **34** | ADC1_CH6. Input-only pin with no boot-strapping role, and on ADC1 so readings keep working while Wi-Fi is on (ADC2 is unusable with Wi-Fi active) | `src/infrastructure/esp32/BoardPins.h` |
| START COFFEE button (input) | **33** | Not a strapping pin, has an internal pull-up, and is an RTC GPIO so it can wake the chip from deep sleep later (battery roadmap). GPIO32 or 27 are equivalent alternatives | same |
| Buzzer | **25** | Plain output, no boot constraints, DAC-capable if a passive buzzer is used later | same |
| Status LED | **2** | The DevKit's on-board blue LED | same |

Avoid GPIO 0, 2 (as input), 12 and 15 for new signals: they are strapping
pins and can prevent the board from booting. GPIO 34–39 are input-only and
have no internal pull-ups, so they suit the sensor but not the button.

## Sensor: TEPT4400 + 10 kΩ (emitter follower)

```
3V3 ──────┬── C (collector)
          │     TEPT4400       more light → more current → higher voltage
          └── E (emitter) ──┬──────────── GPIO34 (ADC)
                            │
                           10 kΩ
                            │
                           GND
```

* Reading with the LED off: a few hundred (room light).
* Reading with the LED on: typically 1500–4000 depending on distance and
  brightness. The calibration mode finds the exact numbers.
* The TEPT4400 peaks at 570 nm (green) with a ±30° half-sensitivity angle,
  which suits a green status LED a few millimetres away.

### Lead identification

In the Vishay package drawing the **longer lead is the emitter (E)** and the
shorter one the collector (C). If a reading barely reacts to light, swap the
two leads: with the 10 kΩ resistor in series a brief reversal while testing
does no harm, but do not leave it reversed (the reverse E–C rating is 1.5 V).

### Tuning the resistor

| Symptom | Change |
| --- | --- |
| LED on reads below ~800 | 47 kΩ or 100 kΩ (more gain) |
| LED on saturates at 4095 and LED off is also high | 4.7 kΩ or 2.2 kΩ (less gain) |
| Big day/night drift in the LED-off value | Shield the sensor (heat-shrink tube), not the resistor |

### Mounting

Slide a 5 mm piece of black heat-shrink over the phototransistor so it
only sees straight ahead, place it over the LED and fix it with a strip of
tape. A small 3D-printed cap with a 3 mm hole does the same more neatly.

## START COFFEE button

```
GPIO33 ────┬──── push button ──── GND        INPUT_PULLUP: pressed = LOW
           │
   (internal ~45 kΩ pull-up)
```

A momentary button between GPIO33 and GND is all it takes: the firmware
enables the internal pull-up and debounces in software (50 ms by default,
`AppConfig::buttonDebounceMs`), reacting once per press on the falling
edge. Holding the button does not repeat. For a long cable to a panel
button, add a 10 kΩ external pull-up and a 100 nF capacitor across the
button.

## Buzzer

```
GPIO25 ──── (+) active buzzer (−) ──── GND          ≤ 12 mA buzzers only
```

For louder buzzers (most 5 V models draw 20–30 mA):

```
GPIO25 ──[1 kΩ]── B
                  NPN (2N2222 / BC547 / S8050)
VIN(5V) ── (+) buzzer (−) ── C      E ── GND
```

A passive buzzer needs a square wave; the `Buzzer` port stays the same, only
an adapter using `ledcWriteTone()` would be added.

## Status LED

The on-board LED on GPIO2 needs no wiring. For an external LED:

```
GPIO2 ──[220 Ω]──▶|── GND     (anode = longer lead towards the resistor)
```

## Power

* Development and v0.1: USB from a computer or a 5 V phone charger
  (≥ 500 mA; Wi-Fi peaks draw ~250 mA).
* Battery operation: [battery.md](battery.md).

## Breadboard layout (text)

```
   ESP32 DevKit V1                       breadboard
   ┌───────────────┐
   │ 3V3 ●─────────┼──── TEPT4400 C
   │ GND ●─────────┼──┬─ 10 kΩ ─┬── TEPT4400 E ── also to GPIO34
   │ D34 ●─────────┼──┼─────────┘
   │ D33 ●─────────┼──┼── START button ── GND
   │ D25 ●─────────┼──┼── buzzer (+)
   │               │  └── buzzer (−)
   │ D2  ● (on-board LED)
   │ USB ◄── 5 V
   └───────────────┘
```

## Wokwi equivalent

`wokwi/diagram.json` replaces the phototransistor with a potentiometer on
`D34` (its `SIG` pin), adds the START COFFEE push button between `D33` and
GND, a buzzer on `D25` and a green LED on `D2` through 220 Ω. See
[../testing.md](../testing.md#2-wokwi-simulation).
