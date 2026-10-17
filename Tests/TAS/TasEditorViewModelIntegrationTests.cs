using System.Reflection;
using System.Threading.Tasks;
using Nexen.MovieConverter;
using Nexen.ViewModels;
using Xunit;

namespace Nexen.Tests.TAS;

/// <summary>
/// Integration tests for TasEditorViewModel — testing through the ViewModel's public API
/// with a real ViewModel instance, verifying that actions, undo/redo, clipboard, and
/// incremental frame collection updates all work end-to-end.
/// </summary>
public class TasEditorViewModelIntegrationTests : IDisposable {
	private readonly TasEditorViewModel _vm;

	public TasEditorViewModelIntegrationTests() {
		_vm = new TasEditorViewModel();
	}

	public void Dispose() {
		_vm.Dispose();
	}

	#region Helpers

	private static MovieData CreateTestMovie(int frameCount) {
		var movie = new MovieData {
			Author = "Test",
			GameName = "Test",
			SystemType = SystemType.Snes,
			Region = RegionType.NTSC
		};

		for (int i = 0; i < frameCount; i++) {
			movie.InputFrames.Add(new InputFrame(i) {
				Controllers = [new ControllerInput()]
			});
		}

		return movie;
	}

	/// <summary>
	/// Sets the Movie property (private set) via reflection and triggers reactive subscriptions.
	/// </summary>
	private void SetMovie(MovieData movie) {
		var prop = typeof(TasEditorViewModel).GetProperty(nameof(TasEditorViewModel.Movie));
		prop!.SetValue(_vm, movie);
	}

	#endregion

	#region Construction and Initial State

	[Fact]
	public void Construction_DefaultState_NoMovieLoaded() {
		Assert.Null(_vm.Movie);
		Assert.Empty(_vm.Frames);
		Assert.False(_vm.CanUndo);
		Assert.False(_vm.CanRedo);
		Assert.False(_vm.HasClipboard);
		Assert.False(_vm.IsPlaying);
		Assert.False(_vm.IsRecording);
		Assert.Equal(-1, _vm.SelectedFrameIndex);
		Assert.Equal("No movie loaded", _vm.StatusMessage);
	}

	[Fact]
	public void Construction_GreenzoneAndRecorderInitialized() {
		Assert.NotNull(_vm.Greenzone);
		Assert.NotNull(_vm.Recorder);
		Assert.NotNull(_vm.LuaApi);
	}

	[Fact]
	public void SetMovie_PopulatesFramesCollection() {
		var movie = CreateTestMovie(10);
		SetMovie(movie);

		Assert.Equal(10, _vm.Frames.Count);
		Assert.Same(movie, _vm.Movie);
	}

	[Fact]
	public void SetMovie_FrameNumbersAreOneBased() {
		var movie = CreateTestMovie(5);
		SetMovie(movie);

		for (int i = 0; i < 5; i++) {
			Assert.Equal(i + 1, _vm.Frames[i].FrameNumber);
		}
	}

	[Fact]
	public void SetMovie_DetectsControllerLayout() {
		var movie = CreateTestMovie(3);
		movie.SystemType = SystemType.Nes;
		SetMovie(movie);

		Assert.Equal(ControllerLayout.Nes, _vm.CurrentLayout);
	}

	#endregion

	#region InsertFrames

	[Fact]
	public void InsertFrames_NoMovie_NoOp() {
		_vm.InsertFrames();

		Assert.Null(_vm.Movie);
		Assert.False(_vm.CanUndo);
	}

	[Fact]
	public void InsertFrames_InsertsAtSelectedIndex() {
		var movie = CreateTestMovie(5);
		SetMovie(movie);
		_vm.SelectedFrameIndex = 2;

		_vm.InsertFrames();

		Assert.Equal(6, _vm.Movie!.InputFrames.Count);
		Assert.Equal(6, _vm.Frames.Count);
		Assert.True(_vm.CanUndo);
		Assert.True(_vm.HasUnsavedChanges);
	}

	[Fact]
	public void InsertFrames_AtNegativeIndex_InsertsAtZero() {
		var movie = CreateTestMovie(5);
		SetMovie(movie);
		_vm.SelectedFrameIndex = -1;

		_vm.InsertFrames();

		Assert.Equal(6, _vm.Movie!.InputFrames.Count);
	}

