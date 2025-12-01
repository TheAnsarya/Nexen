# Execution Trace Flow: Mesen2 → DiztinGUIsh

## Complete Step-by-Step Data Flow Analysis

### 1. MESEN2 STARTUP & INITIALIZATION

#### 1.1 User Starts DiztinGUIsh Server
**Location:** `UI` → Tools menu → DiztinGUIsh Server → Start Server
**File:** `UI/InteropDLL/DiztinguishApiWrapper.cpp`

```cpp
DllExport void StartDiztinguishServer(uint16_t port) {
    SnesDebugger* debugger = GetSnesDebugger();
    debugger->StartDiztinguishServer(port);  // Default port 9998
}
```

#### 1.2 SnesDebugger Creates Bridge
**File:** `Core/SNES/Debugger/SnesDebugger.cpp`

```cpp
void SnesDebugger::StartDiztinguishServer(uint16_t port) {
    if(!_diztinguishBridge) {
        _diztinguishBridge = std::make_unique<DiztinguishBridge>(
            _console, this, _debugger
        );
    }
    _diztinguishBridge->StartServer(port);
}
```

#### 1.3 Bridge Starts TCP Server
**File:** `Core/Debugger/DiztinguishBridge.cpp`

```cpp
bool DiztinguishBridge::StartServer(uint16_t port) {
    // Create server socket
    _serverSocket = std::make_unique<Socket>();
    _serverSocket->Bind(port);        // Bind to port 9998
    _serverSocket->Listen(1);         // Accept 1 client
    
    // Start server thread (waits for connections)
    _serverThread = std::make_unique<std::thread>(
        &DiztinguishBridge::ServerThreadMain, this
    );
    
    _debugger->Log("[DiztinGUIsh] Server started on port " + std::to_string(port));
}
```

---

### 2. CONNECTION ESTABLISHMENT

#### 2.1 DiztinGUIsh Connects
**File:** `DiztinGUIsh/Diz.Import/src/mesen/tracelog/MesenLiveTraceClient.cs`

```csharp
public async Task<bool> ConnectAsync(string host = "localhost", int port = 9998) {
    _tcpClient = new TcpClient();
    await _tcpClient.ConnectAsync(host, port, timeoutCts.Token);
    _stream = _tcpClient.GetStream();
    
    // Start receive loop on background thread
    _receiveTask = ReceiveLoopAsync(_cancellationTokenSource.Token);
}
```

#### 2.2 Mesen2 Accepts Connection
**File:** `Core/Debugger/DiztinguishBridge.cpp`

```cpp
void DiztinguishBridge::ServerThreadMain() {
    while(_serverRunning) {
        // Accept() blocks until client connects
        _clientSocket = _serverSocket->Accept();
        
        if(_clientSocket && !_clientSocket->ConnectionError()) {
            _clientConnected = true;
            _debugger->Log("[DiztinGUIsh] Client connected!");
            
            // Send handshake immediately
            SendHandshake();
            
            // Start receive thread (waits for client messages)
            _receiveThread = std::make_unique<std::thread>(
                &DiztinguishBridge::ReceiveThreadMain, this
            );
        }
    }
}
```

---

### 3. HANDSHAKE PROTOCOL

#### 3.1 Mesen2 Sends Handshake
**File:** `Core/Debugger/DiztinguishBridge.cpp`

```cpp
void DiztinguishBridge::SendHandshake() {
    // Get ROM information from cartridge
    BaseCartridge* cart = _console->GetCartridge();
    uint32_t romSize = cart->DebugGetPrgRomSize();        // Actual ROM size
    uint32_t romChecksum = cart->GetCrc32();              // CRC32 checksum
    SnesCartInformation cartInfo = cart->GetHeader();
    string romName = std::string(cartInfo.CartName, 
                                  strnlen(cartInfo.CartName, 21));
    
    // Build handshake message (268 bytes total)
    HandshakeMessage msg = {};
    msg.protocolVersionMajor = 1;
    msg.protocolVersionMinor = 0;
    msg.romChecksum = romChecksum;
    msg.romSize = romSize;
    strncpy_s(msg.romName, sizeof(msg.romName), romName.c_str(), _TRUNCATE);
    
    // Send message
    SendMessage(MessageType::Handshake, &msg, sizeof(msg));
    FlushOutgoingMessages();
}
```

