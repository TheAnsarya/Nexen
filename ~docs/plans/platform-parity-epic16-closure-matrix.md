# Epic 16 Closure Matrix

Issue linkage: [#788](https://github.com/TheAnsarya/Nexen/issues/788), [#799](https://github.com/TheAnsarya/Nexen/issues/799)

Parent issue: [#776](https://github.com/TheAnsarya/Nexen/issues/776)

## Purpose

Use this matrix to track closure evidence for each Epic 16 sub-issue.

## Evidence Matrix Template

| Issue | Scope | Test Command | Benchmark Command | Artifact Paths | Commit | Status |
|---|---|---|---|---|---|---|
| #777 | Atari RIOT timer edge matrix | `Core.Tests.exe --gtest_filter="Atari2600RiotPhaseATests.*" --gtest_brief=1` | N/A | `Core.Tests/Atari2600/Atari2600RiotPhaseATests.cpp` | Prior session | closed |
| #778 | Atari TIA wrap/carry stress | `Core.Tests.exe --gtest_filter="Atari2600TiaPhaseATests.*" --gtest_brief=1` | N/A | `Core.Tests/Atari2600/Atari2600TiaPhaseATests.cpp` | Prior session | closed |
| #779 | Atari mapper invalid-order regressions | `Core.Tests.exe --gtest_filter="Atari2600MapperPhase*Tests.*" --gtest_brief=1` | `Core.Benchmarks.exe --benchmark_filter="BM_Atari2600_*" --benchmark_repetitions=3` | `Core.Tests/Atari2600/Atari2600MapperPhaseATests.cpp` | Prior session | closed |
| #780 | Atari long-run determinism | `Core.Tests.exe --gtest_filter="Atari2600*Determinism*" --gtest_brief=1` | `Core.Benchmarks.exe --benchmark_filter="BM_Atari2600_*" --benchmark_repetitions=3` | `Core.Tests/Atari2600/Atari2600DeterminismTests.cpp` | Prior session | closed |
| #781 | Genesis VDP active-scanline registers | `Core.Tests.exe --gtest_filter="GenesisVdpActiveScanlineRegisterTests.*" --gtest_brief=1` | N/A | `Core.Tests/Genesis/GenesisVdpActiveScanlineRegisterTests.cpp` | `3e1b57f3` | closed |
| #783 | Genesis interrupt coincidence | `Core.Tests.exe --gtest_filter="GenesisInterruptCoincidenceTests.*" --gtest_brief=1` | N/A | `Core.Tests/Genesis/GenesisInterruptCoincidenceTests.cpp` | `27688321` | closed |
| #784 | Genesis Z80 bus overlap stress | `Core.Tests.exe --gtest_filter="GenesisZ80BusOverlapStressTests.*" --gtest_brief=1` | N/A | `Core.Tests/Genesis/GenesisZ80BusOverlapStressTests.cpp` | `15d3da68` | closed |
| #785 | Genesis audio cadence drift | `Core.Tests.exe --gtest_filter="GenesisAudioCadenceDriftTests.*" --gtest_brief=1` | N/A | `Core.Tests/Genesis/GenesisAudioCadenceDriftTests.cpp` | `17d228d0` | closed |
| #786 | Genesis save-state boundary replay | `Core.Tests.exe --gtest_filter="GenesisSaveStateBoundaryReplayTests.*" --gtest_brief=1` | N/A | `Core.Tests/Genesis/GenesisSaveStateBoundaryReplayTests.cpp` | `55d230c7` | closed |
| #794 | Genesis interrupt replay digest suite | `Core.Tests.exe --gtest_filter="GenesisInterruptReplayDigestTests.*" --gtest_brief=1` | N/A | `Core.Tests/Genesis/GenesisInterruptReplayDigestTests.cpp` | `86e3cc89` | closed |
| #796 | Genesis mixed-audio digest checkpoints | `Core.Tests.exe --gtest_filter="GenesisMixedAudioDigestCheckpointTests.*" --gtest_brief=1` | N/A | `Core.Tests/Genesis/GenesisMixedAudioDigestCheckpointTests.cpp` | `282a7131` | closed |
| #787 | Atari/Genesis harness documentation | N/A | N/A | `~docs/plans/` | `<pending>` | open |
| #788 | Epic 16 closure matrix documentation | N/A | N/A | `~docs/plans/platform-parity-epic16-closure-matrix.md` | `<pending>` | open |
| #799 | Parity closure matrix template | N/A | N/A | `~docs/plans/platform-parity-epic16-closure-matrix.md` | `<this commit>` | in-progress |
| #800 | Week-by-week triage cadence | N/A | N/A | `~docs/plans/platform-parity-epic16-weekly-cadence.md` | `<pending>` | open |

## Epic Close Criteria

Epic 16 can be closed when all of these are true:

1. Every required issue row is marked closed.
2. Each closed row has at least one test command and one evidence artifact (or explicit N/A for benchmark-only docs issues).
3. Every implemented code issue has a commit hash and reproducible command evidence.
4. No unresolved deterministic digest drift remains in Atari/Genesis artifact comparison output.
