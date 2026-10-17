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
	[InlineData(true, ConsoleType.Genesis, ConfigWindowTab.Genesis)]
	[InlineData(true, ConsoleType.Ws, ConfigWindowTab.Ws)]
	[InlineData(true, ConsoleType.Lynx, ConfigWindowTab.Lynx)]
	[InlineData(true, ConsoleType.Atari2600, ConfigWindowTab.Atari2600)]
	[InlineData(true, ConsoleType.ChannelF, ConfigWindowTab.ChannelF)]
	public void ResolveInputLandingTab_UsesExpectedConfigTab(bool hasLoadedRom, ConsoleType consoleType, ConfigWindowTab expected) {
		var actual = ConfigViewModel.ResolveInputLandingTab(hasLoadedRom, consoleType);
		Assert.Equal(expected, actual);
	}

	[Theory]
	[InlineData(ConfigRouteIds.Audio, ConfigWindowTab.Audio)]
	[InlineData(ConfigRouteIds.Emulation, ConfigWindowTab.Emulation)]
	[InlineData(ConfigRouteIds.Input, ConfigWindowTab.Input)]
	[InlineData(ConfigRouteIds.Video, ConfigWindowTab.Video)]
	[InlineData(ConfigRouteIds.Nes, ConfigWindowTab.Nes)]
	[InlineData(ConfigRouteIds.Snes, ConfigWindowTab.Snes)]
	[InlineData(ConfigRouteIds.Gameboy, ConfigWindowTab.Gameboy)]
	[InlineData(ConfigRouteIds.Gba, ConfigWindowTab.Gba)]
	[InlineData(ConfigRouteIds.PcEngine, ConfigWindowTab.PcEngine)]
	[InlineData(ConfigRouteIds.Sms, ConfigWindowTab.Sms)]
	[InlineData(ConfigRouteIds.Genesis, ConfigWindowTab.Genesis)]
	[InlineData(ConfigRouteIds.Ws, ConfigWindowTab.Ws)]
	[InlineData(ConfigRouteIds.Lynx, ConfigWindowTab.Lynx)]
	[InlineData(ConfigRouteIds.Atari2600, ConfigWindowTab.Atari2600)]
	[InlineData(ConfigRouteIds.ChannelF, ConfigWindowTab.ChannelF)]
	[InlineData(ConfigRouteIds.OtherConsoles, ConfigWindowTab.OtherConsoles)]
	[InlineData(ConfigRouteIds.Preferences, ConfigWindowTab.Preferences)]
	public void TryResolveRoute_MapsStableRouteIds(string routeId, ConfigWindowTab expectedTab) {
		bool resolved = ConfigViewModel.TryResolveRoute(routeId, out ConfigWindowTab actualTab);
		Assert.True(resolved);
		Assert.Equal(expectedTab, actualTab);
	}

	[Fact]
	public void ResolveRoute_InputActiveSystem_UsesCurrentConsole() {
		ConfigWindowTab noRom = ConfigViewModel.ResolveRoute(ConfigRouteIds.InputActiveSystem, false, ConsoleType.Snes);
		ConfigWindowTab snes = ConfigViewModel.ResolveRoute(ConfigRouteIds.InputActiveSystem, true, ConsoleType.Snes);

		Assert.Equal(ConfigWindowTab.Input, noRom);
		Assert.Equal(ConfigWindowTab.Snes, snes);
	}

	[Theory]
	[InlineData(ConfigWindowTab.Audio, ConfigRouteIds.Audio)]
	[InlineData(ConfigWindowTab.Input, ConfigRouteIds.Input)]
	[InlineData(ConfigWindowTab.Preferences, ConfigRouteIds.Preferences)]
	public void GetRouteId_ReturnsExpectedStableId(ConfigWindowTab tab, string expectedRouteId) {
		string routeId = ConfigViewModel.GetRouteId(tab);
		Assert.Equal(expectedRouteId, routeId);
	}
}
