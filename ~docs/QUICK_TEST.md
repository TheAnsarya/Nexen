QUICK TEST INSTRUCTIONS
======================

## STEP 1: Start Mesen2 Server

1. Run: `c:\Users\me\source\repos\Mesen2\bin\win-x64\Release\Mesen.exe`
2. Press F9 to open console
3. Type: `emu.startDiztinguishServer(9998)`
4. You should see: "✅ DiztinGUIsh server started on port 9998"

## STEP 2: Test Protocol Fix

```bash
cd c:\Users\me\source\repos\Mesen2\~docs
python test_protocol_fix.py
```

Expected output:
```
✅ Connected successfully!
✅ CORRECT HANDSHAKE RECEIVED!
   Protocol Version: 1.0
   ROM Checksum: 0x00000000
   ROM Size: 0 bytes
   ROM Name: 'Test ROM'
✅ Sent HandshakeAck
✅ Protocol test SUCCESSFUL!
```

## STEP 3: Test Binary Bridge

```bash
python test_binary_bridge.py
```

## STEP 4: Test DiztinGUIsh

1. Open DiztinGUIsh
2. Try the Import > Mesen2 Live Stream option
3. Should connect to localhost:9998 successfully

## TROUBLESHOOTING

**Connection Refused**: Make sure step 1 is completed and server started
**Protocol Mismatch**: Check if Mesen2 has latest code compiled
**No Data**: Load and run a ROM in Mesen2 to generate trace data