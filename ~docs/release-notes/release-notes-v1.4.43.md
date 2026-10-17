# Nexen v1.4.43 Release Notes

Nexen v1.4.43 is a cleanup and stability release focused on startup UX, debugger settings reliability, and safer default metadata export behavior.

## Highlights

- Startup gameplay experience is cleaner by default.
- Debugger Settings open behavior was hardened to avoid silent no-op cases.
- Pansy integration defaults were tuned to stay enabled for export/logging workflows.
- Existing user configurations are migrated to disable default startup debug stats overlays.

## Included Changes

1. Startup overlay defaults and migration:
- Added a configuration migration that forces `ShowDebugInfo` off on upgrade.
- Prevents visual/audio/misc stats panels and frame graph from appearing by default at startup.

2. Debugger Settings reliability:
- Updated debugger settings window open path to fall back safely when parent/owner resolution fails.
- Eliminates cases where `Debugger -> Settings` appeared to do nothing.

3. Pansy workflow defaults:
- Playing profile now enables:
	- Background CDL recording
	- Auto-export Pansy
	- Save Pansy on ROM unload
- Debug overlays remain disabled by default for normal gameplay.

## Validation

- Release x64 build passed (`Nexen.sln`).
- Focused SNES tests passed:
	- `Core.Tests.exe --gtest_filter=Snes* --gtest_brief=1`
	- 175/175 tests passed.

## Expected Assets For v1.4.43

- Nexen-Windows-x64-v1.4.43.exe
- Nexen-Windows-x64-AoT-v1.4.43.exe
- Nexen-Linux-x64-v1.4.43.AppImage
- Nexen-Linux-ARM64-v1.4.43.AppImage
- Nexen-Linux-x64-v1.4.43.tar.gz
- Nexen-Linux-x64-gcc-v1.4.43.tar.gz
- Nexen-Linux-ARM64-v1.4.43.tar.gz
- Nexen-Linux-ARM64-gcc-v1.4.43.tar.gz
- Nexen-Linux-x64-AoT-v1.4.43.tar.gz
- Nexen-macOS-ARM64-v1.4.43.zip

## Links

- Release tag: [v1.4.43](https://github.com/TheAnsarya/Nexen/releases/tag/v1.4.43)
- Compare: [v1.4.42...v1.4.43](https://github.com/TheAnsarya/Nexen/compare/v1.4.42...v1.4.43)
