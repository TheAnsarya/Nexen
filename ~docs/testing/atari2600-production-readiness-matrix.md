# Atari 2600 Production Readiness Matrix

Parent issue: [#876](https://github.com/TheAnsarya/Nexen/issues/876)

## Purpose

Provide a single checklist for Atari 2600 release-readiness across core emulation, debugger, TAS/movie workflows, save states, screenshots/video, and metadata export.

## Validation Commands

```powershell
# Build Release x64
& "C:\Program Files\Microsoft Visual Studio\18\Insiders\MSBuild\Current\Bin\MSBuild.exe" Nexen.sln /p:Configuration=Release /p:Platform=x64 /t:Build /m /nologo /v:m

# Run full C++ tests
.\bin\win-x64\Release\Core.Tests.exe --gtest_brief=1

# Run full .NET tests
dotnet test --no-build -c Release --nologo -v m

# Target Atari 2600 suites
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=Atari2600IntegrationWiringTests.* --gtest_brief=1
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=Atari2600AudioPhaseATests.* --gtest_brief=1
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=Atari2600TiaPhaseATests.* --gtest_brief=1
```

## Matrix

| Area | Gate | Evidence | Status |
| --- | --- | --- | --- |
| Core boot/run | Atari 2600 ROM loads and executes frames without crash | Integration wiring tests + smoke ROM harness | Complete |
| CPU/RIOT/TIA correctness | Deterministic state/timing behavior under tests | Atari2600CpuPhaseA/RiotPhaseA/TiaPhaseA tests | Complete |
| Mapper behavior | Mapper families switch deterministically and match harness digests | Atari2600MapperPhaseA/B/C/D tests | Complete |
| Controller support | Joystick, Paddle, Keypad, Driving, Booster Grip input behavior verified | Atari2600ControllerTests + config UI wiring | Complete |
| Audio output path | TIA mixed samples reach SoundMixer output pipeline | #844 completion + Atari2600AudioPhaseATests | Complete |
| Video/frame buffer | Frame buffer is rendered and exposed through standard frame interface | Render tests + GetPpuFrame path | Complete |
| Save-state determinism | Save/load replay behavior stable across runs | Atari2600SaveStateDeterminismTests | Complete |
| TAS/movie baseline | A2600 layout detection and subtype mapping in TAS editor | #872 + #873 + #910 + #911 | Complete |
| TAS paddle coordinates | Coordinate edit/visualization in TAS workflow | #874 (closed) | Complete |
| TAS console switches | SELECT/RESET toggle UI and movie roundtrip | #910 + #911 | Complete |
| BK2 converter parity | Joystick, paddle position, console switches in BK2 format | #911 + #912 | Complete |
| Debugger parity | Event viewer + debug pipeline parity and UX validation | #877 (closed) + manual checks | Complete |
| Pansy export parity | A2600 metadata export shape and stability | #878 (closed) | Complete |
| Production docs traceability | Consolidated matrix and linked queue | This document + #847/#841 updates | Complete |

## Remaining Open Work

1. BK2 converter support for keypad/driving/booster-grip controller types ([#913](https://github.com/TheAnsarya/Nexen/issues/913)) — low priority.

## Sign-Off Criteria

Atari 2600 is considered production-ready when all matrix rows are `Complete`, and the latest run of build/C++/.NET suites is green with referenced evidence in the active session log.
