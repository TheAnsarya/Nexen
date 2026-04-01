# Complete Manual Testing Guide: DiztinGUIsh-Mesen2 Integration

## Overview

This comprehensive guide provides step-by-step instructions for manually testing the complete DiztinGUIsh-Mesen2 integration, from building both projects through validating real-time disassembly generation during gameplay.

## Prerequisites

### Required Software
- Visual Studio 2022 (Community or higher)
- .NET 6.0 SDK or higher
- Git for Windows
- CMake 3.20 or higher
- vcpkg package manager

### Required Files
- SNES ROM file for testing (legally obtained)
- Both DiztinGUIsh and Mesen2 source code repositories

## Part 1: Building Mesen2

### Step 1: Clone and Setup Mesen2

1. Open Command Prompt or PowerShell as Administrator
2. Navigate to your development directory:
   ```
   cd C:\dev
   ```
3. Clone the Mesen2 repository:
   ```
   git clone https://github.com/SourMesen/Mesen2.git
   cd Mesen2
   ```

### Step 2: Install Dependencies

1. Install vcpkg if not already installed:
   ```
   git clone https://github.com/Microsoft/vcpkg.git
   cd vcpkg
   .\bootstrap-vcpkg.bat
   .\vcpkg integrate install
   cd ..
   ```

2. Set vcpkg environment variable:
   ```
   set VCPKG_ROOT=C:\dev\vcpkg
   ```

### Step 3: Build Mesen2

1. Open Visual Studio 2022
2. Open the solution file: `Mesen2\Mesen.sln`
3. Set build configuration to **Release** and platform to **x64**
4. Build the entire solution:
   - Right-click on Solution in Solution Explorer
   - Select "Build Solution" or press Ctrl+Shift+B
   - Wait for build completion (5-10 minutes)

### Step 4: Verify Mesen2 Build

1. Navigate to build output directory:
   ```
   cd bin\win-x64\Release
   ```
2. Verify these files exist:
   - `Mesen.exe` (main executable)
   - `Mesen.Core.dll` (core library)
   - `InteropDLL.dll` (interop layer)
   - Required SDL2 and other dependency DLLs

3. Test basic functionality:
   ```
   .\Mesen.exe
   ```
   - Mesen2 should start successfully
   - Close Mesen2 after verification

## Part 2: Building DiztinGUIsh

### Step 1: Clone and Setup DiztinGUIsh

1. Navigate back to development directory:
   ```
   cd C:\dev
   ```
2. Clone DiztinGUIsh repository:
   ```
   git clone https://github.com/DiztinGUIsh/DiztinGUIsh.git
   cd DiztinGUIsh
   ```

### Step 2: Build DiztinGUIsh

1. Open new Visual Studio 2022 instance
2. Open the solution file: `DiztinGUIsh\DiztinGUIsh.sln`
3. Set build configuration to **Release** and platform to **Any CPU**
4. Restore NuGet packages:
   - Right-click on Solution in Solution Explorer
   - Select "Restore NuGet Packages"
   - Wait for completion

5. Build the solution:
   - Right-click on Solution in Solution Explorer  
   - Select "Build Solution" or press Ctrl+Shift+B
   - Wait for build completion (2-5 minutes)

### Step 3: Verify DiztinGUIsh Build

1. Navigate to build output:
   ```
   cd Diz.App.Winforms\bin\Release\net6.0-windows
   ```
2. Verify these files exist:
   - `Diz.App.Winforms.exe` (main executable)
   - All required DiztinGUIsh DLL files
   - Dependencies and configuration files

3. Test basic functionality:
   ```
   .\Diz.App.Winforms.exe
   ```
   - DiztinGUIsh should start successfully
   - Close DiztinGUIsh after verification

## Part 3: Preparing Test Environment

### Step 1: Prepare Test ROM

1. Place your test ROM file in an easily accessible location:
   ```
   C:\TestRoms\TestGame.smc
   ```
   **Note:** Use a legally obtained ROM file. Good test ROMs include:
   - Super Mario World (common, well-documented)
   - Donkey Kong Country (good for testing complex code)
   - Any LoROM or HiROM game

### Step 2: Prepare Test Workspace

1. Create a dedicated testing folder:
   ```
   mkdir C:\DiztinGUIsh-Testing
   cd C:\DiztinGUIsh-Testing
   ```
2. This will be used for DiztinGUIsh project files

## Part 4: Manual Integration Testing

### Step 1: Launch Both Applications

1. **Start Mesen2:**
   ```
   cd C:\dev\Mesen2\bin\win-x64\Release
   .\Mesen.exe
   ```

