# Sega Genesis / Mega Drive

## Overview

Nexen supports Sega Genesis / Mega Drive emulation with save states, rewind, movie recording/playback, TAS editor integration, and debugger tooling.

## Hardware Focus

- CPU: Motorola 68000 execution flow and bus timing integration
- Video: Genesis VDP rendering, tile/sprite paths, and DMA behavior
- Audio: YM + PSG mix cadence and bus write paths
- Input: 3-button and 6-button controller layouts (including MODE lane in TAS)

## TAS and Input Notes

- Genesis TAS layout uses a 12-button grid:
  - Row 0: A, B, C, MOD
  - Row 1: X, Y, Z, STA
  - Row 2: Up, Down, Left, Right
- Piano-roll label aliases normalize to canonical ids:
  - `MOD -> MODE`
  - `STA -> START`
  - `SEL -> SELECT`

## Recheck Commands

Run targeted Genesis TAS parity tests:

```powershell
dotnet test Tests\Nexen.Tests.csproj -c Release --filter "FullyQualifiedName~TasEditorViewModelBranchAndLayoutTests|FullyQualifiedName~GenesisTasLayoutTests|FullyQualifiedName~TasInputOverrideMappingParityTests|FullyQualifiedName~InputApiDecoderTests"
```

Run a focused Genesis benchmark (verified command):

```powershell
.\bin\win-x64\Release\Core.Benchmarks.exe --benchmark_filter="BM_Genesis_StepFrameScaffold_OneScanline" --benchmark_min_time=0.01s --benchmark_repetitions=1
```

Run the Genesis VDP benchmark family:

```powershell
.\bin\win-x64\Release\Core.Benchmarks.exe --benchmark_filter="BM_GenesisVdp_.*" --benchmark_min_time=0.01s --benchmark_repetitions=1
```

## Related Links

- [Systems Index](README.md)
- [Performance Guide](../PERFORMANCE.md)
- [TAS Editor Manual](../TAS-Editor-Manual.md)
- [Sega Genesis Architecture and Incremental Plan](../../~docs/plans/genesis-architecture-and-incremental-plan.md)
