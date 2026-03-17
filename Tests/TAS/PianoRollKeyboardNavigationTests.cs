using Nexen.Controls;
using Xunit;

namespace Nexen.Tests.TAS;

public class PianoRollKeyboardNavigationTests {
	[Fact]
	public void ComputeNextButtonLane_Forward_WrapsAtEnd() {
		int next = PianoRollControl.ComputeNextButtonLane(3, 4, reverse: false);
		Assert.Equal(0, next);
	}

	[Fact]
	public void ComputeNextButtonLane_Reverse_WrapsAtStart() {
		int next = PianoRollControl.ComputeNextButtonLane(0, 4, reverse: true);
		Assert.Equal(3, next);
	}

	[Fact]
	public void ComputeNextButtonLane_EmptyLaneCount_ReturnsZero() {
		int next = PianoRollControl.ComputeNextButtonLane(5, 0, reverse: false);
		Assert.Equal(0, next);
	}

	[Fact]
	public void ComputeFrameSelectionAfterMove_RightMove_AdvancesCursor() {
		var result = PianoRollControl.ComputeFrameSelectionAfterMove(10, 10, 10, 100, 1, extendSelection: false);
		Assert.Equal((11, 11, 11), result);
	}

	[Fact]
	public void ComputeFrameSelectionAfterMove_LeftMove_ClampsAtZero() {
		var result = PianoRollControl.ComputeFrameSelectionAfterMove(0, 0, 0, 100, -1, extendSelection: false);
		Assert.Equal((0, 0, 0), result);
	}

	[Fact]
	public void ComputeFrameSelectionAfterMove_EndJump_GoesToLastFrame() {
		var result = PianoRollControl.ComputeFrameSelectionAfterMove(25, 25, 25, 100, int.MaxValue, extendSelection: false);
		Assert.Equal((99, 99, 99), result);
	}

	[Fact]
	public void ComputeFrameSelectionAfterMove_HomeJump_GoesToFirstFrame() {
		var result = PianoRollControl.ComputeFrameSelectionAfterMove(25, 25, 25, 100, int.MinValue, extendSelection: false);
		Assert.Equal((0, 0, 0), result);
	}

	[Fact]
	public void ComputeFrameSelectionAfterMove_ShiftMove_ExtendsSelectionEnd() {
		var result = PianoRollControl.ComputeFrameSelectionAfterMove(10, 8, 10, 100, 3, extendSelection: true);
		Assert.Equal((8, 13, 13), result);
	}

	[Fact]
	public void ComputeFrameSelectionAfterMove_NoFrames_ReturnsInvalidSelection() {
		var result = PianoRollControl.ComputeFrameSelectionAfterMove(0, 0, 0, 0, 1, extendSelection: false);
		Assert.Equal((-1, -1, -1), result);
	}
}
