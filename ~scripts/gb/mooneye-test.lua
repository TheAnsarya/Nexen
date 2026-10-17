-- Mooneye GB test protocol handler for Nexen --testRunner.
-- Mooneye tests signal completion by executing LD B,B (opcode 0x40).
-- On completion, registers contain a Fibonacci-like signature:
--   B=3, C=5, D=8, E=13, H=21, L=34 = PASS
--   Any register containing 0x42 = FAIL
--
-- This script polls GB CPU registers every frame and checks for the
-- completion signature. Mooneye tests typically write the result to
-- registers and then enter an infinite loop executing LD B,B.
--
-- Exit codes:
--   0 = pass (Fibonacci signature found)
--   1 = fail (0x42 signature or wrong values)
--   2 = timeout (test did not complete within frame budget)
--  90 = wrong console type

local MAX_FRAMES = 18000  -- 5 minutes at 60fps
local POLL_INTERVAL = 30  -- check every 30 frames

-- Mooneye pass signature: Fibonacci-like sequence
local PASS_B = 3
local PASS_C = 5
local PASS_D = 8
local PASS_E = 13
local PASS_H = 21
local PASS_L = 34
local FAIL_MARKER = 0x42

local function checkRegisters(state)
	local cpu = state["cpu"]
	if cpu == nil then
		return nil
	end

	local b = cpu["b"]
	local c = cpu["c"]
	local d = cpu["d"]
	local e = cpu["e"]
	local h = cpu["h"]
	local l = cpu["l"]

	if b == nil then
		return nil
	end

	-- Check for pass: Fibonacci signature
	if b == PASS_B and c == PASS_C and d == PASS_D and e == PASS_E and h == PASS_H and l == PASS_L then
		return "pass"
	end

	-- Check for fail: 0x42 in any register
	if b == FAIL_MARKER or c == FAIL_MARKER or d == FAIL_MARKER or e == FAIL_MARKER or h == FAIL_MARKER or l == FAIL_MARKER then
		return "fail"
	end

	return nil
end

local function onFrameEnd()
	local state = emu.getState()
	local consoleType = state["consoleType"]
	if consoleType ~= "Gameboy" then
		emu.log("mooneye-test: wrong console type: " .. tostring(consoleType))
		emu.stop(90)
		return
	end

	local frameCount = state["frameCount"] or 0

	if (frameCount % POLL_INTERVAL) == 0 and frameCount > 0 then
		local result = checkRegisters(state)

		if result == "pass" then
			emu.log("mooneye-test: PASS at frame " .. tostring(frameCount))
			emu.stop(0)
			return
		elseif result == "fail" then
			local cpu = state["cpu"]
			emu.log("mooneye-test: FAIL at frame " .. tostring(frameCount)
				.. " B=" .. tostring(cpu["b"])
				.. " C=" .. tostring(cpu["c"])
				.. " D=" .. tostring(cpu["d"])
				.. " E=" .. tostring(cpu["e"])
				.. " H=" .. tostring(cpu["h"])
				.. " L=" .. tostring(cpu["l"]))
			emu.stop(1)
			return
		end
	end

	if frameCount >= MAX_FRAMES then
		emu.log("mooneye-test: TIMEOUT at frame " .. tostring(frameCount))
		emu.stop(2)
	end
end

emu.addEventCallback(onFrameEnd, emu.eventType.endFrame)
emu.log("mooneye-test: started, polling registers every " .. tostring(POLL_INTERVAL) .. " frames, max " .. tostring(MAX_FRAMES))
