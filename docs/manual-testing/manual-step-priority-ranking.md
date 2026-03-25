# Manual Step Priority Ranking

This document ranks manual test steps for Atari 2600 release sign-off.

## Priority Levels

| Priority | Meaning | Release Impact |
|---|---|---|
| Critical | Core correctness, determinism, data integrity, and debugger truth | Release blocker |
| High | Mainline user workflows and TAS tooling behavior | Release blocker |
| Medium | Productivity and UX quality improvements | Fix before next milestone |
| Low | Nice-to-have polish | Track as follow-up issue |

## Ranked Step List

| ID | Area | Manual Step | Priority |
|---|---|---|---|
| A26-DBG-01 | Debugger UI | Disassembly updates correctly while stepping frame-by-frame | Critical |
| A26-DBG-02 | Debugger UI | Breakpoints hit exact expected address and pause reliably | Critical |
| A26-DBG-03 | Debugger UI | Memory viewer reflects TIA and RIOT register writes live | Critical |
| A26-DBG-04 | Debugger UI | Trace logger includes expected Atari 2600 tags and ordering | High |
| A26-DBG-05 | Debugger UI | Event viewer state remains stable when running and pausing repeatedly | High |
| A26-TAS-01 | TAS UI | Atari 2600 controller subtype button set matches selected port type | Critical |
| A26-TAS-02 | TAS UI | Paddle position editor appears only for paddle ports and writes values | Critical |
| A26-TAS-03 | TAS UI | Console SELECT and RESET toggles apply frame commands and undo correctly | Critical |
| A26-TAS-04 | TAS UI | Multi-port edit mode updates only the selected port input | High |
| A26-TAS-05 | TAS UI | Input preview matches frame data after edit, undo, and redo | High |
| A26-MOV-01 | Movie I/O | Nexen movie roundtrip preserves paddle position and console switches | Critical |
| A26-MOV-02 | Movie I/O | BK2 roundtrip preserves joystick, paddle position, and console switches | Critical |
| A26-MOV-03 | Movie I/O | Import and export mismatch paths produce clear validation errors | High |
| A26-RUN-01 | Runtime | Save state load resumes with identical controller and frame state | Critical |
| A26-RUN-02 | Runtime | Rewind and frame advance behavior stay deterministic over repeated cycles | High |

## Execution Order

1. Run all Critical steps first.
2. Run all High steps next.
3. Run Medium and Low only after blocker levels are clean.

## Failure Handling

1. Open or update a GitHub issue immediately.
2. Mark severity using this ranking table.
3. Attach reproduction steps and evidence reference.
