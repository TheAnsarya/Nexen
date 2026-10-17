using Nexen.MovieConverter;
using Xunit;

namespace Nexen.Tests.TAS;

/// <summary>
/// Tests for frame search and filter functionality.
/// Validates searching by button state, comment text, lag frames, and frame ranges.
/// Tracks: #661 (Frame search and filter)
/// </summary>
public class FrameSearchTests {
	#region Helpers

	private static MovieData CreateSearchableMovie(int frameCount) {
		var movie = new MovieData {
			Author = "Test",
			GameName = "Test Game",
			SystemType = SystemType.Nes
		};

		for (int i = 0; i < frameCount; i++) {
			var frame = new InputFrame(i) {
				Controllers = [
					new ControllerInput {
						A = i % 2 == 0,
						B = i % 3 == 0,
						Up = i % 10 == 0,
						Start = i % 50 == 0
					}
				],
				IsLagFrame = i % 7 == 0,
				Comment = i % 25 == 0 ? $"Checkpoint {i}" : null
			};
			movie.InputFrames.Add(frame);
		}

		return movie;
	}

	private static List<int> SearchByButton(MovieData movie, Func<ControllerInput, bool> predicate) {
		var matches = new List<int>();
		for (int i = 0; i < movie.InputFrames.Count; i++) {
			if (movie.InputFrames[i].Controllers.Length > 0 && predicate(movie.InputFrames[i].Controllers[0])) {
				matches.Add(i);
			}
		}
		return matches;
	}

	private static List<int> SearchByComment(MovieData movie, string searchText) {
		var matches = new List<int>();
		for (int i = 0; i < movie.InputFrames.Count; i++) {
			if (movie.InputFrames[i].Comment is not null
				&& movie.InputFrames[i].Comment!.Contains(searchText, StringComparison.OrdinalIgnoreCase)) {
				matches.Add(i);
			}
		}
		return matches;
	}

	private static List<int> SearchLagFrames(MovieData movie) {
		var matches = new List<int>();
		for (int i = 0; i < movie.InputFrames.Count; i++) {
			if (movie.InputFrames[i].IsLagFrame) {
				matches.Add(i);
			}
		}
		return matches;
	}

	private static List<int> SearchByFrameRange(MovieData movie, int start, int end) {
		var matches = new List<int>();
		for (int i = Math.Max(0, start); i <= Math.Min(end, movie.InputFrames.Count - 1); i++) {
			matches.Add(i);
		}
		return matches;
	}

	#endregion

	#region Button State Search

	[Fact]
	public void SearchByButton_A_FindsEvenFrames() {
		var movie = CreateSearchableMovie(100);
		var matches = SearchByButton(movie, c => c.A);
		Assert.Equal(50, matches.Count); // Every even frame
		Assert.All(matches, idx => Assert.True(idx % 2 == 0));
	}

	[Fact]
	public void SearchByButton_B_FindsEveryThirdFrame() {
		var movie = CreateSearchableMovie(100);
		var matches = SearchByButton(movie, c => c.B);
		Assert.Equal(34, matches.Count); // 0, 3, 6, ..., 99
	}

	[Fact]
	public void SearchByButton_Up_FindsEveryTenthFrame() {
		var movie = CreateSearchableMovie(100);
		var matches = SearchByButton(movie, c => c.Up);
		Assert.Equal(10, matches.Count);
		Assert.All(matches, idx => Assert.True(idx % 10 == 0));
	}

	[Fact]
	public void SearchByButton_Start_FindsEvery50thFrame() {
		var movie = CreateSearchableMovie(200);
		var matches = SearchByButton(movie, c => c.Start);
		Assert.Equal(4, matches.Count); // 0, 50, 100, 150
	}

	[Fact]
	public void SearchByButton_AAndB_FindsCombination() {
		var movie = CreateSearchableMovie(100);
		var matches = SearchByButton(movie, c => c.A && c.B);
		// A=even, B=div3 → both: div6 (0, 6, 12, ..., 96)
		Assert.Equal(17, matches.Count);
	}

	[Fact]
	public void SearchByButton_NonePressed_FindsNoInput() {
		var movie = CreateSearchableMovie(100);
		var matches = SearchByButton(movie, c => !c.A && !c.B && !c.Up && !c.Start);
		Assert.True(matches.Count > 0);
		// Verify none of the searched buttons are pressed
		foreach (int idx in matches) {
			var ctrl = movie.InputFrames[idx].Controllers[0];
			Assert.False(ctrl.A);
			Assert.False(ctrl.B);
			Assert.False(ctrl.Up);
			Assert.False(ctrl.Start);
		}
	}

