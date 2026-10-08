---
title: "Coffee Sentinel — Architecture"
subtitle: "An ESP32 watcher that tells the office when the coffee is ready"
author: "Coffee Sentinel contributors"
date: "2026-10-08 · firmware 0.1.0"
lang: en
---

# Overview

Coffee Sentinel is a small, fully external device that watches the green
READY LED of an office coffee machine and announces when the coffee is
done: first with a local buzzer, then (optionally) with a Wi-Fi webhook that
a home-automation hub turns into an Alexa announcement.

The machine is never opened, modified or wired. A phototransistor simply
looks at the LED.

```
Coffee machine
   | green LED
   v
Phototransistor (TEPT4400 + 10 kOhm)
   | analog voltage
   v
ESP32 DevKit V1
   +-- Buzzer      BEEP . pause . BEEP
   +-- Status LED
   +-- Wi-Fi -> HTTP webhook -> Home Assistant -> Alexa: "O cafe esta pronto."
```

## Goals

* **Reliable "once per coffee" detection.** No repeated alerts while the LED
  stays on, no false alerts from flicker, reflections or a blinking LED.
* **Testable without hardware.** Every decision the device takes is plain
  C++ that runs on a desktop under `pio test -e native`.
* **Grow without rewrites.** Wi-Fi, battery, MQTT and a display are planned;
  the layering keeps them as additions, not modifications.
* **Simple.** A handful of small classes, no heap allocations in the control
  loop, no blocking delays, no framework beyond Arduino-ESP32.

## Non-goals

* Controlling the machine (no relay, no mains).
* Running on anything but a single device per machine (for now).

# Architecture

## Layers

![Dependency direction between the layers](images/diagrams/layers.svg)

| Layer | Directory | Depends on | Contents |
|------------|------------------------|-----------------|------------------------------|
| Core | `src/core` | `<cstdint>` | `CoffeeDetector` (state machine), `Hysteresis`, `CoffeeConfig`, `CoffeeState`, `EventType`, `LightReading`, calibration math |
| Hardware ports | `src/hardware` | core | Interfaces `LightSensor`, `Buzzer`, `StatusLed`, `Notifier`, `Clock`, `Logger`, `NetworkLink`, `ConfigStore`; platform-free drivers `PatternPlayer`, `CompositeNotifier`, `InMemoryConfigStore` |
| Application | `src/application` | core, hardware | `CoffeeSentinelApp` (main loop coordination), `Calibrator`, `AppConfig`, `AlertPatterns` |
| Infrastructure | `src/infrastructure/esp32` | hardware, Arduino | `Esp32AnalogLightSensor`, `Esp32Buzzer`, `Esp32StatusLed`, `ArduinoClock`, `SerialLogger`, `SerialNotifier`, `Esp32WiFiLink`, `WebhookNotifier`, `NvsConfigStore`, `BoardPins` |
| Composition root | `src/main.cpp` | everything | Instantiates the adapters and the application; chooses normal or calibration mode |

Dependencies only point downwards. The `native` PlatformIO environment
compiles the first three layers and the tests; nothing in them includes an
Arduino header.

## Components

![System architecture](images/diagrams/system-architecture.svg)

**`CoffeeDetector`** receives one `LightReading` and the current time per
sample and returns an `EventType` (`None`, `CoffeeReady`, `CoffeeReset`,
`SensorFailure`, `SensorRecovered`). It owns a `Hysteresis` and the state
machine described below. It knows nothing about hardware.

**`CoffeeSentinelApp`** runs the loop: every `sampleIntervalMs` it reads the
sensor, feeds the detector, logs state transitions and routes events:

* `CoffeeReady` → start the buzzer pattern; when the pattern ends, call the
  `Notifier` once and acknowledge the detector (`markNotified`).
* `CoffeeReset` → optionally notify.
* `SensorFailure` → warning log and a distinctive chirp; `SensorRecovered` → log.
* Network link up/down → `WiFiConnected` / `WiFiDisconnected` events.

**`PatternPlayer`** plays an on/off `Pattern` on any `DigitalOutput` without
blocking. The buzzer and the status LED each have one; patterns live in
`AlertPatterns.h` (ready beep, sensor-failure chirp, blink styles per state).

**`Notifier`** implementations: `SerialNotifier` (always on),
`WebhookNotifier` (JSON POST, compiled in when `include/secrets.h` exists),
`CompositeNotifier` (fan-out to both).

**`Logger`** formats `[TAG] message` lines with DEBUG/INFO/WARN/ERROR levels;
`SerialLogger` writes them to the UART, `MockLogger` captures them in tests.

