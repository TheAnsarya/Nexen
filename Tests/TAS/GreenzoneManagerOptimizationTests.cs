using Nexen.TAS;
using Xunit;

namespace Nexen.Tests.TAS;

/// <summary>
/// Tests for GreenzoneManager SortedSet-based optimizations (issue #262).
/// Validates that frame index, memory tracking, and O(log n) lookups work correctly.
/// </summary>
public class GreenzoneManagerOptimizationTests : IDisposable {
	private readonly GreenzoneManager _greenzone = new();

	public void Dispose() {
		_greenzone.Dispose();
		GC.SuppressFinalize(this);
	}

	#region LatestFrame / EarliestFrame Tests

	[Fact]
	public void LatestFrame_Empty_ReturnsNegativeOne() {
		Assert.Equal(-1, _greenzone.LatestFrame);
	}

	[Fact]
	public void EarliestFrame_Empty_ReturnsNegativeOne() {
		Assert.Equal(-1, _greenzone.EarliestFrame);
	}

	[Fact]
	public void LatestFrame_AfterCapture_ReturnsCorrectFrame() {
		_greenzone.CaptureState(100, new byte[64], forceCapture: true);
		_greenzone.CaptureState(200, new byte[64], forceCapture: true);
		_greenzone.CaptureState(50, new byte[64], forceCapture: true);

		Assert.Equal(200, _greenzone.LatestFrame);
	}

	[Fact]
	public void EarliestFrame_AfterCapture_ReturnsCorrectFrame() {
		_greenzone.CaptureState(100, new byte[64], forceCapture: true);
		_greenzone.CaptureState(200, new byte[64], forceCapture: true);
		_greenzone.CaptureState(50, new byte[64], forceCapture: true);

		Assert.Equal(50, _greenzone.EarliestFrame);
	}

	[Fact]
	public void LatestFrame_AfterInvalidation_UpdatesCorrectly() {
		_greenzone.CaptureState(100, new byte[64], forceCapture: true);
		_greenzone.CaptureState(200, new byte[64], forceCapture: true);
		_greenzone.CaptureState(300, new byte[64], forceCapture: true);

		_greenzone.InvalidateFrom(250);

		Assert.Equal(200, _greenzone.LatestFrame);
	}

	[Fact]
	public void EarliestFrame_AfterInvalidation_RemainsCorrect() {
		_greenzone.CaptureState(100, new byte[64], forceCapture: true);
		_greenzone.CaptureState(200, new byte[64], forceCapture: true);
		_greenzone.CaptureState(300, new byte[64], forceCapture: true);

		_greenzone.InvalidateFrom(250);

		Assert.Equal(100, _greenzone.EarliestFrame);
	}

	[Fact]
	public void LatestFrame_AfterClear_ReturnsNegativeOne() {
		_greenzone.CaptureState(100, new byte[64], forceCapture: true);
		_greenzone.CaptureState(200, new byte[64], forceCapture: true);

		_greenzone.Clear();

		Assert.Equal(-1, _greenzone.LatestFrame);
	}

	#endregion

	#region TotalMemoryUsage Tests

	[Fact]
	public void TotalMemoryUsage_Empty_ReturnsZero() {
		Assert.Equal(0, _greenzone.TotalMemoryUsage);
	}

	[Fact]
	public void TotalMemoryUsage_TracksAdditions() {
		_greenzone.CaptureState(100, new byte[1024], forceCapture: true);
		_greenzone.CaptureState(200, new byte[2048], forceCapture: true);

		Assert.Equal(3072, _greenzone.TotalMemoryUsage);
	}

	[Fact]
	public void TotalMemoryUsage_TracksReplacements() {
		_greenzone.CaptureState(100, new byte[1024], forceCapture: true);
		Assert.Equal(1024, _greenzone.TotalMemoryUsage);

		// Replace with larger data
		_greenzone.CaptureState(100, new byte[4096], forceCapture: true);
		Assert.Equal(4096, _greenzone.TotalMemoryUsage);
	}

