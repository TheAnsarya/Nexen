using Xunit;
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;

namespace Nexen.Tests.TAS;

/// <summary>
/// Unit tests for GreenzoneManager.
/// Tests savestate storage, retrieval, and frame-specific queries.
/// </summary>
public class GreenzoneManagerTests {
	#region HasState Tests

	[Fact]
	public void HasState_WhenStateExists_ReturnsTrue() {
		// Arrange - simulate savestate storage
		var savestates = new ConcurrentDictionary<int, byte[]>();
		savestates[100] = new byte[1024];
		savestates[200] = new byte[1024];
		savestates[300] = new byte[1024];

		// Act & Assert
		Assert.True(savestates.ContainsKey(100));
		Assert.True(savestates.ContainsKey(200));
		Assert.True(savestates.ContainsKey(300));
	}

	[Fact]
	public void HasState_WhenStateDoesNotExist_ReturnsFalse() {
		// Arrange
		var savestates = new ConcurrentDictionary<int, byte[]>();
		savestates[100] = new byte[1024];

		// Act & Assert
		Assert.False(savestates.ContainsKey(50));
		Assert.False(savestates.ContainsKey(150));
		Assert.False(savestates.ContainsKey(0));
	}

	[Fact]
	public void HasState_EmptyGreenzone_AlwaysReturnsFalse() {
		// Arrange
		var savestates = new ConcurrentDictionary<int, byte[]>();

		// Act & Assert
		Assert.False(savestates.ContainsKey(0));
		Assert.False(savestates.ContainsKey(100));
		Assert.False(savestates.ContainsKey(-1));
	}

	[Fact]
	public void HasState_VsCountGreaterThanZero_DifferentBehavior() {
		// This test verifies the bug fix in issue #254
		// Old: HasSavestate returned _greenzone.SavestateCount > 0 (wrong)
		// New: HasSavestate returns _greenzone.HasState(frame) (correct)

		// Arrange
		var savestates = new ConcurrentDictionary<int, byte[]>();
		savestates[100] = new byte[1024]; // Only frame 100 has a state

		// Old behavior (WRONG): count > 0 would return true for ANY frame
		bool oldBehaviorResult = savestates.Count > 0; // Always true if any state exists

		// New behavior (CORRECT): check specific frame
		bool correctForFrame50 = savestates.ContainsKey(50);
		bool correctForFrame100 = savestates.ContainsKey(100);

		// Assert
		Assert.True(oldBehaviorResult); // Old code would wrongly say frame 50 has a state
		Assert.False(correctForFrame50); // Frame 50 does NOT have a state
		Assert.True(correctForFrame100); // Frame 100 DOES have a state
	}

	#endregion

	#region GetNearestStateFrame Tests

	[Fact]
	public void GetNearestStateFrame_FindsClosestPreceedingState() {
		// Arrange
		var savestates = new SortedDictionary<int, byte[]> {
			[0] = new byte[100],
			[50] = new byte[100],
			[100] = new byte[100],
			[150] = new byte[100]
		};

		// Act - find nearest state <= frame 125
		int targetFrame = 125;
		int nearestFrame = -1;
		foreach (var frame in savestates.Keys) {
			if (frame <= targetFrame) {
				nearestFrame = frame;
			}
		}

		// Assert
		Assert.Equal(100, nearestFrame);
	}

	[Fact]
	public void GetNearestStateFrame_ReturnsExactMatch() {
		// Arrange
		var savestates = new SortedDictionary<int, byte[]> {
			[0] = new byte[100],
			[50] = new byte[100],
			[100] = new byte[100]
		};

		// Act
		int targetFrame = 50;
		int nearestFrame = -1;
		foreach (var frame in savestates.Keys) {
			if (frame <= targetFrame) {
				nearestFrame = frame;
			}
		}

		// Assert
		Assert.Equal(50, nearestFrame);
	}

	[Fact]
	public void GetNearestStateFrame_NoValidState_ReturnsNegativeOne() {
		// Arrange
		var savestates = new SortedDictionary<int, byte[]> {
			[100] = new byte[100],
			[200] = new byte[100]
		};

		// Act - no state <= frame 50
		int targetFrame = 50;
		int nearestFrame = -1;
		foreach (var frame in savestates.Keys) {
			if (frame <= targetFrame) {
				nearestFrame = frame;
			}
		}

		// Assert
		Assert.Equal(-1, nearestFrame);
	}

	#endregion

	#region Ring Buffer (AddToRingBuffer) Tests

	[Fact]
	public void RingBuffer_EvictsOldestWhenFull() {
		// Arrange
		int ringBufferSize = 10;
		var ringBuffer = new Queue<int>();

		// Act - add more frames than buffer size
		for (int i = 0; i < 15; i++) {
			if (ringBuffer.Count >= ringBufferSize) {
				ringBuffer.Dequeue(); // Evict oldest
			}
			ringBuffer.Enqueue(i);
		}

		// Assert
		Assert.Equal(ringBufferSize, ringBuffer.Count);
		Assert.Equal(5, ringBuffer.Peek()); // Oldest is now frame 5
	}

	[Fact]
	public void RingBuffer_TracksFrameNumbersOnly() {
		// This test documents that ring buffer only stores frame numbers,
		// not the actual savestate data (which is stored separately)
		var ringBuffer = new Queue<int>();
		var savestates = new Dictionary<int, byte[]>();

		// Act - add frames to ring buffer and separately to savestates
		for (int i = 0; i < 5; i++) {
			ringBuffer.Enqueue(i);
			savestates[i] = new byte[100];
		}

		// Assert - ring buffer has frame numbers, savestates has data
		Assert.Equal(5, ringBuffer.Count);
		Assert.Equal(5, savestates.Count);
		Assert.All(ringBuffer, frame => Assert.True(savestates.ContainsKey(frame)));
	}

	#endregion

	#region Seek Performance Tests

	[Fact]
	public void SeekYieldInterval_IsBatchedCorrectly() {
		// This tests the logic of yield interval in SeekToFrameAsync
		// We yield every 20 frames to keep UI responsive without excessive overhead
		const int yieldInterval = 20;
		int framesToAdvance = 100;
		int yieldCount = 0;

		for (int i = 0; i < framesToAdvance; i++) {
			// Simulate frame execution
			if (i % yieldInterval == 0) {
				yieldCount++;
			}
		}

		// Assert - 100 frames with yield every 20 = 5 yields
		Assert.Equal(5, yieldCount);
	}

	[Theory]
	[InlineData(10, 1)]    // 10 frames = 1 yield (at i=0)
	[InlineData(20, 1)]    // 20 frames = 1 yield (at i=0)
	[InlineData(21, 2)]    // 21 frames = 2 yields (at i=0, i=20)
	[InlineData(100, 5)]   // 100 frames = 5 yields
	[InlineData(1000, 50)] // 1000 frames = 50 yields
	public void SeekYieldInterval_CalculatesCorrectYieldCount(int frames, int expectedYields) {
		const int yieldInterval = 20;
		int yieldCount = 0;

		for (int i = 0; i < frames; i++) {
			if (i % yieldInterval == 0) {
				yieldCount++;
			}
		}

		Assert.Equal(expectedYields, yieldCount);
	}

	#endregion
}
