local outputPath = os.getenv("NEXEN_STARTUP_SCREENSHOT_PATH")
local targetFrame = tonumber(os.getenv("NEXEN_STARTUP_SCREENSHOT_FRAME") or "180")

local function finish(exitCode)
	if type(emu.exit) == "function" then
		emu.exit(exitCode)
	elseif type(emu.stop) == "function" then
		emu.stop(exitCode)
	end
end

if outputPath == nil or outputPath == "" then
	emu.log("capture-genesis-startup-screenshot: missing NEXEN_STARTUP_SCREENSHOT_PATH")
	finish(2)
	return
end

if targetFrame == nil or targetFrame < 1 then
	targetFrame = 1
end

local function writePngBytes(bytes)
	if bytes == nil or #bytes == 0 then
		emu.log("capture-genesis-startup-screenshot: empty screenshot payload")
		finish(4)
		return
	end

	local file, err = io.open(outputPath, "wb")
	if file == nil then
		emu.log("capture-genesis-startup-screenshot: failed to open output file: " .. tostring(err))
		finish(3)
		return
	end

	file:write(bytes)
	file:close()
	finish(0)
end

if type(emu.step) == "function" and emu.stepType ~= nil and emu.stepType.frame ~= nil then
	for _ = 1, targetFrame do
		emu.step(1, emu.stepType.frame)
	end

	local ok, pngBytes = pcall(emu.takeScreenshot)
	if not ok then
		emu.log("capture-genesis-startup-screenshot: takeScreenshot failed")
		finish(5)
		return
	end

	writePngBytes(pngBytes)
	return
end

local frameCounter = 0
emu.addEventCallback(function()
	frameCounter = frameCounter + 1
	if frameCounter < targetFrame then
		return
	end

	local ok, pngBytes = pcall(emu.takeScreenshot)
	if not ok then
		emu.log("capture-genesis-startup-screenshot: takeScreenshot failed")
		finish(5)
		return
	end

	writePngBytes(pngBytes)
end, emu.eventType.endFrame)