**`ConfigStore`** persists `CoffeeConfig`: `NvsConfigStore` on the ESP32
(Preferences/NVS), `InMemoryConfigStore` in tests. The firmware loads
stored settings at boot and falls back to the defaults if none are valid.

## One coffee, end to end

![Notification sequence](images/diagrams/notification-sequence.svg)

# Hardware flow

![Hardware flow](images/diagrams/hardware-flow.svg)

The TEPT4400 is wired as an emitter follower: collector to 3.3 V, emitter to
GPIO34 and to a 10 kΩ resistor to ground. More light means more collector
current and a higher voltage on the ADC (0–4095 at 12 bits). GPIO34 is on
ADC1, which stays usable while Wi-Fi is active.

The buzzer is an active type on GPIO25 (through an NPN transistor if it
draws more than ~12 mA). The status LED is the DevKit's on-board LED on
GPIO2. Details, pin rationale and alternatives are in `docs/hardware/wiring.md`.

# State machine

![State machine](images/diagrams/state-machine.svg)

| State | Meaning | Leaves when |
|---------|------------------|-----------------------------------------|
| `IDLE` | Light off, waiting | light on → `DETECTING` (or `NOTIFIED` if it is the first observation after boot, see below) |
| `DETECTING` | Light on, confirming | light off → `IDLE`; on for `readyConfirmationMs` → `READY` + **`CoffeeReady`** |
| `READY` | Coffee confirmed, local alert playing | application calls `markNotified()` → `NOTIFIED` |
| `NOTIFIED` | Alert delivered, waiting for the LED to go off | light off → `REARMING` |
| `REARMING` | Light off, confirming | light on → `NOTIFIED` (same coffee); off for `rearmConfirmationMs` → `IDLE` + **`CoffeeReset`** |

## Hysteresis

"Light on/off" is not `reading > threshold`. The `Hysteresis` class turns on
at `reading >= lightOnThreshold` (default 1800) and only turns off again at
`reading <= lightOffThreshold` (default 1200). Readings between the two keep
the previous state, so a value hovering around a threshold cannot flicker.

## Temporal confirmation

Both confirmation windows (`readyConfirmationMs`, `rearmConfirmationMs`,
3000 ms by default) use `millis()`-style `uint32_t` timestamps compared with
unsigned subtraction, which is correct across the 49.7-day roll-over. A LED
that blinks while brewing never stays on long enough to confirm.

## Sensor failure

An invalid reading (adapter reports failure, or the value exceeds
`sensorMaxValue`) raises `SensorFailure` once and freezes the state machine.
When valid readings return, `SensorRecovered` is raised and the current
confirmation window restarts: "continuously on" cannot be claimed across a
gap in observation.

## Boot with the light already on

A device that reboots while the coffee is already ready must not announce it
again. If the first valid observation shows the light on, the detector goes
straight to `NOTIFIED` and waits for the LED to turn off. The configuration
flag `alertIfLightOnAtBoot` turns the behaviour off for designs that reboot
on purpose (deep sleep).

# Bill of materials

| Qty | Part | Role |
|-----|--------------------------|----------------------------|
| 1 | ESP32 DevKit V1 | Controller, Wi-Fi |
| 1 | TEPT4400 phototransistor | Sees the machine's green LED |
| 1 | 10 kΩ resistor | Sensor load (current → voltage) |
| 1 | Active buzzer | Local alert |
| 1 | Breadboard, jumper wires | Prototyping |
| 1 | USB cable + 5 V supply | Power and flashing |
| opt. | NPN transistor + 1 kΩ | Buzzer driver above ~12 mA |
| opt. | LED + 220 Ω | External status LED |
| opt. | LiPo cell, charger/protection module, switch | Battery mode (roadmap) |
| opt. | 3D-printed cap / heat-shrink | Light shield and mounting |

The full table with notes and prices is in `docs/hardware/BOM.md`.

# Operation

## Boot

1. Serial at 115200 baud, ADC configured (12-bit, 11 dB attenuation), GPIOs set.
2. `CoffeeConfig` loaded from NVS; defaults if absent or invalid (logged).
3. If `include/secrets.h` exists: Wi-Fi station starts and the webhook
   notifier is registered next to the serial notifier.
4. `[INFO] Coffee Sentinel 0.1.0 booting`, `[CONFIG] …`, `[INFO] State: IDLE`.

## Main loop

Every iteration (no `delay()` anywhere):

