# Atari 2600 Mapper Order and Regression Matrix

Issue linkage: [#699](https://github.com/TheAnsarya/Nexen/issues/699)

Parent issue: [#695](https://github.com/TheAnsarya/Nexen/issues/695)

## Goal

Define mapper implementation priority and a dependency-aware regression matrix with explicit expected behavior targets.

## Prioritized Mapper Order

1. `2K/4K` fixed mapping baseline.
2. `F8` (8K bankswitch) and `F6` (16K bankswitch).
3. `F4` (32K bankswitch) and `FE`.
4. `E0` segmented bankswitch.
5. `3F` (Tigervision-style).
6. Additional rare or homebrew-specific mappers.

## Justification

- Order favors high coverage of common cartridges first.
- Early phases reduce ambiguity in CPU/TIA/RIOT validation by limiting mapper complexity.
- Later phases isolate high-risk or less-common formats.

## Regression Matrix

| Mapper Family | ROM Archetype | Required Checks |
|---|---|---|
| 2K/4K | Baseline fixed carts | Boot stability, deterministic frame stepping |
| F8/F6 | Mainstream bankswitched carts | Bank transition correctness and no alias drift |
| F4/FE | Larger ROM windows | Correct switch boundary behavior under stress loops |
| E0 | Segmented windows | Segment select and mixed-region read validation |
| 3F | Tigervision style | Conflicts with low-address writes and expected switch behavior |

## Baseline ROM Target Set

| Target ID | Mapper Family | Purpose |
|---|---|---|
| A26-BASE-2K-01 | 2K/4K | Fixed-bank baseline for boot and frame-step determinism |
| A26-BSW-F8-01 | F8 | Basic bank-switch transition and readback verification |
| A26-BSW-F6-01 | F6 | Multi-switch sequence stability validation |
| A26-BSW-F4-01 | F4 | Large-window switching stress case |
| A26-BSW-FE-01 | FE | Alternate switching path validation |
| A26-BSW-E0-01 | E0 | Segmented window selection correctness |
| A26-BSW-3F-01 | 3F | Tigervision-style switching behavior checks |

Note: concrete ROM file names and checksums will be captured in a harness manifest alongside test assets to avoid drift and keep expected behavior immutable.

## Acceptance Thresholds

- Functional: all required checks pass for each implemented mapper family.
- Timing: no new frame-step digest drift introduced by mapper addition.
- Determinism: repeated runs produce stable harness summaries.

## Sequenced Follow-Up Issues

- Phase A: implement 2K/4K + F8/F6 support and tests.
- Phase B: implement F4/FE + E0 with matrix expansion.
- Phase C: implement 3F and rare mapper families with dedicated edge-case tests.

Dependency issue placeholders (to be replaced with issue IDs once created):

- Phase A tracking issue: [#711](https://github.com/TheAnsarya/Nexen/issues/711)
- Phase B tracking issue: [#712](https://github.com/TheAnsarya/Nexen/issues/712)
- Phase C tracking issue: [#713](https://github.com/TheAnsarya/Nexen/issues/713)

## Phase A Evidence

- Implemented in feature branch: fixed `2K/4K`, `F8`, and `F6` mapper handling with bankswitch hotspots.
- Added focused regression tests: `Atari2600MapperPhaseATests`.
- Added digest stability regression check against a Phase A baseline corpus.

Focused command:

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=Atari2600MapperPhaseATests.*:Atari2600TimingSpikeHarnessTests.* --gtest_brief=1
```

Observed result snapshot:

- `[==========] 11 tests from 2 test suites ran.`

## Phase B Evidence

- Implemented `F4`, `FE`, and `E0` mapper handling with bankswitch and segmented-window read mapping.
- Added focused regression tests: `Atari2600MapperPhaseBTests`.
- Re-ran harness and mapper suites together to validate deterministic behavior under the expanded corpus.

Focused command:

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=Atari2600MapperPhaseATests.*:Atari2600MapperPhaseBTests.*:Atari2600TimingSpikeHarnessTests.* --gtest_brief=1
```

Observed result snapshot:

- `[==========] 15 tests from 3 test suites ran.`

## Phase C Evidence

- Implemented `3F` (Tigervision-style) mapper behavior with switchable lower 2K window and fixed upper 2K window.
- Added deterministic rare/homebrew fallback mode (`fallback`) for non-standard mapper cases.
- Updated mapper write routing to observe low-address switch writes while keeping prior mapper paths intact.
- Added dedicated edge-case tests: `Atari2600MapperPhaseCTests`.

Focused command:

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=Atari2600MapperPhaseATests.*:Atari2600MapperPhaseBTests.*:Atari2600MapperPhaseCTests.*:Atari2600TimingSpikeHarnessTests.* --gtest_brief=1
```

Observed result snapshot:

- `[==========] 18 tests from 4 test suites ran.`

## Deferred Future-Work Linkage

Status: Future Work only. Do not start these issues until explicitly scheduled.

- Parent future-work epic: [#717](https://github.com/TheAnsarya/Nexen/issues/717)
- Behavior-accurate mapper completion: [#724](https://github.com/TheAnsarya/Nexen/issues/724)
- Atari compatibility harness expansion: [#725](https://github.com/TheAnsarya/Nexen/issues/725)
- Post-correctness performance gates: [#727](https://github.com/TheAnsarya/Nexen/issues/727)

## Related Research

- [Atari 2600 Bankswitching and Cartridge Formats](../research/platform-parity/atari-2600/bankswitching.md)
- [Platform Parity Source Index](../research/platform-parity/source-index.md)
