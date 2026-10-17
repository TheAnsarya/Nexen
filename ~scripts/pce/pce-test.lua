-- PCE (TurboGrafx-16/PC Engine) test protocol handler for Nexen --testRunner.
-- Polls PCE memory for test results from PeterLemon/PCE test ROMs.
-- Tests write pass/fail status to zero-page RAM.
--
-- Exit codes:
--   0 = pass
--   1 = fail
--   2 = timeout
--  90 = wrong console type

local MAX_FRAMES = 36000  -- 10 minutes at ~60fps
local POLL_INTERVAL = 60  -- check every 60 frames (1 second)
local RESULT_ADDR = 0x2000 -- RAM result location (MPR2 bank, typically RAM)

local function onFrameEnd()
	local state = emu.getState()
	local consoleType = state["consoleType"]
	if consoleType ~= "PcEngine" then
		emu.log("pce-test: wrong console type: " .. tostring(consoleType))
		emu.stop(90)
		return
	end

	local frameCount = state["frameCount"] or 0

	if (frameCount % POLL_INTERVAL) == 0 and frameCount > 0 then
		local result = emu.read(RESULT_ADDR, emu.memType.pceMemory)

		if result ~= 0x80 then
			if result == 0x00 then
				emu.log("pce-test: PASS at frame " .. tostring(frameCount))
				emu.stop(0)
			else
				emu.log("pce-test: FAIL (code $" .. string.format("%02x", result) .. ") at frame " .. tostring(frameCount))
				emu.stop(1)
			end
			return
		end
	end

	if frameCount >= MAX_FRAMES then
		emu.log("pce-test: TIMEOUT at frame " .. tostring(frameCount))
		emu.stop(2)
	end
end

emu.addEventCallback(onFrameEnd, emu.eventType.endFrame)
emu.log("pce-test: started, polling $" .. string.format("%04x", RESULT_ADDR) .. " every " .. tostring(POLL_INTERVAL) .. " frames, max " .. tostring(MAX_FRAMES))
