-- Comprehensive DiztinGUIsh server diagnostic

local port = 9998

emu.log("========================================")
emu.log("DiztinGUIsh Server Diagnostic")
emu.log("========================================")

-- Test 1: Check if server is already running
local isRunning = emu.isDiztinguishServerRunning()
emu.log("1. Server already running: " .. tostring(isRunning))

if isRunning then
    emu.log("   Server is already active - stopping it first...")
    emu.stopDiztinguishServer()
    emu.sleep(1000)  -- Wait 1 second
end

-- Test 2: Start the server
emu.log("2. Starting server on port " .. port .. "...")
local success = emu.startDiztinguishServer(port)

if success then
    emu.log("   ✅ StartServer returned TRUE")
    
    -- Wait a moment for server to fully initialize
    emu.sleep(500)
    
    -- Test 3: Verify it's actually running
    local nowRunning = emu.isDiztinguishServerRunning()
    emu.log("3. Server running after start: " .. tostring(nowRunning))
    
    if nowRunning then
        -- Test 4: Get server info
        local serverPort = emu.getDiztinguishServerPort()
        emu.log("4. Server port: " .. serverPort)
        
        local clientConnected = emu.isDiztinguishClientConnected()
        emu.log("5. Client connected: " .. tostring(clientConnected))
        
        emu.log("")
        emu.log("✅ SERVER IS READY!")
        emu.log("   Now connect from DiztinGUIsh:")
        emu.log("   Tools → Import → Import Mesen2 Live Trace...")
        emu.log("   Host: localhost, Port: " .. serverPort)
        emu.log("")
        emu.log("   Leave this script running - it keeps the server active.")
        emu.log("   Server will continue streaming data while emulation runs.")
    else
        emu.log("")
        emu.log("❌ ERROR: StartServer returned true but server is not running!")
        emu.log("   This indicates an internal Mesen2 issue.")
        emu.log("   Check if port " .. port .. " is already in use by another program.")
    end
else
    emu.log("   ❌ StartServer returned FALSE")
    emu.log("")
    emu.log("POSSIBLE CAUSES:")
    emu.log("1. Port " .. port .. " is already in use by another program")
    emu.log("2. No SNES ROM is loaded")
    emu.log("3. Emulation is not running")
    emu.log("")
    emu.log("SOLUTIONS:")
    emu.log("1. Make sure Mesen2 is running a SNES game")
    emu.log("2. Try a different port (e.g., change port = 9999 at top of script)")
    emu.log("3. Close any other program using port " .. port)
end

emu.log("========================================")
