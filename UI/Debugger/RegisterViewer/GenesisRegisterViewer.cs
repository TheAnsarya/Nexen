using System.Collections.Generic;
using Nexen.Debugger.ViewModels;
using Nexen.Interop;
using static Nexen.Debugger.ViewModels.RegEntry;

namespace Nexen.Debugger.RegisterViewer;

public sealed class GenesisRegisterViewer {
	private static string DecodeTranscriptFlags(byte flags) {
		if ((flags & 0x80) != 0) {
			string hsOp = (flags & 0x01) != 0 ? "Write" : "Read";
			string channel = (flags & 0x04) != 0 ? "Z80-RESET" : "Z80-BUSREQ";
			string hsRole = (flags & 0x02) != 0 ? "State" : "Command";
			return hsOp + " | HS | " + channel + " | " + hsRole;
		}

		string op = (flags & 0x01) != 0 ? "Write" : "Read";
		string lane = (flags & 0x40) != 0
			? "G6"
			: ((flags & 0x20) != 0
				? "G5"
				: ((flags & 0x10) != 0
					? "G4"
					: ((flags & 0x08) != 0
						? "G3"
						: ((flags & 0x04) != 0 ? "G2" : "G1"))));
		string role = (flags & 0x02) != 0 ? "Response" : "Command";
		return op + " | " + lane + " | " + role;
	}

	public static List<RegisterViewerTab> GetTabs(ref GenesisState state) {
		return new List<RegisterViewerTab>() {
			GetCpuTab(ref state),
			GetVdpTab(ref state),
			GetPsgTab(ref state),
			GetIoTab(ref state),
		};
	}

	private static RegisterViewerTab GetCpuTab(ref GenesisState state) {
		GenesisM68kState cpu = state.Cpu;
		List<RegEntry> entries = [];

		entries.Add(new RegEntry("", "Core"));
		entries.Add(new RegEntry("", "PC", cpu.PC, Format.X32));
		entries.Add(new RegEntry("", "SR", cpu.SR, Format.X16));
		entries.Add(new RegEntry("", "USP", cpu.USP, Format.X32));
		entries.Add(new RegEntry("", "SSP", cpu.SSP, Format.X32));
		entries.Add(new RegEntry("", "CycleCount", cpu.CycleCount));
		entries.Add(new RegEntry("", "Stopped", cpu.Stopped));

		entries.Add(new RegEntry("", "Data registers"));
		for (int i = 0; i < 8; i++) {
			entries.Add(new RegEntry("", "D" + i, cpu.D[i], Format.X32));
		}

		entries.Add(new RegEntry("", "Address registers"));
		for (int i = 0; i < 8; i++) {
			entries.Add(new RegEntry("", "A" + i, cpu.A[i], Format.X32));
		}

		return new RegisterViewerTab("CPU", entries, CpuType.Genesis);
	}

