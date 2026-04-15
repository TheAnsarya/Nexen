using Nexen.Windows;
using Xunit;

namespace Nexen.Tests.Windows;

public sealed class MainWindowPauseBehaviorTests {
	[Fact]
	public void ShouldPauseInMenusAndConfig_DisabledOption_ReturnsFalse() {
		bool result = MainWindow.ShouldPauseInMenusAndConfig(
			pauseWhenInMenusAndConfig: false,
			isGameRunning: true,
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
			isConfigWindow: true,
			isMainMenuOpen: false
		);

		Assert.True(result);
	}
}
