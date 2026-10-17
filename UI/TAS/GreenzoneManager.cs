using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using Nexen.Interop;

namespace Nexen.TAS;

/// <summary>
/// Manages greenzone savestates for efficient TAS rerecording.
/// The greenzone is a set of savestates that allow instant seeking to any frame.
/// </summary>
public sealed class GreenzoneManager : IDisposable {
	private readonly ConcurrentDictionary<int, SavestateData> _savestates = new();
	private readonly SortedSet<int> _frameIndex = new();
	private readonly object _stateLock = new();
	private long _totalMemoryUsage;
	private readonly SemaphoreSlim _compressionSemaphore = new(1, 1);
	private readonly Queue<int> _ringBuffer = new();
	private CancellationTokenSource? _seekCts;
	private readonly object _seekLock = new();
	private readonly object _pruneLock = new();
	private readonly List<int> _pruneBuffer = new();
	private int _capturesSinceLastCompression;
	private bool _disposed;

	/// <summary>Gets or sets the interval between automatic greenzone captures.</summary>
	public int CaptureInterval { get; set; } = 60; // Every 60 frames by default

	/// <summary>Gets or sets the maximum number of savestates to keep in memory.</summary>
	public int MaxSavestates { get; set; } = 1000;

	/// <summary>Gets or sets the ring buffer size for recent frames (fast rewind).</summary>
	public int RingBufferSize { get; set; } = 120; // ~2 seconds at 60fps

	/// <summary>Gets or sets whether compression is enabled for older states.</summary>
	public bool CompressionEnabled { get; set; } = true;

	/// <summary>Gets or sets how many captures between compression sweeps (debounce). Default 10.</summary>
	public int CompressionDebounceInterval { get; set; } = 10;

	/// <summary>Gets or sets the frame threshold after which states get compressed.</summary>
	public int CompressionThreshold { get; set; } = 300; // 5 seconds

	/// <summary>Gets or sets the maximum memory budget in bytes. When exceeded, oldest states are pruned.</summary>
	public long MaxMemoryBytes { get; set; } = 256L * 1024 * 1024; // 256 MB default

	/// <summary>Gets the current frame with the latest savestate. O(log n) via SortedSet.</summary>
	public int LatestFrame {
		get {
			lock (_stateLock) {
				return _frameIndex.Count > 0 ? _frameIndex.Max : -1;
			}
		}
	}

	/// <summary>Gets the earliest frame with a savestate. O(log n) via SortedSet.</summary>
	public int EarliestFrame {
		get {
			lock (_stateLock) {
				return _frameIndex.Count > 0 ? _frameIndex.Min : -1;
			}
		}
	}

	/// <summary>Gets the total number of savestates.</summary>
	public int SavestateCount => _savestates.Count;

	/// <summary>Gets the total memory usage of all savestates in bytes. O(1) tracked field.</summary>
	public long TotalMemoryUsage => Interlocked.Read(ref _totalMemoryUsage);

	/// <summary>Occurs when a savestate is captured.</summary>
	public event EventHandler<GreenzoneEventArgs>? SavestateCaptured;

	/// <summary>Occurs when a savestate is loaded.</summary>
	public event EventHandler<GreenzoneEventArgs>? SavestateLoaded;

	/// <summary>Occurs when memory limit is reached and pruning occurs.</summary>
	public event EventHandler<GreenzonePruneEventArgs>? SavestatesPruned;

