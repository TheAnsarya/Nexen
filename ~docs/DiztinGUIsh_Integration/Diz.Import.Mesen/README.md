# Diz.Import.Mesen - DiztinGUIsh Integration for Mesen2

**Live SNES trace streaming from Mesen2 to DiztinGUIsh**

This library provides seamless integration between [Mesen2](https://github.com/TheAnsarya/Mesen2) SNES emulator and [DiztinGUIsh](https://github.com/IsoFrieze/DiztinGUIsh) disassembler, enabling **real-time trace data streaming** for live analysis and disassembly.

---

## 🎯 What This Does

- **Live CPU Tracing:** Stream every CPU instruction from Mesen2 to DiztinGUIsh in real-time
- **CDL Integration:** Automatically update Code/Data Logger information as the game executes  
- **Flag Tracking:** Capture SNES-specific M/X flags, Data Bank, Direct Page registers
- **Frame Synchronization:** Track frame boundaries for timing-aware analysis
- **Seamless UX:** Integrates with DiztinGUIsh's existing UI and workflow

---

## ⚡ Quick Start

### 1. **Prerequisites**

- ✅ **Mesen2 v0.7.0+** with DiztinGUIsh server support
- ✅ **DiztinGUIsh v2.5+** 
- ✅ **.NET 6.0** SDK
- ✅ **SNES ROM** loaded in both applications

### 2. **Start Mesen2 Server**

```lua
-- In Mesen2 Script Window
emu.startDiztinguishServer(9998)
-- Should show: "✅ Server started successfully!"
```

### 3. **Connect from DiztinGUIsh**

```csharp
// Add to your DiztinGUIsh project
using Diz.Import.Mesen;

// Create importer
var importer = new MesenTraceLogImporter(project.Data.GetSnesApi());

// Connect and start streaming
await importer.ConnectAsync("localhost", 9998);

// Data will now stream automatically!
```

### 4. **Watch Live Updates**

- ▶️ **Play the game** in Mesen2
- 📊 **See disassembly update** in DiztinGUIsh in real-time
- 🎯 **Code/data detection** happens automatically
- ⚡ **M/X flags tracked** for accurate 65816 disassembly

---

## 🏗️ Architecture

```
┌─────────────────┐    TCP/IP     ┌─────────────────────┐
│     Mesen2      │◄─────────────►│    DiztinGUIsh     │
│                 │   Port 9998   │                     │
│ ┌─────────────┐ │               │ ┌─────────────────┐ │
│ │   SNES      │ │               │ │ Diz.Import.     │ │  
│ │ Debugger    │ │               │ │ Mesen           │ │
│ │             │ │               │ │                 │ │
│ │ CPU Hooks   │ │               │ │ Live Importer   │ │
│ │ CDL Hooks   │ │               │ │                 │ │
│ │ Frame Hooks │ │               │ │ Protocol Parser │ │
│ └─────────────┘ │               │ └─────────────────┘ │
│                 │               │                     │
│ DiztinguishBridge│               │   SNES Data Model  │
│ (TCP Server)     │               │   (Real-time       │
│                 │               │    updates)         │
└─────────────────┘               └─────────────────────┘
```

### **Message Flow:**

1. **CPU Execution** → Mesen2 captures instruction + flags
2. **TCP Message** → Binary protocol sends trace data  
3. **Parse & Convert** → C# client parses to DiztinGUIsh format
4. **Update Model** → SNES data automatically updated
5. **UI Refresh** → Disassembly view shows live changes

---

## 📡 Protocol Messages

### **Core Message Types:**

| Type | Purpose | Frequency | Size |
|------|---------|-----------|------|
| `HANDSHAKE` | Server identification | Once | 21 bytes |
| `EXEC_TRACE` | CPU instruction executed | ~1.79M/sec | 14 bytes |
| `CDL_UPDATE` | Code/Data classification | Variable | 5 bytes |  
| `FRAME_START/END` | Frame boundaries | 60 FPS | 4 bytes |
| `CPU_STATE` | Register snapshot | On-demand | 17 bytes |

### **Example Trace Message:**
```csharp
ExecTraceMessage {
    PC = 0x00FFE4,           // Program Counter  
    Opcode = 0x78,           // SEI instruction
    MFlag = true,            // 8-bit accumulator
    XFlag = true,            // 8-bit index  
    DataBank = 0x00,         // Data Bank = $00
    DirectPage = 0x0000,     // Direct Page = $0000
    EffectiveAddr = 0x000000 // No operand
}
```

---

## 🔧 Usage Examples

### **Basic Live Import**

```csharp
using Diz.Import.Mesen;
using Microsoft.Extensions.Logging;

// Set up logging (optional)
var logger = LoggerFactory.Create(b => b.AddConsole())
    .CreateLogger<MesenTraceLogImporter>();

// Create importer for your SNES project
var importer = new MesenTraceLogImporter(
    snesData: project.Data.GetSnesApi(), 
    logger: logger);

// Configure import behavior
importer.Settings.AddTraceComments = true;  // Add trace annotations
importer.Settings.LogFrameMarkers = false;  // Don't spam frame logs

// Connect to Mesen2
if (await importer.ConnectAsync("localhost", 9998))
{
    Console.WriteLine("✅ Connected! Live streaming started.");
    Console.WriteLine($"📊 {importer.ConnectionStats}");
    
    // Let it run...
    await Task.Delay(TimeSpan.FromMinutes(5));
    
    // Finalize and get statistics
    importer.CopyTempGeneratedCommentsIntoMainSnesData();
    Console.WriteLine($"📈 Final stats: {importer.CurrentStats}");
}
```

### **Integration with ProjectController**

```csharp
// Extension method approach (fits DiztinGUIsh patterns)
using var importer = await projectController.StartMesenLiveImportAsync(
    host: "localhost",
    port: 9998,
    settings: new MesenImportSettings 
    {
        AddTraceComments = true,
        LogCpuStates = false
    });

// Import runs in background, updating your project automatically
// Call when done:
long bytesModified = importer.CurrentStats.NumRomBytesModified;
Console.WriteLine($"Modified {bytesModified} ROM bytes");
```

### **Event-Driven Processing**

```csharp
var client = new MesenLiveTraceClient();

// Handle specific events
client.HandshakeReceived += (sender, handshake) =>
    Console.WriteLine($"Connected: {handshake}");

client.ExecTraceReceived += (sender, trace) =>
    Console.WriteLine($"Executed: {trace}");

client.CdlUpdateReceived += (sender, cdl) =>
    Console.WriteLine($"CDL: {cdl}");

await client.ConnectAsync("localhost", 9998);
// Events fire automatically as data streams in
```

---

## 🔥 Demo Application

Test the connection without DiztinGUIsh:

```bash
# Build the demo
dotnet build MesenDemo.csproj

# Run with default settings (10 seconds)  
dotnet run -- --host localhost --port 9998 --duration 10

# Run verbose mode (shows all messages)
dotnet run -- --verbose --duration 0  # 0 = infinite

# Example output:
🎮 Mesen2 DiztinGUIsh Live Trace Demo
📡 Connecting to localhost:9998
✅ Connected! Protocol v1, Mesen2 v0.7.0, Port 9998, ROM 1048576 bytes
🔄 TRACE #1: PC:$00FFE4 Op:$78 M:8 X:8 DB:$00 D:$0000 EA:$000000
📝 CDL #1: $00FFE4: C (0x01)
⏱️  00:10 | 📊 Exec: 17,900 (1,790/sec) | 📝 CDL: 145 (14.5/sec) | 🖼️  Frames: 600 (60.0 FPS)
```

---

## ⚙️ Configuration

### **MesenImportSettings**

```csharp
var settings = new MesenImportSettings
{
    // Add "TRACE: PC:$xxxx..." comments to ROM bytes
    AddTraceComments = true,
    
    // Log CPU state changes (for debugging)
    LogCpuStates = false,
    
    // Log frame start/end markers (for debugging)  
    LogFrameMarkers = false,
    
    // Network timeout in milliseconds
    NetworkTimeoutMs = 5000
};
```

### **Client Configuration**

```csharp
var client = new MesenLiveTraceClient(logger);

// Timeouts
client.ConnectTimeoutMs = 3000;    // Connection timeout
client.ReceiveTimeoutMs = 5000;    // Read timeout

// Debugging
client.LogRawMessages = true;      // Log all message types
```

---

## 📊 Performance

### **Typical Rates (Super Metroid gameplay):**

- **CPU Instructions:** ~1,790,000/sec (SNES CPU frequency)
- **Network Messages:** ~1,790,000/sec execution traces
- **Bandwidth:** ~34 MB/sec (19 bytes per trace × 1.79M/sec)
- **CDL Updates:** ~500-2,000/sec (varies by game activity)
- **Frame Rate:** 60 FPS (frame start/end markers)

### **Optimization:**

- ✅ **Binary Protocol:** Compact message format
- ✅ **Batched Updates:** DiztinGUIsh updates in chunks  
- ✅ **Async I/O:** Non-blocking network operations
- ✅ **Object Pooling:** Reduces GC pressure (following BSNES pattern)
- ✅ **Selective Logging:** Debug logs only when needed

---

## 🧪 Testing

### **Unit Tests**

```bash
# Run protocol parsing tests  
dotnet test Diz.Import.Mesen.Tests

# Coverage report
dotnet test --collect:"XPlat Code Coverage"
```

### **Integration Tests**

1. **Start Mesen2** with test ROM
2. **Enable server:** `emu.startDiztinguishServer(9998)`  
3. **Run demo:** `dotnet run MesenDemo.csproj --duration 30`
4. **Verify output:** Should show traces, CDL updates, frame markers

### **Performance Tests**

```bash
# Stress test with high-activity ROM
dotnet run MesenDemo.csproj --duration 300 --verbose
# Should handle 1.79M messages/sec without dropped data
```

---

## 🚀 DiztinGUIsh Integration

### **Adding to DiztinGUIsh Solution**

1. **Copy project** to `DiztinGUIsh/Diz.Import.Mesen/`
2. **Add project reference** to `DiztinGUIsh.sln`
3. **Update main project** with reference:
   ```xml
   <ProjectReference Include="..\..\Diz.Import.Mesen\Diz.Import.Mesen.csproj" />
   ```

### **UI Integration Points**

```csharp
// Add to main menu
"File" → "Import" → "Mesen2 Live Stream..."

// Add toolbar button  
[Live Stream] button with connection indicator

// Add status panel
"Mesen2: Connected - 1,234,567 traces received (1,790/sec)"
```

### **Controller Integration**

```csharp
// In ProjectController.cs, add:
public async Task<long> ImportMesenTraceLive(string host, int port)
{
    var importer = new MesenTraceLogImporter(Project.Data.GetSnesApi());
    var connected = await importer.ConnectAsync(host, port);
    
    if (!connected) return 0;
    
    // UI integration: show progress, allow cancellation
    // Return when user stops or connection drops
    
    importer.CopyTempGeneratedCommentsIntoMainSnesData();
    
    var modified = importer.CurrentStats.NumRomBytesModified;
    if (modified > 0) MarkChanged();
    
    return modified;
}
```

---

## 🐛 Troubleshooting

### **Connection Issues**

**❌ "Failed to connect to localhost:9998"**

✅ **Check:**
- Mesen2 is running
- SNES ROM is loaded  
- Server is started: `emu.startDiztinguishServer(9998)`
- No firewall blocking port 9998

**❌ "Protocol version mismatch"**

✅ **Check:**
- Using compatible Mesen2 version (v0.7.0+)
- Update Diz.Import.Mesen library

### **Performance Issues**

**❌ "Messages dropped / lag"**

✅ **Solutions:**
- Increase `NetworkTimeoutMs`
- Check network bandwidth (may need ~34 MB/sec)
- Disable verbose logging (`LogRawMessages = false`)
- Close unnecessary applications

**❌ "High memory usage"**

✅ **Solutions:**
- Call `CopyTempGeneratedCommentsIntoMainSnesData()` periodically  
- Disable trace comments (`AddTraceComments = false`)
- Limit connection duration

### **Data Quality Issues**

**❌ "Incorrect disassembly"**

✅ **Check:**
- Same ROM loaded in both applications
- ROM CRC32 matches (shown in handshake)
- M/X flags are being tracked correctly
- Not running from RAM addresses

---

## 🤝 Contributing

### **Code Style**
- Follow DiztinGUIsh conventions
- Use nullable reference types
- Add XML documentation
- Include unit tests

### **Protocol Changes**
- Update both Mesen2 (C++) and this library (C#)
- Maintain backward compatibility
- Document message format changes

### **Performance**
- Profile before optimizing
- Maintain thread safety
- Follow existing object pooling patterns

---

## 📜 License

**GPL-3.0** (same as DiztinGUIsh)

---

## 🙏 Credits

- **Mesen2:** [TheAnsarya](https://github.com/TheAnsarya) - SNES emulator with DiztinGUIsh integration
- **DiztinGUIsh:** [IsoFrieze](https://github.com/IsoFrieze) - SNES disassembler and analysis tool
- **Protocol Design:** Inspired by BSNES live tracing architecture

---

## 🔗 Links

- 📁 **Mesen2 Repository:** https://github.com/TheAnsarya/Mesen2
- 📁 **DiztinGUIsh Repository:** https://github.com/IsoFrieze/DiztinGUIsh  
- 📖 **Protocol Documentation:** [DiztinguishProtocol.h](../../Core/Debugger/DiztinguishProtocol.h)
- 🚀 **Quick Start Guide:** [QUICK_START.md](../../QUICK_START.md)

---

**Happy Disassembling!** 🎮✨