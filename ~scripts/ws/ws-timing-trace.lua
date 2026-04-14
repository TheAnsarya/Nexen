-- WonderSwan timing trace dumper for #1285 scaffold ROM.
-- Reads compact trace entries from WS RAM and emits JSON lines via emu.log.
--
-- Exit codes:
--   0 = pass (trace dumped)
--   1 = fail (ROM reported failure)
--   2 = timeout
--  90 = wrong console type

local MAX_FRAMES = 6000
local POLL_INTERVAL = 15

local RESULT_ADDR = 0x0000
local TRACE_COUNT_ADDR = 0x0001
local TRACE_BASE_ADDR = 0x0100
local TRACE_STRIDE = 6
local sawRunningMarker = false

local traceFile = nil

local function getTraceOutputPath()
	if TRACE_OUTPUT_PATH ~= nil and TRACE_OUTPUT_PATH ~= "" then
		return TRACE_OUTPUT_PATH
	end

	if os ~= nil and os.getenv ~= nil then
		local envPath = os.getenv("NEXEN_WS_TRACE_OUTPUT")
		if envPath ~= nil and envPath ~= "" then
			return envPath
		end
	end

	return nil
end

local function writeTraceLine(line)
	if traceFile ~= nil then
		traceFile:write(line .. "\n")
	end
	if print ~= nil then
		print("ws-trace-json:" .. line)
	end
	emu.log("ws-trace-json:" .. line)
end

local function emitTraceLine(index, eventId, scanline, irqActive, irqEnabled, hTimer, vTimer)
	writeTraceLine("{\"index\":" .. tostring(index)
		.. ",\"eventId\":" .. tostring(eventId)
		.. ",\"scanline\":" .. tostring(scanline)
		.. ",\"irqActive\":" .. tostring(irqActive)
		.. ",\"irqEnabled\":" .. tostring(irqEnabled)
		.. ",\"hTimer\":" .. tostring(hTimer)
		.. ",\"vTimer\":" .. tostring(vTimer)
		.. "}")
end

local function dumpTrace(frameCount)
	local traceCount = emu.read(TRACE_COUNT_ADDR, emu.memType.wsMemory)
	if print ~= nil then
		print("ws-timing-trace: dumping " .. tostring(traceCount) .. " entries at frame " .. tostring(frameCount))
	end
	emu.log("ws-timing-trace: dumping " .. tostring(traceCount) .. " entries at frame " .. tostring(frameCount))

	if traceCount == 0 then
		local scanline = emu.read(0x0002, emu.memType.wsPort)
		local irqActive = emu.read(0x00b4, emu.memType.wsPort)
		local irqEnabled = emu.read(0x00b2, emu.memType.wsPort)
		local hTimer = emu.read(0x00a8, emu.memType.wsPort)
		local vTimer = emu.read(0x00aa, emu.memType.wsPort)
		emitTraceLine(0, 0xff, scanline, irqActive, irqEnabled, hTimer, vTimer)
		return
	end

	for i = 0, traceCount - 1 do
		local base = TRACE_BASE_ADDR + (i * TRACE_STRIDE)
		local eventId = emu.read(base + 0, emu.memType.wsMemory)
		local scanline = emu.read(base + 1, emu.memType.wsMemory)
		local irqActive = emu.read(base + 2, emu.memType.wsMemory)
		local irqEnabled = emu.read(base + 3, emu.memType.wsMemory)
		local hTimer = emu.read(base + 4, emu.memType.wsMemory)
		local vTimer = emu.read(base + 5, emu.memType.wsMemory)
		emitTraceLine(i, eventId, scanline, irqActive, irqEnabled, hTimer, vTimer)
	end
end

local function onFrameEnd()
	local state = emu.getState()
	local consoleType = state["consoleType"]
	if consoleType ~= "Ws" then
		if print ~= nil then
			print("ws-timing-trace: wrong console type: " .. tostring(consoleType))
		end
		emu.log("ws-timing-trace: wrong console type: " .. tostring(consoleType))
		emu.stop(90)
		return
	end

	local frameCount = state["frameCount"] or 0
	if (frameCount % POLL_INTERVAL) == 0 and frameCount > 0 then
		local result = emu.read(RESULT_ADDR, emu.memType.wsMemory)
		if result == 0x80 then
			sawRunningMarker = true
		end

		if result == 0x00 then
			dumpTrace(frameCount)
			if traceFile ~= nil then
				traceFile:flush()
				traceFile:close()
				traceFile = nil
			end
			if print ~= nil then
				print("ws-timing-trace: PASS")
			end
			emu.log("ws-timing-trace: PASS")
			emu.stop(0)
			return
		elseif sawRunningMarker and result > 0x00 and result < 0x80 then
			if traceFile ~= nil then
				traceFile:flush()
				traceFile:close()
				traceFile = nil
			end
			if print ~= nil then
				print("ws-timing-trace: FAIL (code $" .. string.format("%02x", result) .. ")")
			end
			emu.log("ws-timing-trace: FAIL (code $" .. string.format("%02x", result) .. ")")
			emu.stop(1)
			return
		end
	end

	if frameCount >= MAX_FRAMES then
		if traceFile ~= nil then
			traceFile:flush()
			traceFile:close()
			traceFile = nil
		end
		if print ~= nil then
			print("ws-timing-trace: TIMEOUT at frame " .. tostring(frameCount))
		end
		emu.log("ws-timing-trace: TIMEOUT at frame " .. tostring(frameCount))
		emu.stop(2)
	end
end

local traceOutputPath = getTraceOutputPath()
if traceOutputPath ~= nil and io ~= nil and io.open ~= nil then
	traceFile = io.open(traceOutputPath, "w")
	if traceFile == nil then
		emu.log("ws-timing-trace: WARN unable to open output file: " .. tostring(traceOutputPath))
	end
end

emu.addEventCallback(onFrameEnd, emu.eventType.endFrame)
if print ~= nil then
	print("ws-timing-trace: started")
end
emu.log("ws-timing-trace: started")
