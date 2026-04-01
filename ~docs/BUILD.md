# Building Mesen2 with DiztinGUIsh Integration

Complete guide to building Mesen2 with the DiztinGUIsh streaming feature.

---

## Prerequisites

### Windows
- **Visual Studio 2022** (Community, Professional, or Enterprise)
  - C++ Desktop Development workload
  - Windows 10 SDK
  - MSBuild tools
- **Git** (for cloning repository)
- **Optional:** Python 3.7+ (for testing)

### Linux
- **GCC 9+** or **Clang 10+**
- **Make**
- **SDL2 development libraries**
- **Git**
- **pkg-config**
- **Optional:** Python 3.7+ (for testing)

### macOS
- **Xcode** 12+ with Command Line Tools
- **Homebrew** (recommended)
- **SDL2** (via Homebrew)
- **Git**
- **Optional:** Python 3.7+ (for testing)

---

## Quick Build

### Windows (Visual Studio)

```powershell
# Clone repository
git clone https://github.com/TheAnsarya/Mesen2.git
cd Mesen2

# Build Release configuration
msbuild Mesen.sln /p:Configuration=Release /p:Platform=x64

# Output: bin\win-x64\Mesen.exe
```

**Build time:** ~5 minutes (first build), ~30 seconds (incremental)

---

### Windows (Command Line - Faster)

```powershell
# Use Developer Command Prompt for VS 2022
# Or run: "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"

cd Mesen2
msbuild Mesen.sln /p:Configuration=Release /p:Platform=x64 /m
```

The `/m` flag enables multi-core build (much faster).

---

### Linux (Makefile)

```bash
# Clone repository
git clone https://github.com/TheAnsarya/Mesen2.git
cd Mesen2

# Install dependencies (Ubuntu/Debian)
sudo apt-get install build-essential libsdl2-dev pkg-config

# Build
make -j$(nproc)

# Output: bin/linux-x64/Mesen
```

**Build time:** ~3 minutes (first build), ~20 seconds (incremental)

---

### macOS (Makefile)

```bash
# Clone repository
git clone https://github.com/TheAnsarya/Mesen2.git
cd Mesen2

# Install dependencies via Homebrew
brew install sdl2

# Build
make -j$(sysctl -n hw.ncpu)

# Output: bin/osx-x64/Mesen
```

---

## Build Configurations

### Debug Build
**Purpose:** Development, debugging with breakpoints  
**Speed:** Slower emulation, larger binary  
**Symbols:** Full debug information included

```powershell
# Windows
msbuild Mesen.sln /p:Configuration=Debug /p:Platform=x64

# Linux/macOS
make clean
make debug
```

---

### Release Build  
**Purpose:** Normal usage, testing, distribution  
**Speed:** Full speed emulation, optimized  
**Symbols:** Minimal debug information

```powershell
# Windows
msbuild Mesen.sln /p:Configuration=Release /p:Platform=x64

# Linux/macOS
make clean
make
```

---

### Profile-Guided Optimization (PGO)
**Purpose:** Maximum performance  
**Speed:** Fastest possible emulation  
**Build time:** Much longer (requires profiling run)

**Windows:**
```powershell
# Step 1: Build with instrumentation
msbuild Mesen.sln /p:Configuration=Release /p:Platform=x64 /p:WholeProgramOptimization=PGInstrument

# Step 2: Run emulator with typical workload (trains optimizer)
bin\win-x64\Mesen.exe
# Load ROM, play for a few minutes, close

# Step 3: Build with PGO optimization
msbuild Mesen.sln /p:Configuration=Release /p:Platform=x64 /p:WholeProgramOptimization=PGOptimize
```

**Linux:**
```bash
./buildPGO.sh
```

**Performance gain:** 10-20% faster emulation

---

## Verifying DiztinGUIsh Integration

### Check Files Exist

**After building, verify these files compiled:**

**Windows:**
```powershell
# Check DiztinGUIsh bridge is linked
dumpbin /SYMBOLS bin\win-x64\MesenCore.dll | findstr "DiztinguishBridge"

# Should show symbols like:
# DiztinguishBridge::StartServer
# DiztinguishBridge::OnCpuExec
# etc.
```

**Linux/macOS:**
```bash
# Check DiztinGUIsh symbols
nm bin/linux-x64/libMesenCore.so | grep Diztinguish

# Should show symbols like:
# DiztinguishBridge::StartServer
# DiztinguishBridge::OnCpuExec
# etc.
```

---

### Test Lua API

**Launch Mesen2:**
```bash
# Windows
bin\win-x64\Mesen.exe

# Linux
./bin/linux-x64/Mesen

# macOS
./bin/osx-x64/Mesen
```

