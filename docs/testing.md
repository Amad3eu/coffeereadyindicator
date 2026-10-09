# Testing without hardware

The whole detection logic runs on your computer. You can develop, test and
even simulate the complete firmware before buying a single component.

## What runs where

| Layer | Native tests (`pio test -e native`) | Wokwi simulation | Real ESP32 |
| --- | :-: | :-: | :-: |
| `core/` (brew session, light monitor, hysteresis, events, config, calibration math) | ✅ | ✅ | ✅ |
| `hardware/` (ports, `Debouncer`, `PatternPlayer`, `Logger`, `CompositeNotifier`) | ✅ | ✅ | ✅ |
| `application/` (`CoffeeSentinelApp`, `Calibrator`) | ✅ with mocks | ✅ | ✅ |
| `infrastructure/esp32/` ADC sensor, GPIO button/buzzer/LED, serial, NVS | ✗ | ✅ | ✅ |
| `infrastructure/esp32/` Wi-Fi link, webhook | ✗ | partial (Wokwi has a virtual Wi-Fi gateway) | ✅ |
| Real optics: sensor placement, thresholds, room light | ✗ | ✗ | ✅ only |

## 1. Native unit tests

```bash
python3 -m pip install platformio   # once
pio test -e native                  # all suites
pio test -e native -f test_brew_session          # one suite
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
| `test_light_monitor` | Readings → light on/off, sensor failure and recovery reported once |
| `test_brew_session` | The session lifecycle: green before START ignored, START with green on never alerts, confirmed OFF period arms, short green ignored, stable green alerts once, oscillations after COMPLETED ignored, new START → new cycle, duplicate START ignored, sensor failure/recovery during a cycle, `millis()` overflow, reboot in every state |
| `test_debouncer` | Contact bounce, one press per hold, release/press separation, button held at boot, overflow |
| `test_application` | Button → session wiring, START blip, duplicate START warning, debounce through the app, buzzer/notifier once per session, LED per state, sensor failure chirps, config fallback, failed delivery, Wi-Fi events, sampling interval, log levels, reboot |
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
`MockLightSensor`, `MockStartButton` (raw level), `MockBuzzer`,
`MockStatusLed`, `MockNotifier`, `MockNetworkLink`, `MockLogger` (captures
lines; `contains()`/`count()`). `test/support/` has `SessionDriver` (feeds
a light monitor + brew session over time) and `AppHarness` (a fully mocked
application with a `pressStart()` helper).

A typical scenario reads like this:

```cpp
SessionDriver d(defaultConfig());
d.feedValue(kGreen, 1000);           // the machine is idle and green
d.pressStart();                      // START: session opens, green ignored
d.feedValue(kDark, 3500);            // machine brewing: armed
d.feedValue(kGreen, 3500);           // steady green: coffee
assertState(BrewState::Ready, d.state());
TEST_ASSERT_EQUAL_UINT(1, d.count(EventType::CoffeeReady));
```

## 2. Wokwi simulation

Wokwi runs the real ESP32 firmware binary in the browser or in VS Code. The
project ships `wokwi/diagram.json` (ESP32 DevKit V1 + potentiometer + START
COFFEE push button + buzzer + LED) and `wokwi/wokwi.toml`.

The potentiometer stands in for the phototransistor: turn it up and the
ADC on GPIO34 rises exactly like it would with the green LED on. The push
button on GPIO33 is the device's START COFFEE button.

Three things to try by hand (the automated `scenario.yaml` chains them):

| Scenario | Steps | Expected |
| --- | --- | --- |
| A · START while already green | pot high → press START → wait → pot low → wait 3 s → pot high → wait 3 s | nothing on START; `STARTING -> BREWING` after the low period; exactly one `CoffeeReady` after the high period |
| B · short green | press START → pot low 3 s → pot high 1.5 s → pot low → pot high 3 s | no alert for the short green; one alert for the long one |
| C · after COMPLETED | turn the pot up and down as often as you like → press START → pot low 3 s → pot high 3 s | no new `CoffeeReady` until START; the new session alerts once |

### VS Code

1. Install the [Wokwi for VS Code](https://marketplace.visualstudio.com/items?itemName=Wokwi.wokwi-vscode) extension and request a free license (`Wokwi: Request a new License`).
2. Build the firmware: `pio run -e esp32dev`.
3. Run `Wokwi: Select Config File` and pick `wokwi/wokwi.toml`, then `Wokwi: Start Simulator`.
4. Open the serial monitor inside the simulator window. Press START COFFEE: `IDLE -> STARTING` and a short blip. Keep the potentiometer low for 3 s: `STARTING -> BREWING`. Turn it up and hold for 3 s: `BREWING -> READY`, the buzzer plays BEEP · BEEP, `[NOTIFY] Coffee is ready`, `READY -> COMPLETED`.
5. Move the potentiometer as much as you like: nothing happens until the next START.

### CLI (headless, also used in CI)

```bash
curl -L https://wokwi.com/ci/install.sh | sh     # installs wokwi-cli
export WOKWI_CLI_TOKEN=...                        # https://wokwi.com/dashboard/ci
pio run -e esp32dev
wokwi-cli wokwi --scenario scenario.yaml --timeout 90000
```

`wokwi/scenario.yaml` presses the button, moves the potentiometer through
scenarios A, B and C and asserts the expected serial lines. The CI workflow runs it automatically when the
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
* Whether your machine's red "brewing" light reads below the thresholds (phase 2 of the calibration).
* Wi-Fi signal quality near the coffee machine.

Everything else (timing, hysteresis, session transitions, the arming rule,
alert-once semantics, debouncing, reboot behaviour, overflow safety) is
already proven by the native tests.