**Binary Format (268 bytes):**
```
Offset | Size | Field
-------|------|------------------
0      | 2    | protocolVersionMajor (1)
2      | 2    | protocolVersionMinor (0)
4      | 4    | romChecksum (CRC32)
8      | 4    | romSize (bytes)
12     | 256  | romName (null-terminated)
```

#### 3.2 DiztinGUIsh Receives Handshake
**File:** `DiztinGUIsh/Diz.Import/src/mesen/tracelog/MesenLiveTraceClient.cs`

```csharp
private async Task ReceiveLoopAsync(CancellationToken cancellationToken) {
    while (!cancellationToken.IsCancellationRequested && IsConnected) {
        // Read 5-byte message header
        var headerBytes = await ReadExactBytesAsync(5, cancellationToken);
        var messageType = (MesenMessageType)headerBytes[0];
        var messageLength = BitConverter.ToUInt32(headerBytes, 1);
        
        // Read payload
        var payloadBytes = await ReadExactBytesAsync((int)messageLength, cancellationToken);
        
        // Parse and dispatch
        ProcessMessage(messageType, payloadBytes);
    }
}

private static MesenHandshakeMessage ParseHandshakeMessage(byte[] data) {
    return new MesenHandshakeMessage {
        ProtocolVersionMajor = BitConverter.ToUInt16(data, 0),
        ProtocolVersionMinor = BitConverter.ToUInt16(data, 2),
        RomChecksum = BitConverter.ToUInt32(data, 4),
        RomSize = BitConverter.ToUInt32(data, 8),
        RomName = Encoding.ASCII.GetString(data, 12, 256).TrimEnd('\0')
    };
}
```

#### 3.3 DiztinGUIsh Validates & Responds
**File:** `DiztinGUIsh/Diz.Import/src/mesen/tracelog/MesenTraceLogImporter.cs`

```csharp
private void OnHandshakeReceived(object? sender, MesenHandshakeMessage handshake) {
    // Validate protocol version
    var protocolCompatible = handshake.ProtocolVersionMajor == 1;
    
    // Validate ROM size match
    var romCompatible = _romSizeCached <= 0 || 
                        handshake.RomSize <= 0 || 
                        handshake.RomSize == _romSizeCached;
    
    var accepted = protocolCompatible && romCompatible;
    
    // Send acknowledgment
    await _client.SendHandshakeAckAsync(accepted, "DiztinGUIsh v2.0");
    
    // CRITICAL: Send configuration to enable trace streaming!
    if (accepted) {
        await _client.SendConfigAsync(
            enableExecTrace: true,
            enableMemoryAccess: false,
            enableCdlUpdates: true,
            traceFrameInterval: 4,      // Every 4 frames (15 Hz @ 60 FPS)
            maxTracesPerFrame: 1000     // Max batch size
        );
    }
}
```

#### 3.4 Mesen2 Receives HandshakeAck
**File:** `Core/Debugger/DiztinguishBridge.cpp`

```cpp
void DiztinguishBridge::ReceiveThreadMain() {
    _debugger->Log("[DiztinGUIsh] *** RECEIVE THREAD STARTED - Waiting for messages ***");
    
    while(_clientConnected && _serverRunning) {
        // Read message header (5 bytes) - BLOCKS until data available
        int received = _clientSocket->BlockingRecv(headerBuf, sizeof(MessageHeader), 30);
        
        if(received != sizeof(MessageHeader)) {
            break; // Connection closed
        }
        
        // Parse header
        MessageHeader header;
        header.type = (MessageType)headerBuf[0];
        header.length = *(uint32_t*)(headerBuf + 1);
        
        // Read payload
        std::vector<uint8_t> payload(header.length);
        received = _clientSocket->BlockingRecv((char*)payload.data(), header.length, 30);
        
        // Process message
        ProcessIncomingMessage(header.type, payload);
    }
}

void DiztinguishBridge::HandleHandshakeAck(const HandshakeAckMessage& msg) {
    if(!msg.accepted) {
        _debugger->Log("[DiztinGUIsh] Handshake rejected by client");
        _clientConnected = false;
        return;
    }
    _debugger->Log("[DiztinGUIsh] Handshake accepted by " + std::string(msg.clientName));
}
```

