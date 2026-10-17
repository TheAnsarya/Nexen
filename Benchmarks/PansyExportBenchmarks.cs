using System.Buffers;
using System.IO.Compression;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using BenchmarkDotNet.Attributes;
using BenchmarkDotNet.Configs;
using BenchmarkDotNet.Order;

namespace Nexen.Benchmarks;

/// <summary>
/// Benchmarks for Pansy export operations.
/// Goal: Increase performance without interrupting emulation.
/// </summary>
[MemoryDiagnoser]
[Orderer(SummaryOrderPolicy.FastestToSlowest)]
[GroupBenchmarksBy(BenchmarkLogicalGroupRule.ByCategory)]
public class PansyExportBenchmarks {
	// Test data sizes
	private byte[] _smallCdlData = null!;   // 32KB - typical small ROM
	private byte[] _mediumCdlData = null!;   // 512KB - typical NES/SNES
	private byte[] _largeCdlData = null!;   // 4MB - large SNES/GBA ROM
	private byte[] _sparseJumpCdlData = null!; // 4MB - sparse jump graph
	private byte[] _denseJumpCdlData = null!;  // 4MB - dense jump graph

	// SNES CDL data with IndexMode8/MemoryMode8 flags in upper nibble
	private byte[] _snesMediumCdlData = null!;  // 512KB - SNES with mode flags
	private byte[] _snesLargeCdlData = null!;   // 4MB - large SNES ROM

	// GBA CDL data with Thumb flag in upper nibble
	private byte[] _gbaMediumCdlData = null!;   // 512KB - GBA with THUMB flags

	private List<TestLabel> _smallLabels = null!;   // 100 labels
	private List<TestLabel> _mediumLabels = null!;  // 1000 labels
	private List<TestLabel> _largeLabels = null!;   // 10000 labels

	// Pre-allocated buffers for optimized versions
	private byte[] _reusableBuffer = null!;

	[GlobalSetup]
	public void Setup() {
		var random = new Random(42); // Deterministic seed

		// Generate CDL data with realistic patterns (code/data/jump targets)
		_smallCdlData = GenerateCdlData(32 * 1024, random);
		_mediumCdlData = GenerateCdlData(512 * 1024, random);
		_largeCdlData = GenerateCdlData(4 * 1024 * 1024, random);
		_sparseJumpCdlData = GenerateJumpGraphCdlData(4 * 1024 * 1024, jumpStride: 128);
		_denseJumpCdlData = GenerateJumpGraphCdlData(4 * 1024 * 1024, jumpStride: 8);

		// Generate SNES CDL data with platform-specific upper nibble flags
		_snesMediumCdlData = GenerateSnesCdlData(512 * 1024, random);
		_snesLargeCdlData = GenerateSnesCdlData(4 * 1024 * 1024, random);

		// Generate GBA CDL data with Thumb flag
		_gbaMediumCdlData = GenerateGbaCdlData(512 * 1024, random);

		// Generate test labels
		_smallLabels = GenerateLabels(100, random);
		_mediumLabels = GenerateLabels(1000, random);
		_largeLabels = GenerateLabels(10000, random);

		// Pre-allocate reusable buffer (largest needed)
		_reusableBuffer = new byte[8 * 1024 * 1024]; // 8MB
	}

	private static byte[] GenerateCdlData(int size, Random random) {
		var data = new byte[size];
		int i = 0;
		while (i < size) {
			// Simulate code regions (60% of ROM)
			if (random.NextDouble() < 0.6) {
				int codeLength = random.Next(4, 128);
				for (int j = 0; j < codeLength && i + j < size; j++) {
					data[i + j] = 0x01; // Code flag
					if (random.NextDouble() < 0.1) {
						data[i + j] |= 0x04; // Jump target
					}

					if (random.NextDouble() < 0.05) {
						data[i + j] |= 0x08; // Sub entry point
					}
				}

				i += codeLength;
			} else {
				// Data regions (40%)
				int dataLength = random.Next(8, 256);
				for (int j = 0; j < dataLength && i + j < size; j++) {
					data[i + j] = 0x02; // Data flag
				}

				i += dataLength;
			}
		}

		return data;
	}

	private static List<TestLabel> GenerateLabels(int count, Random random) {
		var labels = new List<TestLabel>(count);
		for (int i = 0; i < count; i++) {
			labels.Add(new TestLabel {
				Address = random.Next(0, 0x10000),
				Label = $"label_{i:X4}",
				Comment = random.NextDouble() < 0.3 ? $"Comment for label {i}" : null,
				Length = random.Next(1, 32)
			});
		}

		return labels;
	}