	/// <summary>
	/// Captures a savestate at the specified frame.
	/// </summary>
	/// <param name="frame">The frame number.</param>
	/// <param name="data">The savestate data.</param>
	/// <param name="forceCapture">If true, captures regardless of interval settings.</param>
	public void CaptureState(int frame, byte[] data, bool forceCapture = false) {
		if (_disposed) {
			return;
		}

		// Check if we should capture based on interval
		if (!forceCapture && frame % CaptureInterval != 0) {
			return;
		}

		var savestate = new SavestateData {
			Frame = frame,
			Payload = new SavestatePayload(data, isCompressed: false),
			CaptureTime = DateTime.UtcNow,
		};

		// Track memory: subtract old state if replacing, add new
		if (_savestates.TryGetValue(frame, out var existing)) {
			Interlocked.Add(ref _totalMemoryUsage, -(existing.Payload?.Data.LongLength ?? 0));
		}

		_savestates[frame] = savestate;

		lock (_stateLock) {
			_frameIndex.Add(frame);
		}

		Interlocked.Add(ref _totalMemoryUsage, data.Length);
		AddToRingBuffer(frame);

		// Prune if we're over the count limit or memory budget
		if (_savestates.Count > MaxSavestates || Interlocked.Read(ref _totalMemoryUsage) > MaxMemoryBytes) {
			PruneSavestates();
		}

		// Compress old states in background (debounced to avoid firing on every single capture)
		if (CompressionEnabled && ++_capturesSinceLastCompression >= CompressionDebounceInterval) {
			_capturesSinceLastCompression = 0;
			_ = CompressOldStatesAsync();
		}

		SavestateCaptured?.Invoke(this, new GreenzoneEventArgs(frame, data.Length));
	}

	/// <summary>
	/// Captures a savestate from the current emulation state.
	/// </summary>
	/// <param name="frame">The frame number.</param>
	public void CaptureCurrentState(int frame) {
		if (_disposed || !EmuApi.IsRunning()) {
			return;
		}

		try {
			// Query required buffer size first (pass null buffer)
			int requiredSize = EmuApi.SaveStateToMemory(IntPtr.Zero, 0);

			if (requiredSize <= 0) {
				return;
			}

			// Allocate buffer and save state directly to memory
			byte[] data = new byte[requiredSize];
			unsafe {
				fixed (byte* ptr = data) {
					int actualSize = EmuApi.SaveStateToMemory((IntPtr)ptr, requiredSize);

					if (actualSize > 0 && actualSize <= requiredSize) {
						// Trim if actual size differs
						if (actualSize < requiredSize) {
							Array.Resize(ref data, actualSize);
						}

						CaptureState(frame, data, forceCapture: true);
					}
				}
			}
		} catch (Exception ex) {
			System.Diagnostics.Debug.WriteLine($"Failed to capture greenzone state at frame {frame}: {ex.Message}");
		}
	}

	/// <summary>
	/// Loads a savestate for the specified frame.
	/// </summary>
	/// <param name="frame">The target frame number.</param>
	/// <returns>True if a state was loaded, false otherwise.</returns>
	public bool LoadState(int frame) {
		if (_disposed) {
			return false;
		}

		// Try exact match first
		if (_savestates.TryGetValue(frame, out var exactState)) {
			return LoadSavestateData(exactState);
		}

		// Find nearest earlier state
		var nearestFrame = GetNearestStateFrame(frame);

		if (nearestFrame < 0 || !_savestates.TryGetValue(nearestFrame, out var nearestState)) {
			return false;
		}

		return LoadSavestateData(nearestState);
	}

	/// <summary>
	/// Gets the nearest frame with a savestate at or before the target frame.
	/// O(log n) via SortedSet.GetViewBetween.
	/// </summary>
	/// <param name="targetFrame">The target frame.</param>
	/// <returns>The nearest frame with a savestate, or -1 if none exists.</returns>
	public int GetNearestStateFrame(int targetFrame) {
		lock (_stateLock) {
			if (_frameIndex.Count == 0) {
				return -1;
			}

			int minFrame = _frameIndex.Min;

			if (minFrame > targetFrame) {
				return -1;
			}

			return _frameIndex.GetViewBetween(minFrame, targetFrame).Max;
		}
	}