	[Fact]
	public void TotalMemoryUsage_AfterInvalidation_Decreases() {
		_greenzone.CaptureState(100, new byte[1024], forceCapture: true);
		_greenzone.CaptureState(200, new byte[2048], forceCapture: true);
		_greenzone.CaptureState(300, new byte[4096], forceCapture: true);

		long beforeInvalidation = _greenzone.TotalMemoryUsage;
		Assert.Equal(7168, beforeInvalidation); // 1024 + 2048 + 4096

		_greenzone.InvalidateFrom(200);

		Assert.Equal(1024, _greenzone.TotalMemoryUsage); // Only frame 100 remains
	}

	[Fact]
	public void TotalMemoryUsage_AfterClear_ReturnsZero() {
		_greenzone.CaptureState(100, new byte[1024], forceCapture: true);
		_greenzone.CaptureState(200, new byte[2048], forceCapture: true);

		_greenzone.Clear();

		Assert.Equal(0, _greenzone.TotalMemoryUsage);
	}

	#endregion

	#region GetNearestStateFrame Tests (SortedSet-based)

	[Fact]
	public void GetNearestStateFrame_Empty_ReturnsNegativeOne() {
		Assert.Equal(-1, _greenzone.GetNearestStateFrame(100));
	}

	[Fact]
	public void GetNearestStateFrame_ExactMatch_ReturnsFrame() {
		_greenzone.CaptureState(100, new byte[64], forceCapture: true);
		_greenzone.CaptureState(200, new byte[64], forceCapture: true);

		Assert.Equal(100, _greenzone.GetNearestStateFrame(100));
		Assert.Equal(200, _greenzone.GetNearestStateFrame(200));
	}

	[Fact]
	public void GetNearestStateFrame_BetweenFrames_ReturnsEarlier() {
		_greenzone.CaptureState(100, new byte[64], forceCapture: true);
		_greenzone.CaptureState(200, new byte[64], forceCapture: true);
		_greenzone.CaptureState(300, new byte[64], forceCapture: true);

		Assert.Equal(100, _greenzone.GetNearestStateFrame(150));
		Assert.Equal(200, _greenzone.GetNearestStateFrame(250));
		Assert.Equal(200, _greenzone.GetNearestStateFrame(299));
	}

	[Fact]
	public void GetNearestStateFrame_BeforeAllFrames_ReturnsNegativeOne() {
		_greenzone.CaptureState(100, new byte[64], forceCapture: true);
		_greenzone.CaptureState(200, new byte[64], forceCapture: true);

		Assert.Equal(-1, _greenzone.GetNearestStateFrame(50));
		Assert.Equal(-1, _greenzone.GetNearestStateFrame(0));
	}

	[Fact]
	public void GetNearestStateFrame_AfterAllFrames_ReturnsLatest() {
		_greenzone.CaptureState(100, new byte[64], forceCapture: true);
		_greenzone.CaptureState(200, new byte[64], forceCapture: true);

		Assert.Equal(200, _greenzone.GetNearestStateFrame(500));
		Assert.Equal(200, _greenzone.GetNearestStateFrame(int.MaxValue));
	}

	[Fact]
	public void GetNearestStateFrame_SingleFrame_FindsIt() {
		_greenzone.CaptureState(0, new byte[64], forceCapture: true);

		Assert.Equal(0, _greenzone.GetNearestStateFrame(0));
		Assert.Equal(0, _greenzone.GetNearestStateFrame(100));
		Assert.Equal(-1, _greenzone.GetNearestStateFrame(-1));
	}

	[Fact]
	public void GetNearestStateFrame_AfterInvalidation_ReflectsChanges() {
		_greenzone.CaptureState(100, new byte[64], forceCapture: true);
		_greenzone.CaptureState(200, new byte[64], forceCapture: true);
		_greenzone.CaptureState(300, new byte[64], forceCapture: true);

		_greenzone.InvalidateFrom(200);

		Assert.Equal(100, _greenzone.GetNearestStateFrame(250));
		Assert.Equal(100, _greenzone.GetNearestStateFrame(200));
		Assert.Equal(100, _greenzone.GetNearestStateFrame(100));
	}

