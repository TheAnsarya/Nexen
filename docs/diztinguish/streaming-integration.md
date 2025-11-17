# Mesen2 ↔ DiztinGUIsh Real-Time Streaming Integration

**Status:** Planning Phase  
**Priority:** HIGH  
**Target Systems:** SNES (primary), NES, GB/GBC (future)  

---

## Executive Summary

This document outlines a comprehensive plan for real-time streaming integration between Mesen2 (multi-system emulator with debugger) and DiztinGUIsh (SNES disassembler). Unlike the basic CDL/label exchange approach (see [README.md](./README.md)), this proposal leverages Mesen2's existing socket infrastructure to create a live debugging bridge with bidirectional streaming of execution state, code/data analysis, and debug symbols.

**Key Advantages:**
- ✅ Leverages existing Mesen2 socket infrastructure (proven in netplay)
- ✅ Real-time disassembly updates as code executes
- ✅ Bidirectional communication (Mesen2 ↔ DiztinGUIsh)
- ✅ Stream execution traces, memory state, CPU context
- ✅ Live symbol synchronization
- ✅ Best-in-class debugging experience

---

## Architecture Overview

### High-Level Design

```
┌─────────────────────────────────┐         ┌──────────────────────────────┐
│         Mesen2 Emulator         │         │      DiztinGUIsh             │
│                                 │         │                              │
│  ┌──────────────────────────┐  │         │  ┌────────────────────────┐  │
│  │   SNES Emulation Core    │  │         │  │  Disassembly Engine    │  │
│  │                          │  │         │  │                        │  │
│  │  • CPU Execution         │  │         │  │  • 65816 Decoder       │  │
│  │  • Memory Access         │  │         │  │  • Context Tracking    │  │
│  │  • PPU/APU State         │  │         │  │  • Label Management    │  │
│  └──────────┬───────────────┘  │         │  └────────────┬───────────┘  │
│             │                   │         │               │              │
│  ┌──────────▼───────────────┐  │         │  ┌────────────▼───────────┐  │
│  │  Debugger + CDL Tracker  │  │         │  │  Project Manager       │  │
│  │                          │  │         │  │                        │  │
│  │  • Breakpoints           │  │         │  │  • ROM Data            │  │
│  │  • Watches               │  │         │  │  • Annotations         │  │
│  │  • Code/Data Logger      │  │         │  │  • Auto-stepping       │  │
│  └──────────┬───────────────┘  │         │  └────────────┬───────────┘  │
│             │                   │         │               │              │
│  ┌──────────▼───────────────┐  │  TCP    │  ┌────────────▼───────────┐  │
│  │  DiztinGUIsh Bridge      │◄─┼─────────┼─►│  Mesen2 Client         │  │
│  │                          │  │  Socket │  │                        │  │
│  │  • Stream Manager        │  │  (Port  │  │  • Stream Receiver     │  │
│  │  • Message Encoder       │  │  9998)  │  │  • Message Decoder     │  │
│  │  • Event Broadcaster     │  │         │  │  • Event Handler       │  │
│  └──────────────────────────┘  │         │  └────────────────────────┘  │
│                                 │         │                              │
│  [Existing Socket Class]        │         │  [New Network Component]     │
└─────────────────────────────────┘         └──────────────────────────────┘
```

### Communication Protocol

The integration uses a custom binary protocol over TCP sockets, inspired by Mesen2's existing netplay protocol:

**Connection Model:**
- **Server Mode:** Mesen2 runs as server, DiztinGUIsh connects as client
- **Port:** Default 9998 (configurable)
- **Protocol:** TCP with message framing
- **Encoding:** Binary (compact) with optional JSON (debugging)

---

## Message Types & Data Streams

### Stream Categories

#### 1. **Execution Stream** (Mesen2 → DiztinGUIsh)
Real-time execution information as the emulator runs.

