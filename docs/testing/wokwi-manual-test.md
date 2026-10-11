# Manual Wokwi test of the brew session

Three scenarios exercised by hand in the simulator, with the serial output
to expect at each important point. `wokwi/scenario.yaml` automates the same
sequence; this page is for watching it happen.

## Setup

1. Build the firmware: `pio run -e esp32dev`.
2. VS Code: install the Wokwi extension, run **Wokwi: Select Config File**
   and pick `wokwi/wokwi.toml`, then **Wokwi: Start Simulator**. CLI
   alternative: `wokwi-cli wokwi` (interactive, needs `WOKWI_CLI_TOKEN`).
3. Open the serial monitor in the simulator. The boot lines are:

```
[INFO] Coffee Sentinel 0.1.0 booting
[CONFIG] lightOn>=1800 lightOff<=1200 cycleStart=3000ms ready=3000ms sample=50ms debounce=50ms
[INFO] State: IDLE
[INFO] Press START to begin a brew cycle
```

Parts: the **potentiometer** is the light seen by the phototransistor
(fully clockwise ≈ 4095 = green LED on; fully counter-clockwise ≈ 0 = dark),
the green **push button** is START COFFEE, the buzzer and the LED on D2
are the device's own. "ON" below means the potentiometer well above the
on-threshold (right half of its travel); "OFF" means near zero.

Timing defaults: `cycleStartConfirmationMs = 3000`, `readyConfirmationMs = 3000`.

## Scenario A — START while the machine is already green

Initial state: potentiometer ON (green), device IDLE.

| Step | Action | Expected |
| --- | --- | --- |
| 1 | Start the simulator | boot lines above, LED off, no sound |
| 2 | Leave the potentiometer ON | nothing: a green light outside a session is ignored |
| 3 | Press START COFFEE | one short blip; log below; LED blinks fast |
| 4 | Wait 10 s with the potentiometer still ON | **no** `CoffeeReady`, no second beep |
| 5 | Check the state | still `STARTING` |
| 6 | Turn the potentiometer to OFF | nothing yet |
| 7 | Keep it OFF for 3 s (`cycleStartConfirmationMs`) | `STARTING -> BREWING`, `BrewArmed`; LED slow heartbeat |
| 8 | Confirm the log shows `[EVENT] BrewArmed` | |
| 9 | Turn the potentiometer back to ON | nothing yet |
| 10 | Keep it ON for 3 s (`readyConfirmationMs`) | `BREWING -> READY`, `CoffeeReady` |
| 11 | Count the `[EVENT] CoffeeReady` lines | exactly **one** |
| 12 | Listen | BEEP · pause · BEEP (about 0.4 s) |
| 13 | Check the notifier line | `[NOTIFY] Coffee is ready` after the beeps |
| 14 | Check the state | `READY -> COMPLETED`, `BrewCompleted`; LED solid |

Expected log, step 3:

```
[BUTTON] START pressed
[STATE] IDLE -> STARTING
[EVENT] BrewStarted
[INFO] Waiting for the machine to leave its green state
```

Steps 7–8:

```
[STATE] STARTING -> BREWING
[EVENT] BrewArmed
[INFO] Armed: the next stable green light means coffee
```

Steps 10–14:

```
[STATE] BREWING -> READY
[EVENT] CoffeeReady
[BUZZER] Playing ready pattern
[NOTIFY] Coffee is ready
[STATE] READY -> COMPLETED
[EVENT] BrewCompleted
[INFO] Session closed; press START for the next brew
```

Pass criteria: no `CoffeeReady` before step 10; exactly one afterwards.

## Scenario B — transient green

Start from a fresh boot (or from COMPLETED: press START, then continue).

| Step | Action | Expected |
| --- | --- | --- |
| 1 | Potentiometer OFF, press START COFFEE | `IDLE -> STARTING` (or `COMPLETED -> STARTING`), blip |
| 2 | Keep OFF for 3 s | `STARTING -> BREWING`, `BrewArmed` |
| 3 | Turn ON for about 1.5 s | nothing |
| 4 | Turn back OFF | nothing: the ON window restarts; **no** `CoffeeReady` |
| 5 | Confirm the log has no `[EVENT] CoffeeReady` | state still `BREWING` |
| 6 | Turn ON again | |
| 7 | Keep ON for 3 s | `BREWING -> READY`, `CoffeeReady`, beeps, notify, `COMPLETED` |
| 8 | Count `[EVENT] CoffeeReady` | exactly one for this session |

Expected log for steps 3–5: nothing new (the `[SENSOR]` lines only appear at
DEBUG level). The heartbeat LED keeps blinking slowly.

## Scenario C — after completion

| Step | Action | Expected |
| --- | --- | --- |
| 1 | Finish a brew (scenario A or B) | `READY -> COMPLETED` |
| 2 | Leave the device in COMPLETED | LED solid |
| 3 | Move the potentiometer OFF/ON/OFF/ON… several times, pausing more than 3 s on each | **no** new `[EVENT] CoffeeReady`, no beeps, no state change |
| 4 | Confirm the log is silent | |
| 5 | Press START COFFEE | `COMPLETED -> STARTING`, `BrewStarted`, blip |
| 6 | Potentiometer OFF for 3 s | `STARTING -> BREWING` |
| 7 | Potentiometer ON for 3 s | `BREWING -> READY` |
| 8 | Count `[EVENT] CoffeeReady` since step 5 | exactly one: a new session may alert again |

Expected log, step 5:

```
[BUTTON] START pressed
[STATE] COMPLETED -> STARTING
[EVENT] BrewStarted
```

## Extra checks worth a minute

| Action | Expected |
| --- | --- |
| Press START twice during STARTING or BREWING | second press: `[EVENT] StartIgnored` and `[WARN][BUTTON] START ignored: a brew cycle is already active (BREWING)`; nothing restarts |
| Hold START for 5 s | one `[BUTTON] START pressed`, not several |
| Disconnect the potentiometer wire from D34 while BREWING | `[EVENT] SensorFailure`, three chirps, session frozen; reconnect → `[EVENT] SensorRecovered`, confirmation restarts |
| Reset the board mid-session | boots in `IDLE`; the potentiometer position is ignored until START |
