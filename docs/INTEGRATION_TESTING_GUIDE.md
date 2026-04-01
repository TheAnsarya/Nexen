# Integration Testing Guide

## Purpose

This guide covers testing the DiztinGUIsh-Mesen2 integration to ensure it works correctly across different scenarios and ROM types.

## Test Scenarios

### Basic Functionality Tests

1. **Connection Test**
   - Start Mesen2 with ROM loaded
   - Start DiztinGUIsh server in Mesen2: `emu.startDiztinguishServer(9998)`
   - Connect from DiztinGUIsh
   - Verify connection established successfully

2. **Trace Capture Test**
   - Load same ROM in both applications
   - Establish connection
   - Play game for 30 seconds
   - Verify traces are captured in DiztinGUIsh
   - Check that executed code is properly marked

3. **Disconnection Test**
   - Establish connection
   - Stop server in Mesen2
   - Verify DiztinGUIsh handles disconnection gracefully
   - Reconnect and verify functionality restored

### ROM Compatibility Tests

Test with various ROM types:
- LoROM games (Super Mario World, Zelda: A Link to the Past)
- HiROM games (Secret of Mana, Final Fantasy VI) 
- ExHiROM games (Tales of Phantasia)
- SuperFX games (Star Fox, Super Mario World 2)

### Performance Tests

1. **High Activity Games**
   - Load games with intense CPU usage (Contra III, Gradius III)
   - Monitor trace capture rate
   - Verify no dropped traces or connection issues

2. **Long Running Sessions**
   - Run capture session for 1+ hours
   - Monitor memory usage in both applications
   - Verify stable performance over time

### Error Handling Tests

1. **Network Issues**
   - Disconnect network during capture
   - Test with high latency connections
   - Verify proper error messages and recovery

2. **Invalid Data**
   - Load different ROMs in each application
   - Verify appropriate error handling
   - Test with corrupted ROM files

## Expected Results

- Connection should establish within 2 seconds
- Trace capture should handle 1M+ traces per second
- Memory usage should remain stable during long sessions
- Disconnections should be handled gracefully with clear error messages
- All SNES ROM types should work correctly

## Testing Tools

Use these Mesen2 Lua commands for testing:

```lua
-- Start server
emu.startDiztinguishServer(9998)

-- Check server status  
emu.getDiztinguishServerStatus()

-- Stop server
emu.stopDiztinguishServer()
```

## Bug Reporting

When reporting issues, include:

1. Mesen2 version number
2. DiztinGUIsh version number  
3. ROM file being used (if not copyrighted)
4. Exact steps to reproduce
5. Error messages or unexpected behavior
6. System specifications (OS, CPU, RAM)