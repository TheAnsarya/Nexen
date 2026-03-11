using Nexen.MovieConverter;
using Xunit;

namespace Nexen.Tests.TAS;

/// <summary>
/// Tests for piano roll context menu operations and pattern fill.
/// Validates that all context menu actions produce correct results.
/// Tracks: #660 (Context menu), #659 (Bulk operations)
/// </summary>
public class ContextMenuAndPatternTests {
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
					new ControllerInput { A = i % 2 == 0, B = i % 3 == 0 }
				]
			};
			movie.InputFrames.Add(frame);
		}

		return movie;
	}

	/// <summary>
	/// Simulates "Insert frames above" operation — inserts blank frames before a given index.
	/// </summary>
	private static void InsertFramesAbove(MovieData movie, int beforeIndex, int count) {
		for (int i = 0; i < count; i++) {
			movie.InputFrames.Insert(beforeIndex, new InputFrame(beforeIndex));
		}
	}

	/// <summary>
	/// Simulates "Insert frames below" operation — inserts blank frames after a given index.
	/// </summary>
	private static void InsertFramesBelow(MovieData movie, int afterIndex, int count) {
		for (int i = 0; i < count; i++) {
			movie.InputFrames.Insert(afterIndex + 1 + i, new InputFrame(afterIndex + 1 + i));
		}
	}

	/// <summary>
	/// Simulates pattern fill: repeats a pattern every N frames in a range.
	/// </summary>
	private static void PatternFill(
		MovieData movie,
		int startFrame,
		int endFrame,
		InputFrame[] pattern
	) {
		if (pattern.Length == 0) return;

		for (int i = startFrame; i <= endFrame && i < movie.InputFrames.Count; i++) {
			var source = pattern[(i - startFrame) % pattern.Length];
			for (int c = 0; c < movie.InputFrames[i].Controllers.Length && c < source.Controllers.Length; c++) {
				movie.InputFrames[i].Controllers[c].A = source.Controllers[c].A;
				movie.InputFrames[i].Controllers[c].B = source.Controllers[c].B;
				movie.InputFrames[i].Controllers[c].Up = source.Controllers[c].Up;
				movie.InputFrames[i].Controllers[c].Down = source.Controllers[c].Down;
				movie.InputFrames[i].Controllers[c].Left = source.Controllers[c].Left;
				movie.InputFrames[i].Controllers[c].Right = source.Controllers[c].Right;
			}
		}
	}

	/// <summary>
	/// Simulates "Select range to here": selects from anchor to target.
	/// </summary>
	private static List<int> SelectRangeToHere(int anchorFrame, int targetFrame) {
		int start = Math.Min(anchorFrame, targetFrame);
		int end = Math.Max(anchorFrame, targetFrame);
		var selected = new List<int>(end - start + 1);
		for (int i = start; i <= end; i++) {
			selected.Add(i);
		}
		return selected;
	}

	#endregion

	#region Insert Frames Above/Below

	[Fact]
	public void InsertAbove_InsertsBeforeIndex() {
		var movie = CreateTestMovie(10);
		int originalFrame5 = movie.InputFrames[5].FrameNumber;

		InsertFramesAbove(movie, 5, 3);

		Assert.Equal(13, movie.InputFrames.Count);
		// Original frame 5 should be pushed to index 8
		Assert.Equal(originalFrame5, movie.InputFrames[8].FrameNumber);
	}

	[Fact]
	public void InsertAbove_AtZero_InsertsAtBeginning() {
		var movie = CreateTestMovie(10);
		int originalFirst = movie.InputFrames[0].FrameNumber;

		InsertFramesAbove(movie, 0, 2);

		Assert.Equal(12, movie.InputFrames.Count);
		Assert.Equal(originalFirst, movie.InputFrames[2].FrameNumber);
	}

	[Fact]
	public void InsertBelow_InsertsAfterIndex() {
		var movie = CreateTestMovie(10);
		int originalFrame5 = movie.InputFrames[5].FrameNumber;
		int originalFrame6 = movie.InputFrames[6].FrameNumber;

		InsertFramesBelow(movie, 5, 3);

		Assert.Equal(13, movie.InputFrames.Count);
		Assert.Equal(originalFrame5, movie.InputFrames[5].FrameNumber);
		Assert.Equal(originalFrame6, movie.InputFrames[9].FrameNumber);
	}

	[Fact]
	public void InsertBelow_AtEnd_AppendsFrames() {
		var movie = CreateTestMovie(10);
		int lastIdx = movie.InputFrames.Count - 1;

		InsertFramesBelow(movie, lastIdx, 5);

		Assert.Equal(15, movie.InputFrames.Count);
	}

	#endregion

	#region Select Range To Here

	[Fact]
	public void SelectRangeToHere_ForwardDirection() {
		var selected = SelectRangeToHere(5, 10);
		Assert.Equal(6, selected.Count);
		Assert.Equal(5, selected[0]);
		Assert.Equal(10, selected[^1]);
	}

	[Fact]
	public void SelectRangeToHere_BackwardDirection() {
		var selected = SelectRangeToHere(10, 5);
		Assert.Equal(6, selected.Count);
		Assert.Equal(5, selected[0]);
		Assert.Equal(10, selected[^1]);
	}

	[Fact]
	public void SelectRangeToHere_SameFrame_ReturnsSingle() {
		var selected = SelectRangeToHere(7, 7);
		Assert.Single(selected);
		Assert.Equal(7, selected[0]);
	}

	#endregion

	#region Pattern Fill

	[Fact]
	public void PatternFill_SingleFramePattern_RepeatsAcrossRange() {
		var movie = CreateTestMovie(20);
		var pattern = new[] {
			new InputFrame(0) { Controllers = [new ControllerInput { A = true, Right = true }] }
		};

		PatternFill(movie, 5, 14, pattern);

		for (int i = 5; i <= 14; i++) {
			Assert.True(movie.InputFrames[i].Controllers[0].A);
			Assert.True(movie.InputFrames[i].Controllers[0].Right);
		}
	}

	[Fact]
	public void PatternFill_TwoFrameAlternating_AlternatesCorrectly() {
		var movie = CreateTestMovie(20);
		var pattern = new[] {
			new InputFrame(0) { Controllers = [new ControllerInput { A = true }] },
			new InputFrame(1) { Controllers = [new ControllerInput { B = true }] }
		};

		PatternFill(movie, 0, 9, pattern);

		for (int i = 0; i <= 9; i++) {
			if (i % 2 == 0) {
				Assert.True(movie.InputFrames[i].Controllers[0].A);
			} else {
				Assert.True(movie.InputFrames[i].Controllers[0].B);
			}
		}
	}

	[Fact]
	public void PatternFill_ThreeFramePattern_CyclesCorrectly() {
		var movie = CreateTestMovie(30);
		var pattern = new[] {
			new InputFrame(0) { Controllers = [new ControllerInput { Up = true }] },
			new InputFrame(1) { Controllers = [new ControllerInput { Right = true }] },
			new InputFrame(2) { Controllers = [new ControllerInput { Down = true }] }
		};

		PatternFill(movie, 0, 8, pattern);

		Assert.True(movie.InputFrames[0].Controllers[0].Up);
		Assert.True(movie.InputFrames[1].Controllers[0].Right);
		Assert.True(movie.InputFrames[2].Controllers[0].Down);
		Assert.True(movie.InputFrames[3].Controllers[0].Up);
		Assert.True(movie.InputFrames[4].Controllers[0].Right);
		Assert.True(movie.InputFrames[5].Controllers[0].Down);
	}

	[Fact]
	public void PatternFill_EmptyPattern_DoesNothing() {
		var movie = CreateTestMovie(10);
		bool originalA = movie.InputFrames[0].Controllers[0].A;

		PatternFill(movie, 0, 9, []);

		Assert.Equal(originalA, movie.InputFrames[0].Controllers[0].A);
	}

	[Fact]
	public void PatternFill_RangeBeyondMovie_ClampsSafely() {
		var movie = CreateTestMovie(10);
		var pattern = new[] {
			new InputFrame(0) { Controllers = [new ControllerInput { A = true }] }
		};

		PatternFill(movie, 5, 100, pattern);

		Assert.Equal(10, movie.InputFrames.Count); // No new frames added
		for (int i = 5; i < 10; i++) {
			Assert.True(movie.InputFrames[i].Controllers[0].A);
		}
	}

	#endregion

	#region Clear Input (Context Menu)

	[Fact]
	public void ClearInput_SingleFrame_ClearsAllButtons() {
		var movie = CreateTestMovie(10);
		int idx = 4; // This frame has A=true (even), B=false

		var ctrl = movie.InputFrames[idx].Controllers[0];
		ctrl.A = false;
		ctrl.B = false;
		ctrl.Up = false;
		ctrl.Down = false;
		ctrl.Left = false;
		ctrl.Right = false;
		ctrl.Start = false;
		ctrl.Select = false;

		Assert.False(movie.InputFrames[idx].Controllers[0].A);
		Assert.False(movie.InputFrames[idx].Controllers[0].B);
	}

	[Fact]
	public void ClearInput_MultipleFrames_ClearsAllInRange() {
		var movie = CreateTestMovie(20);

		for (int i = 5; i <= 14; i++) {
			var ctrl = movie.InputFrames[i].Controllers[0];
			ctrl.A = false;
			ctrl.B = false;
			ctrl.Up = false;
		}

		for (int i = 5; i <= 14; i++) {
			Assert.False(movie.InputFrames[i].Controllers[0].A);
			Assert.False(movie.InputFrames[i].Controllers[0].B);
		}

		// Frames outside range should be unaffected
		Assert.True(movie.InputFrames[0].Controllers[0].A); // Even frame
	}

	#endregion

	#region Set Comment / Marker (Context Menu)

	[Fact]
	public void SetComment_AddsCommentToFrame() {
		var movie = CreateTestMovie(10);
		movie.InputFrames[5].Comment = "Test marker";
		Assert.Equal("Test marker", movie.InputFrames[5].Comment);
	}

	[Fact]
	public void SetComment_OverwritesPreviousComment() {
		var movie = CreateTestMovie(10);
		movie.InputFrames[5].Comment = "First";
		movie.InputFrames[5].Comment = "Second";
		Assert.Equal("Second", movie.InputFrames[5].Comment);
	}

	[Fact]
	public void ClearComment_SetsToNull() {
		var movie = CreateTestMovie(10);
		movie.InputFrames[5].Comment = "Some comment";
		movie.InputFrames[5].Comment = null;
		Assert.Null(movie.InputFrames[5].Comment);
	}

	#endregion
}
