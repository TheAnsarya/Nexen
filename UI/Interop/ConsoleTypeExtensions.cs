using System;

namespace Nexen.Interop;
public static class ConsoleTypeExtensions {
	public static CpuType GetMainCpuType(this ConsoleType type) {
		return type switch {
			ConsoleType.Snes => CpuType.Snes,
			ConsoleType.Nes => CpuType.Nes,
			ConsoleType.Gameboy => CpuType.Gameboy,
			ConsoleType.PcEngine => CpuType.Pce,
			ConsoleType.Sms => CpuType.Sms,
			ConsoleType.Gba => CpuType.Gba,
			ConsoleType.Ws => CpuType.Ws,
			ConsoleType.Lynx => CpuType.Lynx,
			ConsoleType.Genesis => CpuType.Genesis,
			ConsoleType.Atari2600 => CpuType.Atari2600,
			ConsoleType.ChannelF => CpuType.ChannelF,
			_ => throw new Exception("Invalid type")
		};
	}

	public static bool SupportsCheats(this ConsoleType type) {
		return type switch {
			ConsoleType.Gba => false,
			ConsoleType.Ws => false,
			ConsoleType.Lynx => false,
			ConsoleType.Atari2600 => false,
			ConsoleType.ChannelF => false,
			_ => true
		};
	}

	public static bool SupportsTasEditor(this ConsoleType type) {
		return type switch {
			ConsoleType.ChannelF => true,
			_ => true
		};
	}

	public static bool SupportsMovieTools(this ConsoleType type) {
		return type switch {
			ConsoleType.ChannelF => true,
			_ => true
		};
	}
}
