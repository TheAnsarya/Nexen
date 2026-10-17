-- GBA jsmolka test protocol handler for Nexen --testRunner.
-- jsmolka/gba-tests write results to a known IWRAM address.
-- The test framework writes 0x00 on pass, non-zero on fail to
-- the result address after completing.
--
-- Default result address: 0x03007f00 (IWRAM)
-- The test number appears at 0x03007f04.
--
-- Some tests use BG mode 4 to display results visually, but
-- memory polling is more reliable for automated testing.
--
-- Exit codes:
--   0 = pass
--   1 = fail (test number in log)
--   2 = timeout (test did not complete within frame budget)
--  90 = wrong console type

local MAX_FRAMES = 18000  -- 5 minutes at 60fps
local POLL_INTERVAL = 60  -- check every 60 frames (1 second)
local RESULT_ADDR = 0x03007f00
local STATUS_RUNNING = 0xff

local function onFrameEnd()
	local state = emu.getState()
	local consoleType = state["consoleType"]
	if consoleType ~= "Gba" then
		emu.log("gba-jsmolka-test: wrong console type: " .. tostring(consoleType))
		emu.stop(90)
		return
	end

	local frameCount = state["frameCount"] or 0

	if (frameCount % POLL_INTERVAL) == 0 and frameCount > 0 then
		local result = emu.read(RESULT_ADDR, emu.memType.gbaMemory)

		if result ~= STATUS_RUNNING then
			if result == 0x00 then
				emu.log("gba-jsmolka-test: PASS at frame " .. tostring(frameCount))
				emu.stop(0)
			else
				emu.log("gba-jsmolka-test: FAIL (code $" .. string.format("%02x", result)
					.. ") at frame " .. tostring(frameCount))
				emu.stop(1)
			end
			return
		end
	end

	if frameCount >= MAX_FRAMES then
		emu.log("gba-jsmolka-test: TIMEOUT at frame " .. tostring(frameCount))
		emu.stop(2)
	end
end

emu.addEventCallback(onFrameEnd, emu.eventType.endFrame)
emu.log("gba-jsmolka-test: started, polling $" .. string.format("%08x", RESULT_ADDR)
	.. " every " .. tostring(POLL_INTERVAL) .. " frames, max " .. tostring(MAX_FRAMES))
