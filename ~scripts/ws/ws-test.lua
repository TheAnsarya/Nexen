-- WonderSwan test protocol handler for Nexen --testRunner.
-- Polls WS CPU memory for test results from ws-test-suite.
-- Tests write pass/fail status to internal RAM.
--
-- Exit codes:
--   0 = pass
--   1 = fail
--   2 = timeout
--  90 = wrong console type

local MAX_FRAMES = 36000  -- 10 minutes at ~75fps
local POLL_INTERVAL = 75  -- check every 75 frames (~1 second at WS refresh rate)
local RESULT_ADDR = 0x0000 -- Internal RAM result location

local function onFrameEnd()
	local state = emu.getState()
	local consoleType = state["consoleType"]
	if consoleType ~= "Ws" then
		emu.log("ws-test: wrong console type: " .. tostring(consoleType))
		emu.stop(90)
		return
	end

	local frameCount = state["frameCount"] or 0

	if (frameCount % POLL_INTERVAL) == 0 and frameCount > 0 then
		local result = emu.read(RESULT_ADDR, emu.memType.wsMemory)

		if result ~= 0x80 then
			if result == 0x00 then
				emu.log("ws-test: PASS at frame " .. tostring(frameCount))
				emu.stop(0)
			else
				emu.log("ws-test: FAIL (code $" .. string.format("%02x", result) .. ") at frame " .. tostring(frameCount))
				emu.stop(1)
			end
			return
		end
	end

	if frameCount >= MAX_FRAMES then
		emu.log("ws-test: TIMEOUT at frame " .. tostring(frameCount))
		emu.stop(2)
	end
end

emu.addEventCallback(onFrameEnd, emu.eventType.endFrame)
emu.log("ws-test: started, polling $" .. string.format("%04x", RESULT_ADDR) .. " every " .. tostring(POLL_INTERVAL) .. " frames, max " .. tostring(MAX_FRAMES))