	/// <summary>
	/// Generates deterministic CDL for sparse/dense jump-graph benchmarking.
	/// </summary>
	private static byte[] GenerateJumpGraphCdlData(int size, int jumpStride) {
		var data = new byte[size];
		for (int i = 0; i < size; i++) {
			byte flags = (i % 4 == 0) ? (byte)0x01 : (byte)0x02;
			if (i % jumpStride == 0) {
				flags |= 0x04;
			}

			data[i] = flags;
		}

		return data;
	}

	/// <summary>
	/// Generates SNES CDL data with IndexMode8 (0x10) and MemoryMode8 (0x20) flags.
	/// ~40% of code bytes have mode flags set (realistic for 65816 instructions).
	/// </summary>
	private static byte[] GenerateSnesCdlData(int size, Random random) {
		var data = new byte[size];
		int i = 0;
		while (i < size) {
			if (random.NextDouble() < 0.6) {
				int codeLength = random.Next(4, 128);
				for (int j = 0; j < codeLength && i + j < size; j++) {
					data[i + j] = 0x01; // Code flag
					// ~40% of code bytes have SNES mode flags
					if (random.NextDouble() < 0.25) data[i + j] |= 0x10; // IndexMode8
					if (random.NextDouble() < 0.25) data[i + j] |= 0x20; // MemoryMode8
					if (random.NextDouble() < 0.1) data[i + j] |= 0x04;  // Jump target
				}

				i += codeLength;
			} else {
				int dataLength = random.Next(8, 256);
				for (int j = 0; j < dataLength && i + j < size; j++) {
					data[i + j] = 0x02; // Data flag
				}

				i += dataLength;
			}
		}

		return data;
	}

	/// <summary>
	/// Generates GBA CDL data with Thumb (0x20) flag.
	/// ~60% of code bytes are in THUMB mode (realistic for mixed ARM/THUMB code).
	/// </summary>
	private static byte[] GenerateGbaCdlData(int size, Random random) {
		var data = new byte[size];
		int i = 0;
		while (i < size) {
			if (random.NextDouble() < 0.6) {
				int codeLength = random.Next(4, 128);
				bool isThumbRegion = random.NextDouble() < 0.6; // 60% THUMB
				for (int j = 0; j < codeLength && i + j < size; j++) {
					data[i + j] = 0x01; // Code flag
					if (isThumbRegion) data[i + j] |= 0x20; // THUMB
					if (random.NextDouble() < 0.1) data[i + j] |= 0x04; // Jump target
				}

				i += codeLength;
			} else {
				int dataLength = random.Next(8, 256);
				for (int j = 0; j < dataLength && i + j < size; j++) {
					data[i + j] = 0x02; // Data flag
				}

				i += dataLength;
			}
		}

		return data;
	}

	#region CDL Data Conversion Benchmarks

	/// <summary>
	/// Original: Loop-based CdlFlags[] to byte[] conversion
	/// </summary>
	[Benchmark(Baseline = true)]
	[BenchmarkCategory("CdlConversion")]
	public byte[] CdlConversion_Original_Medium() {
		// Simulates: for (int i = 0; i < cdlFlags.Length; i++) result[i] = (byte)cdlFlags[i];
		var result = new byte[_mediumCdlData.Length];
		for (int i = 0; i < _mediumCdlData.Length; i++) {
			result[i] = _mediumCdlData[i];
		}

		return result;
	}

	/// <summary>
	/// Optimized: Memory<T>.Span.CopyTo (SIMD)
	/// </summary>
	[Benchmark]
	[BenchmarkCategory("CdlConversion")]
	public byte[] CdlConversion_SpanCopy_Medium() {
		var result = new byte[_mediumCdlData.Length];
		_mediumCdlData.AsSpan().CopyTo(result);
		return result;
	}

	/// <summary>
	/// Optimized: Buffer.BlockCopy (native memcpy)
	/// </summary>
	[Benchmark]
	[BenchmarkCategory("CdlConversion")]
	public byte[] CdlConversion_BlockCopy_Medium() {
		var result = new byte[_mediumCdlData.Length];
		Buffer.BlockCopy(_mediumCdlData, 0, result, 0, _mediumCdlData.Length);
		return result;
	}

	/// <summary>
	/// Optimized: ArrayPool to avoid allocation
	/// </summary>
	[Benchmark]
	[BenchmarkCategory("CdlConversion")]
	public int CdlConversion_ArrayPool_Medium() {
		var result = ArrayPool<byte>.Shared.Rent(_mediumCdlData.Length);
		try {
			_mediumCdlData.AsSpan().CopyTo(result);
			return result.Length; // Return something to prevent optimization
		} finally {
			ArrayPool<byte>.Shared.Return(result);
		}
	}

	#endregion

	#region Jump Target Extraction Benchmarks

	/// <summary>
	/// Original: Linear scan with list allocation
	/// </summary>
	[Benchmark(Baseline = true)]
	[BenchmarkCategory("JumpTargets")]
	public List<uint> JumpTargets_Original_Medium() {
		var targets = new List<uint>();
		for (int i = 0; i < _mediumCdlData.Length; i++) {
			if ((_mediumCdlData[i] & 0x04) != 0) { // Jump target flag
				targets.Add((uint)i);
			}
		}

		return targets;
	}

