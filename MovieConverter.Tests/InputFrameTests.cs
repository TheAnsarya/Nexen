namespace Nexen.MovieConverter.Tests;

/// <summary>
/// Tests for InputFrame class
/// </summary>
public class InputFrameTests {
	[Fact]
	public void Constructor_InitializesControllersArray() {
		var frame = new InputFrame();

		Assert.Equal(InputFrame.MaxPorts, frame.Controllers.Length);
		Assert.All(frame.Controllers, c => Assert.NotNull(c));
	}

	[Fact]
	public void Constructor_WithFrameNumber_SetsFrameNumber() {
		var frame = new InputFrame(42);

		Assert.Equal(42, frame.FrameNumber);
	}

	[Fact]
	public void HasAnyInput_ReturnsFalseForEmptyFrame() {
		var frame = new InputFrame();

		Assert.False(frame.HasAnyInput);
	}

	[Fact]
	public void HasAnyInput_ReturnsTrueWhenControllerHasInput() {
		var frame = new InputFrame();
		frame.Controllers[0].A = true;

		Assert.True(frame.HasAnyInput);
	}

	[Fact]
	public void GetPort_ReturnsCorrectController() {
		var frame = new InputFrame();
		frame.Controllers[1].B = true;

		var port1 = frame.GetPort(1);

		Assert.True(port1.B);
	}

	[Fact]
	public void GetPort_ThrowsForInvalidPort() {
		var frame = new InputFrame();

		Assert.Throws<ArgumentOutOfRangeException>(() => frame.GetPort(-1));
		Assert.Throws<ArgumentOutOfRangeException>(() => frame.GetPort(InputFrame.MaxPorts));
	}

	[Fact]
	public void Clone_CreatesDeepCopy() {
		var frame = new InputFrame(100) {
			Command = FrameCommand.SoftReset,
			IsLagFrame = true,
			Comment = "Test comment"
		};
		frame.Controllers[0].A = true;

		var clone = frame.Clone();

		Assert.Equal(frame.FrameNumber, clone.FrameNumber);
		Assert.Equal(frame.Command, clone.Command);
		Assert.Equal(frame.IsLagFrame, clone.IsLagFrame);
		Assert.Equal(frame.Comment, clone.Comment);
		Assert.Equal(frame.Controllers[0].A, clone.Controllers[0].A);

		// Verify it's a deep copy
		clone.Controllers[0].A = false;
		Assert.NotEqual(frame.Controllers[0].A, clone.Controllers[0].A);
	}

	[Fact]
	public void ToNexenLogLine_FormatsCorrectly() {
		var frame = new InputFrame();
		frame.Controllers[0] = new ControllerInput { A = true, B = true, Up = true };

		string line = frame.ToNexenLogLine(1);

		Assert.Contains("A", line);
		Assert.Contains("B", line);
		Assert.Contains("U", line);
	}

	[Fact]
	public void ToNexenLogLine_IncludesLagMarker() {
		var frame = new InputFrame { IsLagFrame = true };

		string line = frame.ToNexenLogLine(1);

		Assert.Contains("LAG", line);
	}

	[Fact]
	public void ToNexenLogLine_IncludesComment() {
		var frame = new InputFrame { Comment = "Test comment" };

		string line = frame.ToNexenLogLine(1);

		Assert.Contains("# Test comment", line);
	}

	[Fact]
	public void FromNexenLogLine_ParsesCorrectly() {
		// Nexen format is BYsSUDLRAXLR (12 chars)
		// B at 0, A at 8, Up at 4
		string line = "B...U...A...|............|# Test";

		var frame = InputFrame.FromNexenLogLine(line, 5);

		Assert.Equal(5, frame.FrameNumber);
		Assert.True(frame.Controllers[0].A);
		Assert.True(frame.Controllers[0].B);
		Assert.True(frame.Controllers[0].Up);
		Assert.Equal("Test", frame.Comment);
	}

	[Fact]
	public void FromNexenLogLine_ParsesLagFrame() {
		string line = "........|LAG";

		var frame = InputFrame.FromNexenLogLine(line, 0);

		Assert.True(frame.IsLagFrame);
	}

	[Fact]
	public void Equality_ComparesCorrectly() {
		var frame1 = new InputFrame(10);
		frame1.Controllers[0].A = true;

		var frame2 = new InputFrame(10);
		frame2.Controllers[0].A = true;

		var frame3 = new InputFrame(10);
		frame3.Controllers[0].B = true;

		Assert.True(frame1.Equals(frame2));
		Assert.False(frame1.Equals(frame3));
	}
}
