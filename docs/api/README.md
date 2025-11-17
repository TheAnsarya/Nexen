# Mesen2 API Documentation

**Status:** In Progress  
**Last Updated:** 2025  

---

## Overview

This directory contains API documentation for Mesen2's core components and interop layer.

---

## Documentation Structure

### Core APIs (C++)

- [Core API Reference](./core/README.md) - Native emulation core APIs
- [Debugger API](./debugger/README.md) - Debugging interface
- [SNES Core API](./snes/README.md) - SNES-specific APIs
- [NES Core API](./nes/README.md) - NES-specific APIs
- [Other Systems](./systems/README.md) - GB, GBA, PCE, SMS, WS APIs

### Interop APIs (C#)

- [Interop Overview](./interop/README.md) - C++/C# interop layer
- [Debug API](./interop/DebugApi.md) - `DebugApi.cs`
- [Emulation API](./interop/EmuApi.md) - `EmuApi.cs`
- [Config API](./interop/ConfigApi.md) - `ConfigApi.cs`
- [Input API](./interop/InputApi.md) - `InputApi.cs`
- [Record API](./interop/RecordApi.md) - `RecordApi.cs`
- [Netplay API](./interop/NetplayApi.md) - `NetplayApi.cs`
- [History API](./interop/HistoryApi.md) - `HistoryApi.cs`
- [Test API](./interop/TestApi.md) - `TestApi.cs`

### UI APIs

- [UI Components](./ui/README.md) - Avalonia UI components
- [ViewModels](./ui/viewmodels.md) - MVVM view models
- [Debugger UI](./ui/debugger.md) - Debugger UI components

---

## Quick Reference

### Common Tasks

#### Loading a ROM
```csharp
// C# example
EmuApi.LoadRom("path/to/rom.sfc", "");
```

#### Setting a Breakpoint
```csharp
// C# example
var breakpoint = new InteropBreakpoint {
    Type = BreakpointType.Execute,
    Address = 0x8000,
    Enabled = true
};
DebugApi.AddBreakpoint(breakpoint);
```

#### Reading Memory
```csharp
// C# example
byte[] data = DebugApi.GetMemoryValues(
    MemoryType.SnesMemory, 
    startAddress, 
    length
);
```

---

## API Categories

### Emulation Control
- Load/unload ROMs
- Start/stop/pause/reset emulation
- Save states and rewind
- Speed control
- Frame advance

### Debugging
- Breakpoints (execution, read, write)
- Step execution (step in, step over, step out)
- Memory inspection and modification
- Register inspection and modification
- Trace logging
- Profiling
- Event tracking

### Configuration
- Video settings
- Audio settings
- Input mapping
- Emulation settings
- Per-game overrides

### Recording & Playback
- Movie recording
- Video recording
- Screenshot capture
- History tracking
- Netplay

### File I/O
- ROM loading
- Save data management
- State management
- Import/export (symbols, cheats, etc.)

---

## Data Structures

### Console State
Each system has a corresponding state structure:
- `NesState`
- `SnesState`
- `GbState`
- `GbaState`
- `PceState`
- `SmsState`
- `WsState`

### Debug Types
- `BreakpointType` - Execution, Read, Write, etc.
- `MemoryType` - Various memory regions per system
- `CpuType` - CPU type identifiers
- `ConsoleType` - System identifiers

---

## Integration Points

### For DiztinGUIsh Integration

**CDL Export:**
```csharp
// Proposed API
byte[] cdlData = DebugApi.GetCodeDataLog(ConsoleType.Snes);
File.WriteAllBytes("output.cdl", cdlData);
```

**Label Export:**
```csharp
// Existing API (to be documented)
// See: UI/Debugger/Labels/LabelManager.cs
LabelManager.Export("labels.mlb");
```

**Trace Logging:**
```csharp
// Existing API (to be enhanced)
// See: UI/Debugger/ViewModels/TraceLoggerViewModel.cs
TraceLogger.StartLogging("trace.txt");
```

---

## Extension Points

### Adding New Import Formats
Implement `ISymbolProvider` interface:
```csharp
public interface ISymbolProvider
{
    List<CodeLabel> Import(string path);
}
```

Examples:
- `DbgImporter` - .dbg files
- `ElfImporter` - ELF files
- `RgbdsSymbolFile` - RGBDS symbols
- (Add custom importers here)

### Custom Debugger Views
Extend debugger dock system:
```csharp
// See: UI/Debugger/DebuggerDockFactory.cs
// Create custom ITool implementations
```

---

## Platform-Specific Notes

### Windows
- Uses native Win32 APIs for some features
- DirectX support for rendering

### Linux
- SDL2 required for input/audio/video
- Some features may differ

### macOS
- SDL2 required
- App bundle packaging considerations

---

## Versioning

**Current API Version:** 2.1.1

API stability:
- **Core emulation:** Stable, changes rare
- **Interop layer:** Relatively stable
- **UI components:** May change with UI updates
- **Configuration:** Backward compatible where possible

---

## Examples

See `examples/` directory for:
- Loading and running ROMs
- Implementing custom importers
- Creating debugger tools
- Scripting with Lua
- (More examples to be added)

---

## Contributing

When adding new APIs:
1. Document in this structure
2. Include code examples
3. Note any platform-specific behavior
4. Update version history
5. Add unit tests

---

## Future Documentation

Planned documentation:
- [ ] Complete Core API reference
- [ ] Complete Interop API reference
- [ ] Comprehensive examples
- [ ] Lua scripting guide
- [ ] Plugin development guide
- [ ] Performance optimization guide

---

## Related Documentation

- [Main Project README](../README.md)
- [DiztinGUIsh Integration](../diztinguish/README.md)
- [Debugger Guide](../debugger/README.md)
- [Build Instructions](../../COMPILING.md)

