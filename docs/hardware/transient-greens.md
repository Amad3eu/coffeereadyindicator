# Transient greens: the one assumption that must be measured

## The assumption

The brew session treats **a light that is ON continuously for
`readyConfirmationMs`** (while armed) as "coffee ready". It cannot tell a
"ready" green from any other green; it only knows how long the green has
been steady.

The real machine may show green **before** the coffee is ready: while it
processes, while it reprocesses, between phases. The design copes with that
only if every such intermediate green is **shorter** than
`readyConfirmationMs`. That has never been measured; 3000 ms is a sensible
starting value, not a final one.

## What to measure

During one complete brew on the real machine (see
[bring-up.md](bring-up.md), section 7), with the device mounted and the
DEBUG sensor log on, record:

| Event | Approximate duration | Light reading while it lasts |
| --- | --- | --- |
| Intermediate green 1 | | |
| Intermediate green 2 | | |
| Intermediate green 3 (if any) | | |
| Final green (ready) | | |
| Red | | |
| Off / dark | | |
| Blinking (on time / off time) | | |

Repeat for at least three brews; machines are not perfectly repeatable.

## Decision

- **Every intermediate green shorter than `readyConfirmationMs`, with
  margin** (say, at most half of it): the current filter is sufficient.
  Keep the value, note the measured maximum in this file.
- **An intermediate green close to or longer than `readyConfirmationMs`**:
  the current strategy needs review. Document the numbers first, then
  choose among these options, cheapest first. None of them should be
  implemented before the measurement proves it necessary.

| Option | Cost | When it fits |
| --- | --- | --- |
| Raise `readyConfirmationMs` (e.g. 8000 ms) | one number | the final green lasts much longer than any intermediate one |
| Require a specific sequence (e.g. red → green with no dark gap, or N intermediate greens before the final one) | small core change + tests | the machine's pattern is deterministic |
| Use the light intensity (intermediate green dimmer/brighter than final) | calibration + a third threshold | readings differ reliably |
| RGB colour sensor (e.g. TCS34725) | new hardware + adapter | colours carry the information and intensity does not |
| Second phototransistor on another indicator | new hardware + port | another LED tells "ready" unambiguously |

## `readyConfirmationMs`

| | |
| --- | --- |
| Default | `3000` ms |
| Defined in | `src/core/CoffeeConfig.h` (`CoffeeConfig::readyConfirmationMs`) |
| Overridden by | values stored in NVS (`NvsConfigStore`, key `readyMs`) through `loadConfig()` in `src/main.cpp`; the calibration mode does not change it |
| Unit | milliseconds (`uint32_t`, overflow-safe arithmetic) |
| Effect | how long the light must stay ON, while armed, before `CoffeeReady`; also the minimum delay between the ready light and the alert |
| Validation | must be greater than 0 (`CoffeeConfig::validate()`); the application falls back to the defaults and logs `[ERROR][CONFIG]` otherwise |
| Logged at boot | `[CONFIG] … ready=3000ms …` |

How to test after changing it:

1. `pio test -e native`: the suites use their own values, so they keep
   passing and prove the logic is unchanged.
2. Wokwi ([manual test](../testing/wokwi-manual-test.md), scenario B): a
   green shorter than the new value must not alert, one longer must.
3. On the machine: one full brew with the device mounted.

The same table applies to `cycleStartConfirmationMs` (key `cycleMs`), the
time the light must stay OFF after START before the session arms.
