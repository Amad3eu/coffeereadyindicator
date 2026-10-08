# ADR 0001 — Four source layers with one-way dependencies

**Status:** accepted · **Date:** 2026-10-08

## Context

The project is a small firmware, but it has to be developed and tested on a
desktop before any hardware exists, and it will grow (Wi-Fi, battery, MQTT,
display). A single `main.cpp` would make the detection logic depend on
`Arduino.h` and impossible to test natively.

## Decision

```
src/core            domain logic; depends on <cstdint> only
src/hardware        ports (abstract interfaces) + platform-free drivers that use them
src/application     coordination: samples, dispatches events, logs
src/infrastructure  ESP32 adapters implementing the ports
src/main.cpp        composition root
```

Dependencies point downwards only: `infrastructure → hardware → core`,
`application → hardware/core`, `main → everything`. `core` and `hardware`
never include Arduino headers. The `native` PlatformIO environment compiles
`core`, `hardware` and `application` and nothing else.

## Consequences

* Every behaviour that matters (timing, thresholds, alert-once, reboot) is
  testable with mocks on a desktop.
* A new board or sensor means a new adapter in `infrastructure`, nothing else.
* Slightly more files than a sketch. Each is small and named after one
  responsibility, which keeps the overhead in the "organised" rather than
  "enterprise" range.

## Rejected

* **Everything in `lib/` as PlatformIO libraries.** More idiomatic for
  reuse across projects, but adds `library.json` ceremony for a single
  firmware. Can be revisited if a second device shares the core.
* **Templates instead of virtual interfaces** (policy-based design). Saves
  a vtable lookup per sample, which is irrelevant at 20 samples/s, and
  makes mocks and error messages harder to read.
