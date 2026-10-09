# Testing without hardware

The whole detection logic runs on your computer. You can develop, test and
even simulate the complete firmware before buying a single component.

## What runs where

| Layer | Native tests (`pio test -e native`) | Wokwi simulation | Real ESP32 |
| --- | :-: | :-: | :-: |
| `core/` (detector, hysteresis, events, config, calibration math) | ✅ | ✅ | ✅ |
| `hardware/` (ports, `PatternPlayer`, `Logger`, `CompositeNotifier`) | ✅ | ✅ | ✅ |
| `application/` (`CoffeeSentinelApp`, `Calibrator`) | ✅ with mocks | ✅ | ✅ |
| `infrastructure/esp32/` ADC sensor, GPIO buzzer/LED, serial, NVS | ✗ | ✅ | ✅ |
| `infrastructure/esp32/` Wi-Fi link, webhook | ✗ | partial (Wokwi has a virtual Wi-Fi gateway) | ✅ |
| Real optics: sensor placement, thresholds, room light | ✗ | ✗ | ✅ only |

## 1. Native unit tests

```bash
python3 -m pip install platformio   # once
pio test -e native                  # all suites
pio test -e native -f test_state_machine         # one suite
pio test -e native -v                            # verbose Unity output
COFFEE_TEST_VERBOSE=1 pio test -e native -f test_application   # echo captured log lines
```

The `native` environment compiles `src/core`, `src/hardware` and
`src/application` with the host compiler (GCC or Clang, C++17, `-Wall -Wextra
-Wpedantic -Werror`) together with each `test/test_*` folder. Nothing from
`src/infrastructure` or `src/main.cpp` is included, which is what keeps the
build free of Arduino headers.

| Suite | Covers |
| --- | --- |
| `test_hysteresis` | On/off thresholds, dead band, flicker immunity |
| `test_state_machine` | The 15 lifecycle scenarios from the project brief: no light, short flash, sustained light, exactly one `CoffeeReady`, 20 minutes on, light off, re-arm, second coffee, oscillation near thresholds, invalid readings, `millis()` overflow, reboot in each state |
| `test_application` | Buzzer beeps, notification after the pattern, LED behaviour, sensor failure chirps, config fallback, failed delivery, Wi-Fi events, sampling interval, log levels |
| `test_pattern_player` | Step timing, looping, stop, stalls, overflow |
| `test_calibration` | Threshold suggestion math and the three-phase calibrator |
| `test_config` | `CoffeeConfig` validation, `InMemoryConfigStore`, `CompositeNotifier` |
| `test_enum_strings` | Log names, payload slugs and messages of every enum, including the fallbacks |

### Sanitizers and coverage

Two more native environments reuse the same suites:

```bash
pio test -e native-sanitize   # AddressSanitizer + UndefinedBehaviorSanitizer (make test-sanitize)
scripts/coverage.sh --html    # gcov/gcovr line coverage, report in docs/generated/coverage/ (make coverage)
```

CI runs both. The coverage gate fails below **100 % line coverage** and
70 % branch coverage of `src/core`, `src/hardware` and `src/application`:
every line of the platform-independent layers has to be exercised by a
test. Branch coverage is informative rather than strict because gcov
counts compiler-generated branches (exception paths, short-circuits).

### Static analysis

```bash
pio check -e native --fail-on-defect medium --fail-on-defect high   # make check
```

cppcheck runs over the same three layers (configuration in
`platformio.ini`, `check_*` keys); CI fails on medium or high defects.
`.clang-tidy` configures the checks IDEs run on the fly.

### Mocks

`test/mocks/` has one mock per port: `MockClock` (advance time by hand),
`MockLightSensor`, `MockBuzzer`, `MockStatusLed`, `MockNotifier`,
`MockNetworkLink`, `MockLogger` (captures lines; `contains()`/`count()`).
`test/support/` has `DetectorDriver` (feeds a detector over time) and
`AppHarness` (a fully mocked application).

A typical scenario reads like this:

```cpp
DetectorDriver d = bootedInTheDark();
d.feedValue(kBright, 3500);          // light on for 3.5 s
assertState(CoffeeState::Ready, d.detector.state());
TEST_ASSERT_EQUAL_UINT(1, d.count(EventType::CoffeeReady));
```

## 2. Wokwi simulation

Wokwi runs the real ESP32 firmware binary in the browser or in VS Code. The
project ships `wokwi/diagram.json` (ESP32 DevKit V1 + potentiometer +
buzzer + LED) and `wokwi/wokwi.toml`.

The potentiometer stands in for the phototransistor: turn it up and the
ADC on GPIO34 rises exactly like it would with the LED on.

### VS Code

1. Install the [Wokwi for VS Code](https://marketplace.visualstudio.com/items?itemName=Wokwi.wokwi-vscode) extension and request a free license (`Wokwi: Request a new License`).
2. Build the firmware: `pio run -e esp32dev`.
3. Run `Wokwi: Select Config File` and pick `wokwi/wokwi.toml`, then `Wokwi: Start Simulator`.
4. Open the serial monitor inside the simulator window. Drag the potentiometer: the log shows `IDLE -> DETECTING -> READY -> NOTIFIED`, the buzzer sounds and the LED lights.
5. Turn it back down: `NOTIFIED -> REARMING -> IDLE`.

### CLI (headless, also used in CI)

```bash
curl -L https://wokwi.com/ci/install.sh | sh     # installs wokwi-cli
export WOKWI_CLI_TOKEN=...                        # https://wokwi.com/dashboard/ci
pio run -e esp32dev
wokwi-cli wokwi --scenario scenario.yaml --timeout 90000
```

`wokwi/scenario.yaml` turns the potentiometer up and down and asserts the
expected serial lines. The CI workflow runs it automatically when the
repository has a `WOKWI_CLI_TOKEN` secret.

### Simulating the sensor differently

Replace the potentiometer with `wokwi-photoresistor-sensor` (an LDR module
with an analog output) if you prefer sliding a lux value. Wire its `AO` to
`D34`, `VCC` to `3V3`, `GND` to `GND`.

## 3. Real hardware

```bash
pio run -e esp32dev -t upload       # flash
pio device monitor -b 115200        # watch the log
```

Set `kLogLevel` to `LogLevel::Debug` in `src/main.cpp` to also print the raw
sensor value every 5 s (`[SENSOR] value=342 light=off`). Run the calibration
mode once the sensor is mounted: see [calibration.md](calibration.md).

## 4. What only hardware can tell you

* Whether the chosen resistor gives a usable span between LED off and on.
* How much room light leaks into the sensor at the mounting spot.
* Whether your buzzer needs a transistor driver.
* Wi-Fi signal quality near the coffee machine.

Everything else (timing, hysteresis, state transitions, alert-once
semantics, reboot behaviour, overflow safety) is already proven by the
native tests.
