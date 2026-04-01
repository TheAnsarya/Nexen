# DiztinGUIsh Repository Analysis

**Date:** November 17, 2025  
**Repository:** https://github.com/IsoFrieze/DiztinGUIsh  
**Purpose:** Understand DiztinGUIsh architecture for Mesen2 integration

---

## Key Findings

### 1. **Real-Time Tracelog Capturing (EXISTING FEATURE!)**

**CRITICAL DISCOVERY:** DiztinGUIsh **already** has socket-based tracelog capturing!

From README.md:
> **Realtime tracelog capturing**: We provide a tight integration with a custom BSNES build to capture CPU tracelog data over a socket connection. You don't have to play the game at 2FPS anymore, or deal with wrangling gigabyte-sized tracelog files. Simply hit 'capture' and Diz will talk directly to a running BSNES CPU, capturing data for as long as you like. Turn the ROM visualizer on and watch this process in realtime.

**Implications:**
- ✅ DiztinGUIsh **already supports socket-based trace capturing**
- ✅ Existing reference implementation with custom BSNES
- ✅ Proven workflow for real-time data capture
- ✅ Our Mesen2 integration follows the **same pattern**

### 2. Project Structure

**Technology Stack:**
- **Language:** C# .NET
- **UI Framework:** Windows Forms (primary), Eto.Forms (cross-platform work in progress)
- **Architecture:** Modular with dependency injection (LightInject)
- **Testing:** xUnit

**Key Projects:**

| Project | Purpose |
|---------|---------|
| `Diz.Core` | Core data model and serialization |
| `Diz.Cpu.65816` | SNES 65816 CPU-specific code |
| `Diz.Controllers` | Business logic and controllers |
| `Diz.Ui.Winforms` | Windows Forms UI |
| `Diz.Ui.Eto` | Cross-platform UI (WIP) |
| `Diz.Import` | Import functionality (BizHawk CDL, BSNES tracelog, etc.) |
| `Diz.LogWriter` | Assembly export functionality |
| `Diz.App.Winforms` | Windows Forms application entry point |
| `Diz.App.Eto` | Cross-platform app (WIP) |

### 3. Core Interfaces

**IData Interface (Main Data Model):**
```csharp
public interface IData : 
    INotifyPropertyChanged,
    IReadOnlyByteSource,
    IRomMapProvider,
    IRomBytesProvider,
    IMarkable,
    IArchitectureSettable,
    IArchitectureGettable,
    ICommentTextProvider,
    IRegionProvider
{
    IDataStoreProvider<IArchitectureApi> Apis { get; }
    IDataStoreProvider<IDataTag> Tags { get; }
    ILabelServiceWithTempLabels Labels { get; }
    SortedDictionary<int, string> Comments { get; }
}
```

**ISnesRomByte Interface (Per-Byte Data):**
```csharp
public interface ISnesRomByte : INotifyPropertyChanged
{
    byte DataBank { get; set; }
    int DirectPage { get; set;}
    bool XFlag { get; set;}
    bool MFlag { get; set;}
    FlagType TypeFlag { get; set; }  // Code/Data/Graphics/etc.
    Architecture Arch { get; set; }
    InOutPoint Point { get; set; }
}
```

**Key Observations:**
- ✅ DiztinGUIsh already tracks **M/X flags** per byte!
- ✅ Has **DataBank** and **DirectPage** tracking
- ✅ Has **FlagType** for Code/Data/Graphics classification (CDL equivalent)
- ✅ Perfect match for our streaming message format!

### 4. Import Infrastructure

**Existing Import Controllers:**

```csharp
public interface ITraceLogImporters
{
    void ImportBizHawkCdl(string filename);
    long ImportBsnesUsageMap(string fileName);
    long ImportBsnesTraceLogs(string[] fileNames);
}
```

**Implementation Files:**
- `Diz.Import/bizhawk/` - BizHawk CDL importer
- `Diz.Import/bsnes/tracelog/` - BSNES tracelog importer
- `Diz.Import/bsnes/usagemap/` - BSNES usage map importer

**Pattern for Mesen2 Integration:**
- Create `Diz.Import/mesen/` directory
- Implement `IMesenLiveTraceImporter` interface
- Add to `ITraceLogImporters` interface

### 5. Project Controller

**IProjectController Interface:**
```csharp
public interface IProjectController : 
    ITraceLogImporters,
    IFixInstructionUtils,
    IDataUtilities
{
    Project Project { get; }
    
    event ProjectChangedEvent ProjectChanged;
    
    IProjectView ProjectView { get; set; }
    
    bool OpenProject(string filename);
    string SaveProject(string filename);
    bool ImportRomAndCreateNewProject(string romFilename);
    void ImportLabelsCsv(ILabelEditorView labelEditor, bool replaceAll);
    void SelectOffset(int offset, ISnesNavigation.HistoryArgs historyArgs = null);
    bool ConfirmSettingsThenExportAssembly();
    bool ExportAssemblyWithCurrentSettings();
    void MarkChanged();
}
```

