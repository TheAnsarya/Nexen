using Nexen.MovieConverter;
using Xunit;

namespace Nexen.Tests.TAS;

/// <summary>
/// Tests for multi-frame selection operations.
/// Validates that selecting multiple frames works correctly and
/// all frame operations function properly on ranges.
/// Tracks: #658 (Multi-frame selection)
/// </summary>
public class MultiFrameSelectionTests {
	#region Helpers

	private static MovieData CreateTestMovie(int frameCount) {
		var movie = new MovieData {
			Author = "Test",
			GameName = "Test Game",
			SystemType = SystemType.Nes
		};

		for (int i = 0; i < frameCount; i++) {
			var frame = new InputFrame(i) {
				Controllers = [
					new ControllerInput { A = i % 2 == 0, B = i % 3 == 0, Up = i % 5 == 0 }
				]
			};
			movie.InputFrames.Add(frame);
		}

		return movie;
	}

	private static List<int> SelectRange(int start, int count) {
		var selected = new List<int>(count);
		for (int i = start; i < start + count; i++) {
			selected.Add(i);
		}
		return selected;
	}

	private static List<int> SelectScattered(params int[] indices) => new(indices);

	#endregion

	#region Contiguous Range Selection

	[Fact]
	public void SelectRange_ReturnsContiguousIndices() {
		var selected = SelectRange(5, 10);
		Assert.Equal(10, selected.Count);
		Assert.Equal(5, selected[0]);
		Assert.Equal(14, selected[^1]);
	}

	[Fact]
	public void SelectRange_SingleFrame_ReturnsSingleIndex() {
		var selected = SelectRange(42, 1);
		Assert.Single(selected);
		Assert.Equal(42, selected[0]);
	}

	[Fact]
	public void SelectRange_AtStart_ReturnsFromZero() {
		var selected = SelectRange(0, 5);
		Assert.Equal(5, selected.Count);
		Assert.Equal(0, selected[0]);
	}

	[Theory]
	[InlineData(100, 0, 50)]
	[InlineData(100, 50, 50)]
	[InlineData(1000, 0, 1000)]
	public void SelectRange_VariousSizes_CorrectCount(int movieSize, int start, int count) {
		var movie = CreateTestMovie(movieSize);
		var selected = SelectRange(start, count);
		Assert.Equal(count, selected.Count);
		Assert.All(selected, idx => Assert.InRange(idx, start, start + count - 1));
	}

	#endregion

	#region Scattered Selection

	[Fact]
	public void SelectScattered_NonContiguous_ReturnsExactIndices() {
		var selected = SelectScattered(0, 5, 10, 20, 50);
		Assert.Equal(5, selected.Count);
		Assert.Contains(0, selected);
		Assert.Contains(50, selected);
	}

	[Fact]
	public void SelectScattered_Empty_ReturnsEmpty() {
		var selected = SelectScattered();
		Assert.Empty(selected);
	}

	[Fact]
	public void SelectScattered_SingleFrame_ReturnsSingle() {
		var selected = SelectScattered(7);
		Assert.Single(selected);
	}

	#endregion

	#region Bulk Delete on Selection

	[Fact]
	public void BulkDelete_ContiguousRange_RemovesCorrectFrames() {
		var movie = CreateTestMovie(20);
		var selected = SelectRange(5, 5); // Remove frames 5-9

		// Remove from end to preserve indices
		foreach (int idx in selected.OrderByDescending(i => i)) {
			movie.InputFrames.RemoveAt(idx);
		}

		Assert.Equal(15, movie.InputFrames.Count);
		// Frame 4 should still be at index 4
		Assert.Equal(4, movie.InputFrames[4].FrameNumber);
		// Frame 10 should now be at index 5
		Assert.Equal(10, movie.InputFrames[5].FrameNumber);
	}

	[Fact]
	public void BulkDelete_ScatteredSelection_RemovesCorrectFrames() {
		var movie = CreateTestMovie(20);
		var selected = SelectScattered(0, 5, 10, 15, 19);

		foreach (int idx in selected.OrderByDescending(i => i)) {
			movie.InputFrames.RemoveAt(idx);
		}

		Assert.Equal(15, movie.InputFrames.Count);
		// None of the removed frame numbers should remain
		var remaining = movie.InputFrames.Select(f => f.FrameNumber).ToHashSet();
		Assert.DoesNotContain(0, remaining);
		Assert.DoesNotContain(5, remaining);
		Assert.DoesNotContain(10, remaining);
		Assert.DoesNotContain(15, remaining);
		Assert.DoesNotContain(19, remaining);
	}

