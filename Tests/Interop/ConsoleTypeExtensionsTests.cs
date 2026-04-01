using Nexen.Interop;
using Xunit;

namespace Nexen.Tests.Interop;

public sealed class ConsoleTypeExtensionsTests {
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