	private static RegisterViewerTab GetVdpTab(ref GenesisState state) {
		GenesisVdpState vdp = state.Vdp;
		List<RegEntry> entries = [];
		uint dmaLength = (uint)((vdp.Registers[20] << 8) | vdp.Registers[19]);
		uint dmaEffectiveLength = dmaLength == 0 ? 0x10000u : dmaLength;
		uint dmaSource = (uint)(((vdp.Registers[23] & 0x3F) << 17) | (vdp.Registers[22] << 9) | (vdp.Registers[21] << 1));
		string dmaModeLabel = vdp.DmaMode switch {
			0 => "68kCopy",
			1 => "Fill",
			2 => "VramCopy",
			3 => "RawMode3",
			_ => "Unknown"
		};

		entries.AddRange(new List<RegEntry>() {
			new RegEntry("", "State"),
			new RegEntry("", "FrameCount", vdp.FrameCount),
			new RegEntry("", "HCounter", vdp.HCounter),
			new RegEntry("", "VCounter", vdp.VCounter),
			new RegEntry("", "StatusRegister", vdp.StatusRegister, Format.X16),
			new RegEntry("", "Status.VBlank", (vdp.StatusRegister & 0x0008) != 0),
			new RegEntry("", "Status.HBlank", (vdp.StatusRegister & 0x0004) != 0),
			new RegEntry("", "Status.DmaBusy", (vdp.StatusRegister & 0x0002) != 0),
			new RegEntry("", "Status.PalMode", (vdp.StatusRegister & 0x0001) != 0),
			new RegEntry("", "Status.VIntPending", (vdp.StatusRegister & 0x0080) != 0),
			new RegEntry("", "Status.SprOverflow", (vdp.StatusRegister & 0x0040) != 0),
			new RegEntry("", "Status.SprCollision", (vdp.StatusRegister & 0x0020) != 0),
			new RegEntry("", "Status.OddFrame", (vdp.StatusRegister & 0x0010) != 0),
			new RegEntry("", "Status.FifoFull", (vdp.StatusRegister & 0x0100) != 0),
			new RegEntry("", "Status.FifoEmpty", (vdp.StatusRegister & 0x0200) != 0),
			new RegEntry("", "AddressRegister", vdp.AddressRegister, Format.X32),
			new RegEntry("", "CodeRegister", vdp.CodeRegister, Format.X8),
			new RegEntry("", "WritePending", vdp.WritePending),
			new RegEntry("", "DataPortBuffer", vdp.DataPortBuffer, Format.X16),
			new RegEntry("", "HIntCounter", vdp.HIntCounter, Format.X8),
			new RegEntry("", "DMA"),
			new RegEntry("", "DmaActive", vdp.DmaActive),
			new RegEntry("", "DmaMode", vdp.DmaMode, Format.X8),
			new RegEntry("", "DmaModeLabel: " + dmaModeLabel),
			new RegEntry("", "DmaLength", dmaLength, Format.X16),
			new RegEntry("", "DmaEffectiveLength", dmaEffectiveLength, Format.X32),
			new RegEntry("", "DmaSource", dmaSource, Format.X32),
		});

		for (int i = 0; i < 24; i++) {
			entries.Add(new RegEntry("R" + i, "Register " + i, vdp.Registers[i], Format.X8));
		}

		return new RegisterViewerTab("VDP", entries, CpuType.Genesis);
	}

	private static RegisterViewerTab GetPsgTab(ref GenesisState state) {
		GenesisPsgState psg = state.Psg;
		List<RegEntry> entries = [];

		entries.Add(new RegEntry("", "Core"));
		entries.Add(new RegEntry("", "LatchedRegister", psg.LatchedRegister, Format.X8));
		entries.Add(new RegEntry("", "NoiseMode", psg.NoiseMode, Format.X8));
		entries.Add(new RegEntry("", "NoiseShiftRegister", psg.NoiseShiftRegister, Format.X16));
		entries.Add(new RegEntry("", "WriteCount", psg.WriteCount));

		for (int i = 0; i < 4; i++) {
			entries.Add(new RegEntry("", "Channel " + i));
			entries.Add(new RegEntry("", "ToneCounter", psg.Channels[i].ToneCounter, Format.X16));
			entries.Add(new RegEntry("", "Volume", psg.Channels[i].Volume, Format.X8));
		}

		return new RegisterViewerTab("PSG", entries, CpuType.Genesis);
	}