	[Fact]
	public void BulkDelete_AllFrames_ResultsInEmptyMovie() {
		var movie = CreateTestMovie(10);
		var selected = SelectRange(0, 10);

		foreach (int idx in selected.OrderByDescending(i => i)) {
			movie.InputFrames.RemoveAt(idx);
		}

		Assert.Empty(movie.InputFrames);
	}

	#endregion

	#region Bulk Copy on Selection

	[Fact]
	public void BulkCopy_ContiguousRange_ClonesAllFrames() {
		var movie = CreateTestMovie(20);
		var selected = SelectRange(5, 5);

		var clipboard = selected.Select(i => movie.InputFrames[i].Clone()).ToList();

		Assert.Equal(5, clipboard.Count);
		for (int i = 0; i < clipboard.Count; i++) {
			Assert.Equal(movie.InputFrames[selected[i]].Controllers[0].A, clipboard[i].Controllers[0].A);
			Assert.NotSame(movie.InputFrames[selected[i]], clipboard[i]);
		}
	}

	[Fact]
	public void BulkCopy_ScatteredSelection_ClonesOnlySelected() {
		var movie = CreateTestMovie(20);
		var selected = SelectScattered(2, 7, 13);

		var clipboard = selected.Select(i => movie.InputFrames[i].Clone()).ToList();

		Assert.Equal(3, clipboard.Count);
		Assert.Equal(movie.InputFrames[2].Controllers[0].A, clipboard[0].Controllers[0].A);
		Assert.Equal(movie.InputFrames[7].Controllers[0].A, clipboard[1].Controllers[0].A);
		Assert.Equal(movie.InputFrames[13].Controllers[0].A, clipboard[2].Controllers[0].A);
	}

	[Fact]
	public void BulkCopy_IsDeepClone_ModifyingOriginalDoesNotAffectClipboard() {
		var movie = CreateTestMovie(10);
		var selected = SelectRange(0, 5);

		var clipboard = selected.Select(i => movie.InputFrames[i].Clone()).ToList();

		// Modify original
		movie.InputFrames[0].Controllers[0].A = !movie.InputFrames[0].Controllers[0].A;

		// Clipboard should be unaffected
		Assert.NotEqual(movie.InputFrames[0].Controllers[0].A, clipboard[0].Controllers[0].A);
	}

	#endregion

	#region Bulk Clear Input

	[Fact]
	public void BulkClear_ContiguousRange_ClearsAllButtons() {
		var movie = CreateTestMovie(20);
		var selected = SelectRange(5, 5);

		foreach (int idx in selected) {
			var ctrl = movie.InputFrames[idx].Controllers[0];
			ctrl.A = false;
			ctrl.B = false;
			ctrl.Up = false;
			ctrl.Down = false;
			ctrl.Left = false;
			ctrl.Right = false;
		}

		foreach (int idx in selected) {
			var ctrl = movie.InputFrames[idx].Controllers[0];
			Assert.False(ctrl.A);
			Assert.False(ctrl.B);
			Assert.False(ctrl.Up);
		}
	}

	[Fact]
	public void BulkClear_DoesNotAffectUnselectedFrames() {
		var movie = CreateTestMovie(20);
		var selected = SelectRange(5, 5);

		// Record original values for frame 0
		bool frame0A = movie.InputFrames[0].Controllers[0].A;

		foreach (int idx in selected) {
			movie.InputFrames[idx].Controllers[0].A = false;
		}

		// Unselected frame should be unchanged
		Assert.Equal(frame0A, movie.InputFrames[0].Controllers[0].A);
	}

	#endregion

	#region Bulk Set Button

	[Fact]
	public void BulkSetButton_SetsSpecificButton_AcrossRange() {
		var movie = CreateTestMovie(20);
		var selected = SelectRange(0, 10);

		// Set "A" button on all selected frames
		foreach (int idx in selected) {
			movie.InputFrames[idx].Controllers[0].A = true;
		}

		foreach (int idx in selected) {
			Assert.True(movie.InputFrames[idx].Controllers[0].A);
		}
	}

	[Fact]
	public void BulkToggleButton_InvertsButtonState() {
		var movie = CreateTestMovie(10);
		var originalStates = movie.InputFrames.Select(f => f.Controllers[0].A).ToList();
		var selected = SelectRange(0, 10);

		foreach (int idx in selected) {
			movie.InputFrames[idx].Controllers[0].A = !movie.InputFrames[idx].Controllers[0].A;
		}

		for (int i = 0; i < 10; i++) {
			Assert.NotEqual(originalStates[i], movie.InputFrames[i].Controllers[0].A);
		}
	}

	#endregion
}
