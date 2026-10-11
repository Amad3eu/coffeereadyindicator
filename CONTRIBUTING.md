# Contributing to Coffee Sentinel

Thanks for helping. This page explains how the project is organised and
what a good change looks like, so reviews are quick and boring.

## Ground rules

* **Hardware safety is not negotiable.** No change may require opening the
  coffee machine, touching its wiring or using mains voltage. See
  [docs/hardware/safety.md](docs/hardware/safety.md).
* **Logic stays testable on a desktop.** `src/core`, `src/hardware` and
  `src/application` must not include Arduino or ESP-IDF headers. If you
  need the hardware, add a port (interface) in `src/hardware` and an adapter
  in `src/infrastructure`, plus a mock in `test/mocks`.
* **No blocking in the control flow.** No `delay()` outside `setup()`-time
  initialisation; use `millis()`-style timers through the `Clock` port.
* **No magic numbers.** Tunables go into `CoffeeConfig` / `AppConfig`;
  pins into `BoardPins.h`; patterns into `AlertPatterns.h`.
* **Secrets never enter git.** `include/secrets.h` is ignored; keep it that way.

## Development setup

```bash
python3 -m pip install platformio "clang-format~=23.1" gcovr
pio test -e native            # unit tests (no hardware)
pio test -e native-sanitize   # same tests under ASan/UBSan
scripts/coverage.sh           # coverage gate (gcovr)
pio run -e esp32dev           # firmware
scripts/check-format.sh --fix # or: make format
```

Optional: the Wokwi VS Code extension for simulation, Docker (or pandoc +
Mermaid CLI) for `make docs`, Pillow for `make gif`.

## Making a change

1. Open an issue first for anything bigger than a bug fix, so the design can
   be discussed before the code.
2. Branch from `main`.
3. Add or update tests. CI enforces 100 % line coverage of `src/core`,
   `src/hardware` and `src/application`, so new logic needs a test that
   exercises every line (`make coverage` shows what is missing).
4. Run what CI runs: `make test`, `make test-sanitize`, `make coverage`,
   `make check`, `make check-wokwi`, `make build`, `make check-format`.
5. Update the docs that your change affects (README section, `docs/*.md`,
   diagrams in `docs/diagrams/*.mmd`). If a decision is worth remembering,
   add an ADR under `docs/architecture/adr/`.
6. Open the pull request using the template.

## Commit messages

[Conventional Commits](https://www.conventionalcommits.org/):

```
feat: add webhook retry while the light stays on
fix: restart confirmation window after sensor recovery
test: cover millis() overflow during re-arm
docs: describe the TP4056 battery option
ci: cache the ESP32 toolchain
chore: bump espressif32 platform
refactor: extract GpioOutput from buzzer and LED adapters
```

Small, focused commits are easier to review and to bisect.

## Code style

* `clang-format` 23 (config in `.clang-format`): Google base, 4 spaces, 110 columns.
  `pip install "clang-format~=23.1"` gives the same binary CI uses.
* Names: `PascalCase` types, `camelCase` functions and variables,
  `trailingUnderscore_` members, `kConstant` for constants, one class per file.
* Prefer `uint16_t`/`uint32_t` with explicit casts in `printf`-style logs.
* Keep interfaces tiny (one to three pure virtual methods).
* `const` wherever it is true; references over pointers; no `new` after boot.
* Comments say *why*, the code says *what*.

## Tests

* Unity, one suite per `test/test_*/` folder, each with its own `main()`.
* Use the helpers: `SessionDriver` for core scenarios, `AppHarness` for
  application scenarios, `assertState`/`assertEvent` for readable failures.
* Time is always driven through `MockClock`; never sleep in a test.
* Name tests after the behaviour: `test_light_returning_during_rearm_does_not_alert_again`.

## Adding hardware support

1. Define or reuse a port in `src/hardware` (e.g. `BatteryMonitor`).
2. Implement the ESP32 adapter in `src/infrastructure/esp32`.
3. Add a mock in `test/mocks` and use it in `test_application`.
4. Wire it in `src/main.cpp`.
5. Document pins in `BoardPins.h` and `docs/hardware/wiring.md`, parts in `BOM.md`.

## Releasing

1. Update `CHANGELOG.md` and the version in `platformio.ini` (`COFFEE_SENTINEL_VERSION`).
2. Tag `vX.Y.Z`; CI builds and uploads `firmware.bin` as an artifact.
3. Regenerate the PDF/GIF if the docs changed (`make docs`, `make gif`).