	private static RegisterViewerTab GetIoTab(ref GenesisState state) {
		GenesisIoState io = state.Io;
		List<RegEntry> entries = [];

		entries.Add(new RegEntry("", "Controller ports"));
		for (int i = 0; i < 3; i++) {
			entries.Add(new RegEntry("", "DataPort[" + i + "]", io.DataPort[i], Format.X8));
			entries.Add(new RegEntry("", "CtrlPort[" + i + "]", io.CtrlPort[i], Format.X8));
		}

		entries.Add(new RegEntry("", "Serial"));
		for (int i = 0; i < 3; i++) {
			entries.Add(new RegEntry("", "TxData[" + i + "]", io.TxData[i], Format.X8));
			entries.Add(new RegEntry("", "RxData[" + i + "]", io.RxData[i], Format.X8));
			entries.Add(new RegEntry("", "SCtrl[" + i + "]", io.SCtrl[i], Format.X8));
		}

		entries.Add(new RegEntry("", "TH"));
		entries.Add(new RegEntry("", "ThCount[0]", io.ThCount[0], Format.X8));
		entries.Add(new RegEntry("", "ThCount[1]", io.ThCount[1], Format.X8));
		entries.Add(new RegEntry("", "ThState[0]", io.ThState[0], Format.X8));
		entries.Add(new RegEntry("", "ThState[1]", io.ThState[1], Format.X8));
		entries.Add(new RegEntry("", "TMSS"));
		entries.Add(new RegEntry("", "TmssEnabled", io.TmssEnabled, Format.X8));
		entries.Add(new RegEntry("", "TmssUnlocked", io.TmssUnlocked, Format.X8));

		entries.Add(new RegEntry("", "Sega CD Transcript"));
		entries.Add(new RegEntry("", "Flag Legend"));
		entries.Add(new RegEntry("", "0x01", "Write operation bit", 0x01));
		entries.Add(new RegEntry("", "0x02", "Response lane bit", 0x02));
		entries.Add(new RegEntry("", "0x04", "Lane group G2", 0x04));
		entries.Add(new RegEntry("", "0x08", "Lane group G3", 0x08));
		entries.Add(new RegEntry("", "0x10", "Lane group G4", 0x10));
		entries.Add(new RegEntry("", "0x20", "Lane group G5", 0x20));
		entries.Add(new RegEntry("", "0x40", "Lane group G6", 0x40));
		entries.Add(new RegEntry("", "0x80", "Runtime handshake marker", 0x80));
		entries.Add(new RegEntry("", "LaneCount", io.TranscriptLaneCount));
		entries.Add(new RegEntry("", "LaneDigestHi", (uint)(io.TranscriptLaneDigest >> 32), Format.X32));
		entries.Add(new RegEntry("", "LaneDigestLo", (uint)(io.TranscriptLaneDigest & 0xFFFFFFFF), Format.X32));

		for (int i = 0; i < 4; i++) {
			uint address = io.TranscriptEntryAddress[i];
			byte value = io.TranscriptEntryValue[i];
			byte flags = io.TranscriptEntryFlags[i];
			string decodedFlags = DecodeTranscriptFlags(flags);
			entries.Add(new RegEntry("", "Entry[" + i + "] Addr", address, Format.X32));
			entries.Add(new RegEntry("", "Entry[" + i + "] Value", value, Format.X8));
			entries.Add(new RegEntry("", "Entry[" + i + "] Flags", flags, Format.X8));
			entries.Add(new RegEntry("", "Entry[" + i + "] Decoded", decodedFlags, flags));
		}

		entries.Add(new RegEntry("", "Debug Transcript"));
		entries.Add(new RegEntry("", "DebugLaneCount", io.DebugTranscriptLaneCount));
		entries.Add(new RegEntry("", "DebugLaneDigestHi", (uint)(io.DebugTranscriptLaneDigest >> 32), Format.X32));
		entries.Add(new RegEntry("", "DebugLaneDigestLo", (uint)(io.DebugTranscriptLaneDigest & 0xFFFFFFFF), Format.X32));

		for (int i = 0; i < 4; i++) {
			uint address = io.DebugTranscriptEntryAddress[i];
			byte value = io.DebugTranscriptEntryValue[i];
			byte flags = io.DebugTranscriptEntryFlags[i];
			string decodedFlags = DecodeTranscriptFlags(flags);
			entries.Add(new RegEntry("", "DebugEntry[" + i + "] Addr", address, Format.X32));
			entries.Add(new RegEntry("", "DebugEntry[" + i + "] Value", value, Format.X8));
			entries.Add(new RegEntry("", "DebugEntry[" + i + "] Flags", flags, Format.X8));
			entries.Add(new RegEntry("", "DebugEntry[" + i + "] Decoded", decodedFlags, flags));
		}

		return new RegisterViewerTab("I/O", entries, CpuType.Genesis);
	}
}
