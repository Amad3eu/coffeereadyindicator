# ADR 0006 — A green light only means coffee inside a brew session

**Status:** accepted · **Date:** 2026-10-09 · supersedes ADR 0004

## Context

The real machine does not keep its green LED for "ready" only: it alternates
red and green while processing, and it stays green after a brew until
someone starts the next one. Treating every stable green as coffee (the
v0.1 draft) therefore produced alerts nobody asked for. The device has one
phototransistor and no way to read the machine's internal state.

## Decision

Detection is split into two core components:

* `LightMonitor` turns readings into "on/off" (hysteresis) plus sensor
  health. It knows nothing about coffee.
* `BrewSession` is the state machine `IDLE → STARTING → BREWING → READY →
  COMPLETED`, opened only by the START COFFEE button:
  * `STARTING` ignores the current light and waits for it to be **off for
    `cycleStartConfirmationMs`**. A START pressed while the machine is still
    green therefore cannot alert; the session first needs evidence that the
    machine left that state. A green flash during the wait restarts it.
  * `BREWING` is armed: a light **on for `readyConfirmationMs`** fires
    `CoffeeReady` once.
  * `READY` lasts while the buzzer plays; after the notification the session
    is `COMPLETED` and the light is ignored until the next START.
  * START during `STARTING`/`BREWING`/`READY` → `StartIgnored` (logged, no
    silent restart). START in `IDLE`/`COMPLETED` → new session.
  * Invalid readings freeze the session; on recovery the window in progress
    restarts. A reboot returns to `IDLE` and needs a new START.

The button is a raw level port (`StartButton`); debouncing and press-edge
detection live in the platform-free `Debouncer`, driven by the application.

With one phototransistor, "red" versus "green" is a calibration matter: the
OFF reference is measured with the machine in its non-green state (off or
red), so the thresholds sit between the two colours. Machines whose red is
brighter than their green need a second sensor or an RGB sensor (roadmap).

## Consequences

* No alert without a deliberate START, and at most one per session.
* The first green after a brew is never mistaken for the next coffee.
* A START pressed by mistake leaves the session waiting; cancelling it
  needs a reboot until a long-press/timeout lands (roadmap).
* The old `CoffeeDetector`, its `DETECTING/NOTIFIED/REARMING` states,
  `CoffeeReset`, `rearmConfirmationMs` and `alertIfLightOnAtBoot` are gone.