**Message Type: `EXEC_TRACE`**
```cpp
struct ExecTraceMessage {
    uint8_t  messageType;       // 0x01
    uint32_t frameNumber;       // Current frame
    uint32_t cycleCount;        // CPU cycles in frame
    uint16_t entryCount;        // Number of trace entries
    
    // Variable-length array of entries:
    struct TraceEntry {
        uint32_t address;       // Program counter (24-bit in SNES)
        uint8_t  opcode;        // Instruction opcode
        uint8_t  operand1;      // First operand byte (if any)
        uint8_t  operand2;      // Second operand byte (if any)
        uint8_t  flags;         // CPU flags (NVMXDIZc)
        uint8_t  mFlag;         // M flag state (0 or 1)
        uint8_t  xFlag;         // X flag state (0 or 1)
        uint8_t  dataBank;      // Data bank register
        uint16_t directPage;    // Direct page register
        uint32_t effectiveAddr; // Calculated effective address (0 if N/A)
    } entries[entryCount];
};
```

**Frequency:** Configurable (every N frames, or on-demand)

---

**Message Type: `MEMORY_ACCESS`**
```cpp
struct MemoryAccessMessage {
    uint8_t  messageType;       // 0x02
    uint32_t frameNumber;
    uint16_t accessCount;
    
    struct MemoryAccess {
        uint32_t address;       // Memory address accessed
        uint8_t  accessType;    // READ=1, WRITE=2, EXECUTE=3
        uint8_t  dataType;      // CODE=1, DATA=2, GFX=4, AUDIO=8
        uint8_t  value;         // Value read/written (if applicable)
    } accesses[accessCount];
};
```

---

#### 2. **State Synchronization** (Mesen2 → DiztinGUIsh)

**Message Type: `CPU_STATE`**
```cpp
struct CpuStateMessage {
    uint8_t  messageType;       // 0x10
    uint32_t frameNumber;
    
    // Full CPU state snapshot
    uint32_t pc;                // Program counter
    uint8_t  a;                 // Accumulator (low byte)
    uint8_t  ah;                // Accumulator (high byte, 65816)
    uint8_t  x;                 // X register
    uint8_t  y;                 // Y register
    uint8_t  sp;                // Stack pointer
    uint8_t  flags;             // Status flags
    uint8_t  dataBank;          // Data bank
    uint16_t directPage;        // Direct page
    bool     emulationMode;     // 6502 emulation mode
};
```

**Message Type: `CDL_UPDATE`**
```cpp
struct CdlUpdateMessage {
    uint8_t  messageType;       // 0x11
    uint32_t startAddress;      // ROM address
    uint32_t length;            // Number of bytes
    uint8_t  cdlData[length];   // CDL flags per byte
};
```

---

#### 3. **Debug Events** (Mesen2 → DiztinGUIsh)

**Message Type: `BREAKPOINT_HIT`**
```cpp
struct BreakpointMessage {
    uint8_t  messageType;       // 0x20
    uint32_t address;           // Breakpoint address
    uint8_t  breakpointType;    // EXEC=1, READ=2, WRITE=3
    uint32_t frameNumber;
    CpuStateMessage cpuState;   // Full CPU state at break
};
```

**Message Type: `LABEL_UPDATED`**
```cpp
struct LabelUpdateMessage {
    uint8_t  messageType;       // 0x21
    uint32_t address;
    uint8_t  labelLength;
    char     labelName[256];    // Null-terminated
    uint8_t  commentLength;
    char     comment[512];      // Null-terminated
};
```

---

#### 4. **Control Commands** (DiztinGUIsh → Mesen2)

**Message Type: `SET_BREAKPOINT`**
```cpp
struct SetBreakpointMessage {
    uint8_t  messageType;       // 0x30
    uint32_t address;
    uint8_t  breakpointType;    // EXEC=1, READ=2, WRITE=3
    bool     enabled;
};
```

**Message Type: `REQUEST_MEMORY`**
```cpp
struct RequestMemoryMessage {
    uint8_t  messageType;       // 0x31
    uint32_t startAddress;
    uint32_t length;
    uint8_t  memoryType;        // ROM=1, RAM=2, SRAM=3, etc.
};
```

