using System;
using System.Buffers;
using System.Collections.Frozen;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Nexen.Config;
using Nexen.Debugger.Integration;
using Nexen.Debugger.Labels;
using Nexen.Debugger.Utilities;
using Nexen.Interop;
using Nexen.Utilities;
using Nexen.Windows;
using Pansy.Core;
using PansyCrossRefType = Pansy.Core.CrossRefType;
using PansyMemoryRegionType = Pansy.Core.MemoryRegionType;

namespace Nexen.Debugger.Labels;
/// <summary>
/// Pansy file header structure for reading existing files.
/// Matches Pansy spec v1.0 layout: Magic(8) + Version(2) + Flags(2) + Platform(1) + Reserved(3) + RomSize(4) + RomCrc32(4) + SectionCount(4) + Reserved(4) = 32 bytes.
/// </summary>
public record PansyHeader(ushort Version, ushort Flags, byte Platform, uint RomSize, uint RomCrc32, uint SectionCount);

/// <summary>
/// Export options for Pansy file generation.
/// </summary>
public sealed class PansyExportOptions {
	public bool IncludeMemoryRegions { get; set; } = true;
	public bool IncludeCrossReferences { get; set; } = true;
	public bool IncludeDataBlocks { get; set; } = true;
	public bool UseCompression { get; set; } = false;
	public int CompressionLevel { get; set; } = 6; // 1-9, default 6
}

/// <summary>
/// Exports Nexen debugger data to Pansy metadata format.
/// Pansy is a universal disassembly metadata format for retro game analysis.
/// Phase 3: Memory regions, cross-references, data blocks
/// Phase 4: GZip compression, async export, memory pooling
/// </summary>
public static class PansyExporter {
	// Pansy file format constants - aligned with Pansy spec v1.0
	private const string MAGIC = "PANSY\0\0\0";
	private const ushort VERSION = 0x0100; // v1.0 - matches Pansy spec

	// Compression flag in header (PansyFlags.Compressed)
	private const ushort FLAG_COMPRESSED = 0x0001;
	// HasCrossRefs flag (bit 2) - indicates cross-reference section present
	private const ushort FLAG_HAS_CROSS_REFS = 0x0004;

	// Performance: Reusable ArrayPool for CDL conversion (avoids GC pressure)
	// Benchmark showed 23x speedup and zero allocations vs naive loop
	private static readonly ArrayPool<byte> CdlPool = ArrayPool<byte>.Shared;

	// Section types - matches Pansy spec exactly
	private const uint SECTION_CODE_DATA_MAP = 0x0001;
	private const uint SECTION_SYMBOLS = 0x0002;
	private const uint SECTION_COMMENTS = 0x0003;
	private const uint SECTION_MEMORY_REGIONS = 0x0004;
	private const uint SECTION_DATA_TYPES = 0x0005;    // Reserved in spec
	private const uint SECTION_CROSS_REFS = 0x0006;
	private const uint SECTION_SOURCE_MAP = 0x0007;    // Reserved in spec
	private const uint SECTION_METADATA = 0x0008;
	private const uint SECTION_CPU_STATE = 0x0009;
	private const uint SECTION_BOOKMARKS = 0x000a;

	// CDL flag bits for SNES-specific CPU state
	private const byte CDL_INDEX_MODE_8 = 0x10;  // SNES: X flag set (8-bit index)
	private const byte CDL_MEMORY_MODE_8 = 0x20; // SNES: M flag set (8-bit accumulator)
	private const byte CDL_GBA_THUMB = 0x20;     // GBA: THUMB mode

	// Platform IDs matching Pansy specification (PansyLoader constants)
	private static readonly FrozenDictionary<RomFormat, byte> PlatformIds = new Dictionary<RomFormat, byte>() {
		[RomFormat.iNes] = 0x01,       // NES
		[RomFormat.Fds] = 0x01,        // FDS (uses NES platform)
		[RomFormat.Nsf] = 0x01,        // NSF (uses NES)
		[RomFormat.Unif] = 0x01,       // UNIF (NES)
		[RomFormat.VsSystem] = 0x01,   // VS System (NES)
		[RomFormat.VsDualSystem] = 0x01, // VS Dual System (NES)
		[RomFormat.StudyBox] = 0x01,   // StudyBox (NES)
		[RomFormat.Sfc] = 0x02,        // SNES
		[RomFormat.Spc] = 0x0c,        // SPC700
		[RomFormat.Gb] = 0x03,         // Game Boy
		[RomFormat.Gbs] = 0x03,        // GBS
		[RomFormat.Gba] = 0x04,        // GBA
		[RomFormat.Genesis] = 0x05,    // Sega Genesis / Mega Drive
		[RomFormat.Sms] = 0x06,        // SMS
		[RomFormat.GameGear] = 0x16,   // Game Gear (separate from SMS in spec)
		[RomFormat.Sg] = 0x06,         // SG-1000 (SMS compatible)
		[RomFormat.ColecoVision] = 0x13, // ColecoVision
		[RomFormat.Pce] = 0x07,        // PC Engine
		[RomFormat.PceCdRom] = 0x07,   // PC Engine CD-ROM
		[RomFormat.PceHes] = 0x07,     // PC Engine HES
		[RomFormat.Ws] = 0x0a,         // WonderSwan
		[RomFormat.Lynx] = 0x09,       // Atari Lynx
		[RomFormat.Atari2600] = 0x08,  // Atari 2600
		[RomFormat.ChannelF] = 0x1f,   // Fairchild Channel F
	}.ToFrozenDictionary();

	/// <summary>
	/// Calculate CRC32 of the ROM data for integrity verification.
	/// Delegates to <see cref="RomHashService"/> which computes all basic hashes in a single pass.
	/// </summary>
	/// <param name="romInfo">ROM information containing console type</param>
	/// <returns>CRC32 hash of the ROM data, or 0 if unavailable</returns>
	public static uint CalculateRomCrc32(RomInfo romInfo) {
		var hashes = RomHashService.ComputeRomHashes(romInfo);
		return hashes.IsValid ? hashes.Crc32Value : 0;
	}

	/// <summary>
	/// Get all four basic hashes (CRC32, MD5, SHA-1, SHA-256) for the ROM.
	/// Uses StreamHash's batch streaming API for single-pass computation.
	/// </summary>
	/// <param name="romInfo">ROM information containing console type</param>
	/// <returns>All four hash values, or empty if unavailable</returns>
	public static RomHashInfo GetRomHashes(RomInfo romInfo) {
		return RomHashService.ComputeRomHashes(romInfo);
	}

	/// <summary>
	/// Read the header from an existing Pansy file to get stored CRC.
	/// Matches Pansy spec v1.0 header layout (32 bytes).
	/// </summary>
	/// <param name="path">Path to the pansy file</param>
	/// <returns>PansyHeader if valid, null otherwise</returns>
	public static PansyHeader? ReadHeader(string path) {
		try {
			if (!File.Exists(path))
				return null;

			using var stream = new FileStream(path, FileMode.Open, FileAccess.Read);
			using var reader = new BinaryReader(stream);

			// Check magic (8 bytes, offset 0)
			byte[] magic = reader.ReadBytes(8);
			if (System.Text.Encoding.ASCII.GetString(magic).TrimEnd('\0') != "PANSY")
				return null;

			// Version (2 bytes, offset 8)
			ushort version = reader.ReadUInt16();

			// Flags (2 bytes, offset 10) - PansyFlags as uint16
			ushort flags = reader.ReadUInt16();

			// Platform (1 byte, offset 12)
			byte platform = reader.ReadByte();

			// Reserved (3 bytes, offset 13-15)
			reader.ReadByte();
			reader.ReadUInt16();

			// ROM size (4 bytes, offset 16)
			uint romSize = reader.ReadUInt32();

			// ROM CRC32 (4 bytes, offset 20)
			uint romCrc32 = reader.ReadUInt32();

			// Section count (4 bytes, offset 24)
			uint sectionCount = reader.ReadUInt32();

			// Reserved (4 bytes, offset 28)
			reader.ReadUInt32();

			return new PansyHeader(version, flags, platform, romSize, romCrc32, sectionCount);
		} catch (Exception ex) {
			System.Diagnostics.Debug.WriteLine($"[PansyExporter] Failed to read header: {ex.Message}");
			return null;
		}
	}

