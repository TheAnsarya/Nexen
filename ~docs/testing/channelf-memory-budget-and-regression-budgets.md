# Channel F Memory Budget and Regression Budgets

## Goal

Define deterministic memory baselines and regression thresholds for Channel F runtime and tooling workflows.

## Budget Baselines

| Area | Baseline Policy | Regression Threshold | Severity |
|---|---|---|---|
| Runtime core steady-state | Establish from release build with no debugger windows | > 5% sustained increase | High |
| Runtime with debugger open | Establish from release build with CPU + memory views | > 7% sustained increase | High |
| TAS/movie active capture | Establish from runtime recording workload | > 8% sustained increase | Medium |
| Save-state serialize/deserialize | Establish from repeated roundtrips across golden corpus | > 10% sustained increase | Medium |
| Channel F benchmark process footprint | Establish from benchmark-only harness run | > 6% sustained increase | Medium |

## Measurement Rules

1. Use release x64 builds only.
2. Run each scenario at least three times and compare median values.
3. Keep capture conditions stable: same ROM, same frame count, same debugger layout.
4. Record raw measurements and computed deltas in session logs.

## Regression Response Policy

- High severity threshold breach: block issue closure until explained and mitigated.
- Medium severity threshold breach: require mitigation plan or approved exception.
- Any unexplained memory growth across two consecutive runs is treated as regression.

## Evidence Checklist

- Baseline value and date.
- Current value and date.
- Percent delta.
- Workload description.
- Link to related issue and remediation notes.
