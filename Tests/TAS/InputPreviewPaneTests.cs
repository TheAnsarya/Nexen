using System.Reflection;
using Nexen.MovieConverter;
using Nexen.ViewModels;
using Xunit;

namespace Nexen.Tests.TAS;

public class InputPreviewPaneTests {
	private static MovieData CreateMovie(int frameCount, int controllerCount, SystemType systemType) {
		var movie = new MovieData {
			Author = "PreviewTests",
			GameName = "Preview Game",
			SystemType = systemType,
			Region = RegionType.NTSC
		};

		for (int frameIndex = 0; frameIndex < frameCount; frameIndex++) {
			var frame = new InputFrame(frameIndex) {
				Controllers = new ControllerInput[controllerCount]
			};

			for (int port = 0; port < controllerCount; port++) {
				frame.Controllers[port] = new ControllerInput();
			}

			movie.InputFrames.Add(frame);
		}

		return movie;
	}

	private static void SetMovie(TasEditorViewModel vm, MovieData movie) {
		var prop = typeof(TasEditorViewModel).GetProperty(nameof(TasEditorViewModel.Movie));
		prop!.SetValue(vm, movie);
		vm.Recorder.Movie = movie;
	}

	[Fact]
	public void SelectedFrameHexPreview_NoSelection_ShowsPlaceholder() {
		using var vm = new TasEditorViewModel();
		SetMovie(vm, CreateMovie(1, 1, SystemType.Snes));

		Assert.Equal("No frame selected", vm.SelectedFrameHexPreview);
	}

	[Fact]
	public void SelectedFrameHexPreview_SelectedFrame_ShowsPerPortHexBytes() {
		using var vm = new TasEditorViewModel();
		var movie = CreateMovie(1, 2, SystemType.Snes);
		movie.InputFrames[0].Controllers[0].A = true;
		movie.InputFrames[0].Controllers[0].Start = true;
		movie.InputFrames[0].Controllers[0].Up = true;
		movie.InputFrames[0].Controllers[1].B = true;
		movie.InputFrames[0].Controllers[1].Right = true;
		SetMovie(vm, movie);

		vm.SelectedFrameIndex = 0;

		Assert.Contains("P1 [Gamepad] 41 01", vm.SelectedFrameHexPreview);
		Assert.Contains("P2 [Gamepad] 02 08", vm.SelectedFrameHexPreview);
	}

	[Fact]
	public void SelectedFrameHexPreview_IncludesAnalogAndTriggerBytes() {
		using var vm = new TasEditorViewModel();
		var movie = CreateMovie(1, 1, SystemType.Snes);
		movie.InputFrames[0].Controllers[0].AnalogX = -1;
		movie.InputFrames[0].Controllers[0].AnalogY = 1;
		movie.InputFrames[0].Controllers[0].TriggerL = 0x80;
		SetMovie(vm, movie);

		vm.SelectedFrameIndex = 0;

		Assert.Contains("P1 [Gamepad] 00 00 ff 01 80", vm.SelectedFrameHexPreview);
	}

	[Fact]
	public void InputPreviewButtons_UsesCurrentLayoutButtons() {
		using var vm = new TasEditorViewModel();
		SetMovie(vm, CreateMovie(1, 1, SystemType.Genesis));

		vm.SelectedFrameIndex = 0;

		Assert.Contains(vm.InputPreviewButtons, button => button.ButtonId == "C");
		Assert.Contains(vm.InputPreviewButtons, button => button.ButtonId == "Z");
		Assert.Equal(4, vm.InputPreviewColumns);
	}

	[Fact]
	public void InputPreviewButtons_ReflectPressedStates_ForGenesisExtendedButtons() {
		using var vm = new TasEditorViewModel();
		var movie = CreateMovie(1, 1, SystemType.Genesis);
		movie.InputFrames[0].Controllers[0].C = true;
		movie.InputFrames[0].Controllers[0].Z = true;
		SetMovie(vm, movie);

		vm.SelectedFrameIndex = 0;

		Assert.True(vm.InputPreviewButtons.First(button => button.ButtonId == "C").IsPressed);
		Assert.True(vm.InputPreviewButtons.First(button => button.ButtonId == "Z").IsPressed);
		Assert.False(vm.InputPreviewButtons.First(button => button.ButtonId == "A").IsPressed);
	}

	[Fact]
	public void ToggleButton_RefreshesPreviewHexAndDiagram() {
		using var vm = new TasEditorViewModel();
		SetMovie(vm, CreateMovie(1, 1, SystemType.Nes));
		vm.SelectedFrameIndex = 0;

		vm.ToggleButton(0, "A");

		Assert.Contains("P1 [Gamepad] 01 00", vm.SelectedFrameHexPreview);
		Assert.True(vm.InputPreviewButtons.First(button => button.ButtonId == "A").IsPressed);
	}

	[Fact]
	public void ControllerInput_SetAndGetButton_SupportsGenesisExtendedButtons() {
		var input = new ControllerInput();

		input.SetButton("C", true);
		input.SetButton("Z", true);
		input.SetButton("MODE", true);

		Assert.True(input.GetButton("C"));
		Assert.True(input.GetButton("z"));
		Assert.True(input.GetButton("Mode"));
	}
}