	/// <summary>
	/// Optimized: Pre-sized list to avoid resizing
	/// </summary>
	[Benchmark]
	[BenchmarkCategory("JumpTargets")]
	public List<uint> JumpTargets_Presized_Medium() {
		// Estimate ~10% are jump targets
		var targets = new List<uint>(_mediumCdlData.Length / 10);
		for (int i = 0; i < _mediumCdlData.Length; i++) {
			if ((_mediumCdlData[i] & 0x04) != 0) {
				targets.Add((uint)i);
			}
		}

		return targets;
	}

	/// <summary>
	/// Optimized: Span-based with pre-count
	/// </summary>
	[Benchmark]
	[BenchmarkCategory("JumpTargets")]
	public uint[] JumpTargets_SpanPrecount_Medium() {
		ReadOnlySpan<byte> span = _mediumCdlData;

		// First pass: count
		int count = 0;
		foreach (byte b in span) {
			if ((b & 0x04) != 0) count++;
		}

		// Second pass: extract
		var result = new uint[count];
		int idx = 0;
		for (int i = 0; i < span.Length; i++) {
			if ((span[i] & 0x04) != 0) {
				result[idx++] = (uint)i;
			}
		}

		return result;
	}

	/// <summary>
	/// Optimized: SIMD-accelerated extraction using Vector<byte>
	/// </summary>
	[Benchmark]
	[BenchmarkCategory("JumpTargets")]
	public uint[] JumpTargets_Simd_Medium() {
		return ExtractFlagsSimd(_mediumCdlData, 0x04);
	}

	[Benchmark]
	[BenchmarkCategory("JumpTargets", "SparseDense")]
	public List<uint> JumpTargets_Original_SparseLarge() {
		return ExtractJumpTargetsOriginal(_sparseJumpCdlData);
	}

	[Benchmark]
	[BenchmarkCategory("JumpTargets", "SparseDense")]
	public uint[] JumpTargets_Optimized_SparseLarge() {
		return ExtractJumpTargetsOptimized(_sparseJumpCdlData);
	}

	[Benchmark]
	[BenchmarkCategory("JumpTargets", "SparseDense")]
	public List<uint> JumpTargets_Original_DenseLarge() {
		return ExtractJumpTargetsOriginal(_denseJumpCdlData);
	}

	[Benchmark]
	[BenchmarkCategory("JumpTargets", "SparseDense")]
	public uint[] JumpTargets_Optimized_DenseLarge() {
		return ExtractJumpTargetsOptimized(_denseJumpCdlData);
	}

	private static uint[] ExtractFlagsSimd(byte[] data, byte flag) {
		// Use vectorized counting first
		ReadOnlySpan<byte> span = data;
		int count = 0;

		// Fallback counting (SIMD would add complexity)
		foreach (byte b in span) {
			if ((b & flag) != 0) count++;
		}

		// Extract indices
		var result = new uint[count];
		int idx = 0;
		for (int i = 0; i < data.Length; i++) {
			if ((data[i] & flag) != 0) {
				result[idx++] = (uint)i;
			}
		}

		return result;
	}

	#endregion

	#region Compression Benchmarks

	/// <summary>
	/// Original: GZip with CompressionLevel.Optimal
	/// </summary>
	[Benchmark(Baseline = true)]
	[BenchmarkCategory("Compression")]
	public byte[] Compression_GzipOptimal_Medium() {
		using var output = new MemoryStream();
		using (var gzip = new GZipStream(output, CompressionLevel.Optimal)) {
			gzip.Write(_mediumCdlData, 0, _mediumCdlData.Length);
		}

		return output.ToArray();
	}

	/// <summary>
	/// Optimized: GZip with CompressionLevel.Fastest
	/// </summary>
	[Benchmark]
	[BenchmarkCategory("Compression")]
	public byte[] Compression_GzipFastest_Medium() {
		using var output = new MemoryStream();
		using (var gzip = new GZipStream(output, CompressionLevel.Fastest)) {
			gzip.Write(_mediumCdlData, 0, _mediumCdlData.Length);
		}

		return output.ToArray();
	}

	/// <summary>
	/// Optimized: Deflate with CompressionLevel.Fastest (less overhead than GZip)
	/// </summary>
	[Benchmark]
	[BenchmarkCategory("Compression")]
	public byte[] Compression_DeflateFastest_Medium() {
		using var output = new MemoryStream();
		using (var deflate = new DeflateStream(output, CompressionLevel.Fastest)) {
			deflate.Write(_mediumCdlData, 0, _mediumCdlData.Length);
		}

		return output.ToArray();
	}