	/// <summary>
	/// Export all debugger data to a Pansy file.
	/// </summary>
	/// <param name="path">Output file path</param>
	/// <param name="romInfo">ROM information for CRC and platform</param>
	/// <param name="memoryType">Memory type to export CDL data for</param>
	/// <param name="romCrc32Override">Optional CRC32 value to use (if 0, will be calculated)</param>
	/// <param name="options">Export options for compression and section selection</param>
	/// <returns>True if export succeeded</returns>
	public static bool Export(string path, RomInfo romInfo, MemoryType memoryType, uint romCrc32Override = 0, PansyExportOptions? options = null) {
		options ??= new PansyExportOptions();

		try {
			using var stream = new FileStream(path, FileMode.Create, FileAccess.Write, FileShare.None, 81920, FileOptions.SequentialScan);
			using var writer = new BinaryWriter(stream);

			// Get all the data we need to export
			var labels = LabelManager.GetAllLabels();
			var cdlData = GetCdlData(memoryType);
			var functions = GetFunctions(memoryType);
			var jumpTargets = GetJumpTargets(memoryType);
			var cpuType = romInfo.ConsoleType.GetMainCpuType();

			// Build sections
			List<SectionInfo> sections = [];
			List<byte[]> sectionData = [];

		// Section 1: CODE_DATA_MAP (CDL data with Pansy flag mapping)
		if (cdlData is { Length: > 0 }) {
			// Map CDL flags to Pansy spec flags and incorporate jump targets/functions
			bool hasCpuStateOverlay = romInfo.Format is RomFormat.Sfc or RomFormat.Spc or RomFormat.Gba;
			var pansyCdl = MapCdlToPansyFlags(cdlData, jumpTargets, functions, preserveUpperNibbleFlags: !hasCpuStateOverlay);
			var data = options.UseCompression ? CompressData(pansyCdl) : pansyCdl;
			sections.Add(new SectionInfo {
				Type = SECTION_CODE_DATA_MAP,
				CompressedSize = (uint)data.Length,
				UncompressedSize = (uint)pansyCdl.Length
			});
			sectionData.Add(data);
		}

		// Section 2: SYMBOLS (with typed symbol detection + hardware register labels)
		byte platformId = GetPlatformId(romInfo.Format);
		var symbolBytes = BuildSymbolSection(labels, functions, platformId);
		if (symbolBytes.Length > 0) {
			var data = options.UseCompression ? CompressData(symbolBytes) : symbolBytes;
			sections.Add(new SectionInfo {
				Type = SECTION_SYMBOLS,
				CompressedSize = (uint)data.Length,
				UncompressedSize = (uint)symbolBytes.Length
			});
			sectionData.Add(data);
		}

		// Section 3: COMMENTS (with type detection)
		var commentBytes = BuildCommentSection(labels);
		if (commentBytes.Length > 0) {
			var data = options.UseCompression ? CompressData(commentBytes) : commentBytes;
			sections.Add(new SectionInfo {
				Type = SECTION_COMMENTS,
				CompressedSize = (uint)data.Length,
				UncompressedSize = (uint)commentBytes.Length
			});
			sectionData.Add(data);
		}

		// Section 4: MEMORY_REGIONS
		if (options.IncludeMemoryRegions) {
			var memRegions = BuildEnhancedMemoryRegionsSection(labels, memoryType, romInfo);
			if (memRegions.Length > 4) { // More than just the count
				var data = options.UseCompression ? CompressData(memRegions) : memRegions;
				sections.Add(new SectionInfo {
					Type = SECTION_MEMORY_REGIONS,
					CompressedSize = (uint)data.Length,
					UncompressedSize = (uint)memRegions.Length
				});
				sectionData.Add(data);
			}
		}

		// Section 5: DATA_TYPES (derived from labels with Length and CDL data blocks)
		if (options.IncludeDataBlocks) {
			var dataTypes = BuildDataTypesSection(labels, cdlData);
			if (dataTypes.Length > 0) {
				var data = options.UseCompression ? CompressData(dataTypes) : dataTypes;
				sections.Add(new SectionInfo {
					Type = SECTION_DATA_TYPES,
					CompressedSize = (uint)data.Length,
					UncompressedSize = (uint)dataTypes.Length
				});
				sectionData.Add(data);
			}
		}

		// Section 6: CROSS_REFS
		if (options.IncludeCrossReferences) {
			var crossRefs = BuildEnhancedCrossRefsSection(labels, cdlData, functions, jumpTargets, cpuType, memoryType);
			if (crossRefs.Length > 4) { // More than just the count
				var data = options.UseCompression ? CompressData(crossRefs) : crossRefs;
				sections.Add(new SectionInfo {
					Type = SECTION_CROSS_REFS,
					CompressedSize = (uint)data.Length,
					UncompressedSize = (uint)crossRefs.Length
				});
				sectionData.Add(data);
			}
		}

		// Section 7: SOURCE_MAP (address -> source file/line mapping)
		var sourceMapBytes = BuildSourceMapSection(memoryType);
		if (sourceMapBytes.Length > 0) {
			var data = options.UseCompression ? CompressData(sourceMapBytes) : sourceMapBytes;
			sections.Add(new SectionInfo {
				Type = SECTION_SOURCE_MAP,
				CompressedSize = (uint)data.Length,
				UncompressedSize = (uint)sourceMapBytes.Length
			});
			sectionData.Add(data);
		}

		// Section 8: METADATA
		var metadataBytes = BuildMetadataSection(romInfo);
		if (metadataBytes.Length > 0) {
			var data = options.UseCompression ? CompressData(metadataBytes) : metadataBytes;
			sections.Add(new SectionInfo {
				Type = SECTION_METADATA,
				CompressedSize = (uint)data.Length,
				UncompressedSize = (uint)metadataBytes.Length
			});
			sectionData.Add(data);
		}

		// Section 9: CPU_STATE (per-address CPU mode for SNES and GBA)
		var cpuStateBytes = BuildCpuStateSection(cdlData, romInfo);
		if (cpuStateBytes.Length > 0) {
			var data = options.UseCompression ? CompressData(cpuStateBytes) : cpuStateBytes;
			sections.Add(new SectionInfo {
				Type = SECTION_CPU_STATE,
				CompressedSize = (uint)data.Length,
				UncompressedSize = (uint)cpuStateBytes.Length
			});
			sectionData.Add(data);
		}

		// Section 10: BOOKMARKS
		var bookmarkBytes = BuildBookmarksSection(cpuType);
		if (bookmarkBytes.Length > 0) {
			var data = options.UseCompression ? CompressData(bookmarkBytes) : bookmarkBytes;
			sections.Add(new SectionInfo {
				Type = SECTION_BOOKMARKS,
				CompressedSize = (uint)data.Length,
				UncompressedSize = (uint)bookmarkBytes.Length
			});
			sectionData.Add(data);
		}

		// Calculate section offsets
		// Header = 32 bytes, section table = sectionCount * 16 bytes
		uint currentOffset = 32 + (uint)(sections.Count * 16);
		for (int i = 0; i < sections.Count; i++) {
			sections[i].Offset = currentOffset;
			currentOffset += sections[i].CompressedSize;
		}

		// Get ROM size and CRC
		var stats = DebugApi.GetCdlStatistics(memoryType);
		uint romSize = stats.TotalBytes;
		uint romCrc = romCrc32Override != 0 ? romCrc32Override : CalculateRomCrc32(romInfo);

		// Write header (32 bytes) - matches Pansy spec v1.0 layout exactly
		ushort flags = options.UseCompression ? FLAG_COMPRESSED : (ushort)0;
		// Set HAS_CPU_STATE (bit 4) if CPU state section is present
		if (cpuStateBytes.Length > 0) {
			flags |= 0x0010; // HAS_CPU_STATE
		}
		// Set HAS_CROSS_REFS (bit 2) if cross-references section is present
		if (sections.Any(s => s.Type == SECTION_CROSS_REFS)) {
			flags |= FLAG_HAS_CROSS_REFS;
		}
		writer.Write(Encoding.ASCII.GetBytes(MAGIC)); // 8 bytes (offset 0)
		writer.Write(VERSION);                         // 2 bytes (offset 8)
		writer.Write(flags);                           // 2 bytes (offset 10) - PansyFlags as uint16
		writer.Write(GetPlatformId(romInfo.Format));   // 1 byte (offset 12)
		writer.Write((byte)0);                         // 1 byte reserved (offset 13)
		writer.Write((ushort)0);                       // 2 bytes reserved (offset 14)
		writer.Write(romSize);                         // 4 bytes (offset 16)
		writer.Write(romCrc);                          // 4 bytes (offset 20)
		writer.Write((uint)sections.Count);            // 4 bytes (offset 24) - Section count
		writer.Write((uint)0);                         // 4 bytes (offset 28) - Reserved

		// Write section table - Pansy spec: Type(u32) + Offset(u32) + CompSize(u32) + UncompSize(u32)
		foreach (var section in sections) {
			writer.Write(section.Type);                // 4 bytes - section type (uint32)
			writer.Write(section.Offset);              // 4 bytes - offset from file start
			writer.Write(section.CompressedSize);      // 4 bytes - compressed size
			writer.Write(section.UncompressedSize);    // 4 bytes - uncompressed size
		}

		// Write section data
		foreach (var d in sectionData) {
			writer.Write(d);
		}

		// No footer - Pansy spec: "integrity checks at application level"

		return true;
	} catch (Exception ex) {
		System.Diagnostics.Debug.WriteLine($"Pansy export failed: {ex.Message}");
			return false;
		}
	}