**Message Type: `PUSH_LABELS`**
```cpp
struct PushLabelsMessage {
    uint8_t  messageType;       // 0x32
    uint16_t labelCount;
    
    struct Label {
        uint32_t address;
        uint8_t  labelLength;
        char     labelName[256];
        uint8_t  commentLength;
        char     comment[512];
    } labels[labelCount];
};
```

---

#### 5. **Session Management** (Bidirectional)

**Message Type: `HANDSHAKE`**
```cpp
struct HandshakeMessage {
    uint8_t  messageType;       // 0x00
    uint8_t  protocolVersion;   // Current: 1
    uint16_t clientType;        // MESEN2=1, DIZTINGUISH=2
    char     clientVersion[32]; // e.g., "Mesen2 2.1.0"
    uint32_t romChecksum;       // CRC32 of loaded ROM
    uint32_t romSize;
};
```

**Message Type: `CONFIG_STREAM`**
```cpp
struct ConfigStreamMessage {
    uint8_t  messageType;       // 0x01
    bool     enableExecTrace;
    bool     enableMemoryAccess;
    bool     enableCdlUpdates;
    uint16_t traceFrameInterval; // Send trace every N frames (0=manual)
    uint32_t maxEntriesPerFrame; // Limit trace size
};
```

**Message Type: `DISCONNECT`**
```cpp
struct DisconnectMessage {
    uint8_t  messageType;       // 0xFF
    uint8_t  reasonCode;        // NORMAL=0, ERROR=1, etc.
    char     reason[256];       // Human-readable reason
};
```

---

## Implementation Plan

### Phase 1: Foundation (Weeks 1-4)

#### Mesen2 Changes

**1.1: Create DiztinGUIsh Bridge Component**
- [ ] Create `Core/Debugger/DiztinguishBridge.h/.cpp`
- [ ] Implement server socket using existing `Utilities/Socket.h`
- [ ] Message encoding/decoding framework
- [ ] Connection management (accept, disconnect, heartbeat)
- [ ] Thread-safe message queue

**Files to Create:**
```
Core/Debugger/DiztinguishBridge.h
Core/Debugger/DiztinguishBridge.cpp
Core/Debugger/DiztinguishMessage.h
Core/Debugger/DiztinguishProtocol.h
```

**1.2: Integrate with SNES Debugger**
- [ ] Hook execution trace capture
- [ ] Hook memory access events (via CDL system)
- [ ] Hook breakpoint events
- [ ] Add UI toggle: "Enable DiztinGUIsh Connection"
- [ ] Add connection status indicator

**Files to Modify:**
```
Core/SNES/Debugger/SnesDebugger.h/.cpp
UI/Debugger/Windows/DebuggerWindow.axaml
UI/Debugger/ViewModels/DebuggerWindowViewModel.cs
```

**1.3: Configuration & Settings**
- [ ] Add DiztinGUIsh settings to debugger config
- [ ] Port configuration
- [ ] Stream options (what to send)
- [ ] Performance tuning (buffer sizes, intervals)

**Files to Modify:**
```
Core/Shared/SettingTypes.h
UI/Config/DebuggerConfig.cs
```

---

#### DiztinGUIsh Changes

**1.4: Create Mesen2 Client Component**
- [ ] Create networking module (using .NET sockets or similar)
- [ ] Message decoding framework
- [ ] Connection manager (connect, reconnect, timeout handling)
- [ ] Event dispatcher for received messages

**Files to Create (approximate - TBD based on DiztinGUIsh structure):**
```
Diz.Integration.Mesen2/MesenClient.cs
Diz.Integration.Mesen2/MesenProtocol.cs
Diz.Integration.Mesen2/MessageHandler.cs
```

**1.5: UI Integration**
- [ ] Add "Connect to Mesen2" menu item
- [ ] Connection dialog (hostname, port)
- [ ] Connection status indicator
- [ ] Stream visualization (optional - show incoming data rate)

---

### Phase 2: Real-Time Disassembly (Weeks 5-8)

**2.1: Execution Trace Processing**
- [ ] Receive `EXEC_TRACE` messages
- [ ] Extract M/X flag context from traces
- [ ] Auto-mark code regions as executed
- [ ] Update disassembly view in real-time
- [ ] Highlight recently executed instructions

