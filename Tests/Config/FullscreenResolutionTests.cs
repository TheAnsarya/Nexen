using Nexen.Config;
using Xunit;

namespace Nexen.Tests.Config;

/// <summary>
/// Tests for FullscreenResolution extension methods after the dictionary lookup optimization.
/// </summary>
public sealed class FullscreenResolutionTests {
	[Theory]
	[InlineData(FullscreenResolution._3840x2160, 3840, 2160)]
	[InlineData(FullscreenResolution._2560x1440, 2560, 1440)]
	[InlineData(FullscreenResolution._1920x1080, 1920, 1080)]
	[InlineData(FullscreenResolution._1680x1050, 1680, 1050)]
	[InlineData(FullscreenResolution._1600x900, 1600, 900)]
	[InlineData(FullscreenResolution._1366x768, 1366, 768)]
	[InlineData(FullscreenResolution._1280x720, 1280, 720)]
	[InlineData(FullscreenResolution._1024x768, 1024, 768)]
	[InlineData(FullscreenResolution._800x600, 800, 600)]
	[InlineData(FullscreenResolution._640x480, 640, 480)]
	public void GetWidth_ReturnsCorrectValue(FullscreenResolution res, int expectedWidth, int _) {
		Assert.Equal(expectedWidth, res.GetWidth());
	}

	[Theory]
	[InlineData(FullscreenResolution._3840x2160, 3840, 2160)]
	[InlineData(FullscreenResolution._2560x1440, 2560, 1440)]
	[InlineData(FullscreenResolution._1920x1080, 1920, 1080)]
	[InlineData(FullscreenResolution._1680x1050, 1680, 1050)]
	[InlineData(FullscreenResolution._1600x900, 1600, 900)]
	[InlineData(FullscreenResolution._1366x768, 1366, 768)]
	[InlineData(FullscreenResolution._1280x720, 1280, 720)]
	[InlineData(FullscreenResolution._1024x768, 1024, 768)]
	[InlineData(FullscreenResolution._800x600, 800, 600)]
	[InlineData(FullscreenResolution._640x480, 640, 480)]
	public void GetHeight_ReturnsCorrectValue(FullscreenResolution res, int _, int expectedHeight) {
		Assert.Equal(expectedHeight, res.GetHeight());
	}

	[Fact]
	public void Default_ReturnsZero() {
		Assert.Equal(0, FullscreenResolution.Default.GetWidth());
		Assert.Equal(0, FullscreenResolution.Default.GetHeight());
	}

	[Fact]
	public void AllResolutions_HaveValidDimensions() {
		foreach (var res in Enum.GetValues<FullscreenResolution>()) {
			int width = res.GetWidth();
			int height = res.GetHeight();

			if (res == FullscreenResolution.Default) {
				Assert.Equal(0, width);
				Assert.Equal(0, height);
			} else {
				Assert.True(width > 0, $"{res} should have positive width");
				Assert.True(height > 0, $"{res} should have positive height");
				Assert.True(width >= height, $"{res} width ({width}) should be >= height ({height})");
			}
		}
	}

	[Fact]
	public void AllNonDefault_AreInDescendingWidthOrder() {
		var resolutions = Enum.GetValues<FullscreenResolution>()
			.Where(r => r != FullscreenResolution.Default)
			.ToArray();

		for (int i = 1; i < resolutions.Length; i++) {
			int prevWidth = resolutions[i - 1].GetWidth();
			int currWidth = resolutions[i].GetWidth();
			Assert.True(prevWidth >= currWidth,
				$"{resolutions[i - 1]} ({prevWidth}) should be >= {resolutions[i]} ({currWidth})");
		}
	}
}
