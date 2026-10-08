# ADR 0005 — Native unit tests through PlatformIO's `native` platform and Unity

**Status:** accepted · **Date:** 2026-10-08

## Context

Tests must run on a Linux machine with no ESP32 attached, from the same
tool that builds the firmware, and in CI.

## Decision

* `[env:native]` uses `platform = native`, `test_framework = unity`,
  `test_build_src = yes` and a `build_src_filter` that includes only
  `core/`, `hardware/` and `application/`.
* Mocks are header-only under `test/mocks/` and shared helpers under
  `test/support/`; `-Itest` puts them on the include path. PlatformIO only
  treats `test_*` folders as suites, so these folders are never run.
* Each suite is one `test_*.cpp` with a `main()` that lists its tests.
* The host build is strict (`-Wall -Wextra -Wpedantic -Wshadow -Werror`);
  the firmware build keeps `-Wall` because the Arduino core headers are not
  warning-free.

## Consequences

* `pio test -e native` is the only command developers and CI need.
* Unity is C and minimalist: no fixtures or matchers. Small helper
  functions (`assertState`, `DetectorDriver`, `AppHarness`) make the tests
  readable without pulling in GoogleTest or Catch2 (either can be added
  later with `test_framework` on a second environment).
* Everything the native build compiles must stay free of Arduino headers,
  which is a useful forcing function for the layering (ADR 0001).