	/// <summary>
	/// Seeks to a specific frame, loading the nearest savestate and advancing if needed.
	/// Cancels any previous in-flight seek automatically.
	/// </summary>
	/// <param name="targetFrame">The target frame.</param>
	/// <param name="progress">Optional progress callback receiving (currentFrame, targetFrame).</param>
	/// <returns>The actual frame reached after seeking, or -1 if cancelled/failed.</returns>
	public async Task<int> SeekToFrameAsync(int targetFrame, Action<int, int>? progress = null) {
		if (_disposed || !EmuApi.IsRunning()) {
			return -1;
		}

		// Cancel any previous seek and create a new token
		CancellationToken ct;
		lock (_seekLock) {
			_seekCts?.Cancel();
			_seekCts?.Dispose();
			_seekCts = new CancellationTokenSource();
			ct = _seekCts.Token;
		}

		int nearestFrame = GetNearestStateFrame(targetFrame);

		if (nearestFrame < 0) {
			return -1;
		}

		// Load the nearest state
		if (!LoadState(nearestFrame)) {
			return -1;
		}

		// If we need to advance, do so efficiently
		int framesToAdvance = targetFrame - nearestFrame;

		if (framesToAdvance > 0) {
			const int yieldInterval = 20;

			for (int i = 0; i < framesToAdvance; i++) {
				if (ct.IsCancellationRequested) {
					return -1;
				}

				EmuApi.ExecuteShortcut(new ExecuteShortcutParams {
					Shortcut = Config.Shortcuts.EmulatorShortcut.RunSingleFrame
				});

				if (i % yieldInterval == 0) {
					progress?.Invoke(nearestFrame + i, targetFrame);
					await Task.Yield();
				}
			}
		}

		SavestateLoaded?.Invoke(this, new GreenzoneEventArgs(targetFrame, 0));
		return targetFrame;
	}

	/// <summary>
	/// Cancels any in-flight seek operation.
	/// </summary>
	public void CancelSeek() {
		lock (_seekLock) {
			_seekCts?.Cancel();
		}
	}

	/// <summary>
	/// Invalidates all savestates from the specified frame onwards.
	/// Call this when the movie is modified at a frame.
	/// </summary>
	/// <param name="fromFrame">The first frame to invalidate.</param>
	public void InvalidateFrom(int fromFrame) {
		List<int> framesToRemove;

		lock (_stateLock) {
			if (_frameIndex.Count == 0 || _frameIndex.Max < fromFrame) {
				framesToRemove = [];
			} else {
				var view = _frameIndex.GetViewBetween(fromFrame, _frameIndex.Max);
				framesToRemove = new List<int>(view.Count);
				foreach (var f in view) {
					framesToRemove.Add(f);
				}
			}

			foreach (var frame in framesToRemove) {
				_frameIndex.Remove(frame);
			}

			// Also clear ring buffer entries — rebuild in-place to avoid allocation
			int count = _ringBuffer.Count;
			for (int i = 0; i < count; i++) {
				int f = _ringBuffer.Dequeue();
				if (f < fromFrame) {
					_ringBuffer.Enqueue(f);
				}
			}
		}

		foreach (var frame in framesToRemove) {
			if (_savestates.TryRemove(frame, out var state)) {
				Interlocked.Add(ref _totalMemoryUsage, -(state.Payload?.Data.LongLength ?? 0));
			}
		}
	}

	/// <summary>
	/// Clears all savestates.
	/// </summary>
	public void Clear() {
		_savestates.Clear();

		lock (_stateLock) {
			_frameIndex.Clear();
			_ringBuffer.Clear();
		}

		Interlocked.Exchange(ref _totalMemoryUsage, 0);
	}

	/// <summary>
	/// Gets information about all stored savestates.
	/// </summary>
	/// <returns>List of savestate information.</returns>
	public List<SavestateInfo> GetSavestateInfo() {
		return _savestates.Values
			.Select(s => {
				var p = s.Payload;
				return new SavestateInfo {
					Frame = s.Frame,
					Size = p?.Data.Length ?? 0,
					IsCompressed = p?.IsCompressed ?? false,
					CaptureTime = s.CaptureTime
				};
			})
			.OrderBy(s => s.Frame)
			.ToList();
	}

	/// <summary>
	/// Checks if a savestate exists for the specified frame.
	/// </summary>
	/// <param name="frame">The frame number.</param>
	/// <returns>True if a savestate exists.</returns>
	public bool HasState(int frame) => _savestates.ContainsKey(frame);

