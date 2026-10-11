# Changelog

All notable changes to this project are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses
[Semantic Versioning](https://semver.org/).

## [Unreleased]

Target: **v0.1.0** — the software is complete and tested; the physical
validation on the real coffee machine is still pending
([release checklist](docs/releases/v0.1-checklist.md)).

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
- False-ready protection: a START pressed while the machine is already green cannot alert; the session first waits for the light to be off for `cycleStartConfirmationMs`.
- Interactive three-phase calibration mode (`esp32dev-calibration` environment); the OFF reference is measured with the machine in its non-green state (off or red).
- 102 native unit tests (Unity) with mocks for sensor, button, clock, buzzer, LED, notifier, network and logger; sanitizer run, 100 % line-coverage gate and cppcheck in CI.
- Wokwi simulation (`wokwi/`, potentiometer + START button) with an automated scenario covering START-while-green, short green and post-completion behaviour.
- GitHub Actions: native tests, sanitizers, coverage gate, cppcheck, firmware builds, clang-format check, static Wokwi scenario check, real Wokwi run when `WOKWI_CLI_TOKEN` exists (skipped and reported otherwise), docs/GIF artifacts.

### Fixed

- Wokwi scenario: every expected serial line is now waited for right after its trigger (wokwi-cli only matches bytes emitted after the wait starts), the simulated-time timeout is 45 s and warnings fail the run, so a stuck simulation no longer burns the 5-minute Wokwi CI limit.
- Documentation: README, architecture document and PDF pipeline, ADRs, hardware docs (BOM, wiring, safety, battery, bring-up checklist, transient-green validation), calibration, testing (manual Wokwi scenarios, Wokwi CI), release checklist, Alexa integration, roadmap, Mermaid diagrams, README GIF generator.

[Unreleased]: https://github.com/Amad3eu/coffeereadyindicator/commits/main
