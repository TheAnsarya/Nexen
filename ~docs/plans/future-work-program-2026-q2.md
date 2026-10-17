# Nexen Future Work Program - 2026 Q2

## Scope

This plan defines the execution framework for the platform-parity expansion program under Epic [#673](https://github.com/TheAnsarya/Nexen/issues/673).

Q2 focus:

- Publish a stable documentation and tutorial index for future-work tracks.
- Complete Atari 2600 feasibility analysis and create implementation-ready sub-issues.
- Complete Sega Genesis feasibility analysis and create implementation-ready sub-issues.

## Issue Map

| Issue | Role | Expected Deliverable |
|---|---|---|
| [#694](https://github.com/TheAnsarya/Nexen/issues/694) | Documentation and execution scaffolding | Future-work index, tutorials, plan linking, session-log coverage |
| [#695](https://github.com/TheAnsarya/Nexen/issues/695) | Atari 2600 feasibility spike | Architecture constraints, test harness plan, phased issue breakdown |
| [#696](https://github.com/TheAnsarya/Nexen/issues/696) | Sega Genesis feasibility spike | CPU/VDP risk analysis, validation strategy, phased issue breakdown |

Q3 extension issues:

| Issue | Role | Expected Deliverable |
|---|---|---|
| [#704](https://github.com/TheAnsarya/Nexen/issues/704) | Atari 2600 research corpus | Source-linked subsystem tree and implementation checklist |
| [#705](https://github.com/TheAnsarya/Nexen/issues/705) | Genesis comparative research | Emulator code comparison and subsystem integration guidance |
| [#706](https://github.com/TheAnsarya/Nexen/issues/706) | Compatibility execution planning | Sonic 1 and Jurassic Park milestone-gated path |
| [#707](https://github.com/TheAnsarya/Nexen/issues/707) | Multi-session research cadence | Repeatable research/tutorial workflow and reporting model |

## Milestones

### Milestone 1 - Documentation Foundation

Owner issue: [#694](https://github.com/TheAnsarya/Nexen/issues/694)

Deliverables:

- [docs/FUTURE-WORK.md](../../docs/FUTURE-WORK.md)
- [docs/TUTORIALS.md](../../docs/TUTORIALS.md)
- README link-tree updates in [docs/README.md](../../docs/README.md) and [~docs/README.md](../README.md)
- Session log entry in [~docs/session-logs/](../session-logs/)

Acceptance criteria:

- New docs are discoverable from root README and docs indexes.
- Markdown structural rules pass for all changed files.
- Issue [#694](https://github.com/TheAnsarya/Nexen/issues/694) has implementation summary and closure reference.

### Milestone 2 - Atari 2600 Feasibility and Validation Path

Owner issue: [#695](https://github.com/TheAnsarya/Nexen/issues/695)

Primary plan: [Atari 2600 Feasibility and Test Harness Plan](atari-2600-feasibility-and-harness-plan.md)

Deliverables:

- A detailed Atari 2600 feasibility report linked from [Long-Term Missing Platforms](long-term-missing-platforms.md).
- A targeted validation harness plan including CPU, TIA timing, RIOT behavior, and mapper coverage.
- Sub-issue decomposition for staged implementation.

Acceptance criteria:

- Feasibility assumptions are explicit and testable.
- Validation harness includes deterministic pass/fail signals.
- Sub-issues are sized for incremental delivery.

### Milestone 3 - Sega Genesis Feasibility and Validation Path

Owner issue: [#696](https://github.com/TheAnsarya/Nexen/issues/696)

Primary plan: [Sega Genesis Architecture and Incremental Plan](genesis-architecture-and-incremental-plan.md)

Deliverables:

- A detailed Genesis architecture report with 68000, Z80, VDP, and audio subsystem boundaries.
- Risk register for timing, bus arbitration, and FM synthesis accuracy.
- Sub-issue decomposition for staged implementation.

Acceptance criteria:

- Architecture boundaries identify minimum viable implementation scope.
- Risks have concrete mitigation and test plans.
- Sub-issues align with correctness-first emulator policy.

## Non-Goals (Q2)

- Production implementation of Atari 2600 or Genesis emulation cores.
- UI polish unrelated to platform-parity deliverables.
- Release packaging changes unrelated to roadmap execution.

## Quality Gates

1. All documentation changes must satisfy markdown structure requirements.
2. Every implementation step must be linked to an issue before coding begins.
3. Performance-sensitive proposals must include explicit correctness safeguards.
4. Session logs must record what was delivered and what remains.

## Reporting Cadence

- Update issue status on milestone completion.
- Add a session log for each meaningful roadmap progress session.
- Keep [docs/FUTURE-WORK.md](../../docs/FUTURE-WORK.md) synchronized with active issues.

## Q3 Follow-On Plan

Q2 feasibility outputs are extended by the Q3 research plan:

- [Platform Parity Research Program - 2026 Q3](platform-parity-research-program-2026-q3.md)
