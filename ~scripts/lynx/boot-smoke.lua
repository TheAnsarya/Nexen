-- Lynx headless boot smoke script for Nexen --testRunner.
-- Exit codes:
--   0 = boot smoke pass
--   2 = boot smoke fail (insufficient frame/screen activity)
--  90 = wrong console type

local frameCounter = 0
local maxNonZeroSamples = 0
local maxUniqueColors = 0
local sampleCount = 0

local function sampleScreen()
	local buffer = emu.getScreenBuffer()
	if buffer == nil or #buffer == 0 then
		return
	end

	local seenColors = {}
	local nonZero = 0
	local stride = 8
	for i = 1, #buffer, stride do
		local color = buffer[i]
		if color ~= 0 then
			nonZero = nonZero + 1
		end
		seenColors[color] = true
	end

	local uniqueCount = 0
	for _ in pairs(seenColors) do
		uniqueCount = uniqueCount + 1
	end

	if nonZero > maxNonZeroSamples then
		maxNonZeroSamples = nonZero
	end
	if uniqueCount > maxUniqueColors then
		maxUniqueColors = uniqueCount
	end

	sampleCount = sampleCount + 1
end

local function applyBootInputs()
	-- Pulse Pause/Start to progress menu states where required.
	if frameCounter >= 120 and frameCounter < 126 then
		emu.setInput({ start = true }, 0)
	end

	-- Pulse A to pass title/menu prompts where required.
	if frameCounter >= 300 and frameCounter < 306 then
		emu.setInput({ a = true }, 0)
	end
end

local function finalizeAndStop()
	local state = emu.getState()
	local consoleType = state["consoleType"]
	if consoleType ~= "Lynx" then
		emu.log("boot-smoke: non-Lynx console detected: " .. tostring(consoleType))
		emu.stop(90)
		return
	end

	local bootPass = frameCounter >= 600 and maxNonZeroSamples >= 32 and maxUniqueColors >= 8 and sampleCount >= 4
	if bootPass then
		emu.log("boot-smoke: PASS frame=" .. tostring(frameCounter)
			.. " nonZeroMax=" .. tostring(maxNonZeroSamples)
			.. " uniqueColorsMax=" .. tostring(maxUniqueColors)
			.. " samples=" .. tostring(sampleCount))
		emu.stop(0)
	else
		emu.log("boot-smoke: FAIL frame=" .. tostring(frameCounter)
			.. " nonZeroMax=" .. tostring(maxNonZeroSamples)
			.. " uniqueColorsMax=" .. tostring(maxUniqueColors)
			.. " samples=" .. tostring(sampleCount))
		emu.stop(2)
	end
end

local function onFrameEnd()
	local state = emu.getState()
	frameCounter = state["frameCount"] or frameCounter

	if (frameCounter % 120) == 0 then
		sampleScreen()
	end

	if frameCounter >= 900 then
		finalizeAndStop()
	end
end

emu.addEventCallback(applyBootInputs, emu.eventType.inputPolled)
emu.addEventCallback(onFrameEnd, emu.eventType.endFrame)
