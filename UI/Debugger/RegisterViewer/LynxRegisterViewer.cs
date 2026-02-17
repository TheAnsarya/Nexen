using System;
using System.Collections.Generic;
using Nexen.Debugger.ViewModels;
using Nexen.Interop;
using static Nexen.Debugger.ViewModels.RegEntry;

namespace Nexen.Debugger.RegisterViewer;

public sealed class LynxRegisterViewer {
	public static List<RegisterViewerTab> GetTabs(ref LynxState state) {
		List<RegisterViewerTab> tabs = new() {
			GetCpuTab(ref state),
			GetMikeyTab(ref state),
			GetSuzyTab(ref state),
		};

		return tabs;
	}

	private static RegisterViewerTab GetCpuTab(ref LynxState state) {
		ref LynxCpuState cpu = ref state.Cpu;

		List<RegEntry> entries = new() {
			new RegEntry("", "A", cpu.A, Format.X8),
			new RegEntry("", "X", cpu.X, Format.X8),
			new RegEntry("", "Y", cpu.Y, Format.X8),
			new RegEntry("", "SP", cpu.SP, Format.X8),
			new RegEntry("", "PC", cpu.PC, Format.X16),
			new RegEntry("", "PS", cpu.PS, Format.X8),
			new RegEntry("", ""),
			new RegEntry("PS.7", "Negative", (cpu.PS & 0x80) != 0),
			new RegEntry("PS.6", "Overflow", (cpu.PS & 0x40) != 0),
			new RegEntry("PS.3", "Decimal", (cpu.PS & 0x08) != 0),
			new RegEntry("PS.2", "IRQ Disable", (cpu.PS & 0x04) != 0),
			new RegEntry("PS.1", "Zero", (cpu.PS & 0x02) != 0),
			new RegEntry("PS.0", "Carry", (cpu.PS & 0x01) != 0),
			new RegEntry("", ""),
			new RegEntry("", "IRQ Flag", cpu.IRQFlag, Format.X8),
			new RegEntry("", "NMI Flag", cpu.NmiFlag),
			new RegEntry("", "Stop State", cpu.StopState),
		};

		return new RegisterViewerTab("CPU", entries, CpuType.Lynx, MemoryType.LynxMemory);
	}

	private static RegisterViewerTab GetMikeyTab(ref LynxState state) {
		ref LynxMikeyState mikey = ref state.Mikey;

		List<RegEntry> entries = new() {
			new RegEntry("$FD80", "IRQ Enabled", mikey.IrqEnabled, Format.X8),
			new RegEntry("$FD81", "IRQ Pending", mikey.IrqPending, Format.X8),
			new RegEntry("", ""),
			new RegEntry("$FD92", "Display Control", mikey.DisplayControl, Format.X8),
			new RegEntry("", "Display Address", mikey.DisplayAddress, Format.X16),
			new RegEntry("", "Current Scanline", mikey.CurrentScanline),
			new RegEntry("", ""),
			new RegEntry("$FD8C", "Serial Control (SERCTL)", mikey.SerialControl, Format.X8),
			new RegEntry("", "  TX IRQ Enable", mikey.UartTxIrqEnable),
			new RegEntry("", "  RX IRQ Enable", mikey.UartRxIrqEnable),
			new RegEntry("", "  Parity Enable", mikey.UartParityEnable),
			new RegEntry("", "  Parity Even", mikey.UartParityEven),
			new RegEntry("", "  Send Break", mikey.UartSendBreak),
			new RegEntry("", "RX Data", (byte)(mikey.UartRxData & 0xFF), Format.X8),
			new RegEntry("", "  RX Ready", mikey.UartRxReady),
			new RegEntry("", "  Overrun Error", mikey.UartRxOverrunError),
			new RegEntry("", "  Framing Error", mikey.UartRxFramingError),
			new RegEntry("", "TX Data", (byte)(mikey.UartTxData & 0xFF), Format.X8),
			new RegEntry("", ""),
		};

		// Add timer entries
		for (int i = 0; i < 8 && mikey.Timers != null; i++) {
			ref LynxTimerState t = ref mikey.Timers[i];
			entries.Add(new RegEntry("", $"Timer {i}"));
			entries.Add(new RegEntry($"T{i}", "  Count", t.Count, Format.X8));
			entries.Add(new RegEntry($"T{i}", "  Backup", t.BackupValue, Format.X8));
			entries.Add(new RegEntry($"T{i}", "  Control A", t.ControlA, Format.X8));
			entries.Add(new RegEntry($"T{i}", "  Control B", t.ControlB, Format.X8));
			entries.Add(new RegEntry($"T{i}", "  Done", t.TimerDone));
			entries.Add(new RegEntry($"T{i}", "  Linked", t.Linked));
		}

		return new RegisterViewerTab("Mikey", entries, CpuType.Lynx, MemoryType.LynxMemory);
	}

	private static RegisterViewerTab GetSuzyTab(ref LynxState state) {
		ref LynxSuzyState suzy = ref state.Suzy;

		List<RegEntry> entries = new() {
			new RegEntry("", "Sprite Busy", suzy.SpriteBusy),
			new RegEntry("", "Sprite Enabled", suzy.SpriteEnabled),
			new RegEntry("", "SCB Address", suzy.SCBAddress, Format.X16),
			new RegEntry("", "Sprite Control 0", suzy.SpriteControl0, Format.X8),
			new RegEntry("", "Sprite Control 1", suzy.SpriteControl1, Format.X8),
			new RegEntry("", "Sprite Init", suzy.SpriteInit, Format.X8),
			new RegEntry("", ""),
			new RegEntry("", "Math Registers"),
			new RegEntry("", "  A", suzy.MathA, Format.X16),
			new RegEntry("", "  B", suzy.MathB, Format.X16),
			new RegEntry("", "  C", (UInt16)suzy.MathC, Format.X16),
			new RegEntry("", "  D", (UInt16)suzy.MathD, Format.X16),
			new RegEntry("", "  E", suzy.MathE, Format.X16),
			new RegEntry("", "  F", suzy.MathF, Format.X16),
			new RegEntry("", "  G", suzy.MathG, Format.X16),
			new RegEntry("", "  H", suzy.MathH, Format.X16),
			new RegEntry("", "  J", suzy.MathJ, Format.X16),
			new RegEntry("", "  K", suzy.MathK, Format.X16),
			new RegEntry("", "  M", suzy.MathM, Format.X16),
			new RegEntry("", "  N", suzy.MathN, Format.X16),
			new RegEntry("", "  Sign", suzy.MathSign),
			new RegEntry("", "  Accumulate", suzy.MathAccumulate),
			new RegEntry("", "  In Progress", suzy.MathInProgress),
			new RegEntry("", "  Overflow", suzy.MathOverflow),
			new RegEntry("", ""),
			new RegEntry("", "Joystick", suzy.Joystick, Format.X8),
			new RegEntry("", "Switches", suzy.Switches, Format.X8),
		};

		return new RegisterViewerTab("Suzy", entries, CpuType.Lynx, MemoryType.LynxMemory);
	}
}