1. `NetworkLink::update()` — non-blocking reconnection, connection events.
2. Every `sampleIntervalMs` (50 ms): read the sensor, update the detector,
   log a transition if the state changed, handle the event if any.
3. Update both pattern players (buzzer, LED).
4. If the detector is `READY` and the buzzer finished: notify, then
   `markNotified()`.

## Logging

```
[INFO] Coffee Sentinel 0.1.0 booting
[CONFIG] lightOn>=1800 lightOff<=1200 ready=3000ms rearm=3000ms sample=50ms
[INFO] State: IDLE
[STATE] IDLE -> DETECTING
[STATE] DETECTING -> READY
[EVENT] CoffeeReady
[BUZZER] Playing ready pattern
[NOTIFY] Coffee is ready
[STATE] READY -> NOTIFIED
[STATE] NOTIFIED -> REARMING
[STATE] REARMING -> IDLE
[EVENT] CoffeeReset
```

DEBUG adds `[SENSOR] value=342 light=off` every 5 s. WARN and ERROR lines
are prefixed with their level, e.g. `[WARN][SENSOR] Invalid readings; detection paused`.

## Calibration

The `esp32dev-calibration` environment boots into a three-phase interactive
routine (ambient, LED off, LED on) that prints the measured levels and
suggested thresholds:

```
[CALIBRATION] Ambient: 322
[CALIBRATION] LED OFF: 341
[CALIBRATION] LED ON: 2870
[CALIBRATION] Suggested thresholds: lightOnThreshold=1858 lightOffThreshold=1099
```

Thresholds are placed at 60 % and 30 % of the span between the LED-off and
LED-on levels. See `docs/calibration.md`.

## Notifications

`SerialNotifier` prints `[NOTIFY] Coffee is ready`. `WebhookNotifier` POSTs

```
{"event":"coffee_ready","message":"Coffee is ready","device":"coffee-sentinel","version":"0.1.0"}
```

to the configured URL with a 3 s timeout, after the local buzzer pattern has
finished so a slow network never stretches the first beep. Delivery failures
are logged as warnings; the state machine moves on regardless.

# Safety and security

## Hardware safety

* The coffee machine is **never opened or wired**; the sensor couples
  optically through the machine's own LED window.
* **No mains voltage** anywhere in the design or the roadmap.
* 3.3 V logic only on ADC pins; buzzers above ~12 mA go through a transistor.
* Battery operation requires a protected cell or a charger with protection.

## Software security

* Credentials and the webhook URL live in `include/secrets.h`, which is
  git-ignored; `include/secrets.example.h` is the template.
* The device only makes outbound HTTP requests; it exposes no server.
* HTTPS is supported without certificate validation in v0.2 (acceptable on a
  LAN; pinning is on the roadmap).
* Home Assistant webhooks should use a long random id and `local_only: true`.

## Testing as a safety net

71 native unit tests cover the detection lifecycle, hysteresis, pattern
timing, calibration, configuration validation and the application wiring,
including `millis()` overflow and reboots in every state. CI runs them on
every push together with the firmware build and a formatting check.

# Roadmap

| Version | Scope |
|---------|--------------------------------------------------------|
| v0.1 | Light detection, state machine, buzzer, tests, Wokwi, CI (this document) |
| v0.2 | Wi-Fi link and webhook notifier (already compiled when `secrets.h` exists), webhook retry, NVS save from the console, mDNS |
| v0.3 | Home Assistant automation for Alexa, MQTT discovery |
| v0.4 | Battery monitoring, lazy Wi-Fi, light/deep sleep, low-quiescent board profile |
| v0.5 | OTA updates, remote configuration, release workflow |
| later | OLED display, battery percentage, several machines, web dashboard, brewing statistics, 3D-printed case, automatic calibration |

Details: `docs/roadmap.md`.

# Appendix: repository layout

```
.github/workflows/      ci.yml (tests, firmware, format, Wokwi), docs.yml (PDF, GIF)
docs/                   this document, ADRs, hardware docs, diagrams (.mmd), images
include/                secrets.example.h
scripts/                build_docs.py, generate_demo_gif.py, check-format.sh
src/core                domain logic
src/hardware            ports and platform-free drivers
src/application         coordination and calibration
src/infrastructure/esp32  Arduino adapters
src/main.cpp            composition root
test/                   Unity suites, mocks and helpers (pio test -e native)
wokwi/                  diagram.json, wokwi.toml, scenario.yaml
platformio.ini          esp32dev, esp32dev-calibration, native
```
