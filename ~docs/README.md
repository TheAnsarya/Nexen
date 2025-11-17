# DiztinGUIsh Integration Documentation

This directory contains comprehensive documentation for the Mesen2 ↔ DiztinGUIsh live tracing integration project.

## 📁 Directory Structure

```
~docs/
├── README.md                          # This file - documentation index
├── TESTING.md                         # Testing guide with 7 test scenarios
├── test_mesen_stream.py              # Python test client for protocol validation
├── manual prompts log.txt            # Raw conversation history
├── session_logs/                     # Detailed session summaries
│   ├── 2025-11-17_initial_session.md
│   └── 2025-11-17_continuation_session.md
├── chat_logs/                        # (Reserved for future use)
└── session_summaries/                # (Reserved for future use)
```

## 📚 Documentation Files

### Quick Start
- **[TESTING.md](TESTING.md)** - Start here for testing the integration
  - 3-step setup guide (build → client → server)
  - 7 comprehensive test scenarios
  - Expected outputs and pass criteria
  - Debugging tips

### Session Logs
- **[2025-11-17_initial_session.md](session_logs/2025-11-17_initial_session.md)**
  - Initial documentation and TCP server implementation
  - 4 commits, 3,400+ lines of code/docs
  - Phase 1 infrastructure setup

- **[2025-11-17_continuation_session.md](session_logs/2025-11-17_continuation_session.md)**
  - GitHub issue tracking system
  - CPU execution hooks and CDL tracking
  - Testing infrastructure
  - 5 commits, 1,300+ lines

### Testing Tools
- **[test_mesen_stream.py](test_mesen_stream.py)** - Python protocol test client
  - Connects to Mesen2 server (port 9998)
  - Parses and validates all message types
  - Real-time statistics and bandwidth monitoring
  - Usage: `python test_mesen_stream.py [--port PORT] [--duration SECONDS]`

## 🎯 Project Status

### Current Status: Phase 1 - 75% Complete

**Completed:**
- ✅ GitHub issues (11 issues: 1 epic + 10 tasks)
- ✅ TCP server infrastructure (DiztinguishBridge)
- ✅ Protocol specification (15 message types)
- ✅ CPU execution hooks (M/X flag extraction)
- ✅ Frame lifecycle tracking
- ✅ CDL tracking (differential updates)
- ✅ Testing infrastructure (Python client + guide)
- ✅ Comprehensive documentation

