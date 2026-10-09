# Hardware bring-up checklist

The first test with a real ESP32, sensor, button and buzzer, before the
device goes anywhere near the coffee machine. Work top to bottom; tick the
boxes in a copy of this file (or in the pull request that records the
session). Record numbers in the tables: they are the input for the
calibration and for the [transient green](transient-greens.md) decision.

Everything here is low-voltage DC. The coffee machine is **never** opened or
wired; the sensor only looks at its LED from the outside
([safety.md](safety.md)).

## 1. Before applying power

- [ ] Board is an **ESP32 DevKit V1** (ESP32-WROOM-32) or the pin map in `src/infrastructure/esp32/BoardPins.h` was adjusted.
- [ ] Light sensor on **GPIO34** (ADC1_CH6, input-only).
- [ ] START COFFEE button on **GPIO33**, other leg to GND.
- [ ] Buzzer on **GPIO25** (through an NPN driver if it draws more than ~12 mA).
- [ ] Status LED: on-board LED (GPIO2), or external LED + 220 Ω on GPIO2.
- [ ] One **common GND** for sensor, button, buzzer and board.
- [ ] Sensor and button use **3.3 V** only; nothing on an ADC pin can exceed 3.3 V.
- [ ] Phototransistor polarity: collector to 3V3, emitter to GPIO34 and to the 10 kΩ resistor to GND (longer lead = emitter on the TEPT4400; see [wiring.md](wiring.md)).
- [ ] Buzzer is an **active** type (beeps on DC); polarity respected (+ to GPIO/driver).
- [ ] **No electrical connection of any kind to the coffee machine** or to mains.
- [ ] Sensor is purely optical and external (tape or cap; nothing drilled or glued permanently).
- [ ] USB cable is a data cable; power comes from USB only (battery is a later step).

## 2. First power-up

- [ ] Flash: `pio run -e esp32dev -t upload`; monitor: `pio device monitor -b 115200`.
- [ ] Boot lines appear: `[INFO] Coffee Sentinel 0.1.0 booting`, `[CONFIG] …`, `[INFO] State: IDLE`, `[INFO] Press START to begin a brew cycle`.
- [ ] No `[ERROR][CONFIG]` line (defaults or NVS settings valid).
- [ ] Nothing beeps or blinks by itself with the light off.

## 3. START button

- [ ] `INPUT_PULLUP` confirmed: GPIO33 reads HIGH with the button released, LOW while pressed (a multimeter on the pin, or `[BUTTON] START pressed` only when pressed).
- [ ] One press → exactly one `[BUTTON] START pressed` and one `[EVENT] BrewStarted`, plus a short blip.
- [ ] Debounce: ten quick presses with a pause between them → ten events, none doubled; a "sloppy" press never produces two.
- [ ] Holding the button for 5 s → a single event.
- [ ] Button held while resetting the board → boots to `IDLE`, **no** session opened.
- [ ] Second press during STARTING/BREWING → `[EVENT] StartIgnored` + `[WARN][BUTTON] START ignored …`, state unchanged.

## 4. Sensor readings

Set `kLogLevel = LogLevel::Debug` in `src/main.cpp` (or use the calibration
firmware, which prints live values) and fill in the table. Take each
reading for at least 10 s and note the spread.

| Condition | ADC min | ADC mean | ADC max | Notes |
| --- | --- | --- | --- | --- |
| Ambient, sensor away from the machine, normal office light | | | | |
| Ambient, office lights off | | | | |
| Ambient, direct sunlight / strong lamp | | | | |
| Sensor mounted, machine switched off | | | | |
| Sensor mounted, machine **red** (brewing) | | | | |
| Sensor mounted, machine **green** (ready) | | | | |
| Sensor mounted, machine blinking / other indicator | | | | |
| Sensor mounted, light shield (heat-shrink / cap) fitted | | | | |

Interpretation:

- [ ] Green is clearly above red and above every ambient condition (ideally by ≥ 1000 counts). If not, add the light shield and move the sensor closer before anything else.
- [ ] Red and "off" are both below the future `lightOffThreshold`.
- [ ] No condition saturates at 4095 except (acceptably) green itself; if red saturates too, lower the load resistor (4.7 kΩ).

## 5. Calibration

- [ ] Run `make calibrate` (`esp32dev-calibration` firmware), phase 1 ambient, phase 2 with the machine **not green** (off, or red while brewing), phase 3 green.
- [ ] Record the suggestion: `lightOnThreshold = ____`, `lightOffThreshold = ____`.
- [ ] Both thresholds sit between the red and the green readings from section 4, with margin on both sides.
- [ ] Hysteresis band (on − off) is wide compared with the noise in the table (several times the max−min spread).
- [ ] Apply the values (defaults in `src/core/CoffeeConfig.h`, or `loadConfig()` in `src/main.cpp`), rebuild, flash.
- [ ] With the machine red, the DEBUG line shows `light=off`; with it green, `light=on`. Red is never classified as green.

