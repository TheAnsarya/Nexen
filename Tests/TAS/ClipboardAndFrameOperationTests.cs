using Nexen.MovieConverter;
using Xunit;

namespace Nexen.Tests.TAS;

/// <summary>
/// Tests for TAS clipboard operations and frame manipulation logic.
/// Tests the cut/copy/paste patterns used by TasEditorViewModel
/// without requiring the full ViewModel (which depends on UI).
/// </summary>
public class ClipboardAndFrameOperationTests {
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

	#endregion

	#region Copy Logic Tests

	[Fact]
	public void Copy_ClonesSelectedFrame() {
		var movie = CreateTestMovie(10);
		int selectedIndex = 4;
		var original = movie.InputFrames[selectedIndex];

		// Simulate copy: clone the selected frame
		var clipboard = new List<InputFrame> { original.Clone() };

		Assert.Single(clipboard);
		Assert.Equal(original.Controllers[0].A, clipboard[0].Controllers[0].A);
		Assert.NotSame(original, clipboard[0]);
	}

	[Fact]
	public void Copy_ClipboardIsIndependentOfMovie() {
		var movie = CreateTestMovie(10);
		int selectedIndex = 3;
		var clipboard = new List<InputFrame> { movie.InputFrames[selectedIndex].Clone() };

		// Modify original after copy
		movie.InputFrames[selectedIndex].Controllers[0].A = !movie.InputFrames[selectedIndex].Controllers[0].A;

		// Clipboard should be unchanged
		Assert.NotEqual(movie.InputFrames[selectedIndex].Controllers[0].A, clipboard[0].Controllers[0].A);
	}

	#endregion

	#region Paste Logic Tests

	[Fact]
	public void Paste_InsertsClonedFrameAfterSelection() {
		var movie = CreateTestMovie(5);
		int selectedIndex = 2;

		// Copy frame 2
		var clipboard = new List<InputFrame> { movie.InputFrames[selectedIndex].Clone() };

		// Paste after selection
		int insertAt = selectedIndex + 1;
		var clonedForPaste = clipboard.Select(f => f.Clone()).ToList();
		movie.InputFrames.InsertRange(insertAt, clonedForPaste);

		Assert.Equal(6, movie.InputFrames.Count);
		Assert.Equal(
			clipboard[0].Controllers[0].A,
			movie.InputFrames[insertAt].Controllers[0].A);
	}

	[Fact]
	public void Paste_MultipleTimes_CreatesSeparateCopies() {
		var movie = CreateTestMovie(5);
		var clipboard = new List<InputFrame> {
			new(0) { Controllers = [new ControllerInput { Start = true }] }
		};

		// Paste 3 times
		for (int p = 0; p < 3; p++) {
			var cloned = clipboard.Select(f => f.Clone()).ToList();
			movie.InputFrames.InsertRange(movie.InputFrames.Count, cloned);
		}

		Assert.Equal(8, movie.InputFrames.Count);

		// All pasted frames should be independent copies
		movie.InputFrames[5].Controllers[0].Start = false;
		Assert.True(movie.InputFrames[6].Controllers[0].Start);
		Assert.True(movie.InputFrames[7].Controllers[0].Start);
	}

	#endregion

	#region Cut Logic Tests

	[Fact]
	public void Cut_CopiesAndRemovesFrame() {
		var movie = CreateTestMovie(10);
		int cutIndex = 5;
		var originalA = movie.InputFrames[cutIndex].Controllers[0].A;

		// Copy
		var clipboard = new List<InputFrame> { movie.InputFrames[cutIndex].Clone() };

		// Delete
		movie.InputFrames.RemoveAt(cutIndex);

		Assert.Equal(9, movie.InputFrames.Count);
		Assert.Single(clipboard);
		Assert.Equal(originalA, clipboard[0].Controllers[0].A);
	}

