# Architecture decision records

Short, dated notes on the decisions that shape the code base. Add a new file
when a decision is made or reversed; never rewrite history.

| ADR | Decision |
| --- | --- |
| [0001](0001-layered-structure.md) | Four source layers with one-way dependencies |
| [0002](0002-events-as-values.md) | Events are return values; the application is the only dispatcher |
| [0003](0003-hysteresis-and-confirmation.md) | Hysteresis plus temporal confirmation instead of a single threshold |
| [0004](0004-boot-with-light-on.md) | A light that is already on at boot does not trigger an alert |
| [0005](0005-native-tests-with-platformio.md) | Native unit tests through PlatformIO's `native` platform and Unity |
