# ADR 0004 — A light already on at boot does not trigger an alert

**Status:** accepted · **Date:** 2026-10-08

## Context

The device has no persistent memory of the machine's state. After a power
cut, a firmware update or a reset, the LED may already be on because the
coffee was ready before the device started watching. Alerting then would be
a false "fresh coffee" announcement, possibly hours late.

## Decision

If the very first valid observation shows the light on, the detector enters
`NOTIFIED` directly, without an event, and waits for the light to go off.
The flag `CoffeeConfig::alertIfLightOnAtBoot` (default `false`) restores
the alternative behaviour for setups where a reboot at the exact moment the
coffee finishes is more likely than a reboot during a long "ready" period
(e.g. a future deep-sleep mode that reboots on every wake-up).

## Consequences

* Reboots are silent and safe by default; the next brew is detected
  normally (tested in `test_state_machine`).
* A reboot inside the 3-second confirmation window of a brew that has just
  finished misses that one alert. Accepted: rare, and the buzzer is not the
  only way people notice coffee.
* Deep-sleep designs must set `alertIfLightOnAtBoot = true` or keep state in
  RTC memory; documented in `docs/hardware/battery.md`.
