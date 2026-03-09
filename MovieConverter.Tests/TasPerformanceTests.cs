using Xunit;

namespace Nexen.MovieConverter.Tests;

/// <summary>
/// Performance regression tests for TAS/movie operations.
/// Verifies correctness of operations on large movie datasets
/// and ensures optimized code paths produce correct results.
/// </summary>
public class TasPerformanceTests {
	#region TruncateAt Stress Tests

	[Theory]
	[InlineData(10000, 0)]
	[InlineData(10000, 5000)]
	[InlineData(10000, 9999)]
	[InlineData(100000, 50000)]
	public void TruncateAt_LargeMovie_ProducesCorrectCount(int totalFrames, int truncateAt) {
		var movie = CreateMovie(totalFrames);

		movie.TruncateAt(truncateAt);

		Assert.Equal(truncateAt + 1, movie.TotalFrames);
	}

	[Fact]
	public void TruncateAt_NegativeIndex_ClearsAllFrames() {
		var movie = CreateMovie(1000);

		movie.TruncateAt(-1);

		Assert.Equal(0, movie.TotalFrames);
	}

	[Fact]
	public void TruncateAt_BeyondEnd_KeepsAllFrames() {
		var movie = CreateMovie(100);

		movie.TruncateAt(200);

		Assert.Equal(100, movie.TotalFrames);
	}

	[Fact]
	public void TruncateAt_PreservesFrameData() {
		var movie = CreateMovieWithInput(100);

		movie.TruncateAt(49);

		Assert.Equal(50, movie.TotalFrames);
		// Verify frame 0 still has correct input
		Assert.True(movie.InputFrames[0].Controllers[0].A);
		// Verify frame 49 still has correct input
		Assert.Equal(49, movie.InputFrames[49].FrameNumber);
	}

	#endregion

	#region Clone Stress Tests

	[Theory]
	[InlineData(100)]
	[InlineData(1000)]
	[InlineData(10000)]
	public void Clone_LargeMovie_PreservesAllFrames(int frameCount) {
		var movie = CreateMovieWithInput(frameCount);

		var clone = movie.Clone();

		Assert.Equal(movie.TotalFrames, clone.TotalFrames);
		// Spot-check first, middle, last frame
		AssertFrameEqual(movie.InputFrames[0], clone.InputFrames[0]);
		AssertFrameEqual(movie.InputFrames[frameCount / 2], clone.InputFrames[frameCount / 2]);
		AssertFrameEqual(movie.InputFrames[frameCount - 1], clone.InputFrames[frameCount - 1]);
	}

	[Fact]
	public void Clone_IsIndependent_ModifyingCloneDoesNotAffectOriginal() {
		var movie = CreateMovieWithInput(100);

		var clone = movie.Clone();

		// Modify clone
		clone.InputFrames[0].Controllers[0].A = false;
		clone.InputFrames[50].Controllers[0].B = true;
		clone.TruncateAt(50);

		// Original unchanged
		Assert.Equal(100, movie.TotalFrames);
		Assert.True(movie.InputFrames[0].Controllers[0].A);
		Assert.False(movie.InputFrames[50].Controllers[0].B);
	}

	#endregion

	#region CalculateInputCrc32 Tests

	[Fact]
	public void CalculateInputCrc32_Deterministic() {
		var movie = CreateMovieWithInput(1000);

		uint crc1 = movie.CalculateInputCrc32();
		uint crc2 = movie.CalculateInputCrc32();

		Assert.Equal(crc1, crc2);
	}

	[Fact]
	public void CalculateInputCrc32_DifferentInput_DifferentHash() {
		var movie1 = CreateMovieWithInput(100);
		var movie2 = CreateMovieWithInput(100);
		movie2.InputFrames[50].Controllers[0].Start = true;

		uint crc1 = movie1.CalculateInputCrc32();
		uint crc2 = movie2.CalculateInputCrc32();

		Assert.NotEqual(crc1, crc2);
	}

	[Fact]
	public void CalculateInputCrc32_EmptyMovie_ProducesHash() {
		var movie = new MovieData();

		uint crc = movie.CalculateInputCrc32();

		// Should produce a valid hash (not throw)
		Assert.True(crc >= 0);
	}

	#endregion

	#region InputFrame Parsing Round-Trip

	[Fact]
	public void InputFrame_NexenLogLine_RoundTrip() {
		var frame = new InputFrame(42) {
			Controllers = [
				new ControllerInput { A = true, Up = true, Start = true },
				new ControllerInput { B = true, Left = true },
				new ControllerInput(),
				new ControllerInput(),
				new ControllerInput(),
				new ControllerInput(),
				new ControllerInput(),
				new ControllerInput()
			]
		};

		string logLine = frame.ToNexenLogLine(2);
		var parsed = InputFrame.FromNexenLogLine(logLine, 42);

		Assert.Equal(frame.Controllers[0].A, parsed.Controllers[0].A);
		Assert.Equal(frame.Controllers[0].Up, parsed.Controllers[0].Up);
		Assert.Equal(frame.Controllers[0].Start, parsed.Controllers[0].Start);
		Assert.Equal(frame.Controllers[1].B, parsed.Controllers[1].B);
		Assert.Equal(frame.Controllers[1].Left, parsed.Controllers[1].Left);
	}

