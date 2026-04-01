# DiztinGUIsh-Mesen2 Integration Guide

## Overview

This integration allows DiztinGUIsh to use Mesen2 as a debugging emulator, replacing bsnes+ for live ROM analysis. DiztinGUIsh can capture execution traces from Mesen2 in real-time as you play games, automatically updating the disassembly and code analysis.

## Prerequisites

- Mesen2 emulator (latest version with DiztinGUIsh integration)
- DiztinGUIsh with Mesen2 support
- Same ROM file loaded in both applications

## Basic Usage

### 1. Setup
1. Open DiztinGUIsh and load your ROM file
2. Open Mesen2 and load the same ROM file
3. In DiztinGUIsh, go to File → Import → Live Capture → "Mesen2 Live Streaming"

### 2. Start Live Capture
1. In Mesen2, open the debugger console (F2)
2. Type: `emu.startDiztinguishServer(9998)`
3. In DiztinGUIsh, connect to localhost:9998
4. Start playing the game in Mesen2

### 3. Real-time Analysis
- As you play, DiztinGUIsh will automatically update the disassembly
- Executed code paths will be marked and analyzed
- CDL (Code/Data Log) data will be captured in real-time
- Use keyboard shortcut Ctrl+F6 to quickly access live capture

## Network Configuration

### Local Usage
- Host: `localhost` or `127.0.0.1`
- Port: `9998` (default)

### Remote Debugging
- Configure Mesen2 host IP address in DiztinGUIsh connection dialog
- Ensure firewall allows connections on port 9998
- Network latency should be <10ms for optimal performance

## Troubleshooting

### Connection Issues
- Verify both applications are using the same ROM file
- Check that Mesen2 server is started before connecting from DiztinGUIsh
- Ensure port 9998 is not blocked by firewall

### Performance Issues
- Close unnecessary applications to free up CPU/memory
- Use local connection when possible
- Consider reducing Mesen2 speed if trace data is too fast

### Data Synchronization
- If disassembly appears incorrect, verify ROM files are identical
- Restart both applications if data becomes out of sync
- Check that both applications support the same ROM format

## Advanced Features

### Custom Port Configuration
```lua
-- Use different port in Mesen2
emu.startDiztinguishServer(9999)
```

### Scripted Analysis
- Use Mesen2's Lua scripting with DiztinGUIsh capture
- Automate specific game state analysis
- Create custom breakpoints that trigger DiztinGUIsh updates

## Benefits Over bsnes+

1. **Modern Emulator**: Mesen2 is actively maintained with better compatibility
2. **Performance**: Higher execution speed and more efficient trace capture  
3. **Accuracy**: More accurate SNES emulation for precise analysis
4. **Features**: Advanced debugging features in Mesen2
5. **Integration**: Purpose-built integration designed specifically for DiztinGUIsh

## Best Practices

1. Always load the same ROM in both applications before connecting
2. Start Mesen2 server before attempting connection from DiztinGUIsh
3. Use local connections when possible for best performance
4. Save your DiztinGUIsh project regularly during live capture sessions
5. Close capture connection when finished to free up resources