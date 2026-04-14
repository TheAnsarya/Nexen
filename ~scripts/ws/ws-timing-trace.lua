-- WonderSwan timing trace dumper for #1285 scaffold ROM.
-- Reads compact trace entries from WS RAM and emits JSON lines via emu.log.
--
-- Exit codes:
--   0 = pass (trace dumped)
--   1 = fail (ROM reported failure)
--   2 = timeout
--  90 = wrong console type

local MAX_FRAMES = 6000
local POLL_INTERVAL = 1

local RESULT_ADDR = 0x0000
local TRACE_COUNT_ADDR = 0x0001
local TRACE_BASE_ADDR = 0x0100
local TRACE_STRIDE = 4

local function emitTraceLine(index, eventId, scanline, data0, data1)
	emu.log("ws-trace-json:{\"index\":" .. tostring(index)
		.. ",\"eventId\":" .. tostring(eventId)
		.. ",\"scanline\":" .. tostring(scanline)
		.. ",\"data0\":" .. tostring(data0)
		.. ",\"data1\":" .. tostring(data1)
		.. "}")
end

local function dumpTrace(frameCount)
	local traceCount = emu.read(TRACE_COUNT_ADDR, emu.memType.wsMemory)
	emu.log("ws-timing-trace: dumping " .. tostring(traceCount) .. " entries at frame " .. tostring(frameCount))

	for i = 0, traceCount - 1 do
		local base = TRACE_BASE_ADDR + (i * TRACE_STRIDE)
		local eventId = emu.read(base + 0, emu.memType.wsMemory)
		local scanline = emu.read(base + 1, emu.memType.wsMemory)
		local data0 = emu.read(base + 2, emu.memType.wsMemory)
		local data1 = emu.read(base + 3, emu.memType.wsMemory)
		emitTraceLine(i, eventId, scanline, data0, data1)
	end
end

local function onFrameEnd()
	local state = emu.getState()
	local consoleType = state["consoleType"]
	if consoleType ~= "Ws" then
		emu.log("ws-timing-trace: wrong console type: " .. tostring(consoleType))
		emu.stop(90)
		return
	end

	local frameCount = state["frameCount"] or 0
	if (frameCount % POLL_INTERVAL) == 0 and frameCount > 0 then
		local result = emu.read(RESULT_ADDR, emu.memType.wsMemory)
		if result ~= 0x80 then
			if result == 0x00 then
				dumpTrace(frameCount)
				emu.log("ws-timing-trace: PASS")
				emu.stop(0)
			else
				emu.log("ws-timing-trace: FAIL (code $" .. string.format("%02x", result) .. ")")
				emu.stop(1)
			end
			return
		end
	end

	if frameCount >= MAX_FRAMES then
		emu.log("ws-timing-trace: TIMEOUT at frame " .. tostring(frameCount))
		emu.stop(2)
	end
end

emu.addEventCallback(onFrameEnd, emu.eventType.endFrame)
emu.log("ws-timing-trace: started")
