local out = io.open("c:/Users/me/source/repos/Nexen/_tmp_genesis_2206_nexen_trace.log", "w")
out:write("# frame-level genesis trace\n")
out:write("# fields: frame pc sr hclock vclock vdpStatus displayEnabled vintPending hintPending dmaSource dmaLength d0 a0\n")

local function toint(v)
	if v == nil then
		return -1
	end
	return tonumber(v) or -1
end

local function toflag(v)
	if v then
		return 1
	end
	return 0
end

emu.addEventCallback(emu.eventType.endFrame, function()
	local s = emu.getState()
	local frame = toint(s["vdp.frameCount"])
	local pc = toint(s["cpu.pc"])
	local sr = toint(s["cpu.sr"])
	local hclock = toint(s["vdp.hClock"])
	local vclock = toint(s["vdp.vClock"])
	local vdpStatus = toint(s["vdp.status"])
	local dmaSource = toint(s["vdp.dmaSource"])
	local dmaLength = toint(s["vdp.dmaLength"])
	local d0 = toint(s["cpu.d0"])
	local a0 = toint(s["cpu.a0"])
	local displayEnabled = toflag(s["vdp.displayEnabled"])
	local vintPending = toflag(s["vdp.vintPending"])
	local hintPending = toflag(s["vdp.hintPending"])

	out:write(string.format(
		"F%04d PC=%06X SR=%04X HC=%03d VC=%03d VDP=%04X DISP=%d VINT=%d HINT=%d DMA=%06X/%d D0=%08X A0=%08X\\n",
		frame,
		pc & 0xFFFFFF,
		sr & 0xFFFF,
		hclock,
		vclock,
		vdpStatus & 0xFFFF,
		displayEnabled,
		vintPending,
		hintPending,
		dmaSource & 0xFFFFFF,
		dmaLength,
		d0 & 0xFFFFFFFF,
		a0 & 0xFFFFFFFF
	))

	if frame >= 120 then
		out:flush()
		out:close()
		emu.stop(0)
	end
end)