	/// <summary>
	/// Optimized: Brotli with quality=1 (fastest)
	/// </summary>
	[Benchmark]
	[BenchmarkCategory("Compression")]
	public byte[] Compression_BrotliFastest_Medium() {
		using var output = new MemoryStream();
		using (var brotli = new BrotliStream(output, CompressionLevel.Fastest)) {
			brotli.Write(_mediumCdlData, 0, _mediumCdlData.Length);
		}

		return output.ToArray();
	}

	/// <summary>
	/// Optimized: ZLib with Fastest
	/// </summary>
	[Benchmark]
	[BenchmarkCategory("Compression")]
	public byte[] Compression_ZLibFastest_Medium() {
		using var output = new MemoryStream();
		using (var zlib = new ZLibStream(output, CompressionLevel.Fastest)) {
			zlib.Write(_mediumCdlData, 0, _mediumCdlData.Length);
		}

		return output.ToArray();
	}

	/// <summary>
	/// Large data: GZip Optimal vs Fastest comparison
	/// </summary>
	[Benchmark]
	[BenchmarkCategory("Compression", "Large")]
	public byte[] Compression_GzipOptimal_Large() {
		using var output = new MemoryStream();
		using (var gzip = new GZipStream(output, CompressionLevel.Optimal)) {
			gzip.Write(_largeCdlData, 0, _largeCdlData.Length);
		}

		return output.ToArray();
	}

	[Benchmark]
	[BenchmarkCategory("Compression", "Large")]
	public byte[] Compression_GzipFastest_Large() {
		using var output = new MemoryStream();
		using (var gzip = new GZipStream(output, CompressionLevel.Fastest)) {
			gzip.Write(_largeCdlData, 0, _largeCdlData.Length);
		}

		return output.ToArray();
	}

	#endregion

	#region Symbol Section Building Benchmarks

	/// <summary>
	/// Original: LINQ Where().ToList() + MemoryStream allocation
	/// </summary>
	[Benchmark(Baseline = true)]
	[BenchmarkCategory("SymbolSection")]
	public byte[] SymbolSection_Original_Medium() {
		// Simulates: labels.Where(l => !string.IsNullOrEmpty(l.Label)).ToList();
		var filtered = _mediumLabels.Where(l => !string.IsNullOrEmpty(l.Label)).ToList();

		using var ms = new MemoryStream();
		using var writer = new BinaryWriter(ms);

		writer.Write((uint)filtered.Count);
		foreach (var label in filtered) {
			writer.Write((uint)label.Address);
			writer.Write((byte)1); // MemoryType placeholder
			writer.Write((byte)1); // SymbolType placeholder
			writer.Write((ushort)0); // Flags

			byte[] nameBytes = Encoding.UTF8.GetBytes(label.Label ?? "");
			writer.Write((ushort)nameBytes.Length);
			writer.Write(nameBytes);
		}

		return ms.ToArray();
	}

	/// <summary>
	/// Optimized: No LINQ, pre-sized MemoryStream
	/// </summary>
	[Benchmark]
	[BenchmarkCategory("SymbolSection")]
	public byte[] SymbolSection_NoLinq_Medium() {
		// Count first without allocating intermediate list
		int count = 0;
		foreach (var label in _mediumLabels) {
			if (!string.IsNullOrEmpty(label.Label)) count++;
		}

		// Estimate size: 4 (count) + count * (4 + 1 + 1 + 2 + 2 + ~12 avg name) ≈ count * 22 + 4
		using var ms = new MemoryStream((count * 22) + 4);
		using var writer = new BinaryWriter(ms);

		writer.Write((uint)count);
		foreach (var label in _mediumLabels) {
			if (string.IsNullOrEmpty(label.Label)) continue;

			writer.Write((uint)label.Address);
			writer.Write((byte)1);
			writer.Write((byte)1);
			writer.Write((ushort)0);

			byte[] nameBytes = Encoding.UTF8.GetBytes(label.Label);
			writer.Write((ushort)nameBytes.Length);
			writer.Write(nameBytes);
		}

		return ms.ToArray();
	}