	[Fact]
	public void InsertFrames_FrameNumbersUpdatedCorrectly() {
		var movie = CreateTestMovie(5);
		SetMovie(movie);
		_vm.SelectedFrameIndex = 2;

		_vm.InsertFrames();

		// All frame numbers should be 1-based sequential
		for (int i = 0; i < _vm.Frames.Count; i++) {
			Assert.Equal(i + 1, _vm.Frames[i].FrameNumber);
		}
	}

	#endregion

	#region DeleteFrames

	[Fact]
	public void DeleteFrames_NoMovie_NoOp() {
		_vm.DeleteFrames();

		Assert.False(_vm.CanUndo);
	}

	[Fact]
	public void DeleteFrames_NegativeIndex_NoOp() {
		var movie = CreateTestMovie(5);
		SetMovie(movie);
		_vm.SelectedFrameIndex = -1;

		_vm.DeleteFrames();

		Assert.Equal(5, _vm.Movie!.InputFrames.Count);
		Assert.False(_vm.CanUndo);
	}

	[Fact]
	public void DeleteFrames_ValidIndex_RemovesFrame() {
		var movie = CreateTestMovie(5);
		SetMovie(movie);
		_vm.SelectedFrameIndex = 2;

		_vm.DeleteFrames();

		Assert.Equal(4, _vm.Movie!.InputFrames.Count);
		Assert.Equal(4, _vm.Frames.Count);
		Assert.True(_vm.CanUndo);
	}

	[Fact]
	public void DeleteFrames_LastFrame_AdjustsSelection() {
		var movie = CreateTestMovie(3);
		SetMovie(movie);
		_vm.SelectedFrameIndex = 2;

		_vm.DeleteFrames();

		Assert.Equal(2, _vm.Movie!.InputFrames.Count);
		// Selection should be adjusted to last valid index
		Assert.True(_vm.SelectedFrameIndex < _vm.Movie.InputFrames.Count);
	}

	[Fact]
	public void DeleteFrames_FrameNumbersUpdatedCorrectly() {
		var movie = CreateTestMovie(5);
		SetMovie(movie);
		_vm.SelectedFrameIndex = 1;

		_vm.DeleteFrames();

		for (int i = 0; i < _vm.Frames.Count; i++) {
			Assert.Equal(i + 1, _vm.Frames[i].FrameNumber);
		}
	}

	#endregion

	#region ToggleButton

	[Fact]
	public void ToggleButton_NoMovie_NoOp() {
		_vm.ToggleButton(0, "A");

		Assert.False(_vm.CanUndo);
	}

	[Fact]
	public void ToggleButton_TogglesButtonState() {
		var movie = CreateTestMovie(3);
		SetMovie(movie);
		_vm.SelectedFrameIndex = 1;

		Assert.False(movie.InputFrames[1].Controllers[0].A);
		_vm.ToggleButton(0, "A");
		Assert.True(movie.InputFrames[1].Controllers[0].A);
	}

	[Fact]
	public void ToggleButton_TogglesTwice_RestoresOriginal() {
		var movie = CreateTestMovie(3);
		SetMovie(movie);
		_vm.SelectedFrameIndex = 0;

		_vm.ToggleButton(0, "B");
		Assert.True(movie.InputFrames[0].Controllers[0].B);

		_vm.ToggleButton(0, "B");
		Assert.False(movie.InputFrames[0].Controllers[0].B);
	}

	[Theory]
	[InlineData("A", "a")]
	[InlineData("B", "b")]
	[InlineData("X", "x")]
	[InlineData("Y", "y")]
	[InlineData("L", "l")]
	[InlineData("R", "r")]
	[InlineData("UP", "up")]
	[InlineData("DOWN", "down")]
	[InlineData("LEFT", "left")]
	[InlineData("RIGHT", "right")]
	[InlineData("START", "start")]
	[InlineData("SELECT", "select")]
	public void ToggleButton_CaseInsensitive_BothCasesWork(string upper, string lower) {
		var movie = CreateTestMovie(3);
		SetMovie(movie);
		_vm.SelectedFrameIndex = 0;

		_vm.ToggleButton(0, upper);
		var afterUpper = GetButtonState(movie.InputFrames[0].Controllers[0], upper);
		Assert.True(afterUpper);

		// Toggle back with lowercase
		_vm.ToggleButton(0, lower);
		var afterLower = GetButtonState(movie.InputFrames[0].Controllers[0], upper);
		Assert.False(afterLower);
	}

