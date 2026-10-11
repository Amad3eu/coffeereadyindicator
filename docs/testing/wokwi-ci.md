# Wokwi in CI: what runs, what is skipped, how to enable the real scenario

## Two jobs, one honest answer

| Job | Runs when | What it does |
| --- | --- | --- |
| **Wokwi scenario check** | always | Token-independent: `scripts/check_wokwi_scenario.py` validates `wokwi/diagram.json`, `wokwi/wokwi.toml` and `wokwi/scenario.yaml` against the firmware (every `set-control` targets an existing part with a valid control; every `wait-serial` line can really be printed by the firmware). It then reports whether the `WOKWI_CLI_TOKEN` secret exists. |
| **Wokwi simulation** | only when the secret exists | Downloads the `esp32dev` firmware built by the **ESP32 firmware** job and runs `wokwi/scenario.yaml` for real with `wokwi/wokwi-ci-action`. |

Without the secret the **Wokwi simulation** job is **skipped** (grey in the
PR checks, never green), the scenario-check job prints a
`::warning` annotation and a "real scenario SKIPPED" line in the run
summary. A green check mark on *Wokwi simulation* therefore always means the
scenario actually ran and passed.

How to read the result:

| Where | Executed and passed | Skipped (no token) | Executed and failed |
| --- | --- | --- | --- |
| PR checks | *Wokwi simulation* ✓ | *Wokwi simulation* "skipped" | *Wokwi simulation* ✗ |
| Run summary | "Wokwi: real scenario EXECUTED and passed" | "Wokwi: real scenario SKIPPED" | "Wokwi: real scenario EXECUTED and FAILED" |
| Annotations | notice | warning | error |

A skipped simulation is **not** a validation. Until the token is configured
and the job has run green at least once, treat the automated simulation as
*pending* in the release checklist.

## Configuring the token

1. Create a Wokwi CI token at <https://wokwi.com/dashboard/ci> (a Wokwi
   account is required; the CI tier has a free quota).
2. In the GitHub repository: **Settings → Secrets and variables → Actions →
   New repository secret**.
3. Name: `WOKWI_CLI_TOKEN` (exactly). Value: the token.
4. Re-run the CI workflow (or push a commit). The *Wokwi scenario check*
   summary changes to "token configured" and *Wokwi simulation* runs.

Pull requests from forks never receive secrets, so the simulation stays
skipped there by design.

## Time limits and how the scenario is written

* The free Wokwi CI plan stops a simulation after **5 minutes of wall-clock
  time** ("API Error: Connection timed out after 5 minutes"). A scenario
  that waits for a line that never comes burns the whole budget.
* `--timeout` (the action's `timeout` input) is in **simulated**
  milliseconds. The ESP32 simulates at roughly half real time with this
  firmware, so the CI value of `45000` is about 90 s of wall-clock time:
  a stuck scenario fails in well under two minutes instead of five.
* The scenario itself needs about 28 s of simulated time (the static check
  prints the explicit delays and keeps them under 20 s).
* `wait-serial` only matches bytes emitted **after the step starts**
  (wokwi-cli registers a pause point at that moment). Every expected line
  is therefore waited for immediately after the action that triggers it,
  and the START button is released only after its lines were matched.
  Putting a `delay` between the action and the wait can swallow the line
  and hang the scenario; `scripts/check_wokwi_scenario.py` rejects a
  `wait-serial` that directly follows a `delay`.
* `fail_text: "[WARN]"` makes any warning (duplicate START, sensor failure)
  fail the run immediately.

## Running it yourself

```bash
curl -L https://wokwi.com/ci/install.sh | sh        # installs wokwi-cli
export WOKWI_CLI_TOKEN=...                           # same token
pio run -e esp32dev
wokwi-cli wokwi --scenario scenario.yaml --timeout 45000 --fail-text "[WARN]"
python3 scripts/check_wokwi_scenario.py              # the static check CI runs
```

The manual walk-through of the same three scenarios is in
[wokwi-manual-test.md](wokwi-manual-test.md).
