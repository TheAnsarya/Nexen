-- Channel F boot smoke test for Nexen --testRunner.
-- Checks that the Channel F boots correctly by verifying screen output.
--
-- Exit codes:
--   0 = pass (non-zero pixels detected, boot screen rendered)
--   1 = fail (screen still blank after timeout)
--   2 = timeout
--  90 = wrong console type

local MAX_FRAMES = 300  -- Channel F runs at ~59.94 fps, ~5 seconds
local CHECK_INTERVAL = 60
local observedNonZero = false

local function onFrameEnd()
	local state = emu.getState()
	local consoleType = state["consoleType"]
	if consoleType ~= "ChannelF" then
		emu.log("channelf-boot-smoke: wrong console type: " .. tostring(consoleType))
		emu.stop(90)
		return
	end

	local frameCount = state["frameCount"] or 0

	if (frameCount % CHECK_INTERVAL) == 0 and frameCount > 0 then
		-- Sample screen buffer for non-zero pixels
		local buf = emu.getScreenBuffer()
		if buf then
			local nonZero = 0
			for i = 1, math.min(#buf, 1000) do
				if buf[i] ~= 0 then
					nonZero = nonZero + 1
				end
			end
			if nonZero > 10 then
				observedNonZero = true
			end
		end
	end

	if frameCount >= MAX_FRAMES then
		if observedNonZero then
			emu.log("channelf-boot-smoke: PASS (display active)")
			emu.stop(0)
		else
			emu.log("channelf-boot-smoke: FAIL (display blank)")
			emu.stop(1)
		end
	end
end

emu.addEventCallback(onFrameEnd, emu.eventType.endFrame)
emu.log("channelf-boot-smoke: started, checking display every " .. tostring(CHECK_INTERVAL) .. " frames, max " .. tostring(MAX_FRAMES))