#### 3.5 Mesen2 Receives Config
**File:** `Core/Debugger/DiztinguishBridge.cpp`

```cpp
void DiztinguishBridge::HandleConfigStream(const ConfigStreamMessage& msg) {
    _config = msg;
    _configReceived = true;  // ← CRITICAL FLAG SET HERE
    
    _debugger->Log("[DiztinGUIsh] *** CONFIG RECEIVED - Trace streaming now enabled ***");
    _debugger->Log("  ExecTrace: " + std::string(_config.enableExecTrace ? "ON" : "OFF"));
    _debugger->Log("  Trace Interval: " + std::to_string(_config.traceFrameInterval) + " frames");
    _debugger->Log("  Max Traces/Frame: " + std::to_string(_config.maxTracesPerFrame));
}
```

**Binary Format (6 bytes):**
```
Offset | Size | Field
-------|------|------------------
0      | 1    | enableExecTrace (0 or 1)
1      | 1    | enableMemoryAccess (0 or 1)
2      | 1    | enableCdlUpdates (0 or 1)
3      | 1    | traceFrameInterval (frames)
4      | 2    | maxTracesPerFrame (uint16)
```

---

### 4. CPU EXECUTION TRACING (MAIN DATA FLOW)

#### 4.1 CPU Executes Instruction
**File:** `Core/SNES/SnesCpu.cpp`

```cpp
void SnesCpu::Exec() {
    if(_state.StopState == SnesCpuStopState::Running) {
        // CRITICAL: ProcessInstruction() calls debugger hooks
        _emu->ProcessInstruction<CpuType::Snes>();  // ← THIS TRIGGERS TRACING
        
        RunOp();           // Execute the actual 65816 instruction
        CheckForInterrupts();
    }
}
```

#### 4.2 Emulator Calls Debugger Hook
**File:** `Shared/Emulator.h` (template method)

```cpp
template<CpuType cpuType>
void ProcessInstruction() {
    if(HasDebugger(cpuType)) {
        GetDebugger(cpuType)->ProcessInstruction();  // ← Calls SnesDebugger
    }
}
```

#### 4.3 SnesDebugger Processes Instruction
**File:** `Core/SNES/Debugger/SnesDebugger.cpp`

```cpp
void SnesDebugger::ProcessInstruction() {
    SnesCpuState& state = GetCpuState();
    uint32_t pc = (state.K << 16) | state.PC;  // Build 24-bit address
    uint8_t opCode = _memoryMappings->Peek(pc);
    
    // ══════════════════════════════════════════════════════════
    // FRAME TRACKING - Detects when PPU frame number changes
    // ══════════════════════════════════════════════════════════
    if(_diztinguishBridge && _diztinguishBridge->IsClientConnected()) {
        static uint32_t lastFrame = 0;
        uint32_t currentFrame = _ppu->GetFrameCount();
        
        if(currentFrame != lastFrame) {
            if(lastFrame > 0) {
                // Frame ended - flush buffered traces
                _diztinguishBridge->OnFrameEnd();  // ← SENDS BATCHES
            }
            _diztinguishBridge->OnFrameStart(currentFrame);
            lastFrame = currentFrame;
        }
    }
    
    // Update Code/Data Logger
    if(addressInfo.Type == MemoryType::SnesPrgRom) {
        uint8_t cdlFlags = SnesDisUtils::GetOpFlags(_prevOpCode, pc, _prevProgramCounter);
        _cdl->SetCode(addressInfo.Address, cdlFlags);
        
        // Notify bridge of CDL change
        if(_diztinguishBridge && _diztinguishBridge->IsClientConnected()) {
            _diztinguishBridge->OnCdlChanged(addressInfo.Address, cdlFlags);
        }
    }
    
    // ... (breakpoint checks, etc.) ...
    
    // ══════════════════════════════════════════════════════════
    // EXECUTION TRACE - Records every instruction execution
    // ══════════════════════════════════════════════════════════
    if(_diztinguishBridge && _diztinguishBridge->IsClientConnected()) {
        bool mFlag = (state.PS & ProcFlags::MemoryMode8) != 0;
        bool xFlag = (state.PS & ProcFlags::IndexMode8) != 0;
        uint32_t effectiveAddr = 0;  // TODO: Calculate from instruction
        
        _diztinguishBridge->OnCpuExec(pc, opCode, mFlag, xFlag, 
                                       state.DBR, state.D, effectiveAddr);
    }
}
```

