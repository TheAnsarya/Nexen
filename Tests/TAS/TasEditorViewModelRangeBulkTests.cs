using System.Reflection;
using System.Linq;
using Nexen.MovieConverter;
using Nexen.ViewModels;
using Xunit;

namespace Nexen.Tests.TAS;

public class TasEditorViewModelRangeBulkTests : IDisposable {
	private readonly TasEditorViewModel _vm;

	public TasEditorViewModelRangeBulkTests() {
		_vm = new TasEditorViewModel();
	}

	public void Dispose() {
		_vm.Dispose();
	}

	private static MovieData CreateTestMovie(int frameCount, int controllerCount = 1) {
		var movie = new MovieData {
			Author = "RangeBulkTest",
			GameName = "Test",
			SystemType = SystemType.Snes,
			Region = RegionType.NTSC
		};

		for (int i = 0; i < frameCount; i++) {
			var frame = new InputFrame(i) {
				Controllers = new ControllerInput[controllerCount]
			};
			for (int c = 0; c < controllerCount; c++) {
				frame.Controllers[c] = new ControllerInput();
			}
			movie.InputFrames.Add(frame);
		}

		return movie;
	}

	private void SetMovie(MovieData movie) {
		var prop = typeof(TasEditorViewModel).GetProperty(nameof(TasEditorViewModel.Movie));
		prop!.SetValue(_vm, movie);
		_vm.Recorder.Movie = movie;
	}

	[Fact]
	public void SelectAllFrames_NoMovie_NoSelection() {
		_vm.SelectAllFrames();

		Assert.Empty(_vm.SelectedFrameIndices);
		Assert.Equal(-1, _vm.SelectedFrameIndex);
	}

	[Fact]
	public void SelectAllFrames_SelectsEveryFrame() {
		SetMovie(CreateTestMovie(12));

		_vm.SelectAllFrames();

		Assert.Equal(12, _vm.SelectedFrameIndices.Count);
		Assert.Equal(0, _vm.SelectedFrameIndices.Min);
		Assert.Equal(11, _vm.SelectedFrameIndices.Max);
		Assert.Equal(0, _vm.SelectedFrameIndex);
	}

	[Fact]
	public void SelectNoFrames_ClearsSelection() {
		SetMovie(CreateTestMovie(8));
		_vm.SelectAllFrames();

		_vm.SelectNoFrames();

		Assert.Empty(_vm.SelectedFrameIndices);
		Assert.Equal(-1, _vm.SelectedFrameIndex);
	}

	[Fact]
	public void SelectFrameRange_SelectsInclusiveRange() {
		SetMovie(CreateTestMovie(20));

		_vm.SelectFrameRange(3, 9);

		Assert.Equal(7, _vm.SelectedFrameIndices.Count);
		Assert.Equal(3, _vm.SelectedFrameIndices.Min);
		Assert.Equal(9, _vm.SelectedFrameIndices.Max);
	}

	[Fact]
	public void SelectFrameRange_ReversedEndpoints_SelectsCorrectRange() {
		SetMovie(CreateTestMovie(20));

		_vm.SelectFrameRange(14, 5);

		Assert.Equal(10, _vm.SelectedFrameIndices.Count);
		Assert.Equal(5, _vm.SelectedFrameIndices.Min);
		Assert.Equal(14, _vm.SelectedFrameIndices.Max);
	}

	[Fact]
	public void SelectFrameRange_ClampsToMovieBounds() {
		SetMovie(CreateTestMovie(6));

		_vm.SelectFrameRange(-5, 50);

		Assert.Equal(6, _vm.SelectedFrameIndices.Count);
		Assert.Equal(0, _vm.SelectedFrameIndices.Min);
		Assert.Equal(5, _vm.SelectedFrameIndices.Max);
	}

	[Fact]
	public void SelectRangeTo_UsesSelectedFrameAsAnchor() {
		SetMovie(CreateTestMovie(10));
		_vm.SelectedFrameIndex = 2;

		_vm.SelectRangeTo(6);

		Assert.Equal(new[] { 2, 3, 4, 5, 6 }, _vm.SelectedFrameIndices);
	}

	[Fact]
	public void InsertFramesAt_InsertsRequestedCount() {
		SetMovie(CreateTestMovie(5, controllerCount: 2));

		_vm.InsertFramesAt(2, 3);

		Assert.Equal(8, _vm.Movie!.InputFrames.Count);
		Assert.Equal(3, _vm.SelectedFrameIndices.Count);
		Assert.All(_vm.Movie.InputFrames.Skip(2).Take(3), frame => Assert.Equal(2, frame.Controllers.Length));
	}

