using System.Reflection;
using Nexen.MovieConverter;
using Nexen.TAS;
using Nexen.ViewModels;
using Xunit;

namespace Nexen.Tests.TAS;

/// <summary>
/// Tests for TasEditorViewModel branch management, controller layout
/// detection, and frame navigation.
/// </summary>
public class TasEditorViewModelBranchAndLayoutTests : IDisposable {
	private readonly TasEditorViewModel _vm;

	public TasEditorViewModelBranchAndLayoutTests() {
		_vm = new TasEditorViewModel();
	}

	public void Dispose() {
		_vm.Dispose();
	}

	#region Helpers

	private static MovieData CreateTestMovie(int frameCount, SystemType system = SystemType.Nes) {
		var movie = new MovieData {
			Author = "BranchTest",
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

	#region Branch Management — CreateBranch

	[Fact]
	public void CreateBranch_NullMovie_DoesNotThrow() {
		// No movie loaded — should be a no-op
		_vm.CreateBranch();
		Assert.Empty(_vm.Branches);
	}

	[Fact]
	public void CreateBranch_AddsToBranchesCollection() {
		SetMovie(CreateTestMovie(10));
		Assert.Empty(_vm.Branches);

		_vm.CreateBranch();

		Assert.Single(_vm.Branches);
	}

	[Fact]
	public void CreateBranch_SetsStatusMessage() {
		SetMovie(CreateTestMovie(10));

		_vm.CreateBranch();

		Assert.Contains("Created branch", _vm.StatusMessage);
		Assert.Contains(_vm.Branches[0].Name, _vm.StatusMessage);
	}

	[Fact]
	public void CreateBranch_MultipleBranches() {
		SetMovie(CreateTestMovie(10));

		_vm.CreateBranch();
		_vm.CreateBranch();
		_vm.CreateBranch();

		Assert.Equal(3, _vm.Branches.Count);
	}

	[Fact]
	public void CreateBranch_SnapshotsFrameCount() {
		SetMovie(CreateTestMovie(15));

		_vm.CreateBranch();

		Assert.Equal(15, _vm.Branches[0].FrameCount);
	}

	[Fact]
	public void CreateBranch_SnapshotsFrameData() {
		var movie = CreateTestMovie(5);
		movie.InputFrames[2].Controllers[0].A = true;
		SetMovie(movie);

		_vm.CreateBranch();

		Assert.True(_vm.Branches[0].Frames[2].Controllers[0].A);
	}

	#endregion

	#region Branch Management — LoadBranch

	[Fact]
	public void LoadBranch_NullMovie_DoesNotThrow() {
		var branch = new BranchData {
			Name = "test",
			Frames = [new InputFrame(0) { Controllers = [new ControllerInput()] }],
			FrameCount = 1
		};

		_vm.LoadBranch(branch);
		// No crash = success
	}

	[Fact]
	public void LoadBranch_ReplacesMovieFrames() {
		SetMovie(CreateTestMovie(5));

		var branchFrames = new List<InputFrame>();
		for (int i = 0; i < 10; i++) {
			branchFrames.Add(new InputFrame(i) { Controllers = [new ControllerInput()] });
		}
		branchFrames[3].Controllers[0].B = true;

		var branch = new BranchData {
			Name = "extended",
			Frames = branchFrames,
			FrameCount = 10
		};

		_vm.LoadBranch(branch);

		Assert.Equal(10, _vm.Movie!.InputFrames.Count);
		Assert.True(_vm.Movie.InputFrames[3].Controllers[0].B);
	}

	[Fact]
	public void LoadBranch_SetsHasUnsavedChanges() {
		SetMovie(CreateTestMovie(5));
		_vm.HasUnsavedChanges = false;

		var branch = new BranchData {
			Name = "test",
			Frames = [new InputFrame(0) { Controllers = [new ControllerInput()] }],
			FrameCount = 1
		};

		_vm.LoadBranch(branch);

		Assert.True(_vm.HasUnsavedChanges);
	}

	[Fact]
	public void LoadBranch_SetsStatusMessage() {
		SetMovie(CreateTestMovie(5));

		var branch = new BranchData {
			Name = "my-branch",
			Frames = [new InputFrame(0) { Controllers = [new ControllerInput()] }],
			FrameCount = 1
		};

		_vm.LoadBranch(branch);

		Assert.Contains("Loaded branch", _vm.StatusMessage);
		Assert.Contains("my-branch", _vm.StatusMessage);
	}

	[Fact]
	public void CreateAndLoadBranch_RoundTrip() {
		var movie = CreateTestMovie(8);
		movie.InputFrames[4].Controllers[0].Up = true;
		SetMovie(movie);

		_vm.CreateBranch();
		var savedBranch = _vm.Branches[0];

		// Modify movie
		_vm.Movie!.InputFrames[4].Controllers[0].Up = false;
		Assert.False(_vm.Movie.InputFrames[4].Controllers[0].Up);

		// Load branch — should restore original state
		_vm.LoadBranch(savedBranch);

		Assert.True(_vm.Movie.InputFrames[4].Controllers[0].Up);
	}

	#endregion

	#region Rerecord And Refresh Regression

	[Theory]
	[InlineData(SystemType.Lynx)]
	[InlineData(SystemType.A2600)]
	public void RerecordFromSelected_WithoutGreenzoneState_SetsFailureStatus_ForNonGenesisSystems(SystemType system) {
		SetMovie(CreateTestMovie(8, system));
		_vm.SelectedFrameIndex = 4;
		_vm.HasUnsavedChanges = false;

		_vm.RerecordFromSelected();

		Assert.Equal("Failed to rerecord - no greenzone state available for this frame", _vm.StatusMessage);
		Assert.False(_vm.HasUnsavedChanges);
		Assert.Equal(8, _vm.Movie!.InputFrames.Count);
	}

	[Theory]
	[InlineData(SystemType.Lynx)]
	[InlineData(SystemType.A2600)]
	public void RerecordFromSelected_OutOfRangeFrame_SetsOutOfRangeStatus_ForNonGenesisSystems(SystemType system) {
		SetMovie(CreateTestMovie(8, system));
		_vm.SelectedFrameIndex = 64;

		_vm.RerecordFromSelected();

		Assert.Equal("Failed to rerecord - selected frame is out of range", _vm.StatusMessage);
		Assert.Equal(8, _vm.Movie!.InputFrames.Count);
	}

	[Theory]
	[InlineData(SystemType.Lynx)]
	[InlineData(SystemType.A2600)]
	public void RefreshFrames_ClampsSelectedAndPlaybackFrame_AfterMovieTruncation(SystemType system) {
		var movie = CreateTestMovie(10, system);
		SetMovie(movie);

		_vm.SelectedFrameIndex = 9;
		_vm.PlaybackFrame = 9;

		movie.InputFrames.RemoveRange(4, movie.InputFrames.Count - 4);
		_vm.RefreshFrames();

		Assert.Equal(4, _vm.Movie!.InputFrames.Count);
		Assert.Equal(3, _vm.SelectedFrameIndex);
		Assert.Equal(3, _vm.PlaybackFrame);
	}

	#endregion

	#region Greenzone Seek Path Guard (Lynx / Atari 2600)

	/// <summary>
	/// SeekToFrameAsync exits early when no movie is loaded.
	/// Verify no crash or status mutation occurs when seek is attempted with no movie.
	/// </summary>
	[Fact]
	public async System.Threading.Tasks.Task SeekToFrameAsync_WithNoMovie_ExitsCleanly() {
		// No movie loaded — seeking should be a no-op regardless of platform
		Assert.Null(_vm.Movie);
		string statusBefore = _vm.StatusMessage;

		// Should not throw even when no movie or window
		await _vm.SeekToFrameAsync();

		// Status must not change — guard exited cleanly
		Assert.Equal(statusBefore, _vm.StatusMessage);
	}

	/// <summary>
	/// When a movie is loaded but the greenzone has no savestates, seeking must be blocked.
	/// Verify the ViewModel reports the correct result for both Lynx and Atari 2600.
	/// </summary>
	[Theory]
	[InlineData(SystemType.Lynx)]
	[InlineData(SystemType.A2600)]
	public async System.Threading.Tasks.Task SeekToFrameAsync_WithMovieButNoGreenzoneStates_ExitsCleanlyForPlatform(SystemType system) {
		SetMovie(CreateTestMovie(100, system));
		_vm.SelectedFrameIndex = 50;
		string statusBefore = _vm.StatusMessage;

		// Greenzone.SavestateCount == 0 and _window == null → early exit
		await _vm.SeekToFrameAsync();

		// Guard must not alter status message when seek is blocked
		Assert.Equal(statusBefore, _vm.StatusMessage);
	}

	/// <summary>
	/// After truncation, SelectedFrameIndex is clamped. A subsequent call to SeekToFrameAsync
	/// must not observe an out-of-range frame index. Validates truncation + seek interaction
	/// for Lynx and Atari 2600 system types.
	/// </summary>
	[Theory]
	[InlineData(SystemType.Lynx)]
	[InlineData(SystemType.A2600)]
	public void SeekTarget_AfterTruncation_IsWithinMovieBounds(SystemType system) {
		var movie = CreateTestMovie(50, system);
		SetMovie(movie);

		_vm.SelectedFrameIndex = 49;

		// Truncate to 20 frames and refresh
		movie.InputFrames.RemoveRange(20, movie.InputFrames.Count - 20);
		_vm.RefreshFrames();

		// SelectedFrameIndex must always be within movie bounds after refresh
		Assert.InRange(_vm.SelectedFrameIndex, 0, _vm.Movie!.InputFrames.Count - 1);
		// The seek dialog would cap to maxFrame = InputFrames.Count - 1 = 19
		int maxValidSeekFrame = _vm.Movie.InputFrames.Count - 1;
		Assert.True(_vm.SelectedFrameIndex <= maxValidSeekFrame,
			$"SelectedFrameIndex={_vm.SelectedFrameIndex} exceeds maxValidSeekFrame={maxValidSeekFrame}");
	}

	#endregion

	#region Controller Layout Detection

	[Theory]
	[InlineData(SystemType.Nes, ControllerLayout.Nes)]
	[InlineData(SystemType.Snes, ControllerLayout.Snes)]
	[InlineData(SystemType.Gb, ControllerLayout.GameBoy)]
	[InlineData(SystemType.Gbc, ControllerLayout.GameBoy)]
	[InlineData(SystemType.Gba, ControllerLayout.Gba)]
	[InlineData(SystemType.Genesis, ControllerLayout.Genesis)]
	[InlineData(SystemType.Sms, ControllerLayout.MasterSystem)]
	[InlineData(SystemType.Pce, ControllerLayout.PcEngine)]
	[InlineData(SystemType.Ws, ControllerLayout.WonderSwan)]
	[InlineData(SystemType.Lynx, ControllerLayout.Lynx)]
	[InlineData(SystemType.A2600, ControllerLayout.Atari2600)]
	public void DetectControllerLayout_SystemType(SystemType system, ControllerLayout expected) {
		SetMovie(CreateTestMovie(1, system));

		// DetectControllerLayout is called reactively when Movie changes
		Assert.Equal(expected, _vm.CurrentLayout);
	}

	[Fact]
	public void DetectControllerLayout_NullMovie_DefaultsSnes() {
		// Default is SNES before any movie is loaded
		Assert.Equal(ControllerLayout.Snes, _vm.CurrentLayout);
	}

	[Fact]
	public void DetectControllerLayout_UnknownSystem_FallbackToSourceFormat_Fm2() {
		var movie = CreateTestMovie(1, SystemType.Other);
		movie.SourceFormat = MovieFormat.Fm2;
		SetMovie(movie);

		Assert.Equal(ControllerLayout.Nes, _vm.CurrentLayout);
	}

	[Fact]
	public void DetectControllerLayout_UnknownSystem_FallbackToSourceFormat_Vbm() {
		var movie = CreateTestMovie(1, SystemType.Other);
		movie.SourceFormat = MovieFormat.Vbm;
		SetMovie(movie);

		Assert.Equal(ControllerLayout.Gba, _vm.CurrentLayout);
	}

	[Fact]
	public void DetectControllerLayout_UnknownSystem_FallbackToSourceFormat_Gmv() {
		var movie = CreateTestMovie(1, SystemType.Other);
		movie.SourceFormat = MovieFormat.Gmv;
		SetMovie(movie);

		Assert.Equal(ControllerLayout.Genesis, _vm.CurrentLayout);
	}

	[Fact]
	public void DetectControllerLayout_UnknownSystem_FallbackToSourceFormat_Smv() {
		var movie = CreateTestMovie(1, SystemType.Other);
		movie.SourceFormat = MovieFormat.Smv;
		SetMovie(movie);

		Assert.Equal(ControllerLayout.Snes, _vm.CurrentLayout);
	}

	[Fact]
	public void DetectControllerLayout_UnknownSystem_FallbackToSourceFormat_Lsmv() {
		var movie = CreateTestMovie(1, SystemType.Other);
		movie.SourceFormat = MovieFormat.Lsmv;
		SetMovie(movie);

		Assert.Equal(ControllerLayout.Snes, _vm.CurrentLayout);
	}

	[Fact]
	public void DetectControllerLayout_UnknownSystem_UnknownFormat_DefaultsSnes() {
		var movie = CreateTestMovie(1, SystemType.Other);
		movie.SourceFormat = MovieFormat.Unknown;
		SetMovie(movie);

		Assert.Equal(ControllerLayout.Snes, _vm.CurrentLayout);
	}

	[Fact]
	public void DetectControllerLayout_SetsStatusMessage() {
		SetMovie(CreateTestMovie(1, SystemType.Nes));

		Assert.Contains("Detected controller layout", _vm.StatusMessage);
		Assert.Contains("Nes", _vm.StatusMessage);
	}

	#endregion

	#region Controller Buttons per Layout

	[Theory]
	[InlineData(ControllerLayout.Nes, 8)]
	[InlineData(ControllerLayout.Snes, 12)]
	[InlineData(ControllerLayout.GameBoy, 8)]
	[InlineData(ControllerLayout.Gba, 10)]
	[InlineData(ControllerLayout.Genesis, 12)]
	[InlineData(ControllerLayout.MasterSystem, 6)]
	[InlineData(ControllerLayout.PcEngine, 8)]
	[InlineData(ControllerLayout.WonderSwan, 11)]
	[InlineData(ControllerLayout.Lynx, 9)]
	[InlineData(ControllerLayout.Atari2600, 5)]
	public void ControllerButtons_CorrectCount(ControllerLayout layout, int expectedCount) {
		_vm.CurrentLayout = layout;

		Assert.Equal(expectedCount, _vm.ControllerButtons.Count);
	}

	[Fact]
	public void ControllerButtons_NesHasExpectedButtons() {
		_vm.CurrentLayout = ControllerLayout.Nes;

		var ids = _vm.ControllerButtons.Select(b => b.ButtonId).ToList();
		Assert.Contains("A", ids);
		Assert.Contains("B", ids);
		Assert.Contains("SELECT", ids);
		Assert.Contains("START", ids);
		Assert.Contains("UP", ids);
		Assert.Contains("DOWN", ids);
		Assert.Contains("LEFT", ids);
		Assert.Contains("RIGHT", ids);
	}

	[Fact]
	public void ControllerButtons_SnesHasShoulderButtons() {
		_vm.CurrentLayout = ControllerLayout.Snes;

		var ids = _vm.ControllerButtons.Select(b => b.ButtonId).ToList();
		Assert.Contains("L", ids);
		Assert.Contains("R", ids);
		Assert.Contains("X", ids);
		Assert.Contains("Y", ids);
	}

	[Fact]
	public void ControllerButtons_GenesisHasSixFaceButtons() {
		_vm.CurrentLayout = ControllerLayout.Genesis;

		var ids = _vm.ControllerButtons.Select(b => b.ButtonId).ToList();
		Assert.Contains("A", ids);
		Assert.Contains("B", ids);
		Assert.Contains("C", ids);
		Assert.Contains("X", ids);
		Assert.Contains("Y", ids);
		Assert.Contains("Z", ids);
		Assert.Contains("MODE", ids);
	}

	[Fact]
	public void ControllerButtons_MasterSystem_MinimalButtons() {
		_vm.CurrentLayout = ControllerLayout.MasterSystem;

		var ids = _vm.ControllerButtons.Select(b => b.ButtonId).ToList();
		Assert.Contains("A", ids);
		Assert.Contains("B", ids);
		Assert.DoesNotContain("START", ids);
		Assert.DoesNotContain("SELECT", ids);
	}

	[Fact]
	public void ControllerButtons_UpdatedWhenLayoutChanges() {
		_vm.CurrentLayout = ControllerLayout.Nes;
		Assert.Equal(8, _vm.ControllerButtons.Count);

		_vm.CurrentLayout = ControllerLayout.Snes;
		Assert.Equal(12, _vm.ControllerButtons.Count);

		_vm.CurrentLayout = ControllerLayout.MasterSystem;
		Assert.Equal(6, _vm.ControllerButtons.Count);
	}

	[Fact]
	public void PaddleEditor_VisibleAndEditable_ForAtariPaddlePort() {
		var movie = CreateTestMovie(2, SystemType.A2600);
		movie.PortTypes = [ControllerType.Atari2600Paddle, ControllerType.None];
		movie.InputFrames[0].Controllers[0].PaddlePosition = 32;
		SetMovie(movie);

		_vm.SelectedFrameIndex = 0;
		Assert.True(_vm.IsPaddleCoordinateEditorVisible);
		Assert.Equal(32, _vm.SelectedPaddlePosition);

		_vm.SelectedPaddlePosition = 200;
		Assert.Equal((byte)200, _vm.Movie!.InputFrames[0].Controllers[0].PaddlePosition);
	}

	[Fact]
	public void ControllerButtons_SettingMovieUpdatesButtons() {
		SetMovie(CreateTestMovie(1, SystemType.Genesis));

		// Setting a Genesis movie should give us 12 buttons including MODE.
		Assert.Equal(12, _vm.ControllerButtons.Count);
	}

	#endregion

	#region Console Switch Toggle (Atari 2600)

	[Fact]
	public void ConsoleSwitchPanel_VisibleOnly_ForAtari2600Layout() {
		SetMovie(CreateTestMovie(2, SystemType.A2600));
		_vm.SelectedFrameIndex = 0;
		Assert.True(_vm.IsConsoleSwitchPanelVisible);

		SetMovie(CreateTestMovie(2, SystemType.Nes));
		_vm.SelectedFrameIndex = 0;
		Assert.False(_vm.IsConsoleSwitchPanelVisible);
	}

	[Fact]
	public void ToggleConsoleSwitchSelect_SetsFrameCommandFlag() {
		SetMovie(CreateTestMovie(3, SystemType.A2600));
		_vm.SelectedFrameIndex = 1;

		_vm.ToggleConsoleSwitchSelect();
		Assert.True(_vm.Movie!.InputFrames[1].Command.HasFlag(FrameCommand.Atari2600Select));
		Assert.True(_vm.IsSelectedFrameSelectActive);

		// Toggle off
		_vm.ToggleConsoleSwitchSelect();
		Assert.False(_vm.Movie!.InputFrames[1].Command.HasFlag(FrameCommand.Atari2600Select));
		Assert.False(_vm.IsSelectedFrameSelectActive);
	}

	[Fact]
	public void ToggleConsoleSwitchReset_SetsFrameCommandFlag() {
		SetMovie(CreateTestMovie(3, SystemType.A2600));
		_vm.SelectedFrameIndex = 1;

		_vm.ToggleConsoleSwitchReset();
		Assert.True(_vm.Movie!.InputFrames[1].Command.HasFlag(FrameCommand.Atari2600Reset));
		Assert.True(_vm.IsSelectedFrameResetActive);
	}

	[Fact]
	public void ToggleConsoleSwitchSelect_Undoable() {
		SetMovie(CreateTestMovie(3, SystemType.A2600));
		_vm.SelectedFrameIndex = 0;

		_vm.ToggleConsoleSwitchSelect();
		Assert.True(_vm.Movie!.InputFrames[0].Command.HasFlag(FrameCommand.Atari2600Select));
		Assert.True(_vm.CanUndo);

		_vm.Undo();
		Assert.False(_vm.Movie!.InputFrames[0].Command.HasFlag(FrameCommand.Atari2600Select));
	}

	[Fact]
	public void ToggleConsoleSwitchReset_Undoable_AndRedoable() {
		SetMovie(CreateTestMovie(3, SystemType.A2600));
		_vm.SelectedFrameIndex = 0;

		_vm.ToggleConsoleSwitchReset();
		Assert.True(_vm.Movie!.InputFrames[0].Command.HasFlag(FrameCommand.Atari2600Reset));

		_vm.Undo();
		Assert.False(_vm.Movie!.InputFrames[0].Command.HasFlag(FrameCommand.Atari2600Reset));

		_vm.Redo();
		Assert.True(_vm.Movie!.InputFrames[0].Command.HasFlag(FrameCommand.Atari2600Reset));
	}

	[Fact]
	public void ToggleConsoleSwitchSelect_BothFlagsIndependent() {
		SetMovie(CreateTestMovie(3, SystemType.A2600));
		_vm.SelectedFrameIndex = 0;

		_vm.ToggleConsoleSwitchSelect();
		_vm.ToggleConsoleSwitchReset();

		var cmd = _vm.Movie!.InputFrames[0].Command;
		Assert.True(cmd.HasFlag(FrameCommand.Atari2600Select));
		Assert.True(cmd.HasFlag(FrameCommand.Atari2600Reset));

		// Undo only removes Reset
		_vm.Undo();
		cmd = _vm.Movie!.InputFrames[0].Command;
		Assert.True(cmd.HasFlag(FrameCommand.Atari2600Select));
		Assert.False(cmd.HasFlag(FrameCommand.Atari2600Reset));
	}

	[Fact]
	public void ToggleConsoleSwitchSelect_NoMovie_DoesNotThrow() {
		_vm.ToggleConsoleSwitchSelect();
		_vm.ToggleConsoleSwitchReset();
		// Should not throw
	}

	[Fact]
	public void ToggleConsoleSwitchSelect_NoSelectedFrame_DoesNotThrow() {
		SetMovie(CreateTestMovie(3, SystemType.A2600));
		_vm.SelectedFrameIndex = -1;

		_vm.ToggleConsoleSwitchSelect();
		// Should not throw or modify any frame
	}

	[Fact]
	public void ConsoleSwitchState_UpdatesOnFrameSelection() {
		SetMovie(CreateTestMovie(3, SystemType.A2600));
		_vm.Movie!.InputFrames[1].Command = FrameCommand.Atari2600Select;
		_vm.Movie!.InputFrames[2].Command = FrameCommand.Atari2600Reset;

		_vm.SelectedFrameIndex = 0;
		Assert.False(_vm.IsSelectedFrameSelectActive);
		Assert.False(_vm.IsSelectedFrameResetActive);

		_vm.SelectedFrameIndex = 1;
		Assert.True(_vm.IsSelectedFrameSelectActive);
		Assert.False(_vm.IsSelectedFrameResetActive);

		_vm.SelectedFrameIndex = 2;
		Assert.False(_vm.IsSelectedFrameSelectActive);
		Assert.True(_vm.IsSelectedFrameResetActive);
	}

	#endregion

	#region Frame Navigation — FrameAdvance

	// Note: FrameAdvance/FrameRewind call EmuApi.IsRunning() (native P/Invoke)
	// after state mutations. We catch DllNotFoundException to verify state changes.

	[Fact]
	public void FrameAdvance_NullMovie_DoesNotThrow() {
		_vm.FrameAdvance();
		Assert.Equal(0, _vm.PlaybackFrame);
	}

	[Fact]
	public void FrameAdvance_IncrementsPlaybackFrame() {
		SetMovie(CreateTestMovie(10));
		_vm.PlaybackFrame = 3;

		try { _vm.FrameAdvance(); } catch (DllNotFoundException) { }

		Assert.Equal(4, _vm.PlaybackFrame);
	}

	[Fact]
	public void FrameAdvance_SetsSelectedFrameIndex() {
		SetMovie(CreateTestMovie(10));
		_vm.PlaybackFrame = 5;

		try { _vm.FrameAdvance(); } catch (DllNotFoundException) { }

		Assert.Equal(6, _vm.SelectedFrameIndex);
	}

	[Fact]
	public void FrameAdvance_SetsStatusMessage() {
		SetMovie(CreateTestMovie(10));
		_vm.PlaybackFrame = 2;

		try { _vm.FrameAdvance(); } catch (DllNotFoundException) { }

		Assert.Contains("Frame", _vm.StatusMessage);
	}

	[Fact]
	public void FrameAdvance_AtLastFrame_DoesNotAdvance() {
		SetMovie(CreateTestMovie(10));
		_vm.PlaybackFrame = 9; // last frame (index 0-9)

		_vm.FrameAdvance();

		Assert.Equal(9, _vm.PlaybackFrame);
	}

	[Fact]
	public void FrameAdvance_AtSecondToLast_AdvancesToLast() {
		SetMovie(CreateTestMovie(10));
		_vm.PlaybackFrame = 8;

		try { _vm.FrameAdvance(); } catch (DllNotFoundException) { }

		Assert.Equal(9, _vm.PlaybackFrame);
	}

	#endregion

	#region Frame Navigation — FrameRewind

	[Fact]
	public void FrameRewind_NullMovie_DoesNotThrow() {
		_vm.FrameRewind();
		Assert.Equal(0, _vm.PlaybackFrame);
	}

	[Fact]
	public void FrameRewind_DecrementsPlaybackFrame() {
		SetMovie(CreateTestMovie(10));
		_vm.PlaybackFrame = 5;

		try { _vm.FrameRewind(); } catch (DllNotFoundException) { }

		Assert.Equal(4, _vm.PlaybackFrame);
	}

	[Fact]
	public void FrameRewind_SetsSelectedFrameIndex() {
		SetMovie(CreateTestMovie(10));
		_vm.PlaybackFrame = 5;

		try { _vm.FrameRewind(); } catch (DllNotFoundException) { }

		Assert.Equal(4, _vm.SelectedFrameIndex);
	}

	[Fact]
	public void FrameRewind_SetsStatusMessage() {
		SetMovie(CreateTestMovie(10));
		_vm.PlaybackFrame = 3;

		try { _vm.FrameRewind(); } catch (DllNotFoundException) { }

		Assert.Contains("Frame", _vm.StatusMessage);
	}

	[Fact]
	public void FrameRewind_AtFrameZero_DoesNotRewind() {
		SetMovie(CreateTestMovie(10));
		_vm.PlaybackFrame = 0;

		_vm.FrameRewind();

		Assert.Equal(0, _vm.PlaybackFrame);
	}

	[Fact]
	public void FrameRewind_AtFrameOne_RewindsToZero() {
		SetMovie(CreateTestMovie(10));
		_vm.PlaybackFrame = 1;

		try { _vm.FrameRewind(); } catch (DllNotFoundException) { }

		Assert.Equal(0, _vm.PlaybackFrame);
	}

	#endregion

	#region Frame Navigation — Combined

	[Fact]
	public void FrameAdvanceAndRewind_AreInverse() {
		SetMovie(CreateTestMovie(10));
		_vm.PlaybackFrame = 5;

		try { _vm.FrameAdvance(); } catch (DllNotFoundException) { }
		Assert.Equal(6, _vm.PlaybackFrame);

		try { _vm.FrameRewind(); } catch (DllNotFoundException) { }
		Assert.Equal(5, _vm.PlaybackFrame);
	}

	[Fact]
	public void FrameAdvance_MultipleTimes() {
		SetMovie(CreateTestMovie(10));
		_vm.PlaybackFrame = 0;

		for (int i = 0; i < 5; i++) {
			try { _vm.FrameAdvance(); } catch (DllNotFoundException) { }
		}

		Assert.Equal(5, _vm.PlaybackFrame);
	}

	#endregion

	#region Multi-Port Controller Editing

	private static MovieData CreateMultiPortMovie(int frameCount, int portCount, SystemType system = SystemType.Nes) {
		var movie = new MovieData {
			Author = "MultiPortTest",
			GameName = "Test Game",
			SystemType = system,
			Region = RegionType.NTSC,
			ControllerCount = portCount,
		};

		for (int i = 0; i < frameCount; i++) {
			var controllers = new ControllerInput[portCount];
			for (int p = 0; p < portCount; p++) {
				controllers[p] = new ControllerInput();
			}
			movie.InputFrames.Add(new InputFrame(i) { Controllers = controllers });
		}

		return movie;
	}

	[Fact]
	public void ActivePortCount_SingleController_HidesSelector() {
		SetMovie(CreateTestMovie(5));
		Assert.Equal(1, _vm.ActivePortCount);
		Assert.False(_vm.IsPortSelectorVisible);
	}

	[Fact]
	public void ActivePortCount_TwoControllers_ShowsSelector() {
		var movie = CreateMultiPortMovie(5, 2);
		SetMovie(movie);
		Assert.Equal(2, _vm.ActivePortCount);
		Assert.True(_vm.IsPortSelectorVisible);
		Assert.Equal(1, _vm.MaxEditPort);
	}

	[Fact]
	public void SelectedEditPort_DefaultsToZero() {
		SetMovie(CreateMultiPortMovie(3, 2));
		Assert.Equal(0, _vm.SelectedEditPort);
	}

	[Fact]
	public void SelectedEditPort_ClampsWhenMovieChanges() {
		var twoPort = CreateMultiPortMovie(3, 2);
		SetMovie(twoPort);
		_vm.SelectedEditPort = 1;
		Assert.Equal(1, _vm.SelectedEditPort);

		// Load a single-port movie — should clamp back to 0
		SetMovie(CreateTestMovie(3));
		Assert.Equal(0, _vm.SelectedEditPort);
	}

	[Fact]
	public void ToggleButton_UsesSelectedEditPort() {
		var movie = CreateMultiPortMovie(3, 2);
		SetMovie(movie);
		_vm.SelectedFrameIndex = 0;
		_vm.SelectedEditPort = 1;

		// Toggle button on port 1
		_vm.ToggleButton(1, "A");

		// Port 1 should be toggled, port 0 should not
		Assert.False(movie.InputFrames[0].Controllers[0].A);
		Assert.True(movie.InputFrames[0].Controllers[1].A);
	}

	[Fact]
	public void PaddleEditor_UsesSelectedEditPort() {
		var movie = CreateMultiPortMovie(2, 2, SystemType.A2600);
		movie.PortTypes = [ControllerType.Atari2600Joystick, ControllerType.Atari2600Paddle];
		movie.InputFrames[0].Controllers[1].PaddlePosition = 64;
		SetMovie(movie);

		// Port 0 is joystick — paddle editor should NOT be visible
		_vm.SelectedEditPort = 0;
		_vm.SelectedFrameIndex = 0;
		Assert.False(_vm.IsPaddleCoordinateEditorVisible);

		// Port 1 is paddle — paddle editor SHOULD be visible
		_vm.SelectedEditPort = 1;
		Assert.True(_vm.IsPaddleCoordinateEditorVisible);
		Assert.Equal(64, _vm.SelectedPaddlePosition);
	}

	[Fact]
	public void Atari2600Buttons_ChangeBySelectedPort() {
		var movie = CreateMultiPortMovie(2, 2, SystemType.A2600);
		movie.PortTypes = [ControllerType.Atari2600Joystick, ControllerType.Atari2600Keypad];
		SetMovie(movie);

		// Port 0 = joystick (5 buttons)
		_vm.SelectedEditPort = 0;
		Assert.Equal(5, _vm.ControllerButtons.Count);

		// Port 1 = keypad (12 buttons)
		_vm.SelectedEditPort = 1;
		Assert.Equal(12, _vm.ControllerButtons.Count);
	}

	[Theory]
	[InlineData(ControllerType.Atari2600Joystick, 5, "UP")]
	[InlineData(ControllerType.Atari2600Paddle, 1, "FIRE")]
	[InlineData(ControllerType.Atari2600Keypad, 12, "POUND")]
	[InlineData(ControllerType.Atari2600DrivingController, 3, "LEFT")]
	[InlineData(ControllerType.Atari2600BoosterGrip, 7, "BOOSTER")]
	public void Atari2600Buttons_ExposeExpectedSubtypeButtons(ControllerType portType, int expectedCount, string expectedButtonId) {
		var movie = CreateMultiPortMovie(2, 2, SystemType.A2600);
		movie.PortTypes = [portType, ControllerType.Atari2600Joystick];
		SetMovie(movie);

		_vm.SelectedEditPort = 0;

		Assert.Equal(expectedCount, _vm.ControllerButtons.Count);
		Assert.Contains(_vm.ControllerButtons, b => b.ButtonId == expectedButtonId);
	}

	[Theory]
	[InlineData(ControllerType.Atari2600Joystick)]
	[InlineData(ControllerType.Atari2600Paddle)]
	[InlineData(ControllerType.Atari2600Keypad)]
	[InlineData(ControllerType.Atari2600DrivingController)]
	[InlineData(ControllerType.Atari2600BoosterGrip)]
	public void ConsoleSwitchPanel_Visible_ForAllAtari2600ControllerSubtypes(ControllerType portType) {
		var movie = CreateMultiPortMovie(2, 1, SystemType.A2600);
		movie.PortTypes = [portType];
		SetMovie(movie);

		_vm.SelectedFrameIndex = 0;
		Assert.True(_vm.IsConsoleSwitchPanelVisible);
	}

	[Theory]
	[InlineData(ControllerType.Atari2600Joystick, false)]
	[InlineData(ControllerType.Atari2600Paddle, true)]
	[InlineData(ControllerType.Atari2600Keypad, false)]
	[InlineData(ControllerType.Atari2600DrivingController, false)]
	[InlineData(ControllerType.Atari2600BoosterGrip, false)]
	public void PaddleEditor_Visibility_MatchesAtari2600ControllerSubtype(ControllerType portType, bool expectedVisible) {
		var movie = CreateMultiPortMovie(2, 1, SystemType.A2600);
		movie.PortTypes = [portType];
		movie.InputFrames[0].Controllers[0].PaddlePosition = 77;
		SetMovie(movie);

		_vm.SelectedFrameIndex = 0;
		Assert.Equal(expectedVisible, _vm.IsPaddleCoordinateEditorVisible);

		if (expectedVisible) {
			Assert.Equal(77, _vm.SelectedPaddlePosition);
		}
	}

	#endregion
}