**Key Points:**
- Called for **EVERY instruction** the CPU executes
- Only works if debugger is enabled (Tools → Debugger must be open)
- Tracks frame boundaries using PPU frame counter
- Sends both execution traces AND CDL updates

#### 4.4 Bridge Buffers Trace Entry
**File:** `Core/Debugger/DiztinguishBridge.cpp`

```cpp
void DiztinguishBridge::OnCpuExec(uint32_t pc, uint8_t opcode, 
                                   bool mFlag, bool xFlag, uint8_t db, 
                                   uint16_t dp, uint32_t effectiveAddr) {
    // Check if streaming enabled
    if(!_clientConnected || !_configReceived || !_config.enableExecTrace) {
        return;  // Config not received yet - traces ignored!
    }
    
    // Don't exceed max traces per frame
    if(_traceBuffer.size() >= _config.maxTracesPerFrame) {
        return;  // Buffer full - drop trace
    }
    
    // Create trace entry (15 bytes)
    ExecTraceEntry entry;
    entry.pc = pc;                 // 24-bit program counter
    entry.opcode = opcode;         // Opcode byte
    entry.mFlag = mFlag ? 1 : 0;   // Memory/Accumulator mode
    entry.xFlag = xFlag ? 1 : 0;   // Index register mode
    entry.dbRegister = db;         // Data bank register
    entry.dpRegister = dp;         // Direct page register
    entry.effectiveAddr = effectiveAddr;  // Calculated address
    
    _traceBuffer.push_back(entry);  // ← BUFFERED, NOT SENT YET
}
```

**Binary Format (15 bytes per entry):**
```
Offset | Size | Field           | Description
-------|------|-----------------|---------------------------
0      | 4    | pc              | 24-bit address (top byte ignored)
4      | 1    | opcode          | Instruction opcode
5      | 1    | mFlag           | 0=16-bit A, 1=8-bit A
6      | 1    | xFlag           | 0=16-bit X/Y, 1=8-bit X/Y
7      | 1    | dbRegister      | Data bank (DBR)
8      | 2    | dpRegister      | Direct page (D)
10     | 4    | effectiveAddr   | 24-bit effective address
```

#### 4.5 Frame End Triggers Batch Send
**File:** `Core/Debugger/DiztinguishBridge.cpp`

```cpp
void DiztinguishBridge::OnFrameEnd() {
    if(!_clientConnected || !_configReceived) {
        return;
    }
    
    // ══════════════════════════════════════════════════════════
    // SEND EXECUTION TRACE BATCH
    // ══════════════════════════════════════════════════════════
    if(_config.enableExecTrace && !_traceBuffer.empty()) {
        uint32_t framesSinceLastSend = _currentFrame - _lastTraceSentFrame;
        
        // Check if interval reached (default: every 4 frames)
        if(framesSinceLastSend >= _config.traceFrameInterval) {
            // Build batch header (6 bytes)
            ExecTraceBatchMessage batchHeader;
            batchHeader.frameNumber = _currentFrame;
            batchHeader.entryCount = (uint16_t)std::min(
                _traceBuffer.size(), 
                (size_t)_config.maxTracesPerFrame
            );
            
            // Calculate total payload size
            uint32_t totalSize = sizeof(batchHeader) + 
                                 batchHeader.entryCount * sizeof(ExecTraceEntry);
            
            // Build complete payload
            std::vector<uint8_t> payload(totalSize);
            memcpy(payload.data(), &batchHeader, sizeof(batchHeader));
            memcpy(payload.data() + sizeof(batchHeader), 
                   _traceBuffer.data(), 
                   batchHeader.entryCount * sizeof(ExecTraceEntry));
            
            // Queue message
            SendMessage(MessageType::ExecTraceBatch, payload.data(), totalSize);
            
            // Clear sent entries from buffer
            _traceBuffer.erase(_traceBuffer.begin(), 
                              _traceBuffer.begin() + batchHeader.entryCount);
            _lastTraceSentFrame = _currentFrame;
        }
    }
    
    // ══════════════════════════════════════════════════════════
    // SEND CDL UPDATES (if dirty)
    // ══════════════════════════════════════════════════════════
    if(_config.enableCdlUpdates && !_cdlDirty.empty()) {
        // ... (similar batching logic for CDL) ...
    }
    
    // ══════════════════════════════════════════════════════════
    // FLUSH ALL QUEUED MESSAGES TO SOCKET
    // ══════════════════════════════════════════════════════════
    FlushOutgoingMessages();
}
```

