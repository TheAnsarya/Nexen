using Nexen.Interop;
using Nexen.ViewModels;
using Xunit;

namespace Nexen.Tests.ViewModels;

public sealed class ConfigViewModelTests {
	[Theory]
	[InlineData(false, ConsoleType.Snes, ConfigWindowTab.Input)]
	[InlineData(true, ConsoleType.Nes, ConfigWindowTab.Nes)]
	[InlineData(true, ConsoleType.Snes, ConfigWindowTab.Snes)]
	[InlineData(true, ConsoleType.Gameboy, ConfigWindowTab.Gameboy)]
	[InlineData(true, ConsoleType.Gba, ConfigWindowTab.Gba)]
	[InlineData(true, ConsoleType.PcEngine, ConfigWindowTab.PcEngine)]
	[InlineData(true, ConsoleType.Sms, ConfigWindowTab.Sms)]
	[InlineData(true, ConsoleType.Ws, ConfigWindowTab.Ws)]
	[InlineData(true, ConsoleType.Lynx, ConfigWindowTab.Lynx)]
	[InlineData(true, ConsoleType.Atari2600, ConfigWindowTab.Atari2600)]
	[InlineData(true, ConsoleType.ChannelF, ConfigWindowTab.ChannelF)]
	[InlineData(true, ConsoleType.Genesis, ConfigWindowTab.Input)]
	public void ResolveInputLandingTab_UsesExpectedConfigTab(bool hasLoadedRom, ConsoleType consoleType, ConfigWindowTab expected) {
		var actual = ConfigViewModel.ResolveInputLandingTab(hasLoadedRom, consoleType);
		Assert.Equal(expected, actual);
	}
}
