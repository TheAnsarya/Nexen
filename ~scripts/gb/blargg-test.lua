-- GB blargg test protocol handler for Nexen --testRunner.
-- GB blargg tests write results to serial output and memory.
-- Polls address $a000 in GB WorkRam for blargg result codes:
--   0x80 = running (test still in progress)
--   0x00 = pass (all tests passed)
--   0x01-0x7f = fail (failed test number)
--
-- Some GB blargg tests use different memory locations than NES.
-- The standard GB blargg output location is in external RAM ($a000).
--
-- Exit codes:
--   0 = pass
--   1 = fail (test number in log)
--   2 = timeout (test did not complete within frame budget)
--  90 = wrong console type

local MAX_FRAMES = 36000  -- 10 minutes at 60fps
local POLL_INTERVAL = 60  -- check every 60 frames (1 second)
local RESULT_ADDR = 0xa000

local function onFrameEnd()
	local state = emu.getState()
	local consoleType = state["consoleType"]
	if consoleType ~= "Gameboy" then
		emu.log("gb-blargg-test: wrong console type: " .. tostring(consoleType))
		emu.stop(90)
		return
	end

	local frameCount = state["frameCount"] or 0

	if (frameCount % POLL_INTERVAL) == 0 and frameCount > 0 then
		local result = emu.read(RESULT_ADDR, emu.memType.gbMemory)

		if result ~= 0x80 then
			if result == 0x00 then
				emu.log("gb-blargg-test: PASS at frame " .. tostring(frameCount))
				emu.stop(0)
			else
				emu.log("gb-blargg-test: FAIL (test #" .. tostring(result) .. ") at frame " .. tostring(frameCount))
				emu.stop(1)
			end
			return
		end
	end

	if frameCount >= MAX_FRAMES then
		emu.log("gb-blargg-test: TIMEOUT at frame " .. tostring(frameCount))
		emu.stop(2)
	end
end

emu.addEventCallback(onFrameEnd, emu.eventType.endFrame)
emu.log("gb-blargg-test: started, polling $a000 every " .. tostring(POLL_INTERVAL) .. " frames, max " .. tostring(MAX_FRAMES))
