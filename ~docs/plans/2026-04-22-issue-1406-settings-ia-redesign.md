# Issue #1406 Settings Information Architecture Redesign (2026-04-22)

## Scope

Parent epic: #1402

This implementation slice delivers:

- Stable route IDs for settings deep links and tests
- Route resolution that supports context-first Input landing (`active-system`)
- Updated navigation model documentation with migration notes

## Before and After IA Map

### Before

- Settings actions opened tabs directly by enum value from menu click handlers.
- Generic Input entry path depended on ad-hoc constructor logic, not a reusable route model.
- No canonical route ID namespace existed for deep-link entry points.

### After

Top-level route namespace:

- `settings.audio`
- `settings.emulation`
- `settings.video`
- `settings.input`
- `settings.input.active-system`
- `settings.preferences`
- `settings.system.nes`
- `settings.system.snes`
- `settings.system.gameboy`
- `settings.system.gba`
- `settings.system.pce`
- `settings.system.sms`
- `settings.system.ws`
- `settings.system.lynx`
- `settings.system.atari2600`
- `settings.system.channelf`
- `settings.system.other`

Route model behavior:

- Known route IDs resolve through `ConfigViewModel.TryResolveRoute(...)`.
- Input context route `settings.input.active-system` resolves through `ResolveInputLandingTab(...)`.
- Unknown routes safely fall back to `ConfigWindowTab.Audio`.

## Navigation Step Reduction

Common task comparison:

| Task | Before | After |
| --- | --- | --- |
| Open active-system input config from main menu | `Settings > Input` then manually switch to active system tab in many cases | `Settings > Input` lands on active system tab automatically when ROM is loaded |
| Deep-link/test entry into settings areas | Enum-only coupling in code paths | Stable route IDs (`settings.*`) decouple callers from enum ordering |

## Migration Notes

- Existing enum-based tab selection remains supported.
- New call sites should prefer route IDs and `ResolveRoute(...)`.
- `ConfigRouteIds` is the canonical source for route constants.
- `ConfigViewModel.GetRouteId(...)` provides reverse mapping for tests/instrumentation.
- Keep route strings stable to preserve external links and automation contracts.

## Validation

Automated coverage added/updated:

- `ConfigViewModelTests.ResolveInputLandingTab_UsesExpectedConfigTab`
- `ConfigViewModelTests.TryResolveRoute_MapsStableRouteIds`
- `ConfigViewModelTests.ResolveRoute_InputActiveSystem_UsesCurrentConsole`
- `ConfigViewModelTests.GetRouteId_ReturnsExpectedStableId`

Run:

- `dotnet test Tests/Nexen.Tests.csproj -c Release --nologo`
