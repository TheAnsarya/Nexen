-- SNES blargg test protocol handler for Nexen --testRunner.
-- Polls WRAM address for blargg result codes.
-- SNES blargg tests typically write to WRAM $6000:
--   0x80 = running (test still in progress)
--   0x00 = pass (all tests passed)
--   0x01-0x7f = fail (failed test number)
--
-- Exit codes:
--   0 = pass
--   1 = fail (test number in log)
--   2 = timeout (test did not complete within frame budget)
--  90 = wrong console type

local MAX_FRAMES = 36000  -- 10 minutes at 60fps
local POLL_INTERVAL = 60  -- check every 60 frames (1 second)
local RESULT_ADDR = 0x6000

local function onFrameEnd()
	local state = emu.getState()
	local consoleType = state["consoleType"]
	if consoleType ~= "Snes" then
		emu.log("snes-blargg-test: wrong console type: " .. tostring(consoleType))
		emu.stop(90)
		return
	end

	local frameCount = state["frameCount"] or 0

	if (frameCount % POLL_INTERVAL) == 0 and frameCount > 0 then
		local result = emu.read(RESULT_ADDR, emu.memType.snesMemory)

		if result ~= 0x80 then
			if result == 0x00 then
				emu.log("snes-blargg-test: PASS at frame " .. tostring(frameCount))
				emu.stop(0)
			else
				emu.log("snes-blargg-test: FAIL (test #" .. tostring(result) .. ") at frame " .. tostring(frameCount))
				emu.stop(1)
			end
			return
		end
	end

	if frameCount >= MAX_FRAMES then
		emu.log("snes-blargg-test: TIMEOUT at frame " .. tostring(frameCount))
		emu.stop(2)
	end
end

emu.addEventCallback(onFrameEnd, emu.eventType.endFrame)
emu.log("snes-blargg-test: started, polling $6000 every " .. tostring(POLL_INTERVAL) .. " frames, max " .. tostring(MAX_FRAMES))
