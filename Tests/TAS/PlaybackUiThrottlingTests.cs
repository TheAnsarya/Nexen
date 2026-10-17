using Nexen.ViewModels;
using Xunit;

namespace Nexen.Tests.TAS;

public class PlaybackUiThrottlingTests {
	[Theory]
	[InlineData(1.0, 1)]
	[InlineData(2.0, 2)]
	[InlineData(4.0, 4)]
	[InlineData(0.5, 1)]
	public void ComputePlaybackUiUpdateInterval_MapsSpeedToExpectedInterval(double playbackSpeed, int expectedInterval) {
		int interval = TasEditorViewModel.ComputePlaybackUiUpdateInterval(playbackSpeed);

		Assert.Equal(expectedInterval, interval);
	}

	[Fact]
	public void ShouldRefreshPlaybackUi_AtSixtyFps_RefreshesEveryFrame() {
		for (int nextFrame = 0; nextFrame < 20; nextFrame++) {
			Assert.True(TasEditorViewModel.ShouldRefreshPlaybackUi(nextFrame, 100, 1.0));
		}
	}

	[Fact]
	public void ShouldRefreshPlaybackUi_AtOneTwentyFps_RefreshesEveryTwoFrames() {
		Assert.True(TasEditorViewModel.ShouldRefreshPlaybackUi(0, 100, 2.0));
		Assert.False(TasEditorViewModel.ShouldRefreshPlaybackUi(1, 100, 2.0));
		Assert.True(TasEditorViewModel.ShouldRefreshPlaybackUi(2, 100, 2.0));
		Assert.False(TasEditorViewModel.ShouldRefreshPlaybackUi(3, 100, 2.0));
	}

	[Fact]
	public void ShouldRefreshPlaybackUi_AtTwoFortyFps_RefreshesEveryFourFrames() {
		Assert.True(TasEditorViewModel.ShouldRefreshPlaybackUi(0, 300, 4.0));
		Assert.False(TasEditorViewModel.ShouldRefreshPlaybackUi(1, 300, 4.0));
		Assert.False(TasEditorViewModel.ShouldRefreshPlaybackUi(2, 300, 4.0));
		Assert.False(TasEditorViewModel.ShouldRefreshPlaybackUi(3, 300, 4.0));
		Assert.True(TasEditorViewModel.ShouldRefreshPlaybackUi(4, 300, 4.0));
	}

	[Fact]
	public void ShouldRefreshPlaybackUi_FinalFrame_AlwaysRefreshes() {
		Assert.True(TasEditorViewModel.ShouldRefreshPlaybackUi(59, 60, 4.0));
		Assert.True(TasEditorViewModel.ShouldRefreshPlaybackUi(9, 10, 2.0));
	}
}