**Integration Point:** Add live tracing UI controls to project controller.

### 6. Data Model

**Project Class:**
```csharp
public class Project
{
    public IData Data { get; set; }
    public ProjectSession Session { get; set; }
    public string AttachedRomFilename { get; set; }
    // ... other properties
}
```

**Data Flow:**
```
Mesen2 Socket Server → DiztinGUIsh Client → ITraceLogImporters
                                            ↓
                                        Update IData
                                            ↓
                                  Notify UI (INotifyPropertyChanged)
```

### 7. Dependency Injection

**Service Registration Pattern:**
```csharp
[UsedImplicitly] 
public class DizImportCompositionRoot : ICompositionRoot
{
    public void Compose(IServiceRegistry serviceRegistry)
    {
        serviceRegistry.Register<IBizHawkCdlImporter, BizHawkCdlImporter>();
        serviceRegistry.Register<IBsnesTracelogImporter, BsnesTracelogImporter>();
        // Add: serviceRegistry.Register<IMesenLiveTraceImporter, MesenLiveTraceImporter>();
    }
}
```

**LightInject Framework:** DiztinGUIsh uses LightInject for dependency injection.

---

## Integration Architecture (Updated)

### Client-Side Implementation (DiztinGUIsh)

**New Classes to Create:**

1. **`Diz.Import.Mesen/MesenLiveTraceClient.cs`**
   ```csharp
   public class MesenLiveTraceClient : IMesenLiveTraceClient
   {
       private TcpClient _client;
       private NetworkStream _stream;
       private IData _data;
       
       public void Connect(string host, int port) { /* ... */ }
       public void Disconnect() { /* ... */ }
       public void StartCapture() { /* ... */ }
       public void StopCapture() { /* ... */ }
       
       private void ProcessExecTraceMessage(ExecTraceMessage msg) 
       {
           var offset = /* calculate from msg.pc */;
           var romByte = _data.GetSnesRomByte(offset);
           romByte.MFlag = msg.m_flag;
           romByte.XFlag = msg.x_flag;
           romByte.DataBank = msg.db_register;
           romByte.DirectPage = msg.dp_register;
           romByte.TypeFlag = FlagType.Code;
       }
   }
   ```

2. **`Diz.Import.Mesen/MesenProtocol.cs`**
   ```csharp
   public enum MesenMessageType : byte
   {
       Handshake = 0x01,
       HandshakeAck = 0x02,
       ConfigStream = 0x03,
       ExecTrace = 0x10,
       MemoryAccess = 0x11,
       CpuState = 0x12,
       CdlUpdate = 0x13,
       // ... etc
   }
   
   [StructLayout(LayoutKind.Sequential, Pack = 1)]
   public struct ExecTraceMessage
   {
       public uint Pc;
       public byte Opcode;
       public byte MFlag;
       public byte XFlag;
       public byte DbRegister;
       public ushort DpRegister;
       public uint EffectiveAddr;
   }
   ```

3. **UI: Add "Connect to Mesen2" Menu Item**
   - File → Import → Connect to Mesen2...
   - Dialog: Host, Port, Connect/Disconnect buttons
   - Status indicator (Connected/Disconnected)
   - Real-time trace capture toggle

---

## Compatibility Analysis

### Socket Communication

**DiztinGUIsh Side (Client):**
```csharp
using System.Net.Sockets;

var client = new TcpClient();
client.Connect("localhost", 9998);
var stream = client.GetStream();

// Read messages
var header = new byte[5];
stream.Read(header, 0, 5);
var msgType = header[0];
var length = BitConverter.ToInt32(header, 1);
var payload = new byte[length];
stream.Read(payload, 0, length);
```

**Mesen2 Side (Server):**
```cpp
// Already exists in Utilities/Socket.h
Socket server;
server.Bind(9998);
server.Listen(1);
auto client = server.Accept();

// Send messages
uint8_t header[5];
header[0] = MessageType::ExecTrace;
*(uint32_t*)(header + 1) = sizeof(ExecTraceMessage);
client->Send(header, 5);
client->Send(&traceMsg, sizeof(ExecTraceMessage));
```

**Compatibility:** ✅ Perfect - Both use standard TCP sockets

### Data Structure Mapping

