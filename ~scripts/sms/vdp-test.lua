-- SMS VDP test protocol handler for Nexen --testRunner.
-- Polls SMS memory for VDP test results.
-- VDP tests write pass/fail status to a known RAM location.
--
-- Exit codes:
--   0 = pass
--   1 = fail
--   2 = timeout
--  90 = wrong console type

local MAX_FRAMES = 36000  -- 10 minutes at 60fps
local POLL_INTERVAL = 60  -- check every 60 frames (1 second)
local RESULT_ADDR = 0xc000

local function onFrameEnd()
	local state = emu.getState()
	local consoleType = state["consoleType"]
	if consoleType ~= "Sms" then
		emu.log("sms-vdp-test: wrong console type: " .. tostring(consoleType))
		emu.stop(90)
		return
	end

	local frameCount = state["frameCount"] or 0

	if (frameCount % POLL_INTERVAL) == 0 and frameCount > 0 then
		local result = emu.read(RESULT_ADDR, emu.memType.smsMemory)

		if result ~= 0x80 then
			if result == 0x00 then
				emu.log("sms-vdp-test: PASS at frame " .. tostring(frameCount))
				emu.stop(0)
			else
				emu.log("sms-vdp-test: FAIL (code $" .. string.format("%02x", result) .. ") at frame " .. tostring(frameCount))
				emu.stop(1)
			end
			return
		end
	end

	if frameCount >= MAX_FRAMES then
		emu.log("sms-vdp-test: TIMEOUT at frame " .. tostring(frameCount))
		emu.stop(2)
	end
end

emu.addEventCallback(onFrameEnd, emu.eventType.endFrame)
emu.log("sms-vdp-test: started, polling $" .. string.format("%04x", RESULT_ADDR) .. " every " .. tostring(POLL_INTERVAL) .. " frames, max " .. tostring(MAX_FRAMES))
