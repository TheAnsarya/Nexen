# DiztinGUIsh ↔ Mesen2 Connection Diagnostic Checklist

## 🔍 How to View Diagnostic Logs

### Mesen2 Logs

**Option 1: Debug Log Window (RECOMMENDED)**
1. In Mesen2, go to **Tools** → **Debug Log** (or press the keyboard shortcut)
2. This window shows all `_debugger->Log()` output in real-time
3. Look for lines starting with `[DiztinGUIsh]`

**Option 2: Console Output**
If you launched Mesen2 from a terminal:
```powershell
cd c:\Users\me\source\repos\Mesen2\bin\win-x64\Release
.\Mesen.exe
```
All logs will appear in the console window.

### DiztinGUIsh Logs

**Debug Output Window (if running from Visual Studio):**
- View → Output → Select "Debug" from dropdown
- Look for lines starting with `[DiztinGUIsh]`

**OR run from PowerShell to see console output:**
```powershell
cd c:\Users\me\source\repos\DiztinGUIsh\Diz.App.Winforms\bin\Release\net9.0-windows
.\Diz.App.Winforms.exe
```

---

## ✅ Connection Diagnostic Steps

### Step 1: Verify Server Started
**In Mesen2 Debug Log, you should see:**
```
[DiztinGUIsh] Created socket, attempting to bind to port 9998...
[DiztinGUIsh] Socket bound successfully, calling Listen(1)...
[DiztinGUIsh] Socket listening, setting server running flag...
[DiztinGUIsh] Starting server thread...
[DiztinGUIsh] Server started successfully on port 9998 - waiting for connections
[DiztinGUIsh] Enabled SNES debugger for streaming
```

✅ **If you see these:** Server is running correctly  
❌ **If missing:** Server failed to start - check for port conflicts

---

### Step 2: Verify Client Connected
**In Mesen2 Debug Log, you should see:**
```
[DiztinGUIsh] Calling Accept() - this will block until client connects...
[DiztinGUIsh] Accept() returned, checking socket status...
[DiztinGUIsh] Client connected successfully! Sending handshake...
[DiztinGUIsh] Handshake sent, starting receive thread...
[DiztinGUIsh] Receive thread started, waiting for it to complete...
```

✅ **If you see these:** DiztinGUIsh successfully connected  
❌ **If stuck at "Calling Accept()"**: DiztinGUIsh hasn't connected yet

---

### Step 3: Verify Handshake Completed
**In Mesen2 Debug Log, you should see:**
```
[DiztinGUIsh] Handshake accepted by DiztinGUIsh Client
```

✅ **If you see this:** Handshake successful  
❌ **If you see "Handshake rejected"**: Connection refused by client

---

### Step 4: **CRITICAL** - Verify Config Received
**In Mesen2 Debug Log, you MUST see:**
```
[DiztinGUIsh] *** CONFIG RECEIVED - Trace streaming now enabled ***
[DiztinGUIsh] Configuration received:
[DiztinGUIsh]   ExecTrace: ON
[DiztinGUIsh]   MemoryAccess: OFF
[DiztinGUIsh]   CDL Updates: ON
[DiztinGUIsh]   Trace Interval: 4 frames
[DiztinGUIsh]   Max Traces/Frame: 1000
```

✅ **If you see "CONFIG RECEIVED"**: DiztinGUIsh sent config, traces can now flow  
❌ **If MISSING**: **THIS IS YOUR PROBLEM** - DiztinGUIsh never sent the config message!

**Without this message:**
- `_configReceived` flag stays `false`
- `OnFrameEnd()` returns early (line 410: `if(!_clientConnected || !_configReceived)`)
- `OnCpuExec()` returns early (line 471: `if(!_clientConnected || !_configReceived || !_config.enableExecTrace)`)
- **NO TRACES ARE BUFFERED OR SENT**

---

### Step 5: Verify Game is Running
**Check that:**
1. A ROM is loaded in Mesen2
2. Emulation is **NOT PAUSED** (press F5 to resume if paused)
3. Frames are advancing (watch the frame counter in status bar)

❌ **If paused:** `OnFrameEnd()` won't be called, so traces won't be sent even if buffered

---

### Step 6: Verify Traces Being Sent (Every 4 Frames)
**In Mesen2 Debug Log, you should see (repeatedly):**
```
[DiztinGUIsh] Sent ExecTraceBatch: frame=120, entries=856
[DiztinGUIsh] Sent ExecTraceBatch: frame=124, entries=892
[DiztinGUIsh] Sent ExecTraceBatch: frame=128, entries=901
```

