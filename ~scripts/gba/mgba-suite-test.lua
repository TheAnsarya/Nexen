-- mGBA test suite protocol handler for Nexen --testRunner.
-- mGBA suite tests write results to IWRAM at a known address.
-- Result protocol:
--   Address 0x03007ff0: status byte (0 = pass, non-zero = fail code)
--   Address 0x03007ff4: test count completed
--
-- Exit codes:
--   0 = pass
--   1 = fail
--   2 = timeout
--  90 = wrong console type

local MAX_FRAMES = 36000  -- 10 minutes at 60fps
local POLL_INTERVAL = 60  -- check every 60 frames
local RESULT_ADDR = 0x03007ff0
local STATUS_RUNNING = 0xff

local function onFrameEnd()
	local state = emu.getState()
	local consoleType = state["consoleType"]
	if consoleType ~= "Gba" then
		emu.log("mgba-suite-test: wrong console type: " .. tostring(consoleType))
		emu.stop(90)
		return
	end

	local frameCount = state["frameCount"] or 0

	if (frameCount % POLL_INTERVAL) == 0 and frameCount > 0 then
		local result = emu.read(RESULT_ADDR, emu.memType.gbaMemory)

		if result ~= STATUS_RUNNING then
			if result == 0x00 then
				emu.log("mgba-suite-test: PASS at frame " .. tostring(frameCount))
				emu.stop(0)
			else
				emu.log("mgba-suite-test: FAIL (code $" .. string.format("%02x", result)
					.. ") at frame " .. tostring(frameCount))
				emu.stop(1)
			end
			return
		end
	end

	if frameCount >= MAX_FRAMES then
		emu.log("mgba-suite-test: TIMEOUT at frame " .. tostring(frameCount))
		emu.stop(2)
	end
end

emu.addEventCallback(onFrameEnd, emu.eventType.endFrame)
emu.log("mgba-suite-test: started, polling $" .. string.format("%08x", RESULT_ADDR)
	.. " every " .. tostring(POLL_INTERVAL) .. " frames, max " .. tostring(MAX_FRAMES))