	[Fact]
	public void ToggleButton_InvalidPort_NoOp() {
		var movie = CreateTestMovie(3);
		SetMovie(movie);
		_vm.SelectedFrameIndex = 0;

		// Port out of range should not crash or create undo entry
		_vm.ToggleButton(99, "A");
		Assert.False(_vm.CanUndo);
	}

	[Fact]
	public void ToggleButton_UnknownButton_CreatesModifyActionButNoChange() {
		var movie = CreateTestMovie(3);
		SetMovie(movie);
		_vm.SelectedFrameIndex = 0;

		// Unknown button name — switch falls through, still creates action
		_vm.ToggleButton(0, "TURBO");
		// Action is created but no button actually changed
		Assert.True(_vm.CanUndo);
	}

	#endregion

	#region ToggleButtonAtFrame

	[Fact]
	public void ToggleButtonAtFrame_SetsSpecificState() {
		var movie = CreateTestMovie(5);
		SetMovie(movie);

		_vm.ToggleButtonAtFrame(3, 0, "A", true);

		Assert.True(movie.InputFrames[3].Controllers[0].A);
		Assert.True(_vm.CanUndo);
	}

	[Fact]
	public void ToggleButtonAtFrame_NoMovie_NoOp() {
		_vm.ToggleButtonAtFrame(0, 0, "A", true);

		Assert.False(_vm.CanUndo);
	}

	[Fact]
	public void ToggleButtonAtFrame_OutOfRange_NoOp() {
		var movie = CreateTestMovie(5);
		SetMovie(movie);

		_vm.ToggleButtonAtFrame(99, 0, "A", true);
		Assert.False(_vm.CanUndo);
	}

	[Fact]
	public void ToggleButtonAtFrame_SetTrue_ThenFalse() {
		var movie = CreateTestMovie(5);
		SetMovie(movie);

		_vm.ToggleButtonAtFrame(2, 0, "B", true);
		Assert.True(movie.InputFrames[2].Controllers[0].B);

		_vm.ToggleButtonAtFrame(2, 0, "B", false);
		Assert.False(movie.InputFrames[2].Controllers[0].B);
	}

	#endregion

	#region SetCommentAsync

	[Fact]
	public async Task SetCommentAsync_NoMovie_NoOp() {
		await _vm.SetCommentAsync();
		// Should not throw — no movie loaded
	}

	[Fact]
	public async Task SetCommentAsync_NoWindow_NoOp() {
		var movie = CreateTestMovie(5);
		SetMovie(movie);
		_vm.SelectedFrameIndex = 2;

		// Without a window, the dialog can't show — should return without changes
		await _vm.SetCommentAsync();
		Assert.Null(movie.InputFrames[2].Comment);
	}

	[Fact]
	public async Task SetCommentAsync_OutOfRange_NoOp() {
		var movie = CreateTestMovie(5);
		SetMovie(movie);
		_vm.SelectedFrameIndex = 10;

		await _vm.SetCommentAsync();
		// Should not throw, all frames should be unchanged
		Assert.All(movie.InputFrames, f => Assert.Null(f.Comment));
	}

	#endregion

	#region ClearSelectedInput

	[Fact]
	public void ClearSelectedInput_NoMovie_NoOp() {
		_vm.ClearSelectedInput();
		Assert.False(_vm.CanUndo);
	}

	[Fact]
	public void ClearSelectedInput_ClearsAllButtons() {
		var movie = CreateTestMovie(3);
		movie.InputFrames[1].Controllers[0].A = true;
		movie.InputFrames[1].Controllers[0].B = true;
		movie.InputFrames[1].Controllers[0].Up = true;
		SetMovie(movie);
		_vm.SelectedFrameIndex = 1;

		_vm.ClearSelectedInput();

		Assert.False(movie.InputFrames[1].Controllers[0].A);
		Assert.False(movie.InputFrames[1].Controllers[0].B);
		Assert.False(movie.InputFrames[1].Controllers[0].Up);
		Assert.True(_vm.CanUndo);
	}

