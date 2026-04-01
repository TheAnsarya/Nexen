# Mesen2 DiztinGUIsh Server Setup Instructions

## 🚀 Quick Start Guide

### Step 1: Start Mesen2
1. **Launch Mesen2** emulator
2. **Load a ROM** (any SNES game)
3. **Start emulation** (make sure it's not paused)

### Step 2: Enable DiztinGUIsh Server
In Mesen2, open the **Lua Console** and run:
```lua
-- Start DiztinGUIsh server on port 9998
emu.startDiztinguishServer(9998)
```

You should see a message like:
```
DiztinGUIsh server started on port 9998
```

### Step 3: Connect from DiztinGUIsh
1. **Open DiztinGUIsh**
2. **Load/Create a project**
3. Go to **Tools → Import → Mesen2 Live Trace**
4. Connect to **localhost:9998**
5. **Start streaming!**

## 🔧 Troubleshooting

### "Connection Refused" Error
**Cause:** Mesen2 server is not running or not accepting connections

**Solutions:**
- ✅ Make sure Mesen2 is running
- ✅ Load a ROM in Mesen2  
- ✅ Run `emu.startDiztinguishServer(9998)` in Lua console
- ✅ Check that emulation is not paused
- ✅ Verify port 9998 is not blocked by firewall

### "Connection Successful but No Data" 
**Cause:** Server is running but not streaming execution traces

**Solutions:**
- ✅ Make sure a ROM is loaded and **actively running** (not paused)
- ✅ The game should be executing code for traces to be generated
- ✅ Try stepping through the game or letting it run

### "Connection Drops Immediately"
**Cause:** Usually a setup issue or server configuration problem

**Solutions:**
- ✅ Make sure Mesen2 DiztinGUIsh server is properly started
- ✅ Check that no other application is using port 9998
- ✅ Try restarting the server: `emu.stopDiztinguishServer()` then `emu.startDiztinguishServer(9998)`

## 🧪 Testing Your Setup

Run this quick test to verify your Mesen2 server is working:

### Windows PowerShell:
```powershell
cd "path\to\Mesen2\~docs"
.\test_connection_lifecycle.ps1
```

### Python:
```bash
python test_connection_lifecycle.py
```

A successful test will show:
```
✅ Streaming is working! Received X data messages
🎉 Connection test PASSED
```

## 📝 Mesen2 Lua Commands

```lua
-- Start server
emu.startDiztinguishServer(9998)

-- Stop server
emu.stopDiztinguishServer()

-- Check server status
emu.getDiztinguishServerStatus()
```

## ⚡ Pro Tips

1. **Keep Mesen2 running** - The server stops when Mesen2 closes
2. **Use consistent ports** - Default 9998 works well
3. **Load ROM first** - Server needs active emulation to stream data
4. **Check firewall** - Windows may block the connection initially
5. **Test connection** - Use the provided test scripts to verify setup

## 🐛 Still Having Issues?

If you've followed all steps and still can't connect:

1. **Check Mesen2 version** - Make sure you have DiztinGUIsh support
2. **Try different port** - `emu.startDiztinguishServer(9999)` 
3. **Restart Mesen2** - Sometimes a fresh start helps
4. **Check Windows firewall** - Allow Mesen2 through firewall
5. **Run test script** - Use the connection test to diagnose the problem