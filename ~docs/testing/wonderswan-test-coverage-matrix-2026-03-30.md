# WonderSwan Test Coverage Matrix (2026-03-30)

Parent issue: [#1096](https://github.com/TheAnsarya/Nexen/issues/1096)

## Purpose

Track WonderSwan automated and manual test coverage across CPU, memory, APU, timers, DMA, EEPROM, controller, debugger, movie/TAS, and persistence workflows.

## Existing Coverage Snapshot

| Area | Current Evidence | Status |
| --- | --- | --- |
| PPU rendering logic | `Core.Tests/WS/WsPpuTests.cpp` | Partial |
| Input decode mapping | `Tests/Interop/InputApiDecoderTests.cs` (WonderSwan region) | Partial |
| Pansy metadata export | `Tests/Debugger/Labels/WonderSwanPansyExportTests.cs` | Partial |
| Core memory open-bus baseline | #1089 (`WsMemoryManager` open-bus latch) | Partial |
| CPU instruction correctness | No dedicated V30MZ execution suite in `Core.Tests/WS/` | Gap |
| Memory/banking/port behavior | No dedicated `WsMemoryManager` behavior suite | Gap |
| APU channel/timing behavior | No dedicated `WsApu` behavior suite | Gap |
| DMA and timer interactions | No dedicated `WsDmaController`/`WsTimer` suite | Gap |
| EEPROM behavior and timing | No dedicated `WsEeprom` suite | Gap |
| Controller runtime integration | No dedicated WS runtime controller integration suite | Gap |
| Integration smoke workflows | No WS end-to-end integration harness in `Core.Tests/WS/` | Gap |

## Quality Gates

1. Dedicated test files exist for each subsystem gap area.
2. At least one deterministic integration test validates multi-subsystem frame progression.
3. WS test suites run in regular local validation commands and are CI-visible.
4. TAS/movie and save-state workflows include WS-specific regression checks.

## Execution Plan (Fast Incremental)

1. Add `WsMemoryManagerTests.cpp` for mapped/unmapped reads, open-bus latch, and key port routing.
2. Add `WsControllerTests.cpp` for controller orientation/button/turbo behavior.
3. Add `WsTimerDmaTests.cpp` for timer counters and DMA register progression.
4. Add `WsEepromTests.cpp` for command, busy/ready, and persistence behavior.
5. Add `WsCpuSmokeTests.cpp` for representative instruction/flag/interrupt paths.
6. Add `WsIntegrationTests.cpp` for short deterministic frame-run smoke checks.

## Validation Commands

```powershell
# Build Release x64
& "C:\Program Files\Microsoft Visual Studio\18\Insiders\MSBuild\Current\Bin\MSBuild.exe" Nexen.sln /p:Configuration=Release /p:Platform=x64 /t:Build /m /nologo /v:m

# Run focused WonderSwan suites (expanded as new suites land)
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=Ws* --gtest_brief=1

# Run managed interop and debugger label tests
dotnet test --no-build -c Release --filter "FullyQualifiedName~InputApiDecoderTests|FullyQualifiedName~WonderSwanPansyExportTests"
```