	[Fact]
	public void CutPaste_MovesFrameToNewPosition() {
		var movie = CreateTestMovie(10);
		int cutIndex = 2;
		var cutFrame = movie.InputFrames[cutIndex];
		var expectedA = cutFrame.Controllers[0].A;

		// Cut
		var clipboard = new List<InputFrame> { cutFrame.Clone() };
		movie.InputFrames.RemoveAt(cutIndex);
		Assert.Equal(9, movie.InputFrames.Count);

		// Paste at end
		var cloned = clipboard.Select(f => f.Clone()).ToList();
		movie.InputFrames.InsertRange(movie.InputFrames.Count, cloned);

		Assert.Equal(10, movie.InputFrames.Count);
		Assert.Equal(expectedA, movie.InputFrames[^1].Controllers[0].A);
	}

	#endregion

	#region Insert Frame Tests

	[Fact]
	public void InsertFrame_CreatesNewBlankFrame() {
		var movie = CreateTestMovie(5);
		int insertAt = 3;

		movie.InputFrames.Insert(insertAt, new InputFrame(insertAt));

		Assert.Equal(6, movie.InputFrames.Count);
		Assert.False(movie.InputFrames[insertAt].Controllers[0].A);
		Assert.False(movie.InputFrames[insertAt].Controllers[0].B);
	}

	[Fact]
	public void InsertFrame_AtBeginning_ShiftsAll() {
		var movie = CreateTestMovie(5);
		var firstFrame = movie.InputFrames[0];

		movie.InputFrames.Insert(0, new InputFrame(0));

		Assert.Equal(6, movie.InputFrames.Count);
		Assert.Same(firstFrame, movie.InputFrames[1]);
	}

	[Fact]
	public void InsertFrame_AtEnd_Appends() {
		var movie = CreateTestMovie(5);

		movie.InputFrames.Insert(5, new InputFrame(5));

		Assert.Equal(6, movie.InputFrames.Count);
	}

	#endregion

	#region Delete Frame Tests

	[Fact]
	public void DeleteFrame_RemovesCorrectFrame() {
		var movie = CreateTestMovie(10);
		var frameAfterDelete = movie.InputFrames[4];

		movie.InputFrames.RemoveAt(3);

		Assert.Equal(9, movie.InputFrames.Count);
		Assert.Same(frameAfterDelete, movie.InputFrames[3]);
	}

	[Fact]
	public void DeleteFrame_LastFrame_ReducesCount() {
		var movie = CreateTestMovie(5);

		movie.InputFrames.RemoveAt(4);

		Assert.Equal(4, movie.InputFrames.Count);
	}

	[Fact]
	public void DeleteFrame_OnlyFrame_LeavesEmpty() {
		var movie = CreateTestMovie(1);

		movie.InputFrames.RemoveAt(0);

		Assert.Empty(movie.InputFrames);
	}

	#endregion

	#region Clear Input Tests

	[Fact]
	public void ClearInput_ResetsAllButtons() {
		var movie = CreateTestMovie(5);
		var frame = movie.InputFrames[2];
		frame.Controllers[0].A = true;
		frame.Controllers[0].B = true;
		frame.Controllers[0].Start = true;
		frame.Controllers[0].Up = true;

		// Simulate ClearSelectedInput
		for (int i = 0; i < frame.Controllers.Length; i++) {
			frame.Controllers[i] = new ControllerInput();
		}

		Assert.False(frame.Controllers[0].A);
		Assert.False(frame.Controllers[0].B);
		Assert.False(frame.Controllers[0].Start);
		Assert.False(frame.Controllers[0].Up);
	}

	[Fact]
	public void ClearInput_DoesNotAffectOtherFrames() {
		var movie = CreateTestMovie(5);
		movie.InputFrames[1].Controllers[0].A = true;
		movie.InputFrames[2].Controllers[0].A = true;
		movie.InputFrames[3].Controllers[0].A = true;

		// Clear only frame 2
		var frame = movie.InputFrames[2];
		for (int i = 0; i < frame.Controllers.Length; i++) {
			frame.Controllers[i] = new ControllerInput();
		}

		Assert.True(movie.InputFrames[1].Controllers[0].A);
		Assert.False(movie.InputFrames[2].Controllers[0].A);
		Assert.True(movie.InputFrames[3].Controllers[0].A);
	}