	/// <summary>
	/// Optimized: ArrayBufferWriter for zero-copy building
	/// </summary>
	[Benchmark]
	[BenchmarkCategory("SymbolSection")]
	public byte[] SymbolSection_BufferWriter_Medium() {
		int count = 0;
		foreach (var label in _mediumLabels) {
			if (!string.IsNullOrEmpty(label.Label)) count++;
		}

		var buffer = new ArrayBufferWriter<byte>((count * 22) + 4);

		// Write count
		WriteUInt32(buffer, (uint)count);

		foreach (var label in _mediumLabels) {
			if (string.IsNullOrEmpty(label.Label)) continue;

			WriteUInt32(buffer, (uint)label.Address);
			buffer.GetSpan(4)[0] = 1; // MemoryType
			buffer.Advance(1);
			buffer.GetSpan(1)[0] = 1; // SymbolType
			buffer.Advance(1);
			WriteUInt16(buffer, 0); // Flags

			int nameLength = Encoding.UTF8.GetByteCount(label.Label);
			WriteUInt16(buffer, (ushort)nameLength);
			var nameSpan = buffer.GetSpan(nameLength);
			Encoding.UTF8.GetBytes(label.Label, nameSpan);
			buffer.Advance(nameLength);
		}

		return buffer.WrittenSpan.ToArray();
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	private static void WriteUInt32(ArrayBufferWriter<byte> buffer, uint value) {
		var span = buffer.GetSpan(4);
		System.Buffers.Binary.BinaryPrimitives.WriteUInt32LittleEndian(span, value);
		buffer.Advance(4);
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	private static void WriteUInt16(ArrayBufferWriter<byte> buffer, ushort value) {
		var span = buffer.GetSpan(2);
		System.Buffers.Binary.BinaryPrimitives.WriteUInt16LittleEndian(span, value);
		buffer.Advance(2);
	}

	#endregion

	#region Data Block Detection Benchmarks

	/// <summary>
	/// Original: Sequential scan with LINQ filter
	/// </summary>
	[Benchmark(Baseline = true)]
	[BenchmarkCategory("DataBlocks")]
	public List<(uint Start, uint End)> DataBlocks_Original_Medium() {
		var blocks = new List<(uint Start, uint End)>();
		int? blockStart = null;

		for (int i = 0; i < _mediumCdlData.Length; i++) {
			bool isData = (_mediumCdlData[i] & 0x02) != 0 && (_mediumCdlData[i] & 0x01) == 0;

			if (isData && blockStart == null) {
				blockStart = i;
			} else if (!isData && blockStart != null) {
				blocks.Add(((uint)blockStart.Value, (uint)(i - 1)));
				blockStart = null;
			}
		}

		if (blockStart != null) {
			blocks.Add(((uint)blockStart.Value, (uint)(_mediumCdlData.Length - 1)));
		}

		// LINQ filter
		return blocks.Where(b => b.End - b.Start >= 4).ToList();
	}

	/// <summary>
	/// Optimized: Inline filtering, no LINQ
	/// </summary>
	[Benchmark]
	[BenchmarkCategory("DataBlocks")]
	public List<(uint Start, uint End)> DataBlocks_NoLinq_Medium() {
		var blocks = new List<(uint Start, uint End)>(128); // Pre-size estimate
		int? blockStart = null;

		for (int i = 0; i < _mediumCdlData.Length; i++) {
			bool isData = (_mediumCdlData[i] & 0x02) != 0 && (_mediumCdlData[i] & 0x01) == 0;

			if (isData && blockStart == null) {
				blockStart = i;
			} else if (!isData && blockStart != null) {
				// Filter inline: only add if size >= 4
				if (i - 1 - blockStart.Value >= 4) {
					blocks.Add(((uint)blockStart.Value, (uint)(i - 1)));
				}

				blockStart = null;
			}
		}

		if (blockStart != null && _mediumCdlData.Length - 1 - blockStart.Value >= 4) {
			blocks.Add(((uint)blockStart.Value, (uint)(_mediumCdlData.Length - 1)));
		}

		return blocks;
	}

	/// <summary>
	/// Optimized: Span-based with early continue
	/// </summary>
	[Benchmark]
	[BenchmarkCategory("DataBlocks")]
	public List<(uint Start, uint End)> DataBlocks_Span_Medium() {
		ReadOnlySpan<byte> span = _mediumCdlData;
		var blocks = new List<(uint Start, uint End)>(128);

		int i = 0;
		while (i < span.Length) {
			byte flags = span[i];
			bool isData = (flags & 0x02) != 0 && (flags & 0x01) == 0;

			if (isData) {
				int start = i;
				// Scan forward to find end of data block
				while (i < span.Length) {
					flags = span[i];
					if ((flags & 0x02) == 0 || (flags & 0x01) != 0) break;
					i++;
				}
				// Only add significant blocks
				if (i - 1 - start >= 4) {
					blocks.Add(((uint)start, (uint)(i - 1)));
				}
			} else {
				i++;
			}
		}

		return blocks;
	}

	#endregion

	#region CPU State Section Benchmarks

	/// <summary>
	/// Baseline: Two-pass scan (count then build) with BinaryWriter — matches PansyExporter.BuildCpuStateSection.
	/// Tests SNES CDL (512KB) where code bytes have IndexMode8/MemoryMode8 flags.
	/// </summary>
	[Benchmark(Baseline = true)]
	[BenchmarkCategory("CpuState")]
	public byte[] CpuState_TwoPass_SnesMedium() {
		return BuildCpuStateTwoPass(_snesMediumCdlData, isSnes: true);
	}

	/// <summary>
	/// Single-pass: Write entries directly without pre-counting.
	/// Trades initial MemoryStream capacity guess for removal of redundant scan.
	/// </summary>
	[Benchmark]
	[BenchmarkCategory("CpuState")]
	public byte[] CpuState_SinglePass_SnesMedium() {
		return BuildCpuStateSinglePass(_snesMediumCdlData, isSnes: true);
	}

	/// <summary>
	/// Span-based: Use Span + MemoryMarshal for writing entries.
	/// Avoids BinaryWriter overhead on each Write call.
	/// </summary>
	[Benchmark]
	[BenchmarkCategory("CpuState")]
	public byte[] CpuState_SpanWrite_SnesMedium() {
		return BuildCpuStateSpanWrite(_snesMediumCdlData, isSnes: true);
	}

	/// <summary>
	/// Two-pass baseline on large SNES ROM (4MB).
	/// </summary>
	[Benchmark(Baseline = true)]
	[BenchmarkCategory("CpuStateLarge")]
	public byte[] CpuState_TwoPass_SnesLarge() {
		return BuildCpuStateTwoPass(_snesLargeCdlData, isSnes: true);
	}

	/// <summary>
	/// Span-based on large SNES ROM (4MB).
	/// </summary>
	[Benchmark]
	[BenchmarkCategory("CpuStateLarge")]
	public byte[] CpuState_SpanWrite_SnesLarge() {
		return BuildCpuStateSpanWrite(_snesLargeCdlData, isSnes: true);
	}

	/// <summary>
	/// Two-pass baseline on GBA ROM (512KB) with Thumb flag.
	/// </summary>
	[Benchmark(Baseline = true)]
	[BenchmarkCategory("CpuStateGba")]
	public byte[] CpuState_TwoPass_GbaMedium() {
		return BuildCpuStateTwoPass(_gbaMediumCdlData, isSnes: false);
	}

	/// <summary>
	/// Span-based on GBA ROM (512KB) with Thumb flag.
	/// </summary>
	[Benchmark]
	[BenchmarkCategory("CpuStateGba")]
	public byte[] CpuState_SpanWrite_GbaMedium() {
		return BuildCpuStateSpanWrite(_gbaMediumCdlData, isSnes: false);
	}

	// --- CPU State implementation variants ---

	private const byte CDL_CODE = 0x01;
	private const byte CDL_SNES_INDEX_8 = 0x10;
	private const byte CDL_SNES_MEMORY_8 = 0x20;
	private const byte CDL_GBA_THUMB = 0x20;

	/// <summary>
	/// Two-pass: Count entries first, then build (matches production code).
	/// </summary>
	private static byte[] BuildCpuStateTwoPass(byte[] cdlData, bool isSnes) {
		int count = 0;
		for (int i = 0; i < cdlData.Length; i++) {
			byte cdl = cdlData[i];
			if ((cdl & CDL_CODE) == 0) continue;
			if (isSnes && (cdl & (CDL_SNES_INDEX_8 | CDL_SNES_MEMORY_8)) != 0) count++;
			else if (!isSnes && (cdl & CDL_GBA_THUMB) != 0) count++;
		}

		if (count == 0) return [];

		using var ms = new MemoryStream(count * 9);
		using var writer = new BinaryWriter(ms);
		for (int i = 0; i < cdlData.Length; i++) {
			byte cdl = cdlData[i];
			if ((cdl & CDL_CODE) == 0) continue;

			if (isSnes) {
				bool x8 = (cdl & CDL_SNES_INDEX_8) != 0;
				bool m8 = (cdl & CDL_SNES_MEMORY_8) != 0;
				if (!x8 && !m8) continue;
				writer.Write((uint)i);
				byte flags = 0;
				if (x8) flags |= 0x01;
				if (m8) flags |= 0x02;
				writer.Write(flags);
				writer.Write((byte)0);
				writer.Write((ushort)0);
				writer.Write((byte)0);
			} else {
				if ((cdl & CDL_GBA_THUMB) == 0) continue;
				writer.Write((uint)i);
				writer.Write((byte)0);
				writer.Write((byte)0);
				writer.Write((ushort)0);
				writer.Write((byte)3);
			}
		}

		return ms.ToArray();
	}

	/// <summary>
	/// Single-pass: Skip pre-count, use estimated capacity.
	/// </summary>
	private static byte[] BuildCpuStateSinglePass(byte[] cdlData, bool isSnes) {
		// Estimate ~20% of code bytes have mode flags
		using var ms = new MemoryStream(cdlData.Length / 5 * 9);
		using var writer = new BinaryWriter(ms);

		for (int i = 0; i < cdlData.Length; i++) {
			byte cdl = cdlData[i];
			if ((cdl & CDL_CODE) == 0) continue;

			if (isSnes) {
				bool x8 = (cdl & CDL_SNES_INDEX_8) != 0;
				bool m8 = (cdl & CDL_SNES_MEMORY_8) != 0;
				if (!x8 && !m8) continue;
				writer.Write((uint)i);
				byte flags = 0;
				if (x8) flags |= 0x01;
				if (m8) flags |= 0x02;
				writer.Write(flags);
				writer.Write((byte)0);
				writer.Write((ushort)0);
				writer.Write((byte)0);
			} else {
				if ((cdl & CDL_GBA_THUMB) == 0) continue;
				writer.Write((uint)i);
				writer.Write((byte)0);
				writer.Write((byte)0);
				writer.Write((ushort)0);
				writer.Write((byte)3);
			}
		}

		return ms.ToArray();
	}

	/// <summary>
	/// Span-based: Write 9-byte entries directly using MemoryMarshal.
	/// Avoids per-field BinaryWriter overhead.
	/// </summary>
	private static byte[] BuildCpuStateSpanWrite(byte[] cdlData, bool isSnes) {
		// Count first (fast scan)
		int count = 0;
		for (int i = 0; i < cdlData.Length; i++) {
			byte cdl = cdlData[i];
			if ((cdl & CDL_CODE) == 0) continue;
			if (isSnes && (cdl & (CDL_SNES_INDEX_8 | CDL_SNES_MEMORY_8)) != 0) count++;
			else if (!isSnes && (cdl & CDL_GBA_THUMB) != 0) count++;
		}

		if (count == 0) return [];

		var result = new byte[count * 9];
		int offset = 0;

		for (int i = 0; i < cdlData.Length; i++) {
			byte cdl = cdlData[i];
			if ((cdl & CDL_CODE) == 0) continue;

			if (isSnes) {
				bool x8 = (cdl & CDL_SNES_INDEX_8) != 0;
				bool m8 = (cdl & CDL_SNES_MEMORY_8) != 0;
				if (!x8 && !m8) continue;
				BitConverter.TryWriteBytes(result.AsSpan(offset), (uint)i);
				byte flags = 0;
				if (x8) flags |= 0x01;
				if (m8) flags |= 0x02;
				result[offset + 4] = flags;
				result[offset + 5] = 0; // DataBank
				result[offset + 6] = 0; // DirectPage low
				result[offset + 7] = 0; // DirectPage high
				result[offset + 8] = 0; // CpuMode: Native65816
			} else {
				if ((cdl & CDL_GBA_THUMB) == 0) continue;
				BitConverter.TryWriteBytes(result.AsSpan(offset), (uint)i);
				result[offset + 4] = 0; // Flags
				result[offset + 5] = 0; // DataBank
				result[offset + 6] = 0; // DirectPage low
				result[offset + 7] = 0; // DirectPage high
				result[offset + 8] = 3; // CpuMode: THUMB
			}

			offset += 9;
		}

		return result;
	}

	#endregion

	#region Full Export Simulation Benchmarks

	/// <summary>
	/// Simulates full export with original patterns
	/// </summary>
	[Benchmark(Baseline = true)]
	[BenchmarkCategory("FullExport")]
	public byte[] FullExport_Original_Medium() {
		// Build all sections
		var symbols = BuildSymbolSectionOriginal(_mediumLabels);
		var cdl = _mediumCdlData.ToArray(); // Copy
		var jumpTargets = ExtractJumpTargetsOriginal(_mediumCdlData);

		// Compress sections
		var compressedSymbols = CompressOriginal(symbols);
		var compressedCdl = CompressOriginal(cdl);

		// Build final output
		using var ms = new MemoryStream();
		using var writer = new BinaryWriter(ms);

		writer.Write(System.Text.Encoding.ASCII.GetBytes("PNSY"));
		writer.Write((ushort)1);
		writer.Write(compressedSymbols);
		writer.Write(compressedCdl);
		writer.Write(jumpTargets.Count);
		foreach (var target in jumpTargets) writer.Write(target);

		return ms.ToArray();
	}

	/// <summary>
	/// Simulates full export with optimized patterns
	/// </summary>
	[Benchmark]
	[BenchmarkCategory("FullExport")]
	public byte[] FullExport_Optimized_Medium() {
		// Build all sections with optimizations
		var symbols = BuildSymbolSectionOptimized(_mediumLabels);
		var cdl = ArrayPool<byte>.Shared.Rent(_mediumCdlData.Length);
		try {
			_mediumCdlData.AsSpan().CopyTo(cdl);
			var jumpTargets = ExtractJumpTargetsOptimized(_mediumCdlData);

			// Compress with Fastest level
			var compressedSymbols = CompressFastest(symbols);
			var compressedCdl = CompressFastest(cdl.AsSpan(0, _mediumCdlData.Length));

			// Build final output
			using var ms = new MemoryStream(compressedSymbols.Length + compressedCdl.Length + 1024);
			using var writer = new BinaryWriter(ms);

			writer.Write(System.Text.Encoding.ASCII.GetBytes("PNSY"));
			writer.Write((ushort)1);
			writer.Write(compressedSymbols);
			writer.Write(compressedCdl);
			writer.Write(jumpTargets.Length);
			foreach (var target in jumpTargets) writer.Write(target);

			return ms.ToArray();
		} finally {
			ArrayPool<byte>.Shared.Return(cdl);
		}
	}

	/// <summary>
	/// Simulates full export with CPU State section added (SNES).
	/// Shows the overhead of adding CPU State to the export pipeline.
	/// </summary>
	[Benchmark]
	[BenchmarkCategory("FullExport")]
	public byte[] FullExport_WithCpuState_Medium() {
		var symbols = BuildSymbolSectionOptimized(_mediumLabels);
		var cdl = ArrayPool<byte>.Shared.Rent(_snesMediumCdlData.Length);
		try {
			_snesMediumCdlData.AsSpan().CopyTo(cdl);
			var jumpTargets = ExtractJumpTargetsOptimized(_snesMediumCdlData);
			var cpuState = BuildCpuStateSpanWrite(_snesMediumCdlData, isSnes: true);

			var compressedSymbols = CompressFastest(symbols);
			var compressedCdl = CompressFastest(cdl.AsSpan(0, _snesMediumCdlData.Length));

			using var ms = new MemoryStream(compressedSymbols.Length + compressedCdl.Length + cpuState.Length + 1024);
			using var writer = new BinaryWriter(ms);

			writer.Write(System.Text.Encoding.ASCII.GetBytes("PNSY"));
			writer.Write((ushort)1);
			writer.Write(compressedSymbols);
			writer.Write(compressedCdl);
			writer.Write(jumpTargets.Length);
			foreach (var target in jumpTargets) writer.Write(target);
			writer.Write(cpuState);

			return ms.ToArray();
		} finally {
			ArrayPool<byte>.Shared.Return(cdl);
		}
	}

	private static byte[] BuildSymbolSectionOriginal(List<TestLabel> labels) {
		var filtered = labels.Where(l => !string.IsNullOrEmpty(l.Label)).ToList();
		using var ms = new MemoryStream();
		using var writer = new BinaryWriter(ms);
		writer.Write((uint)filtered.Count);
		foreach (var label in filtered) {
			writer.Write((uint)label.Address);
			byte[] nameBytes = Encoding.UTF8.GetBytes(label.Label ?? "");
			writer.Write((ushort)nameBytes.Length);
			writer.Write(nameBytes);
		}

		return ms.ToArray();
	}

	private static byte[] BuildSymbolSectionOptimized(List<TestLabel> labels) {
		int count = 0;
		foreach (var l in labels) if (!string.IsNullOrEmpty(l.Label)) count++;

		using var ms = new MemoryStream((count * 20) + 4);
		using var writer = new BinaryWriter(ms);
		writer.Write((uint)count);
		foreach (var label in labels) {
			if (string.IsNullOrEmpty(label.Label)) continue;
			writer.Write((uint)label.Address);
			byte[] nameBytes = Encoding.UTF8.GetBytes(label.Label);
			writer.Write((ushort)nameBytes.Length);
			writer.Write(nameBytes);
		}

		return ms.ToArray();
	}

	private static List<uint> ExtractJumpTargetsOriginal(byte[] cdl) {
		var targets = new List<uint>();
		for (int i = 0; i < cdl.Length; i++) {
			if ((cdl[i] & 0x04) != 0) targets.Add((uint)i);
		}

		return targets;
	}

	private static uint[] ExtractJumpTargetsOptimized(byte[] cdl) {
		int count = 0;
		foreach (byte b in cdl) if ((b & 0x04) != 0) count++;

		var result = new uint[count];
		int idx = 0;
		for (int i = 0; i < cdl.Length; i++) {
			if ((cdl[i] & 0x04) != 0) result[idx++] = (uint)i;
		}

		return result;
	}

	private static byte[] CompressOriginal(byte[] data) {
		using var output = new MemoryStream();
		using (var deflate = new DeflateStream(output, CompressionLevel.Optimal)) {
			deflate.Write(data, 0, data.Length);
		}

		return output.ToArray();
	}

	private static byte[] CompressFastest(ReadOnlySpan<byte> data) {
		using var output = new MemoryStream();
		using (var deflate = new DeflateStream(output, CompressionLevel.Fastest)) {
			deflate.Write(data);
		}

		return output.ToArray();
	}

	#endregion

	/// <summary>
	/// Test label structure for benchmarks
	/// </summary>
	public sealed class TestLabel {
		public int Address { get; set; }
		public string? Label { get; set; }
		public string? Comment { get; set; }
		public int Length { get; set; }
	}
}
