# ADR 0003 — Hysteresis plus temporal confirmation

**Status:** accepted · **Date:** 2026-10-08

## Context

A phototransistor in front of an LED gives a noisy analog value that also
drifts with room light. A single threshold would flicker and could trigger
on reflections. Some machines blink their LED while brewing.

## Decision

Two independent mechanisms, both in `core`:

1. **Hysteresis** (`Hysteresis`): the light is "on" once a reading reaches
   `lightOnThreshold` and "off" only once it drops to `lightOffThreshold`.
   Readings in between keep the previous state.
2. **Temporal confirmation** (`BrewSession`): the light must be
   continuously off for `cycleStartConfirmationMs` before a session is
   armed, and continuously on for `readyConfirmationMs` before
   `CoffeeReady` (updated by ADR 0006; originally the second window
   re-armed the detector).

Invalid sensor readings freeze the state machine; when valid readings
return, the current confirmation window restarts, because "continuously"
can no longer be claimed.

Timers compare `now - start >= duration` with unsigned arithmetic, which is
correct across the `uint32_t` `millis()` roll-over (tested).

## Consequences

* Flicker-free by construction; blinking LEDs never confirm.
* Calibration produces the two thresholds from the LED-off and LED-on levels
  (60 % / 30 % of the span); the gap between them is the noise margin.
* A detection is delayed by `readyConfirmationMs` (3 s by default), which
  is irrelevant for a 15-minute brew.