**In Mesen2:**
1. Load any SNES ROM
2. Open Debugger (F7 or Debug → Debugger)
3. Open Script Window (Debug → Script Window)
4. In Lua console, type:
   ```lua
   emu.startDiztinguishServer
   ```
5. Press Tab (autocomplete should show the function)
6. If it appears, **integration successful!** ✅

---

## Common Build Errors

### Error: MSB8020 (Windows)

**Error:**
```
error MSB8020: The build tools for Visual Studio 2019 (Platform Toolset = 'v142') 
cannot be found.
```

**Cause:** Project configured for wrong Visual Studio version

**Solution:**
```powershell
# Retarget solution for VS 2022
devenv Mesen.sln /upgrade
```

Or in Visual Studio:
- Right-click solution
- Retarget Solution
- Select Windows SDK 10.0 and v143 platform toolset

---

### Error: SDL2 not found (Linux)

**Error:**
```
fatal error: SDL2/SDL.h: No such file or directory
```

**Solution:**
```bash
# Ubuntu/Debian
sudo apt-get install libsdl2-dev

# Fedora/RHEL
sudo dnf install SDL2-devel

# Arch Linux
sudo pacman -S sdl2
```

---

### Error: C++ version mismatch

**Error:**
```
error: 'std::filesystem' has not been declared
```

**Cause:** Compiler doesn't support C++17

**Solution:**
```bash
# Update compiler
# Ubuntu 18.04 (needs newer GCC)
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo apt-get install gcc-9 g++-9

# Set as default
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 90
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-9 90
```

---

### Error: LNK2001 unresolved external symbol

**Error (Windows):**
```
error LNK2001: unresolved external symbol "public: bool __cdecl 
DiztinguishBridge::StartServer(unsigned short)"
```

**Cause:** DiztinguishBridge.cpp not compiled or linked

**Solution:**
1. Check Core.vcxproj includes DiztinguishBridge.cpp
2. Clean and rebuild:
   ```powershell
   msbuild Mesen.sln /t:Clean
   msbuild Mesen.sln /p:Configuration=Release /p:Platform=x64
   ```

---

### Error: 'DiztinguishBridge' is not a member of 'SnesDebugger'

**Cause:** DiztinguishBridge.h not included

**Solution:**
Check SnesDebugger.h has:
```cpp
#include "Debugger/DiztinguishBridge.h"
```

And check DiztinguishBridge.h forward declares:
```cpp
class SnesDebugger;
```

---

## Build Performance Tips

### Windows - Multi-core Build

**Use all CPU cores:**
```powershell
msbuild Mesen.sln /p:Configuration=Release /p:Platform=x64 /m
```

**Specific core count:**
```powershell
msbuild Mesen.sln /p:Configuration=Release /p:Platform=x64 /m:8
```

---

### Linux - Parallel Make

**Use all cores:**
```bash
make -j$(nproc)
```

**Specific core count:**
```bash
make -j8
```

---

### Clean Build

**Windows:**
```powershell
msbuild Mesen.sln /t:Clean
msbuild Mesen.sln /p:Configuration=Release /p:Platform=x64
```

**Linux/macOS:**
```bash
make clean
make
```

---

## Incremental Builds

After modifying DiztinGUIsh code, rebuild only changed files:

**Modified DiztinguishBridge.cpp only:**
```powershell
# Windows - rebuild just Core project
msbuild Core\Core.vcxproj /p:Configuration=Release /p:Platform=x64

# Linux/macOS - make detects changes automatically
make
```

**Build time:** ~5-10 seconds (vs ~5 minutes for full rebuild)

---

## Build Output

### Binary Locations

**Windows:**
- Executable: `bin\win-x64\Mesen.exe`
- Core DLL: `bin\win-x64\MesenCore.dll`
- Interop DLL: `bin\win-x64\InteropDLL.dll`

**Linux:**
- Executable: `bin/linux-x64/Mesen`
- Core library: `bin/linux-x64/libMesenCore.so`

**macOS:**
- Executable: `bin/osx-x64/Mesen`
- Core library: `bin/osx-x64/libMesenCore.dylib`

---

### File Sizes

**Typical sizes (Release build):**
- Mesen.exe: ~15 MB
- MesenCore.dll: ~8 MB
- libMesenCore.so: ~6 MB (Linux, stripped)

**Debug build:** 2-3x larger (includes symbols)

---

## Testing the Build

### Basic Functionality Test

1. Launch Mesen2
2. Load a SNES ROM
3. Press F5 to run
4. Verify emulation works at 60 FPS

**Expected:** Game runs smoothly

---

### DiztinGUIsh Integration Test

