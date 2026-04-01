# Epic 23 Menu, TAS, and Input Validation Matrix (2026-03-29)

## Scope

This matrix consolidates validation evidence for Epic 23:

- #1071 Menu resource-key and error-dialog failures
- #1072 TAS/movie crash-path stabilization
- #1073 Full controller button mapping coverage
- #1074 UI modernization and consistency pass
- #1075 Documentation, logs, benchmarks, and validation artifacts

## Validation Sources

- C++ tests: `Core.Tests.exe --gtest_brief=1`
- Managed tests: `dotnet test --no-build -c Release`
- Benchmark smoke subset:
	- `Core.Benchmarks.exe --benchmark_format=console --benchmark_min_time=0.01s --benchmark_filter="BM_(NesCpu|SnesCpu|GbCpu)_.*"`
- Manual workflows:
	- `~docs/testing/tas-movie-testing.md`
	- `docs/manual-testing/atari2600-debug-and-tas-manual-test.md`
	- `docs/manual-testing/lynx-debug-and-tas-manual-test.md`
	- `~docs/testing/ui-settings-coverage-matrix-2026-03-28.md`

## Menu Reliability Matrix

| Surface | Validation Focus | Current Status | Evidence |
|---|---|---|---|
| Main menu command bindings | No placeholder/error-dialog behavior for wired commands | Partial | Epic 23 issue set and UI settings coverage matrix |
| Resource-key resolution | Missing keys no longer break menu open/dispatch paths | Partial | #1071 tracking and session logs |
| Cross-platform menu parity | Shared menu actions remain consistent across system contexts | Partial | Manual workflows in TAS/manual-testing guides |

## TAS and Movie Stability Matrix

| Surface | Validation Focus | Current Status | Evidence |
|---|---|---|---|
| TAS window open/close | No crash while opening/closing editor from menus | Partial | `~docs/testing/tas-movie-testing.md` |
| Playback/seek/edit loops | No crash while seeking, editing, or replaying input ranges | Partial | TAS release hardening notes and managed tests |
| Console switching during TAS | Layout/command safety while switching system context | Partial | Existing TAS editor test suites |
| Save/load interactions | Movie/TAS data survives save-state and project workflows | Partial | Manual test docs and test suite coverage |

## Input Mapping Coverage Matrix

| Platform Group | Validation Focus | Current Status | Evidence |
|---|---|---|---|
| NES/SNES/GB/GBA/SMS/PCE | Controller mapping coverage and decode behavior | Complete | `InputApiDecoderTests` checkpoint (`input-mapping-coverage-checkpoint-2026-03-30.md`) |
| Atari 2600 | Port subtype mapping and decode correctness (joystick/driving/booster/keypad) | Complete | `InputApiDecoderTests` checkpoint (`input-mapping-coverage-checkpoint-2026-03-30.md`) |
| Lynx | Input mapping decode reliability for core button lanes | Complete | `InputApiDecoderTests` checkpoint (`input-mapping-coverage-checkpoint-2026-03-30.md`) |
| Channel F | Full button mapping and UI discoverability | Partial | #1073 follow-up for Channel F-specific mapping surface parity |

## Benchmark and Regression Signals

| Signal | Result |
|---|---|
| C++ test suite | Pass (2436/2436 in latest local validation) |
| .NET test suite | Pass (587/587 in latest local validation) |
| C++ benchmark smoke subset | Pass with stable filter |
| CI benchmark workflow | Hardened in #1099 and #1100 |

## Remaining Gaps

1. Promote partial menu/TAS/input rows to complete with explicit per-command QA checkmarks.
2. Add dedicated Channel F TAS/input manual checklist once full mapping rollout is finished.
3. Attach CI run links for benchmark smoke stability after merge-to-master run.

## Sign-Off Snapshot

Epic 23 documentation and validation artifacts are now consolidated and linkable from internal documentation indexes. This file is the matrix artifact for #1075 and can be expanded incrementally as additional fixes land.
