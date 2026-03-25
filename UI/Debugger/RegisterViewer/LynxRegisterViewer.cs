using System;
using System.Collections.Generic;
using Nexen.Debugger.ViewModels;
using Nexen.Interop;
using static Nexen.Debugger.ViewModels.RegEntry;

namespace Nexen.Debugger.RegisterViewer;

/// <summary>
/// Register viewer for the Atari Lynx debugger.
/// Displays CPU (65SC02), Mikey (timers, display, UART/ComLynx), and Suzy (sprites, math) state.
/// UART registers reference §2 (SERCTL write), §3 (SERCTL read), §4 (SERDAT).
/// </summary>
public sealed class LynxRegisterViewer {
	/// <summary>Build all register viewer tabs for the Lynx debugger.</summary>
	public static List<RegisterViewerTab> GetTabs(ref LynxState state) {
		List<RegisterViewerTab> tabs = new() {
			GetCpuTab(ref state),
			GetMikeyTab(ref state),
			GetSuzyTab(ref state),
			GetMathTab(ref state),
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
			new RegEntry("$FC10", "SCB Address", suzy.SCBAddress, Format.X16),
			new RegEntry("$FC04", "Sprite Control 0", suzy.SpriteControl0, Format.X8),
			new RegEntry("$FC05", "Sprite Control 1", suzy.SpriteControl1, Format.X8),
			new RegEntry("$FC08", "Sprite Init", suzy.SpriteInit, Format.X8),
			new RegEntry("", ""),
			new RegEntry("", "SPRSYS Flags"),
			new RegEntry("", "  Unsafe Access", suzy.UnsafeAccess),
			new RegEntry("", "  Sprite Collision", suzy.SpriteToSpriteCollision),
			new RegEntry("", "  Stop On Current", suzy.StopOnCurrent),
			new RegEntry("", "  VStretch", suzy.VStretch),
			new RegEntry("", "  LeftHand", suzy.LeftHand),
			new RegEntry("", ""),
			new RegEntry("$FCB0", "Joystick", suzy.Joystick, Format.X8),
			new RegEntry("$FCB1", "Switches", suzy.Switches, Format.X8),
		};

		return new RegisterViewerTab("Suzy", entries, CpuType.Lynx, MemoryType.LynxMemory);
	}

	private static RegisterViewerTab GetMathTab(ref LynxState state) {
		ref LynxSuzyState suzy = ref state.Suzy;

		List<RegEntry> entries = new() {
			new RegEntry("", "Multiply Operands (ABCD)"),
			new RegEntry("$FC52-55", "ABCD", suzy.MathABCD, Format.X32),
			new RegEntry("$FC55", "  A (byte 3)", (byte)(suzy.MathABCD >> 24), Format.X8),
			new RegEntry("$FC54", "  B (byte 2)", (byte)(suzy.MathABCD >> 16), Format.X8),
			new RegEntry("$FC53", "  C (byte 1)", (byte)(suzy.MathABCD >> 8), Format.X8),
			new RegEntry("$FC52", "  D (byte 0)", (byte)(suzy.MathABCD), Format.X8),
			new RegEntry("", ""),
			new RegEntry("", "Result / Dividend (EFGH)"),
			new RegEntry("$FC60-63", "EFGH", suzy.MathEFGH, Format.X32),
			new RegEntry("$FC63", "  E (byte 3)", (byte)(suzy.MathEFGH >> 24), Format.X8),
			new RegEntry("$FC62", "  F (byte 2)", (byte)(suzy.MathEFGH >> 16), Format.X8),
			new RegEntry("$FC61", "  G (byte 1)", (byte)(suzy.MathEFGH >> 8), Format.X8),
			new RegEntry("$FC60", "  H (byte 0)", (byte)(suzy.MathEFGH), Format.X8),
			new RegEntry("", ""),
			new RegEntry("", "Accumulator / Remainder (JKLM)"),
			new RegEntry("$FC6C-6F", "JKLM", suzy.MathJKLM, Format.X32),
			new RegEntry("$FC6F", "  J (byte 3)", (byte)(suzy.MathJKLM >> 24), Format.X8),
			new RegEntry("$FC6E", "  K (byte 2)", (byte)(suzy.MathJKLM >> 16), Format.X8),
			new RegEntry("$FC6D", "  L (byte 1)", (byte)(suzy.MathJKLM >> 8), Format.X8),
			new RegEntry("$FC6C", "  M (byte 0)", (byte)(suzy.MathJKLM), Format.X8),
			new RegEntry("", ""),
			new RegEntry("", "Divisor (NP)"),
			new RegEntry("$FC56-57", "NP", suzy.MathNP, Format.X16),
			new RegEntry("$FC57", "  N (byte 1)", (byte)(suzy.MathNP >> 8), Format.X8),
			new RegEntry("$FC56", "  P (byte 0)", (byte)(suzy.MathNP), Format.X8),
			new RegEntry("", ""),
			new RegEntry("", "Status"),
			new RegEntry("", "  AB Sign", suzy.MathAB_sign),
			new RegEntry("", "  CD Sign", suzy.MathCD_sign),
			new RegEntry("", "  EFGH Sign", suzy.MathEFGH_sign),
			new RegEntry("", "  Signed Mode", suzy.MathSign),
			new RegEntry("", "  Accumulate", suzy.MathAccumulate),
			new RegEntry("", "  In Progress", suzy.MathInProgress),
			new RegEntry("", "  Overflow", suzy.MathOverflow),
			new RegEntry("", "  Last Carry", suzy.LastCarry),
		};

		return new RegisterViewerTab("Math", entries, CpuType.Lynx, MemoryType.LynxMemory);
	}
}