**2.2: CDL Integration**
- [ ] Receive `CDL_UPDATE` messages
- [ ] Merge incoming CDL with project data
- [ ] Visualize coverage in UI
- [ ] Export merged CDL back to file

**2.3: Memory Analysis**
- [ ] Receive `MEMORY_ACCESS` messages
- [ ] Identify graphics data (PPU reads)
- [ ] Identify audio data (APU reads)
- [ ] Auto-categorize data types

---

### Phase 3: Bidirectional Control (Weeks 9-12)

**3.1: Breakpoint Synchronization**
- [ ] DiztinGUIsh can set breakpoints in Mesen2
- [ ] Mesen2 notifies DiztinGUIsh when breakpoint hit
- [ ] Visualize breakpoint locations in both tools
- [ ] Synchronized step/continue controls

**3.2: Label Synchronization**
- [ ] Push labels from DiztinGUIsh to Mesen2
- [ ] Pull labels from Mesen2 to DiztinGUIsh
- [ ] Live updates when labels change
- [ ] Conflict resolution (manual or auto-merge)

**3.3: Memory Inspection**
- [ ] DiztinGUIsh requests memory dumps
- [ ] Mesen2 sends memory snapshots
- [ ] Visualize memory in DiztinGUIsh UI
- [ ] Compare memory over time

---

### Phase 4: Advanced Features (Weeks 13-16)

**4.1: Performance Optimization**
- [ ] Implement message batching
- [ ] Compression for large messages
- [ ] Selective streaming (only changed data)
- [ ] Adaptive update rates based on connection speed

**4.2: Session Recording**
- [ ] Record entire streaming session to file
- [ ] Replay sessions offline
- [ ] Export to standard formats (trace logs, CDL, etc.)
- [ ] Session analytics (coverage, hotspots, etc.)

**4.3: Multi-System Support**
- [ ] Extend protocol to support NES
- [ ] Extend protocol to support GB/GBC
- [ ] System-specific message types
- [ ] Protocol negotiation based on system

---

## Technical Specifications

### Socket Implementation Details

**Using Mesen2's Existing Socket Class:**

The implementation will leverage `Utilities/Socket.h` which already provides:
- ✅ Cross-platform socket support (Windows/Linux/macOS)
- ✅ Non-blocking I/O
- ✅ TCP with Nagle disabled (low latency)
- ✅ Large send/recv buffers (256KB)
- ✅ Connection error handling

**Example Server Setup (Mesen2):**
```cpp
// In DiztinguishBridge.cpp
void DiztinguishBridge::StartServer(uint16_t port) {
    _listener = std::make_unique<Socket>();
    _listener->Bind(port);
    _listener->Listen(1); // Only one client at a time
    
    _serverThread = std::thread([this]() {
        while (_isRunning) {
            auto clientSocket = _listener->Accept();
            if (!clientSocket->ConnectionError()) {
                HandleClient(std::move(clientSocket));
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
}

void DiztinguishBridge::HandleClient(std::unique_ptr<Socket> socket) {
    // Handshake
    HandshakeMessage handshake;
    int bytesRead = socket->Recv((char*)&handshake, sizeof(handshake), 0);
    
    if (handshake.clientType == CLIENT_DIZTINGUISH) {
        _clientSocket = std::move(socket);
        _isConnected = true;
        OnClientConnected();
    }
}

void DiztinguishBridge::SendMessage(const DiztinguishMessage& msg) {
    if (_isConnected && _clientSocket) {
        msg.Send(*_clientSocket);
    }
}
```

---

### Message Framing

To handle variable-length messages over TCP streams:

```cpp
struct MessageFrame {
    uint8_t  messageType;
    uint32_t messageLength;  // Length of payload (excluding header)
    uint8_t  payload[...];   // Variable-length payload
};
```

**Sending:**
```cpp
void DiztinguishMessage::Send(Socket& socket) {
    uint32_t length = GetPayloadLength();
    
    // Send header
    socket.Send((char*)&_messageType, 1, 0);
    socket.Send((char*)&length, 4, 0);
    
    // Send payload
    socket.Send((char*)_payload.data(), length, 0);
}
```