✅ **If you see these every ~67ms (4 frames at 60 FPS)**: Traces are being sent!  
❌ **If missing but config received**: Either game is paused or `_traceBuffer` is empty

---

### Step 7: Verify Traces Being Received
**In DiztinGUIsh Debug Output, you should see:**
```
[DiztinGUIsh] *** TRACE RECEIVED: PC=$808FE2, Opcode=$AD ***
[DiztinGUIsh] *** TRACE RECEIVED: PC=$808FE5, Opcode=$C9 ***
[DiztinGUIsh] *** TRACE RECEIVED: PC=$808FE7, Opcode=$F0 ***
```

✅ **If you see these:** Traces are flowing end-to-end!  
❌ **If missing but Mesen2 sent traces**: Network issue or DiztinGUIsh receive loop broken

---

## 🐛 Common Issues and Solutions

### Issue: "CONFIG RECEIVED" Never Appears

**Root Cause:** DiztinGUIsh never sent the `ConfigStreamMessage`

**Check in DiztinGUIsh code:**
```csharp
// MesenLiveTraceClient.cs - after handshake, should send config
await SendConfigAsync();
```

**Solution:** Verify `SendConfigAsync()` is being called after handshake

---

### Issue: Traces Sent But Not Received

**Root Cause:** DiztinGUIsh receive loop not processing messages

**Check:**
1. Is `ReceiveLoopAsync()` running?
2. Are exceptions being swallowed?
3. Is the stream still connected?

**Add logging to `ReceiveLoopAsync()` in DiztinGUIsh**

---

### Issue: Connection Closes Immediately

**Root Cause:** Already fixed! Was caused by `using` statement disposing importer

**Verify:** Check that `_activeMesenImporter` is stored in private field, not disposed

---

### Issue: No Traces in Buffer

**Root Cause:** `OnCpuExec()` not being called

**Check:**
1. `SetDebuggerFlag(SnesDebuggerEnabled, true)` was called (should see in log at startup)
2. ROM is actually executing code
3. Not in debugger breakpoint

---

## 📊 Expected Log Sequence (Full Successful Connection)

### Mesen2 Side:
```
[DiztinGUIsh] Created socket, attempting to bind to port 9998...
[DiztinGUIsh] Socket bound successfully, calling Listen(1)...
[DiztinGUIsh] Server started successfully on port 9998 - waiting for connections
[DiztinGUIsh] Enabled SNES debugger for streaming
[DiztinGUIsh] Calling Accept() - this will block until client connects...
[DiztinGUIsh] Accept() returned, checking socket status...
[DiztinGUIsh] Client connected successfully! Sending handshake...
[DiztinGUIsh] Handshake sent, starting receive thread...
[DiztinGUIsh] Handshake accepted by DiztinGUIsh Client
[DiztinGUIsh] *** CONFIG RECEIVED - Trace streaming now enabled ***  ← CRITICAL!
[DiztinGUIsh] Configuration received:
[DiztinGUIsh]   ExecTrace: ON
[DiztinGUIsh]   Trace Interval: 4 frames
[DiztinGUIsh] Sent ExecTraceBatch: frame=4, entries=856
[DiztinGUIsh] Sent ExecTraceBatch: frame=8, entries=892
[DiztinGUIsh] Sent ExecTraceBatch: frame=12, entries=901
```

### DiztinGUIsh Side:
```
[DiztinGUIsh] *** TRACE RECEIVED: PC=$808FE2, Opcode=$AD ***
[DiztinGUIsh] *** TRACE RECEIVED: PC=$808FE5, Opcode=$C9 ***
[DiztinGUIsh] *** TRACE RECEIVED: PC=$808FE7, Opcode=$F0 ***
```

---

## 🎯 What to Check NOW

1. **Open Mesen2 Debug Log** (Tools → Debug Log)
2. **Scroll to find connection logs**
3. **Check if you see "*** CONFIG RECEIVED ***"**
   - ✅ **YES**: Config received, check if game is running (Step 5)
   - ❌ **NO**: **DiztinGUIsh never sent config** - need to fix `SendConfigAsync()`

4. **If config received, check for "Sent ExecTraceBatch"**
   - ✅ **YES**: Traces being sent, check DiztinGUIsh receive side
   - ❌ **NO**: Game is paused or `_traceBuffer` empty

5. **Report what you see** - copy relevant log lines and we'll diagnose next step!