**Pending:**
- ⏳ DiztinGUIsh C# client (Issue #10) - **CRITICAL PATH**
- ⏳ Connection UI in Mesen2 (Issue #9)
- ⏳ CPU state snapshots (Issue #5)
- ⏳ Label synchronization (Issue #6)
- ⏳ Breakpoint control (Issue #7)
- ⏳ Memory dump support (Issue #8)
- ⏳ Integration testing (Issue #11)

### GitHub Issues
See [GitHub Issues](https://github.com/TheAnsarya/Mesen2/issues) for complete tracking:
- **Epic #1:** DiztinGUIsh Live Tracing Integration
- **Issues #2-#11:** Individual implementation tasks (S1-S10)

### Closed Issues
- ✅ #2 (S1): DiztinGUIsh Bridge Infrastructure
- ✅ #3 (S2): Execution Trace Streaming  
- ✅ #4 (S3): Memory Access and CDL Streaming

## 🚀 Quick Start Testing

### Prerequisites
1. Mesen2 compiled with DiztinGUIsh bridge
2. Python 3.7+ installed
3. SNES ROM file for testing

### Test the Server

**Step 1: Build Mesen2**
```bash
msbuild Mesen.sln /p:Configuration=Release /p:Platform=x64
```

**Step 2: Start Test Client**
```bash
cd ~docs
python test_mesen_stream.py
```

**Step 3: Start Mesen2 Server**
1. Launch Mesen2
2. Load any SNES ROM
3. Open debugger (Debug → Debugger)
4. Start server (API TBD - see Issue #9)

**Expected Output:**
```
📡 HANDSHAKE: Protocol v1, Emulator: Mesen2 v0.7.0
🎬 FRAME_START: Frame #1
🔍 EXEC_TRACE #0: $008000 = $78 (M8 X8) DB=$00 D=$0000
📝 CDL_UPDATE: ROM $000000 = Code | M8 | X8 (0x31)
...
```

See [TESTING.md](TESTING.md) for complete testing guide.

## 📖 Key Documentation Locations

### Main Documentation (docs/)
Located in the project's main `docs/` directory:
- **[PROJECT_SUMMARY.md](../docs/PROJECT_SUMMARY.md)** - Repository overview and architecture
- **[diztinguish/README.md](../docs/diztinguish/README.md)** - DiztinGUIsh integration overview
- **[diztinguish/API.md](../docs/diztinguish/API.md)** - Complete API documentation
- **[diztinguish/PROTOCOL.md](../docs/diztinguish/PROTOCOL.md)** - Binary protocol specification

### Source Code
- **Core/SNES/Debugger/DiztinguishBridge.h** - Bridge interface
- **Core/SNES/Debugger/DiztinguishBridge.cpp** - TCP server implementation (850+ lines)
- **Core/SNES/Debugger/DiztinguishProtocol.h** - Message type definitions
- **Core/SNES/Debugger/SnesDebugger.cpp** - Execution hooks integration

## 🔧 Technical Architecture

### Message Flow
```
Mesen2 Emulation
    ↓
ProcessInstruction() [every CPU instruction, ~1.79M/sec]
    ↓
DiztinguishBridge [message queue + worker thread]
    ↓
TCP Socket [port 9998]
    ↓
Client (Python test client OR DiztinGUIsh C#)
```

### Message Types (15 total)
- **0x01** HANDSHAKE - Protocol version and emulator info
- **0x04** EXEC_TRACE - CPU execution (PC, opcode, M/X flags)
- **0x05** CDL_UPDATE - Code/Data Logger changes
- **0x0E/0x0F** FRAME_START/END - Frame boundaries
- **0x02/0x03** CPU_STATE_REQUEST/RESPONSE - Register snapshots
- **0x08-0x0A** LABEL_ADD/UPDATE/DELETE - Label synchronization
- **0x0B-0x0D** BREAKPOINT_ADD/REMOVE/HIT - Breakpoint control
- **0x06/0x07** MEMORY_DUMP_REQUEST/RESPONSE - Memory access
- **0xFF** ERROR - Error messages

See [PROTOCOL.md](../docs/diztinguish/PROTOCOL.md) for complete specification.

## 📊 Performance Characteristics

### Bandwidth Estimates
- **Execution traces:** ~22 MB/sec (13 bytes × 1.79M/sec)
- **CDL updates:** <1 MB/sec (differential, after initial scan)
- **Frame markers:** ~540 bytes/sec (negligible)
- **Total:** 20-30 MB/sec sustained

### Optimizations
- Frame-based batching (every 16.7ms @ 60 FPS)
- Differential CDL updates (only changed addresses)
- 1-second batching for CDL messages
- Thread-safe message queue

## 🐛 Debugging

### Common Issues

**Server not starting:**
- Check port 9998 not in use
- Check firewall allows TCP connections
- Look for "DiztinGUIsh server listening" message

**No messages received:**
- Verify client connected to correct port
- Check emulation is running (not paused)
- Enable verbose logging in bridge

**Incorrect message data:**
- Add hex dump: `print(data.hex())`
- Verify little-endian byte order
- Check message size matches expected

See [TESTING.md](TESTING.md) for complete debugging guide.

## 🎯 Next Steps

### CRITICAL PATH: DiztinGUIsh C# Client (Issue #10)
This is the **only blocker** to end-to-end live tracing.

**Tasks:**
1. Create `Diz.Import.Mesen` project
2. Port protocol to C# (`MesenProtocol.cs`)
3. Implement TCP client (`MesenLiveTraceClient.cs`)
4. Integrate with DiztinGUIsh data model
5. Add connection UI
6. Test end-to-end

**After that:**
- Connection UI in Mesen2 (Issue #9)
- CPU state snapshots (Issue #5)
- Label sync, breakpoints, memory dumps (Issues #6-#8)
- Integration testing and documentation (Issue #11)

## 📝 Contributing

When working on this project:

1. **Update session logs:** Add entries to `session_logs/` directory
2. **Update GitHub issues:** Close issues as work completes
3. **Test with Python client:** Validate protocol changes
4. **Update documentation:** Keep API.md and PROTOCOL.md current
5. **Follow commit conventions:**
   - `feat:` for new features
   - `fix:` for bug fixes
   - `docs:` for documentation
   - `test:` for testing infrastructure
   - `build:` for build system changes

## 📞 Questions?

- Create GitHub issue with `diztinguish` label
- Check existing issues: https://github.com/TheAnsarya/Mesen2/issues
- Review session logs for context and decisions

## 📄 License

This integration follows the Mesen2 project license (see LICENSE in repository root).

---

**Last Updated:** 2025-11-17  
**Status:** Phase 1 - 75% Complete  
**Next Milestone:** DiztinGUIsh C# Client Implementation