	[Fact]
	public void ClearSelectedInput_OutOfRange_NoOp() {
		var movie = CreateTestMovie(3);
		SetMovie(movie);
		_vm.SelectedFrameIndex = 5;

		_vm.ClearSelectedInput();
		Assert.False(_vm.CanUndo);
	}

	#endregion

	#region Copy/Cut/Paste

	[Fact]
	public void Copy_NoMovie_NoOp() {
		_vm.Copy();
		Assert.False(_vm.HasClipboard);
	}

	[Fact]
	public void Copy_SetsHasClipboard() {
		var movie = CreateTestMovie(5);
		SetMovie(movie);
		_vm.SelectedFrameIndex = 2;

		_vm.Copy();

		Assert.True(_vm.HasClipboard);
	}

	[Fact]
	public void CopyThenPaste_InsertsClonedFrame() {
		var movie = CreateTestMovie(5);
		movie.InputFrames[2].Controllers[0].A = true;
		movie.InputFrames[2].Controllers[0].Start = true;
		SetMovie(movie);
		_vm.SelectedFrameIndex = 2;

		_vm.Copy();
		_vm.Paste();

		// Should have 6 frames (original 5 + 1 pasted)
		Assert.Equal(6, movie.InputFrames.Count);
		Assert.Equal(6, _vm.Frames.Count);
		// The pasted frame (at index 3) should have the same buttons
		Assert.True(movie.InputFrames[3].Controllers[0].A);
		Assert.True(movie.InputFrames[3].Controllers[0].Start);
		Assert.True(_vm.CanUndo);
	}

	[Fact]
	public void CopyThenPaste_IsDeepCopy() {
		var movie = CreateTestMovie(5);
		movie.InputFrames[0].Controllers[0].A = true;
		SetMovie(movie);
		_vm.SelectedFrameIndex = 0;

		_vm.Copy();
		_vm.Paste();

		// Modify original — pasted copy should be independent
		movie.InputFrames[0].Controllers[0].A = false;
		Assert.True(movie.InputFrames[1].Controllers[0].A);
	}

	[Fact]
	public void Paste_WithoutCopy_NoOp() {
		var movie = CreateTestMovie(5);
		SetMovie(movie);
		_vm.SelectedFrameIndex = 0;

		_vm.Paste();

		Assert.Equal(5, movie.InputFrames.Count);
		Assert.False(_vm.CanUndo);
	}

	[Fact]
	public void Cut_RemovesFrameAndSetsClipboard() {
		var movie = CreateTestMovie(5);
		movie.InputFrames[2].Controllers[0].X = true;
		SetMovie(movie);
		_vm.SelectedFrameIndex = 2;

		_vm.Cut();

		Assert.Equal(4, movie.InputFrames.Count);
		Assert.Equal(4, _vm.Frames.Count);
		Assert.True(_vm.HasClipboard);
		Assert.True(_vm.CanUndo);
	}

	[Fact]
	public void CutThenPaste_RestoresFrame() {
		var movie = CreateTestMovie(5);
		movie.InputFrames[2].Controllers[0].Y = true;
		SetMovie(movie);
		_vm.SelectedFrameIndex = 2;

		_vm.Cut();
		Assert.Equal(4, movie.InputFrames.Count);

		_vm.SelectedFrameIndex = 0;
		_vm.Paste();

		Assert.Equal(5, movie.InputFrames.Count);
		// The pasted frame should have Y set
		Assert.True(movie.InputFrames[1].Controllers[0].Y);
	}

	[Fact]
	public void Cut_NoMovie_NoOp() {
		_vm.Cut();
		Assert.False(_vm.HasClipboard);
	}

	#endregion

	#region Undo/Redo Through ViewModel

	[Fact]
	public void Undo_NoActions_NoOp() {
		var movie = CreateTestMovie(5);
		SetMovie(movie);

		_vm.Undo();

		Assert.Equal(5, movie.InputFrames.Count);
		Assert.False(_vm.CanUndo);
	}