**Receiving:**
```cpp
std::unique_ptr<DiztinguishMessage> ReadMessage(Socket& socket) {
    uint8_t messageType;
    uint32_t length;
    
    // Read header
    socket.Recv((char*)&messageType, 1, 0);
    socket.Recv((char*)&length, 4, 0);
    
    // Read payload
    std::vector<uint8_t> payload(length);
    socket.Recv((char*)payload.data(), length, 0);
    
    return DecodeMessage(messageType, payload);
}
```

---

### Performance Considerations

**Bandwidth Estimation:**

Assuming 60 FPS gameplay with moderate activity:

| Stream Type | Per Frame | Per Second | Per Minute |
|-------------|-----------|------------|------------|
| Exec Trace (100 instructions) | 2.5 KB | 150 KB | 9 MB |
| Memory Access (500 accesses) | 6 KB | 360 KB | 21.6 MB |
| CDL Updates (differential) | 0.5 KB | 30 KB | 1.8 MB |
| **Total** | **9 KB** | **540 KB** | **32.4 MB** |

**Optimization Strategies:**
1. **Frame Skipping:** Send traces every N frames (default: every 4 frames = 15 Hz)
2. **Differential Updates:** Only send changed CDL bytes
3. **Compression:** zlib compression for large payloads
4. **Batching:** Combine multiple small messages into batches
5. **Throttling:** Adaptive rate limiting based on connection speed

**Result:** ~135 KB/s (1 Mbps) with default settings - easily achievable on LAN

---

### Security Considerations

**Threat Model:**
- Local network only (not internet-facing by default)
- No authentication in Phase 1 (optional in Phase 4)
- DiztinGUIsh can send commands to Mesen2 (breakpoints, etc.)

**Mitigations:**
1. **Localhost Only:** Default binding to 127.0.0.1
2. **Optional Password:** Simple shared secret authentication
3. **Command Whitelisting:** Only allow safe debug commands
4. **Rate Limiting:** Prevent command flooding
5. **UI Confirmation:** Require user approval for connections

---

## Use Cases & Workflows

### Workflow 1: Live Disassembly Session

**Scenario:** Developer wants to disassemble Super Mario World while playing.

1. **Setup:**
   - Load Super Mario World in Mesen2
   - Start DiztinGUIsh Bridge (Debugger → "Start DiztinGUIsh Server")
   - Launch DiztinGUIsh
   - Connect to Mesen2 (File → "Connect to Mesen2...")
   - DiztinGUIsh receives ROM checksum and creates new project

2. **Play Through Levels:**
   - Play through World 1-1 in Mesen2
   - DiztinGUIsh receives execution traces in real-time
   - Code regions automatically marked as executed
   - M/X flags automatically detected and applied
   - Graphics data identified from PPU reads

3. **Analysis:**
   - Pause emulation in Mesen2
   - DiztinGUIsh shows coverage map (what code was executed)
   - Developer adds labels in DiztinGUIsh
   - Labels automatically appear in Mesen2 debugger

4. **Debugging:**
   - Set breakpoint in DiztinGUIsh at subroutine entry
   - Breakpoint automatically set in Mesen2
   - Continue emulation
   - Breakpoint hit, both tools pause
   - Inspect CPU state in Mesen2, disassembly in DiztinGUIsh

5. **Export:**
   - Disconnect from Mesen2
   - DiztinGUIsh exports final disassembly with labels
   - Mesen2 exports complete CDL file
   - Developer has fully annotated, accurate disassembly

---

### Workflow 2: Automated Analysis

**Scenario:** Use scripting to automate disassembly across multiple ROMs.

1. **Script Setup:**
   - Write Python script using DiztinGUIsh API
   - Connect to Mesen2 server
   - Load ROM list

2. **Automated Run:**
   - For each ROM:
     - Load ROM in Mesen2 via API
     - Create DiztinGUIsh project
     - Run automated gameplay (AI, TAS, or scripted inputs)
     - Collect execution traces
     - Export disassembly

