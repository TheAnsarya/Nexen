# Epic 16 Closure Matrix

Issue linkage: [#788](https://github.com/TheAnsarya/Nexen/issues/788), [#799](https://github.com/TheAnsarya/Nexen/issues/799)

Parent issue: [#776](https://github.com/TheAnsarya/Nexen/issues/776)

## Purpose

Use this matrix to track closure evidence for each Epic 16 sub-issue.

## Evidence Matrix Template

| Issue | Scope | Test Command | Benchmark Command | Artifact Paths | Commit | Status |
|---|---|---|---|---|---|---|
| #777 | Atari RIOT timer edge matrix | `Core.Tests.exe --gtest_filter="Atari2600RiotPhaseATests.*" --gtest_brief=1` | N/A | `artifacts/reference-validation/atari-harness.txt` | `<hash>` | open/in-progress/closed |
| #778 | Atari TIA wrap/carry stress | `Core.Tests.exe --gtest_filter="Atari2600TiaPhaseATests.*" --gtest_brief=1` | N/A | `artifacts/reference-validation/atari-harness.txt` | `<hash>` | open/in-progress/closed |
| #779 | Atari mapper invalid-order regressions | `Core.Tests.exe --gtest_filter="Atari2600MapperPhase*Tests.*" --gtest_brief=1` | `Core.Benchmarks.exe --benchmark_filter="BM_Atari2600_*" --benchmark_repetitions=3` | `artifacts/reference-validation/atari-genesis-benchmarks.json` | `<hash>` | open/in-progress/closed |
| #780 | Atari long-run determinism | `Core.Tests.exe --gtest_filter="Atari2600*Determinism*" --gtest_brief=1` | `Core.Benchmarks.exe --benchmark_filter="BM_Atari2600_*" --benchmark_repetitions=3` | `artifacts/reference-validation/atari-harness.txt` | `<hash>` | open/in-progress/closed |
| #781 | Genesis VDP ordering | `Core.Tests.exe --gtest_filter="GenesisVdp*Tests.*" --gtest_brief=1` | `Core.Benchmarks.exe --benchmark_filter="BM_Genesis_*" --benchmark_repetitions=3` | `artifacts/reference-validation/genesis-harness.txt` | `<hash>` | open/in-progress/closed |
| #783 | Genesis interrupt arbitration | `Core.Tests.exe --gtest_filter="GenesisInterruptSchedulingTests.*" --gtest_brief=1` | N/A | `artifacts/reference-validation/genesis-harness.txt` | `<hash>` | open/in-progress/closed |
| #784 | Genesis Z80 overlap | `Core.Tests.exe --gtest_filter="GenesisZ80HandoffTests.*" --gtest_brief=1` | N/A | `artifacts/reference-validation/genesis-harness.txt` | `<hash>` | open/in-progress/closed |
| #785 | Genesis audio cadence drift | `Core.Tests.exe --gtest_filter="GenesisPsgMixedAudioTests.*:GenesisYm2612TimingTests.*" --gtest_brief=1` | `Core.Benchmarks.exe --benchmark_filter="BM_Genesis_Audio*" --benchmark_repetitions=3` | `artifacts/reference-validation/atari-genesis-benchmarks.json` | `<hash>` | open/in-progress/closed |
| #786 | Genesis save-state boundary replay | `Core.Tests.exe --gtest_filter="GenesisSaveStateDeterminismTests.*" --gtest_brief=1` | N/A | `artifacts/reference-validation/genesis-harness.txt` | `<hash>` | open/in-progress/closed |
| #787/#798/#22 | Drift comparator tooling | `powershell -File scripts/compare-reference-validation.ps1` | N/A | `artifacts/reference-validation/*` | `<hash>` | open/in-progress/closed |
| #788/#799/#23 | Closure matrix maintenance | N/A | N/A | `~docs/plans/platform-parity-epic16-closure-matrix.md` | `<hash>` | open/in-progress/closed |
| #800/#24 | Weekly cadence and triage checklist | N/A | N/A | `~docs/plans/platform-parity-epic16-weekly-cadence.md` | `<hash>` | open/in-progress/closed |

## Epic Close Criteria

Epic 16 can be closed when all of these are true:

1. Every required issue row is marked closed.
2. Each closed row has at least one test command and one evidence artifact (or explicit N/A for benchmark-only docs issues).
3. Every implemented code issue has a commit hash and reproducible command evidence.
4. No unresolved deterministic digest drift remains in Atari/Genesis artifact comparison output.