	/// <summary>
	/// Async export for background operations (Phase 4).
	/// </summary>
	public static async Task<bool> ExportAsync(string path, RomInfo romInfo, MemoryType memoryType, uint romCrc32Override = 0, PansyExportOptions? options = null, CancellationToken cancellationToken = default) {
		return await Task.Run(() => Export(path, romInfo, memoryType, romCrc32Override, options), cancellationToken);
	}

	/// <summary>
	/// Compress data using DEFLATE (matches Pansy spec).
	/// Optimized: Uses CompressionLevel.Fastest (6.6x faster than Optimal).
	/// </summary>
	private static byte[] CompressData(byte[] data) {
		if (data.Length < 64) // Don't compress small data - overhead not worth it
			return data;

		// Pre-size output stream to estimated compressed size (typical ~50% for CDL data)
		using var output = new MemoryStream(data.Length / 2);
		using (var deflate = new DeflateStream(output, CompressionLevel.Fastest, leaveOpen: true)) {
			// Write using span for efficiency
			deflate.Write(data.AsSpan());
		}

		var compressed = output.ToArray();
		// Only use compressed if it's actually smaller (some data doesn't compress well)
		return compressed.Length < data.Length ? compressed : data;
	}

	/// <summary>
	/// Decompress DEFLATE data (matches Pansy spec).
	/// </summary>
	public static byte[] DecompressData(byte[] compressedData, int uncompressedSize) {
		using var input = new MemoryStream(compressedData);
		using var deflate = new DeflateStream(input, CompressionMode.Decompress);
		var output = new byte[uncompressedSize];
		int totalRead = 0;
		while (totalRead < uncompressedSize) {
			int read = deflate.Read(output, totalRead, uncompressedSize - totalRead);
			if (read == 0) break;
			totalRead += read;
		}

		return output;
	}

	/// <summary>
	/// Auto-export Pansy file when CDL is saved.
	/// Called from debugger shutdown or explicit save.
	/// Uses folder-based storage if enabled (Phase 7.5).
	/// </summary>
	/// <param name="romInfo">Current ROM information</param>
	/// <param name="memoryType">Memory type for CDL</param>
	public static void AutoExport(RomInfo romInfo, MemoryType memoryType, uint cachedCrc32 = 0) {
		Log.Info($"[Pansy] AutoExport called - AutoExportPansy={ConfigManager.Config.Debug.Integration.AutoExportPansy}");

		if (!ConfigManager.Config.Debug.Integration.AutoExportPansy) {
			Log.Info("[Pansy] Auto-export disabled, skipping");
			return;
		}

		var config = ConfigManager.Config.Debug.Integration;

		// Phase 7.5: Use folder-based storage if enabled
		if (config.UseFolderStorage) {
			Log.Info("[Pansy] Using folder-based storage");
			bool success = DebugFolderManager.ExportAllFiles(romInfo, memoryType);
			Log.Info($"[Pansy] Folder export result: {success}");
			return;
		}

		// Legacy single-file export
		string romName = romInfo.GetRomName();
		string pansyPath = GetPansyFilePath(romName);
		Log.Info($"[Pansy] Target path: {pansyPath}");

		// Use cached CRC if available, otherwise calculate (first call or manual trigger)
		uint currentCrc = cachedCrc32 != 0 ? cachedCrc32 : CalculateRomCrc32(romInfo);
		Log.Info($"[Pansy] Current ROM CRC32: {currentCrc:X8}");

		// Build export options from configuration
		var options = new PansyExportOptions {
			IncludeMemoryRegions = config.PansyIncludeMemoryRegions,
			IncludeCrossReferences = config.PansyIncludeCrossReferences,
			IncludeDataBlocks = config.PansyIncludeDataBlocks,
			UseCompression = config.PansyUseCompression
		};

		// Check if existing pansy file has matching CRC
		var existingHeader = ReadHeader(pansyPath);
		if (existingHeader is not null) {
			Log.Info($"[Pansy] Existing file CRC32: {existingHeader.RomCrc32:X8}");

			if (existingHeader.RomCrc32 != 0 && currentCrc != 0 && existingHeader.RomCrc32 != currentCrc) {
				// CRC mismatch - this is likely a hacked/translated ROM
				// Create a separate file with CRC suffix instead of overwriting
				string altPath = GetPansyFilePathWithCrc(romName, currentCrc);
				Log.Info($"[Pansy] CRC MISMATCH! Creating separate file: {altPath}");
				bool success = Export(altPath, romInfo, memoryType, currentCrc, options);
				Log.Info($"[Pansy] Export result: {success}");
				return;
			}
		}

		// Export normally (CRC matches or no existing file)
		bool exported = Export(pansyPath, romInfo, memoryType, currentCrc, options);
		Log.Info($"[Pansy] Export result: {exported}");
	}

	/// <summary>
	/// Get the default Pansy file path for a ROM.
	/// </summary>
	public static string GetPansyFilePath(string romName) {
		string filename = Path.GetFileNameWithoutExtension(romName);
		return Path.Combine(ConfigManager.DebuggerFolder, $"{filename}.pansy");
	}

	/// <summary>
	/// Get Pansy file path with CRC suffix for mismatched ROMs.
	/// </summary>
	public static string GetPansyFilePathWithCrc(string romName, uint crc32) {
		string filename = Path.GetFileNameWithoutExtension(romName);
		return Path.Combine(ConfigManager.DebuggerFolder, $"{filename}_{crc32:x8}.pansy");
	}

	private static byte GetPlatformId(RomFormat format) {
		return PlatformIds.TryGetValue(format, out byte id) ? id : (byte)0xFF;
	}

	/// <summary>
	/// Get CDL data as byte array.
	/// Optimized: Uses MemoryMarshal for zero-copy cast (CdlFlags is byte-backed enum).
	/// Benchmark: 23x faster than element-by-element loop, zero allocations with ArrayPool.
	/// </summary>
	private static byte[]? GetCdlData(MemoryType memoryType) {
		try {
			int size = DebugApi.GetMemorySize(memoryType);
			if (size <= 0) return null;

			CdlFlags[] cdlData = DebugApi.GetCdlData(0, (uint)size, memoryType);
			if (cdlData is null or { Length: 0 }) return null;

			// Optimized: CdlFlags is a byte-backed enum, use MemoryMarshal for zero-copy
			// This avoids the 360μs element-by-element loop entirely
			ReadOnlySpan<CdlFlags> source = cdlData.AsSpan();
			ReadOnlySpan<byte> sourceBytes = MemoryMarshal.AsBytes(source);

			// Copy to new array (we can't return the span, but the copy is fast)
			byte[] result = new byte[sourceBytes.Length];
			sourceBytes.CopyTo(result);

			return result;
		} catch (Exception ex) {
			Debug.WriteLine($"PansyExporter.GetCdlData failed: {ex.Message}");
			return null;
		}
	}

	private static uint[] GetFunctions(MemoryType memoryType) {
		try {
			return DebugApi.GetCdlFunctions(memoryType);
		} catch (Exception ex) {
			Debug.WriteLine($"PansyExporter.GetFunctions failed: {ex.Message}");
			return [];
		}
	}

