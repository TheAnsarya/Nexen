using Xunit;
using System;
using System.Collections.Generic;
using System.Reflection;
using Nexen.MovieConverter;
using Nexen.TAS;

namespace Nexen.Tests.TAS;

/// <summary>
/// Unit tests for InputRecorder functionality.
/// Tests truncation, recording modes, and input manipulation.
/// </summary>
public class InputRecorderTests {
	#region Test Helpers

	private static MovieData CreateTestMovie(int frameCount) {
		var movie = new MovieData {
			Author = "Test",
			GameName = "Test Game",
			SystemType = SystemType.Nes,
			Region = RegionType.NTSC
		};

		for (int i = 0; i < frameCount; i++) {
			var frame = new InputFrame(i) {
				Controllers = new ControllerInput[] { new ControllerInput { A = i % 3 == 0 } }
			};
			movie.InputFrames.Add(frame);
		}

		return movie;
	}

	/// <summary>
	/// Sets the internal recording state via reflection to avoid EmuApi native interop calls.
	/// </summary>
	private static void SetRecordingState(InputRecorder recorder, RecordingMode mode, int insertPosition = -1) {
		var type = typeof(InputRecorder);
		type.GetField("_isRecording", BindingFlags.NonPublic | BindingFlags.Instance)!.SetValue(recorder, true);
		type.GetField("_mode", BindingFlags.NonPublic | BindingFlags.Instance)!.SetValue(recorder, mode);

		var movie = (MovieData)type.GetProperty("Movie")!.GetValue(recorder)!;
		int pos = insertPosition >= 0 ? insertPosition : movie.InputFrames.Count;
		type.GetField("_insertPosition", BindingFlags.NonPublic | BindingFlags.Instance)!.SetValue(recorder, pos);
		int startFrame = mode == RecordingMode.Append ? movie.InputFrames.Count : pos;
		type.GetField("_recordStartFrame", BindingFlags.NonPublic | BindingFlags.Instance)!.SetValue(recorder, startFrame);
		type.GetProperty("FramesRecorded")!.SetValue(recorder, 0);
	}

	#endregion

	#region Truncation Tests

	[Fact]
	public void Truncate_WithRemoveRange_RemovesCorrectFrames() {
		// Arrange
		var movie = CreateTestMovie(100);
		int truncateAt = 50;
		int expectedCount = truncateAt;

		// Act - this mimics the fix we made (RemoveRange instead of loop)
		int removeCount = movie.InputFrames.Count - truncateAt;
		movie.InputFrames.RemoveRange(truncateAt, removeCount);

		// Assert
		Assert.Equal(expectedCount, movie.InputFrames.Count);
		Assert.Equal(truncateAt - 1, movie.InputFrames[^1].FrameNumber);
	}

	[Fact]
	public void Truncate_AtBeginning_LeavesEmptyMovie() {
		// Arrange
		var movie = CreateTestMovie(100);

		// Act
		movie.InputFrames.RemoveRange(0, movie.InputFrames.Count);

		// Assert
		Assert.Empty(movie.InputFrames);
	}

	[Fact]
	public void Truncate_AtEnd_NoChange() {
		// Arrange
		var movie = CreateTestMovie(100);
		int originalCount = movie.InputFrames.Count;

		// Act - truncate at end (remove 0 frames)
		int removeCount = movie.InputFrames.Count - movie.InputFrames.Count;
		if (removeCount > 0) {
			movie.InputFrames.RemoveRange(movie.InputFrames.Count, removeCount);
		}

		// Assert
		Assert.Equal(originalCount, movie.InputFrames.Count);
	}

	[Fact]
	public void Truncate_PreservesFrameData() {
		// Arrange
		var movie = CreateTestMovie(10);
		movie.InputFrames[3].Controllers[0].A = true;
		movie.InputFrames[3].Controllers[0].B = true;

		// Act - truncate at frame 5
		movie.InputFrames.RemoveRange(5, movie.InputFrames.Count - 5);

		// Assert - frame 3 data preserved
		Assert.True(movie.InputFrames[3].Controllers[0].A);
		Assert.True(movie.InputFrames[3].Controllers[0].B);
	}

	[Theory]
	[InlineData(10, 5)]
	[InlineData(100, 50)]
	[InlineData(1000, 500)]
	[InlineData(10000, 5000)]
	public void Truncate_VariousSizes_WorksCorrectly(int movieSize, int truncateAt) {
		// Arrange
		var movie = CreateTestMovie(movieSize);

		// Act
		int removeCount = movie.InputFrames.Count - truncateAt;
		movie.InputFrames.RemoveRange(truncateAt, removeCount);

		// Assert
		Assert.Equal(truncateAt, movie.InputFrames.Count);
	}

	#endregion

	#region RemoveRange vs RemoveAt Comparison Tests

