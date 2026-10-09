# ADR 0002 — Events are return values; the application is the only dispatcher

**Status:** accepted · **Date:** 2026-10-08

## Context

The core must announce `BrewStarted`, `BrewArmed`, `CoffeeReady`,
`BrewCompleted`, `StartIgnored`, `SensorFailure` and `SensorRecovered`
without knowing about buttons, buzzers, HTTP or Alexa.
Options were an observer/event-bus with listener registration, callbacks
(`std::function`), or plain return values.

## Decision

`BrewSession::update()` / `start()` / `complete()` and
`LightMonitor::update()` each return a single `EventType` (or `None`).
`CoffeeSentinelApp` consumes them and routes them to the buzzer pattern
player, the status LED, the `Notifier` and the `Logger`. Connectivity events
(`WiFiConnected`, `WiFiDisconnected`) are produced by the application itself
by polling the `NetworkLink` port, so every event goes through the same
`handleEvent()` switch.

One event per sample is sufficient because the only situation that could
produce two (sensor recovery coinciding with a confirmation expiry) is
defined away: recovery restarts the confirmation window (ADR 0003).

## Consequences

* No heap, no `std::function`, no registration order to reason about.
* Tests simply assert on the returned values.
* Adding a third consumer (MQTT, display) means one more case in
  `handleEvent()`. If consumers multiply, a fixed-capacity listener list can
  be introduced in `application` without touching `core`.
