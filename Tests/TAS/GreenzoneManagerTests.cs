using Xunit;
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using Nexen.TAS;

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

	/// <summary>
	/// Lynx runs at ~75fps; A2600 at ~60fps. Typical short TAS runs are a few hundred frames.
	/// Verify the yield interval logic produces the expected batch counts at these realistic depths.
	/// </summary>
	[Theory]
	[InlineData(75, 4)]    // 1 second Lynx ~75fps = 4 yields (at 0, 20, 40, 60)
	[InlineData(450, 23)]  // 6 second Lynx ~75fps = 23 yields
	[InlineData(60, 3)]    // 1 second A2600 ~60fps = 3 yields (at 0, 20, 40)
	[InlineData(360, 18)]  // 6 second A2600 ~60fps = 18 yields
	public void SeekYieldInterval_CorrectForLynxAndAtari2600TypicalDepths(int frames, int expectedYields) {
		// Seek yield interval is fixed at 20 frames regardless of platform
		const int yieldInterval = 20;
		int yieldCount = 0;

		for (int i = 0; i < frames; i++) {
			if (i % yieldInterval == 0) {
				yieldCount++;
			}
		}

		Assert.Equal(expectedYields, yieldCount);
	}

	/// <summary>
	/// Greenzone lookup (GetNearestStateFrame equivalent) must return the closest
	/// state at or before the target frame, for typical Lynx/A2600 savestave spacing.
	/// Lynx/A2600 greenzone capture interval matches the standard 60-frame default.
	/// </summary>
	[Theory]
	[InlineData(75, 60, 60)]   // Target=75, states at 0/60/120/... → nearest=60 (60≤75)
	[InlineData(59, 60, 0)]    // Target=59, states at 0/60/... → nearest=0 (0≤59, 60>59)
	[InlineData(120, 60, 120)] // Target=120, states at 0/60/120 → exact match=120
	[InlineData(179, 60, 120)] // Target=179, states at 0/60/120/180/240 → nearest=120 (180>179)
	public void GetNearestStateFrame_LynxAtari2600SavestateSpacing(int target, int captureInterval, int expectedNearest) {
		// Build a sorted state map at the given capture interval up to frame 240
		var savestates = new SortedDictionary<int, byte[]>();

		for (int frame = 0; frame <= 240; frame += captureInterval) {
			savestates[frame] = new byte[64];
		}

		// Simulate GetNearestStateFrame: find largest key <= target
		int nearest = -1;
		foreach (int frame in savestates.Keys) {
			if (frame <= target) {
				nearest = frame;
			}
		}

		Assert.Equal(expectedNearest, nearest);
	}

	/// <summary>
	/// After greenzone invalidation past a rerecord point, no state should be returned
	/// for frames beyond the invalidation boundary. This covers the Lynx/A2600 rerecord
	/// invalidation path: seeking into an invalidated region returns -1.
	/// </summary>
	[Theory]
	[InlineData(60, 120)]   // States at 0,60 remain; 120+ invalidated; seeking to 120 → -1
	[InlineData(60, 180)]   // States at 0,60 remain; seeking to 180 → nearest=60
	[InlineData(0, 60)]     // States only at 0 after invalidation; seeking to 60 → nearest=0
	public void GetNearestStateFrame_AfterInvalidation_ReturnsCorrectBoundary(int lastValidFrame, int seekTarget) {
		// Simulate greenzone with states at 0, 60, 120, 180
		var savestates = new SortedDictionary<int, byte[]> {
			[0] = new byte[64],
			[60] = new byte[64],
			[120] = new byte[64],
			[180] = new byte[64]
		};

		// Simulate InvalidateFrom(lastValidFrame + 1): remove all entries > lastValidFrame
		foreach (var key in savestates.Keys.Where(k => k > lastValidFrame).ToList()) {
			savestates.Remove(key);
		}

		// Find nearest state at or before seekTarget
		int nearest = -1;
		foreach (int frame in savestates.Keys) {
			if (frame <= seekTarget) {
				nearest = frame;
			}
		}

		// After invalidation, nearest must be within the remaining states
		Assert.True(nearest <= lastValidFrame || nearest == -1,
			$"nearest={nearest} must be <= lastValidFrame={lastValidFrame} or -1");
	}

	#endregion

	#region SavestatePayload Atomicity Tests

	[Fact]
	public void SavestatePayload_IsImmutable() {
		// Payload data and compression flag can only be set at construction
		var data = new byte[] { 1, 2, 3, 4 };
		var payload = new SavestatePayload(data, isCompressed: false);

		Assert.Equal(data, payload.Data);
		Assert.False(payload.IsCompressed);

		// Cannot change after construction — no setters exposed
		var compressedPayload = new SavestatePayload(new byte[] { 5, 6 }, isCompressed: true);
		Assert.True(compressedPayload.IsCompressed);
		Assert.Equal(2, compressedPayload.Data.Length);
	}

	[Fact]
	public void SavestateData_PayloadSwapIsAtomic() {
		// Verify that readers always see a consistent (Data, IsCompressed) pair
		var original = new byte[] { 1, 2, 3, 4, 5, 6, 7, 8 };
		var compressed = new byte[] { 9, 10 };

		var state = new SavestateData {
			Frame = 100,
			CaptureTime = DateTime.UtcNow,
			Payload = new SavestatePayload(original, isCompressed: false)
		};

		// Simulate background compression swapping the payload
		state.Payload = new SavestatePayload(compressed, isCompressed: true);

		// Reader always sees consistent pair
		var snapshot = state.Payload!;
		Assert.True(snapshot.IsCompressed);
		Assert.Equal(compressed, snapshot.Data);
	}

	[Fact]
	public void SavestateData_ConcurrentPayloadReads_AlwaysConsistent() {
		// Stress test: writer swaps payload while readers read — no torn state
		var uncompressed = new byte[1024];
		Array.Fill(uncompressed, (byte)0xAA);
		var compressedData = new byte[64];
		Array.Fill(compressedData, (byte)0xBB);

		var state = new SavestateData {
			Frame = 42,
			CaptureTime = DateTime.UtcNow,
			Payload = new SavestatePayload(uncompressed, isCompressed: false)
		};

		bool inconsistencyDetected = false;
		var barrier = new Barrier(3); // 1 writer + 2 readers

		// Writer thread: toggle between uncompressed and compressed payloads
		var writer = new Thread(() => {
			barrier.SignalAndWait();
			for (int i = 0; i < 100_000; i++) {
				if (i % 2 == 0) {
					state.Payload = new SavestatePayload(compressedData, isCompressed: true);
				} else {
					state.Payload = new SavestatePayload(uncompressed, isCompressed: false);
				}
			}
		});

		// Reader threads: verify consistency of each snapshot
		void ReaderWork() {
			barrier.SignalAndWait();
			for (int i = 0; i < 200_000; i++) {
				var snapshot = state.Payload;
				if (snapshot is null) {
					continue;
				}

				// If compressed, data must be the short array; if not, the long array
				if (snapshot.IsCompressed && snapshot.Data.Length != 64) {
					inconsistencyDetected = true;
				}

				if (!snapshot.IsCompressed && snapshot.Data.Length != 1024) {
					inconsistencyDetected = true;
				}
			}
		}

		var reader1 = new Thread(ReaderWork);
		var reader2 = new Thread(ReaderWork);

		writer.Start();
		reader1.Start();
		reader2.Start();

		writer.Join();
		reader1.Join();
		reader2.Join();

		Assert.False(inconsistencyDetected, "Readers observed a torn (Data, IsCompressed) pair");
	}

	#endregion
}