## 6. False positives

With a session armed (`BREWING`) and the machine not green, each of these
must leave the state at `BREWING` with no `CoffeeReady`:

| Disturbance | Result (no alert = pass) |
| --- | --- |
| Office lighting on/off | |
| Phone flash pointed at the sensor | |
| Flashlight sweep | |
| Person walking past / shadow | |
| Sensor moved a few millimetres off the LED | |
| Cover / case fitted and removed | |
| Machine's **red** indicator for the whole brew | |
| Reflections from the carafe or the counter | |

A disturbance that produces a steady reading above `lightOnThreshold` for
longer than `readyConfirmationMs` *will* alert: that is the threshold or
the shielding being wrong, not the logic. Fix it in section 4/5.

## 7. The real machine

Record what the machine's indicator does during one complete brew, from its
own start button to the carafe being removed. Use a stopwatch or the serial
timestamps (`pio device monitor` with the `time` filter).

| Event | Duration (s) | Notes |
| --- | --- | --- |
| Red after start | | |
| Off / dark periods | | |
| Intermediate green 1 | | |
| Intermediate green 2 | | |
| Blinking phases (on/off period) | | |
| Final green (ready) | | stays until? |
| Green after carafe removed | | |

Then read [transient-greens.md](transient-greens.md). If every intermediate
green is shorter than `readyConfirmationMs`, the current filter is enough.
If one is longer, **do not change the architecture on the spot**: write the
numbers down and choose among the documented options.

End-to-end runs with the device mounted:

- [ ] Brew 1: START pressed when the machine is started → `BrewArmed` once the machine is red/dark → exactly one `CoffeeReady` at the final green.
- [ ] Brew 2: START pressed while the machine is still green from brew 1 → no alert until the machine has been non-green for 3 s, then one alert at the end.
- [ ] After brew 2, leave the device in COMPLETED for an hour with the machine green/idle → no alert.
- [ ] A START pressed by mistake with the machine idle and green → stays STARTING, no alert (reset to cancel; cancel/timeout is on the roadmap).

## 8. Buzzer

- [ ] Audible across the room at the chosen mounting spot.
- [ ] Ready pattern is BEEP · pause · BEEP (about 0.4 s).
- [ ] The START blip (one short 80 ms beep) is clearly different from the ready pattern.
- [ ] The loop never blocks: the LED keeps blinking and the serial log keeps flowing while the buzzer plays.
- [ ] Exactly one ready pattern and one `[NOTIFY]` per session.

## 9. Status LED

| State | Expected | Seen |
| --- | --- | --- |
| IDLE | off | |
| STARTING | fast blink (~4 Hz) | |
| BREWING | slow heartbeat (1 short flash per second) | |
| READY | solid | |
| COMPLETED | solid until the next START | |

## 10. Wi-Fi and webhook (only if `include/secrets.h` exists)

- [ ] `[WIFI] Connecting to …` then `[WIFI] Connected to …, IP …`; `[EVENT] WiFiConnected`.
- [ ] Wrong password / router off: the device keeps detecting coffee; `[WIFI]` retries every 15 s without blocking.
- [ ] Router back on: `[EVENT] WiFiConnected` again without a reset.
- [ ] Webhook: the endpoint receives `{"event":"coffee_ready", …}` once per session; `[WEBHOOK] Delivered (HTTP 2xx)`.
- [ ] Endpoint down: `[WARN][WEBHOOK] …` and `[WARN][NOTIFY] Delivery failed …`, but the buzzer still played and the session still completes.
- [ ] Without `secrets.h`: `No include/secrets.h found: running offline`; everything else identical.

## 11. Power

- [ ] USB 5 V supply, ≥ 500 mA; no brown-out resets during Wi-Fi bursts.
- [ ] Battery operation is **not** part of this bring-up ([battery.md](battery.md), roadmap v0.4).

## 12. Sign-off

| Item | Result | Date | Notes |
| --- | --- | --- | --- |
| ESP32 real tested | | | |
| Sensor real tested | | | |
| Button real tested | | | |
| Buzzer real tested | | | |
| Red vs green calibrated | | | |
| Transient green durations measured | | | |
| False positives tested | | | |
| Machine-mounted test completed | | | |

Only when every row passes does the software of v0.1 become a *validated*
device. Until then it is "ready for physical validation".
