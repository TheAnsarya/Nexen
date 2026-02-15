using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Linq;
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
	private readonly SemaphoreSlim _compressionSemaphore = new(1, 1);
	private readonly object _ringBufferLock = new();
	private readonly Queue<int> _ringBuffer = new();
	private bool _disposed;

	/// <summary>Gets or sets the interval between automatic greenzone captures.</summary>
	public int CaptureInterval { get; set; } = 60; // Every 60 frames by default

	/// <summary>Gets or sets the maximum number of savestates to keep in memory.</summary>
	public int MaxSavestates { get; set; } = 1000;

	/// <summary>Gets or sets the ring buffer size for recent frames (fast rewind).</summary>
	public int RingBufferSize { get; set; } = 120; // ~2 seconds at 60fps

	/// <summary>Gets or sets whether compression is enabled for older states.</summary>
	public bool CompressionEnabled { get; set; } = true;

	/// <summary>Gets or sets the frame threshold after which states get compressed.</summary>
	public int CompressionThreshold { get; set; } = 300; // 5 seconds

	/// <summary>Gets the current frame with the latest savestate.</summary>
	public int LatestFrame => _savestates.Keys.DefaultIfEmpty(-1).Max();

	/// <summary>Gets the earliest frame with a savestate.</summary>
	public int EarliestFrame => _savestates.Keys.DefaultIfEmpty(-1).Min();

	/// <summary>Gets the total number of savestates.</summary>
	public int SavestateCount => _savestates.Count;

	/// <summary>Gets the total memory usage of all savestates in bytes.</summary>
	public long TotalMemoryUsage => _savestates.Values.Sum(s => s.Data?.LongLength ?? 0);

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
			// Still add to ring buffer for recent frames
			AddToRingBuffer(frame);
			return;
		}

		var savestate = new SavestateData {
			Frame = frame,
			Data = data,
			CaptureTime = DateTime.UtcNow,
			IsCompressed = false
		};

		_savestates[frame] = savestate;
		AddToRingBuffer(frame);

		// Prune if we're over the limit
		if (_savestates.Count > MaxSavestates) {
			PruneSavestates();
		}

		// Compress old states in background
		if (CompressionEnabled) {
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

		// Save to a temporary file and read the data
		string tempPath = Path.Combine(Path.GetTempPath(), $"nexen_greenzone_{frame}.mss");

		try {
			EmuApi.SaveStateFile(tempPath);

			if (File.Exists(tempPath)) {
				byte[] data = File.ReadAllBytes(tempPath);
				CaptureState(frame, data, forceCapture: true);
				File.Delete(tempPath);
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
		var nearestFrame = _savestates.Keys
			.Where(f => f <= frame)
			.DefaultIfEmpty(-1)
			.Max();

		if (nearestFrame < 0 || !_savestates.TryGetValue(nearestFrame, out var nearestState)) {
			return false;
		}

		return LoadSavestateData(nearestState);
	}

	/// <summary>
	/// Gets the nearest frame with a savestate at or before the target frame.
	/// </summary>
	/// <param name="targetFrame">The target frame.</param>
	/// <returns>The nearest frame with a savestate, or -1 if none exists.</returns>
	public int GetNearestStateFrame(int targetFrame) {
		return _savestates.Keys
			.Where(f => f <= targetFrame)
			.DefaultIfEmpty(-1)
			.Max();
	}

	/// <summary>
	/// Seeks to a specific frame, loading the nearest savestate and advancing if needed.
	/// </summary>
	/// <param name="targetFrame">The target frame.</param>
	/// <returns>The actual frame reached after seeking.</returns>
	public async Task<int> SeekToFrameAsync(int targetFrame) {
		if (_disposed || !EmuApi.IsRunning()) {
			return -1;
		}

		int nearestFrame = GetNearestStateFrame(targetFrame);

		if (nearestFrame < 0) {
			return -1;
		}

		// Load the nearest state
		if (!LoadState(nearestFrame)) {
			return -1;
		}

		// If we need to advance, do so
		int framesToAdvance = targetFrame - nearestFrame;

		for (int i = 0; i < framesToAdvance; i++) {
			// Execute a single frame
			EmuApi.ExecuteShortcut(new ExecuteShortcutParams {
				Shortcut = Config.Shortcuts.EmulatorShortcut.RunSingleFrame
			});

			// Small delay to allow frame to complete
			await Task.Delay(1);
		}

		SavestateLoaded?.Invoke(this, new GreenzoneEventArgs(targetFrame, 0));
		return targetFrame;
	}

	/// <summary>
	/// Invalidates all savestates from the specified frame onwards.
	/// Call this when the movie is modified at a frame.
	/// </summary>
	/// <param name="fromFrame">The first frame to invalidate.</param>
	public void InvalidateFrom(int fromFrame) {
		var framesToRemove = _savestates.Keys.Where(f => f >= fromFrame).ToList();

		foreach (var frame in framesToRemove) {
			_savestates.TryRemove(frame, out _);
		}

		// Also clear ring buffer entries
		lock (_ringBufferLock) {
			var newBuffer = new Queue<int>(_ringBuffer.Where(f => f < fromFrame));
			_ringBuffer.Clear();
			foreach (var f in newBuffer) {
				_ringBuffer.Enqueue(f);
			}
		}
	}

	/// <summary>
	/// Clears all savestates.
	/// </summary>
	public void Clear() {
		_savestates.Clear();

		lock (_ringBufferLock) {
			_ringBuffer.Clear();
		}
	}

	/// <summary>
	/// Gets information about all stored savestates.
	/// </summary>
	/// <returns>List of savestate information.</returns>
	public List<SavestateInfo> GetSavestateInfo() {
		return _savestates.Values
			.Select(s => new SavestateInfo {
				Frame = s.Frame,
				Size = s.Data?.Length ?? 0,
				IsCompressed = s.IsCompressed,
				CaptureTime = s.CaptureTime
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
		lock (_ringBufferLock) {
			// Add to ring buffer if within recent range
			if (_ringBuffer.Count >= RingBufferSize) {
				int oldestFrame = _ringBuffer.Dequeue();

				// Don't remove from main storage if it's at an interval
				if (oldestFrame % CaptureInterval != 0) {
					_savestates.TryRemove(oldestFrame, out _);
				}
			}

			_ringBuffer.Enqueue(frame);
		}
	}

	private bool LoadSavestateData(SavestateData state) {
		if (state.Data is null) {
			return false;
		}

		byte[] data = state.IsCompressed ? DecompressData(state.Data) : state.Data;

		// Save to temp file and load
		string tempPath = Path.Combine(Path.GetTempPath(), $"nexen_greenzone_load_{state.Frame}.mss");

		try {
			File.WriteAllBytes(tempPath, data);
			EmuApi.LoadStateFile(tempPath);
			File.Delete(tempPath);

			SavestateLoaded?.Invoke(this, new GreenzoneEventArgs(state.Frame, data.Length));
			return true;
		} catch (Exception ex) {
			System.Diagnostics.Debug.WriteLine($"Failed to load greenzone state at frame {state.Frame}: {ex.Message}");
			return false;
		}
	}

	private void PruneSavestates() {
		// Remove oldest non-interval states first
		var toRemove = _savestates
			.Where(kv => kv.Key % CaptureInterval != 0)
			.OrderBy(kv => kv.Key)
			.Take(_savestates.Count - MaxSavestates + 100) // Leave some headroom
			.Select(kv => kv.Key)
			.ToList();

		int prunedCount = 0;
		long freedMemory = 0;

		foreach (var frame in toRemove) {
			if (_savestates.TryRemove(frame, out var state)) {
				prunedCount++;
				freedMemory += state.Data?.LongLength ?? 0;
			}
		}

		SavestatesPruned?.Invoke(this, new GreenzonePruneEventArgs(prunedCount, freedMemory));
	}

	private async Task CompressOldStatesAsync() {
		await _compressionSemaphore.WaitAsync();

		try {
			var latestFrame = LatestFrame;
			var toCompress = _savestates
				.Where(kv => !kv.Value.IsCompressed && latestFrame - kv.Key > CompressionThreshold)
				.ToList();

			foreach (var kv in toCompress) {
				if (kv.Value.Data is not null && !kv.Value.IsCompressed) {
					var compressed = CompressData(kv.Value.Data);

					// Only use compressed if it's actually smaller
					if (compressed.Length < kv.Value.Data.Length) {
						kv.Value.Data = compressed;
						kv.Value.IsCompressed = true;
					}
				}
			}
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
		_savestates.Clear();
		_compressionSemaphore.Dispose();
	}
}

/// <summary>
/// Internal class to store savestate data.
/// </summary>
internal sealed class SavestateData {
	public int Frame { get; set; }
	public byte[]? Data { get; set; }
	public DateTime CaptureTime { get; set; }
	public bool IsCompressed { get; set; }
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
