-- Atari 2600 boot smoke test for Nexen --testRunner.
-- Verifies the console boots and runs by checking TIA output.
-- Reads INTIM ($284) timer register to confirm RIOT is ticking.
--
-- Exit codes:
--   0 = pass (timer is actively counting)
--   1 = fail (timer stuck or no activity detected)
--   2 = timeout
--  90 = wrong console type

local MAX_FRAMES = 300  -- ~5 seconds at 60fps
local CHECK_INTERVAL = 30
local samples = {}

local function onFrameEnd()
	local state = emu.getState()
	local consoleType = state["consoleType"]
	if consoleType ~= "Atari2600" then
		emu.log("a2600-boot-smoke: wrong console type: " .. tostring(consoleType))
		emu.stop(90)
		return
	end

	local frameCount = state["frameCount"] or 0

	if (frameCount % CHECK_INTERVAL) == 0 and frameCount > 0 then
		-- Read INTIM (timer value) at $284 in the TIA/RIOT address space
		-- In Atari 2600 memory map, $280-$29F is RIOT I/O
		local timerVal = emu.read(0x284, emu.memType.atari2600Memory)
		table.insert(samples, timerVal)
	end

	if frameCount >= MAX_FRAMES then
		-- Check if we got varying timer values (RIOT is running)
		if #samples < 2 then
			emu.log("a2600-boot-smoke: FAIL (insufficient samples)")
			emu.stop(1)
			return
		end

		local allSame = true
		for i = 2, #samples do
			if samples[i] ~= samples[1] then
				allSame = false
				break
			end
		end

		if allSame then
			emu.log("a2600-boot-smoke: FAIL (timer stuck at $" .. string.format("%02x", samples[1]) .. ")")
			emu.stop(1)
		else
			emu.log("a2600-boot-smoke: PASS (timer active, " .. tostring(#samples) .. " samples)")
			emu.stop(0)
		end
	end
end

emu.addEventCallback(onFrameEnd, emu.eventType.endFrame)
emu.log("a2600-boot-smoke: started, sampling INTIM ($284) every " .. tostring(CHECK_INTERVAL) .. " frames")