	[Fact]
	public void RemoveRange_ProducesIdenticalResult_ToRemoveAtLoop() {
		// Arrange - two identical movies
		var movie1 = CreateTestMovie(50);
		var movie2 = CreateTestMovie(50);
		int removeFrom = 25;

		// Act - RemoveRange approach (new)
		int removeCount = movie1.InputFrames.Count - removeFrom;
		movie1.InputFrames.RemoveRange(removeFrom, removeCount);

		// Act - RemoveAt loop approach (old - O(n²))
		while (movie2.InputFrames.Count > removeFrom) {
			movie2.InputFrames.RemoveAt(removeFrom);
		}

		// Assert - identical results
		Assert.Equal(movie1.InputFrames.Count, movie2.InputFrames.Count);
		for (int i = 0; i < movie1.InputFrames.Count; i++) {
			Assert.Equal(
				movie1.InputFrames[i].Controllers[0].A,
				movie2.InputFrames[i].Controllers[0].A
			);
		}
	}

	#endregion

	#region Branch Tests

	[Fact]
	public void CreateBranch_ClonesAllFrames_WithCorrectCapacity() {
		// Arrange
		var movie = CreateTestMovie(200);
		var greenzone = new GreenzoneManager();
		var recorder = new InputRecorder(greenzone) { Movie = movie, CaptureSavestates = false };

		// Act
		var branch = recorder.CreateBranch("Test Branch");

		// Assert
		Assert.Equal(200, branch.Frames.Count);
		Assert.Equal("Test Branch", branch.Name);

		// Verify deep clone — mutating original shouldn't affect branch
		movie.InputFrames[0].Controllers[0].A = !movie.InputFrames[0].Controllers[0].A;
		Assert.NotEqual(movie.InputFrames[0].Controllers[0].A, branch.Frames[0].Controllers[0].A);
	}

	[Fact]
	public void LoadBranch_ReplacesAllFrames_WithDeepClone() {
		// Arrange
		var movie = CreateTestMovie(50);
		var greenzone = new GreenzoneManager();
		var recorder = new InputRecorder(greenzone) { Movie = movie, CaptureSavestates = false };
		var branch = recorder.CreateBranch("Saved");

		// Modify original
		movie.InputFrames.Clear();
		Assert.Empty(movie.InputFrames);

		// Act
		recorder.LoadBranch(branch);

		// Assert
		Assert.Equal(50, movie.InputFrames.Count);

		// Verify deep clone — mutating loaded frames shouldn't affect branch
		movie.InputFrames[0].Controllers[0].B = true;
		Assert.False(branch.Frames[0].Controllers[0].B);
	}

	#endregion

	#region Recording Mode Tests

	[Fact]
	public void RecordFrame_AppendMode_AppendsToEnd() {
		var movie = CreateTestMovie(5);
		var greenzone = new GreenzoneManager();
		var recorder = new InputRecorder(greenzone) { Movie = movie, CaptureSavestates = false };

		SetRecordingState(recorder, RecordingMode.Append);

		var input = new[] { new ControllerInput { A = true, B = true } };
		recorder.RecordFrame(input);
		recorder.RecordFrame(input);

		Assert.Equal(7, movie.InputFrames.Count);
		Assert.True(movie.InputFrames[5].Controllers[0].A);
		Assert.True(movie.InputFrames[6].Controllers[0].A);
		Assert.Equal(2, recorder.FramesRecorded);
	}

	[Fact]
	public void RecordFrame_InsertMode_InsertsAtPosition() {
		var movie = CreateTestMovie(5);
		var greenzone = new GreenzoneManager();
		var recorder = new InputRecorder(greenzone) { Movie = movie, CaptureSavestates = false };

		SetRecordingState(recorder, RecordingMode.Insert, insertPosition: 2);

		var input = new[] { new ControllerInput { B = true } };
		recorder.RecordFrame(input);
		recorder.RecordFrame(input);

		Assert.Equal(7, movie.InputFrames.Count);
		Assert.True(movie.InputFrames[2].Controllers[0].B);
		Assert.True(movie.InputFrames[3].Controllers[0].B);
	}

	[Fact]
	public void RecordFrame_OverwriteMode_ReplacesExistingFrames() {
		var movie = CreateTestMovie(10);
		var greenzone = new GreenzoneManager();
		var recorder = new InputRecorder(greenzone) { Movie = movie, CaptureSavestates = false };

		SetRecordingState(recorder, RecordingMode.Overwrite, insertPosition: 3);

		var input = new[] { new ControllerInput { Start = true } };
		recorder.RecordFrame(input);
		recorder.RecordFrame(input);

		Assert.Equal(10, movie.InputFrames.Count);
		Assert.True(movie.InputFrames[3].Controllers[0].Start);
		Assert.True(movie.InputFrames[4].Controllers[0].Start);
	}