2. **Start DiztinGUIsh:**
   ```
   cd C:\dev\DiztinGUIsh\Diz.App.Winforms\bin\Release\net6.0-windows
   .\Diz.App.Winforms.exe
   ```

### Step 2: Load ROM in Mesen2

1. In Mesen2:
   - File → Open
   - Navigate to `C:\TestRoms\TestGame.smc`
   - Select and open the ROM
   - Game should load and display on screen
   - **Do not start playing yet**

### Step 3: Create DiztinGUIsh Project

1. In DiztinGUIsh:
   - File → New Project
   - Browse and select the same ROM file: `C:\TestRoms\TestGame.smc`
   - Choose project save location: `C:\DiztinGUIsh-Testing\TestGame.diz`
   - Click "Create Project"
   - Project should load showing initial disassembly view

### Step 4: Setup DiztinGUIsh for Live Capture

1. In DiztinGUIsh main window:
   - Go to File → Import → Live Capture
   - Select "Mesen2 Live Streaming"
   - Connection dialog should appear

2. Configure connection settings:
   - Host: `localhost` (or `127.0.0.1`)
   - Port: `9998`
   - **Do not click Connect yet**

### Step 5: Start Mesen2 DiztinGUIsh Server

1. In Mesen2, open the debugger console:
   - Tools → Debugger (or press F11)
   - In debugger window, go to Tools → Script Window
   - Or press F2 to open console directly

2. In the Lua console, type:
   ```lua
   emu.startDiztinguishServer(9998)
   ```
   - Press Enter
   - Console should display: "DiztinGUIsh server started on port 9998"

3. Verify server status:
   ```lua
   emu.getDiztinguishServerStatus()
   ```
   - Should show server is running and port number

### Step 6: Establish Connection

1. Return to DiztinGUIsh connection dialog
2. Click "Connect"
3. **Expected results:**
   - Connection should establish within 1-2 seconds
   - Status should change to "Connected"
   - No error messages should appear

4. **If connection fails:**
   - Verify Mesen2 server is running (check console output)
   - Check Windows Firewall settings
   - Ensure both applications are using same ROM file
   - Try restarting both applications and repeating steps

### Step 7: Begin Live Trace Capture

1. In DiztinGUIsh live capture window:
   - Click "Start Capture" button
   - Status should show "Capturing..."
   - Statistics counters should be visible but showing zeros

2. In Mesen2:
   - Return to main emulation window
   - **Start playing the game**
   - Use keyboard or gamepad controls to move character/navigate menus

### Step 8: Verify Real-Time Disassembly Generation

1. **Immediate verification (within 5 seconds):**
   - DiztinGUIsh statistics should show increasing numbers:
     - Messages Received: rapidly increasing count
     - Traces Captured: should be > 1000 and growing
     - Bytes/sec: should show network activity
   - No error messages in either application

2. **Watch DiztinGUIsh disassembly view:**
   - Switch to main DiztinGUIsh window (keep capture dialog open)
   - Navigate through disassembly using scroll bars
   - **Look for green highlighting** on recently executed code
   - Executed addresses should be marked differently from unexecuted code

3. **Continue playing for 30 seconds:**
   - Perform various actions in game:
     - Move character around
     - Access menus
     - Trigger different game events
   - Return periodically to DiztinGUIsh to observe changes

### Step 9: Detailed Verification

1. **Check specific code sections:**
   - In DiztinGUIsh, navigate to reset vector area (around $8000-$8100)
   - This code should definitely be marked as executed
   - Look for NMI and IRQ handlers that may be highlighted

2. **Verify data vs code detection:**
   - Navigate through ROM using Page Up/Down in DiztinGUIsh
   - Executed code sections should be clearly differentiated
   - Data sections should remain unmarked

3. **Check statistics accuracy:**
   - Note current "Traces Captured" count in DiztinGUIsh
   - Play for exactly 10 more seconds
   - Count should increase by approximately 178,000 (SNES runs at ~1.79MHz)

### Step 10: Test Pause/Resume Functionality

1. **Pause game in Mesen2:**
   - Press spacebar or use emulation menu to pause
   - DiztinGUIsh trace capture should slow dramatically
   - Statistics should show much lower activity

2. **Resume game:**
   - Unpause in Mesen2
   - DiztinGUIsh activity should immediately resume normal levels

### Step 11: Test Disconnection and Reconnection

1. **Graceful disconnection:**
   - In DiztinGUIsh, click "Stop Capture"
   - Click "Disconnect"
   - Status should show "Disconnected"