	#endregion

	#region Comment Search

	[Fact]
	public void SearchByComment_Checkpoint_FindsCommentedFrames() {
		var movie = CreateSearchableMovie(100);
		var matches = SearchByComment(movie, "Checkpoint");
		Assert.Equal(4, matches.Count); // 0, 25, 50, 75
	}

	[Fact]
	public void SearchByComment_CaseInsensitive() {
		var movie = CreateSearchableMovie(100);
		var upper = SearchByComment(movie, "CHECKPOINT");
		var lower = SearchByComment(movie, "checkpoint");
		Assert.Equal(upper.Count, lower.Count);
	}

	[Fact]
	public void SearchByComment_NoMatch_ReturnsEmpty() {
		var movie = CreateSearchableMovie(100);
		var matches = SearchByComment(movie, "nonexistent");
		Assert.Empty(matches);
	}

	[Fact]
	public void SearchByComment_PartialMatch_FindsSubstring() {
		var movie = CreateSearchableMovie(200);
		var matches = SearchByComment(movie, "100");
		Assert.Contains(100, matches);
	}

	[Fact]
	public void SearchByComment_EmptyMovie_ReturnsEmpty() {
		var movie = new MovieData { SystemType = SystemType.Nes };
		var matches = SearchByComment(movie, "anything");
		Assert.Empty(matches);
	}

	#endregion

	#region Lag Frame Search

	[Fact]
	public void SearchLagFrames_FindsEvery7thFrame() {
		var movie = CreateSearchableMovie(100);
		var matches = SearchLagFrames(movie);
		Assert.Equal(15, matches.Count); // 0, 7, 14, ..., 98
		Assert.All(matches, idx => Assert.True(movie.InputFrames[idx].IsLagFrame));
	}

	[Fact]
	public void SearchLagFrames_NoLagFrames_ReturnsEmpty() {
		var movie = new MovieData { SystemType = SystemType.Nes };
		for (int i = 0; i < 50; i++) {
			movie.InputFrames.Add(new InputFrame(i) { IsLagFrame = false });
		}
		var matches = SearchLagFrames(movie);
		Assert.Empty(matches);
	}

	#endregion

	#region Frame Range Search

	[Fact]
	public void SearchByRange_ReturnsFramesInRange() {
		var movie = CreateSearchableMovie(100);
		var matches = SearchByFrameRange(movie, 10, 19);
		Assert.Equal(10, matches.Count);
		Assert.Equal(10, matches[0]);
		Assert.Equal(19, matches[^1]);
	}

	[Fact]
	public void SearchByRange_ClampedToMovieBounds() {
		var movie = CreateSearchableMovie(50);
		var matches = SearchByFrameRange(movie, 40, 100);
		Assert.Equal(10, matches.Count); // 40..49
	}

	[Fact]
	public void SearchByRange_NegativeStart_ClampsToZero() {
		var movie = CreateSearchableMovie(50);
		var matches = SearchByFrameRange(movie, -10, 5);
		Assert.Equal(6, matches.Count); // 0..5
		Assert.Equal(0, matches[0]);
	}

	[Fact]
	public void SearchByRange_SingleFrame_ReturnsSingle() {
		var movie = CreateSearchableMovie(100);
		var matches = SearchByFrameRange(movie, 42, 42);
		Assert.Single(matches);
		Assert.Equal(42, matches[0]);
	}

	#endregion

	#region Navigation Between Matches

	[Fact]
	public void NavigateNext_FromCurrentIndex_FindsNextMatch() {
		var movie = CreateSearchableMovie(100);
		var matches = SearchByButton(movie, c => c.Up); // 0, 10, 20, ...

		// Starting from frame 5, next match should be 10
		int current = 5;
		int next = matches.FirstOrDefault(m => m > current, -1);
		Assert.Equal(10, next);
	}

	[Fact]
	public void NavigatePrevious_FromCurrentIndex_FindsPreviousMatch() {
		var movie = CreateSearchableMovie(100);
		var matches = SearchByButton(movie, c => c.Up); // 0, 10, 20, ...

		// Starting from frame 15, previous match should be 10
		int current = 15;
		int prev = matches.LastOrDefault(m => m < current, -1);
		Assert.Equal(10, prev);
	}

	[Fact]
	public void NavigateNext_AtLastMatch_ReturnsNoMatch() {
		var movie = CreateSearchableMovie(100);
		var matches = SearchByButton(movie, c => c.Up);
		int lastMatch = matches[^1]; // 90

		int next = matches.FirstOrDefault(m => m > lastMatch, -1);
		Assert.Equal(-1, next);
	}

	#endregion
}