**Batch Message Format:**
```
Header (5 bytes):
  Byte 0:      MessageType::ExecTraceBatch (0x03)
  Bytes 1-4:   Payload length (uint32)

Payload:
  Batch Header (6 bytes):
    Bytes 0-3:   frameNumber (uint32)
    Bytes 4-5:   entryCount (uint16)
  
  Entries (15 bytes × entryCount):
    Entry 0: [15 bytes - ExecTraceEntry]
    Entry 1: [15 bytes - ExecTraceEntry]
    ...
    Entry N: [15 bytes - ExecTraceEntry]
```

**Example:** 856 entries per batch
- Header: 5 bytes
- Batch header: 6 bytes
- Entries: 856 × 15 = 12,840 bytes
- **Total: 12,851 bytes per batch**
- Frequency: Every 4 frames = **15 Hz @ 60 FPS**

#### 4.6 Socket Sends Data
**File:** `Utilities/Socket.cpp`

```cpp
void DiztinguishBridge::FlushOutgoingMessages() {
    std::lock_guard<std::mutex> lock(_queueMutex);
    
    while(!_outgoingMessages.empty()) {
        const auto& message = _outgoingMessages.front();
        
        // BufferedSend queues data in socket's send buffer
        _clientSocket->BufferedSend((char*)message.data(), (int)message.size());
        _messagesSent++;
        _bytesSent += message.size();
        
        _outgoingMessages.pop();
    }
    
    // SendBuffer actually transmits buffered data
    _clientSocket->SendBuffer();
}

void Socket::SendBuffer() {
    std::lock_guard<std::mutex> lock(_sendBufferMutex);
    
    if(_sendBuffer.empty()) return;
    
    // Wait for socket to be ready (non-blocking mode)
    fd_set writeSockets;
    writeSockets.fd_count = 1;
    writeSockets.fd_array[0] = _socket;
    
    TIMEVAL timeout;
    timeout.tv_sec = 5;   // 5 second timeout
    timeout.tv_usec = 0;
    
    int ready = select((int)_socket + 1, nullptr, &writeSockets, nullptr, &timeout);
    if(ready <= 0) {
        return;  // Socket not ready or timeout
    }
    
    // Send actual data
    int sent = send(_socket, _sendBuffer.data(), (int)_sendBuffer.size(), 0);
    if(sent > 0) {
        _bytesSent += sent;
        _sendBuffer.clear();
    }
}
```

---

### 5. DIZTINGUISH RECEIVES & PROCESSES

#### 5.1 Client Receives Batch
**File:** `DiztinGUIsh/Diz.Import/src/mesen/tracelog/MesenLiveTraceClient.cs`

```csharp
private async Task ReceiveLoopAsync(CancellationToken cancellationToken) {
    while (!cancellationToken.IsCancellationRequested && IsConnected) {
        // Read 5-byte header
        var headerBytes = await ReadExactBytesAsync(5, cancellationToken);
        var messageType = (MesenMessageType)headerBytes[0];
        var messageLength = BitConverter.ToUInt32(headerBytes, 1);
        
        // Read payload
        var payloadBytes = await ReadExactBytesAsync((int)messageLength, cancellationToken);
        
        // Update stats
        MessagesReceived++;
        BytesReceived += 5 + payloadBytes.Length;
        
        // Dispatch based on type
        ProcessMessage(messageType, payloadBytes);
    }
}

private void ProcessMessage(MesenMessageType messageType, byte[]? payload) {
    switch (messageType) {
        case MesenMessageType.ExecTraceBatch:
            if (payload != null && payload.Length >= 6) {
                ParseAndDispatchExecTraceBatch(payload);
            }
            break;
        // ... other message types ...
    }
}
```

#### 5.2 Parse & Dispatch Batch
**File:** `DiztinGUIsh/Diz.Import/src/mesen/tracelog/MesenLiveTraceClient.cs`

