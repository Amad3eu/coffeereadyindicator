# Changelog

All notable changes to this project are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses
[Semantic Versioning](https://semver.org/).

## [Unreleased]

## [0.1.0] - 2026-10-08

### Added

- Coffee detection core: hysteresis comparator, temporal confirmation,
  `IDLE → DETECTING → READY → NOTIFIED → REARMING` state machine, domain events,
  sensor-failure handling, overflow-safe timers, boot-with-light-on policy.
- Non-blocking `PatternPlayer` driving the buzzer (BEEP · pause · BEEP) and the status LED.
- Leveled, tagged `Logger` with a serial adapter.
- `Notifier` port with `SerialNotifier`, `WebhookNotifier` (JSON POST) and `CompositeNotifier`.
- `NetworkLink` port with a non-blocking ESP32 Wi-Fi adapter, compiled in only when `include/secrets.h` exists.
- `ConfigStore` port with in-memory and NVS (Preferences) implementations.
- Interactive three-phase calibration mode (`esp32dev-calibration` environment).
- 71 native unit tests (Unity) with mocks for sensor, clock, buzzer, LED, notifier, network and logger.
- Wokwi simulation (`wokwi/`) with an automated scenario.
- GitHub Actions: native tests, firmware builds, clang-format check, optional Wokwi run, docs/GIF artifacts.
- Documentation: README, architecture document and PDF pipeline, ADRs, hardware docs (BOM, wiring, safety, battery), calibration, testing, Alexa integration, roadmap, Mermaid diagrams, README GIF generator.

[Unreleased]: https://github.com/Amad3eu/coffeindicator/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/Amad3eu/coffeindicator/releases/tag/v0.1.0