	[Fact]
	public void InsertFramesAt_PositionBeyondEnd_ClampsToAppend() {
		SetMovie(CreateTestMovie(5));

		_vm.InsertFramesAt(999, 2);

		Assert.Equal(7, _vm.Movie!.InputFrames.Count);
		Assert.Equal(new[] { 5, 6 }, _vm.SelectedFrameIndices);
	}

	[Fact]
	public void InsertFramesAt_InvalidCount_NoOp() {
		SetMovie(CreateTestMovie(5));

		_vm.InsertFramesAt(2, 0);

		Assert.Equal(5, _vm.Movie!.InputFrames.Count);
		Assert.False(_vm.CanUndo);
	}

	[Fact]
	public void DeleteFrames_WithMultiSelection_DeletesDistinctFrames() {
		SetMovie(CreateTestMovie(10));
		_vm.SelectedFrameIndices = new SortedSet<int> { 2, 4, 7 };
		_vm.SelectedFrameIndex = 7;

		_vm.DeleteFrames();

		Assert.Equal(7, _vm.Movie!.InputFrames.Count);
		Assert.True(_vm.CanUndo);
	}

	[Fact]
	public void DeleteFrames_WithMultiSelection_UndoRestoresAll() {
		SetMovie(CreateTestMovie(10));
		_vm.SelectedFrameIndices = new SortedSet<int> { 1, 3, 5 };
		_vm.SelectedFrameIndex = 5;

		_vm.DeleteFrames();
		Assert.Equal(7, _vm.Movie!.InputFrames.Count);

		_vm.Undo();
		Assert.Equal(10, _vm.Movie!.InputFrames.Count);
	}

	[Fact]
	public void ClearSelectedInput_WithMultiSelection_ClearsOnlyTargets() {
		var movie = CreateTestMovie(6);
		movie.InputFrames[1].Controllers[0].A = true;
		movie.InputFrames[2].Controllers[0].A = true;
		movie.InputFrames[3].Controllers[0].A = true;
		SetMovie(movie);
		_vm.SelectedFrameIndices = new SortedSet<int> { 1, 3 };
		_vm.SelectedFrameIndex = 3;

		_vm.ClearSelectedInput();

		Assert.False(movie.InputFrames[1].Controllers[0].A);
		Assert.True(movie.InputFrames[2].Controllers[0].A);
		Assert.False(movie.InputFrames[3].Controllers[0].A);
	}

	[Fact]
	public void ClearSelectedInput_WithMultiSelection_UndoRestores() {
		var movie = CreateTestMovie(5);
		movie.InputFrames[0].Controllers[0].B = true;
		movie.InputFrames[2].Controllers[0].B = true;
		SetMovie(movie);
		_vm.SelectedFrameIndices = new SortedSet<int> { 0, 2 };
		_vm.SelectedFrameIndex = 2;

		_vm.ClearSelectedInput();
		Assert.False(movie.InputFrames[0].Controllers[0].B);
		Assert.False(movie.InputFrames[2].Controllers[0].B);

		_vm.Undo();
		Assert.True(movie.InputFrames[0].Controllers[0].B);
		Assert.True(movie.InputFrames[2].Controllers[0].B);
	}

	[Fact]
	public void SetButtonOnSelectedFrames_SetsButtonAcrossRange() {
		var movie = CreateTestMovie(6);
		SetMovie(movie);
		_vm.SelectFrameRange(1, 4);

		_vm.SetButtonOnSelectedFrames(0, "A", true);

		for (int i = 1; i <= 4; i++) {
			Assert.True(movie.InputFrames[i].Controllers[0].A);
		}
		Assert.False(movie.InputFrames[0].Controllers[0].A);
		Assert.False(movie.InputFrames[5].Controllers[0].A);
	}

	[Fact]
	public void SetButtonOnSelectedFrames_ClearsButtonAcrossRange() {
		var movie = CreateTestMovie(6);
		for (int i = 0; i < movie.InputFrames.Count; i++) {
			movie.InputFrames[i].Controllers[0].B = true;
		}
		SetMovie(movie);
		_vm.SelectFrameRange(2, 5);

		_vm.SetButtonOnSelectedFrames(0, "B", false);

		Assert.True(movie.InputFrames[0].Controllers[0].B);
		Assert.True(movie.InputFrames[1].Controllers[0].B);
		for (int i = 2; i <= 5; i++) {
			Assert.False(movie.InputFrames[i].Controllers[0].B);
		}
	}

