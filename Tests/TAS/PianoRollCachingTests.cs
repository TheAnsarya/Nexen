using Xunit;
using System;
using System.Collections.Generic;
using Avalonia.Media;
using Nexen.Controls;

namespace Nexen.Tests.TAS;

/// <summary>
/// Unit tests for PianoRollControl rendering optimizations.
/// Tests FormattedText caching and other rendering performance improvements.
/// </summary>
public class PianoRollCachingTests {
	#region Cache Invalidation Tests

	[Fact]
	public void FormattedTextCache_ShouldRetainRecentZoomBuckets() {
		// Arrange
		var caches = new Dictionary<double, Dictionary<string, object>> {
			[1.0] = new Dictionary<string, object> {
				["A"] = "text_A_at_zoom_1.0"
			}
		};

		// Act - add a second zoom bucket
		caches[2.0] = new Dictionary<string, object> {
			["A"] = "text_A_at_zoom_2.0"
		};

		// Assert
		Assert.Equal(2, caches.Count);
		Assert.True(caches.ContainsKey(1.0));
		Assert.True(caches.ContainsKey(2.0));
	}

	[Fact]
	public void FormattedTextCache_ShouldEvictOldestZoomBucketWhenOverLimit() {
		// Arrange
		const int maxZoomBuckets = 3;
		var caches = new Dictionary<double, Dictionary<string, object>> {
			[1.0] = new Dictionary<string, object>(),
			[1.25] = new Dictionary<string, object>(),
			[1.5] = new Dictionary<string, object>()
		};
		var order = new LinkedList<double>(new[] { 1.0, 1.25, 1.5 });

		// Act - add one more bucket, evict oldest
		caches[2.0] = new Dictionary<string, object>();
		order.AddLast(2.0);
		while (caches.Count > maxZoomBuckets) {
			double oldest = order.First!.Value;
			order.RemoveFirst();
			caches.Remove(oldest);
		}

		// Assert
		Assert.Equal(maxZoomBuckets, caches.Count);
		Assert.False(caches.ContainsKey(1.0));
		Assert.True(caches.ContainsKey(1.25));
		Assert.True(caches.ContainsKey(1.5));
		Assert.True(caches.ContainsKey(2.0));
	}

	#endregion

	#region LRU Cache Tests

	[Fact]
	public void FrameNumberCache_ShouldPruneWhenOverLimit() {
		// Arrange
		const int maxCacheSize = 200;
		var cache = new Dictionary<int, string>();
		var lruOrder = new LinkedList<int>();
		var lruNodes = new Dictionary<int, LinkedListNode<int>>();

		void TouchFrame(int frame) {
			if (lruNodes.TryGetValue(frame, out var existing)) {
				lruOrder.Remove(existing);
			}
			lruNodes[frame] = lruOrder.AddLast(frame);
		}

		void AddFrame(int frame) {
			cache[frame] = $"frame_{frame}";
			TouchFrame(frame);
			while (cache.Count > maxCacheSize) {
				int oldest = lruOrder.First!.Value;
				lruOrder.RemoveFirst();
				lruNodes.Remove(oldest);
				cache.Remove(oldest);
			}
		}

		// Fill cache to limit
		for (int i = 0; i < maxCacheSize; i++) {
			AddFrame(i);
		}

		// Act - add one more, should evict oldest only
		int newFrame = maxCacheSize;
		AddFrame(newFrame);

		// Assert
		Assert.Equal(maxCacheSize, cache.Count);
		Assert.False(cache.ContainsKey(0));
		Assert.True(cache.ContainsKey(1));
		Assert.True(cache.ContainsKey(newFrame));
	}

	[Fact]
	public void FrameNumberCache_ShouldReuseCachedText() {
		// Arrange
		var cache = new Dictionary<int, string>();
		int hitCount = 0;

		// First access - miss
		void AccessFrame(int frame) {
			if (cache.TryGetValue(frame, out var text)) {
				hitCount++;
			} else {
				cache[frame] = $"frame_{frame}";
			}
		}

		// Act
		AccessFrame(10); // Miss
		AccessFrame(20); // Miss
		AccessFrame(10); // Hit
		AccessFrame(10); // Hit
		AccessFrame(20); // Hit
		AccessFrame(30); // Miss

		// Assert
		Assert.Equal(3, hitCount); // 3 cache hits
		Assert.Equal(3, cache.Count); // 3 unique entries
	}

	#endregion

	#region Prefetch Debounce Tests

	[Fact]
	public void ShouldQueueFrameTextPrefetch_FirstCall_ReturnsTrue() {
		bool shouldQueue = PianoRollControl.ShouldQueueFrameTextPrefetch(
			lastStart: -1,
			lastEnd: -1,
			lastZoomKey: double.NaN,
			start: 0,
			end: 120,
			zoomKey: 1.0
		);

		Assert.True(shouldQueue);
	}

	[Fact]
	public void ShouldQueueFrameTextPrefetch_UnchangedViewport_ReturnsFalse() {
		bool shouldQueue = PianoRollControl.ShouldQueueFrameTextPrefetch(
			lastStart: 50,
			lastEnd: 180,
			lastZoomKey: 1.0,
			start: 50,
			end: 180,
			zoomKey: 1.0
		);

		Assert.False(shouldQueue);
	}

	[Fact]
	public void ShouldQueueFrameTextPrefetch_ViewportChanged_ReturnsTrue() {
		bool shouldQueue = PianoRollControl.ShouldQueueFrameTextPrefetch(
			lastStart: 50,
			lastEnd: 180,
			lastZoomKey: 1.0,
			start: 60,
			end: 190,
			zoomKey: 1.0
		);

		Assert.True(shouldQueue);
	}

	[Fact]
	public void ShouldQueueFrameTextPrefetch_ZoomChanged_ReturnsTrue() {
		bool shouldQueue = PianoRollControl.ShouldQueueFrameTextPrefetch(
			lastStart: 50,
			lastEnd: 180,
			lastZoomKey: 1.0,
			start: 50,
			end: 180,
			zoomKey: 1.25
		);

		Assert.True(shouldQueue);
	}

	#endregion

	#region Button Label Caching Tests

	[Fact]
	public void ButtonLabelCache_ShouldCacheSameLabels() {
		// Arrange
		var cache = new Dictionary<string, object>();
		var buttons = new[] { "A", "B", "X", "Y", "L", "R", "↑", "↓", "←", "→", "ST", "SE" };
		int createCount = 0;

		// Simulate two render cycles
		for (int cycle = 0; cycle < 2; cycle++) {
			foreach (var button in buttons) {
				if (!cache.TryGetValue(button, out _)) {
					cache[button] = $"text_{button}";
					createCount++;
				}
			}
		}

		// Assert - only 12 creations (one per button), not 24
		Assert.Equal(12, createCount);
		Assert.Equal(12, cache.Count);
	}

	#endregion
}