	[Fact]
	public void RecordFrame_OverwriteMode_AppendsBeyondEnd() {
		var movie = CreateTestMovie(5);
		var greenzone = new GreenzoneManager();
		var recorder = new InputRecorder(greenzone) { Movie = movie, CaptureSavestates = false };

		SetRecordingState(recorder, RecordingMode.Overwrite, insertPosition: 4);

		var input = new[] { new ControllerInput { A = true } };
		recorder.RecordFrame(input);
		recorder.RecordFrame(input);

		Assert.Equal(6, movie.InputFrames.Count);
	}

	[Fact]
	public void RecordFrame_NotRecording_IsNoOp() {
		var movie = CreateTestMovie(5);
		var greenzone = new GreenzoneManager();
		var recorder = new InputRecorder(greenzone) { Movie = movie, CaptureSavestates = false };

		var input = new[] { new ControllerInput { A = true } };
		recorder.RecordFrame(input);

		Assert.Equal(5, movie.InputFrames.Count);
		Assert.Equal(0, recorder.FramesRecorded);
	}

	[Fact]
	public void Recording_MultipleAppendFrames_PreservesRecordedCount() {
		var movie = CreateTestMovie(5);
		var greenzone = new GreenzoneManager();
		var recorder = new InputRecorder(greenzone) { Movie = movie, CaptureSavestates = false };

		SetRecordingState(recorder, RecordingMode.Append);
		recorder.RecordFrame([new ControllerInput()]);
		recorder.RecordFrame([new ControllerInput()]);
		Assert.Equal(2, recorder.FramesRecorded);
	}

	[Fact]
	public void RecordFrame_InsertMode_ShiftsExistingFrames() {
		var movie = CreateTestMovie(5);
		// Save the original frame at index 2
		bool originalA = movie.InputFrames[2].Controllers[0].A;

		var greenzone = new GreenzoneManager();
		var recorder = new InputRecorder(greenzone) { Movie = movie, CaptureSavestates = false };

		SetRecordingState(recorder, RecordingMode.Insert, insertPosition: 2);
		recorder.RecordFrame([new ControllerInput { L = true }]);

		Assert.Equal(6, movie.InputFrames.Count);
		Assert.True(movie.InputFrames[2].Controllers[0].L);
		// Original frame 2 should now be at index 3
		Assert.Equal(originalA, movie.InputFrames[3].Controllers[0].A);
	}

	[Fact]
	public void RecordFrame_LagFrame_IsPreserved() {
		var movie = CreateTestMovie(5);
		var greenzone = new GreenzoneManager();
		var recorder = new InputRecorder(greenzone) { Movie = movie, CaptureSavestates = false };

		SetRecordingState(recorder, RecordingMode.Append);
		recorder.RecordFrame([new ControllerInput()], isLagFrame: true);

		Assert.True(movie.InputFrames[^1].IsLagFrame);
	}

	#endregion

	#region RerecordCount Initialization Tests

	[Fact]
	public void RerecordCount_InitializesFromLoadedMovie() {
		// Arrange — movie has an existing rerecord count
		var movie = CreateTestMovie(10);
		movie.RerecordCount = 42;

		var greenzone = new GreenzoneManager();
		var recorder = new InputRecorder(greenzone);

		// Act — set the movie
		recorder.Movie = movie;

		// Assert — rerecord count is initialized from the movie
		Assert.Equal(42, recorder.RerecordCount);
	}

	[Fact]
	public void RerecordCount_ZeroForNewMovie() {
		// Arrange — fresh movie with no rerecord count
		var movie = CreateTestMovie(10);
		movie.RerecordCount = 0;

		var greenzone = new GreenzoneManager();
		var recorder = new InputRecorder(greenzone);

		// Act
		recorder.Movie = movie;

		// Assert
		Assert.Equal(0, recorder.RerecordCount);
	}

	[Fact]
	public void RerecordCount_ClampsLargeValues() {
		// Arrange — movie with a very large rerecord count
		var movie = CreateTestMovie(10);
		movie.RerecordCount = (ulong)int.MaxValue + 100;

		var greenzone = new GreenzoneManager();
		var recorder = new InputRecorder(greenzone);

		// Act
		recorder.Movie = movie;

		// Assert — clamped to int.MaxValue
		Assert.Equal(int.MaxValue, recorder.RerecordCount);
	}

	[Fact]
	public void RerecordCount_UpdatesWhenMovieChanges() {
		// Arrange
		var movie1 = CreateTestMovie(5);
		movie1.RerecordCount = 10;

		var movie2 = CreateTestMovie(5);
		movie2.RerecordCount = 99;

		var greenzone = new GreenzoneManager();
		var recorder = new InputRecorder(greenzone);

		// Act — set first movie
		recorder.Movie = movie1;
		Assert.Equal(10, recorder.RerecordCount);

		// Act — switch to second movie
		recorder.Movie = movie2;
		Assert.Equal(99, recorder.RerecordCount);
	}

	#endregion
}
