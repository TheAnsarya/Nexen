-- Generic memory-poll test runner for Nexen --testRunner.
-- Polls a specified memory address for pass/fail values.
-- Configurable via global variables set before loading this script:
--
--   TEST_ADDRESS     = memory address to poll (required)
--   TEST_MEM_TYPE    = emu.memType value (default: emu.memType.nesMemory)
--   TEST_PASS_VALUE  = byte value indicating pass (default: 0x00)
--   TEST_FAIL_VALUE  = byte value indicating fail (default: 0xff)
--   TEST_RUN_VALUE   = byte value indicating still running (default: 0x80)
--   TEST_MAX_FRAMES  = maximum frames before timeout (default: 18000)
--   TEST_POLL_INTERVAL = frames between polls (default: 60)
--
-- Exit codes:
--   0 = pass (TEST_PASS_VALUE found)
--   1 = fail (TEST_FAIL_VALUE found or unexpected value)
--   2 = timeout (test did not complete within frame budget)
--   3 = configuration error (missing TEST_ADDRESS)

-- Read configuration from globals
local address = TEST_ADDRESS
local memType = TEST_MEM_TYPE or emu.memType.nesMemory
local passValue = TEST_PASS_VALUE or 0x00
local failValue = TEST_FAIL_VALUE or 0xff
local runValue = TEST_RUN_VALUE or 0x80
local maxFrames = TEST_MAX_FRAMES or 18000
local pollInterval = TEST_POLL_INTERVAL or 60

if address == nil then
	emu.log("memory-poll-test: ERROR - TEST_ADDRESS not set")
	emu.stop(3)
	return
end

local function onFrameEnd()
	local state = emu.getState()
	local frameCount = state["frameCount"] or 0

	if (frameCount % pollInterval) == 0 and frameCount > 0 then
		local result = emu.read(address, memType)

		if result == passValue then
			emu.log("memory-poll-test: PASS at frame " .. tostring(frameCount)
				.. " addr=$" .. string.format("%04x", address)
				.. " value=$" .. string.format("%02x", result))
			emu.stop(0)
			return
		elseif result == failValue then
			emu.log("memory-poll-test: FAIL at frame " .. tostring(frameCount)
				.. " addr=$" .. string.format("%04x", address)
				.. " value=$" .. string.format("%02x", result))
			emu.stop(1)
			return
		elseif result ~= runValue then
			emu.log("memory-poll-test: FAIL (unexpected value) at frame " .. tostring(frameCount)
				.. " addr=$" .. string.format("%04x", address)
				.. " value=$" .. string.format("%02x", result))
			emu.stop(1)
			return
		end
	end

	if frameCount >= maxFrames then
		emu.log("memory-poll-test: TIMEOUT at frame " .. tostring(frameCount)
			.. " addr=$" .. string.format("%04x", address))
		emu.stop(2)
	end
end

emu.addEventCallback(onFrameEnd, emu.eventType.endFrame)
emu.log("memory-poll-test: started, polling $" .. string.format("%04x", address)
	.. " every " .. tostring(pollInterval) .. " frames, max " .. tostring(maxFrames)
	.. " pass=$" .. string.format("%02x", passValue)
	.. " fail=$" .. string.format("%02x", failValue)
	.. " run=$" .. string.format("%02x", runValue))