2. **Stop server in Mesen2:**
   ```lua
   emu.stopDiztinguishServer()
   ```

3. **Test reconnection:**
   - Restart server: `emu.startDiztinguishServer(9998)`
   - In DiztinGUIsh, click "Connect" again
   - Should reconnect successfully
   - Resume capture and verify functionality

## Part 5: Advanced Testing Scenarios

### Test Different ROM Types

Repeat the above process with different ROM types:

1. **HiROM game** (e.g., Secret of Mana):
   - Verify address mapping works correctly
   - Check that bank boundaries are handled properly

2. **SuperFX game** (e.g., Star Fox):
   - Test with enhanced chip games
   - Verify complex memory mapping scenarios

### Test Performance Under Load

1. **High activity gameplay:**
   - Play intense action sequences
   - Monitor both applications for lag or freezing
   - Verify trace capture keeps up with game speed

2. **Extended sessions:**
   - Run capture for 10+ minutes continuously
   - Monitor memory usage in Task Manager
   - Check for memory leaks or performance degradation

### Test Error Conditions

1. **Network interruption:**
   - Disconnect network adapter during capture
   - Verify graceful error handling
   - Test reconnection after network restoration

2. **Resource exhaustion:**
   - Run other memory-intensive applications
   - Verify both applications handle low memory gracefully

## Part 6: Validation Checklist

### ✅ Basic Functionality
- [ ] Both applications build without errors
- [ ] Both applications launch successfully
- [ ] Same ROM loads in both applications
- [ ] Server starts in Mesen2 without errors
- [ ] DiztinGUIsh connects successfully
- [ ] Live capture begins immediately when game starts

### ✅ Real-Time Updates
- [ ] Statistics update in real-time during gameplay
- [ ] Disassembly highlights change as game progresses
- [ ] Executed code is clearly marked in DiztinGUIsh
- [ ] Performance remains smooth in both applications
- [ ] No crashes or freezes during extended play

### ✅ Data Accuracy
- [ ] Reset vector code is marked as executed
- [ ] Code vs data detection appears correct
- [ ] Trace counts align with expected SNES CPU frequency
- [ ] Different ROM types work correctly
- [ ] Complex memory mapping scenarios handled properly

### ✅ Robustness
- [ ] Pause/resume cycles work correctly
- [ ] Disconnect/reconnect functions properly
- [ ] Error conditions handled gracefully
- [ ] Extended sessions remain stable
- [ ] No memory leaks detected

## Part 7: Troubleshooting Common Issues

### Build Problems

**Mesen2 build fails:**
- Verify all dependencies installed correctly
- Check vcpkg integration
- Ensure Visual Studio has C++ workload installed
- Try cleaning and rebuilding solution

**DiztinGUIsh build fails:**
- Restore NuGet packages manually
- Verify .NET 6.0 SDK installed
- Check for missing dependencies
- Try building in Debug mode first

### Runtime Problems

**Connection fails:**
- Verify firewall settings
- Check port 9998 is not in use by other applications
- Ensure both applications use identical ROM files
- Try different port number

**Poor performance:**
- Close unnecessary applications
- Run in Release mode only
- Check system meets minimum requirements
- Monitor CPU and memory usage

**Data inconsistencies:**
- Verify ROM file integrity
- Check file sizes match exactly
- Try different ROM file
- Restart both applications fresh

### Data Validation

**Missing trace data:**
- Verify game is actually running (not paused)
- Check server status in Mesen2 console
- Confirm connection is active in DiztinGUIsh
- Try different game area to generate more activity

**Incorrect disassembly:**
- Verify ROM header information is correct
- Check for ROM format mismatches (LoROM vs HiROM)
- Confirm both applications detect same ROM type
- Try well-known ROM file for comparison

## Success Criteria

The integration test is considered successful when:

1. **Complete build success** for both applications
2. **Successful connection establishment** within 2 seconds
3. **Real-time trace capture** at expected rates (1M+ traces/sec)
4. **Accurate disassembly highlighting** that updates during gameplay
5. **Stable performance** during extended testing (10+ minutes)
6. **Proper error handling** for disconnect/reconnect scenarios
7. **Correct data interpretation** for different ROM types

## Documentation

After successful testing, document:

1. Build times and any issues encountered
2. Performance measurements (traces/sec, memory usage)
3. ROM compatibility results
4. Any bugs or unexpected behaviors discovered
5. Recommendations for optimal usage

This completes the comprehensive manual testing procedure for the DiztinGUIsh-Mesen2 integration.