3. **Batch Analysis:**
   - Compare disassemblies across ROM versions
   - Identify common code patterns
   - Generate coverage reports

---

## Testing Strategy

### Unit Tests

**Mesen2 Tests:**
- [ ] Message encoding/decoding correctness
- [ ] Socket connection handling
- [ ] Thread safety of message queue
- [ ] CDL data extraction accuracy

**DiztinGUIsh Tests:**
- [ ] Message parsing correctness
- [ ] Protocol version compatibility
- [ ] Label import/export roundtrip
- [ ] Connection error handling

---

### Integration Tests

**End-to-End Scenarios:**
- [ ] Connect DiztinGUIsh to Mesen2
- [ ] Stream execution traces for 1000 frames
- [ ] Verify trace accuracy against known ROM
- [ ] Push 1000 labels from DiztinGUIsh
- [ ] Verify labels appear in Mesen2
- [ ] Set/hit breakpoints bidirectionally

**Performance Tests:**
- [ ] Measure bandwidth under various scenarios
- [ ] Test with slow connections (simulate WAN latency)
- [ ] Stress test with 10,000 instructions/frame
- [ ] Memory leak testing (24-hour connection)

---

### Test ROMs

**Standard Test Suite:**
1. **Super Mario World** (LoROM, moderate complexity)
2. **The Legend of Zelda: A Link to the Past** (HiROM, large ROM)
3. **Super Metroid** (Complex code, bank switching)
4. **Custom Test ROM** (All addressing modes, edge cases)

---

## Risk Analysis

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Network latency impacts UX | Medium | Medium | Implement async updates, show "syncing" indicators |
| Protocol incompatibility across versions | High | High | Strict version checking, graceful degradation |
| Performance overhead in Mesen2 | Medium | High | Make streaming opt-in, optimize hot paths |
| DiztinGUIsh can't handle high-frequency updates | Medium | Medium | Implement throttling, queue management |
| Security vulnerabilities | Low | Medium | Localhost-only default, optional authentication |
| Complexity exceeds ROI | Medium | High | Implement in phases, validate each phase before continuing |

---

## Success Metrics

### Phase 1 Success Criteria
- [ ] Mesen2 can accept DiztinGUIsh connections
- [ ] Handshake protocol works reliably
- [ ] Basic message exchange (ping/pong)
- [ ] Connection status visible in both UIs
- [ ] No crashes or memory leaks in 1-hour test

### Phase 2 Success Criteria
- [ ] Execution traces stream in real-time
- [ ] DiztinGUIsh displays trace data correctly
- [ ] M/X flag detection accuracy > 95%
- [ ] CDL coverage matches manual verification
- [ ] Latency < 100ms for local connections

### Phase 3 Success Criteria
- [ ] Bidirectional label sync works
- [ ] 100% label preservation (no data loss)
- [ ] Breakpoints set from DiztinGUIsh work
- [ ] Both tools stay synchronized during debugging
- [ ] No race conditions or deadlocks

### Phase 4 Success Criteria
- [ ] Bandwidth reduced by 50% via optimizations
- [ ] Session recording/playback works
- [ ] Multi-system support (NES) implemented
- [ ] User documentation complete
- [ ] 10+ community beta testers report success

---

## Alternative Architectures Considered

### Alt 1: Named Pipes (Windows) / Unix Domain Sockets (Linux)
**Pros:** Faster than TCP for local IPC  
**Cons:** Platform-specific, less portable  
**Decision:** Rejected - TCP is portable and fast enough

### Alt 2: Shared Memory
**Pros:** Highest performance for large data transfers  
**Cons:** Complex synchronization, platform-specific  
**Decision:** Rejected - overkill for this use case

### Alt 3: HTTP/REST API
**Pros:** Well-understood, many libraries  
**Cons:** Higher overhead, request/response model doesn't fit streaming  
**Decision:** Rejected - TCP sockets better for real-time streaming

### Alt 4: WebSocket
**Pros:** Modern, browser-compatible, bidirectional  
**Cons:** Requires HTTP handshake, additional dependencies  
**Decision:** Maybe for Phase 4 - could enable web-based clients

