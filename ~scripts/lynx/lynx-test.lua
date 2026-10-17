-- Lynx test protocol handler for Nexen --testRunner.
-- Polls Lynx CPU memory for test results from custom test ROMs.
-- Tests write pass/fail status to zero page $00.
--
-- Exit codes:
--   0 = pass
--   1 = fail
--   2 = timeout
--  90 = wrong console type

local MAX_FRAMES = 1800   -- 30 seconds at ~60fps
local POLL_INTERVAL = 60  -- check every 60 frames (~1 second)
local RESULT_ADDR = 0x0000 -- Zero page result location

local function onFrameEnd()
	local state = emu.getState()
	local consoleType = state["consoleType"]
	if consoleType ~= "Lynx" then
		emu.log("lynx-test: wrong console type: " .. tostring(consoleType))
		emu.stop(90)
		return
	end

	local frameCount = state["frameCount"] or 0

	if (frameCount % POLL_INTERVAL) == 0 and frameCount > 0 then
		local result = emu.read(RESULT_ADDR, emu.memType.lynxMemory)

		if result ~= 0x80 then
			if result == 0x00 then
				emu.log("lynx-test: PASS at frame " .. tostring(frameCount))
				emu.stop(0)
			else
				emu.log("lynx-test: FAIL (code $" .. string.format("%02x", result) .. ") at frame " .. tostring(frameCount))
				emu.stop(1)
			end
			return
		end
	end

	if frameCount >= MAX_FRAMES then
		emu.log("lynx-test: TIMEOUT at frame " .. tostring(frameCount))
		emu.stop(2)
	end
end

emu.addEventCallback(onFrameEnd, emu.eventType.endFrame)
emu.log("lynx-test: started, polling $" .. string.format("%04x", RESULT_ADDR) .. " every " .. tostring(POLL_INTERVAL) .. " frames, max " .. tostring(MAX_FRAMES))
