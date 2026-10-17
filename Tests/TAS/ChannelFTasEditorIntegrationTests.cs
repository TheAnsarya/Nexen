using System.Reflection;
using Nexen.MovieConverter;
using Nexen.ViewModels;
using Xunit;

namespace Nexen.Tests.TAS;

/// <summary>
/// Integration coverage for Channel F TAS editor behavior.
/// Ensures Channel F movies auto-select the Channel F layout and expose
/// the expected gesture-centric lanes in the editor.
/// </summary>
public sealed class ChannelFTasEditorIntegrationTests : System.IDisposable {
	private readonly TasEditorViewModel _vm = new();

	public void Dispose() {
		_vm.Dispose();
	}

	private static MovieData CreateChannelFMovie(int frameCount) {
		var movie = new MovieData {
			Author = "ChannelFTest",
			GameName = "Channel F Test",
			SystemType = SystemType.ChannelF,
			Region = RegionType.NTSC,
		};

		for (int i = 0; i < frameCount; i++) {
			movie.InputFrames.Add(new InputFrame(i) {
				Controllers = [new ControllerInput()]
			});
		}

		return movie;
	}

	private void SetMovie(MovieData movie) {
		var prop = typeof(TasEditorViewModel).GetProperty(nameof(TasEditorViewModel.Movie), BindingFlags.Instance | BindingFlags.Public);
		Assert.NotNull(prop);
		prop!.SetValue(_vm, movie);
		_vm.Recorder.Movie = movie;
	}

	[Fact]
	public void ChannelFMovie_DetectsChannelFLayout() {
		SetMovie(CreateChannelFMovie(3));
		Assert.Equal(ControllerLayout.ChannelF, _vm.CurrentLayout);
	}

	[Fact]
	public void ChannelFMovie_ExposesExpectedPianoRollLanes() {
		SetMovie(CreateChannelFMovie(1));
		Assert.Equal(ControllerLayout.ChannelF, _vm.CurrentLayout);

		var labels = _vm.PianoRollButtonLabels;
		Assert.Equal(8, labels.Count);
		Assert.Contains("Right", labels);
		Assert.Contains("Left", labels);
		Assert.Contains("Back", labels);
		Assert.Contains("Fwd", labels);
		Assert.Contains("TwL", labels);
		Assert.Contains("TwR", labels);
		Assert.Contains("Pull", labels);
		Assert.Contains("Push", labels);
	}
}
