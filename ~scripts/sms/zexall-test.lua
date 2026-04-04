-- SMS ZEXALL Z80 test protocol handler for Nexen --testRunner.
-- ZEXALL writes test results to RAM. The test prints results to screen
-- and signals completion by writing to a known address.
-- For SMS, ZEXALL typically runs for a long time and outputs to VDP.
--
-- This script uses a generic memory poll approach:
--   Poll a RAM address for completion status.
--
-- Exit codes:
--   0 = pass
--   1 = fail (test number in log)
--   2 = timeout (test did not complete within frame budget)
--  90 = wrong console type

local MAX_FRAMES = 108000  -- 30 minutes at 60fps (Z80 tests are slow)
local POLL_INTERVAL = 600  -- check every 600 frames (10 seconds)
local RESULT_ADDR = 0xc000 -- RAM result location

local function onFrameEnd()
	local state = emu.getState()
	local consoleType = state["consoleType"]
	if consoleType ~= "Sms" then
		emu.log("sms-zexall-test: wrong console type: " .. tostring(consoleType))
		emu.stop(90)
		return
	end

	local frameCount = state["frameCount"] or 0

	if (frameCount % POLL_INTERVAL) == 0 and frameCount > 0 then
		local result = emu.read(RESULT_ADDR, emu.memType.smsMemory)

		if result ~= 0x80 then
			if result == 0x00 then
				emu.log("sms-zexall-test: PASS at frame " .. tostring(frameCount))
				emu.stop(0)
			else
				emu.log("sms-zexall-test: FAIL (code $" .. string.format("%02x", result) .. ") at frame " .. tostring(frameCount))
				emu.stop(1)
			end
			return
		end
	end

	if frameCount >= MAX_FRAMES then
		emu.log("sms-zexall-test: TIMEOUT at frame " .. tostring(frameCount))
		emu.stop(2)
	end
end

emu.addEventCallback(onFrameEnd, emu.eventType.endFrame)
emu.log("sms-zexall-test: started, polling $" .. string.format("%04x", RESULT_ADDR) .. " every " .. tostring(POLL_INTERVAL) .. " frames, max " .. tostring(MAX_FRAMES))