	[Fact]
	public void Undo_AfterInsert_RemovesFrame() {
		var movie = CreateTestMovie(5);
		SetMovie(movie);
		_vm.SelectedFrameIndex = 2;

		_vm.InsertFrames();
		Assert.Equal(6, movie.InputFrames.Count);

		_vm.Undo();
		Assert.Equal(5, movie.InputFrames.Count);
		Assert.Equal(5, _vm.Frames.Count);
		Assert.False(_vm.CanUndo);
		Assert.True(_vm.CanRedo);
	}

	[Fact]
	public void Redo_AfterUndo_ReInsertsFrame() {
		var movie = CreateTestMovie(5);
		SetMovie(movie);
		_vm.SelectedFrameIndex = 2;

		_vm.InsertFrames();
		_vm.Undo();
		Assert.Equal(5, movie.InputFrames.Count);

		_vm.Redo();
		Assert.Equal(6, movie.InputFrames.Count);
		Assert.Equal(6, _vm.Frames.Count);
		Assert.True(_vm.CanUndo);
		Assert.False(_vm.CanRedo);
	}

	[Fact]
	public void Undo_AfterDelete_RestoresFrame() {
		var movie = CreateTestMovie(5);
		movie.InputFrames[2].Controllers[0].L = true;
		SetMovie(movie);
		_vm.SelectedFrameIndex = 2;

		_vm.DeleteFrames();
		Assert.Equal(4, movie.InputFrames.Count);

		_vm.Undo();
		Assert.Equal(5, movie.InputFrames.Count);
		Assert.True(movie.InputFrames[2].Controllers[0].L);
	}

	[Fact]
	public void Undo_AfterToggle_RestoresButtonState() {
		var movie = CreateTestMovie(5);
		SetMovie(movie);
		_vm.SelectedFrameIndex = 0;

		Assert.False(movie.InputFrames[0].Controllers[0].A);
		_vm.ToggleButton(0, "A");
		Assert.True(movie.InputFrames[0].Controllers[0].A);

		_vm.Undo();
		Assert.False(movie.InputFrames[0].Controllers[0].A);
	}

	[Fact]
	public void Undo_AfterClear_RestoresInput() {
		var movie = CreateTestMovie(3);
		movie.InputFrames[0].Controllers[0].A = true;
		movie.InputFrames[0].Controllers[0].R = true;
		SetMovie(movie);
		_vm.SelectedFrameIndex = 0;

		_vm.ClearSelectedInput();
		Assert.False(movie.InputFrames[0].Controllers[0].A);
		Assert.False(movie.InputFrames[0].Controllers[0].R);

		_vm.Undo();
		Assert.True(movie.InputFrames[0].Controllers[0].A);
		Assert.True(movie.InputFrames[0].Controllers[0].R);
	}

	[Fact]
	public void NewAction_ClearsRedoStack() {
		var movie = CreateTestMovie(5);
		SetMovie(movie);
		_vm.SelectedFrameIndex = 0;

		_vm.InsertFrames();
		_vm.Undo();
		Assert.True(_vm.CanRedo);

		// New action should clear redo
		_vm.SelectedFrameIndex = 0;
		_vm.ToggleButton(0, "A");
		Assert.False(_vm.CanRedo);
	}

	[Fact]
	public void MultipleUndoRedo_FullCycle() {
		var movie = CreateTestMovie(5);
		SetMovie(movie);

		// Insert
		_vm.SelectedFrameIndex = 0;
		_vm.InsertFrames();
		Assert.Equal(6, movie.InputFrames.Count);

		// Toggle
		_vm.SelectedFrameIndex = 0;
		_vm.ToggleButton(0, "A");

		// Delete
		_vm.SelectedFrameIndex = 2;
		_vm.DeleteFrames();
		Assert.Equal(5, movie.InputFrames.Count);

		// Undo all 3
		_vm.Undo(); // undo delete
		Assert.Equal(6, movie.InputFrames.Count);

		_vm.Undo(); // undo toggle
		Assert.False(movie.InputFrames[0].Controllers[0].A);

		_vm.Undo(); // undo insert
		Assert.Equal(5, movie.InputFrames.Count);
		Assert.False(_vm.CanUndo);

		// Redo all 3
		_vm.Redo(); // redo insert
		Assert.Equal(6, movie.InputFrames.Count);

		_vm.Redo(); // redo toggle
		Assert.True(movie.InputFrames[0].Controllers[0].A);

		_vm.Redo(); // redo delete
		Assert.Equal(5, movie.InputFrames.Count);
		Assert.False(_vm.CanRedo);
	}

