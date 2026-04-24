using Nexen.Interop;
using Xunit;

namespace Nexen.Tests.Interop;

public sealed class ConsoleTypeExtensionsTests {
	[Theory]
	[InlineData(ConsoleType.Snes, CpuType.Snes)]
	[InlineData(ConsoleType.Nes, CpuType.Nes)]
	[InlineData(ConsoleType.Gameboy, CpuType.Gameboy)]
	[InlineData(ConsoleType.PcEngine, CpuType.Pce)]
	[InlineData(ConsoleType.Sms, CpuType.Sms)]
	[InlineData(ConsoleType.Gba, CpuType.Gba)]
	[InlineData(ConsoleType.Ws, CpuType.Ws)]
	[InlineData(ConsoleType.Lynx, CpuType.Lynx)]
	[InlineData(ConsoleType.Genesis, CpuType.Genesis)]
	[InlineData(ConsoleType.Atari2600, CpuType.Atari2600)]
	[InlineData(ConsoleType.ChannelF, CpuType.ChannelF)]
	public void GetMainCpuType_ReturnsExpectedValue(ConsoleType consoleType, CpuType expectedCpuType) {
		Assert.Equal(expectedCpuType, consoleType.GetMainCpuType());
	}

	[Theory]
	[InlineData(ConsoleType.ChannelF, true)]
	[InlineData(ConsoleType.Nes, true)]
	[InlineData(ConsoleType.Snes, true)]
	[InlineData(ConsoleType.Gameboy, true)]
	public void SupportsTasEditor_ReturnsExpectedValue(ConsoleType consoleType, bool expected) {
		Assert.Equal(expected, consoleType.SupportsTasEditor());
	}

	[Theory]
	[InlineData(ConsoleType.ChannelF, true)]
	[InlineData(ConsoleType.Nes, true)]
	[InlineData(ConsoleType.Snes, true)]
	[InlineData(ConsoleType.Gameboy, true)]
	public void SupportsMovieTools_ReturnsExpectedValue(ConsoleType consoleType, bool expected) {
		Assert.Equal(expected, consoleType.SupportsMovieTools());
	}
}
