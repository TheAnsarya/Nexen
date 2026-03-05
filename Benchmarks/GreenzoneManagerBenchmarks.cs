using BenchmarkDotNet.Attributes;
using BenchmarkDotNet.Configs;
using BenchmarkDotNet.Order;
using Nexen.TAS;

namespace Nexen.Benchmarks;

/// <summary>
/// Benchmarks for GreenzoneManager operations — state capture, lookup, invalidation,
/// pruning, compression, and ring buffer. Covers scaling from 100 to 10,000 savestates.
/// </summary>
[MemoryDiagnoser]
[Orderer(SummaryOrderPolicy.FastestToSlowest)]
[GroupBenchmarksBy(BenchmarkLogicalGroupRule.ByCategory)]
public class GreenzoneManagerBenchmarks {
	private GreenzoneManager _greenzone = null!;
	private byte[] _stateData = null!;
	private byte[] _largeStateData = null!;

	[Params(100, 1_000, 10_000)]
	public int StateCount { get; set; }

	[GlobalSetup]
	public void Setup() {
		// Typical savestate size: 64KB for NES, 256KB for SNES
		_stateData = new byte[64 * 1024];
		_largeStateData = new byte[256 * 1024];
		var random = new Random(42);
		random.NextBytes(_stateData);
		random.NextBytes(_largeStateData);

		// Pre-populate greenzone with states at every CaptureInterval
		_greenzone = new GreenzoneManager {
			CaptureInterval = 60,
			MaxSavestates = StateCount + 500, // Leave headroom to avoid pruning during setup
			CompressionEnabled = false, // Disable async compression to avoid interference
			RingBufferSize = 120
		};

		for (int i = 0; i < StateCount; i++) {
			int frame = i * 60;
			_greenzone.CaptureState(frame, (byte[])_stateData.Clone(), forceCapture: true);
		}
	}

	[GlobalCleanup]
	public void Cleanup() {
		_greenzone.Dispose();
	}

	#region State Capture

	[Benchmark(Description = "Capture state (new frame)")]
	[BenchmarkCategory("Capture")]
	public void CaptureNewState() {
		int frame = (StateCount + 1) * 60;
		_greenzone.CaptureState(frame, (byte[])_stateData.Clone(), forceCapture: true);
		// Remove it so next iteration works the same
		_greenzone.InvalidateFrom(frame);
	}

	[Benchmark(Description = "Capture state (replace existing)")]
	[BenchmarkCategory("Capture")]
	public void CaptureReplaceState() {
		// Replace the state at frame 0 (always exists)
		_greenzone.CaptureState(0, (byte[])_stateData.Clone(), forceCapture: true);
	}

	#endregion

	#region Nearest State Lookup

	[Benchmark(Description = "GetNearestStateFrame (middle)")]
	[BenchmarkCategory("Lookup")]
	public int LookupMiddle() {
		// Target a frame in the middle that doesn't have an exact state
		int targetFrame = (StateCount / 2) * 60 + 30;
		return _greenzone.GetNearestStateFrame(targetFrame);
	}

	[Benchmark(Description = "GetNearestStateFrame (near end)")]
	[BenchmarkCategory("Lookup")]
	public int LookupNearEnd() {
		int targetFrame = (StateCount - 1) * 60 - 1;
		return _greenzone.GetNearestStateFrame(targetFrame);
	}

	[Benchmark(Description = "GetNearestStateFrame (before start)")]
	[BenchmarkCategory("Lookup")]
	public int LookupBeforeStart() {
		// No state exists before frame 0 — should return -1
		return _greenzone.GetNearestStateFrame(-1);
	}

	[Benchmark(Description = "GetNearestStateFrame (exact match)")]
	[BenchmarkCategory("Lookup")]
	public int LookupExactMatch() {
		int targetFrame = (StateCount / 2) * 60;
		return _greenzone.GetNearestStateFrame(targetFrame);
	}

	#endregion

	#region Property Access

	[Benchmark(Description = "LatestFrame (SortedSet.Max)")]
	[BenchmarkCategory("Properties")]
	public int GetLatestFrame() => _greenzone.LatestFrame;

	[Benchmark(Description = "EarliestFrame (SortedSet.Min)")]
	[BenchmarkCategory("Properties")]
	public int GetEarliestFrame() => _greenzone.EarliestFrame;

	[Benchmark(Description = "SavestateCount")]
	[BenchmarkCategory("Properties")]
	public int GetSavestateCount() => _greenzone.SavestateCount;

	[Benchmark(Description = "TotalMemoryUsage (Interlocked.Read)")]
	[BenchmarkCategory("Properties")]
	public long GetTotalMemoryUsage() => _greenzone.TotalMemoryUsage;

	#endregion

	#region HasState Check

	[Benchmark(Description = "HasState (exists)")]
	[BenchmarkCategory("HasState")]
	public bool HasStateExists() => _greenzone.HasState((StateCount / 2) * 60);

	[Benchmark(Description = "HasState (doesn't exist)")]
	[BenchmarkCategory("HasState")]
	public bool HasStateMissing() => _greenzone.HasState((StateCount / 2) * 60 + 1);

	#endregion

	#region Invalidation

	[Benchmark(Description = "InvalidateFrom (last 10%)")]
	[BenchmarkCategory("Invalidate")]
	public void InvalidateLast10Percent() {
		// Save the count to know how many states to restore
		int startFrame = (int)(StateCount * 0.9) * 60;

		_greenzone.InvalidateFrom(startFrame);

		// Restore invalidated states for next iteration
		for (int i = (int)(StateCount * 0.9); i < StateCount; i++) {
			_greenzone.CaptureState(i * 60, (byte[])_stateData.Clone(), forceCapture: true);
		}
	}

	#endregion

	#region Compression

	[Benchmark(Description = "GZip compress 64KB state")]
	[BenchmarkCategory("Compression")]
	public byte[] Compress64KB() {
		return CompressData(_stateData);
	}

	[Benchmark(Description = "GZip compress 256KB state")]
	[BenchmarkCategory("Compression")]
	public byte[] Compress256KB() {
		return CompressData(_largeStateData);
	}

	[Benchmark(Description = "GZip roundtrip 64KB")]
	[BenchmarkCategory("Compression")]
	public byte[] CompressDecompress64KB() {
		var compressed = CompressData(_stateData);
		return DecompressData(compressed);
	}

	/// <summary>Mirror of GreenzoneManager.CompressData (private).</summary>
	private static byte[] CompressData(byte[] data) {
		using var output = new System.IO.MemoryStream(data.Length * 3 / 4);
		using (var gzip = new System.IO.Compression.GZipStream(output, System.IO.Compression.CompressionLevel.Fastest)) {
			gzip.Write(data, 0, data.Length);
		}
		return output.ToArray();
	}

	/// <summary>Mirror of GreenzoneManager.DecompressData (private).</summary>
	private static byte[] DecompressData(byte[] compressedData) {
		using var input = new System.IO.MemoryStream(compressedData);
		using var gzip = new System.IO.Compression.GZipStream(input, System.IO.Compression.CompressionMode.Decompress);
		using var output = new System.IO.MemoryStream(compressedData.Length * 4);
		gzip.CopyTo(output);
		return output.ToArray();
	}

	#endregion
}
