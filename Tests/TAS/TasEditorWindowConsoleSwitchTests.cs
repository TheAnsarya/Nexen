using Nexen.MovieConverter;
using Nexen.ViewModels;
using Nexen.Windows;
using Xunit;

namespace Nexen.Tests.TAS;

/// <summary>
/// Verifies Atari 2600 console-switch mapping used by TAS playback.
/// </summary>
public class TasEditorWindowConsoleSwitchTests {
	[Fact]
	public void ComputeAtari2600ConsoleSwitchState_NonAtariLayout_DoesNotApply() {
		var result = TasEditorWindow.ComputeAtari2600ConsoleSwitchState(
			ControllerLayout.Lynx,
			FrameCommand.Atari2600Select | FrameCommand.Atari2600Reset);

		Assert.False(result.Apply);
		Assert.False(result.Select);
		Assert.False(result.Reset);
	}

	[Fact]
	public void ComputeAtari2600ConsoleSwitchState_AtariLayout_WithNoFlags_ClearsBoth() {
		var result = TasEditorWindow.ComputeAtari2600ConsoleSwitchState(
			ControllerLayout.Atari2600,
			FrameCommand.None);

		Assert.True(result.Apply);
		Assert.False(result.Select);
		Assert.False(result.Reset);
	}

	[Fact]
	public void ComputeAtari2600ConsoleSwitchState_AtariLayout_WithSelectOnly_SetsSelect() {
		var result = TasEditorWindow.ComputeAtari2600ConsoleSwitchState(
			ControllerLayout.Atari2600,
			FrameCommand.Atari2600Select);

		Assert.True(result.Apply);
		Assert.True(result.Select);
		Assert.False(result.Reset);
	}

	[Fact]
	public void ComputeAtari2600ConsoleSwitchState_AtariLayout_WithResetOnly_SetsReset() {
		var result = TasEditorWindow.ComputeAtari2600ConsoleSwitchState(
			ControllerLayout.Atari2600,
			FrameCommand.Atari2600Reset);

		Assert.True(result.Apply);
		Assert.False(result.Select);
		Assert.True(result.Reset);
	}

	[Fact]
	public void ComputeAtari2600ConsoleSwitchState_AtariLayout_WithBothFlags_SetsBoth() {
		var result = TasEditorWindow.ComputeAtari2600ConsoleSwitchState(
			ControllerLayout.Atari2600,
			FrameCommand.Atari2600Select | FrameCommand.Atari2600Reset);

		Assert.True(result.Apply);
		Assert.True(result.Select);
		Assert.True(result.Reset);
	}
}
