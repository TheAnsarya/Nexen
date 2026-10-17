# Lynx Commercial Bank-Addressing Headless Boot Smoke (2026-03-30)

## Scope

Headless execution pass for #1105 using Nexen test-runner mode and the selected 10-title commercial corpus.

Command path:

- `Nexen.exe --testrunner --lynx.usebootrom=false --timeout=45 <boot-smoke.lua> <rom>`

Artifacts:

- Runner script: `~scripts/lynx/run-lynx-boot-smoke.ps1`
- Lua probe: `~scripts/lynx/boot-smoke.lua`
- Raw result JSON: `~docs/research/lynx-commercial-bank-addressing-boot-smoke-results-2026-03-30.json`

## Result Summary

| Title | Exit Code | Boot Result | Notes |
|---|---:|---|---|
| California Games | -1 | Fail | Test-runner returned `-1` before scripted milestone |
| Chip's Challenge | -1 | Fail | Test-runner returned `-1` before scripted milestone |
| Klax | -1 | Fail | Test-runner returned `-1` before scripted milestone |
| Awesome Golf | -1 | Fail | Test-runner returned `-1` before scripted milestone |
| Warbirds | -1 | Fail | Test-runner returned `-1` before scripted milestone |
| Electrocop | -1 | Fail | Test-runner returned `-1` before scripted milestone |
| Zarlor Mercenary | -1 | Fail | Test-runner returned `-1` before scripted milestone |
| Blue Lightning | -1 | Fail | Test-runner returned `-1` before scripted milestone |
| Ninja Gaiden | -1 | Fail | Test-runner returned `-1` before scripted milestone |
| Gauntlet: The Third Encounter | -1 | Fail | Test-runner returned `-1` before scripted milestone |

## Interpretation

- This pass does not yet prove per-title bank/page addressing regressions.
- It does establish that the current headless pathway exits with a uniform `-1` outcome for the selected commercial set under HLE boot mode.
- Manual runtime validation in normal emulator mode is still required to fill `In-Game Result` and isolate address-bit anomalies.

## Follow-up

1. Run each title in non-headless mode and capture first deterministic gameplay milestone.
2. Exercise one paging-relevant transition per title and mark `In-Game Result`.
3. For any mismatch, capture anomaly hypothesis and land deterministic `Core.Tests/Lynx` regression coverage.
