using System.Collections.Generic;
using Nexen.Debugger.ViewModels;
using Nexen.Interop;
using static Nexen.Debugger.ViewModels.RegEntry;

namespace Nexen.Debugger.RegisterViewer;

/// <summary>
/// Register viewer for the Atari 2600 debugger.
/// Displays CPU (6507), TIA (graphics + audio), and RIOT (timer + I/O) state.
/// The 2600 has no VRAM, tiles, or sprites in the traditional sense.
/// Graphics are composed of player/missile/ball objects and a playfield register.
/// </summary>
public sealed class Atari2600RegisterViewer {
	/// <summary>Build all register viewer tabs for the Atari 2600 debugger.</summary>
	public static List<RegisterViewerTab> GetTabs(ref Atari2600State state) {
		List<RegisterViewerTab> tabs = new() {
			GetCpuTab(ref state),
			GetTiaTab(ref state),
			GetRiotTab(ref state),
			GetAudioTab(ref state),
		};

		return tabs;
	}

	private static RegisterViewerTab GetCpuTab(ref Atari2600State state) {
		ref Atari2600CpuState cpu = ref state.Cpu;

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
			new RegEntry("", "Cycle Count", cpu.CycleCount),
			new RegEntry("", "IRQ Flag", cpu.IRQFlag, Format.X8),
			new RegEntry("", "NMI Flag", cpu.NmiFlag),
		};

