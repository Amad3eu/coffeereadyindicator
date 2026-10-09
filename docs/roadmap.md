# Roadmap

Versions are milestones, not promises. Each one stays fully testable on a
desktop (`pio test -e native`) and compilable without hardware.

## v0.1 — Local detection ✅ (this repository)

- [x] Light detection with hysteresis and temporal confirmation
- [x] Brew sessions opened by the START COFFEE button: `IDLE → STARTING → BREWING → READY → COMPLETED`, one alert per session, a green light outside a session is ignored
- [x] Robust arming: after START the light must leave the green state before a green can count
- [x] Non-blocking buzzer and status LED patterns
- [x] Domain events (`BrewStarted`, `BrewArmed`, `CoffeeReady`, `BrewCompleted`, `StartIgnored`, `SensorFailure`, …)
- [x] Serial notifier and leveled logging
- [x] Interactive calibration mode
- [x] Native unit tests with mocks for sensor, clock, buzzer, notifier
- [x] Wokwi simulation and CI

## v0.2 — Connectivity

- [x] `Esp32WiFiLink` with non-blocking reconnection (compiled when `secrets.h` exists)
- [x] `WebhookNotifier` posting JSON to any HTTP(S) endpoint
- [ ] Retry a failed webhook while the session is still open
- [ ] Session timeout and cancel (long press) for a START pressed by mistake
- [ ] Save calibration results to NVS from the serial console (`NvsConfigStore` is already in place)
- [ ] mDNS name (`coffee-sentinel.local`) and a `/health` endpoint

## v0.3 — Alexa / Home Assistant integration

- [ ] Documented Home Assistant automation: webhook → Alexa Media Player TTS ([alexa-integration.md](alexa-integration.md))
- [ ] Home Assistant MQTT discovery (binary_sensor `coffee_ready`, sensor `light_level`)
- [ ] Optional direct integration through Virtual Smart Home / IFTTT for people without Home Assistant

## v0.4 — Battery and low power

- [ ] Battery voltage monitoring and low-battery event
- [ ] Lazy Wi-Fi (connect only to notify)
- [ ] Light sleep between samples
- [ ] Deep sleep with wake-on-light on an RTC pin
- [ ] Low-quiescent board profile (FireBeetle / TinyPICO) — see [hardware/battery.md](hardware/battery.md)

## v0.5 — Maintenance

- [ ] OTA updates (ArduinoOTA or HTTP update from a release asset)
- [ ] Remote log level and configuration over MQTT/HTTP
- [ ] Release workflow publishing `firmware.bin` on GitHub Releases

## Ideas beyond v0.5

- OLED display with state, light level and battery percentage
- Multiple coffee machines (one device each, or one device with several sensors)
- Web dashboard served by the ESP32
- Statistics: average brewing time, coffees per day, busiest hour
- 3D-printed case with an integrated light shield
- Automatic calibration (learn the LED-on level from the first brew)
- Two phototransistors with colour filters, or an RGB sensor (TCS34725), to tell red from green directly instead of relying on the OFF-then-ON arming rule
- Passive buzzer melodies (`Pattern` already supports arbitrary step sequences; a tone output adapter is all that is missing)
