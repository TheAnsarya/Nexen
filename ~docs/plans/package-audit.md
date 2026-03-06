# Nexen Package & Dependency Audit

## Overview

This document catalogs every package and third-party dependency in Nexen (inherited from Mesen2 or added since), evaluates whether each can be replaced with standard .NET / Microsoft libraries, and recommends a path forward.

---

## 1. NuGet Packages (C# / .NET)

### 1.1 Core Framework — KEEP (Essential)

| Package | Version | What It Does | Replaceable? |
|---------|---------|-------------|--------------|
| **Avalonia** | 11.3.11 | Cross-platform UI framework (like WPF but works on Linux/macOS/Windows). The entire UI is built on this. | ❌ **No** — This IS the UI framework. No alternative without a complete rewrite. |
| **Avalonia.Desktop** | 11.3.11 | Desktop platform integration for Avalonia (window management, clipboard, etc.) | ❌ **No** — Required by Avalonia. |
| **Avalonia.Themes.Fluent** | 11.3.11 | Microsoft Fluent Design theme for Avalonia controls. | ❌ **No** — Provides the modern look. Could switch to Simple theme but would look worse. |
| **Avalonia.Controls.ColorPicker** | 11.3.11 | Color picker control for the debugger's palette/tile editor. | ❌ **No** — Used in debugger for editing PPU palettes. Standard Avalonia doesn't include one. |
| **Avalonia.Diagnostics** | 11.3.11 | DevTools overlay (Ctrl+F12) for debugging Avalonia visual tree in Debug builds. | ❌ **No** — Essential for UI development/debugging. Only included in Debug builds. |

### 1.2 MVVM & Reactive — KEEP (Architectural)

| Package | Version | What It Does | Replaceable? |
|---------|---------|-------------|--------------|
| **Avalonia.ReactiveUI** | 11.3.9 | Integrates ReactiveUI with Avalonia. Provides `ReactiveWindow`, `ReactiveUserControl`, reactive bindings, and routing. | ⚠️ **Theoretically** replaceable with CommunityToolkit.Mvvm, but would require rewriting every ViewModel, every reactive subscription, and every `WhenAnyValue`/`ObservableAsPropertyHelper` in the app. **Not practical.** |
| **ReactiveUI.Fody** | 19.5.41 | IL weaving for ReactiveUI — auto-generates `RaisePropertyChanged` for `[Reactive]` properties. Eliminates boilerplate. | ⚠️ Same as above — tied to ReactiveUI. Could switch to source generators but Fody works and is battle-tested. |
| **System.Reactive** | 6.0.1 (transitive) | The Reactive Extensions (Rx) library. Foundation for ReactiveUI's observable streams. | ❌ **No** — Required by ReactiveUI. |
| **DynamicData** | 8.4.1 (transitive) | Reactive collections (observable lists/caches). Used for dynamic list binding. | ❌ **No** — Transitive dep of ReactiveUI. |
| **Splat** | 15.1.1 (transitive) | Service locator / logging abstraction for ReactiveUI. | ❌ **No** — Transitive dep of ReactiveUI. |
| **Fody** | 6.8.0 (transitive) | IL weaving engine used by ReactiveUI.Fody. | ❌ **No** — Required by ReactiveUI.Fody. |

### 1.3 Docking — KEEP (Essential Feature)