	#endregion

	#region InvalidateFrom Tests

	[Fact]
	public void InvalidateFrom_RemovesFramesFromAndAbove() {
		_greenzone.CaptureState(100, new byte[64], forceCapture: true);
		_greenzone.CaptureState(200, new byte[64], forceCapture: true);
		_greenzone.CaptureState(300, new byte[64], forceCapture: true);
		_greenzone.CaptureState(400, new byte[64], forceCapture: true);

		_greenzone.InvalidateFrom(250);

		Assert.True(_greenzone.HasState(100));
		Assert.True(_greenzone.HasState(200));
		Assert.False(_greenzone.HasState(300));
		Assert.False(_greenzone.HasState(400));
		Assert.Equal(2, _greenzone.SavestateCount);
	}

	[Fact]
	public void InvalidateFrom_IncludesExactFrame() {
		_greenzone.CaptureState(100, new byte[64], forceCapture: true);
		_greenzone.CaptureState(200, new byte[64], forceCapture: true);

		_greenzone.InvalidateFrom(200);

		Assert.True(_greenzone.HasState(100));
		Assert.False(_greenzone.HasState(200));
	}

	[Fact]
	public void InvalidateFrom_BeyondAllFrames_NoChange() {
		_greenzone.CaptureState(100, new byte[64], forceCapture: true);
		_greenzone.CaptureState(200, new byte[64], forceCapture: true);

		_greenzone.InvalidateFrom(500);

		Assert.Equal(2, _greenzone.SavestateCount);
	}

	[Fact]
	public void InvalidateFrom_BeforeAllFrames_ClearsAll() {
		_greenzone.CaptureState(100, new byte[64], forceCapture: true);
		_greenzone.CaptureState(200, new byte[64], forceCapture: true);

		_greenzone.InvalidateFrom(0);

		Assert.Equal(0, _greenzone.SavestateCount);
		Assert.Equal(-1, _greenzone.LatestFrame);
		Assert.Equal(0, _greenzone.TotalMemoryUsage);
	}

	#endregion

	#region CaptureState Index Consistency

	[Fact]
	public void CaptureState_MaintainsConsistentIndex() {
		// Add states out of order
		_greenzone.CaptureState(300, new byte[64], forceCapture: true);
		_greenzone.CaptureState(100, new byte[64], forceCapture: true);
		_greenzone.CaptureState(500, new byte[64], forceCapture: true);
		_greenzone.CaptureState(200, new byte[64], forceCapture: true);
		_greenzone.CaptureState(400, new byte[64], forceCapture: true);

		Assert.Equal(100, _greenzone.EarliestFrame);
		Assert.Equal(500, _greenzone.LatestFrame);
		Assert.Equal(5, _greenzone.SavestateCount);

		// Nearest frame queries should be correct
		Assert.Equal(200, _greenzone.GetNearestStateFrame(250));
		Assert.Equal(300, _greenzone.GetNearestStateFrame(350));
	}

	[Fact]
	public void CaptureState_ReplacingExisting_MaintainsCount() {
		_greenzone.CaptureState(100, new byte[64], forceCapture: true);
		_greenzone.CaptureState(200, new byte[64], forceCapture: true);

		// Replace existing
		_greenzone.CaptureState(100, new byte[128], forceCapture: true);

		Assert.Equal(2, _greenzone.SavestateCount);
		Assert.Equal(100, _greenzone.EarliestFrame);
		Assert.Equal(200, _greenzone.LatestFrame);
	}

	#endregion

	#region Clear Tests

	[Fact]
	public void Clear_ResetsEverything() {
		_greenzone.CaptureState(100, new byte[1024], forceCapture: true);
		_greenzone.CaptureState(200, new byte[2048], forceCapture: true);

		_greenzone.Clear();

		Assert.Equal(-1, _greenzone.LatestFrame);
		Assert.Equal(-1, _greenzone.EarliestFrame);
		Assert.Equal(0, _greenzone.TotalMemoryUsage);
		Assert.Equal(0, _greenzone.SavestateCount);
		Assert.Equal(-1, _greenzone.GetNearestStateFrame(100));
	}