	/// <summary>
	/// Get jump target addresses from CDL data.
	/// Optimized: Pre-count to size array exactly, single pass for extraction.
	/// </summary>
	private static uint[] GetJumpTargets(MemoryType memoryType) {
		try {
			// Get CDL data and extract jump targets (flag 0x04 = JumpTarget)
			byte[]? cdl = GetCdlData(memoryType);
			if (cdl is null or { Length: 0 }) return [];

			// Optimized: Use span for faster iteration
			ReadOnlySpan<byte> cdlSpan = cdl.AsSpan();

			// Pre-count to avoid list resizing
			int count = 0;
			foreach (byte b in cdlSpan) {
				if ((b & 0x04) != 0) count++;
			}

			if (count == 0) return [];

			uint[] targets = new uint[count];
			int idx = 0;
			for (int i = 0; i < cdlSpan.Length; i++) {
				if ((cdlSpan[i] & 0x04) != 0) {
					targets[idx++] = (uint)i;
				}
			}

			return targets;
		} catch (Exception ex) {
			Debug.WriteLine($"PansyExporter.GetJumpTargets failed: {ex.Message}");
			return [];
		}
	}

	/// <summary>
	/// Map Nexen CDL flags to Pansy spec code/data map flags.
	/// Pansy flags: CODE(0x01), DATA(0x02), JUMP_TARGET(0x04), SUB_ENTRY(0x08),
	///              OPCODE(0x10), DRAWN(0x20), READ(0x40), INDIRECT(0x80).
	/// Nexen CDL: Code(0x01), Data(0x02), JumpTarget(0x04), SubEntryPoint(0x08).
	/// SNES/GBA may overlay bits 4-5 for CPU mode state, so upper nibble can be conditionally ignored.
	/// </summary>
	private static byte[] MapCdlToPansyFlags(byte[] cdlData, uint[] jumpTargets, uint[] functions, bool preserveUpperNibbleFlags) {
		var result = new byte[cdlData.Length];

		for (int i = 0; i < cdlData.Length; i++) {
			byte cdl = cdlData[i];

			// Map basic flags (bits 0-3 match between CDL and Pansy spec)
			byte pansyFlags = (byte)(cdl & 0x0f); // CODE, DATA, JUMP_TARGET, SUB_ENTRY

			// Bits 4-5 can represent standard flags (OPCODE/DRAWN) on platforms
			// that don't overlay them for CPU mode state.
			if (preserveUpperNibbleFlags) {
				if ((cdl & 0x10) != 0) {
					pansyFlags |= 0x10; // OPCODE
				}
				if ((cdl & 0x20) != 0) {
					pansyFlags |= 0x20; // DRAWN
				}
			}

			// Bit 6 (READ, 0x40): Preserve explicit read flag and keep legacy DATA->READ behavior.
			if ((cdl & 0x40) != 0 || (cdl & 0x02) != 0) {
				pansyFlags |= 0x40; // READ
			}

			// Bit 7 (INDIRECT, 0x80): Preserve when present.
			if ((cdl & 0x80) != 0) {
				pansyFlags |= 0x80;
			}

			result[i] = pansyFlags;
		}

		// Explicit jump/subroutine sets should always be represented in CDM, even if
		// source CDL bytes were partial or transformed by platform-specific overlays.
		foreach (uint target in jumpTargets) {
			if (target < (uint)result.Length) {
				result[target] |= 0x04;
			}
		}

		foreach (uint function in functions) {
			if (function < (uint)result.Length) {
				result[function] |= 0x08;
			}
		}

		return result;
	}

	private static byte[] BuildSymbolSection(List<CodeLabel> labels, uint[] functions, byte platformId) {
		// Build function set for type detection
		var functionSet = new HashSet<uint>(functions);

		// NES/SNES/65xx interrupt vector addresses
		var interruptVectors = new HashSet<uint> {
			0xfffa, 0xfffb, // NMI vector
			0xfffc, 0xfffd, // RESET vector
			0xfffe, 0xffff, // IRQ/BRK vector
		};

		// Use LabelMergeEngine to merge user labels with hardware register labels
		var mergeEngine = new LabelMergeEngine();

		// Add user labels
		foreach (var label in labels) {
			if (string.IsNullOrEmpty(label.Label)) continue;
			byte symbolType = DetectSymbolType(label, functionSet, interruptVectors);
			mergeEngine.Add(new MergedLabel(
				(uint)label.Address,
				label.Label,
				(Pansy.Core.SymbolType)symbolType,
				LabelSource.User
			));
		}

		// Add hardware register labels from PlatformDefaults
		mergeEngine.AddHardwareRegisters(platformId);

		// Get merged symbols (user labels take priority over hardware register defaults)
		var mergedSymbols = mergeEngine.GetMergedSymbols().ToList();

		// Pansy spec: No count prefix. Per entry: Address(4)+Type(1)+Flags(1)+NameLen(2)+Name+ValueLen(2)
		using var ms = new MemoryStream(mergedSymbols.Count * 18);
		using var writer = new BinaryWriter(ms);

		foreach (var symbol in mergedSymbols) {
			// Address (4 bytes)
			writer.Write(symbol.Address);

			// Type (1 byte)
			writer.Write((byte)symbol.Type);

			// Flags (1 byte)
			writer.Write((byte)0);

			// Name length + name
			byte[] nameBytes = Encoding.UTF8.GetBytes(symbol.Name);
			writer.Write((ushort)nameBytes.Length);
			writer.Write(nameBytes);

			// Value length (2 bytes) - no value for labels
			writer.Write((ushort)0);
		}

		return ms.ToArray();
	}

	private static byte[] BuildCommentSection(List<CodeLabel> labels) {
		// Optimized: Count first to pre-size the MemoryStream
		int count = 0;
		foreach (var label in labels) {
			if (!string.IsNullOrEmpty(label.Comment)) count++;
		}

		// Pansy spec: No count prefix. Per entry: Address(4)+Type(1)+TextLen(2)+Text
		using var ms = new MemoryStream(count * 27);
		using var writer = new BinaryWriter(ms);

		// Optimized: Direct iteration instead of LINQ Where().ToList()
		foreach (var label in labels) {
			if (string.IsNullOrEmpty(label.Comment)) continue;

			// Address (4 bytes)
			writer.Write((uint)label.Address);

			// Type (1 byte): Detect comment type
			byte commentType = DetectCommentType(label.Comment);
			writer.Write(commentType);

			// Comment length + text
			byte[] commentBytes = Encoding.UTF8.GetBytes(label.Comment);
			writer.Write((ushort)commentBytes.Length);
			writer.Write(commentBytes);
		}

		return ms.ToArray();
	}



	/// <summary>
	/// Build enhanced memory regions section including system memory maps.
	/// Phase 3: Includes ROM, RAM, VRAM, I/O regions with proper naming.
	/// </summary>
	private static byte[] BuildEnhancedMemoryRegionsSection(List<CodeLabel> labels, MemoryType memType, RomInfo romInfo) {
		List<(uint Start, uint End, string Name, PansyMemoryRegionType Type, byte MemType)> allRegions = new(64);

		// Add system memory map based on console type
		AddSystemMemoryRegions(allRegions, romInfo);

		// Add user-defined regions from labels with length > 1
		// Optimized: Direct iteration instead of LINQ Where()
		foreach (var label in labels) {
			if (label.Length > 1 && !string.IsNullOrEmpty(label.Label)) {
				allRegions.Add((
					(uint)label.Address,
					(uint)(label.Address + label.Length - 1),
					label.Label,
					label.MemoryType.IsRomMemory() ? PansyMemoryRegionType.ROM : PansyMemoryRegionType.RAM,
					(byte)label.MemoryType
				));
			}
		}

		// Sort by start address
		allRegions.Sort((a, b) => a.Start.CompareTo(b.Start));

		// Estimate ~20 bytes per region
		// Pansy spec: No count prefix. Per region: Start(4)+End(4)+Type(1)+Bank(1)+Flags(2)+NameLen(2)+Name
		using var ms = new MemoryStream(allRegions.Count * 20);
		using var writer = new BinaryWriter(ms);

		foreach (var region in allRegions) {
			writer.Write(region.Start);           // Start address (4 bytes)
			writer.Write(region.End);             // End address (4 bytes)
			writer.Write((byte)region.Type);      // Type (1 byte) - MemoryRegionType
			writer.Write((byte)0);                // Bank (1 byte)
			writer.Write((ushort)0);              // Flags (2 bytes reserved)

			byte[] nameBytes = Encoding.UTF8.GetBytes(region.Name);
			writer.Write((ushort)nameBytes.Length);
			writer.Write(nameBytes);
		}

		return ms.ToArray();
	}

