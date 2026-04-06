-- Atari 2600 memory-poll test for Nexen --testRunner.
-- Polls RAM address $80 for blargg-style result codes.
-- Atari 2600 RAM is at $80-$FF (128 bytes, mirrored).
--   0x80 = running (test still in progress)
--   0x00 = pass (all tests passed)
--   0x01-0x7f = fail (failed test number)
--
-- Exit codes:
--   0 = pass
--   1 = fail (test number in log)
--   2 = timeout (test did not complete within frame budget)
--  90 = wrong console type

local MAX_FRAMES = 18000  -- ~5 minutes at 60fps
local POLL_INTERVAL = 60  -- check every 60 frames (1 second)
local RESULT_ADDR = 0x80

local function onFrameEnd()
	local state = emu.getState()
	local consoleType = state["consoleType"]
	if consoleType ~= "Atari2600" then
		emu.log("a2600-memory-poll: wrong console type: " .. tostring(consoleType))
		emu.stop(90)
		return
	end

	local frameCount = state["frameCount"] or 0

	if (frameCount % POLL_INTERVAL) == 0 and frameCount > 0 then
		local result = emu.read(RESULT_ADDR, emu.memType.atari2600Memory)

		if result ~= 0x80 then
			if result == 0x00 then
				emu.log("a2600-memory-poll: PASS at frame " .. tostring(frameCount))
				emu.stop(0)
			else
				emu.log("a2600-memory-poll: FAIL (test #" .. tostring(result) .. ") at frame " .. tostring(frameCount))
				emu.stop(1)
			end
			return
		end
	end

	if frameCount >= MAX_FRAMES then
		emu.log("a2600-memory-poll: TIMEOUT at frame " .. tostring(frameCount))
		emu.stop(2)
	end
end

emu.addEventCallback(onFrameEnd, emu.eventType.endFrame)
emu.log("a2600-memory-poll: started, polling $80 every " .. tostring(POLL_INTERVAL) .. " frames, max " .. tostring(MAX_FRAMES))
