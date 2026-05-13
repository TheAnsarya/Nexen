local targetFrames = 120

emu.addEventCallback(emu.eventType.endFrame, function()
	local state = emu.getState()
	local frame = state["vdp.frameCount"] or 0
	if frame >= targetFrames then
		emu.stop(0)
	end
end)