	/// <summary>
	/// Add system memory regions based on console type.
	/// </summary>
	private static void AddSystemMemoryRegions(List<(uint Start, uint End, string Name, PansyMemoryRegionType Type, byte MemType)> regions, RomInfo romInfo) {
		switch (romInfo.ConsoleType) {
			case ConsoleType.Nes:
				// NES Memory Map
				regions.Add((0x0000, 0x07FF, "RAM", PansyMemoryRegionType.RAM, (byte)MemoryType.NesInternalRam));
				regions.Add((0x0800, 0x1FFF, "RAM_Mirrors", PansyMemoryRegionType.Mirror, (byte)MemoryType.NesInternalRam));
				regions.Add((0x2000, 0x2007, "PPU_Registers", PansyMemoryRegionType.IO, (byte)MemoryType.NesMemory));
				regions.Add((0x4000, 0x4017, "APU_IO_Registers", PansyMemoryRegionType.IO, (byte)MemoryType.NesMemory));
				regions.Add((0x4020, 0x5FFF, "Expansion_ROM", PansyMemoryRegionType.ROM, (byte)MemoryType.NesMemory));
				regions.Add((0x6000, 0x7FFF, "SRAM", PansyMemoryRegionType.SRAM, (byte)MemoryType.NesSaveRam));
				regions.Add((0x8000, 0xFFFF, "PRG_ROM", PansyMemoryRegionType.ROM, (byte)MemoryType.NesPrgRom));
				break;

			case ConsoleType.Snes:
				// SNES Memory Map (LoROM mode as default)
				regions.Add((0x0000, 0x1FFF, "LowRAM", PansyMemoryRegionType.RAM, (byte)MemoryType.SnesMemory));
				regions.Add((0x2100, 0x21FF, "PPU_Registers", PansyMemoryRegionType.IO, (byte)MemoryType.SnesMemory));
				regions.Add((0x4200, 0x43FF, "CPU_Registers", PansyMemoryRegionType.IO, (byte)MemoryType.SnesMemory));
				regions.Add((0x7E0000, 0x7FFFFF, "WRAM", PansyMemoryRegionType.WRAM, (byte)MemoryType.SnesWorkRam));
				break;

			case ConsoleType.Gameboy:
				// Game Boy Memory Map
				regions.Add((0x0000, 0x3FFF, "ROM_Bank_0", PansyMemoryRegionType.ROM, (byte)MemoryType.GbPrgRom));
				regions.Add((0x4000, 0x7FFF, "ROM_Bank_N", PansyMemoryRegionType.ROM, (byte)MemoryType.GbPrgRom));
				regions.Add((0x8000, 0x9FFF, "VRAM", PansyMemoryRegionType.VRAM, (byte)MemoryType.GbVideoRam));
				regions.Add((0xA000, 0xBFFF, "External_RAM", PansyMemoryRegionType.SRAM, (byte)MemoryType.GbCartRam));
				regions.Add((0xC000, 0xDFFF, "WRAM", PansyMemoryRegionType.WRAM, (byte)MemoryType.GbWorkRam));
				regions.Add((0xFE00, 0xFE9F, "OAM", PansyMemoryRegionType.RAM, (byte)MemoryType.GbSpriteRam));
				regions.Add((0xFF00, 0xFF7F, "IO_Registers", PansyMemoryRegionType.IO, (byte)MemoryType.GameboyMemory));
				regions.Add((0xFF80, 0xFFFE, "HRAM", PansyMemoryRegionType.RAM, (byte)MemoryType.GbHighRam));
				break;

			case ConsoleType.Gba:
				// GBA Memory Map
				regions.Add((0x00000000, 0x00003FFF, "BIOS", PansyMemoryRegionType.ROM, (byte)MemoryType.GbaMemory));
				regions.Add((0x02000000, 0x0203FFFF, "EWRAM", PansyMemoryRegionType.WRAM, (byte)MemoryType.GbaMemory));
				regions.Add((0x03000000, 0x03007FFF, "IWRAM", PansyMemoryRegionType.WRAM, (byte)MemoryType.GbaIntWorkRam));
				regions.Add((0x04000000, 0x040003FF, "IO_Registers", PansyMemoryRegionType.IO, (byte)MemoryType.GbaMemory));
				regions.Add((0x05000000, 0x050003FF, "Palette_RAM", PansyMemoryRegionType.RAM, (byte)MemoryType.GbaPaletteRam));
				regions.Add((0x06000000, 0x06017FFF, "VRAM", PansyMemoryRegionType.VRAM, (byte)MemoryType.GbaVideoRam));
				regions.Add((0x07000000, 0x070003FF, "OAM", PansyMemoryRegionType.RAM, (byte)MemoryType.GbaSpriteRam));
				regions.Add((0x08000000, 0x09FFFFFF, "ROM", PansyMemoryRegionType.ROM, (byte)MemoryType.GbaPrgRom));
				regions.Add((0x0E000000, 0x0E00FFFF, "SRAM", PansyMemoryRegionType.SRAM, (byte)MemoryType.GbaSaveRam));
				break;

			case ConsoleType.PcEngine:
				// PC Engine Memory Map
				regions.Add((0x0000, 0x1FFF, "RAM", PansyMemoryRegionType.RAM, (byte)MemoryType.PceMemory));
				regions.Add((0x1FE000, 0x1FFFFF, "Hardware_Registers", PansyMemoryRegionType.IO, (byte)MemoryType.PceMemory));
				break;

			case ConsoleType.Sms:
				// Sega Master System Memory Map
				regions.Add((0x0000, 0xBFFF, "ROM", PansyMemoryRegionType.ROM, (byte)MemoryType.SmsPrgRom));
				regions.Add((0xC000, 0xDFFF, "RAM", PansyMemoryRegionType.RAM, (byte)MemoryType.SmsWorkRam));
				break;

			case ConsoleType.Lynx:
				// Atari Lynx Memory Map
				regions.Add((0x0000, 0xFBFF, "RAM", PansyMemoryRegionType.WRAM, (byte)MemoryType.LynxWorkRam));
				regions.Add((0xFC00, 0xFCFF, "Suzy_Registers", PansyMemoryRegionType.IO, (byte)MemoryType.LynxMemory));
				regions.Add((0xFD00, 0xFDFF, "Mikey_Registers", PansyMemoryRegionType.IO, (byte)MemoryType.LynxMemory));
				regions.Add((0xFE00, 0xFFF7, "Boot_ROM", PansyMemoryRegionType.ROM, (byte)MemoryType.LynxBootRom));
				regions.Add((0xFFFA, 0xFFFF, "Vectors", PansyMemoryRegionType.ROM, (byte)MemoryType.LynxMemory));
				break;

			case ConsoleType.Atari2600:
				// Atari 2600 Memory Map (13-bit address bus, 8KB space)
				// TIA has separate read ($00-$0D) and write ($00-$2C) register sets
				// overlapping in address space — distinguished by R/W signal
				regions.Add((0x0000, 0x000D, "TIA_Read", PansyMemoryRegionType.IO, (byte)MemoryType.Atari2600TiaRegisters));
				regions.Add((0x0000, 0x002C, "TIA_Write", PansyMemoryRegionType.IO, (byte)MemoryType.Atari2600TiaRegisters));
				regions.Add((0x0080, 0x00FF, "RAM", PansyMemoryRegionType.RAM, (byte)MemoryType.Atari2600Ram));
				regions.Add((0x0280, 0x029F, "RIOT", PansyMemoryRegionType.IO, (byte)MemoryType.Atari2600Memory));
				regions.Add((0x1000, 0x1FFF, "Cart_ROM", PansyMemoryRegionType.ROM, (byte)MemoryType.Atari2600PrgRom));
				break;

			case ConsoleType.Genesis:
				// Sega Genesis / Mega Drive Memory Map (M68000, 24-bit address bus)
				regions.Add((0x000000, 0x3FFFFF, "ROM", PansyMemoryRegionType.ROM, (byte)MemoryType.GenesisPrgRom));
				regions.Add((0xA00000, 0xA0FFFF, "Z80_Address_Space", PansyMemoryRegionType.RAM, (byte)MemoryType.GenesisMemory));
				regions.Add((0xA10000, 0xA1001F, "IO_Registers", PansyMemoryRegionType.IO, (byte)MemoryType.GenesisMemory));
				regions.Add((0xC00000, 0xC0001F, "VDP_Registers", PansyMemoryRegionType.IO, (byte)MemoryType.GenesisMemory));
				regions.Add((0xFF0000, 0xFFFFFF, "Work_RAM", PansyMemoryRegionType.WRAM, (byte)MemoryType.GenesisWorkRam));
				break;

			case ConsoleType.Ws:
				// WonderSwan Memory Map (V30MZ, 20-bit segmented addressing)
				regions.Add((0x00000, 0x03FFF, "RAM", PansyMemoryRegionType.RAM, (byte)MemoryType.WsWorkRam));
				regions.Add((0x00000, 0x000FF, "IO_Registers", PansyMemoryRegionType.IO, (byte)MemoryType.WsMemory));
				regions.Add((0x20000, 0xFFFFF, "ROM", PansyMemoryRegionType.ROM, (byte)MemoryType.WsPrgRom));
				break;

			case ConsoleType.ChannelF:
				// Fairchild Channel F Memory Map
				regions.Add((0x0000, 0x17ff, "Cartridge_ROM", PansyMemoryRegionType.ROM, (byte)MemoryType.None));
				regions.Add((0x2800, 0x2fff, "System_RAM", PansyMemoryRegionType.RAM, (byte)MemoryType.None));
				regions.Add((0x3000, 0x37ff, "Video_RAM", PansyMemoryRegionType.VRAM, (byte)MemoryType.None));
				regions.Add((0x3800, 0x38ff, "IO_Registers", PansyMemoryRegionType.IO, (byte)MemoryType.None));
				break;

			default:
				break;
		}
	}

