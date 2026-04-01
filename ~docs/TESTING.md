# DiztinGUIsh Streaming - Testing Guide

## Quick Start Testing

### Prerequisites
- Mesen2 compiled with DiztinGUIsh bridge
- Python 3.7+ (for test client)
- SNES ROM file for testing

### Step 1: Build Mesen2
```bash
# Build in Visual Studio or via command line
msbuild Mesen.sln /p:Configuration=Release /p:Platform=x64
```

### Step 2: Start Test Client
```bash
# Navigate to test directory
cd ~docs

# Run test client (waits for server)
python test_mesen_stream.py
```

Keep this running in a separate terminal.

### Step 3: Start Mesen2 and Enable Server
1. Launch Mesen2
2. Open any SNES ROM
3. Open debugger window (Debug → Debugger)
4. Load Lua script:
   - Debug → Script Window
   - File → Open → `~docs/test_server.lua`
   - Click "Run"
   
   **OR** use Lua console directly:
   ```lua
   emu.startDiztinguishServer(9998)
   ```

The test_server.lua script will:
- Start server on port 9998
- Display connection status
- Monitor for client connections
- Log statistics every second

### Expected Output

**Test Client Terminal:**
```
======================================================================
Mesen2 DiztinGUIsh Streaming Test Client
======================================================================

Connecting to 127.0.0.1:9998...
✅ Connected successfully

======================================================================
📡 Listening for messages... (Ctrl+C to stop)
======================================================================

📡 HANDSHAKE: Protocol v1, Emulator: Mesen2 v0.7.0
🎬 FRAME_START: Frame #1
🔍 EXEC_TRACE #0: $008000 = $78 (M8 X8) DB=$00 D=$0000
🔍 EXEC_TRACE #1: $008001 = $18 (M8 X8) DB=$00 D=$0000
📝 CDL_UPDATE: ROM $000000 = Code | M8 | X8 (0x31)
📝 CDL_UPDATE: ROM $000001 = Code | M8 | X8 (0x31)
...
🎬 FRAME_END
🎬 FRAME_START: Frame #2
...
```

## Test Scenarios

### Test 1: Connection Handshake
**Goal:** Verify server sends handshake on connect

**Steps:**
1. Start test client first
2. Start Mesen2 server
3. Check for HANDSHAKE message

**Expected:**
- Protocol version = 1
- Emulator name = "Mesen2 v0.7.0" (or current version)

**Pass Criteria:**
- ✅ Handshake received within 1 second
- ✅ Protocol version matches expected
- ✅ Emulator name is valid string

---

### Test 2: Execution Trace Capture
**Goal:** Verify CPU execution traces stream correctly

**Steps:**
1. Connect test client
2. Load ROM in Mesen2
3. Run emulation (press F5)
4. Observe EXEC_TRACE messages

**Expected:**
- ~1.79 million traces per second (NTSC SNES @ ~1.79 MHz)
- PC values in valid ROM range
- M/X flags change when code executes REP/SEP
- DB register matches ROM bank

**Pass Criteria:**
- ✅ Continuous stream of traces while running
- ✅ No traces when paused
- ✅ PC values make sense (sequential most of time)
- ✅ M/X flags reflect 8-bit vs 16-bit modes

---

### Test 3: CDL Tracking
**Goal:** Verify CDL updates stream as code executes

**Steps:**
1. Connect test client
2. Load ROM with no existing CDL data
3. Run emulation
4. Observe CDL_UPDATE messages

**Expected:**
- CDL updates for each newly executed address
- Flags include Code (0x01) + M8/X8 flags
- ROM offsets match executed PC values

**Pass Criteria:**
- ✅ CDL updates received
- ✅ Flags include Code bit (0x01)
- ✅ M8/X8 flags match execution context
- ✅ No duplicate updates for same address (differential updates)

---

### Test 4: Frame Lifecycle
**Goal:** Verify frame start/end markers

**Steps:**
1. Connect test client
2. Run emulation
3. Count FRAME_START and FRAME_END messages

**Expected:**
- FRAME_START/END pairs every ~16.7ms (NTSC 60 Hz)
- Frame numbers increment sequentially
- No missing frames

**Pass Criteria:**
- ✅ Frame rate ~60 FPS (NTSC) or ~50 FPS (PAL)
- ✅ Every FRAME_START has matching FRAME_END
- ✅ Frame numbers never skip

---

### Test 5: Bandwidth and Performance
**Goal:** Measure streaming overhead

**Steps:**
1. Run test client with duration limit:
   ```bash
   python test_mesen_stream.py --duration 60
   ```
2. Run Mesen2 for 60 seconds
3. Check statistics output

**Expected Bandwidth:**
- Execution traces: ~13 bytes × 1.79M/sec ≈ **22 MB/sec**
- CDL updates: ~5 bytes × (new addrs/sec) ≈ **variable**
- Frame markers: ~9 bytes × 60/sec ≈ **540 bytes/sec**

**Pass Criteria:**
- ✅ Bandwidth < 50 MB/sec (with batching/compression)
- ✅ No emulation slowdown (still 60 FPS)
- ✅ No memory leaks in long runs

