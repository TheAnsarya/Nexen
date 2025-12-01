# Connection Lifetime Testing Guide

## Purpose
Track why the TCP connection between DiztinGUIsh and Mesen2 is closing/destructing.
The connection MUST stay alive for data to flow continuously.

## Setup

### 1. Start Mesen2
- Launch Mesen2
- Load a ROM
- Open Script Window (Debug → Script Window)
- Run: `start_diztinguish_server.lua` 
- Verify log shows: "✅ DiztinGUIsh server started on port 9998"

### 2. Start Connection Monitor (Terminal 1)
```powershell
cd C:\Users\me\source\repos\Mesen2\~docs
.\monitor_connection_live.ps1
```
This will show real-time connection status.

### 3. Start DiztinGUIsh with Debug Output (Terminal 2)
```powershell
cd C:\Users\me\source\repos\DiztinGUIsh
$env:DOTNET_LOGGING=Debug
.\Diz.App.Winforms\bin\Debug\net9.0-windows\Diz.App.Winforms.exe 2>&1 | Tee-Object -FilePath "C:\Users\me\source\repos\Mesen2\~docs\diz_debug.log"
```

### 4. Open DebugView (Optional but Recommended)
- Download Sysinternals DebugView if not installed
- Run as Administrator
- This captures System.Diagnostics.Debug.WriteLine output
- Look for `[DiztinGUIsh]` messages

## Testing Steps

1. **In DiztinGUIsh**: Create or open a project
2. **Press Ctrl+F6** or menu → Connect to Mesen2
3. **Watch the connection monitor** - should show "✅ CONNECTION ESTABLISHED"
4. **Watch for debug output**:
   - `*** CONNECTION ESTABLISHED: localhost:9998 ***`
   - `*** RECEIVE LOOP STARTED ***`
   - `Handshake accepted by DiztinGUIsh v2.0`
   - `*** CONFIG RECEIVED - Trace streaming now enabled ***`
5. **In Mesen2**: Start playing (unpause if paused)
6. **Watch for trace messages**:
   - `*** BATCH RECEIVED: Frame=X, Entries=Y ***`
   - `*** TRACE RECEIVED: PC=$XXXXXX, Opcode=$XX ***`

## What to Look For

### Connection Staying Alive (GOOD)
```
[19:45:01.234] ✅ CONNECTION ESTABLISHED
[19:45:01.235] 🟢 Connected for 00:00:00
[19:45:02.235] 🟢 Connected for 00:00:01
[19:45:03.235] 🟢 Connected for 00:00:02
... continues ...
```

### Connection Closing (BAD)
```
[19:45:01.234] ✅ CONNECTION ESTABLISHED
[19:45:01.500] ❌ CONNECTION LOST after 0.3s
```

If connection closes, check DebugView for:
- `*** DISCONNECT CALLED ***` - shows what triggered disconnect
- `*** CONNECTION CLOSED: ReadAsync returned 0 bytes ***` - peer disconnected
- `*** RECEIVE LOOP ERROR: ***` - exception occurred
- Stack traces showing WHO called Disconnect()

## Expected Debug Output (Working Connection)

```
[DiztinGUIsh] *** CONNECTION ESTABLISHED: localhost:9998 ***
[DiztinGUIsh] *** RECEIVE LOOP STARTED ***
[DiztinGUIsh] Handshake accepted by DiztinGUIsh v2.0
[DiztinGUIsh] Sending config: ExecTrace=True, CDL=True, Interval=4
[DiztinGUIsh] Config sent successfully
[DiztinGUIsh] *** BATCH RECEIVED: Frame=240, Entries=856 ***
[DiztinGUIsh] *** TRACE RECEIVED: PC=$008012, Opcode=$A9 ***
[DiztinGUIsh] *** TRACE RECEIVED: PC=$008014, Opcode=$8D ***
... continues rapidly ...
```

## Common Issues

### Issue 1: Connection closes immediately
- **Symptom**: Connection lasts <1 second
- **Cause**: Exception in receive loop or handshake rejection
- **Fix**: Check DebugView for exception details

### Issue 2: Connection established but no data
- **Symptom**: Connected for minutes, no BATCH RECEIVED messages
- **Cause**: Config not sent or Mesen2 not sending data
- **Fix**: 
  - Check Mesen2 is UNPAUSED
  - Verify in Mesen2 log: "Sent ExecTraceBatch"
  - Check config was sent: "Config sent successfully"

### Issue 3: Data flows briefly then stops
- **Symptom**: BATCH RECEIVED messages then silence
- **Cause**: Exception during message processing
- **Fix**: Check DebugView for "RECEIVE LOOP ERROR"

## Files Generated
- `diz_debug.log` - Full console output from DiztinGUIsh
- Monitor window - Real-time connection status
- DebugView - System-level debug messages

## Next Steps After Testing
Once you identify WHERE/WHY the connection closes, report:
1. How long connection lasted
2. What debug messages appeared before disconnect
3. Stack trace from DISCONNECT CALLED (if any)
4. Any exceptions in RECEIVE LOOP ERROR