		return new RegisterViewerTab("CPU", entries, CpuType.Atari2600, MemoryType.Atari2600Memory);
	}

	private static RegisterViewerTab GetTiaTab(ref Atari2600State state) {
		ref Atari2600TiaState tia = ref state.Tia;

		// NUSIZ player/missile number-size decode
		string nusiz0Desc = DecodeNusiz(tia.Nusiz0);
		string nusiz1Desc = DecodeNusiz(tia.Nusiz1);

		List<RegEntry> entries = new() {
			new RegEntry("", "Frame", tia.FrameCount),
			new RegEntry("", "Scanline", tia.Scanline),
			new RegEntry("", "Color Clock", tia.ColorClock),
			new RegEntry("", "WSYNC Hold", tia.WsyncHold),
			new RegEntry("", "HMOVE Pending", tia.HmovePending),
			new RegEntry("", ""),

			// Colors
			new RegEntry("", "Colors"),
			new RegEntry("$09", "  Background", tia.ColorBackground, Format.X8),
			new RegEntry("$08", "  Playfield", tia.ColorPlayfield, Format.X8),
			new RegEntry("$06", "  Player 0", tia.ColorPlayer0, Format.X8),
			new RegEntry("$07", "  Player 1", tia.ColorPlayer1, Format.X8),
			new RegEntry("", ""),

			// Playfield
			new RegEntry("", "Playfield"),
			new RegEntry("$0D", "  PF0", tia.Playfield0, Format.X8),
			new RegEntry("$0E", "  PF1", tia.Playfield1, Format.X8),
			new RegEntry("$0F", "  PF2", tia.Playfield2, Format.X8),
			new RegEntry("", "  Reflect", tia.PlayfieldReflect),
			new RegEntry("", "  Score Mode", tia.PlayfieldScoreMode),
			new RegEntry("", "  Priority", tia.PlayfieldPriority),
			new RegEntry("", ""),

			// Players
			new RegEntry("", "Player 0"),
			new RegEntry("$1B", "  GRP0", tia.Player0Graphics, Format.X8),
			new RegEntry("", "  X Pos", tia.Player0X),
			new RegEntry("", "  Reflect", tia.Player0Reflect),
			new RegEntry("$04", "  NUSIZ0", tia.Nusiz0, Format.X8),
			new RegEntry("", "    " + nusiz0Desc),
			new RegEntry("$20", "  HMP0", tia.MotionPlayer0),
			new RegEntry("", "  VDEL", tia.VdelPlayer0),
			new RegEntry("", ""),

			new RegEntry("", "Player 1"),
			new RegEntry("$1C", "  GRP1", tia.Player1Graphics, Format.X8),
			new RegEntry("", "  X Pos", tia.Player1X),
			new RegEntry("", "  Reflect", tia.Player1Reflect),
			new RegEntry("$05", "  NUSIZ1", tia.Nusiz1, Format.X8),
			new RegEntry("", "    " + nusiz1Desc),
			new RegEntry("$21", "  HMP1", tia.MotionPlayer1),
			new RegEntry("", "  VDEL", tia.VdelPlayer1),
			new RegEntry("", ""),

			// Missiles
			new RegEntry("", "Missiles"),
			new RegEntry("$1D", "  M0 Enabled", tia.Missile0Enabled),
			new RegEntry("", "  M0 X Pos", tia.Missile0X),
			new RegEntry("$22", "  HMM0", tia.MotionMissile0),
			new RegEntry("", "  M0 Reset→P0", tia.Missile0ResetToPlayer),
			new RegEntry("$1E", "  M1 Enabled", tia.Missile1Enabled),
			new RegEntry("", "  M1 X Pos", tia.Missile1X),
			new RegEntry("$23", "  HMM1", tia.MotionMissile1),
			new RegEntry("", "  M1 Reset→P1", tia.Missile1ResetToPlayer),
			new RegEntry("", ""),

			// Ball
			new RegEntry("", "Ball"),
			new RegEntry("$1F", "  Enabled", tia.BallEnabled),
			new RegEntry("", "  X Pos", tia.BallX),
			new RegEntry("", "  Size", tia.BallSize),
			new RegEntry("$24", "  HMBL", tia.MotionBall),
			new RegEntry("", "  VDEL", tia.VdelBall),
			new RegEntry("", ""),

			// Collisions
			new RegEntry("", "Collisions"),
			new RegEntry("$30", "  CXM0P", tia.CollisionCxm0p, Format.X8),
			new RegEntry("$31", "  CXM1P", tia.CollisionCxm1p, Format.X8),
			new RegEntry("$32", "  CXP0FB", tia.CollisionCxp0fb, Format.X8),
			new RegEntry("$33", "  CXP1FB", tia.CollisionCxp1fb, Format.X8),
			new RegEntry("$34", "  CXM0FB", tia.CollisionCxm0fb, Format.X8),
			new RegEntry("$35", "  CXM1FB", tia.CollisionCxm1fb, Format.X8),
			new RegEntry("$36", "  CXBLPF", tia.CollisionCxblpf, Format.X8),
			new RegEntry("$37", "  CXPPMM", tia.CollisionCxppmm, Format.X8),
		};

		return new RegisterViewerTab("TIA", entries, CpuType.Atari2600, MemoryType.Atari2600TiaRegisters);
	}

	private static RegisterViewerTab GetRiotTab(ref Atari2600State state) {
		ref Atari2600RiotState riot = ref state.Riot;

		// Decode console switches from PortB
		bool p0Difficulty = (riot.PortB & 0x40) != 0; // bit 6
		bool p1Difficulty = (riot.PortB & 0x80) != 0; // bit 7
		bool colorBW = (riot.PortB & 0x08) != 0;      // bit 3
		bool selectSwitch = (riot.PortB & 0x02) == 0;  // bit 1 (active low)
		bool resetSwitch = (riot.PortB & 0x01) == 0;   // bit 0 (active low)

		// Decode timer divider name
		string timerMode = riot.TimerDivider switch {
			1 => "TIM1T (÷1)",
			8 => "TIM8T (÷8)",
			64 => "TIM64T (÷64)",
			1024 => "T1024T (÷1024)",
			_ => $"÷{riot.TimerDivider}"
		};

		List<RegEntry> entries = new() {
			new RegEntry("", "Timer"),
			new RegEntry("$0284", "  INTIM", (byte)(riot.Timer & 0xff), Format.X8),
			new RegEntry("", "  Full Value", riot.Timer),
			new RegEntry("", "  Mode", timerMode, null),
			new RegEntry("", "  Divider Counter", riot.TimerDividerCounter),
			new RegEntry("", "  Underflowed", riot.TimerUnderflow),
			new RegEntry("$0285", "  Interrupt Flag", riot.InterruptFlag),
			new RegEntry("", ""),

			new RegEntry("", "I/O Ports"),
			new RegEntry("$0280", "  SWCHA (Port A)", riot.PortA, Format.X8),
			new RegEntry("$0281", "  SWACNT (Dir A)", riot.PortADirection, Format.X8),
			new RegEntry("", "  Port A Input", riot.PortAInput, Format.X8),
			new RegEntry("$0282", "  SWCHB (Port B)", riot.PortB, Format.X8),
			new RegEntry("$0283", "  SWBCNT (Dir B)", riot.PortBDirection, Format.X8),
			new RegEntry("", "  Port B Input", riot.PortBInput, Format.X8),
			new RegEntry("", ""),

			new RegEntry("", "Console Switches"),
			new RegEntry("", "  P0 Difficulty", p0Difficulty ? "B (easy)" : "A (hard)", null),
			new RegEntry("", "  P1 Difficulty", p1Difficulty ? "B (easy)" : "A (hard)", null),
			new RegEntry("", "  Color/BW", colorBW ? "Color" : "B/W", null),
			new RegEntry("", "  Select", selectSwitch),
			new RegEntry("", "  Reset", resetSwitch),
		};

		return new RegisterViewerTab("RIOT", entries, CpuType.Atari2600, MemoryType.Atari2600Memory);
	}

	private static RegisterViewerTab GetAudioTab(ref Atari2600State state) {
		ref Atari2600TiaState tia = ref state.Tia;

		string ch0Type = DecodeAudioControl(tia.AudioControl0);
		string ch1Type = DecodeAudioControl(tia.AudioControl1);

		List<RegEntry> entries = new() {
			new RegEntry("", "Channel 0"),
			new RegEntry("$15", "  AUDC0", tia.AudioControl0, Format.X8),
			new RegEntry("", "    Type: " + ch0Type),
			new RegEntry("$17", "  AUDF0", tia.AudioFrequency0, Format.X8),
			new RegEntry("$19", "  AUDV0", tia.AudioVolume0, Format.X8),
			new RegEntry("", "  Counter", tia.AudioCounter0),
			new RegEntry("", "  Phase", tia.AudioPhase0, Format.X8),
			new RegEntry("", ""),

			new RegEntry("", "Channel 1"),
			new RegEntry("$16", "  AUDC1", tia.AudioControl1, Format.X8),
			new RegEntry("", "    Type: " + ch1Type),
			new RegEntry("$18", "  AUDF1", tia.AudioFrequency1, Format.X8),
			new RegEntry("$1A", "  AUDV1", tia.AudioVolume1, Format.X8),
			new RegEntry("", "  Counter", tia.AudioCounter1),
			new RegEntry("", "  Phase", tia.AudioPhase1, Format.X8),
			new RegEntry("", ""),

			new RegEntry("", "Mix"),
			new RegEntry("", "  Last Sample", tia.LastMixedSample, Format.X8),
			new RegEntry("", "  Sample Count", tia.AudioSampleCount),
		};

		return new RegisterViewerTab("Audio", entries, CpuType.Atari2600, MemoryType.Atari2600TiaRegisters);
	}

	/// <summary>Decode NUSIZ register to human-readable description.</summary>
	private static string DecodeNusiz(byte nusiz) {
		byte playerSize = (byte)(nusiz & 0x07);
		byte missileSize = (byte)((nusiz >> 4) & 0x03);
		string playerDesc = playerSize switch {
			0 => "One copy",
			1 => "Two close",
			2 => "Two medium",
			3 => "Three close",
			4 => "Two wide",
			5 => "Double-size",
			6 => "Three medium",
			7 => "Quad-size",
			_ => "?"
		};
		int missilePx = 1 << missileSize;
		return $"{playerDesc}, M={missilePx}px";
	}

	/// <summary>Decode AUDC (audio control) register to waveform name.</summary>
	private static string DecodeAudioControl(byte audc) {
		return (audc & 0x0f) switch {
			0x00 => "Silent",
			0x01 => "Buzzy (4-bit poly)",
			0x02 => "Buzzy/Rumble (÷15→4-bit)",
			0x03 => "Buzzy/Flangy (5-bit→4-bit)",
			0x04 or 0x05 => "Pure Tone (÷2)",
			0x06 or 0x07 => "Bass (÷31)",
			0x08 => "White Noise (9-bit poly)",
			0x09 => "White Noise (5-bit poly)",
			0x0a => "Half-Volume (÷31)",
			0x0b => "Set Last 4 bits",
			0x0c or 0x0d => "Pure Tone (÷6)",
			0x0e or 0x0f => "Pure Tone (÷93)",
			_ => "Unknown"
		};
	}
}