	#endregion

	#region PaintButton

	[Fact]
	public void PaintButton_NoMovie_NoOp() {
		_vm.PaintButton([0, 1, 2], 0, "A", true);
		Assert.False(_vm.CanUndo);
	}

	[Fact]
	public void PaintButton_SetsButtonOnMultipleFrames() {
		var movie = CreateTestMovie(5);
		SetMovie(movie);

		_vm.PaintButton([0, 2, 4], 0, "A", true);

		Assert.True(movie.InputFrames[0].Controllers[0].A);
		Assert.False(movie.InputFrames[1].Controllers[0].A);
		Assert.True(movie.InputFrames[2].Controllers[0].A);
		Assert.False(movie.InputFrames[3].Controllers[0].A);
		Assert.True(movie.InputFrames[4].Controllers[0].A);
		Assert.True(_vm.CanUndo);
	}

	[Fact]
	public void PaintButton_Undo_RestoresOriginalState() {
		var movie = CreateTestMovie(5);
		movie.InputFrames[0].Controllers[0].B = true;
		movie.InputFrames[2].Controllers[0].B = false;
		SetMovie(movie);

		_vm.PaintButton([0, 2], 0, "B", false);
		Assert.False(movie.InputFrames[0].Controllers[0].B);
		Assert.False(movie.InputFrames[2].Controllers[0].B);

		_vm.Undo();
		Assert.True(movie.InputFrames[0].Controllers[0].B);
		Assert.False(movie.InputFrames[2].Controllers[0].B);
	}

	[Fact]
	public void PaintButton_SkipsInvalidFrameIndices() {
		var movie = CreateTestMovie(3);
		SetMovie(movie);

		_vm.PaintButton([-1, 1, 99], 0, "A", true);

		Assert.False(movie.InputFrames[0].Controllers[0].A);
		Assert.True(movie.InputFrames[1].Controllers[0].A);
		Assert.False(movie.InputFrames[2].Controllers[0].A);
	}

	[Fact]
	public void PaintButton_EmptyIndices_NoOp() {
		var movie = CreateTestMovie(3);
		SetMovie(movie);

		_vm.PaintButton([], 0, "A", true);
		Assert.False(_vm.CanUndo);
	}

	#endregion

	#region Playback Controls

	[Fact]
	public void TogglePlayback_StartsWhenNotPlaying() {
		var movie = CreateTestMovie(5);
		SetMovie(movie);
		_vm.SelectedFrameIndex = 2;

		_vm.TogglePlayback();

		Assert.True(_vm.IsPlaying);
		Assert.Equal(2, _vm.PlaybackFrame);
	}

	[Fact]
	public void StartPlayback_NoMovie_NoOp() {
		_vm.StartPlayback();
		Assert.False(_vm.IsPlaying);
	}

	[Fact]
	public void StartPlayback_NegativeSelection_StartsAtZero() {
		var movie = CreateTestMovie(5);
		SetMovie(movie);
		_vm.SelectedFrameIndex = -1;

		_vm.StartPlayback();

		Assert.True(_vm.IsPlaying);
		Assert.Equal(0, _vm.PlaybackFrame);
	}

	#endregion

	#region StatusMessage Updates

	[Fact]
	public void InsertFrames_UpdatesStatusMessage() {
		var movie = CreateTestMovie(5);
		SetMovie(movie);
		_vm.SelectedFrameIndex = 2;

		_vm.InsertFrames();

		Assert.Contains("Inserted", _vm.StatusMessage);
	}

	[Fact]
	public void DeleteFrames_UpdatesStatusMessage() {
		var movie = CreateTestMovie(5);
		SetMovie(movie);
		_vm.SelectedFrameIndex = 2;

		_vm.DeleteFrames();

		Assert.Contains("Deleted", _vm.StatusMessage);
	}

	[Fact]
	public void ClearSelectedInput_UpdatesStatusMessage() {
		var movie = CreateTestMovie(3);
		SetMovie(movie);
		_vm.SelectedFrameIndex = 0;

		_vm.ClearSelectedInput();

		Assert.Contains("Cleared", _vm.StatusMessage);
	}