	#endregion

	#region Comment Toggle Tests

	[Fact]
	public void SetComment_TogglesOn() {
		var movie = CreateTestMovie(5);
		var frame = movie.InputFrames[2];
		Assert.Null(frame.Comment);

		frame.Comment = "Comment 2";

		Assert.Equal("Comment 2", frame.Comment);
	}

	[Fact]
	public void SetComment_TogglesOff() {
		var movie = CreateTestMovie(5);
		var frame = movie.InputFrames[2];
		frame.Comment = "Existing comment";

		frame.Comment = null;

		Assert.Null(frame.Comment);
	}

	#endregion

	#region Branch Operation Tests

	[Fact]
	public void CreateBranch_ClonesAllFrames() {
		var movie = CreateTestMovie(10);

		// Simulate branch creation
		var branchFrames = movie.InputFrames.Select(f => f.Clone()).ToList();

		Assert.Equal(movie.InputFrames.Count, branchFrames.Count);

		// Verify deep copy
		for (int i = 0; i < movie.InputFrames.Count; i++) {
			Assert.NotSame(movie.InputFrames[i], branchFrames[i]);
			Assert.Equal(
				movie.InputFrames[i].Controllers[0].A,
				branchFrames[i].Controllers[0].A);
		}
	}

	[Fact]
	public void LoadBranch_ReplacesAllFrames() {
		var movie = CreateTestMovie(10);
		var branchFrames = new List<InputFrame>();
		for (int i = 0; i < 5; i++) {
			branchFrames.Add(new InputFrame(i) {
				Controllers = [new ControllerInput { Start = true }]
			});
		}

		// Simulate branch load
		movie.InputFrames.Clear();
		foreach (var frame in branchFrames) {
			movie.InputFrames.Add(frame.Clone());
		}

		Assert.Equal(5, movie.InputFrames.Count);
		Assert.True(movie.InputFrames[0].Controllers[0].Start);
	}

	[Fact]
	public void LoadBranch_BranchUnaffectedBySubsequentEdits() {
		var movie = CreateTestMovie(5);
		var branchFrames = movie.InputFrames.Select(f => f.Clone()).ToList();

		// Load branch clones into movie
		movie.InputFrames.Clear();
		foreach (var frame in branchFrames) {
			movie.InputFrames.Add(frame.Clone());
		}

		// Edit movie
		movie.InputFrames[0].Controllers[0].A = !movie.InputFrames[0].Controllers[0].A;

		// Branch should be unchanged
		Assert.NotEqual(
			movie.InputFrames[0].Controllers[0].A,
			branchFrames[0].Controllers[0].A);
	}

	#endregion

	#region Button Toggle Tests

	[Fact]
	public void ToggleButton_FlipsState() {
		var frame = new InputFrame(0) {
			Controllers = [new ControllerInput { A = false }]
		};

		// Toggle A on
		frame.Controllers[0].A = !frame.Controllers[0].A;
		Assert.True(frame.Controllers[0].A);

		// Toggle A off
		frame.Controllers[0].A = !frame.Controllers[0].A;
		Assert.False(frame.Controllers[0].A);
	}

	[Fact]
	public void SetButton_SetsExactState() {
		var frame = new InputFrame(0) {
			Controllers = [new ControllerInput()]
		};

		// Set A to true
		frame.Controllers[0].A = true;
		Assert.True(frame.Controllers[0].A);

		// Set A to true again (no change)
		frame.Controllers[0].A = true;
		Assert.True(frame.Controllers[0].A);

		// Set A to false
		frame.Controllers[0].A = false;
		Assert.False(frame.Controllers[0].A);
	}

	#endregion
}