1. Load SNES ROM
2. Open Debugger (F7)
3. Load `~docs/test_server.lua` script
4. Script should print:
   ```
   Starting DiztinGUIsh server on port 9998...
   ✅ Server started successfully!
   ```

5. In another terminal:
   ```bash
   cd ~docs
   python test_mesen_stream.py
   ```

6. Python client should print:
   ```
   ✅ Connected successfully
   📡 HANDSHAKE: Protocol v1, Emulator: Mesen2 v0.7.0
   ```

**If all steps succeed:** Build is correct! ✅

---

## Packaging for Distribution

### Windows Portable Build

```powershell
# Create portable package
mkdir Mesen2-Portable
copy bin\win-x64\*.exe Mesen2-Portable\
copy bin\win-x64\*.dll Mesen2-Portable\
copy LICENSE Mesen2-Portable\
copy README.md Mesen2-Portable\

# Zip it
Compress-Archive -Path Mesen2-Portable -DestinationPath Mesen2-Portable.zip
```

---

### Linux AppImage

```bash
# Use provided AppImage build script
cd Linux/appimage
./build.sh

# Output: Mesen2-x86_64.AppImage
```

---

## Continuous Integration

### GitHub Actions (Example)

```yaml
name: Build Mesen2

on: [push, pull_request]

jobs:
  build-windows:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v3
      - uses: microsoft/setup-msbuild@v1
      - name: Build
        run: msbuild Mesen.sln /p:Configuration=Release /p:Platform=x64 /m
      - uses: actions/upload-artifact@v3
        with:
          name: mesen-windows
          path: bin/win-x64/

  build-linux:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Install dependencies
        run: sudo apt-get install libsdl2-dev
      - name: Build
        run: make -j$(nproc)
      - uses: actions/upload-artifact@v3
        with:
          name: mesen-linux
          path: bin/linux-x64/
```

---

## Development Workflow

### Recommended IDE Setup

**Windows:**
- Visual Studio 2022 (best integration)
- VS Code (lightweight alternative)

**Linux/macOS:**
- VS Code with C++ extension
- CLion (JetBrains)
- Vim/Emacs with LSP

---

### Quick Edit-Compile-Test Cycle

```bash
# 1. Edit DiztinguishBridge.cpp

# 2. Rebuild (5-10 seconds)
make

# 3. Test
./bin/linux-x64/Mesen
# Load test ROM, verify changes

# 4. If bug found, add debug logging:
#    Log("DiztinGUIsh: message sent");

# 5. Repeat steps 2-4 until working
```

**Cycle time:** <30 seconds per iteration

---

## Troubleshooting Build Issues

### Problem: Build succeeds but crashes on startup

**Check:**
1. Debug vs Release mismatch (dependencies)
2. Missing DLL files
3. Corrupted build cache

**Solution:**
```powershell
# Clean everything
msbuild Mesen.sln /t:Clean
rm -rf bin/
rm -rf obj/

# Rebuild from scratch
msbuild Mesen.sln /p:Configuration=Release /p:Platform=x64
```

---

### Problem: DiztinGUIsh functions not found in Lua

**Check:**
1. DiztinguishBridge linked into MesenCore
2. LuaApi.cpp has function registrations
3. Console type is SNES

**Debug:**
```lua
-- In Lua console
for k, v in pairs(emu) do print(k) end
-- Should show: startDiztinguishServer, stopDiztinguishServer, getDiztinguishServerStatus
```

---

### Problem: Slow build times

**Solutions:**
1. Use `/m` flag (MSBuild) or `-j` (Make)
2. Disable antivirus scanning on build folder
3. Use SSD for source code
4. Close unnecessary programs
5. Use incremental builds (don't clean unless needed)

**Typical times:**
- Clean build: 3-5 minutes
- Incremental: 5-30 seconds
- DiztinGUIsh-only change: 5-10 seconds

---

## Next Steps

After successful build:

1. **Test basic functionality:** Load ROM, verify emulation works
2. **Test DiztinGUIsh:** Run test_server.lua + Python client
3. **Verify streaming:** Check handshake, traces, CDL, frames
4. **Measure performance:** Should still run at 60 FPS
5. **Report issues:** Create GitHub issue if problems found

---

## Resources

- **Main Repository:** https://github.com/TheAnsarya/Mesen2
- **Build Issues:** https://github.com/TheAnsarya/Mesen2/issues
- **Quick Start:** ~docs/QUICK_START.md
- **Testing Guide:** ~docs/TESTING.md
- **Discord:** (Mesen community server)

---

**Happy Building!** 🔨

If you encounter build issues not covered here, please create a GitHub issue with:
- OS and compiler version
- Full error message
- Steps to reproduce
- Output of `git log -1` (current commit)