| Package | Version | What It Does | Replaceable? |
|---------|---------|-------------|--------------|
| **Dock.Avalonia** | 11.3.11.7 | Provides the docking panel system for the debugger — tabs can be dragged, split, floated, and rearranged (like Visual Studio's layout). | ❌ **No** — The debugger's entire layout system depends on this. Only docking library for Avalonia. |
| **Dock.Avalonia.Themes.Fluent** | 11.3.11.7 | Fluent theme styling for docking panels. | ❌ **No** — Required for Dock to look correct with Fluent theme. |
| **Dock.Model.Mvvm** | 11.3.11.7 | MVVM model layer for Dock. | ❌ **No** — Required for Dock ViewModel integration. |

### 1.4 Text Editor — KEEP (Essential Feature)

| Package | Version | What It Does | Replaceable? |
|---------|---------|-------------|--------------|
| **Avalonia.AvaloniaEdit** | 11.4.1 | Port of AvalonEdit (the WPF text editor from SharpDevelop). Provides the assembly code editor in the debugger with syntax highlighting, line numbers, breakpoint margins, folding. | ❌ **No** — Only full-featured code editor for Avalonia. The debugger's disassembly view, script editor, and memory viewer all use this. |

### 1.5 Logging — ✅ POTENTIALLY REPLACEABLE

| Package | Version | What It Does | Replaceable? |
|---------|---------|-------------|--------------|
| **Serilog** | 4.3.0 | Structured logging library. Used for application-level logging. | ✅ **Yes** — .NET 10 has `Microsoft.Extensions.Logging` built-in with structured logging, console, and file providers. Serilog is great but adds 3 packages we could eliminate. |
| **Serilog.Sinks.Console** | 6.1.1 | Writes log output to the console. | ✅ **Yes** — Replaced by `Microsoft.Extensions.Logging.Console`. |
| **Serilog.Sinks.File** | 7.0.0 | Writes log output to rolling log files. | ✅ **Yes** — Replaced by a simple file logging provider or `Microsoft.Extensions.Logging` + file provider. |

**Replacement Plan:** Replace all 3 Serilog packages with `Microsoft.Extensions.Logging` (built into .NET 10, zero additional packages needed). Would require updating logging call sites from `Log.Information(...)` to `ILogger.LogInformation(...)`.

### 1.6 Utility — EVALUATE

| Package | Version | What It Does | Replaceable? |
|---------|---------|-------------|--------------|
| **System.IO.Hashing** | 10.0.2 | Provides CRC32, XxHash, and other non-cryptographic hash functions. Used for ROM identification (CRC32 matching against game databases). | ⚠️ **Partially** — This IS a Microsoft package (part of .NET runtime extensions). In .NET 10 some of these are in the runtime directly. Check if `System.IO.Hashing` ships in-box with .NET 10. Currently still a NuGet package but it's Microsoft-maintained. **Low priority to remove.** |
| **ELFSharp** | 2.17.3 | Reads ELF (Executable and Linkable Format) binary files. Used for loading debug symbols from compiled ROM files (GBA, etc.) for the debugger's symbol viewer. | ⚠️ **Specialized** — No .NET built-in ELF parser. Would require writing our own or keeping this. It's small and does one job well. **Keep for now.** |
| **Dotnet.Bundle** | 0.9.13 | macOS .app bundle creation for dotnet publish. Sets CFBundleIdentifier, icon, etc. | ❌ **No** — Required for macOS deployment. Only used at build time. |
| **CommunityToolkit.Mvvm** | 8.4.0 (transitive) | Microsoft's MVVM toolkit — source generators for `ObservableObject`, `RelayCommand`, etc. Pulled in by Dock. | ❌ **No** — Transitive dependency of Dock. |

### 1.7 Testing & Benchmarking — KEEP (Dev Only)

| Package | Version | What It Does | Replaceable? |
|---------|---------|-------------|--------------|
| **xunit** | 2.9.3 | Unit testing framework. | ❌ **No** — Standard choice, well-integrated with VS. |
| **xunit.runner.visualstudio** | 3.1.5 | VS Test Explorer integration for xUnit. | ❌ **No** — Required for VS test discovery. |
| **Microsoft.NET.Test.Sdk** | 18.0.1 | Test platform SDK. | ❌ **No** — Required for dotnet test. |
| **coverlet.collector** | 6.0.4 | Code coverage collection. | ❌ **No** — Standard coverage tool. |
| **BenchmarkDotNet** | 0.15.8 | Micro-benchmarking framework. | ❌ **No** — Industry standard, no alternative. |

### 1.8 Build & Trimming — KEEP (Infrastructure)

| Package | Version | What It Does | Replaceable? |
|---------|---------|-------------|--------------|
| **Microsoft.NET.ILLink.Tasks** | 10.0.2 | IL trimming for AOT/size optimization. Auto-referenced. | ❌ **No** — Part of .NET SDK. |

---

## 2. Vendored C# Libraries (Checked into Source)

### 2.1 DataBox — EVALUATE

| Library | Location | What It Does | Replaceable? |
|---------|----------|-------------|--------------|
| **DataBox** | `UI/ThirdParty/DataBox/` | A lightweight DataGrid alternative for Avalonia. Provides `DataBoxTextColumn`, `DataBoxCheckBoxColumn`, `DataBoxTemplateColumn`. Used for debugger tables (memory, breakpoints, watches, trace log). | ⚠️ **Potentially** — Avalonia 11 now has a built-in `DataGrid` control. DataBox was created because Avalonia's DataGrid was buggy/incomplete in earlier versions. **Worth evaluating if Avalonia's DataGrid is now sufficient.** Migration would require updating all debugger table views. |

---

## 3. Vendored C++ Libraries (Checked into Source)

### 3.1 Compression & Archive

| Library | Location | What It Does | Replaceable? |
|---------|----------|-------------|--------------|
| **7-Zip (LZMA SDK)** | `SevenZip/` | Reads 7z archives. Used for loading ROMs from 7z files. Full C implementation of 7z decoder. | ❌ **No** — .NET's `System.IO.Compression` only handles ZIP/GZip/Brotli. No 7z support built-in. Essential for ROM loading. |
| **miniz** | `Utilities/miniz.cpp/.h` | Lightweight zlib/deflate implementation. Used for PNG compression, ZIP reading/writing, and save state compression. | ⚠️ **Partially** — Could use zlib via vcpkg, but miniz is single-file and well-tested. The overhead of switching is not worth it. |
| **libspng** | `Utilities/spng.c/.h` | Fast PNG encoder/decoder. Used for screenshot capture and save state thumbnails. | ⚠️ **Partially** — Could use stb_image or libpng, but spng is fast and purpose-built. Keep. |

### 3.2 Scripting

| Library | Location | What It Does | Replaceable? |
|---------|----------|-------------|--------------|
| **Lua 5.4** | `Lua/` | Full Lua scripting engine with socket extensions. Powers the debugger's Lua scripting system for custom game analysis, TAS scripting, and memory watching. | ❌ **No** — Core feature of the debugger. No built-in .NET/C++ scripting alternative without major work. |

### 3.3 Audio Processing

| Library | Location | What It Does | Replaceable? |
|---------|----------|-------------|--------------|
| **blip_buf** (blargg) | `Core/Shared/Audio/` | Band-limited audio resampling. Converts chip audio sample rates to output sample rate without aliasing. | ❌ **No** — Specifically designed for retro audio emulation. No generic replacement. |
| **emu2413** | `Core/Shared/Audio/` | YM2413 (OPLL) FM synthesis chip emulator. Used for SMS/MSX FM sound. | ❌ **No** — Hardware-specific emulation code. |
| **ymfm** | `Core/` | Yamaha FM sound chip emulation. Used for PCE CD-ROM audio. | ❌ **No** — Hardware-specific emulation code. |
| **kissfft** | `Utilities/kissfft.h` | Simple FFT library. Used for audio filtering (equalizer). | ⚠️ **Low priority** — Could use a different FFT library but kissfft is header-only and works. |
| **orfanidis_eq** | `Utilities/` (referenced) | Audio equalizer implementation. | ⚠️ **Low priority** — Specialized audio processing. |
| **nes_ntsc / snes_ntsc** (blargg) | `Utilities/NTSC/` | NTSC video signal simulation for NES/SNES. Produces the characteristic CRT "look." | ❌ **No** — Specialized retro video processing. |
| **stb_vorbis** | Linked/referenced | Ogg Vorbis audio decoder. Used for PCE CD audio tracks. | ❌ **No** — Specialized audio format decoder. |

### 3.4 Video Filters

| Library | Location | What It Does | Replaceable? |
|---------|----------|-------------|--------------|
| **HQX** | `Utilities/HQX/` | HQ2x/HQ3x/HQ4x pixel art upscaling filters. | ❌ **No** — Specialized retro upscaling algorithm. |
| **xBRZ** | `Utilities/xBRZ/` | xBRZ pixel art upscaling filter (higher quality than HQx). | ❌ **No** — Specialized retro upscaling algorithm. |
| **Scale2x** | `Utilities/Scale2x/` | Scale2x/Scale3x pixel art upscaling. | ❌ **No** — Specialized retro upscaling algorithm. |
| **KreedSaiEagle** | `Utilities/KreedSaiEagle/` | Super Eagle and 2xSaI upscaling filters. | ❌ **No** — Classic retro upscaling algorithms. |

### 3.5 Video Codecs

| Library | Location | What It Does | Replaceable? |
|---------|----------|-------------|--------------|
| **ZMBV Codec** (DOSBox) | In Core | Zip Motion Blocks Video. Used for AVI recording with lossless compression. | ⚠️ **Niche** — Could potentially use a modern codec but ZMBV is lightweight and purpose-built. |
| **CSCD Codec** (lsnes) | In Core | CamStudio Codec. Another lossless video codec option. | ⚠️ **Niche** — Same as above. |

### 3.6 Utility Libraries

| Library | Location | What It Does | Replaceable? |
|---------|----------|-------------|--------------|
| **magic_enum** | `Utilities/magic_enum.hpp` | C++ header-only library for enum reflection (name↔value conversion). | ⚠️ **Low priority** — Very convenient, header-only, zero overhead. C++23 has some reflection proposals but nothing shipped yet. Keep. |
| **md5** | `Utilities/md5.cpp/.h` | MD5 hash implementation. Used for ROM identification. | ✅ **Yes** — Could use OpenSSL or a C++ crypto library, but it's tiny and self-contained. Low priority. |
| **sha1** | `Utilities/sha1.cpp/.h` | SHA-1 hash implementation. Used for ROM identification. | ✅ **Yes** — Same as md5. Could consolidate with a crypto library. Low priority. |
| **CRC32** | `Utilities/CRC32.cpp/.h` | CRC32 checksum. Used everywhere for ROM identification. | ❌ **No** — Tight, optimized implementation used in hot paths. |

### 3.7 Platform & Graphics

| Library | Location | What It Does | Replaceable? |
|---------|----------|-------------|--------------|
| **SDL2** | `Sdl/` (wrappers) | Simple DirectMedia Layer. Used on Linux/macOS for window creation, input handling, and audio output. | ❌ **No** — Essential for non-Windows platforms. |
| **DirectXTK** | Referenced in Windows build | DirectX Toolkit for hardware-accelerated rendering on Windows. | ❌ **No** — Windows rendering backend. |
| **SkiaSharp** | Transitive (Avalonia) | 2D graphics rendering engine. Avalonia's rendering backend. | ❌ **No** — Transitive, required by Avalonia. |
| **HarfBuzz** | Transitive (Avalonia) | Text shaping engine. Used by Avalonia for text rendering. | ❌ **No** — Transitive, required by Avalonia. |

### 3.8 Networking

| Library | Location | What It Does | Replaceable? |
|---------|----------|-------------|--------------|
| **UPnP Port Mapper** | `Utilities/UPnPPortMapper.cpp` | UPnP port forwarding for netplay. | ⚠️ **Low priority** — Small, self-contained. |

---

## 4. Summary: What Can Be Replaced?

### ✅ Recommended Replacements (Create Issues)

| # | Current | Replacement | Effort | Packages Removed |
|---|---------|------------|--------|-----------------|
| 1 | **Serilog** (3 packages) | `Microsoft.Extensions.Logging` (built into .NET 10) | Medium | 3 NuGet packages |
| 2 | **DataBox** (vendored) | Avalonia built-in `DataGrid` | High | 1 vendored library |

### ⚠️ Evaluate Later

| # | Current | Notes |
|---|---------|-------|
| 3 | **ELFSharp** | Keep unless we find it's barely used |
| 4 | **md5/sha1** (C++) | Could consolidate to one crypto library |
| 5 | **System.IO.Hashing** | Check if it ships in-box with .NET 10 |

### ❌ Must Keep (No Replacement)

Everything else — Avalonia, ReactiveUI, Dock, AvaloniaEdit, all C++ emulation libraries, compression libraries, video filters, audio engines, Lua, SDL2.

---

## 5. Icon & Graphics Assets (Issue #169)

### Current State
There are **166 PNG/ICO image assets** in `UI/Assets/`. These were inherited from Mesen2 and include:

- System icons (NES, SNES, GB, GBA, SMS, PCE, WS)
- Debugger icons (breakpoints, step controls, memory views)
- UI chrome (arrows, buttons, close icons, toolbar icons)
- Message box icons (error, warning, info, question)

### Approval Gate for #169
**All icon replacements must be approved before being applied.**
A visual catalog showing old vs. proposed new icons will be prepared for review before any changes are committed.
