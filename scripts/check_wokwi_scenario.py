#!/usr/bin/env python3
"""Static validation of the Wokwi simulation files (no token, no simulator).

Checks that:
  * wokwi/diagram.json parses and contains the parts the scenario drives;
  * wokwi/wokwi.toml points at the esp32dev firmware;
  * every ``set-control`` in wokwi/scenario.yaml targets an existing part with a
    known control;
  * every ``wait-serial`` string can actually be produced by the firmware:
    state transitions use real BrewState names, events use real EventType
    names, notifications use real messages, and any other text appears
    literally in src/.

Run it locally with ``python3 scripts/check_wokwi_scenario.py`` (needs PyYAML).
It is what the "Wokwi scenario check" CI job runs; the real simulation only
runs when the WOKWI_CLI_TOKEN secret exists (see docs/testing/wokwi-ci.md).
"""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path

try:
    import yaml
except ImportError:  # pragma: no cover - reported as a usage error
    print("error: PyYAML is required (pip install pyyaml)", file=sys.stderr)
    sys.exit(2)

ROOT = Path(__file__).resolve().parent.parent
DIAGRAM = ROOT / "wokwi" / "diagram.json"
SCENARIO = ROOT / "wokwi" / "scenario.yaml"
TOML = ROOT / "wokwi" / "wokwi.toml"
SRC = ROOT / "src"

KNOWN_CONTROLS = {
    "wokwi-potentiometer": {"position"},
    "wokwi-pushbutton": {"pressed"},
    "wokwi-slide-potentiometer": {"position"},
    "wokwi-photoresistor-sensor": {"lux"},
}

errors: list[str] = []


def fail(message: str) -> None:
    errors.append(message)


def enum_names(source: Path) -> set[str]:
    """Collects the string literals returned by a toString()-style switch."""
    text = source.read_text(encoding="utf-8")
    return set(re.findall(r'return "([^"]+)";', text))


def format_patterns(sources: str) -> list[re.Pattern[str]]:
    """Every C string literal in the sources, as a regex where printf
    conversions (%s, %u, %lu, %d, ...) match any text."""
    patterns = []
    for literal in set(re.findall(r'"((?:[^"\\]|\\.)*)"', sources)):
        if "%" not in literal:
            continue
        parts = re.split(r"%[-+ 0#]*\d*(?:\.\d+)?(?:hh|h|l|ll|z)?[diuxXoscfp%]", literal)
        patterns.append(re.compile(".+?".join(re.escape(part) for part in parts) + "$"))
    return patterns


def produced_by_firmware(message: str, sources: str, patterns: list[re.Pattern[str]]) -> bool:
    if message in sources:
        return True
    return any(pattern.match(message) for pattern in patterns)


def main() -> int:
    diagram = json.loads(DIAGRAM.read_text(encoding="utf-8"))
    parts = {p["id"]: p["type"] for p in diagram["parts"]}
    print(f"diagram.json: {len(parts)} parts, {len(diagram['connections'])} connections")

    toml = TOML.read_text(encoding="utf-8")
    if "esp32dev/firmware.bin" not in toml:
        fail("wokwi.toml does not point at .pio/build/esp32dev/firmware.bin")

    scenario = yaml.safe_load(SCENARIO.read_text(encoding="utf-8"))
    steps = scenario.get("steps", [])
    print(f"scenario.yaml: '{scenario.get('name')}', {len(steps)} steps")

    states = enum_names(SRC / "core" / "BrewState.cpp")
    events = enum_names(SRC / "core" / "Event.cpp")
    sources = "\n".join(p.read_text(encoding="utf-8") for p in sorted(SRC.rglob("*.cpp")))
    sources += "\n".join(p.read_text(encoding="utf-8") for p in sorted(SRC.rglob("*.h")))
    patterns = format_patterns(sources)

    waits = 0
    for index, step in enumerate(steps, start=1):
        if "set-control" in step:
            control = step["set-control"]
            part = control.get("part-id")
            name = control.get("control")
            if part not in parts:
                fail(f"step {index}: set-control targets unknown part '{part}'")
                continue
            allowed = KNOWN_CONTROLS.get(parts[part], set())
            if name not in allowed:
                fail(f"step {index}: control '{name}' is not valid for {parts[part]} ({part})")
        elif "wait-serial" in step:
            waits += 1
            text = str(step["wait-serial"])
            match = re.match(r"\[STATE\] (\w+) -> (\w+)$", text)
            if match:
                for state in match.groups():
                    if state not in states:
                        fail(f"step {index}: unknown state '{state}' in '{text}'")
                continue
            match = re.match(r"\[EVENT\] (\w+)$", text)
            if match:
                if match.group(1) not in events:
                    fail(f"step {index}: unknown event '{match.group(1)}'")
                continue
            match = re.match(r"\[NOTIFY\] (.+)$", text)
            if match:
                if f'"{match.group(1)}"' not in sources:
                    fail(f"step {index}: notification message '{match.group(1)}' is not produced by the firmware")
                continue
            # Other lines: "[TAG] message" where the message must come from a
            # literal or a printf format string in the firmware.
            message = re.sub(r"^\[\w+\] ", "", text)
            if not produced_by_firmware(message, sources, patterns):
                fail(f"step {index}: '{text}' cannot be produced by any firmware log line")
        elif "delay" in step:
            if not re.match(r"^\d+ms$", str(step["delay"])):
                fail(f"step {index}: delay must look like '1500ms', got '{step['delay']}'")
        else:
            fail(f"step {index}: unknown step type {list(step.keys())}")

    print(f"checked {waits} wait-serial expectations against {len(states)} states and {len(events)} events")
    if errors:
        for error in errors:
            print(f"error: {error}", file=sys.stderr)
        return 1
    print("wokwi files OK (static check only; the real scenario needs WOKWI_CLI_TOKEN)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