	[Fact]
	public void InputFrame_NexenLogLine_LargeRoundTrip() {
		// Verify round-trip for many frames
		for (int i = 0; i < 1000; i++) {
			var frame = new InputFrame(i);
			frame.Controllers[0].A = i % 2 == 0;
			frame.Controllers[0].B = i % 3 == 0;
			frame.Controllers[0].Up = i % 5 == 0;
			frame.Controllers[0].Right = i % 7 == 0;

			string logLine = frame.ToNexenLogLine(2);
			var parsed = InputFrame.FromNexenLogLine(logLine, i);

			Assert.Equal(frame.Controllers[0].A, parsed.Controllers[0].A);
			Assert.Equal(frame.Controllers[0].B, parsed.Controllers[0].B);
			Assert.Equal(frame.Controllers[0].Up, parsed.Controllers[0].Up);
			Assert.Equal(frame.Controllers[0].Right, parsed.Controllers[0].Right);
		}
	}

	[Theory]
	[InlineData("")]
	[InlineData("   ")]
	[InlineData("LAG")]
	[InlineData("# Test comment")]
	[InlineData("CMD:RESET")]
	public void InputFrame_SpecialLogLines_ParseWithoutException(string logLine) {
		var frame = InputFrame.FromNexenLogLine(logLine, 0);

		Assert.NotNull(frame);
		Assert.Equal(0, frame.FrameNumber);
	}

	#endregion

	#region ControllerInput Format Tests

	[Fact]
	public void ControllerInput_SmvFormat_RoundTrip() {
		var input = new ControllerInput {
			A = true,
			B = true,
			X = true,
			Y = true,
			L = true,
			R = true,
			Up = true,
			Down = true,
			Left = true,
			Right = true,
			Start = true,
			Select = true
		};

		ushort smv = input.ToSmvFormat();
		var parsed = ControllerInput.FromSmvFormat(smv);

		Assert.Equal(input.A, parsed.A);
		Assert.Equal(input.B, parsed.B);
		Assert.Equal(input.X, parsed.X);
		Assert.Equal(input.Y, parsed.Y);
		Assert.Equal(input.L, parsed.L);
		Assert.Equal(input.R, parsed.R);
		Assert.Equal(input.Up, parsed.Up);
		Assert.Equal(input.Down, parsed.Down);
		Assert.Equal(input.Left, parsed.Left);
		Assert.Equal(input.Right, parsed.Right);
		Assert.Equal(input.Start, parsed.Start);
		Assert.Equal(input.Select, parsed.Select);
	}

	[Fact]
	public void ControllerInput_EmptyInput_AllButtonsFalse() {
		var input = new ControllerInput();

		Assert.False(input.HasInput);
		Assert.False(input.A);
		Assert.False(input.B);
		Assert.False(input.Start);
	}

	[Fact]
	public void ControllerInput_Clone_IsDeepCopy() {
		var input = new ControllerInput { A = true, Up = true };

		var clone = input.Clone();
		clone.A = false;

		Assert.True(input.A);
		Assert.False(clone.A);
	}

	#endregion

	#region Helpers

	private static MovieData CreateMovie(int frameCount) {
		var movie = new MovieData {
			SystemType = SystemType.Snes,
			Region = RegionType.NTSC
		};
		for (int i = 0; i < frameCount; i++) {
			movie.AddFrame(new InputFrame(i));
		}
		return movie;
	}

	private static MovieData CreateMovieWithInput(int frameCount) {
		var movie = new MovieData {
			SystemType = SystemType.Snes,
			Region = RegionType.NTSC,
			Author = "Test"
		};
		for (int i = 0; i < frameCount; i++) {
			var frame = new InputFrame(i);
			frame.Controllers[0].A = i % 2 == 0;
			frame.Controllers[0].B = i % 3 == 0;
			frame.Controllers[0].Up = i % 5 == 0;
			movie.AddFrame(frame);
		}
		return movie;
	}

	private static void AssertFrameEqual(InputFrame expected, InputFrame actual) {
		Assert.Equal(expected.FrameNumber, actual.FrameNumber);
		Assert.Equal(expected.Controllers[0].A, actual.Controllers[0].A);
		Assert.Equal(expected.Controllers[0].B, actual.Controllers[0].B);
		Assert.Equal(expected.Controllers[0].Up, actual.Controllers[0].Up);
	}

	#endregion
}
