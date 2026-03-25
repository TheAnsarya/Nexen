# Atari 2600 + Genesis Parity Tracker

Parent issue: [#750](https://github.com/TheAnsarya/Nexen/issues/750)

Related epics:

- [#740](https://github.com/TheAnsarya/Nexen/issues/740) Atari 2600 + Genesis parity program
- [#847](https://github.com/TheAnsarya/Nexen/issues/847) Atari 2600 production-ready emulator

## Purpose

Track parity milestones, closure evidence, and final acceptance criteria in one place so issue and epic closure remains objective and testable.

## Phase Gates

| Gate | Requirement | Evidence |
|---|---|---|
| Correctness | Core Atari 2600 and Genesis behaviors are covered by targeted tests and pass in Release | Core.Tests targeted filters and total pass counts |
| Build Health | Release x64 solution build is clean (no errors) | MSBuild output artifacts and build logs |
| Debugger UX | Event/timing/memory workflows are functional and not placeholder paths | Issue-linked commits and debugger tests/manual checks |
| TAS/Movie | TAS editor/movie conversion paths preserve platform-specific metadata | MovieConverter tests and issue-linked evidence |
| Metadata Export | Pansy export paths produce stable symbols/comments/xrefs for both tracks | Export tests and sample validation |
| Performance | Benchmarks run and provide deterministic, comparable outputs | Core.Benchmarks + saved benchmark runs |

## Active Atari 2600 Issue Chain

| Issue | Scope | Status |
|---|---|---|
| [#882](https://github.com/TheAnsarya/Nexen/issues/882) | Video filter NTSC palette LUT and color options | Complete |
| [#883](https://github.com/TheAnsarya/Nexen/issues/883) | SaveRomToDisk CDL strip verification tests | Complete |
| [#884](https://github.com/TheAnsarya/Nexen/issues/884) | Event viewer frame background rendering | Complete |
| [#885](https://github.com/TheAnsarya/Nexen/issues/885) | ProcessMemoryAccess debugger wiring | Complete |
| [#890](https://github.com/TheAnsarya/Nexen/issues/890) | MovieConverter Atari 2600 metadata/registration parity | Open |
| [#891](https://github.com/TheAnsarya/Nexen/issues/891) | TAS editor controller layout parity | Open |
| [#892](https://github.com/TheAnsarya/Nexen/issues/892) | Pansy cross-reference parity improvements | Open |

## Active Genesis Cross-Track Anchors

| Issue | Scope | Status |
|---|---|---|
| [#741](https://github.com/TheAnsarya/Nexen/issues/741) | Core.Benchmarks Atari 2600 + Genesis suite | Open (implementation reported) |
| [#750](https://github.com/TheAnsarya/Nexen/issues/750) | Multi-phase parity tracker and closure checklist | In progress |

## Closure Criteria

Close [#750](https://github.com/TheAnsarya/Nexen/issues/750) when all conditions are true:

1. This tracker is present and linked from [~docs/README.md](../README.md).
2. Each referenced sub-issue contains closure evidence (build, tests, or benchmark output).
3. Epic closure criteria are explicit and measurable for both Atari 2600 and Genesis tracks.
4. Remaining open items are either scheduled in active epics or explicitly deferred with rationale.

## Notes

- Keep this file as the single source of parity gate status for cross-session continuity.
- Update rows when issue status changes instead of duplicating status tables in multiple docs.
