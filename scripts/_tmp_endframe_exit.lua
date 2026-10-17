local frames = 0
emu.addEventCallback(function()
	frames = frames + 1
	if frames >= 1 then
		emu.stop(0)
	end
end, emu.eventType.endFrame)