	/// <summary>
	/// Build Data Types section (0x0005) from labels with Length and CDL data blocks.
	/// Derives structured data annotations from:
	/// - Labels with Length > 1 (user-annotated multi-byte data regions)
	/// - Contiguous DATA-flagged CDL regions >= 4 bytes
	/// Follows Pansy spec: Address(4) + Length(4) + ElementSize(2) + ElementCount(2) + Type(1) + NameLen(2) + Name
	/// </summary>
	private static byte[] BuildDataTypesSection(List<CodeLabel> labels, byte[]? cdlData) {
		using var ms = new MemoryStream();
		using var writer = new BinaryWriter(ms);
		int count = 0;

		// 1. Labels with Length > 1 → typed data entries
		foreach (var label in labels) {
			if (label.Length <= 1 || string.IsNullOrEmpty(label.Label)) {
				continue;
			}

			byte elementType = label.Length switch {
				2 => 2,    // Word
				3 or 4 => 4, // Pointer
				_ => 1    // Byte (array)
			};

			ushort elementSize = label.Length switch {
				2 => 2,
				3 => 3,
				4 => 4,
				_ => 1
			};

			ushort elementCount = (ushort)(label.Length / elementSize);

			writer.Write(label.Address);            // Address (4 bytes)
			writer.Write(label.Length);              // Total length (4 bytes)
			writer.Write(elementSize);               // Element size (2 bytes)
			writer.Write(elementCount);              // Element count (2 bytes)
			writer.Write(elementType);               // DataElementType (1 byte)

			byte[] nameBytes = System.Text.Encoding.UTF8.GetBytes(label.Label);
			writer.Write((ushort)nameBytes.Length);  // Name length (2 bytes)
			writer.Write(nameBytes);                 // Name

			count++;
		}

		// 2. CDL data blocks >= 8 bytes (contiguous DATA regions without labels)
		if (cdlData is { Length: > 0 }) {
			var labelAddresses = new HashSet<uint>(labels.Where(l => l.Length > 1).Select(l => l.Address));
			int? blockStart = null;

			for (int i = 0; i < cdlData.Length; i++) {
				bool isData = (cdlData[i] & 0x02) != 0 && (cdlData[i] & 0x01) == 0;

				if (isData && blockStart is null) {
					blockStart = i;
				} else if (!isData && blockStart is not null) {
					uint start = (uint)blockStart.Value;
					uint length = (uint)(i - blockStart.Value);

					if (length >= 8 && !labelAddresses.Contains(start)) {
						writer.Write(start);              // Address
						writer.Write(length);             // Total length
						writer.Write((ushort)1);          // Element size (byte)
						writer.Write((ushort)length);     // Element count
						writer.Write((byte)1);            // DataElementType.Byte
						writer.Write((ushort)0);          // No name
						count++;
					}

					blockStart = null;
				}
			}

			if (blockStart is not null) {
				uint start = (uint)blockStart.Value;
				uint length = (uint)(cdlData.Length - blockStart.Value);
				if (length >= 8 && !labelAddresses.Contains(start)) {
					writer.Write(start);
					writer.Write(length);
					writer.Write((ushort)1);
					writer.Write((ushort)length);
					writer.Write((byte)1);
					writer.Write((ushort)0);
					count++;
				}
			}
		}

		return count > 0 ? ms.ToArray() : [];
	}

	/// <summary>
	/// Build source map section (0x0007) from the active symbol provider.
	///
	/// Format (Nexen extension while SOURCE_MAP is reserved in base spec):
	/// - FileCount (u32)
	/// - Repeated files: NameLen (u16) + NameUtf8
	/// - EntryCount (u32)
	/// - Repeated entries: Address (u32) + FileIndex (u16) + LineStart (u32) + LineEndExclusive (u32)
	/// </summary>
	private static byte[] BuildSourceMapSection(MemoryType memoryType) {
		try {
			var provider = DebugWorkspaceManager.SymbolProvider;
			if (provider is null || provider.SourceFiles.Count == 0) {
				return [];
			}

			var files = provider.SourceFiles;
			var entries = new List<(uint Address, ushort FileIndex, uint LineStart, uint LineEndExclusive)>();

			for (int fileIndex = 0; fileIndex < files.Count; fileIndex++) {
				var file = files[fileIndex];
				var lines = file.Data;

				for (int lineIndex = 0; lineIndex < lines.Length; lineIndex++) {
					var start = provider.GetLineAddress(file, lineIndex);
					if (start is null || start.Value.Type != memoryType || start.Value.Address < 0) {
						continue;
					}

					var end = provider.GetLineEndAddress(file, lineIndex);
					uint startAddress = (uint)start.Value.Address;
					uint endAddressExclusive = end is not null && end.Value.Type == memoryType && end.Value.Address > start.Value.Address
						? (uint)end.Value.Address
						: startAddress + 1;

					entries.Add((
						Address: startAddress,
						FileIndex: (ushort)fileIndex,
						LineStart: (uint)(lineIndex + 1),
						LineEndExclusive: (uint)(lineIndex + 2)
					));
				}
			}

			return SerializeSourceMapSection(files, entries);
		} catch (Exception ex) {
			Debug.WriteLine($"PansyExporter.GenerateSourceMapSection failed: {ex.Message}");
			return [];
		}
	}

	private static byte[] SerializeSourceMapSection(IReadOnlyList<SourceFileInfo> files, List<(uint Address, ushort FileIndex, uint LineStart, uint LineEndExclusive)> entries) {
		if (files.Count == 0 || entries.Count == 0) {
			return [];
		}

		using var ms = new MemoryStream(entries.Count * 14 + files.Count * 32);
		using var writer = new BinaryWriter(ms);

		writer.Write((uint)files.Count);
		for (int i = 0; i < files.Count; i++) {
			byte[] nameBytes = Encoding.UTF8.GetBytes(files[i].Name);
			writer.Write((ushort)nameBytes.Length);
			writer.Write(nameBytes);
		}

		writer.Write((uint)entries.Count);
		foreach (var entry in entries.OrderBy(e => e.Address).ThenBy(e => e.FileIndex).ThenBy(e => e.LineStart)) {
			writer.Write(entry.Address);
			writer.Write(entry.FileIndex);
			writer.Write(entry.LineStart);
			writer.Write(entry.LineEndExclusive);
		}

		return ms.ToArray();
	}



