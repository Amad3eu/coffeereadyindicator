# Changelog

All notable changes to this project are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses
[Semantic Versioning](https://semver.org/).

## [Unreleased]

## [0.1.0] - 2026-10-09

### Added

- Brew sessions: a START COFFEE push button (GPIO33, internal pull-up,
  non-blocking debounce) opens a session; a green light outside a session is
  ignored. State machine `IDLE → STARTING → BREWING → READY → COMPLETED` with a
  robust arming rule (the light must leave the green state after START before a
  green can count), one alert per session, duplicate START ignored and logged.
- Light detection core: `LightMonitor` (hysteresis comparator, sensor-failure
  handling), temporal confirmation windows, overflow-safe timers.
- Non-blocking `PatternPlayer` driving the buzzer (START blip, BEEP · pause · BEEP) and the status LED.
- Leveled, tagged `Logger` with a serial adapter.
- `Notifier` port with `SerialNotifier`, `WebhookNotifier` (JSON POST) and `CompositeNotifier`.
- `NetworkLink` port with a non-blocking ESP32 Wi-Fi adapter, compiled in only when `include/secrets.h` exists.
- `ConfigStore` port with in-memory and NVS (Preferences) implementations.
- Interactive three-phase calibration mode (`esp32dev-calibration` environment).
- 102 native unit tests (Unity) with mocks for sensor, button, clock, buzzer, LED, notifier, network and logger; sanitizer run, 100 % line-coverage gate and cppcheck in CI.
- Wokwi simulation (`wokwi/`, potentiometer + START button) with an automated scenario covering START-while-green, short green and post-completion behaviour.
- GitHub Actions: native tests, firmware builds, clang-format check, optional Wokwi run, docs/GIF artifacts.
- Documentation: README, architecture document and PDF pipeline, ADRs, hardware docs (BOM, wiring, safety, battery), calibration, testing, Alexa integration, roadmap, Mermaid diagrams, README GIF generator.

[Unreleased]: https://github.com/Amad3eu/coffeereadyindicator/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/Amad3eu/coffeereadyindicator/releases/tag/v0.1.0