	[Fact]
	public void Undo_UpdatesStatusMessage() {
		var movie = CreateTestMovie(5);
		SetMovie(movie);
		_vm.SelectedFrameIndex = 0;
		_vm.InsertFrames();

		_vm.Undo();

		Assert.Contains("Undid", _vm.StatusMessage);
	}

	[Fact]
	public void Redo_UpdatesStatusMessage() {
		var movie = CreateTestMovie(5);
		SetMovie(movie);
		_vm.SelectedFrameIndex = 0;
		_vm.InsertFrames();
		_vm.Undo();

		_vm.Redo();

		Assert.Contains("Redid", _vm.StatusMessage);
	}

	[Fact]
	public void Copy_UpdatesStatusMessage() {
		var movie = CreateTestMovie(5);
		SetMovie(movie);
		_vm.SelectedFrameIndex = 3;

		_vm.Copy();

		Assert.Contains("Copied", _vm.StatusMessage);
	}

	[Fact]
	public void Paste_UpdatesStatusMessage() {
		var movie = CreateTestMovie(5);
		SetMovie(movie);
		_vm.SelectedFrameIndex = 0;
		_vm.Copy();

		_vm.Paste();

		Assert.Contains("Pasted", _vm.StatusMessage);
	}

	#endregion

	#region HasUnsavedChanges

	[Fact]
	public void InitiallyLoaded_NoUnsavedChanges() {
		var movie = CreateTestMovie(5);
		SetMovie(movie);

		Assert.False(_vm.HasUnsavedChanges);
	}

	[Fact]
	public void AfterInsert_HasUnsavedChanges() {
		var movie = CreateTestMovie(5);
		SetMovie(movie);
		_vm.SelectedFrameIndex = 0;
		_vm.InsertFrames();

		Assert.True(_vm.HasUnsavedChanges);
	}

	[Fact]
	public void AfterUndo_StillHasUnsavedChanges() {
		var movie = CreateTestMovie(5);
		SetMovie(movie);
		_vm.SelectedFrameIndex = 0;
		_vm.InsertFrames();
		_vm.Undo();

		// Undo still counts as unsaved changes
		Assert.True(_vm.HasUnsavedChanges);
	}

	#endregion

	#region Frames Collection Integrity

	[Fact]
	public void InsertThenDelete_FrameCountRestored() {
		var movie = CreateTestMovie(5);
		SetMovie(movie);
		_vm.SelectedFrameIndex = 2;

		_vm.InsertFrames();
		Assert.Equal(6, _vm.Frames.Count);

		_vm.SelectedFrameIndex = 2;
		_vm.DeleteFrames();
		Assert.Equal(5, _vm.Frames.Count);
	}

	[Fact]
	public void MultipleInserts_FramesSyncWithMovieData() {
		var movie = CreateTestMovie(3);
		SetMovie(movie);

		_vm.SelectedFrameIndex = 0;
		_vm.InsertFrames();
		_vm.InsertFrames();
		_vm.InsertFrames();

		Assert.Equal(6, movie.InputFrames.Count);
		Assert.Equal(6, _vm.Frames.Count);

		// All frame numbers should still be 1-based sequential
		for (int i = 0; i < _vm.Frames.Count; i++) {
			Assert.Equal(i + 1, _vm.Frames[i].FrameNumber);
		}
	}

	[Fact]
	public void FrameViewModel_PointsToCorrectInputFrame() {
		var movie = CreateTestMovie(5);
		movie.InputFrames[3].Controllers[0].A = true;
		SetMovie(movie);

		Assert.Same(movie.InputFrames[3], _vm.Frames[3].Frame);
		Assert.True(_vm.Frames[3].Frame.Controllers[0].A);
	}

	#endregion

	#region Helpers

	private static bool GetButtonState(ControllerInput input, string button) {
		return button.ToUpperInvariant() switch {
			"A" => input.A,
			"B" => input.B,
			"X" => input.X,
			"Y" => input.Y,
			"L" => input.L,
			"R" => input.R,
			"UP" => input.Up,
			"DOWN" => input.Down,
			"LEFT" => input.Left,
			"RIGHT" => input.Right,
			"START" => input.Start,
			"SELECT" => input.Select,
			_ => false
		};
	}

	#endregion
}