	private void AddToRingBuffer(int frame) {
		lock (_stateLock) {
			if (_ringBuffer.Count >= RingBufferSize) {
				int oldestFrame = _ringBuffer.Dequeue();

				// Don't remove from main storage if it's at an interval
				if (oldestFrame % CaptureInterval != 0) {
					if (_savestates.TryRemove(oldestFrame, out var removed)) {
						_frameIndex.Remove(oldestFrame);
						Interlocked.Add(ref _totalMemoryUsage, -(removed.Payload?.Data.LongLength ?? 0));
					}
				}
			}

			_ringBuffer.Enqueue(frame);
		}
	}

	private bool LoadSavestateData(SavestateData state) {
		// Read the immutable payload snapshot atomically — no torn reads possible
		var payload = state.Payload;

		if (payload is null) {
			return false;
		}

		byte[] data = payload.IsCompressed ? DecompressData(payload.Data) : payload.Data;

		try {
			// Load state directly from memory buffer
			unsafe {
				fixed (byte* ptr = data) {
					bool success = EmuApi.LoadStateFromMemory((IntPtr)ptr, data.Length);

					if (success) {
						SavestateLoaded?.Invoke(this, new GreenzoneEventArgs(state.Frame, data.Length));
					}

					return success;
				}
			}
		} catch (Exception ex) {
			System.Diagnostics.Debug.WriteLine($"Failed to load greenzone state at frame {state.Frame}: {ex.Message}");
			return false;
		}
	}

	private void PruneSavestates() {
		lock (_pruneLock) {
			// Zero-allocation pruning: iterate _frameIndex (already sorted) directly
			// instead of LINQ .Where().OrderBy().Take().Select().ToList()
			int removeTarget = _savestates.Count - MaxSavestates + 100; // Leave headroom

			// Also prune for memory budget if exceeded
			long currentMemory = Interlocked.Read(ref _totalMemoryUsage);
			bool memoryOverBudget = currentMemory > MaxMemoryBytes;

			if (removeTarget <= 0 && !memoryOverBudget) {
				return;
			}

			// Reuse the buffer to avoid allocation on every prune call
			_pruneBuffer.Clear();

			lock (_stateLock) {
				foreach (int frame in _frameIndex) {
					// Skip interval-aligned frames (preserve keyframes)
					if (frame % CaptureInterval == 0) {
						continue;
					}

					_pruneBuffer.Add(frame);

					// For count-based pruning, stop once we have enough candidates
					if (!memoryOverBudget && _pruneBuffer.Count >= removeTarget) {
						break;
					}
				}
			}

			int prunedCount = 0;
			long freedMemory = 0;

			foreach (int frame in _pruneBuffer) {
				if (_savestates.TryRemove(frame, out var state)) {
					lock (_stateLock) {
						_frameIndex.Remove(frame);
					}

					long size = state.Payload?.Data.LongLength ?? 0;
					prunedCount++;
					freedMemory += size;
					Interlocked.Add(ref _totalMemoryUsage, -size);

					// For memory-based pruning, stop once we're back under budget
					if (memoryOverBudget && Interlocked.Read(ref _totalMemoryUsage) <= MaxMemoryBytes) {
						break;
					}
				}
			}

			SavestatesPruned?.Invoke(this, new GreenzonePruneEventArgs(prunedCount, freedMemory));
		}
	}

	private async Task CompressOldStatesAsync() {
		await _compressionSemaphore.WaitAsync();

		try {
			int latestFrame = LatestFrame;

			foreach (var kv in _savestates) {
				var payload = kv.Value.Payload;

				if (payload is null || payload.IsCompressed) {
					continue;
				}

				if (latestFrame - kv.Key <= CompressionThreshold) {
					continue;
				}

				byte[] originalData = payload.Data;
				var compressed = CompressData(originalData);

				// Only use compressed if it's actually smaller
				if (compressed.Length < originalData.Length) {
					long sizeDelta = compressed.Length - originalData.Length;
					// Atomically swap the entire payload — readers always see
					// a consistent (Data, IsCompressed) pair
					kv.Value.Payload = new SavestatePayload(compressed, isCompressed: true);
					Interlocked.Add(ref _totalMemoryUsage, sizeDelta);
				}
			}
		} catch (Exception ex) {
			System.Diagnostics.Debug.WriteLine($"Background greenzone compression failed: {ex.Message}");
		} finally {
			_compressionSemaphore.Release();
		}
	}