```csharp
private void ParseAndDispatchExecTraceBatch(byte[] data) {
    // Parse batch header
    var frameNumber = BitConverter.ToUInt32(data, 0);    // bytes 0-3
    var entryCount = BitConverter.ToUInt16(data, 4);     // bytes 4-5
    
    MesenConnectionLogger.Log("CLIENT",
        $" *** BATCH RECEIVED: Frame={frameNumber}, Entries={entryCount} ***");
    
    // Verify payload size
    var expectedSize = 6 + (entryCount * 15);
    if (data.Length < expectedSize) {
        return;  // Corrupted batch
    }
    
    // Parse each trace entry
    for (int i = 0; i < entryCount; i++) {
        var offset = 6 + (i * 15);  // Skip header, 15 bytes per entry
        var entryData = new byte[15];
        Array.Copy(data, offset, entryData, 0, 15);
        
        var trace = ParseExecTraceMessage(entryData);
        
        // Fire event for each individual trace
        ExecTraceReceived?.Invoke(this, trace);
    }
}

private static MesenExecTraceMessage ParseExecTraceMessage(byte[] data) {
    return new MesenExecTraceMessage {
        PC = BitConverter.ToUInt32(data, 0) & 0xFFFFFF,  // 24-bit
        Opcode = data[4],
        MFlag = data[5],
        XFlag = data[6],
        DBRegister = data[7],
        DPRegister = BitConverter.ToUInt16(data, 8),
        EffectiveAddr = BitConverter.ToUInt32(data, 10) & 0xFFFFFF
    };
}
```

#### 5.3 Importer Updates SNES Data
**File:** `DiztinGUIsh/Diz.Import/src/mesen/tracelog/MesenTraceLogImporter.cs`

```csharp
private void OnExecTraceReceived(object? sender, MesenExecTraceMessage trace) {
    // Convert SNES address to ROM offset
    var romOffset = ConvertSnesAddressToRomOffset(trace.PC);
    if (romOffset < 0 || romOffset >= _romSizeCached) {
        return;  // Not in ROM (RAM/registers)
    }
    
    lock (_statsLock) {
        _currentStats.NumRomBytesAnalyzed++;
    }
    
    // Update SNES data model
    var updated = UpdateSnesDataFromTrace(romOffset, trace);
    if (updated) {
        lock (_statsLock) {
            _currentStats.NumRomBytesModified++;
        }
    }
    
    // Add trace comment if enabled
    if (Settings.AddTraceComments) {
        var comment = $"TRACE: PC=${trace.PC:X6} OP=${trace.Opcode:X2}";
        _tracelogCommentsGenerated.TryAdd((int)trace.PC, comment);
        
        lock (_statsLock) {
            _currentStats.NumCommentsMarked++;
        }
    }
}

private bool UpdateSnesDataFromTrace(int romOffset, MesenExecTraceMessage trace) {
    if (_snesData?.Data == null) return false;
    
    try {
        var data = _snesData.Data;
        
        // Mark as opcode (executed instruction)
        var flagByte = data.GetFlagByte(romOffset);
        if (flagByte != FlagType.Opcode) {
            data.SetFlagByte(romOffset, FlagType.Opcode);
            
            // Set M/X flags from trace
            var archFlags = (flagByte & 0x30) |  // Preserve other bits
                           (trace.MFlag != 0 ? 0x20 : 0) |  // M flag
                           (trace.XFlag != 0 ? 0x10 : 0);   // X flag
            data.SetArchitecture(romOffset, archFlags);
            
            return true;  // Data modified
        }
        
        return false;  // Already marked
    }
    catch (Exception) {
        return false;
    }
}
```

---

### 6. DATA PERSISTENCE

#### 6.1 Save to .diz File
**Location:** User clicks "Save" in DiztinGUIsh
**File:** `DiztinGUIsh/Diz.Core/serialization/ProjectSerializer.cs`

```csharp
public void SaveProject(string filename) {
    // Serialize SNES data to XML
    var xml = ProjectXmlSerializer.ExportProject(_snesData);
    File.WriteAllText(filename, xml);
}
```

**.diz File Structure:**
```xml
<Project>
  <RomBytes>
    <!-- Each byte's flags -->
    <Byte offset="0x8000">0x41</Byte>  <!-- 0x41 = Opcode + M+X flags -->
    <Byte offset="0x8001">0x01</Byte>  <!-- 0x01 = Operand -->
    ...
  </RomBytes>
  
  <Comments>
    <Comment offset="0x8000">TRACE: PC=$008000 OP=$C2</Comment>
    ...
  </Comments>
  
  <Labels>
    <Label offset="0x8000" name="Reset_Vector"/>
    ...
  </Labels>
</Project>
```

