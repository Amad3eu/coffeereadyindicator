# Battery operation (planned, v0.4)

v0.1 runs from USB. This page documents the planned evolution to a
rechargeable cell and the numbers behind the design decisions, so the
firmware can grow toward it without surprises.

## Target hardware

```
LiPo / Li-ion 3.7 V (1000–2500 mAh)
        │
charger + protection (TP4056 + DW01/FS8205)   ─ or ─   ESP32 board with built-in LiPo charging
        │                                               (DFRobot FireBeetle, TinyPICO, Lolin32 Lite,
3.3 V regulator (the DevKit's AMS1117 is wasteful;        Adafruit HUZZAH32, …)
a low-quiescent LDO such as the HT7333 or an MCP1700 helps)
        │
      ESP32
```

Two options:

1. **Keep the DevKit V1** and add a TP4056 module. Cheapest, but the on-board
   AMS1117 regulator and the USB-UART chip draw several mA even when the
   ESP32 sleeps. Good for a first experiment, poor autonomy.
2. **Switch to a board with native LiPo support** and a low-quiescent
   regulator. Same firmware (only `BoardPins.h` and the PlatformIO `board`
   change). This is the recommended path.

## Consumption (typical ESP32 DevKit V1 figures)

| Mode | Current | Notes |
| --- | --- | --- |
| Active, Wi-Fi transmitting | 160–260 mA peaks | Only during the webhook POST (well under 1 s per coffee) |
| Active, Wi-Fi connected, idle | 80–120 mA | Keeping the association alive |
| Active, Wi-Fi off (v0.1 offline mode) | 30–50 mA | CPU at 240 MHz polling the ADC every 50 ms |
| Modem sleep (CPU on, radio off) | 15–25 mA | Automatic once Wi-Fi is disconnected |
| Light sleep | ~0.8 mA | Wakes in ~1 ms; RAM retained |
| Deep sleep | ~10 µA (chip) / 1–5 mA (DevKit V1 board) | Board regulator + UART chip dominate on the DevKit |

With a 2000 mAh cell:

| Strategy | Average current | Autonomy |
| --- | --- | --- |
| v0.1 firmware as is, Wi-Fi on | ~100 mA | ~20 h |
| Wi-Fi only when notifying | ~40 mA | ~2 days |
| Light sleep between samples | ~3 mA | ~4 weeks |
| Deep sleep + wake on light (see below) on a low-quiescent board | < 0.5 mA | months |

## Strategies, from easy to hard

1. **Connect to Wi-Fi lazily.** Stay offline until `CoffeeReady`, connect
   (2–5 s), POST, disconnect. The `NetworkLink` port already isolates this
   policy from the application. Cost: the notification arrives a few seconds
   later.
2. **Slow down sampling.** The LED stays on for many minutes; sampling every
   250–500 ms instead of 50 ms loses nothing. `AppConfig::sampleIntervalMs`
   is already the single knob.
3. **Light sleep between samples.** `esp_light_sleep_start()` with a timer
   wake-up. The main loop structure (one `loop()` call per sample) is already
   compatible; only the `Clock` keeps counting (`millis()` is preserved
   across light sleep).
4. **Deep sleep with wake-up on light.** The ESP32 can wake from deep sleep
   when an RTC GPIO crosses a threshold (`ext0`/`ext1`, digital) or through
   the ULP coprocessor reading the ADC. Since GPIO34 is an RTC pin, the
   phototransistor can wake the chip directly, and GPIO33 (the START
   button, also an RTC GPIO) can wake it on a press. A brew session lives
   in RAM, so a deep-sleep design must keep the `BrewSession` state in RTC
   memory across wake-ups (or treat each wake-up as a new START); the core
   class is a plain value and can be copied there as is.
5. **Battery monitoring.** A 100 kΩ/100 kΩ divider from the cell to an ADC1
   pin (or the board's built-in divider) gives the voltage; the
   `LightSensor`-style port pattern applies (`BatteryMonitor` interface,
   ESP32 adapter, mock for tests). Report it in the webhook payload and warn
   below ~3.5 V.

## Why not in v0.1

* Deep sleep changes the boot behaviour, which interacts with the session
  logic. The logic must be solid and well tested first (it is: see
  `test/test_brew_session`).
* The sensor must be characterised on real hardware before choosing a
  wake-up threshold.
* USB power is perfectly fine next to a coffee machine that is already
  plugged into the wall.