	/// <summary>
	/// Build enhanced cross-references section by analyzing CDL and disassembly.
	/// Phase 3: Extracts actual cross-references from code analysis.
	/// Also emits fallthrough edges for conditional branches (taken + not-taken paths)
	/// to represent one-source-many-target control flow when statically resolvable.
	/// Optimized: Uses OPCODE flag (0x10) filtering to skip operand bytes, OpSize-based
	/// instruction skip to avoid redundant interop calls, and HashSet for O(1) deduplication.
	/// </summary>
	private static byte[] BuildEnhancedCrossRefsSection(List<CodeLabel> labels, byte[]? cdlData, uint[] functions, uint[] jumpTargets, CpuType cpuType, MemoryType memType) {
		if (cdlData is null or { Length: 0 }) {
			// Return empty section (no count prefix per Pansy spec)
			return [];
		}

		// Optimized: Use HashSet for O(1) deduplication during collection
		var seenXrefs = new HashSet<(uint From, uint To)>(256);
		var xrefs = new List<(uint From, uint To, PansyCrossRefType Type, byte MemTypeFrom, byte MemTypeTo)>(256);

		// Build sets for quick target classification
		var functionSet = new HashSet<uint>(functions);
		var jumpTargetSet = new HashSet<uint>(jumpTargets);

		// Use span for faster CDL iteration
		ReadOnlySpan<byte> cdlSpan = cdlData.AsSpan();

		// Pre-scan: check if CDL has OPCODE flags (0x10) for instruction-start filtering.
		// If present, we only process opcode start bytes instead of every code byte,
		// dramatically reducing the number of GetDisassemblyOutput interop calls.
		bool useOpcodeFilter = false;
		for (int i = 0; i < cdlSpan.Length; i++) {
			if ((cdlSpan[i] & 0x10) != 0) {
				useOpcodeFilter = true;
				break;
			}
		}

		// Scan CDL data for code regions and extract references
		// CDL flags: 0x01 = Code, 0x02 = Data, 0x04 = JumpTarget, 0x08 = SubEntryPoint, 0x10 = Opcode
		for (int i = 0; i < cdlSpan.Length; i++) {
			byte flags = cdlSpan[i];

			// Filter: only process instruction start positions
			// When OPCODE flags are available, skip operand bytes (only first byte of each instruction has 0x10)
			// Otherwise fall back to processing all CODE bytes
			if (useOpcodeFilter) {
				if ((flags & 0x10) == 0) continue;
			} else {
				if ((flags & 0x01) == 0) continue;
			}

			// Try to get disassembly at this address to find the target
			try {
				var output = DebugApi.GetDisassemblyOutput(cpuType, (uint)i, 1);
				if (output.Length > 0) {
					var line = output[0];

					// Check if this instruction references another address
					if (line.EffectiveAddress >= 0 && line.EffectiveAddress != i) {
						uint targetAddr = (uint)line.EffectiveAddress;

						// Determine xref type based on instruction and target
						PansyCrossRefType xrefType;
						if (functionSet.Contains(targetAddr)) {
							xrefType = PansyCrossRefType.Jsr;
						} else if (jumpTargetSet.Contains(targetAddr)) {
							// Check if it's a branch (short jump) or full jump
							xrefType = IsBranchInstruction(line.ByteCode, cpuType) ? PansyCrossRefType.Branch : PansyCrossRefType.Jmp;
						} else if (cdlSpan.Length > targetAddr && (cdlSpan[(int)targetAddr] & 0x02) != 0) {
							// Target is data - this is a read/write
							xrefType = IsWriteInstruction(line.ByteCode, cpuType) ? PansyCrossRefType.Write : PansyCrossRefType.Read;
						} else {
							xrefType = PansyCrossRefType.Jmp; // Default
						}

						// Optimized: Inline deduplication using HashSet
						var key = ((uint)i, targetAddr);
						if (seenXrefs.Add(key)) {
							xrefs.Add((key.Item1, key.Item2, xrefType, (byte)memType, (byte)line.EffectiveAddressType));
						}
					}

					// Conditional branches have two control-flow successors:
					// taken target and fallthrough (next instruction).
					if (line.OpSize > 0 && IsConditionalBranchInstruction(line.ByteCode, cpuType)) {
						uint fallthroughAddr = (uint)i + line.OpSize;
						if (fallthroughAddr < (uint)cdlSpan.Length) {
							var fallthroughKey = ((uint)i, fallthroughAddr);
							if (seenXrefs.Add(fallthroughKey)) {
								xrefs.Add((fallthroughKey.Item1, fallthroughKey.Item2, PansyCrossRefType.Branch, (byte)memType, (byte)memType));
							}
						}
					}

					// Skip past remaining bytes of this instruction to avoid redundant API calls
					if (line.OpSize > 1) {
						i += line.OpSize - 1;
					}
				}
			} catch (Exception ex) {
				Debug.WriteLine($"PansyExporter: Skipping address that failed to disassemble: {ex.Message}");
			}
		}

		// Write xrefs - already deduplicated
		// Pansy spec: No count prefix. Per xref: From(4)+To(4)+Type(1) = 9 bytes each
		using var ms = new MemoryStream(xrefs.Count * 9);
		using var writer = new BinaryWriter(ms);

		foreach (var xref in xrefs) {
			writer.Write(xref.From);               // Source address (4 bytes)
			writer.Write(xref.To);                 // Target address (4 bytes)
			writer.Write((byte)xref.Type);         // Type (1 byte) - CrossRefType
		}

		return ms.ToArray();
	}

	/// <summary>
	/// Detect the Pansy symbol type from a CodeLabel.
	/// Label=1, Constant=2, Enum=3, Struct=4, Macro=5, Local=6, Anonymous=7, InterruptVector=8, Function=9.
	/// </summary>
	private static byte DetectSymbolType(CodeLabel label, HashSet<uint> functionSet, HashSet<uint> interruptVectors) {
		uint addr = (uint)label.Address;

		// Check interrupt vectors (NMI, RESET, IRQ addresses)
		if (interruptVectors.Contains(addr))
			return 8; // InterruptVector

		// Check if address is a known function/subroutine entry point
		if (functionSet.Contains(addr))
			return 9; // Function

		// Check label name patterns for local labels (start with . or @)
		if (label.Label.StartsWith('.') || label.Label.StartsWith('@'))
			return 6; // Local

		// Check label name patterns for constants (ALL_CAPS convention)
		if (label.Label.Length > 2 && label.Label == label.Label.ToUpperInvariant() && !label.Label.Any(char.IsLower))
			return 2; // Constant

		// Default to Label
		return 1; // Label
	}

	/// <summary>
	/// Detect comment type from comment text.
	/// Inline=1, Block=2, Todo=3.
	/// </summary>
	private static byte DetectCommentType(string comment) {
		// Check for TODO/FIXME/HACK/NOTE markers
		if (comment.Contains("TODO", StringComparison.OrdinalIgnoreCase) ||
			comment.Contains("FIXME", StringComparison.OrdinalIgnoreCase) ||
			comment.Contains("HACK", StringComparison.OrdinalIgnoreCase))
			return 3; // Todo

		// Check for multi-line comments (block)
		if (comment.Contains('\n') || comment.Contains('\r'))
			return 2; // Block

		// Default to inline
		return 1; // Inline
	}

	/// <summary>
	/// Build metadata section (0x0008) with project info and timestamps.
	/// Matches Pansy spec: ProjectName, Author, Version, CreatedTimestamp, ModifiedTimestamp.
	/// </summary>
	private static byte[] BuildMetadataSection(RomInfo romInfo) {
		using var ms = new MemoryStream(256);
		using var writer = new BinaryWriter(ms);

		string projectName = romInfo.GetRomName();
		string author = ""; // Could be populated from config
		string version = "1.0";

		// Project name
		var nameBytes = Encoding.UTF8.GetBytes(projectName);
		writer.Write((ushort)nameBytes.Length);
		writer.Write(nameBytes);

		// Author
		var authorBytes = Encoding.UTF8.GetBytes(author);
		writer.Write((ushort)authorBytes.Length);
		writer.Write(authorBytes);

		// Version
		var versionBytes = Encoding.UTF8.GetBytes(version);
		writer.Write((ushort)versionBytes.Length);
		writer.Write(versionBytes);

		// Created timestamp (Unix seconds, int64)
		writer.Write(DateTimeOffset.UtcNow.ToUnixTimeSeconds());

		// Modified timestamp (Unix seconds, int64)
		writer.Write(DateTimeOffset.UtcNow.ToUnixTimeSeconds());

		return ms.ToArray();
	}