---

## CRITICAL REQUIREMENTS FOR TRACING TO WORK

### ✅ 1. Debugger Must Be Enabled
**Location:** Mesen2 → Tools → Debugger

Without the debugger window open, `ProcessInstruction()` is never called, and no traces are captured.

### ✅ 2. Handshake Must Be Accepted
ROM sizes must match or one side must report size=0 (unknown).

### ✅ 3. Config Must Be Sent
DiztinGUIsh MUST send `ConfigStream` message after handshake, or `_configReceived` flag stays false and all traces are ignored.

### ✅ 4. Emulation Must Be Running
Traces are only captured during actual CPU execution. Paused emulation = no traces.

### ✅ 5. Socket Must Be Non-Blocking with Proper Retry
`BlockingRecv()` uses `select()` to wait for data, preventing the receive thread from breaking on WSAEWOULDBLOCK.

---

## PERFORMANCE CHARACTERISTICS

### Trace Generation Rate
- **SNES CPU:** ~3.58 MHz (3,579,545 Hz)
- **Instructions per frame:** ~59,659 @ 60 FPS
- **Max traces captured:** 1,000 per frame (configurable)
- **Actual traces sent:** ~856-1,000 per batch

### Batch Transmission
- **Frequency:** Every 4 frames = 15 Hz
- **Batch size:** 856 × 15 bytes = 12,840 bytes + 11 byte overhead = **12,851 bytes**
- **Bandwidth:** 12,851 × 15 = **192.8 KB/sec**
- **Messages per second:** 15 batches/sec

### Buffer Management
- **Client buffer:** Pre-allocated 1024 entries
- **Socket buffer:** 256 KB (set in `Socket::SetSocketOptions()`)
- **Send timeout:** 5 seconds (prevents blocking)

---

## TROUBLESHOOTING GUIDE

### No Traces Received

**Check 1:** Is debugger enabled?
```
Mesen2 → Tools → Debugger (window must be open)
```

**Check 2:** Was config sent?
```
Check DiztinGUIsh log for:
"[IMPORTER] *** HANDSHAKE COMPLETE - STREAMING ENABLED ***"
```

**Check 3:** Is emulation running?
```
Mesen2 must be actively playing the game, not paused
```

**Check 4:** Check Mesen2 debugger log:
```
Look for:
"[DiztinGUIsh] *** CONFIG RECEIVED - Trace streaming now enabled ***"
```

### Connection Closes Immediately

**Cause:** Socket send/receive buffer issues

**Fix:** Implemented `BlockingRecv()` with `select()` to wait for data

### Handshake Rejected

**Cause:** ROM size mismatch

**Fix:** Modified `SendHandshake()` to send real ROM size from `cart->DebugGetPrgRomSize()`

---

## SUMMARY

The complete data flow is:

```
CPU.Exec()
  → Emulator.ProcessInstruction<CpuType::Snes>()
    → SnesDebugger.ProcessInstruction()
      → DiztinguishBridge.OnCpuExec()  [buffers entry]
        [60 FPS frame boundary detected]
      → DiztinguishBridge.OnFrameEnd()  [every 4 frames]
        → SendMessage(ExecTraceBatch)  [856-1000 entries]
          → Socket.SendBuffer()  [TCP transmission]

DiztinGUIsh TCP Client
  → ReceiveLoopAsync()  [background thread]
    → ReadExactBytesAsync(5)  [header]
    → ReadExactBytesAsync(payloadSize)  [batch data]
    → ParseAndDispatchExecTraceBatch()
      → ExecTraceReceived event × 856-1000 times
        → MesenTraceLogImporter.OnExecTraceReceived()
          → UpdateSnesDataFromTrace()
            → _snesData.Data.SetFlagByte(romOffset, FlagType.Opcode)
              → [User saves project]
                → .diz file written to disk
```

Every instruction executed by the SNES CPU is captured, batched, transmitted, parsed, and used to update the DiztinGUIsh SNES data model, which can then be saved to disk for persistent analysis.