	[Fact]
	public void SetButtonOnSelectedFrames_UnknownButton_NoChange() {
		var movie = CreateTestMovie(4);
		SetMovie(movie);
		_vm.SelectFrameRange(0, 3);

		_vm.SetButtonOnSelectedFrames(0, "INVALID", true);

		Assert.All(movie.InputFrames, frame => Assert.False(frame.Controllers[0].A));
		Assert.Contains("Unknown button", _vm.StatusMessage);
	}

	[Fact]
	public void SetButtonOnSelectedFrames_UsesSingleSelectionFallback() {
		var movie = CreateTestMovie(4);
		SetMovie(movie);
		_vm.SelectedFrameIndex = 2;
		_vm.SelectedFrameIndices = new SortedSet<int>();

		_vm.SetButtonOnSelectedFrames(0, "A", true);

		Assert.False(movie.InputFrames[0].Controllers[0].A);
		Assert.False(movie.InputFrames[1].Controllers[0].A);
		Assert.True(movie.InputFrames[2].Controllers[0].A);
		Assert.False(movie.InputFrames[3].Controllers[0].A);
	}

	[Fact]
	public void PatternFillSelectedRange_IntervalTwo_RepeatsPattern() {
		var movie = CreateTestMovie(8);
		movie.InputFrames[1].Controllers[0].A = true;
		movie.InputFrames[2].Controllers[0].B = true;
		SetMovie(movie);
		_vm.SelectFrameRange(1, 6);

		_vm.PatternFillSelectedRange(2);

		for (int i = 1; i <= 6; i++) {
			if ((i - 1) % 2 == 0) {
				Assert.True(movie.InputFrames[i].Controllers[0].A);
				Assert.False(movie.InputFrames[i].Controllers[0].B);
			} else {
				Assert.False(movie.InputFrames[i].Controllers[0].A);
				Assert.True(movie.InputFrames[i].Controllers[0].B);
			}
		}
	}

	[Fact]
	public void PatternFillSelectedRange_LargeInterval_UsesRangeLength() {
		var movie = CreateTestMovie(5);
		movie.InputFrames[0].Controllers[0].A = true;
		movie.InputFrames[1].Controllers[0].B = true;
		movie.InputFrames[2].Controllers[0].X = true;
		SetMovie(movie);
		_vm.SelectFrameRange(0, 2);

		_vm.PatternFillSelectedRange(99);

		Assert.True(movie.InputFrames[0].Controllers[0].A);
		Assert.True(movie.InputFrames[1].Controllers[0].B);
		Assert.True(movie.InputFrames[2].Controllers[0].X);
	}

	[Fact]
	public void PatternFillSelectedRange_UndoRestoresOriginalRange() {
		var movie = CreateTestMovie(6);
		movie.InputFrames[0].Controllers[0].A = true;
		movie.InputFrames[1].Controllers[0].B = true;
		SetMovie(movie);
		_vm.SelectFrameRange(0, 5);

		_vm.PatternFillSelectedRange(2);
		Assert.True(movie.InputFrames[4].Controllers[0].A);
		Assert.True(movie.InputFrames[5].Controllers[0].B);

		_vm.Undo();
		Assert.False(movie.InputFrames[4].Controllers[0].A);
		Assert.False(movie.InputFrames[5].Controllers[0].B);
	}

	[Fact]
	public void PatternFillSelectedRange_InvalidInterval_NoOp() {
		var movie = CreateTestMovie(4);
		SetMovie(movie);
		_vm.SelectFrameRange(0, 3);

		_vm.PatternFillSelectedRange(0);

		Assert.False(_vm.CanUndo);
	}

	[Fact]
	public void SelectAllThenClearSelectedInput_ClearsWholeMovie() {
		var movie = CreateTestMovie(5);
		for (int i = 0; i < movie.InputFrames.Count; i++) {
			movie.InputFrames[i].Controllers[0].Start = true;
		}
		SetMovie(movie);
		_vm.SelectAllFrames();

		_vm.ClearSelectedInput();

		Assert.All(movie.InputFrames, frame => Assert.False(frame.Controllers[0].Start));
	}

	[Fact]
	public void BulkOperations_AreSingleUndoSteps() {
		var movie = CreateTestMovie(8);
		SetMovie(movie);
		_vm.SelectFrameRange(2, 5);

		_vm.SetButtonOnSelectedFrames(0, "A", true);
		Assert.True(_vm.CanUndo);
		_vm.Undo();
		Assert.All(movie.InputFrames, frame => Assert.False(frame.Controllers[0].A));

		_vm.SelectFrameRange(2, 5);
		_vm.ClearSelectedInput();
		Assert.True(_vm.CanUndo);
		_vm.Undo();
		Assert.Equal(8, movie.InputFrames.Count);
	}
}