---

### Test 6: Disconnect and Reconnect
**Goal:** Verify robust connection handling

**Steps:**
1. Connect test client
2. Run emulation for 10 seconds
3. Kill test client (Ctrl+C)
4. Restart test client
5. Verify reconnection

**Expected:**
- Server handles disconnect gracefully
- Client can reconnect
- New handshake sent on reconnect

**Pass Criteria:**
- ✅ No crash on disconnect
- ✅ Reconnect succeeds
- ✅ Streaming resumes after reconnect

---

### Test 7: Multiple ROMs
**Goal:** Test with different ROM types

**ROMs to Test:**
- LoROM (e.g., Super Mario World)
- HiROM (e.g., Final Fantasy VI)
- SA-1 (e.g., Super Mario RPG)
- SuperFX (e.g., Star Fox)

**Pass Criteria:**
- ✅ Correct PC addresses for each mapping
- ✅ CDL ROM offsets calculated correctly
- ✅ No crashes with special chips

---

## Manual Testing Checklist

### Server Control
- [ ] Server starts on specified port
- [ ] Port conflicts detected and reported
- [ ] Server stops cleanly
- [ ] Configuration changes applied

### Message Correctness
- [ ] Handshake protocol version correct
- [ ] EXEC_TRACE PC values valid
- [ ] EXEC_TRACE M/X flags accurate
- [ ] CDL_UPDATE flags include Code bit
- [ ] CDL_UPDATE offsets match PC

### Frame Synchronization
- [ ] FRAME_START/END pairs balanced
- [ ] Frame numbers sequential
- [ ] Frame timing correct (~60 Hz)

### Performance
- [ ] No emulation slowdown
- [ ] Bandwidth acceptable
- [ ] Memory usage stable
- [ ] CPU usage reasonable

### Error Handling
- [ ] Disconnect handled gracefully
- [ ] Invalid messages logged
- [ ] Reconnect works
- [ ] No memory leaks

---

## Automated Test Suite (Future)

### Unit Tests (C++)
```cpp
// Core/SNES/Debugger/DiztinguishBridgeTests.cpp
TEST(DiztinguishBridge, HandshakeMessage) {
    DiztinguishBridge bridge(9998);
    ASSERT_TRUE(bridge.Start());
    // ... test handshake format
}

TEST(DiztinguishBridge, ExecTraceFormat) {
    // ... test EXEC_TRACE message construction
}
```

### Integration Tests (Python)
```python
# tests/test_streaming_integration.py
def test_handshake():
    client = MesenStreamClient()
    client.connect()
    msg = client.wait_for_message(MessageType.HANDSHAKE, timeout=5)
    assert msg.protocol_version == 1
```

---

## Debugging Tips

### No Messages Received
1. Check server started: Look for "DiztinGUIsh server listening on port 9998"
2. Check firewall: Windows may block port 9998
3. Check port conflict: Try different port with `--port 9999`
4. Enable verbose logging in Mesen2

### Incorrect Message Data
1. Add hex dump to test client:
   ```python
   print(f"Raw data: {data.hex()}")
   ```
2. Compare with DiztinguishProtocol.h message format
3. Check endianness (should be little-endian)

### Performance Issues
1. Enable batching in DiztinguishBridge (already default)
2. Reduce message frequency (e.g., every 10th trace)
3. Add compression (zlib/lz4)
4. Profile with Visual Studio profiler

### Memory Leaks
1. Run Mesen2 with Valgrind (Linux) or Dr. Memory (Windows)
2. Check message queue growth in DiztinguishBridge
3. Verify thread cleanup on disconnect

---

## Next Steps After Testing

Once basic streaming works:

1. **[Issue #5]** Implement CPU state snapshots
2. **[Issue #9]** Add connection UI in Mesen2
3. **[Issue #10]** Build DiztinGUIsh C# client
4. **[Issue #6-8]** Add label sync, breakpoints, memory dumps
5. **[Issue #11]** Create comprehensive test suite

---

## Test Results Template

```markdown
## Test Session: [Date]

### Environment
- Mesen2 Version: [version]
- OS: [Windows/Linux/macOS]
- ROM: [name, type]
- Test Duration: [seconds]

### Results
- [x] Test 1: Connection Handshake - PASS
- [x] Test 2: Execution Traces - PASS
- [ ] Test 3: CDL Tracking - FAIL (reason)
- ...

### Statistics
- Bandwidth: 18.5 MB/sec
- Traces/sec: 1,789,000
- CDL Updates: 45,234 total
- Frames: 3,600 (60 FPS sustained)

### Issues Found
1. Issue: CDL updates sometimes duplicated
   - Severity: Medium
   - Action: Add deduplication in OnCdlChanged()

### Notes
- Performance excellent, no slowdown
- Memory stable over 10-minute run
- Ready for C# client implementation
```

---

## Contact

Questions or issues? Create GitHub issue with:
- Test scenario that failed
- Expected vs actual output
- Mesen2 version and OS
- ROM type (LoROM/HiROM/etc.)