	#endregion

	#region SavestateCaptured Event

	[Fact]
	public void CaptureState_ForceCapture_RaisesEvent() {
		int capturedFrame = -1;
		int capturedSize = -1;
		_greenzone.SavestateCaptured += (_, e) => {
			capturedFrame = e.Frame;
			capturedSize = e.Size;
		};

		_greenzone.CaptureState(42, new byte[512], forceCapture: true);

		Assert.Equal(42, capturedFrame);
		Assert.Equal(512, capturedSize);
	}

	#endregion

	#region Ring Buffer + InvalidateFrom Integration

	[Fact]
	public void InvalidateFrom_WithManyStates_PreservesAllBelowThreshold() {
		// Capture many states to exercise the ring buffer rebuild path
		for (int i = 0; i < 50; i++) {
			_greenzone.CaptureState(i * 10, new byte[64], forceCapture: true);
		}

		Assert.Equal(50, _greenzone.SavestateCount);

		// Invalidate from frame 250 — should keep frames 0..240 (25 frames)
		_greenzone.InvalidateFrom(250);

		Assert.Equal(25, _greenzone.SavestateCount);
		Assert.Equal(0, _greenzone.EarliestFrame);
		Assert.Equal(240, _greenzone.LatestFrame);

		// All frames below threshold should still be present
		for (int i = 0; i < 25; i++) {
			Assert.True(_greenzone.HasState(i * 10));
		}

		// All frames at/above threshold should be gone
		for (int i = 25; i < 50; i++) {
			Assert.False(_greenzone.HasState(i * 10));
		}
	}

	[Fact]
	public void InvalidateFrom_RepeatedCalls_ConvergesCorrectly() {
		for (int i = 0; i < 20; i++) {
			_greenzone.CaptureState(i * 100, new byte[64], forceCapture: true);
		}

		// Cascade invalidations
		_greenzone.InvalidateFrom(1500);
		Assert.Equal(15, _greenzone.SavestateCount);

		_greenzone.InvalidateFrom(1000);
		Assert.Equal(10, _greenzone.SavestateCount);

		_greenzone.InvalidateFrom(500);
		Assert.Equal(5, _greenzone.SavestateCount);
		Assert.Equal(400, _greenzone.LatestFrame);
	}

	#endregion

	#region GetSavestateInfo Tests

	[Fact]
	public void GetSavestateInfo_Empty_ReturnsEmptyList() {
		var info = _greenzone.GetSavestateInfo();
		Assert.Empty(info);
	}

	[Fact]
	public void GetSavestateInfo_ReturnsCorrectFrameAndSize() {
		_greenzone.CaptureState(100, new byte[1024], forceCapture: true);
		_greenzone.CaptureState(200, new byte[2048], forceCapture: true);

		var info = _greenzone.GetSavestateInfo();

		Assert.Equal(2, info.Count);
		Assert.Equal(100, info[0].Frame);
		Assert.Equal(1024, info[0].Size);
		Assert.Equal(200, info[1].Frame);
		Assert.Equal(2048, info[1].Size);
	}

	[Fact]
	public void GetSavestateInfo_ReturnedInFrameOrder() {
		// Insert out of order
		_greenzone.CaptureState(500, new byte[64], forceCapture: true);
		_greenzone.CaptureState(100, new byte[64], forceCapture: true);
		_greenzone.CaptureState(300, new byte[64], forceCapture: true);

		var info = _greenzone.GetSavestateInfo();

		Assert.Equal(3, info.Count);
		Assert.Equal(100, info[0].Frame);
		Assert.Equal(300, info[1].Frame);
		Assert.Equal(500, info[2].Frame);
	}

