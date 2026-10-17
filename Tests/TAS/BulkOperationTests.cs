using Nexen.MovieConverter;
using Xunit;

namespace Nexen.Tests.TAS;

/// <summary>
/// Tests for bulk list operations (issue #261) and canonical Clone usage (issue #265).
/// Validates RemoveRange/InsertRange/GetRange produce identical results to legacy loops.
/// </summary>
public class BulkOperationTests {
	#region Test Helpers

	private static List<InputFrame> CreateTestFrames(int count) {
		var frames = new List<InputFrame>();
		for (int i = 0; i < count; i++) {
			var frame = new InputFrame(i);
			frame.Controllers[0].A = i % 2 == 0;
			frame.Controllers[0].B = i % 3 == 0;
			frame.Controllers[0].Up = i % 5 == 0;
			frame.IsLagFrame = i % 7 == 0;
			frame.Comment = i % 10 == 0 ? $"Comment at {i}" : null;
			frames.Add(frame);
		}

		return frames;
	}

	private static void AssertFramesEqual(InputFrame expected, InputFrame actual) {
		Assert.Equal(expected.FrameNumber, actual.FrameNumber);
		Assert.Equal(expected.IsLagFrame, actual.IsLagFrame);
		Assert.Equal(expected.Comment, actual.Comment);

		for (int c = 0; c < expected.Controllers.Length && c < actual.Controllers.Length; c++) {
			Assert.Equal(expected.Controllers[c].A, actual.Controllers[c].A);
			Assert.Equal(expected.Controllers[c].B, actual.Controllers[c].B);
			Assert.Equal(expected.Controllers[c].Up, actual.Controllers[c].Up);
		}
	}

	#endregion

	#region InsertRange vs Insert Loop Tests

	[Theory]
	[InlineData(10, 5, 3)]     // Small movie, insert 3 at index 5
	[InlineData(100, 50, 20)]  // Medium movie, insert 20 at index 50
	[InlineData(100, 0, 10)]   // Insert at beginning
	[InlineData(100, 100, 10)] // Insert at end
	public void InsertRange_ProducesIdenticalResult_ToInsertLoop(int movieSize, int insertIndex, int insertCount) {
		// Arrange - two identical frame lists
		var frames1 = CreateTestFrames(movieSize);
		var frames2 = CreateTestFrames(movieSize);
		var toInsert = CreateTestFrames(insertCount);

		// Act - InsertRange (new approach)
		frames1.InsertRange(insertIndex, toInsert);

		// Act - Insert loop (old approach)
		for (int i = 0; i < insertCount; i++) {
			frames2.Insert(insertIndex + i, toInsert[i]);
		}

		// Assert - identical results
		Assert.Equal(frames1.Count, frames2.Count);
		for (int i = 0; i < frames1.Count; i++) {
			AssertFramesEqual(frames1[i], frames2[i]);
		}
	}

	[Fact]
	public void InsertRange_InsertsAtCorrectPosition() {
		var frames = CreateTestFrames(5);
		var toInsert = new List<InputFrame> {
			new(100) { Comment = "Inserted A" },
			new(101) { Comment = "Inserted B" },
		};

		frames.InsertRange(2, toInsert);

		Assert.Equal(7, frames.Count);
		Assert.Equal("Inserted A", frames[2].Comment);
		Assert.Equal("Inserted B", frames[3].Comment);
		Assert.Equal(2, frames[4].FrameNumber); // Original frame at index 2 shifted
	}

	#endregion

	#region RemoveRange vs RemoveAt Loop Tests

	[Theory]
	[InlineData(50, 25, 10)]  // Remove 10 from middle
	[InlineData(100, 0, 50)]  // Remove first half
	[InlineData(100, 50, 50)] // Remove second half
	[InlineData(100, 99, 1)]  // Remove last frame
	public void RemoveRange_ProducesIdenticalResult_ToRemoveAtLoop(int movieSize, int removeIndex, int removeCount) {
		// Arrange
		var frames1 = CreateTestFrames(movieSize);
		var frames2 = CreateTestFrames(movieSize);

		// Act - RemoveRange (new approach)
		frames1.RemoveRange(removeIndex, removeCount);

		// Act - RemoveAt loop (old approach - always removes from same index)
		for (int i = 0; i < removeCount; i++) {
			frames2.RemoveAt(removeIndex);
		}

		// Assert
		Assert.Equal(frames1.Count, frames2.Count);
		for (int i = 0; i < frames1.Count; i++) {
			AssertFramesEqual(frames1[i], frames2[i]);
		}
	}

	[Fact]
	public void RemoveRange_RemovesCorrectFrames() {
		var frames = CreateTestFrames(10);
		int originalFrame5Number = frames[5].FrameNumber;
		int originalFrame7Number = frames[7].FrameNumber;

		// Remove frames at indices 2, 3, 4
		frames.RemoveRange(2, 3);

		Assert.Equal(7, frames.Count);
		Assert.Equal(0, frames[0].FrameNumber);
		Assert.Equal(1, frames[1].FrameNumber);
		Assert.Equal(originalFrame5Number, frames[2].FrameNumber); // Was index 5, now index 2
		Assert.Equal(originalFrame7Number, frames[4].FrameNumber); // Was index 7, now index 4
	}

	#endregion

	#region GetRange vs Skip/Take Tests

