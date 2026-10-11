# Calibration

The firmware ships with thresholds that work for a bright LED and a 10 kΩ
load (`lightOnThreshold = 1800`, `lightOffThreshold = 1200` on the 0–4095
ADC scale). Every machine, sensor position and room is different, so run
the calibration once after mounting the sensor.

## What it measures

| Phase | What to do | Why |
| --- | --- | --- |
| 1 · AMBIENT | Keep the sensor away from the machine, pointed at the room | Baseline of the room light reaching the sensor |
| 2 · LED OFF | Mount the sensor on the machine while it is **not green**: switched off, or showing its red "brewing" light | Baseline at the final position. Using the red state here is what lets a single phototransistor tell "brewing" from "ready" |
| 3 · LED ON | Wait for a brew to finish so the LED is green | Signal level |

The suggested thresholds are placed at 60 % (on) and 30 % (off) of the
distance between *LED OFF* and *LED ON*. The wide gap is deliberate: it is
what makes the hysteresis immune to noise and to slow changes in room light.

## Running it

```bash
make calibrate                      # or:
pio run -e esp32dev-calibration -t upload
pio device monitor -b 115200
```

The calibration firmware prints the live sensor value twice a second and
waits for you to press **Enter** in the serial monitor at each phase:

```
[CALIBRATION] Starting calibration (3 phases). Values are shown live; press ENTER when ready.
[CALIBRATION] Phase 1/3 AMBIENT: keep the sensor away from the machine LED, then press ENTER
[CALIBRATION] live value=322
[CALIBRATION] live value=325
[CALIBRATION] Sampling AMBIENT for 2000 ms...
[CALIBRATION] AMBIENT: 322 (min=318 max=331 samples=100)
[CALIBRATION] Phase 2/3 LED OFF: mount the sensor on the machine with the LED off, then press ENTER
...
[CALIBRATION] Ambient: 322
[CALIBRATION] LED OFF: 341
[CALIBRATION] LED ON: 2870
[CALIBRATION] Suggested thresholds: lightOnThreshold=1858 lightOffThreshold=1099
```

In the PlatformIO monitor, make sure the input is sent (press Enter; the
default `monitor_eol` is fine). In Wokwi, type into the serial terminal.

## Applying the result

Until the serial `save` command lands (see the roadmap), edit the defaults in
[`src/core/CoffeeConfig.h`](../src/core/CoffeeConfig.h) or set them in
`loadConfig()` in `src/main.cpp`, rebuild and flash. The `NvsConfigStore`
adapter already persists a `CoffeeConfig` to flash; wiring a serial command
to it is a small change.

## Reading the numbers

* **LED ON far above LED OFF (span ≥ 1000):** ideal. Keep the suggestion.
* **Span between 100 and 1000:** usable, but add a light shield (heat-shrink
  around the phototransistor) and move it closer to the LED.
* **"LED ON is too close to LED OFF"** (span < 100): the sensor is not seeing
  the LED. Check the orientation (see [hardware/wiring.md](hardware/wiring.md)),
  the load resistor and whether the LED is actually green/visible (some
  machines use a very dim indicator).
* **LED OFF much higher than AMBIENT:** room light is reaching the sensor
  at the mounting position; shield it or the thresholds will drift between
  day and night.
* **Values stuck at 0 or 4095:** wiring problem (0) or the resistor is too
  large for a bright LED (4095, try 4.7 kΩ).

## Red versus green with one phototransistor

The TEPT4400 peaks at 570 nm, so a red LED of similar brightness reads
noticeably lower than a green one, but not zero. Measuring the *red* state
in phase 2 places both thresholds between the red and the green levels:
"ON" then means green only, and the brew session's "light OFF" requirement
is satisfied while the machine shows red. If your machine's red is brighter
than its green, the span check fails ("LED ON is too close to LED OFF");
that machine needs the two-sensor or RGB-sensor evolution listed in the
roadmap.

## Before trusting the timing

`readyConfirmationMs` (3000 ms by default) is only right if every green the
machine shows *before* the coffee is ready is shorter than that. Measure it
on the real machine first: [hardware/transient-greens.md](hardware/transient-greens.md).

## Why hysteresis and confirmation instead of a single threshold

With a single threshold a reading hovering around it would toggle the
"light" state on every sample. The detector uses two thresholds (on at ≥
`lightOnThreshold`, off at ≤ `lightOffThreshold`) **and** requires the light
to stay on for `readyConfirmationMs` before it believes the coffee is ready,
and only inside an armed brew session (see the README). A machine whose LED
*blinks* or alternates red/green during brewing is handled for free: a
green phase shorter than `readyConfirmationMs` never confirms. Raise that
value if your machine shows longer transient greens.
