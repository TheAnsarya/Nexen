using System.Reflection;
using Nexen.MovieConverter;
using Nexen.TAS;
using Nexen.ViewModels;
using Xunit;

namespace Nexen.Tests.TAS;

/// <summary>
/// Tests for GenerateScriptApiHtml content and ViewModel guard methods
/// that exercise state mutations without requiring native DLLs.
/// </summary>
public class TasEditorApiDocAndGuardTests : IDisposable {
	private readonly TasEditorViewModel _vm;

	public TasEditorApiDocAndGuardTests() {
		_vm = new TasEditorViewModel();
	}

	public void Dispose() {
		_vm.Dispose();
	}

	#region Helpers

	private static MovieData CreateTestMovie(int frameCount, SystemType system = SystemType.Nes) {
		var movie = new MovieData {
			Author = "GuardTest",
			GameName = "Test Game",
			SystemType = system,
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
		var prop = typeof(TasEditorViewModel).GetProperty(nameof(TasEditorViewModel.Movie));
		prop!.SetValue(_vm, movie);
		_vm.Recorder.Movie = movie;
	}

	#endregion

	#region GenerateScriptApiHtml — Structure

	[Fact]
	public void GenerateScriptApiHtml_ReturnsValidHtml() {
		string html = TasEditorViewModel.GenerateScriptApiHtml();

		Assert.StartsWith("<!DOCTYPE html>", html);
		Assert.Contains("<html>", html);
		Assert.Contains("</html>", html);
		Assert.Contains("<head>", html);
		Assert.Contains("<body>", html);
	}

	[Fact]
	public void GenerateScriptApiHtml_HasTitle() {
		string html = TasEditorViewModel.GenerateScriptApiHtml();

		Assert.Contains("<title>Nexen TAS Lua API Reference</title>", html);
		Assert.Contains("Nexen TAS Lua API Reference</h1>", html);
	}

	[Fact]
	public void GenerateScriptApiHtml_HasStyleBlock() {
		string html = TasEditorViewModel.GenerateScriptApiHtml();

		Assert.Contains("<style>", html);
		Assert.Contains("</style>", html);
	}

	#endregion

	#region GenerateScriptApiHtml — Sections

	[Theory]
	[InlineData("Movie Information")]
	[InlineData("Frame Navigation")]
	[InlineData("Input Manipulation")]
	[InlineData("Input Table Format")]
	[InlineData("Greenzone Operations")]
	[InlineData("Recording Operations")]
	[InlineData("Branch Operations")]
	[InlineData("Input Search (Brute Force)")]
	[InlineData("Utility Functions")]
	[InlineData("Example: Brute Force Search")]
	public void GenerateScriptApiHtml_ContainsSection(string sectionName) {
		string html = TasEditorViewModel.GenerateScriptApiHtml();

		Assert.Contains(sectionName, html);
	}

	#endregion

	#region GenerateScriptApiHtml — API Functions

	[Theory]
	[InlineData("tas.getCurrentFrame()")]
	[InlineData("tas.getTotalFrames()")]
	[InlineData("tas.getRerecordCount()")]
	[InlineData("tas.getMovieInfo()")]
	[InlineData("tas.frameAdvance()")]
	[InlineData("tas.frameRewind()")]
	[InlineData("tas.pause()")]
	[InlineData("tas.resume()")]
	[InlineData("tas.getFrameInput(frame")]
	[InlineData("tas.setFrameInput(frame")]
	[InlineData("tas.clearFrameInput(frame")]
	[InlineData("tas.insertFrames(position")]
	[InlineData("tas.deleteFrames(position")]
	[InlineData("tas.getGreenzoneInfo()")]
	[InlineData("tas.captureGreenzone()")]
	[InlineData("tas.clearGreenzone()")]
	[InlineData("tas.hasSavestate(frame)")]
	[InlineData("tas.startRecording(")]
	[InlineData("tas.stopRecording()")]
	[InlineData("tas.isRecording()")]
	[InlineData("tas.rerecordFrom(frame)")]
	[InlineData("tas.createBranch(")]
	[InlineData("tas.getBranches()")]
	[InlineData("tas.loadBranch(name)")]
	[InlineData("tas.beginSearch()")]
	[InlineData("tas.testInput(input")]
	[InlineData("tas.setSearchState(key")]
	[InlineData("tas.getSearchState(key)")]
	[InlineData("tas.markBestResult()")]
	[InlineData("tas.finishSearch(")]
	[InlineData("tas.generateInputCombinations(")]
	[InlineData("tas.readMemory(address")]
	[InlineData("tas.writeMemory(address")]
	[InlineData("tas.readMemory16(address")]
	[InlineData("tas.log(message)")]
	[InlineData("tas.displayMessage(message)")]
	public void GenerateScriptApiHtml_ContainsFunction(string functionSignature) {
		string html = TasEditorViewModel.GenerateScriptApiHtml();

		Assert.Contains(functionSignature, html);
	}

	#endregion

	#region GenerateScriptApiHtml — Example Code

	[Fact]
	public void GenerateScriptApiHtml_ContainsBruteForceExample() {
		string html = TasEditorViewModel.GenerateScriptApiHtml();

		Assert.Contains("Find the input that maximizes X position", html);
		Assert.Contains("tas.beginSearch()", html);
		Assert.Contains("tas.generateInputCombinations(buttons)", html);
		Assert.Contains("tas.finishSearch(true)", html);
	}

	[Fact]
	public void GenerateScriptApiHtml_ContainsInputTableFormat() {
		string html = TasEditorViewModel.GenerateScriptApiHtml();

		Assert.Contains("a = true/false", html);
		Assert.Contains("start = true/false", html);
		Assert.Contains("up = true/false", html);
	}

	#endregion

	#region StopPlayback Guards

	[Fact]
	public void StopPlayback_SetsIsPlayingFalse() {
		_vm.IsPlaying = true;

		try { _vm.StopPlayback(); } catch (DllNotFoundException) { }

		Assert.False(_vm.IsPlaying);
	}

	[Fact]
	public void StopPlayback_SetsStatusMessage() {
		_vm.IsPlaying = true;

		try { _vm.StopPlayback(); } catch (DllNotFoundException) { }

		Assert.Contains("Playback stopped", _vm.StatusMessage);
	}

	#endregion

	#region StartRecording Guards

	[Fact]
	public void StartRecording_NullMovie_DoesNotThrow() {
		_vm.StartRecording();
		Assert.False(_vm.IsRecording);
	}

	[Fact]
	public void StartRecording_AlreadyRecording_DoesNotThrow() {
		SetMovie(CreateTestMovie(10));
		_vm.IsRecording = true;

		_vm.StartRecording();

		// Should be a no-op (guard returns)
		Assert.True(_vm.IsRecording);
	}

	#endregion

	#region StopRecording Guards

	[Fact]
	public void StopRecording_NotRecording_DoesNotThrow() {
		_vm.StopRecording();
		Assert.False(_vm.IsRecording);
	}

	#endregion

	#region RerecordFromSelected Guards

	[Fact]
	public void RerecordFromSelected_NullMovie_DoesNotThrow() {
		_vm.RerecordFromSelected();
		// No crash = success
	}

	[Fact]
	public void RerecordFromSelected_NegativeSelectedFrame_DoesNotThrow() {
		SetMovie(CreateTestMovie(10));
		_vm.SelectedFrameIndex = -1;

		_vm.RerecordFromSelected();
		// Guard should prevent crash
	}

	#endregion

	#region TogglePlayback

	[Fact]
	public void TogglePlayback_WhenNotPlaying_StartsPlayback() {
		SetMovie(CreateTestMovie(10));
		_vm.IsPlaying = false;

		try { _vm.TogglePlayback(); } catch (DllNotFoundException) { }

		Assert.True(_vm.IsPlaying);
	}

	[Fact]
	public void TogglePlayback_WhenPlaying_StopsPlayback() {
		_vm.IsPlaying = true;

		try { _vm.TogglePlayback(); } catch (DllNotFoundException) { }

		Assert.False(_vm.IsPlaying);
	}

	#endregion

	#region ShowGreenzoneSettings

	[Fact]
	public void ShowGreenzoneSettings_SetsStatusMessage() {
		_vm.ShowGreenzoneSettings();

		Assert.Contains("Greenzone", _vm.StatusMessage);
	}

	#endregion
}