	[Theory]
	[InlineData(100, 25, 50)]
	[InlineData(100, 0, 100)]
	[InlineData(100, 99, 1)]
	[InlineData(50, 0, 1)]
	public void GetRange_ProducesIdenticalResult_ToSkipTake(int movieSize, int index, int count) {
		var frames = CreateTestFrames(movieSize);

		// GetRange (new approach)
		var result1 = frames.GetRange(index, count);

		// Skip/Take (old approach)
		var result2 = frames.Skip(index).Take(count).ToList();

		Assert.Equal(result1.Count, result2.Count);
		for (int i = 0; i < result1.Count; i++) {
			Assert.Same(result1[i], result2[i]); // Should be exact same references
		}
	}

	#endregion

	#region Fork Truncation Tests

	[Theory]
	[InlineData(100, 50)]
	[InlineData(100, 0)]
	[InlineData(100, 99)]
	[InlineData(1000, 500)]
	public void ForkTruncation_RemoveRange_EquivalentToWhileLoop(int movieSize, int truncateAt) {
		var frames1 = CreateTestFrames(movieSize);
		var frames2 = CreateTestFrames(movieSize);

		// RemoveRange approach (new)
		if (truncateAt < frames1.Count) {
			frames1.RemoveRange(truncateAt, frames1.Count - truncateAt);
		}

		// While loop approach (old)
		while (frames2.Count > truncateAt) {
			frames2.RemoveAt(frames2.Count - 1);
		}

		Assert.Equal(frames1.Count, frames2.Count);
		Assert.Equal(truncateAt, frames1.Count);
		for (int i = 0; i < frames1.Count; i++) {
			AssertFramesEqual(frames1[i], frames2[i]);
		}
	}

	#endregion

	#region InputFrame.Clone Tests (Issue #265)

	[Fact]
	public void InputFrame_Clone_CopiesAllFields() {
		var original = new InputFrame(42) {
			IsLagFrame = true,
			Comment = "Test comment"
		};

		original.Controllers[0].A = true;
		original.Controllers[0].B = true;
		original.Controllers[0].Up = true;
		original.Controllers[0].Start = true;
		original.Controllers[1].X = true;
		original.Controllers[1].Y = true;

		var clone = original.Clone();

		Assert.Equal(42, clone.FrameNumber);
		Assert.True(clone.IsLagFrame);
		Assert.Equal("Test comment", clone.Comment);
		Assert.True(clone.Controllers[0].A);
		Assert.True(clone.Controllers[0].B);
		Assert.True(clone.Controllers[0].Up);
		Assert.True(clone.Controllers[0].Start);
		Assert.True(clone.Controllers[1].X);
		Assert.True(clone.Controllers[1].Y);
	}

	[Fact]
	public void InputFrame_Clone_IsDeepCopy() {
		var original = new InputFrame(0);
		original.Controllers[0].A = true;

		var clone = original.Clone();

		// Modify clone should not affect original
		clone.Controllers[0].A = false;
		clone.IsLagFrame = true;
		clone.Comment = "Changed";

		Assert.True(original.Controllers[0].A);
		Assert.False(original.IsLagFrame);
		Assert.Null(original.Comment);
	}

	[Fact]
	public void InputFrame_Clone_CopiesAllControllerFields() {
		// Comprehensive test that Clone doesn't miss any ControllerInput fields
		var original = new InputFrame(0);
		var ctrl = original.Controllers[0];

		ctrl.A = true;
		ctrl.B = true;
		ctrl.X = true;
		ctrl.Y = true;
		ctrl.L = true;
		ctrl.R = true;
		ctrl.Up = true;
		ctrl.Down = true;
		ctrl.Left = true;
		ctrl.Right = true;
		ctrl.Start = true;
		ctrl.Select = true;

		var clone = original.Clone();
		var cloneCtrl = clone.Controllers[0];

		Assert.True(cloneCtrl.A);
		Assert.True(cloneCtrl.B);
		Assert.True(cloneCtrl.X);
		Assert.True(cloneCtrl.Y);
		Assert.True(cloneCtrl.L);
		Assert.True(cloneCtrl.R);
		Assert.True(cloneCtrl.Up);
		Assert.True(cloneCtrl.Down);
		Assert.True(cloneCtrl.Left);
		Assert.True(cloneCtrl.Right);
		Assert.True(cloneCtrl.Start);
		Assert.True(cloneCtrl.Select);
	}

	[Fact]
	public void Select_Clone_ProducesIndependentList() {
		// Tests the pattern used in paste/copy: _clipboard.Select(f => f.Clone()).ToList()
		var original = CreateTestFrames(5);
		var cloned = original.Select(f => f.Clone()).ToList();

		Assert.Equal(original.Count, cloned.Count);

		// All frames should have equal data
		for (int i = 0; i < original.Count; i++) {
			AssertFramesEqual(original[i], cloned[i]);
		}

		// But different references (deep copy)
		for (int i = 0; i < original.Count; i++) {
			Assert.NotSame(original[i], cloned[i]);
			Assert.NotSame(original[i].Controllers[0], cloned[i].Controllers[0]);
		}

		// Modifying clone doesn't affect original
		cloned[0].Controllers[0].A = !cloned[0].Controllers[0].A;
		Assert.NotEqual(original[0].Controllers[0].A, cloned[0].Controllers[0].A);
	}

	#endregion
}
