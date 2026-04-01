-- Test script for DiztinGUIsh streaming server
-- Usage: Load this in Mesen2's Lua script window

local port = 9998  -- Default DiztinGUIsh port

-- Function to print server status
function checkServerStatus()
    local status = emu.getDiztinguishServerStatus()
    emu.log("=== DiztinGUIsh Server Status ===")
    emu.log("Running: " .. tostring(status.running))
    emu.log("Port: " .. status.port)
    emu.log("Client Connected: " .. tostring(status.clientConnected))
    emu.log("================================")
end

-- Start the server
emu.log("Starting DiztinGUIsh server on port " .. port .. "...")
local success = emu.startDiztinguishServer(port)

if success then
    emu.log("✅ Server started successfully!")
    checkServerStatus()
    
    emu.log("")
    emu.log("Connect to server with:")
    emu.log("  - Python test client: python test_mesen_stream.py")
    emu.log("  - DiztinGUIsh: (not yet implemented)")
    emu.log("")
    emu.log("To stop server, run: emu.stopDiztinguishServer()")
else
    emu.log("❌ Failed to start server")
    emu.log("Possible reasons:")
    emu.log("  - Port " .. port .. " already in use")
    emu.log("  - Not running SNES console")
    emu.log("  - Firewall blocking connections")
end

-- Optional: Add frame callback to show client status
local frameCount = 0
local lastClientStatus = false

emu.addEventCallback(function()
    frameCount = frameCount + 1
    
    -- Check every 60 frames (once per second @ 60 FPS)
    if frameCount % 60 == 0 then
        local status = emu.getDiztinguishServerStatus()
        
        -- Log when client connects/disconnects
        if status.clientConnected ~= lastClientStatus then
            if status.clientConnected then
                emu.log("📡 Client connected!")
            else
                emu.log("📡 Client disconnected")
            end
            lastClientStatus = status.clientConnected
        end
    end
end, emu.eventType.endFrame)

emu.log("Script loaded. Server running in background.")