	[Fact]
	public void GetSavestateInfo_AfterInvalidation_ExcludesRemoved() {
		_greenzone.CaptureState(100, new byte[64], forceCapture: true);
		_greenzone.CaptureState(200, new byte[64], forceCapture: true);
		_greenzone.CaptureState(300, new byte[64], forceCapture: true);

		_greenzone.InvalidateFrom(200);

		var info = _greenzone.GetSavestateInfo();

		Assert.Single(info);
		Assert.Equal(100, info[0].Frame);
	}

	#endregion

	#region CaptureInterval Tests

	[Fact]
	public void CaptureState_WithoutForce_RespectsInterval() {
		_greenzone.CaptureInterval = 60;

		// Frame 60 is on the interval
		_greenzone.CaptureState(60, new byte[64]);
		Assert.True(_greenzone.HasState(60));

		// Frame 30 is NOT on the interval
		_greenzone.CaptureState(30, new byte[64]);
		Assert.False(_greenzone.HasState(30));
	}

	[Fact]
	public void CaptureState_Frame0_AlwaysCapturedByInterval() {
		_greenzone.CaptureInterval = 60;

		// Frame 0: 0 % 60 == 0, so it should be captured
		_greenzone.CaptureState(0, new byte[64]);
		Assert.True(_greenzone.HasState(0));
	}

	[Fact]
	public void CaptureState_ForceCapture_IgnoresInterval() {
		_greenzone.CaptureInterval = 60;

		// Frame 37 is NOT on interval, but forceCapture overrides
		_greenzone.CaptureState(37, new byte[64], forceCapture: true);
		Assert.True(_greenzone.HasState(37));
	}

	#endregion

	#region MaxSavestates Pruning Tests

	[Fact]
	public void CaptureState_OverLimit_PrunesSavestates() {
		_greenzone.MaxSavestates = 10;

		for (int i = 0; i < 20; i++) {
			_greenzone.CaptureState(i, new byte[64], forceCapture: true);
		}

		// Should have pruned back to or below MaxSavestates
		Assert.True(_greenzone.SavestateCount <= _greenzone.MaxSavestates + 1);
	}

	#endregion

	#region Dispose Tests

	[Fact]
	public void CaptureState_AfterDispose_IsNoOp() {
		var gz = new GreenzoneManager();
		gz.CaptureState(100, new byte[64], forceCapture: true);
		Assert.True(gz.HasState(100));

		gz.Dispose();

		// Capture after dispose should be silently ignored
		gz.CaptureState(200, new byte[64], forceCapture: true);
		Assert.False(gz.HasState(200));
	}

	[Fact]
	public void Dispose_MultipleCallsDoNotThrow() {
		var gz = new GreenzoneManager();
		gz.CaptureState(100, new byte[64], forceCapture: true);

		gz.Dispose();
		gz.Dispose(); // No exception
	}

	#endregion

	#region Compression Debounce Tests

	[Fact]
	public void CompressionDebounceInterval_DefaultIsTen() {
		Assert.Equal(10, _greenzone.CompressionDebounceInterval);
	}

	[Fact]
	public void CompressionDebounceInterval_IsSettable() {
		_greenzone.CompressionDebounceInterval = 5;
		Assert.Equal(5, _greenzone.CompressionDebounceInterval);
	}

	[Fact]
	public void CaptureState_BelowDebounceInterval_DoesNotThrow() {
		_greenzone.CompressionEnabled = true;
		_greenzone.CompressionDebounceInterval = 5;

		// Capture fewer states than debounce interval — should not throw
		for (int i = 0; i < 4; i++) {
			_greenzone.CaptureState(i * 60, new byte[64], forceCapture: true);
		}

		Assert.Equal(4, _greenzone.SavestateCount);
	}

	[Fact]
	public void CaptureState_AtDebounceInterval_DoesNotThrow() {
		_greenzone.CompressionEnabled = true;
		_greenzone.CompressionDebounceInterval = 3;

		// Capture exactly at and past debounce interval — should trigger compression without errors
		for (int i = 0; i < 10; i++) {
			_greenzone.CaptureState(i * 60, new byte[64], forceCapture: true);
		}

		Assert.Equal(10, _greenzone.SavestateCount);
	}

	#endregion
}
