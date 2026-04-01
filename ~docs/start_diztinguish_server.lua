-- Start DiztinGUIsh server
local port = 9998
local success = emu.startDiztinguishServer(port)

if success then
    emu.log("✅ DiztinGUIsh server started on port " .. port)
else
    emu.log("❌ Failed to start DiztinGUIsh server on port " .. port)
end