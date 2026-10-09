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

## Running it yourself

```bash
curl -L https://wokwi.com/ci/install.sh | sh        # installs wokwi-cli
export WOKWI_CLI_TOKEN=...                           # same token
pio run -e esp32dev
wokwi-cli wokwi --scenario scenario.yaml --timeout 180000
python3 scripts/check_wokwi_scenario.py              # the static check CI runs
```

The manual walk-through of the same three scenarios is in
[wokwi-manual-test.md](wokwi-manual-test.md).
