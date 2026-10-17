using Nexen.Windows;
using Xunit;

namespace Nexen.Tests.Windows;

public sealed class MainWindowPauseBehaviorTests {
	[Fact]
	public void ShouldPauseInMenusAndConfig_DisabledOption_ReturnsFalse() {
		bool result = MainWindow.ShouldPauseInMenusAndConfig(
			pauseWhenInMenusAndConfig: false,
			isGameRunning: true,
			isRomLoadInProgress: false,
			isConfigWindow: true,
			isMainMenuOpen: true
		);

		Assert.False(result);
	}

	[Fact]
	public void ShouldPauseInMenusAndConfig_NoActiveGame_MenuOpen_ReturnsFalse() {
		bool result = MainWindow.ShouldPauseInMenusAndConfig(
			pauseWhenInMenusAndConfig: true,
			isGameRunning: false,
			isRomLoadInProgress: false,
			isConfigWindow: false,
			isMainMenuOpen: true
		);

		Assert.False(result);
	}

	[Fact]
	public void ShouldPauseInMenusAndConfig_NoActiveGame_ConfigWindowOpen_ReturnsFalse() {
		bool result = MainWindow.ShouldPauseInMenusAndConfig(
			pauseWhenInMenusAndConfig: true,
			isGameRunning: false,
			isRomLoadInProgress: false,
			isConfigWindow: true,
			isMainMenuOpen: false
		);

		Assert.False(result);
	}

	[Fact]
	public void ShouldPauseInMenusAndConfig_ActiveGameAndMenuOpen_ReturnsTrue() {
		bool result = MainWindow.ShouldPauseInMenusAndConfig(
			pauseWhenInMenusAndConfig: true,
			isGameRunning: true,
			isRomLoadInProgress: false,
			isConfigWindow: false,
			isMainMenuOpen: true
		);

		Assert.True(result);
	}

	[Fact]
	public void ShouldPauseInMenusAndConfig_ActiveGameAndConfigWindowOpen_ReturnsTrue() {
		bool result = MainWindow.ShouldPauseInMenusAndConfig(
			pauseWhenInMenusAndConfig: true,
			isGameRunning: true,
			isRomLoadInProgress: false,
			isConfigWindow: true,
			isMainMenuOpen: false
		);

		Assert.True(result);
	}

	[Theory]
	[InlineData(true, false)]
	[InlineData(false, true)]
	public void ShouldPauseInMenusAndConfig_RomLoadInProgress_ReturnsFalse(bool isConfigWindow, bool isMainMenuOpen) {
		bool result = MainWindow.ShouldPauseInMenusAndConfig(
			pauseWhenInMenusAndConfig: true,
			isGameRunning: true,
			isRomLoadInProgress: true,
			isConfigWindow: isConfigWindow,
			isMainMenuOpen: isMainMenuOpen
		);

		Assert.False(result);
	}
}