---

## Future Enhancements

### Web-Based DiztinGUIsh Client
- Convert protocol to WebSocket
- Build browser-based disassembly viewer
- Connect to Mesen2 from any device on network
- Collaborative disassembly sessions

### Mobile Companion App
- View disassembly on tablet while debugging on PC
- Remote breakpoint control
- Coverage visualization

### AI-Assisted Labeling
- Stream execution data to ML model
- Suggest labels based on patterns
- Auto-detect common library functions
- Propose data structure interpretations

### Multi-Emulator Support
- Extend protocol to other emulators (bsnes, Snes9x)
- Generalized debugging protocol
- Industry-standard debugging interface

---

## References

- [Mesen2 Netplay Implementation](../../Core/Netplay/) - Reference for socket usage
- [Mesen2 Socket Class](../../Utilities/Socket.h) - Low-level networking
- [DiztinGUIsh Architecture](./analysis.md) - DiztinGUIsh internals
- [Integration Options](./README.md) - Alternative approaches
- [CDL Format Spec](./README.md#cdl-format-specification-phase-1) - Code/Data Logger details

---

## Appendices

### Appendix A: Complete Message Type Reference

| Type | ID | Direction | Description |
|------|----|-----------|---------------------------------|
| HANDSHAKE | 0x00 | Both | Initial connection handshake |
| CONFIG_STREAM | 0x01 | Both | Configure streaming options |
| EXEC_TRACE | 0x02 | M→D | Execution trace batch |
| MEMORY_ACCESS | 0x03 | M→D | Memory access log |
| CPU_STATE | 0x10 | M→D | Full CPU state snapshot |
| CDL_UPDATE | 0x11 | M→D | Code/Data Logger update |
| BREAKPOINT_HIT | 0x20 | M→D | Breakpoint triggered |
| LABEL_UPDATED | 0x21 | M→D | Label changed in Mesen2 |
| SET_BREAKPOINT | 0x30 | D→M | Set/clear breakpoint |
| REQUEST_MEMORY | 0x31 | D→M | Request memory dump |
| PUSH_LABELS | 0x32 | D→M | Send labels to Mesen2 |
| DISCONNECT | 0xFF | Both | Clean disconnect |

*(M→D = Mesen2 to DiztinGUIsh, D→M = DiztinGUIsh to Mesen2)*

---

### Appendix B: Error Codes

| Code | Name | Description |
|------|------|-------------|
| 0x00 | SUCCESS | Operation completed successfully |
| 0x01 | INVALID_MESSAGE | Malformed message received |
| 0x02 | PROTOCOL_MISMATCH | Incompatible protocol versions |
| 0x03 | ROM_MISMATCH | Checksum doesn't match loaded ROM |
| 0x04 | CONNECTION_LOST | Socket error or timeout |
| 0x05 | AUTHENTICATION_FAILED | Password incorrect |
| 0x06 | UNSUPPORTED_OPERATION | Feature not implemented |
| 0x10 | INTERNAL_ERROR | Unexpected error in handler |

---

### Appendix C: Configuration Examples

**Mesen2 Settings (JSON):**
```json
{
  "DiztinguishBridge": {
    "Enabled": true,
    "Port": 9998,
    "BindAddress": "127.0.0.1",
    "Password": "",
    "StreamSettings": {
      "EnableExecTrace": true,
      "TraceFrameInterval": 4,
      "MaxEntriesPerFrame": 1000,
      "EnableMemoryAccess": false,
      "EnableCdlUpdates": true
    }
  }
}
```

**DiztinGUIsh Connection Config:**
```json
{
  "Mesen2Connection": {
    "Hostname": "127.0.0.1",
    "Port": 9998,
    "AutoConnect": false,
    "ReconnectOnDisconnect": true,
    "BufferSize": 65536
  }
}
```

---

**Document Version:** 1.0  
**Last Updated:** 2025-11-17  
**Authors:** Mesen2-DiztinGUIsh Integration Team  
**Status:** Planning - Awaiting Phase 1 Approval