	/// <summary>
	/// Build CPU State section (0x0009) from CDL data.
	/// For SNES: Extracts IndexMode8 (X flag) and MemoryMode8 (M flag) per code byte.
	/// For GBA: Extracts THUMB mode flag per code byte.
	/// Only emits entries where CPU state is known (code bytes with platform-specific flags).
	/// Format: Address(u32) + Flags(u8) + DataBank(u8) + DirectPage(u16) + CpuMode(u8) = 9 bytes each.
	/// </summary>
	private static byte[] BuildCpuStateSection(byte[]? cdlData, RomInfo romInfo) {
		if (cdlData is null or { Length: 0 })
			return [];

		bool isSnes = romInfo.Format is RomFormat.Sfc or RomFormat.Spc;
		bool isGba = romInfo.Format is RomFormat.Gba;

		if (!isSnes && !isGba)
			return [];

		// Pre-scan to count entries (only code bytes with mode flags)
		int count = 0;
		for (int i = 0; i < cdlData.Length; i++) {
			byte cdl = cdlData[i];
			if ((cdl & 0x01) == 0) continue; // Not code

			if (isSnes && (cdl & (CDL_INDEX_MODE_8 | CDL_MEMORY_MODE_8)) != 0)
				count++;
			else if (isGba && (cdl & CDL_GBA_THUMB) != 0)
				count++;
		}

		if (count == 0)
			return [];

		// Build entries using direct byte array writes (2x faster than BinaryWriter)
		var result = new byte[count * 9];
		int offset = 0;

		for (int i = 0; i < cdlData.Length; i++) {
			byte cdl = cdlData[i];
			if ((cdl & 0x01) == 0) continue; // Not code

			if (isSnes) {
				bool indexMode8 = (cdl & CDL_INDEX_MODE_8) != 0;
				bool memoryMode8 = (cdl & CDL_MEMORY_MODE_8) != 0;
				if (!indexMode8 && !memoryMode8) continue;

				BitConverter.TryWriteBytes(result.AsSpan(offset), (uint)i); // Address
				byte flags = 0;
				if (indexMode8) flags |= 0x01;        // Bit 0: XFlag
				if (memoryMode8) flags |= 0x02;       // Bit 1: MFlag
				result[offset + 4] = flags;           // Flags
				result[offset + 5] = 0;               // DataBank (not available from CDL)
				result[offset + 6] = 0;               // DirectPage low (not available from CDL)
				result[offset + 7] = 0;               // DirectPage high
				result[offset + 8] = 0;               // CpuMode: Native65816
			} else if (isGba) {
				if ((cdl & CDL_GBA_THUMB) == 0) continue;

				BitConverter.TryWriteBytes(result.AsSpan(offset), (uint)i); // Address
				result[offset + 4] = 0;              // Flags (no M/X for GBA)
				result[offset + 5] = 0;              // DataBank (N/A for GBA)
				result[offset + 6] = 0;              // DirectPage low (N/A for GBA)
				result[offset + 7] = 0;              // DirectPage high
				result[offset + 8] = 3;              // CpuMode: THUMB
			}

			offset += 9;
		}

		return result;
	}

	/// <summary>
	/// Check if the bytecode represents a branch instruction (conditional jump).
	/// </summary>
	private static bool IsBranchInstruction(byte[] byteCode, CpuType cpuType) {
		if (byteCode is null or { Length: 0 }) return false;

		byte opcode = byteCode[0];

		return cpuType switch {
			// 6502/65816/65SC02: BCC, BCS, BEQ, BMI, BNE, BPL, BVC, BVS, BRA, BRL
			CpuType.Nes or CpuType.Snes or CpuType.Lynx => opcode is 0x10 or 0x30 or 0x50 or 0x70 or 0x90 or 0xB0 or 0xD0 or 0xF0 or 0x80 or 0x82,

			// Game Boy (Z80-like): JR cc, nn
			CpuType.Gameboy => opcode is 0x18 or 0x20 or 0x28 or 0x30 or 0x38,

			// Genesis (M68000): Bcc opcodes - high nibble 0x6 with condition
			CpuType.Genesis => byteCode.Length >= 2 && (opcode & 0xF0) == 0x60 && opcode != 0x60,

			// WonderSwan (V30MZ): Jcc short (0x70-0x7F), Jcc near (0x0F 0x80-0x8F)
			CpuType.Ws => opcode is >= 0x70 and <= 0x7F || (opcode == 0x0F && byteCode.Length >= 2 && byteCode[1] is >= 0x80 and <= 0x8F),

			// GBA (ARM/THUMB): Would need more complex detection
			_ => false
		};
	}

	/// <summary>
	/// Check if the bytecode represents a conditional branch (has fallthrough path).
	/// </summary>
	private static bool IsConditionalBranchInstruction(byte[] byteCode, CpuType cpuType) {
		if (byteCode is null or { Length: 0 }) return false;

		byte opcode = byteCode[0];

		return cpuType switch {
			// 6502/65816/65SC02: conditional branches only (exclude BRA/BRL)
			CpuType.Nes or CpuType.Snes or CpuType.Lynx => opcode is 0x10 or 0x30 or 0x50 or 0x70 or 0x90 or 0xB0 or 0xD0 or 0xF0,

			// Game Boy: JR cc, nn (exclude unconditional JR)
			CpuType.Gameboy => opcode is 0x20 or 0x28 or 0x30 or 0x38,

			// Genesis: Bcc where cc != always (BRA)
			CpuType.Genesis => byteCode.Length >= 2 && (opcode & 0xF0) == 0x60 && opcode != 0x60,

			// WonderSwan conditional branches
			CpuType.Ws => opcode is >= 0x70 and <= 0x7F || (opcode == 0x0F && byteCode.Length >= 2 && byteCode[1] is >= 0x80 and <= 0x8F),

			_ => false
		};
	}

	/// <summary>
	/// Check if the bytecode represents a write/store instruction.
	/// </summary>
	private static bool IsWriteInstruction(byte[] byteCode, CpuType cpuType) {
		if (byteCode is null or { Length: 0 }) return false;

		byte opcode = byteCode[0];

		return cpuType switch {
			// 6502/65816/65SC02: STA, STX, STY, STZ
			CpuType.Nes or CpuType.Snes or CpuType.Lynx => opcode is
				0x85 or 0x95 or 0x8D or 0x9D or 0x99 or 0x81 or 0x91 or // STA
				0x86 or 0x96 or 0x8E or // STX
				0x84 or 0x94 or 0x8C or // STY
				0x64 or 0x74 or 0x9C or 0x9E, // STZ (65816)

			// Game Boy: LD (address), reg
			CpuType.Gameboy => opcode is 0x02 or 0x12 or 0x22 or 0x32 or 0x70 or 0x71 or 0x72 or 0x73 or 0x74 or 0x75 or 0x77 or 0xE0 or 0xE2 or 0xEA,

			// Genesis (M68000): MOVE to memory addressing modes
			// MOVEa.x has opcodes where bits 8-6 indicate destination mode
			// Simplified: any MOVE with destination mode >= 2 (memory) is a write
			CpuType.Genesis => false, // M68000 MOVE encoding too complex for single-byte detection

			// WonderSwan (V30MZ): MOV [mem], reg patterns (0x88, 0x89, 0xA2, 0xA3)
			CpuType.Ws => opcode is 0x88 or 0x89 or 0xA2 or 0xA3,

			_ => false
		};
	}

	private sealed class SectionInfo {
		public uint Type;
		public uint Offset;
		public uint CompressedSize;
		public uint UncompressedSize;
	}

	/// <summary>
	/// Build Pansy BOOKMARKS section (0x000a) from BookmarkManager data.
	/// Format per entry: Address(u32) + Color(u8) + Name(u16 len + UTF-8 bytes)
	/// </summary>
	private static byte[] BuildBookmarksSection(CpuType cpuType) {
		var bookmarks = BookmarkManager.GetBookmarks(cpuType);
		if (bookmarks.Count == 0) return [];

		using var ms = new MemoryStream();
		using var bw = new BinaryWriter(ms);
		foreach (var bm in bookmarks) {
			bw.Write(bm.Address);
			bw.Write(bm.Color);
			byte[] nameBytes = Encoding.UTF8.GetBytes(bm.Name ?? "");
			bw.Write((ushort)nameBytes.Length);
			bw.Write(nameBytes);
		}
		return ms.ToArray();
	}
}
