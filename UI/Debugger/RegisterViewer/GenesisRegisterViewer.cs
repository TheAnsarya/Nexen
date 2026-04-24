using System.Collections.Generic;
using Nexen.Debugger.ViewModels;
using Nexen.Interop;
using static Nexen.Debugger.ViewModels.RegEntry;

namespace Nexen.Debugger.RegisterViewer;

public sealed class GenesisRegisterViewer {
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

		entries.AddRange(new List<RegEntry>() {
			new RegEntry("", "State"),
			new RegEntry("", "FrameCount", vdp.FrameCount),
			new RegEntry("", "HCounter", vdp.HCounter),
			new RegEntry("", "VCounter", vdp.VCounter),
			new RegEntry("", "StatusRegister", vdp.StatusRegister, Format.X16),
			new RegEntry("", "AddressRegister", vdp.AddressRegister, Format.X32),
			new RegEntry("", "CodeRegister", vdp.CodeRegister, Format.X8),
			new RegEntry("", "WritePending", vdp.WritePending),
			new RegEntry("", "DataPortBuffer", vdp.DataPortBuffer, Format.X16),
			new RegEntry("", "HIntCounter", vdp.HIntCounter, Format.X8),
			new RegEntry("", "DMA"),
			new RegEntry("", "DmaActive", vdp.DmaActive),
			new RegEntry("", "DmaMode", vdp.DmaMode, Format.X8),
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

		return new RegisterViewerTab("I/O", entries, CpuType.Genesis);
	}
}