	/// <summary>
	/// Compresses data using GZip with a pre-sized output stream to avoid MemoryStream.ToArray() copy.
	/// </summary>
	private static byte[] CompressData(byte[] data) {
		// Pre-size to ~75% of input (typical GZip compression ratio for save states)
		using var output = new MemoryStream(data.Length * 3 / 4);
		using (var gzip = new GZipStream(output, CompressionLevel.Fastest)) {
			gzip.Write(data, 0, data.Length);
		}

		return output.ToArray();
	}

	/// <summary>
	/// Decompresses GZip data with a pre-sized output stream to reduce reallocations.
	/// </summary>
	private static byte[] DecompressData(byte[] compressedData) {
		using var input = new MemoryStream(compressedData);
		using var gzip = new GZipStream(input, CompressionMode.Decompress);
		// Pre-size to 4x compressed size (typical decompression ratio)
		using var output = new MemoryStream(compressedData.Length * 4);
		gzip.CopyTo(output);
		return output.ToArray();
	}

	public void Dispose() {
		if (_disposed) {
			return;
		}

		_disposed = true;

		lock (_seekLock) {
			_seekCts?.Cancel();
			_seekCts?.Dispose();
			_seekCts = null;
		}

		_savestates.Clear();

		lock (_stateLock) {
			_frameIndex.Clear();
		}

		Interlocked.Exchange(ref _totalMemoryUsage, 0);
		_compressionSemaphore.Dispose();
	}
}

/// <summary>
/// Immutable snapshot of savestate payload, enabling atomic swap of (Data, IsCompressed).
/// </summary>
internal sealed class SavestatePayload {
	public byte[] Data { get; }
	public bool IsCompressed { get; }

	public SavestatePayload(byte[] data, bool isCompressed) {
		Data = data;
		IsCompressed = isCompressed;
	}
}

/// <summary>
/// Internal class to store savestate data.
/// The <see cref="Payload"/> field is swapped atomically to avoid torn reads
/// during background compression.
/// </summary>
internal sealed class SavestateData {
	public int Frame { get; set; }
	public DateTime CaptureTime { get; set; }

	/// <summary>
	/// Atomic payload: read via <see cref="Volatile.Read{T}(ref T)"/>,
	/// written via <see cref="Volatile.Write{T}(ref T, T)"/>.
	/// </summary>
	private SavestatePayload? _payload;

	public SavestatePayload? Payload {
		get => Volatile.Read(ref _payload);
		set => Volatile.Write(ref _payload, value);
	}
}

/// <summary>
/// Public information about a savestate.
/// </summary>
public sealed class SavestateInfo {
	/// <summary>Gets or sets the frame number.</summary>
	public int Frame { get; set; }

	/// <summary>Gets or sets the size in bytes.</summary>
	public int Size { get; set; }

	/// <summary>Gets or sets whether the state is compressed.</summary>
	public bool IsCompressed { get; set; }

	/// <summary>Gets or sets when the state was captured.</summary>
	public DateTime CaptureTime { get; set; }
}

/// <summary>
/// Event arguments for greenzone events.
/// </summary>
public sealed class GreenzoneEventArgs : EventArgs {
	/// <summary>Gets the frame number.</summary>
	public int Frame { get; }

	/// <summary>Gets the size in bytes.</summary>
	public int Size { get; }

	public GreenzoneEventArgs(int frame, int size) {
		Frame = frame;
		Size = size;
	}
}

/// <summary>
/// Event arguments for greenzone pruning.
/// </summary>
public sealed class GreenzonePruneEventArgs : EventArgs {
	/// <summary>Gets the number of states pruned.</summary>
	public int PrunedCount { get; }

	/// <summary>Gets the amount of memory freed in bytes.</summary>
	public long FreedMemory { get; }

	public GreenzonePruneEventArgs(int prunedCount, long freedMemory) {
		PrunedCount = prunedCount;
		FreedMemory = freedMemory;
	}
}
