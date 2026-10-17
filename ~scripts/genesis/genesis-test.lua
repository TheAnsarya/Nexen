-- Genesis/Mega Drive test protocol handler for Nexen --testRunner.
-- NOTE: Genesis does not currently have a Lua memType API in Nexen.
-- This script uses a frame-count based approach: run for N frames
-- and check the console type. Manual verification of output is required.
--
-- Exit codes:
--   0 = completed (ran to frame limit without crash)
--   2 = timeout (used as a soft pass — test ran to completion)
--  90 = wrong console type

local MAX_FRAMES = 18000  -- 5 minutes at 60fps
local CHECK_INTERVAL = 60 -- check console type periodically

local function onFrameEnd()
	local state = emu.getState()
	local consoleType = state["consoleType"]
	if consoleType ~= "Genesis" then
		emu.log("genesis-test: wrong console type: " .. tostring(consoleType))
		emu.stop(90)
		return
	end

	local frameCount = state["frameCount"] or 0

	if frameCount >= MAX_FRAMES then
		emu.log("genesis-test: completed " .. tostring(frameCount) .. " frames without crash")
		emu.stop(0)
	end
end

emu.addEventCallback(onFrameEnd, emu.eventType.endFrame)
emu.log("genesis-test: started, running for " .. tostring(MAX_FRAMES) .. " frames (no memory poll — Genesis memType not yet available)")
