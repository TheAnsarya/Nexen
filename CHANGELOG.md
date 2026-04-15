# Changelog

All notable changes to Nexen are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.4.33] - 2026-04-15

### Fixed

- **Per-game data migration now requires user confirmation** — ROM load checks for pending save state and battery save migrations and displays a confirmation dialog before moving files (#1274)
- **Menu/config pause suppressed during ROM load** — auto-pause in menu/config dialogs no longer fires during ROM load transitions (#1290, #1300)
- **Pansy.Core package-only enforcement in CI** — CI validates Pansy.Core is consumed as a NuGet package with UI resource prep step (#1298, #1302)
- **UTF-8 stream compatibility and test warning noise** — restored UTF-8 stream compatibility and reduced StringUtilities assertion noise (#1297)
- **GitHub comment formatting** — fixed escaped newline sequences appearing as literal `\n` in published issue comments (#1297)

### Changed

- **GitHub Actions Node 24 runtime** — all CI workflows now force Node 24 runtime, eliminating deprecated Node.js 16/20 warnings (#1303)
- **TODOv2 → issue-scoped TODO triage** — all 20 files with stale `TODOv2` markers converted to `TODO(#issue)` format (#1281)
- **SteamOS guide rewrite** — complete rewrite of `SteamOS.md` with Nexen-specific instructions, linked from README (#1282, #1283)
- **First-party warning reduction** — batch reduction of first-party compiler warnings with CI gating on first-party signal (#1297)

### Added

- **WonderSwan timing gate hardening** — multi-event ordering assertions in WS timing gate tests (#1285, #1287, #1076)
- **Hotpath benchmark stabilization** — repeatable benchmark script with labeled, timestamped JSON output (#1218)
- **Per-game migration count API** — `GetPendingMigrationCount()` and `ConfirmGameDataMigration` dialog (#1274)

### Performance

- **Hotpath branch-prediction triage** — real GBA/NES memory hotpath microbenchmarks tested and discarded non-improving `[[likely]]`/`[[unlikely]]` hints (#1216)

## [1.4.32] - 2026-04-15

### Fixed

- **Settings persistence loss on write failure** — hardening in config save flow now prevents in-memory settings state from being updated when disk writes fail, avoiding silent settings loss across restart (#1291)
- **Startup settings path fallback for read-only portable folders** — when portable config exists but is not writable, startup now automatically falls back to a writable user documents path and migrates `settings.json` when needed (#1295)
- **Menu/config pause triggering during ROM load transitions** — `Pause when in menu and config dialogs` now only pauses while a game is actively running; ROM startup no longer auto-pauses unexpectedly (#1290)

### Changed

- **Save/action notification placement** — boxed save/action notifications are now anchored to the bottom-left with explicit margins to reduce gameplay-center obstruction (#1289)
- **Version metadata and release links** — version identifiers and download links updated to v1.4.32 across release-facing metadata

### Added

- **HUD layout regression coverage** — added deterministic C++ tests for bottom-left notification positioning and viewport clamping in `SystemHudLayoutTests` (#1289)
- **Pause decision regression coverage** — added managed tests for `MainWindow` menu/config pause gating behavior (`MainWindowPauseBehaviorTests`) (#1290)
- **Config home-folder fallback tests** — expanded managed tests for writable path fallback and migration behavior in `ConfigManagerHomeFolderTests` (#1295)

## [1.4.30] - 2026-04-13

### Fixed

- **Menu crash: Debug -> Event Viewer** — Event Viewer window initialization could fail because a required control field was null during XAML-backed control setup; switched the grid control to `x:Name` and hardened constructor lookup to resolve controls by name before creating the ViewModel (#1245)
- **Menu crash: Tools -> TAS Editor -> Open/Create New Movie/Record** — TAS Editor window initialization could throw `NullReferenceException` during control wiring when named controls were not initialized as expected; switched key controls (`SearchBox`, `FrameList`, `PianoRoll`) to `x:Name` for reliable code-behind binding (#1245)
- **TAS Editor fire-and-forget async** — `LoadTasMovieAsync` was called with `_ =` (fire-and-forget), masking potential exceptions; replaced with proper `await` (#1263)
- **Recent Files menu disabled when empty** — `File -> Recent Files` submenu was disabled when no recent files existed; made it always enabled with an "(empty)" placeholder item (#1264)

### Changed

- **Recent Play retention expanded** — increased rolling Recent Play checkpoint capacity from 12 to 36 slots (5-minute cadence, about 3 hours of history) (#1241)
- **Auto Save behavior upgraded to persistent log** — Auto Saves now use timestamped `_auto_` filenames and are no longer single-file overwrite snapshots; save list origin detection supports both legacy and new naming patterns (#1241)
- **Debugger close path hardening** — moved final debugger workspace save/release work off the UI thread and resumed execution before debugger release to reduce close-path deadlock risk when closing the last debugger window (#1242)
- **TAS Editor improvements** — ReactiveUI `[Reactive]` property generation, piano roll button label caching, bare catch elimination, async void safety, undo stack optimization (#1250–#1258)
- **Bare catch elimination (phases 6–8)** — replaced bare `catch` / `catch { }` blocks across UI with typed `catch (Exception ex)` + `Debug.WriteLine` for diagnosability (#1260–#1262)
- **Box-style action notifications** — Tools menu actions (audio/video recording, screenshot, export) now display as centered box-style OSD notifications instead of bottom-left text; simplified message text removes file paths (#1264)
- **GitHub Actions metadata** — added missing `description` properties to `setup-ccache-action` and `build-sha1-action` composite actions

### Performance

- **HexEditor per-frame allocation removed** — `HexEditorDataProvider.Prepare()` called `.ToArray()` on every frame to produce a byte array from an existing byte array; eliminated the redundant copy (#1263)
- **BreakpointManager LINQ → manual loop** — `GetMatchingBreakpoint()` replaced LINQ `.FirstOrDefault()` with a manual `foreach` loop on the hot path (#1263)

### Added

- **Recording filename timestamps** — Sound and video recorder default filenames now include `yyyy-MM-dd_HH-mm-ss` timestamps to prevent silent overwrites of previous recordings (#1265)
- **Event subscription cleanup** — AssemblerWindow and PaletteViewerWindow now properly unsubscribe from events in `OnClosing` to prevent memory leaks (#1235)

## [1.4.25] - 2026-04-10

### Fixed

- **Linux/macOS build failure** — `std::chrono::clock_cast` is a C++23 feature unavailable in clang + libstdc++ on Ubuntu 22.04 / macOS 14; all Linux (clang, gcc, AoT, AppImage) and macOS builds were failing; replaced with portable `time_point_cast` arithmetic already used elsewhere in the same file (`SaveStateManager.cpp`) (#1219)
- **clang `[[nodiscard]]` placement error in `LynxMemoryManager`** — `[[nodiscard]]` placed after `__forceinline` was parsed by clang as an attribute on the return type rather than on the function, causing all Linux/clang CI jobs to fail with `'nodiscard' attribute cannot be applied to types`; moved `[[nodiscard]]` before `__forceinline` (#1220)
- **Accuracy test CI always failing** — `accuracy-tests.yml` builds the full `Nexen.sln` including `UI.csproj`, which has a `ProjectReference` to `Pansy.Core`; the pansy repo was never cloned in CI causing `CS0246: The type or namespace name 'Pansy' could not be found` on every run; added pansy checkout to both `smoke-tests` and `accuracy-tests` jobs (#1219)
- **C++ tests CI missing pansy checkout** — Same pansy reference issue affected `tests.yml`; added checkout there as well (#1219)
- **Accuracy tests running on every push** — `accuracy-tests.yml` `smoke-tests` job ran on every push to master, failing constantly; removed `push`/`pull_request` triggers so the workflow only runs on `workflow_dispatch` (manual) and on the nightly schedule (#1219)
- **Avalonia 12 menu icon loading crash** — `ImageUtilities` threw an unhandled `ArgumentException` on Avalonia 12 when SVG assets failed to decode; hardened the icon load path with SVG → PNG fallback and a non-throwing empty-image fallback to prevent UI-thread crashes (#1212)
- **ReactiveUI Avalonia runtime initialization** — Startup crash regression introduced by Avalonia 12 migration when `UseReactiveUI()` was removed from the builder; restored correct `ReactiveUI.Avalonia` runtime initialization (#1205)
- **Avalonia SVG image loading pipeline** — Avalonia 12 changed the SVG loading API; replaced the old load path with the `Svg.Controls.Skia.Avalonia` pipeline so SVG assets render correctly without relying on PNG fallback masking (#1214)

### Performance

- **CDL recording page cache — ~75–80% CDL overhead reduction** — SNES (and other platforms) emulation ran significantly slower with CDL recording active because every memory read triggered 2 virtual calls + a switch statement per read (~10–15 ns × 3–4M reads/sec); replaced with a flat 4 KB page cache mapping CPU-space addresses directly to absolute ROM offsets, eliminating all virtual calls from the `RecordRead` hot path; `RebuildPageCache` keeps the cache coherent for consoles with dynamic mapping (#1207)
- **Avalonia rendering pipeline lock reduction** — Consolidated `DrawOperation` from 2 separate bitmap lock/unlock pairs to 1, reducing per-frame bitmap lock count from 3 to 2; lowered `DispatcherPriority` from `MaxValue` to `Render` (#1207)
- **SNES memory handler devirtualization** — Every SNES CPU memory access (~3.58M/sec) went through `IMemoryHandler::Read()/Write()` virtual dispatch; for the common case (`RamHandler`/`RomHandler`) this resolved to a trivial array access; added direct-access pointer fields (`_directReadPtr`, `_directWritePtr`, `_directMask`) to `IMemoryHandler` with RAM/ROM handlers opting in at construction time, enabling `SnesMemoryManager` to bypass virtual dispatch with an inline array access for the dominant code paths (#1208)
- **NES VS Dual per-frame allocation eliminated** — `NesPpu::SendFrameVsDualSystem()` allocated `make_unique<uint16_t[]>` (240 KB) on every frame; moved to lazy member allocation (#1208)
- **NES internal RAM fast path** — NES CPU reads to internal RAM (~30% of all CPU reads) went through virtual dispatch via `INesMemoryHandler::ReadRam()`; added `_internalRamPtr`/`_internalRamMask` fields in `NesMemoryManager` so `Read()`/`Write()` bypass virtual dispatch for this case, bringing ~90% of all NES CPU memory accesses through a devirtualized fast path (60% mapper + 30% internal RAM) (#1209)
- **Atari 2600 `std::function` elimination** — Replaced `std::function<uint8_t(uint16_t)>` read/write bus callbacks with a direct `Atari2600Bus*` pointer, eliminating the type-erasure overhead and enabling inlining on every CPU memory access (#1209)
- **Multi-system memory dispatch optimization** — Targeted SMS, Game Boy, PCE, Genesis, and Lynx memory managers with a combination of cached `CheatManager*` pointers (eliminating double-dereference per read), `[[likely]]`/`[[unlikely]]` branch annotations on the dominant paths, and Lynx fast-path/slow-path split (inline header fast path for `addr < 0xfc00` → direct RAM; `.cpp` slow path for overlay dispatch) (#1211)
- **ChannelF CPU callback removal** — Finalized removal of callback-based dispatch in the ChannelF CPU/memory-manager path, replacing with direct calls (#1215)
- **WonderSwan flash dirty-state caching** — Added dirty-flag caching to the WonderSwan flash save-state path to avoid unnecessary I/O on unmodified flash banks (#1215)
- **GBA/NES hotpath benchmarks — `[[likely]]`/`[[unlikely]]` branch hints discarded** — Real emulator-path microbenchmarks for GBA `ProcessDma`, `ProcessInternalCycle` (0%/5%/50% pending-update distributions), and NES `BaseMapper::ReadVram`; measured and discarded `[[likely]]`/`[[unlikely]]` annotations that caused a +14.7% regression on the measured hot paths (#1217, #1218)
- **Repeatable benchmark script** — Added `~scripts/run-memory-hotpath-benchmarks.ps1` for consistent hotpath perf audits with labeled, timestamped JSON output stored under `BenchmarkDotNet.Artifacts/results/hotpath/` (#1218)

---

## [1.4.24] - 2026-04-09

### Changed

- **Avalonia 12.0.0 migration** — Full migration from Avalonia 11.x to 12.0.0, including all API changes, builder pattern updates, and package alignment (#1200)
- **Full dependency modernization** — Updated all NuGet packages to latest versions: Avalonia 12.0.0, Dock.Avalonia 12.0.0.2, ReactiveUI.Avalonia 11.4.12, SkiaSharp.NativeAssets.Linux 3.119.2, xunit.v3 3.2.2, and more (#1200)
- **ReactiveUI.Fody → SourceGenerators** — Migrated from ReactiveUI.Fody (189 files) to ReactiveUI.SourceGenerators 2.6.1 for compile-time property generation (#1201)
- **xUnit v2 → v3** — Migrated test projects from xunit 2.9.3 to xunit.v3 3.2.2 with updated assertions, `CancellationToken` patterns, and `IAsyncLifetime` changes (#1202)

### Added

- **Save state OSD redesign** — Centered badge/date/time box layout for save state on-screen display with improved visual hierarchy (#1199)

### Fixed

- **Native libs always updated in portable mode** — `DependencyHelper` now always updates native libraries to latest version in portable mode, not just on fresh installs (#1200)
- **Post-migration build warnings** — Resolved all CS8618, RXUISG0020, and xUnit1051 warnings from the modernization migration (#1204)

---

## [1.4.23] - 2026-04-09

### Fixed

- **Fresh install crash (Windows/Linux portable)** — `DependencyHelper.ExtractNativeDependencies()` unconditionally skipped extracting `.dll`/`.so` files in portable mode even on fresh installs; now only skips when the file already exists at the destination (#1176, #1194, #1039)
- **FormatException on empty CRC32** — `RomHashInfo.Crc32Value` used `uint.Parse()` on empty string during ROM unload; changed to `TryParse` with 0 fallback (#1039)
- **Linux AoT exit crash** — `TaskCanceledException` from `Tmds.DBus.Protocol` during Avalonia dispatcher cleanup on Linux exit; added `Environment.Exit(0)` after normal shutdown and defensive `UnhandledException` handler (#1195)
- **Designated slot shortcuts not configurable** — Designated slot shortcuts were missing from `PreferencesConfigViewModel.displayOrder`, making them invisible in Preferences (#1193)
- **Accuracy test CI restore** — Added `dotnet restore` step to accuracy test CI workflow (#1192)
- **Whitespace inconsistencies** — Fixed namespace declaration whitespace across multiple files

### Added

- **3 designated save slots (F2-F4)** — Expanded from 1 to 3 designated quick-access save slots with timestamped filenames, history preservation, and slot-specific purple badges showing "Slot 01"-"Slot 03" (#1196)
- **Slot save history** — Designated slot saves now create new timestamped files (`{RomName}_[slot{NN}]_{timestamp}.nexen-save`) instead of overwriting; old saves remain browsable with slot badges in the save state viewer (#1196)
- **Slot number in interop** — Added `slotNumber` field through the full C++ → C# interop pipeline for first-class slot identity in the UI

### Changed

- **Designated save filename format** — Changed from `_designated_{N}_` to `_[slot{NN}]_` for clearer identification; legacy filenames still recognized (#1196)
- **Default slot hotkeys** — F2/Shift+F2 (slot 1), F3/Shift+F3 (slot 2), F4/Shift+F4 (slot 3); F1 remains infinite saves
- **Dark purple badge for non-current slot saves** — Historical slot saves (superseded by a newer save in the same slot) now show a darker purple badge; current (newest) save per slot keeps bright purple
- **NuGet package updates** — Avalonia 11.3.11→11.3.13, Dock.* 11.3.11.7→11.3.11.22, SkiaSharp.NativeAssets.Linux 3.116.1→3.119.2, StreamHash 1.11.2→1.11.3

### Security

- **Tmds.DBus.Protocol pinned** — Pinned from transitive 0.21.2 to direct 0.92.0 to resolve GHSA-xrw6-gwf8-vvr9

---

## [1.4.22] - 2026-04-08

### Fixed

- **Save state timestamp DST offset** — Timestamps displayed 1 hour off during Daylight Saving Time; fixed `ParseTimestampFromFilename()` to use `tm_isdst = -1` for auto-detection instead of assuming standard time (#1188)
- **Screen not updating after loading paused save state** — Loading a save state taken while paused now always pushes the frame to the video decoder, regardless of current pause state (#1188)
- **Pause icon rendering** — Replaced Unicode ⏸ character (rendered inconsistently across fonts) with two drawn Border rectangles for pixel-perfect pause indicator (#1188)
- **Recent Play save fires immediately on ROM load** — First Recent Play auto-save no longer triggers ~100ms after launch; timer initializes to current time instead of Unix epoch, and resets on game load (#1187)
- **Save state picker crash** — Serialized preview loading and added pause byte version validation to prevent crashes from corrupted save states (#1186)

### Added

- **Yellow pause badge** — Paused save states now show a yellow "PAUSED" badge in the visual save state browser (#1176)
- **Save state timestamp parsing tests** — 20 new tests covering valid formats, DST regression, malformed inputs, and edge cases (#1189)
- **Save state timestamp parsing benchmarks** — 4 new benchmarks for single parse, legacy format, invalid early-exit, and batch scan (#1189)

### Changed

- **Eliminated all Nexen-owned compiler warnings** — Fixed 13 warnings across Core, Core.Tests, and Core.Benchmarks (unused variables, deprecated API usage, discarded `[[nodiscard]]` returns) (#1189)
- **`ParseTimestampFromFilename` made static** — Pure function with no instance state, enabling direct unit testing

## [1.4.21] - 2026-04-07

### Fixed

- **Games no longer start paused with debugger open** — Fresh game loads now auto-resume instead of staying paused; `BreakOnPowerCycleReset` only applies to actual power cycles, not new game loads (#1184)
- **Save states now respect pause state on load** — Removed C# UI layer's unconditional resume that was overriding the saved pause state from the C++ layer (#1185)
- **Already Running dialog** — Shows a dialog with "Close and Restart" or "Leave Current Version Running" options when a second instance is launched, instead of silently exiting (#1180)
- **Setup wizard restart reliability** — Uses `ProcessStartInfo` with `UseShellExecute` and error handling for reliable restarts (#1181)
- **Firmware auto-selection** — Automatically scans firmware folder for files matching expected SHA-256 hash before showing file picker dialog (#1182)
- **UI freeze on emulation crash** — Added exception handling around the main emulation loop to prevent UI lockup on fatal errors (#1183)

### Changed

- **Splash screen minimum display** — Increased minimum splash screen time from 2.0s to 2.5s; no maximum (waits for init to complete)

## [1.4.20] - 2026-04-06

### Added

- **Save state pause persistence** — Save states now preserve the emulator pause state; loading a save state taken while paused restores the paused state, and vice versa (save state format v5) (#1039)
- **FullscreenResolution benchmarks** — New BenchmarkDotNet suite comparing dictionary lookup vs string parsing for resolution methods
- **WatchFormat benchmarks** — New BenchmarkDotNet suite for hex formatting and watch removal patterns
- **FullscreenResolution tests** — 14 new xUnit tests validating all resolution values, ordering, and edge cases

### Changed

- **FullscreenResolution lookup optimization** — Replaced string parsing (ToString + Substring + Int32.Parse) with static dictionary lookup for GetWidth/GetHeight; eliminates all string allocations (#1170)
- **WatchManager hex format** — Fixed hex format specifier from uppercase to lowercase per coding standards; uses string interpolation instead of concatenation (#1170)
- **WatchManager.RemoveWatch optimization** — Replaced LINQ `.Where().ToList()` with in-place backward `RemoveAt` loop; eliminates intermediate list allocation (#1170)

## [1.4.19] - 2026-04-06

### Added

- **WonderSwan GetAudioTrackInfo** — Live APU channel state reporting: CH1-CH4 frequency/volume, HyperVoice, PCM, sweep, noise status (#1168)
- **SMS GetAudioTrackInfo** — Live PSG state reporting: 3 tone channels + noise + FM audio detection; GameTitle varies by SMS model
- **GBA GetAudioTrackInfo** — Reports 6 audio channels: SQ1, SQ2, WAV, NOI (PSG) + DMA-A, DMA-B with frequency/volume/mode details
- **GBA RefreshRamCheats** — Cheat engine now applies RAM cheats for GBA (was a stub)
- **Lynx debugger ProcessMemoryAccess** — Wired central memory access tracking for register event logging in the Lynx debugger
- **WS settings serialization** — WonderSwan Model and controller types now saved/restored in settings
- **GBA internal memory control register** — Register 0x04000800 writes now handled (no-op, documented)
- **Atari 2600 Lua test scripts** — `memory-poll.lua` (blargg-style) and `boot-smoke.lua` (RIOT timer verification)
- **Channel F boot-smoke test** — Lua test script for Channel F system verification
- **Expanded test manifests** — 73 tests across 11 platforms (NES 22, GB 12, GBA 9, SNES 7, PCE 5, WS 5, A2600 3, SMS 3, Lynx 3, Genesis 2, Channel F 2)
- **WonderSwan VTOTAL edge-case tests** — 6 PPU VTOTAL + 9 GDMA cycle-counting tests (#1169)
- **InteropDebugConfig Lynx/A2600/ChannelF support** — Added missing Break-on fields and per-system debugger configs (#1163)
- **Accuracy testing CI workflow** — Nightly and manual dispatch CI with ROM download scripts for 11 platforms (#1131, #1138)
- **CPU/hardware test ROMs** — Custom boot smoke ROMs for WS, Lynx, and PCE (#1139, #1140, #1141)
- **Genesis/SMS/PCE/WS test ROM download scripts** — Automated acquisition for accuracy test suites (#1147)

### Fixed

- **Startup crash hardening** — Fixed TypeInitializationException and premature HomeFolder creation on startup (#1076, #1039)
- **GBA debugger register writes** — DebugWrite for I/O registers now explicitly skips writes to prevent side effects (matches GB pattern)
- **ST018 dead code removal** — Removed leftover `GbaCpu::StaticInit()` call and unused include in ST018 coprocessor
- **Lynx palette register address** — Corrected timer test ROM palette register (#1140)
- **SMS SegaCart field documentation** — Clarified that `_bankShift` and `_romWriteEnabled` are set from $FFFC but not yet consumed
- **SMS DebugWrite documentation** — Clarified raw-byte-only semantics matching GB pattern
- **GBA KEYINPUT unused bits** — Documented bits 10-15 behavior (read as 0)
- **Trailing whitespace** — Removed trailing whitespace in DebugConfig.cs, DebuggerConfig.cs, namespace declarations

### Changed

- **EditorConfig** — Assembly tab width corrected from 8 to 4
- **Markdownlint** — Disabled MD007 rule (tab indentation is 1 char, not 4)

### Closed Issues

- #1168 — WS: Implement GetAudioTrackInfo / ProcessAudioPlayerAction
- #1163 — InteropDebugConfig C# struct field alignment
- #1169 — WS: Add VTOTAL edge-case and GDMA cycle-counting tests
- #1164, #1165, #1167 — Closed as duplicates

---

## [1.4.18] - 2026-04-03

### Added

- **Splash screen minimum 2-second display** — Splash screen now shows for at least 2 seconds so branding is visible (#1160)
- **Automated accuracy test infrastructure** — Batch test runner, Lua test scripts (blargg NES/SNES/GB, mooneye GB, generic memory-poll), test manifests, ROM download scripts, CI workflow (#1131, #1133, #1134, #1136, #1137, #1138)
- **RunTest configurable frame count and early exit** — `RunTest` C++/C# interop now accepts `frameCount` and `earlyExitByte` parameters (#1152, #1153, #1155, #1154)

### Fixed

- **WonderSwan PPU double-buffer row index mismatch** — Fixed row index mismatch with low VTOTAL modulo (#1076)
- **Setup wizard deferred HomeFolder creation** — Prevents .so overwrites in portable mode, disables AppImage portable option (#1039)

### Changed

- **GitHub Actions Node.js 24 migration** — Updated all CI actions to Node.js 24-compatible versions, replaced unmaintained write-file-action with shell command (#1156)

### Documentation

- Automated accuracy testing infrastructure plan and session documentation (#1131)
- WonderSwan PPU cross-emulator verification analysis (#1076, #1130)

---

## [1.4.17] - 2026-03-31

### Added

- **Channel F complete emulation core** — Scanline-based frame execution with per-line rendering, correct VBLANK timing, 8-color palette with per-row selection, accurate 4-tone audio model, F8 interrupt delivery with ICB/VBLANK/SMI vector ports (#977, #979, #976, #1125)
- **Channel F SMI timer and IRQ** — PSU/SMI/DMI timer state with ports 0x20/0x21 (#974)
- **Channel F TAS editor integration** — Full TAS editor layout tests for Channel F (#1011)
- **Channel F DisUtils test coverage** — 159 opcode disassembly tests (#1118)
- **Lynx ComLynx multiplayer wiring** — Shared ComLynx cable lifecycle and runtime wiring tests (#955)
- **Lynx bank-addressing validation tests** — Commercial ROM validation matrix (#1105)
- **WonderSwan Flash/RTC test coverage** — Expanded test coverage and benchmarks (#1116)
- **LightweightCdlRecorder + BitUtilities tests** — Additional test coverage and benchmarks (#1117)
- **TestRunner diagnostic improvements** — Named error constants, stderr diagnostics, resource cleanup (#1105)

### Fixed

- **Channel F F8 CPU cycle timing** — Return clock cycles not byte lengths (#1123)
- **Channel F INC opcode** — Now correctly increments accumulator, not DC0 (#1124)
- **Channel F F8 flag bit layout** — Complementary sign and CI/CM carry polarity (#1128)
- **Channel F DisUtils opcode tables** — Corrected $88-$8E memory ALU and $8F BR7 (#1119)
- **Channel F battery save removal** — Console has no battery-backed SRAM
- **Lynx boot-smoke Lua tooling** — Fixed emu.stop() → emu.exit() for proper test-runner integration

## [1.4.12] - 2026-03-30

### Fixed

- **Critical: Updated Dependencies.zip with fresh NexenCore.dll** — v1.4.11 shipped with a stale NexenCore.dll that was missing the `SetAtari2600Config` entry point, causing `EntryPointNotFoundException` on startup and preventing the application from functioning correctly
- **Updated release documentation** — Refreshed RELEASE.md and docs/README.md with current platform support and CI workflow details

## [1.4.11] - 2026-03-30

### Added

- **Pansy SOURCE_MAP section export** — Peony/Nexen pipeline now exports source location mapping to Pansy files (#1069)
- **Channel F TAS gesture lanes** — Full TAS editor support with gesture-based input lanes for Channel F controllers (#1012)
- **WonderSwan test coverage expansion** — Comprehensive state helper, controller behavior, and type-contract test suites (#1096)
- **Lynx commercial ROM validation matrix** — Scaffold for systematic validation against commercial Lynx titles (#1105)
- **UI config/menu regression tests** — Automated regressions for configuration and menu paths (#1043, #1045)

### Fixed

- **WonderSwan EEPROM accuracy** — Implemented cart EEPROM abort control behavior and command-specific timing delays (#1090, #1091)
- **WonderSwan I/O correctness** — Enforced SystemTest port write-only semantics, latched open bus for unmapped memory/ports, side-effect-free debug peeks for serial/input (#1087, #1088, #1089)
- **WonderSwan debugger tracing** — Exposed prefetch reads to debugger trace output (#1094)
- **WonderSwan turbo button support** — Completed turbo button path and clamped speed to valid range (#1095)
- **WonderSwan APU SoundTest** — Documented force-output semantics (#1086)
- **AppImage hardening** — Hardened single-file runtime assets and clang flags for Linux AppImage builds (#1102)
- **CI improvements** — Manual-only Build Nexen workflow with serialized runs (#1101), calibrated warning baselines with strict regression gate (#1056), expanded runtime dependency validation (#1054)

### Documentation

- Channel F readiness, troubleshooting, and preset documentation (#1014, #1020, #1021, #1022, #1023)
- Lynx cart shift-register addressing audit (#956)
- Epic closure checkpoints for modernization and UI quality (#1040, #1070, #1073, #1074)
- Modernization baseline policy and gap closure (#1050)

---

## [1.4.9] - 2026-03-29

### Added

- **WonderSwan issue tracking** — Created 11 WonderSwan/WSC issues for accuracy, cart features, test coverage (#1086-#1096)
- **Channel F PansyImporter support** — Added Channel F (0x1f) and missing platform mappings to PansyImporter (#1008)
- **Channel F file dialog** — Added `.chf` and `.bin` extensions to open ROM dialog with dedicated "Channel F ROM files" filter (#1098)
- **CDL/Pansy defaults enabled** — `AutoLoadCdlFiles` and `BackgroundCdlRecording` now enabled by default (#1098)

### Fixed

- **Linux clang/AoT performance** — Fixed critical build bug where CI command-line `NEXENFLAGS` override stripped `-O3`, `-flto=thin`, and `-m64` flags from clang builds. Introduced `EXTRA_CXXFLAGS` variable to pass `-stdlib=libc++` without overriding internal optimization flags (#1097)
- **C++ benchmark CI stability** — Fixed benchmark smoke step false-failure mode by capturing benchmark output to log file before truncation preview, preserving true process exit codes (#1099)
- **Benchmark smoke scope hardening** — Restricted benchmark smoke filter to stable CPU subset (`BM_(NesCpu|SnesCpu|GbCpu)_.*`) to avoid crash-prone full-suite execution in push CI (#1100)
- **Cross-platform build fix** — Replaced MSVC-only `strncpy_s` with cross-platform `snprintf` in LynxConsole (fixed Linux/macOS/AppImage build failures)
- **CI workflow fixes** — Fixed vcpkg manifest mode in C++ Tests workflow, improved native lib copy step in build workflow to eliminate confusing error messages
- **First-party compiler warnings eliminated** — Removed unused variables (ChannelFEventManager, EmuApiWrapper), added `(void)` casts for `[[nodiscard]]` returns in Emulator and MovieRecorder
- **Missing final newlines** — Added final newlines to CamstudioCodec.cpp and RawCodec.h

---

## [1.4.5] - 2026-03-28

### Added

- **GSU HiROM memory mapping** — SuperFX coprocessor now supports HiROM cartridge layouts (upstream Mesen2 PR #89) (#1061)
	- HiROM banks $40-$6f, $72-$7d mapped with 64KB page increments, mirrors at $c0-$ef, $f2-$ff
	- RAM override at $70-$71 for HiROM carts
	- GSU-side linear 64KB-per-bank addressing preserved
- **Channel F TAS editor support** — Full controller layout with 8 buttons (→, ←, Bk, Fw, ↺, ↻, Pl, Ps) (#1063)
- **Channel F movie converter** — Added `SystemType.ChannelF` for movie format conversion (#1063)

### Fixed

- **GSU accuracy improvements** — Four fixes from upstream Mesen2 PR #90 (#1062):
	- STOP instruction now flushes pixel cache before halting
	- R14 ROM stall guard prevents hangs when program bank > $5f
	- RomDelay/RamDelay underflow prevention (explicit conditional instead of `std::min`)
	- GO register ($301f) properly resets wait flags, pending delays, and clamps CycleCount
- **Channel F tab localization** — Replaced hardcoded "Channel F" text with localized resource key (#1063)

---

## [1.4.4] - 2026-03-27

### Fixed

- **Linux SIGSEGV crash on startup** — Bundle ICU 72.1 via `Microsoft.ICU.ICU4C.Runtime` NuGet package instead of relying on system ICU (#1039)
	- Fixes `locale_get_default_76()` crash in `libicuuc.so.76` on KUbuntu 25.10 and other Linux distros with incompatible system ICU
	- App-local ICU configured via `runtimeconfig.template.json` with `System.Globalization.AppLocalIcu`
	- Full internationalization/multi-language support preserved (not using InvariantGlobalization)

## [1.4.0] - 2026-03-07

### Added

- **Pansy.Core Library Integration** — Full integration with shared Pansy.Core NuGet library (#586)
	- PansyExporter uses `Pansy.Core` enum types (`SymbolType`, `CommentType`, `CrossRefType`, `MemoryRegionType`)
	- `LabelMergeEngine` integrated for intelligent label deduplication and conflict resolution
	- PansyImporter rewritten to use `PansyLoader` for spec-compliant file reading
- **Pansy CPU State Section** — Export per-address CPU mode flags (#578, #579, #580)
	- SNES: IndexMode8/MemoryMode8 from CDL flags per code byte
	- GBA: Thumb mode from CDL flags per code byte
	- 9-byte entry format: Address(4) + Flags(1) + DataBank(1) + DirectPage(2) + CpuMode(1)
	- 21 correctness tests and benchmarks (SpanWrite: 2.1x faster, 50% less memory)
- **Pansy Data Types Section** — Export structured data annotations from labels and CDL (#582, #584)
- **Pansy Hot Reload** — Automatic re-import of Pansy files when changed on disk (#582)
- **Atari Lynx File Format** — `.atari-lynx` format reader/writer with auto-detection (#571-#577)
	- Full format specification, 50 unit tests
	- Cart address shift protocol and HLE boot support
	- ROM file format conversion libraries (.lnx/.lyx/.o/.atari-lynx)
- **TAS Infrastructure Tests** — 57 undo/redo and clipboard operation tests (#563)
- **Save State Manager Benchmarks** — GreenzoneManager capture/lookup/invalidation benchmarks (#564)

### Changed

- **PansyExporter cleanup** — Removed 128 lines of dead code (4 superseded methods), added `FLAG_HAS_CROSS_REFS` (#587)
- **Pansy spec alignment** — Removed count prefixes and extra bytes from section builders (#47)

### Performance

- **NES memory read fast-path** — Avoids virtual dispatch (1.84x faster) (#556)
- **Async save state writes** — Background thread for save state compression (#557-#560)
- **SnesPpuState field reordering** — Cache locality optimization (#525)
- **Rewind interval scaling** — Scale recording interval with emulation speed (#423)
- **Branch prediction hints** — `[[likely]]`/`[[unlikely]]` on hottest unannotated paths (#553)
- **Constexpr expansion** — Pure computation functions marked constexpr (#554)
- **`[[nodiscard]]` attributes** — Applied to critical query methods (#555)

### Fixed

- **Lynx sprite viewer** — Fixed packed mode and tilemap flip mode (#504, #505)
- **Lynx Timer 7 cascade** — Implemented Timer 7 → Audio Channel 0 cascade link (#496)
- **Lynx SYSCTL1 bank strobe** — Implemented bank strobe and cart write path (#499)

---

## [1.3.1] - 2026-03-04

### Fixed

- **Game Boy crash on ROM load** — Fixed null pointer dereference in `DebuggerRequest` copy constructor (#552)
	- Root cause: `std::make_unique` materializes a temporary and calls the copy constructor, unlike `reset(new T(...))` which benefits from C++17 mandatory copy elision
	- Copy constructor accessed `_emu->_debugger` without null-checking `_emu`
	- Affected all ROM loads (NES, SNES, GB, GBA, etc.) when debugger block count > 0
- **Cross-platform CI build failures** — Fixed `strncpy_s` and incomplete type with `unique_ptr` (#550)
	- `strncpy_s` → `strncpy` for Linux/macOS compatibility
	- Added explicit destructors for classes using `unique_ptr` with forward-declared types

### Changed

- **Pansy export/import spec alignment** — Full binary compatibility with Pansy v1.0 spec (#539–#546)
	- Fixed header format, section IDs, platform IDs, cross-ref types, memory region types
	- Fixed compression to use DEFLATE instead of GZip
	- Added DRAWN/READ/INDIRECT CDL flags and fixed SNES CDL collision
	- Added symbol/comment types and metadata section support
	- Fixed importer to match exporter format
	- Optimized cross-ref builder performance
- **Defensive startup** — Added try-catch in MainWindow `ApplyConfig` to prevent startup crashes
- **Internal version bump**: 2.2.0 → 2.2.1

### Performance

- **SNES PPU window mask precomputation** — Precompute window masks once per scanline render call instead of per-pixel (#523)
	- Added `PrecomputeWindowMasks()` called from `RenderScanline()` after `_drawEndX` is set
	- Replaces per-pixel `ProcessMaskWindow` template switch with `bool[6][256]` array lookups
	- Eliminates redundant window count calculation at each call site (sprites, tilemap, mode7, color math)

---

## [1.3.0] - 2026-03-03

### Performance

#### Phase 16.6 — Temp String & Table Elimination

- Eliminated temporary string allocations in TraceLogger, DebugStats, AudioPlayerHud
- Converted runtime-initialized arrays to `constexpr` lookup tables
- Added `reserve()` calls for known-size containers

#### Phase 16.7 — Branch Prediction Hints

- Applied `[[likely]]`/`[[unlikely]]` attributes across 9 CPU/PPU/memory hot-path files
- NES/SNES/GB/GBA/SMS/PCE/WS CPU cores, PPU renderers, memory handlers

#### Phase 16.8 — Constexpr LUT + Copy Elimination

- Base64 decode table: `constexpr` namespace-scope LUT (8.8x faster CheckSignature)
- NTSC filter, FDS audio, VS System: `static constexpr` member arrays
- VirtualFile, FolderUtilities: pass-by-const-ref, `string_view` params (66.3x faster extension set lookup)

#### Phase 16.9 — String Allocation Elimination

- Serializer::UpdatePrefix: in-place append instead of repeated concatenation
- FolderUtilities::CombinePath: `reserve()` + `append()` (eliminates intermediate allocations)
- PcmReader::LoadSamples: pre-reserve sample vector
- DebugStats: `std::format` replaces `stringstream` (3.8x faster)
- **Finding**: MSVC `std::format` is 3.4x slower than `operator+` for short strings — only use to replace `stringstream`

#### Phase 16.10 — Stringstream Elimination

- MD5/SHA1: hex digest via `std::format` (replaces `stringstream` + `setfill`/`setw`)
- RomLoader, NsfLoader: CRC/address formatting via `std::format`
- SaveStateManager: timestamp formatting (×3 sites)
- MessageManager, Debugger, ScriptingContext: `GetLog()` join via `std::format`
- AudioPlayerHud: `reserve()` for output string

### Changed

- Internal version bump: 2.1.1 → 2.2.0
- Missing trailing newlines added to header files

## [1.2.0] - 2026-03-01

### Added

- **Atari Lynx Emulation** — Complete new emulation core (not a Handy fork)
	- 65C02 CPU core with full instruction set and hardware bug emulation
	- Mikey: timer system, interrupts, display DMA, palette, audio
	- Suzy: sprite engine with all BPP modes, quadrant rendering, stretch/tilt, hardware math, collision
	- 4-channel LFSR audio with stereo panning and timer-driven sample generation
	- UART/ComLynx serial port with full register set
	- EEPROM support (93C46/66/86) with auto-detection from LNX header
	- RSA bootloader decryption
	- Boot ROM support with HLE fallback
	- Cart banking with LNX format support
	- MAPCTL memory overlays (ROM, vectors, Mikey, Suzy)
	- Debugger: disassembler, trace logger, event manager, effective address, sprite viewer
	- UI: config panel, input mapping, piano roll layout, movie converter
	- Pansy metadata export for Lynx platform
	- ROM database with ~80 No-Intro entries
	- 67 performance benchmarks, 84+ unit tests, TAS integration tests

- **Upstream Mesen2 Fixes** — 8 bug fixes from open Mesen2 PRs
	- PR #87: SNES DMA uint8_t overflow (could miscalculate cycle overhead)
	- PR #82: NES GetInternalOpenBus returning wrong bus value
	- PR #86: SNES CX4 cache/timing (bus delay, preload, IRQ flag, cycle compare)
	- PR #80: SNES hi-res blend pixel pairing (was blurring entire screen)
	- PR #31: NES NTSC filter dot crawl phase, Bisqwit RGB-YIQ matrix, RGB PPU emphasis
	- PR #74: SNES ExLoRom mapping + S-DD1 map mode fix
	- PR #76: Lua crash fix for non-string error objects
	- PR #85: Linux FontConfig typeface caching for Memory View performance

- **Game Package Export** — Export ROM + metadata as `.nexen-pack.zip`
- **Memory-based SaveState API** — Greenzone uses in-memory states (no temp files)
- **TAS Improvements**
	- Joypad interrupt detection for movie playback
	- InsertFrames/DeleteFrames Lua API
	- SortedSet greenzone lookups, bulk operations
	- O(1) single-frame UpdateFrames optimization
	- FormattedText caching in piano roll
	- SeekToFrameAsync batching (replaced Task.Delay with Task.Yield)
- **Movie Converter Fixes** — BK2/FM2/SMV/LSMV import correctness

### Performance

192 commits of optimization work across 13 phases:

- **Phase 7-9: Core Pipeline**
	- FastBinary positional serializer for run-ahead (eliminates string key overhead)
	- Rewind system: audio ring buffer, O(1) memory tracking, compression direct-write
	- Audio pipeline: ReverbFilter ring buffer, SDL atomics
	- Profiler: cached pointers (6-9x), callstack ring buffer (2x)
	- Lightweight CDL, VideoDecoder frame skip, SoundMixer turbo audio
	- DebugHud: flat buffer pixel tracking (replaces unordered_map)
	- Pansy auto-save off UI thread, ROM CRC32 caching

- **Phase 10-13: Hot Path Audit**
	- Move semantics for RenderedFrame/VideoDecoder
	- HUD string copy elimination, DrawStringCommand move
	- ExpressionEvaluator const ref (9x string copy elimination)
	- ClearFrameEvents swap, GBA pendingIrqs swap-and-pop O(1)
	- GetPortStates buffer reuse (per-frame alloc elimination)
	- TraceLogger persistent buffers, ScriptingContext const ref
	- NesSoundMixer timestamps reserve, selective zeroing
	- SystemHud snprintf, DebugStats stringstream reuse
	- ShortcutKeyHandler move pressedKeys
	- MovieRecorder/NexenMovie const ref, StepBack emplace_back

- **Lynx-Specific Performance**
	- NZ flag lookup table, LFSR branchless popcount
	- Timer mask optimization, video filter branch elimination
	- Branchless flags, branch hints, unsigned bounds checks
	- Cached GetCycleCount, removed redundant null checks

### Fixed

- **43 compiler warnings** across C++ and C# codebase
- **Lynx emulation** — 100+ bug fixes from comprehensive audit:
	- IRQ firing, timer prescaler units, MAPCTL bit assignments
	- Sprite engine: collision buffer, quadrant rendering, pixel format
	- Math engine: register addresses, signed multiply
	- Audio: tick rate, volume, timer cascade, TimerDone blocking
	- Cart: banking, RCART registers, page parameter
	- UART: 3 bug fixes for serial communication
	- DISPCTL flip mode, PBKUP register, 2-byte NOP opcodes

### Testing

- **1495 tests** across 100 test suites (up from 659 in v1.1.0)
	- 65C02 CPU exhaustive instruction logic tests
	- Lynx: hardware reference correctness tests (63), math unit (16), sprite (50+), fuzz (17)
	- TAS: integration tests, movie converter tests, truncation benchmarks
	- Upstream fix verification tests

---

## [1.1.4] - 2026-02-13

### Fixed

- **CI/CD**: Fixed Windows artifact upload with explicit output directory
	- Added `-o build/publish` to dotnet publish command
	- SolutionDir issue caused output to go outside repo root on CI runners

### Changed

- **CI/CD**: Release asset filenames now include version numbers
	- e.g., `Nexen-Windows-x64-v1.1.4.exe` instead of `Nexen-Windows-x64.exe`
	- Makes it clear which version a downloaded file is
- **CI/CD**: Linux binary releases now distributed as `.tar.gz` tarballs
	- Includes LICENSE file in each tarball
	- AppImages remain as standalone `.AppImage` files
- **README**: Updated download links to use versioned filenames
- **Copilot Instructions**: Added release process documentation with mandatory README update steps

---

## [1.1.3] - 2026-02-13

### Fixed

- **CI/CD**: Fixed Windows artifact upload paths (unsuccessful)
	- Corrected path from `build/TmpReleaseBuild/Nexen.exe` to actual publish output
	- Windows x64 and Windows AOT builds now upload artifacts correctly

---

## [1.1.2] - 2026-02-13

### Fixed

- **CI/CD**: Disabled macOS Native AOT builds temporarily
	- .NET 10 ILC compiler crashes with segfault on macOS ARM64
	- All other builds (Windows, Linux, macOS Standard) now publish correctly
	- See [issue #238](https://github.com/TheAnsarya/Nexen/issues/238) for details

### Changed

- **README**: Updated download links and macOS AOT availability notice

---

## [1.1.1] - 2026-02-12

### Fixed

- **CI/CD**: Fixed GitHub Actions build workflow for all platforms
  - Windows: Fixed native library packaging from NuGet cache
  - macOS: Made code signing optional when certificate not configured
  - macOS: Fixed libc++ linking via NEXENFLAGS environment variable
  - Linux AppImage: Fixed `--self-contained true` syntax (was `--no-self-contained false`)

---

## [1.1.0] - 2026-02-12

### Performance

Major performance optimization pass across all CPU emulation cores and UI rendering.

#### C++ Emulator Core

- **Branchless CPU Flag Operations** - Replaced branching conditionals with branchless equivalents
  - NES: CMP, ADD, BIT, ASL/LSR, ROL/ROR (7 operations)
  - SNES: CMP, Add8/16, Sub8/16, shifts, TestBits (10 operations)
  - GB: SetFlagState branchless implementation
  - PCE: CMP, ASL/LSR, ROL/ROR, BIT, TSB/TRB/TST (10 operations)
  - SMS: Merged ClearFlag in UpdateLogicalOpFlags (3 calls → 1 AND mask)

- **[[likely]]/[[unlikely]] Annotations** - Branch prediction hints on hot-path conditionals
  - All CPU Exec() IRQ/NMI/halt/stopped checks annotated

- **PPU Rendering Optimizations**
  - SNES ApplyBrightness: constexpr 512-byte LUT (33% faster)
  - SNES ConvertToHiRes: cached pixel read (38% faster)
  - SNES RenderBgColor: hoisted _cgram[0] out of per-pixel loop
  - SNES DefaultVideoFilter: row pointer hoisting in all 3 loops
  - GB/GBA video filter: flat loop (7% faster)
  - GB PPU: cached GameboyConfig (160x reduction in config access)
  - SMS VDP DrawPixel: cached buffer offset

- **Utility Library**
  - HexUtilities: constexpr nibble LUT for FromHex, eliminated 256-entry vector<string>
  - StringUtilities: string_view for Split, const ref for Trim/CopyToBuffer
  - Equalizer: vector→std::array+span (10.1x faster)

#### C# UI Allocation Reduction

Eliminated thousands of per-frame GC allocations in debugger UI:

- BreakpointBar: 6 brush/pen → ColorHelper cached lookups
- DisassemblyViewer: static ForbidDashStyle + BlackPen1 (−150 allocs/frame)
- CodeLineData ByteCodeStr: string.Create with hex LUT (−400 allocs/refresh)
- PaletteSelector: ColorHelper brush caching (−256 allocs/frame)
- PianoRollControl: 17 per-frame brush/pen/typeface → static readonly
- HexEditor: pre-allocated string[256] hex lookup table
- HexEditorDataProvider: static string[128] printable ASCII cache
- Utf8Utilities: Marshal.PtrToStringUTF8 + ArrayPool<byte>
- GetScriptLog: pooled buffer
- GetSaveStatePreview: ArrayPool
- PictureViewer: cached Pen/Brush/Typeface
- GreenzoneManager: pre-sized MemoryStream
- MemorySearch: direct loop

### Testing

- **659 tests** (up from 421 in v1.0.0)
  - Exhaustive 256×256 comparison tests for CPU flag operations
  - Branching vs branchless equivalence tests
  - Video filter correctness tests
  - Hex formatting tests

### Documentation

- Updated session logs documenting optimization work
- Phase 2-6 optimization plans in `~docs/plans/`

---

## [1.0.1] - 2026-02-07

### Fixed

- **Critical: Release build startup failure** - Fixed missing NexenCore.dll in Dependencies.zip
  - The embedded Dependencies.zip was not including NexenCore.dll, causing "Dll was not found" error on startup
  - Added [scripts/package-dependencies.ps1](scripts/package-dependencies.ps1) helper script for reliable release builds
  - Disabled problematic PreBuildWindows MSBuild target that was failing with dotnet CLI

### Build System

- Improved release build workflow documentation
- Added package-dependencies.ps1 script for Windows release packaging

---

## [1.0.0] - 2026-02-02

Initial public release of Nexen.

### Added

#### TAS Editor

- **Piano Roll View** - Visual timeline for frame-by-frame input editing
- **Greenzone System** - Automatic savestates for instant seeking and rerecording
- **Input Recording** - Record gameplay with Append, Insert, and Overwrite modes
- **Branches** - Save and compare alternate movie versions
- **Lua Scripting** - Automate TAS workflows with comprehensive API
- **Multi-Format Support** - Import/export BK2, FM2, SMV, LSMV, VBM, GMV, NMV
- **Rerecord Tracking** - Monitor optimization progress

#### Save States

- **Infinite Save States** - Unlimited timestamped saves per game
- **Visual Picker** - Grid view with screenshots and timestamps
- **Per-Game Organization** - Saves stored in `SaveStates/{RomName}/` directories
- **Quick Save/Load** - Shift+F1 to save, F1 to browse

#### Debugging

- **🌼 Pansy Export** - Export disassembly metadata to universal format
- **File Sync** - Bidirectional sync with external label files

#### Technical

- **C++23 Modernization** - Smart pointers, std::format, ranges, [[likely]]/[[unlikely]]
- **Unit Tests** - 421+ tests using Google Test framework
- **Benchmarks** - Performance benchmarks with Google Benchmark
- **Doxygen Documentation** - Generated API documentation

### Platforms

- Windows (x64)
- Linux (x64, ARM64, AppImage)
- macOS (Intel x64, Apple Silicon ARM64)

---

## Version History

Nexen is based on [Mesen](https://github.com/SourMesen/Mesen2) by Sour.

For the original Mesen changelog, see the [upstream repository](https://github.com/SourMesen/Mesen2/releases).