| Mesen2 (C++) | DiztinGUIsh (C#) | Match |
|--------------|------------------|-------|
| `uint32_t pc` | `uint Pc` | ✅ 4 bytes |
| `uint8_t m_flag` | `bool MFlag` | ✅ 1 byte |
| `uint8_t x_flag` | `bool XFlag` | ✅ 1 byte |
| `uint8_t db_register` | `byte DataBank` | ✅ 1 byte |
| `uint16_t dp_register` | `int DirectPage` | ✅ 2 bytes (cast to int) |

**Compatibility:** ✅ Excellent - Direct mapping possible

---

## Implementation Plan (Revised)

### Phase 1: DiztinGUIsh Client (2 weeks)

**Week 1:**
- Create `Diz.Import.Mesen` project
- Implement `MesenProtocol.cs` (message definitions)
- Implement `MesenLiveTraceClient.cs` (TCP client)
- Add dependency injection registration

**Week 2:**
- Create UI for "Connect to Mesen2"
- Implement connection dialog
- Add status indicator
- Test with mock Mesen2 server

### Phase 2: Mesen2 Server (Already Planned)

Follow existing streaming-integration.md specification.

### Phase 3: Integration Testing (1 week)

- Test handshake protocol
- Test execution trace streaming
- Verify M/X flag accuracy
- Measure performance

### Phase 4: Polish (1 week)

- Error handling
- Reconnection logic
- UI improvements
- Documentation

---

## Next Steps for Mesen2 Integration

### 1. Review BSNES Integration

**Action:** Study how DiztinGUIsh integrates with BSNES
- Look for existing socket client code
- Understand message protocol used
- Adapt pattern for Mesen2

**Files to Examine:**
- `Diz.Import/bsnes/tracelog/BsnesTracelogImporter.cs`
- Any socket/network code in BSNES integration

### 2. Create Mesen2-Specific Importer

**New Namespace:** `Diz.Import.Mesen`

**Files to Create:**
- `MesenLiveTraceClient.cs` - TCP client
- `MesenProtocol.cs` - Message protocol
- `MesenLiveTraceImporter.cs` - Integration with IData
- `MesenConnectionDialog.cs` - UI for connection
- `ServiceRegistration.cs` - Dependency injection

### 3. Update Mesen2 Streaming Spec

**Changes Based on DiztinGUIsh Analysis:**
- ✅ Confirm message format matches ISnesRomByte
- ✅ Ensure all required fields are included
- ✅ Add any missing fields (Point, TypeFlag)
- ✅ Document bidirectional label sync format

---

## Compatibility Checklist

### Data Model

- [x] M/X flags tracked per byte ✅
- [x] DataBank tracked ✅
- [x] DirectPage tracked ✅
- [x] Code/Data type flags ✅
- [x] Label system ✅
- [x] Comment system ✅

### Import System

- [x] Existing tracelog import infrastructure ✅
- [x] Modular importer design ✅
- [x] INotifyPropertyChanged for UI updates ✅
- [x] Dependency injection ready ✅

### Technical

- [x] C# .NET compatible ✅
- [x] TCP socket support ✅
- [x] Binary data handling ✅
- [x] Cross-platform (Eto.Forms in progress) ✅

---

## Risks and Mitigations

### Risk 1: BSNES Integration Conflicts

**Risk:** Existing BSNES socket integration might conflict with Mesen2
**Mitigation:** Use different port (9998 vs BSNES port), separate connection UI

### Risk 2: Performance

**Risk:** Real-time UI updates might be slow
**Mitigation:** Batch updates, use INotifyPropertyChanged carefully, background thread

### Risk 3: Cross-Platform

**Risk:** DiztinGUIsh has WIP Eto.Forms port
**Mitigation:** Start with Windows Forms, adapt to Eto.Forms later

### Risk 4: Breaking Changes

**Risk:** DiztinGUIsh internal API changes
**Mitigation:** Use public interfaces only, version protocol

---

## Resources

### DiztinGUIsh

- **Repository:** https://github.com/IsoFrieze/DiztinGUIsh
- **Discord:** #diztinguish in https://sneslab.net/
- **Documentation:** https://github.com/IsoFrieze/DiztinGUIsh/blob/master/Diz.App.Winforms/dist/docs/HELP.md

### Mesen2

- **Repository:** https://github.com/TheAnsarya/Mesen2
- **Project Board:** https://github.com/users/TheAnsarya/projects/4
- **Streaming Spec:** docs/diztinguish/streaming-integration.md

---

## Conclusion

**✅ EXCELLENT NEWS:** DiztinGUIsh is well-architected for this integration!

**Key Advantages:**
1. Already has socket-based tracelog capturing (proven with BSNES)
2. Modular design with clear separation of concerns
3. Tracks all necessary per-byte data (M/X flags, DB, DP)
4. Extensible import system
5. Active development and community support

**Recommended Approach:**
1. Study existing BSNES integration code
2. Create parallel Mesen2 importer following same pattern
3. Reuse Mesen2's existing socket infrastructure
4. Test incrementally with simple ROMs
5. Expand to full feature set

**Timeline:** 4-6 weeks for full integration (both sides)

---

**Analysis Completed:** November 17, 2025  
**Analyst:** GitHub Copilot AI Assistant  
**Confidence:** High (based on public repository analysis)
