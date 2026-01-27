using System;
using System.Buffers;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.IO.Hashing;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Mesen.Config;
using Mesen.Debugger.Labels;
using Mesen.Debugger.Utilities;
using Mesen.Interop;
using Mesen.Utilities;
using Mesen.Windows;

namespace Mesen.Debugger.Labels {
	/// <summary>
	/// Pansy file header structure for reading existing files.
	/// </summary>
	public record PansyHeader(ushort Version, byte Platform, byte Flags, uint RomSize, uint RomCrc32, long Timestamp);

	/// <summary>
	/// Cross-reference types for disassembly analysis.
	/// </summary>
	public enum CrossRefType : byte {
		Call = 1,
		Jump = 2,
		Read = 3,
		Write = 4,
		Branch = 5
	}

	/// <summary>
	/// Memory region types for export.
	/// </summary>
	public enum MemoryRegionType : byte {
		Unknown = 0,
		Code = 1,
		Data = 2,
		Ram = 3,
		Io = 4,
		Rom = 5,
		SaveRam = 6,
		WorkRam = 7,
		VideoRam = 8
	}

	/// <summary>
	/// Export options for Pansy file generation.
	/// </summary>
	public class PansyExportOptions {
		public bool IncludeMemoryRegions { get; set; } = true;
		public bool IncludeCrossReferences { get; set; } = true;
		public bool IncludeDataBlocks { get; set; } = true;
		public bool UseCompression { get; set; } = false;
		public int CompressionLevel { get; set; } = 6; // 1-9, default 6
	}

	/// <summary>
	/// Exports Mesen2 debugger data to Pansy metadata format.
	/// Pansy is a universal disassembly metadata format for retro game analysis.
	/// Phase 3: Memory regions, cross-references, data blocks
	/// Phase 4: GZip compression, async export, memory pooling
	/// </summary>
	public static class PansyExporter {
		// Pansy file format constants
		private const string MAGIC = "PANSY\0\0\0";
		private const ushort VERSION = 0x0101; // v1.1 - Phase 3/4 enhancements

		// Compression flag in header
		private const byte FLAG_COMPRESSED = 0x01;

		// Section types
		private const ushort SECTION_CODE_DATA_MAP = 0x0001;
		private const ushort SECTION_SYMBOLS = 0x0002;
		private const ushort SECTION_COMMENTS = 0x0003;
		private const ushort SECTION_MEMORY_REGIONS = 0x0004;
		private const ushort SECTION_DATA_BLOCKS = 0x0005;
		private const ushort SECTION_CROSS_REFS = 0x0006;
		private const ushort SECTION_JUMP_TARGETS = 0x0007;
		private const ushort SECTION_SUB_ENTRY_POINTS = 0x0008;
		private const ushort SECTION_BOOKMARKS = 0x0009;
		private const ushort SECTION_WATCH_ENTRIES = 0x000A;

		// Platform IDs matching Pansy specification
		private static readonly Dictionary<RomFormat, byte> PlatformIds = new() {
			[RomFormat.Sfc] = 0x02,      // SNES
			[RomFormat.Spc] = 0x0D,      // SPC700
			[RomFormat.iNes] = 0x01,     // NES
			[RomFormat.Fds] = 0x0E,      // FDS
			[RomFormat.Nsf] = 0x01,      // NSF (uses NES)
			[RomFormat.Gb] = 0x03,       // Game Boy
			[RomFormat.Gbs] = 0x03,      // GBS
			[RomFormat.Gba] = 0x04,      // GBA
			[RomFormat.Pce] = 0x07,      // PC Engine
			[RomFormat.Sms] = 0x06,      // SMS
			[RomFormat.GameGear] = 0x06, // Game Gear (SMS compatible)
			[RomFormat.Sg] = 0x06,       // SG-1000
			[RomFormat.Ws] = 0x0B,       // WonderSwan
		};

		/// <summary>
		/// Calculate CRC32 of the ROM data for integrity verification.
		/// Uses System.IO.Hashing.Crc32 for standard IEEE polynomial.
		/// </summary>
		/// <param name="romInfo">ROM information containing console type</param>
		/// <returns>CRC32 hash of the ROM data, or 0 if unavailable</returns>
		public static uint CalculateRomCrc32(RomInfo romInfo) {
			try {
				var cpuType = romInfo.ConsoleType.GetMainCpuType();
				var memType = cpuType.GetPrgRomMemoryType();
				byte[] romData = DebugApi.GetMemoryState(memType);
				if (romData is null or { Length: 0 })
					return 0;
				return Crc32.HashToUInt32(romData);
			} catch (Exception ex) {
				System.Diagnostics.Debug.WriteLine($"[PansyExporter] CRC32 calculation failed: {ex.Message}");
				return 0;
			}
		}

		/// <summary>
		/// Read the header from an existing Pansy file to get stored CRC.
		/// </summary>
		/// <param name="path">Path to the pansy file</param>
		/// <returns>PansyHeader if valid, null otherwise</returns>
		public static PansyHeader? ReadHeader(string path) {
			try {
				if (!File.Exists(path))
					return null;

				using var stream = new FileStream(path, FileMode.Open, FileAccess.Read);
				using var reader = new BinaryReader(stream);

				// Check magic (8 bytes)
				byte[] magic = reader.ReadBytes(8);
				if (System.Text.Encoding.ASCII.GetString(magic).TrimEnd('\0') != "PANSY")
					return null;

				// Read header fields
				ushort version = reader.ReadUInt16();
				byte platform = reader.ReadByte();
				byte flags = reader.ReadByte();
				uint romSize = reader.ReadUInt32();
				uint romCrc32 = reader.ReadUInt32();
				long timestamp = reader.ReadInt64();

				return new PansyHeader(version, platform, flags, romSize, romCrc32, timestamp);
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
				using var stream = new FileStream(path, FileMode.Create, FileAccess.Write, FileShare.None, 81920, FileOptions.Asynchronous);
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

				// Section 1: CODE_DATA_MAP (CDL data)
				if (cdlData is { Length: > 0 }) {
					var data = options.UseCompression ? CompressData(cdlData) : cdlData;
					sections.Add(new SectionInfo {
						Type = SECTION_CODE_DATA_MAP,
						CompressedSize = (uint)data.Length,
						UncompressedSize = (uint)cdlData.Length
					});
					sectionData.Add(data);
				}

				// Section 2: SYMBOLS
				var symbolBytes = BuildSymbolSection(labels);
				if (symbolBytes.Length > 0) {
					var data = options.UseCompression ? CompressData(symbolBytes) : symbolBytes;
					sections.Add(new SectionInfo {
						Type = SECTION_SYMBOLS,
						CompressedSize = (uint)data.Length,
						UncompressedSize = (uint)symbolBytes.Length
					});
					sectionData.Add(data);
				}

				// Section 3: COMMENTS
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

				// Section 4: JUMP_TARGETS
				if (jumpTargets.Length > 0) {
					var jtBytes = BuildAddressListSection(jumpTargets);
					var data = options.UseCompression ? CompressData(jtBytes) : jtBytes;
					sections.Add(new SectionInfo {
						Type = SECTION_JUMP_TARGETS,
						CompressedSize = (uint)data.Length,
						UncompressedSize = (uint)jtBytes.Length
					});
					sectionData.Add(data);
				}

				// Section 5: SUB_ENTRY_POINTS
				if (functions.Length > 0) {
					var subBytes = BuildAddressListSection(functions);
					var data = options.UseCompression ? CompressData(subBytes) : subBytes;
					sections.Add(new SectionInfo {
						Type = SECTION_SUB_ENTRY_POINTS,
						CompressedSize = (uint)data.Length,
						UncompressedSize = (uint)subBytes.Length
					});
					sectionData.Add(data);
				}

				// Section 6: MEMORY_REGIONS (Phase 3 - Enhanced Export)
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

				// Section 7: DATA_BLOCKS (Phase 3 - Enhanced Export)
				if (options.IncludeDataBlocks) {
					var dataBlocks = BuildDataBlocksSection(cdlData);
					if (dataBlocks.Length > 4) { // More than just the count
						var data = options.UseCompression ? CompressData(dataBlocks) : dataBlocks;
						sections.Add(new SectionInfo {
							Type = SECTION_DATA_BLOCKS,
							CompressedSize = (uint)data.Length,
							UncompressedSize = (uint)dataBlocks.Length
						});
						sectionData.Add(data);
					}
				}

				// Section 8: CROSS_REFS (Phase 3 - Enhanced Export)
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

				// Calculate section offsets
				uint currentOffset = 32 + 4 + (uint)(sections.Count * 16); // Header + section count + section table
				for (int i = 0; i < sections.Count; i++) {
					sections[i].Offset = currentOffset;
					currentOffset += sections[i].CompressedSize;
				}

				// Get ROM size and CRC
				var stats = DebugApi.GetCdlStatistics(memoryType);
				uint romSize = stats.TotalBytes;
				uint romCrc = romCrc32Override != 0 ? romCrc32Override : CalculateRomCrc32(romInfo);

				// Write header (32 bytes)
				byte flags = options.UseCompression ? FLAG_COMPRESSED : (byte)0;
				writer.Write(Encoding.ASCII.GetBytes(MAGIC)); // 8 bytes
				writer.Write(VERSION);                         // 2 bytes
				writer.Write(GetPlatformId(romInfo.Format));   // 1 byte
				writer.Write(flags);                           // Flags (compression) - 1 byte
				writer.Write(romSize);                         // ROM size - 4 bytes
				writer.Write(romCrc);                          // ROM CRC32 - 4 bytes
				writer.Write((ulong)DateTimeOffset.UtcNow.ToUnixTimeSeconds()); // Timestamp - 8 bytes
				writer.Write((uint)0);                         // Reserved - 4 bytes

				// Write section count
				writer.Write((uint)sections.Count);

				// Write section table
				foreach (var section in sections) {
					writer.Write(section.Type);
					writer.Write((ushort)0); // Reserved
					writer.Write(section.Offset);
					writer.Write(section.CompressedSize);
					writer.Write(section.UncompressedSize);
				}

				// Write section data
				foreach (var data in sectionData) {
					writer.Write(data);
				}

				// Write footer (12 bytes - CRC values)
				writer.Write(romCrc);                    // ROM CRC32
				writer.Write((uint)0);                   // Metadata CRC32 (placeholder)
				writer.Write((uint)0);                   // File CRC32 (placeholder)

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
		/// Compress data using GZip (Phase 4).
		/// </summary>
		private static byte[] CompressData(byte[] data) {
			if (data.Length < 64) // Don't compress small data
				return data;

			using var output = new MemoryStream();
			using (var gzip = new GZipStream(output, CompressionLevel.Optimal, leaveOpen: true)) {
				gzip.Write(data, 0, data.Length);
			}

			var compressed = output.ToArray();
			// Only use compressed if it's actually smaller
			return compressed.Length < data.Length ? compressed : data;
		}

		/// <summary>
		/// Decompress GZip data (Phase 4).
		/// </summary>
		public static byte[] DecompressData(byte[] compressedData, int uncompressedSize) {
			using var input = new MemoryStream(compressedData);
			using var gzip = new GZipStream(input, CompressionMode.Decompress);
			var output = new byte[uncompressedSize];
			int totalRead = 0;
			while (totalRead < uncompressedSize) {
				int read = gzip.Read(output, totalRead, uncompressedSize - totalRead);
				if (read == 0) break;
				totalRead += read;
			}
			return output;
		}

		/// <summary>
		/// Auto-export Pansy file when CDL is saved.
		/// Called from debugger shutdown or explicit save.
		/// </summary>
		/// <param name="romInfo">Current ROM information</param>
		/// <param name="memoryType">Memory type for CDL</param>
		public static void AutoExport(RomInfo romInfo, MemoryType memoryType) {
			System.Diagnostics.Debug.WriteLine($"[Pansy] AutoExport called - AutoExportPansy={ConfigManager.Config.Debug.Integration.AutoExportPansy}");

			if (!ConfigManager.Config.Debug.Integration.AutoExportPansy) {
				System.Diagnostics.Debug.WriteLine("[Pansy] Auto-export disabled, skipping");
				return;
			}

			string romName = romInfo.GetRomName();
			string pansyPath = GetPansyFilePath(romName);
			System.Diagnostics.Debug.WriteLine($"[Pansy] Target path: {pansyPath}");

			// Calculate current ROM CRC32
			uint currentCrc = CalculateRomCrc32(romInfo);
			System.Diagnostics.Debug.WriteLine($"[Pansy] Current ROM CRC32: {currentCrc:X8}");

			// Build export options from configuration
			var options = new PansyExportOptions {
				IncludeMemoryRegions = ConfigManager.Config.Debug.Integration.PansyIncludeMemoryRegions,
				IncludeCrossReferences = ConfigManager.Config.Debug.Integration.PansyIncludeCrossReferences,
				IncludeDataBlocks = ConfigManager.Config.Debug.Integration.PansyIncludeDataBlocks,
				UseCompression = ConfigManager.Config.Debug.Integration.PansyUseCompression
			};

			// Check if existing pansy file has matching CRC
			var existingHeader = ReadHeader(pansyPath);
			if (existingHeader != null) {
				System.Diagnostics.Debug.WriteLine($"[Pansy] Existing file CRC32: {existingHeader.RomCrc32:X8}");

				if (existingHeader.RomCrc32 != 0 && currentCrc != 0 && existingHeader.RomCrc32 != currentCrc) {
					// CRC mismatch - this is likely a hacked/translated ROM
					// Create a separate file with CRC suffix instead of overwriting
					string altPath = GetPansyFilePathWithCrc(romName, currentCrc);
					System.Diagnostics.Debug.WriteLine($"[Pansy] CRC MISMATCH! Creating separate file: {altPath}");
					bool success = Export(altPath, romInfo, memoryType, currentCrc, options);
					System.Diagnostics.Debug.WriteLine($"[Pansy] Export result: {success}");
					return;
				}
			}

			// Export normally (CRC matches or no existing file)
			bool exported = Export(pansyPath, romInfo, memoryType, currentCrc, options);
			System.Diagnostics.Debug.WriteLine($"[Pansy] Export result: {exported}");
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

		private static byte[]? GetCdlData(MemoryType memoryType) {
			try {
				int size = DebugApi.GetMemorySize(memoryType);
				if (size <= 0) return null;

				CdlFlags[] cdlData = DebugApi.GetCdlData(0, (uint)size, memoryType);
				byte[] data = new byte[cdlData.Length];
				for (int i = 0; i < cdlData.Length; i++) {
					data[i] = (byte)cdlData[i];
				}

				return data;
			} catch {
				return null;
			}
		}

		private static uint[] GetFunctions(MemoryType memoryType) {
			try {
				return DebugApi.GetCdlFunctions(memoryType);
			} catch {
				return [];
			}
		}

		private static uint[] GetJumpTargets(MemoryType memoryType) {
			try {
				// Get CDL data and extract jump targets
				byte[] cdl = GetCdlData(memoryType) ?? [];
				List<uint> targets = [];

				for (int i = 0; i < cdl.Length; i++) {
					if ((cdl[i] & 0x04) != 0) { // JumpTarget flag
						targets.Add((uint)i);
					}
				}

				return [.. targets];
			} catch {
				return [];
			}
		}

		private static byte[] BuildSymbolSection(List<CodeLabel> labels) {
			using var ms = new MemoryStream();
			using var writer = new BinaryWriter(ms);

			// Write symbol count
			var symbolLabels = labels.Where(l => !string.IsNullOrEmpty(l.Label)).ToList();
			writer.Write((uint)symbolLabels.Count);

			foreach (var label in symbolLabels) {
				// Address (4 bytes): 24-bit address + 8-bit flags
				uint addressWithFlags = (uint)label.Address;
				writer.Write(addressWithFlags);

				// Type (1 byte): 1 = label
				writer.Write((byte)1);

				// Flags (1 byte)
				writer.Write((byte)0);

				// Memory type (1 byte) - encode as custom byte
				writer.Write((byte)label.MemoryType);

				// Reserved (1 byte)
				writer.Write((byte)0);

				// Name length + name
				byte[] nameBytes = Encoding.UTF8.GetBytes(label.Label);
				writer.Write((ushort)nameBytes.Length);
				writer.Write(nameBytes);
			}

			return ms.ToArray();
		}

		private static byte[] BuildCommentSection(List<CodeLabel> labels) {
			using var ms = new MemoryStream();
			using var writer = new BinaryWriter(ms);

			// Write comment count
			var commentLabels = labels.Where(l => !string.IsNullOrEmpty(l.Comment)).ToList();
			writer.Write((uint)commentLabels.Count);

			foreach (var label in commentLabels) {
				// Address (4 bytes)
				writer.Write((uint)label.Address);

				// Type (1 byte): 1 = inline
				writer.Write((byte)1);

				// Memory type (1 byte)
				writer.Write((byte)label.MemoryType);

				// Reserved (2 bytes)
				writer.Write((ushort)0);

				// Comment length + text
				byte[] commentBytes = Encoding.UTF8.GetBytes(label.Comment);
				writer.Write((ushort)commentBytes.Length);
				writer.Write(commentBytes);
			}

			return ms.ToArray();
		}

		private static byte[] BuildAddressListSection(uint[] addresses) {
			using var ms = new MemoryStream();
			using var writer = new BinaryWriter(ms);

			writer.Write((uint)addresses.Length);
			foreach (var addr in addresses) {
				writer.Write(addr);
			}

			return ms.ToArray();
		}

		/// <summary>
		/// Build memory regions section from labels with length > 1.
		/// Phase 3: Enhanced data export.
		/// </summary>
		private static byte[] BuildMemoryRegionsSection(List<CodeLabel> labels, MemoryType memType) {
			using var ms = new MemoryStream();
			using var writer = new BinaryWriter(ms);

			// Memory regions are labels with length > 1
			var regions = labels.Where(l => l.Length > 1 && !string.IsNullOrEmpty(l.Label)).ToList();
			writer.Write((uint)regions.Count);

			foreach (var region in regions) {
				// Start address (4 bytes)
				writer.Write((uint)region.Address);

				// End address (4 bytes)
				writer.Write((uint)(region.Address + region.Length - 1));

				// Type (1 byte): 0=Unknown, 1=Code, 2=Data, 3=RAM, 4=IO
				byte regionType = 2; // Default to Data
				writer.Write(regionType);

				// Memory type (1 byte)
				writer.Write((byte)region.MemoryType);

				// Flags (2 bytes)
				writer.Write((ushort)0);

				// Name length + name
				byte[] nameBytes = Encoding.UTF8.GetBytes(region.Label);
				writer.Write((ushort)nameBytes.Length);
				writer.Write(nameBytes);
			}

			return ms.ToArray();
		}

		/// <summary>
		/// Build enhanced memory regions section including system memory maps.
		/// Phase 3: Includes ROM, RAM, VRAM, I/O regions with proper naming.
		/// </summary>
		private static byte[] BuildEnhancedMemoryRegionsSection(List<CodeLabel> labels, MemoryType memType, RomInfo romInfo) {
			using var ms = new MemoryStream();
			using var writer = new BinaryWriter(ms);

			List<(uint Start, uint End, string Name, MemoryRegionType Type, byte MemType)> allRegions = [];

			// Add system memory map based on console type
			AddSystemMemoryRegions(allRegions, romInfo);

			// Add user-defined regions from labels with length > 1
			foreach (var label in labels.Where(l => l.Length > 1 && !string.IsNullOrEmpty(l.Label))) {
				allRegions.Add((
					(uint)label.Address,
					(uint)(label.Address + label.Length - 1),
					label.Label,
					MemoryRegionType.Data,
					(byte)label.MemoryType
				));
			}

			// Sort by start address
			allRegions = [.. allRegions.OrderBy(r => r.Start)];

			writer.Write((uint)allRegions.Count);

			foreach (var region in allRegions) {
				writer.Write(region.Start);           // Start address (4 bytes)
				writer.Write(region.End);             // End address (4 bytes)
				writer.Write((byte)region.Type);      // Type (1 byte)
				writer.Write(region.MemType);         // Memory type (1 byte)
				writer.Write((ushort)0);              // Flags (2 bytes)

				byte[] nameBytes = Encoding.UTF8.GetBytes(region.Name);
				writer.Write((ushort)nameBytes.Length);
				writer.Write(nameBytes);
			}

			return ms.ToArray();
		}

		/// <summary>
		/// Add system memory regions based on console type.
		/// </summary>
		private static void AddSystemMemoryRegions(List<(uint Start, uint End, string Name, MemoryRegionType Type, byte MemType)> regions, RomInfo romInfo) {
			switch (romInfo.ConsoleType) {
				case ConsoleType.Nes:
					// NES Memory Map
					regions.Add((0x0000, 0x07FF, "RAM", MemoryRegionType.Ram, (byte)MemoryType.NesInternalRam));
					regions.Add((0x0800, 0x1FFF, "RAM_Mirrors", MemoryRegionType.Ram, (byte)MemoryType.NesInternalRam));
					regions.Add((0x2000, 0x2007, "PPU_Registers", MemoryRegionType.Io, (byte)MemoryType.NesMemory));
					regions.Add((0x4000, 0x4017, "APU_IO_Registers", MemoryRegionType.Io, (byte)MemoryType.NesMemory));
					regions.Add((0x4020, 0x5FFF, "Expansion_ROM", MemoryRegionType.Rom, (byte)MemoryType.NesMemory));
					regions.Add((0x6000, 0x7FFF, "SRAM", MemoryRegionType.SaveRam, (byte)MemoryType.NesSaveRam));
					regions.Add((0x8000, 0xFFFF, "PRG_ROM", MemoryRegionType.Rom, (byte)MemoryType.NesPrgRom));
					break;

				case ConsoleType.Snes:
					// SNES Memory Map (LoROM mode as default)
					regions.Add((0x0000, 0x1FFF, "LowRAM", MemoryRegionType.Ram, (byte)MemoryType.SnesMemory));
					regions.Add((0x2100, 0x21FF, "PPU_Registers", MemoryRegionType.Io, (byte)MemoryType.SnesMemory));
					regions.Add((0x4200, 0x43FF, "CPU_Registers", MemoryRegionType.Io, (byte)MemoryType.SnesMemory));
					regions.Add((0x7E0000, 0x7FFFFF, "WRAM", MemoryRegionType.WorkRam, (byte)MemoryType.SnesWorkRam));
					break;

				case ConsoleType.Gameboy:
					// Game Boy Memory Map
					regions.Add((0x0000, 0x3FFF, "ROM_Bank_0", MemoryRegionType.Rom, (byte)MemoryType.GbPrgRom));
					regions.Add((0x4000, 0x7FFF, "ROM_Bank_N", MemoryRegionType.Rom, (byte)MemoryType.GbPrgRom));
					regions.Add((0x8000, 0x9FFF, "VRAM", MemoryRegionType.VideoRam, (byte)MemoryType.GbVideoRam));
					regions.Add((0xA000, 0xBFFF, "External_RAM", MemoryRegionType.SaveRam, (byte)MemoryType.GbCartRam));
					regions.Add((0xC000, 0xDFFF, "WRAM", MemoryRegionType.WorkRam, (byte)MemoryType.GbWorkRam));
					regions.Add((0xFE00, 0xFE9F, "OAM", MemoryRegionType.Ram, (byte)MemoryType.GbSpriteRam));
					regions.Add((0xFF00, 0xFF7F, "IO_Registers", MemoryRegionType.Io, (byte)MemoryType.GameboyMemory));
					regions.Add((0xFF80, 0xFFFE, "HRAM", MemoryRegionType.Ram, (byte)MemoryType.GbHighRam));
					break;

				case ConsoleType.Gba:
					// GBA Memory Map
					regions.Add((0x00000000, 0x00003FFF, "BIOS", MemoryRegionType.Rom, (byte)MemoryType.GbaMemory));
					regions.Add((0x02000000, 0x0203FFFF, "EWRAM", MemoryRegionType.WorkRam, (byte)MemoryType.GbaMemory));
					regions.Add((0x03000000, 0x03007FFF, "IWRAM", MemoryRegionType.WorkRam, (byte)MemoryType.GbaIntWorkRam));
					regions.Add((0x04000000, 0x040003FF, "IO_Registers", MemoryRegionType.Io, (byte)MemoryType.GbaMemory));
					regions.Add((0x05000000, 0x050003FF, "Palette_RAM", MemoryRegionType.Ram, (byte)MemoryType.GbaPaletteRam));
					regions.Add((0x06000000, 0x06017FFF, "VRAM", MemoryRegionType.VideoRam, (byte)MemoryType.GbaVideoRam));
					regions.Add((0x07000000, 0x070003FF, "OAM", MemoryRegionType.Ram, (byte)MemoryType.GbaSpriteRam));
					regions.Add((0x08000000, 0x09FFFFFF, "ROM", MemoryRegionType.Rom, (byte)MemoryType.GbaPrgRom));
					regions.Add((0x0E000000, 0x0E00FFFF, "SRAM", MemoryRegionType.SaveRam, (byte)MemoryType.GbaSaveRam));
					break;

				case ConsoleType.PcEngine:
					// PC Engine Memory Map
					regions.Add((0x0000, 0x1FFF, "RAM", MemoryRegionType.Ram, (byte)MemoryType.PceMemory));
					regions.Add((0x1FE000, 0x1FFFFF, "Hardware_Registers", MemoryRegionType.Io, (byte)MemoryType.PceMemory));
					break;

				case ConsoleType.Sms:
					// Sega Master System Memory Map
					regions.Add((0x0000, 0xBFFF, "ROM", MemoryRegionType.Rom, (byte)MemoryType.SmsPrgRom));
					regions.Add((0xC000, 0xDFFF, "RAM", MemoryRegionType.Ram, (byte)MemoryType.SmsWorkRam));
					break;

				// Add more console types as needed
				default:
					break;
			}
		}

		/// <summary>
		/// Build data blocks section from CDL data.
		/// Phase 3: Enhanced data export - identifies contiguous data regions.
		/// </summary>
		private static byte[] BuildDataBlocksSection(byte[]? cdlData) {
			using var ms = new MemoryStream();
			using var writer = new BinaryWriter(ms);

			if (cdlData is null or { Length: 0 }) {
				writer.Write((uint)0);
				return ms.ToArray();
			}

			// Find contiguous data blocks (CDL flag 0x02 = Data)
			List<(uint Start, uint End)> blocks = [];
			int? blockStart = null;

			for (int i = 0; i < cdlData.Length; i++) {
				bool isData = (cdlData[i] & 0x02) != 0 && (cdlData[i] & 0x01) == 0; // Data flag, not code

				if (isData && blockStart == null) {
					blockStart = i;
				} else if (!isData && blockStart != null) {
					blocks.Add(((uint)blockStart.Value, (uint)(i - 1)));
					blockStart = null;
				}
			}

			// Handle final block
			if (blockStart != null) {
				blocks.Add(((uint)blockStart.Value, (uint)(cdlData.Length - 1)));
			}

			// Only write blocks larger than 4 bytes (filter noise)
			var significantBlocks = blocks.Where(b => b.End - b.Start >= 4).ToList();
			writer.Write((uint)significantBlocks.Count);

			foreach (var (Start, End) in significantBlocks) {
				writer.Write(Start);  // Start address (4 bytes)
				writer.Write(End);    // End address (4 bytes)
				writer.Write((byte)2);      // Type: Data
				writer.Write((byte)0);      // Flags
				writer.Write((ushort)0);    // Reserved
			}

			return ms.ToArray();
		}

		/// <summary>
		/// Build cross-references section from label data.
		/// Phase 3: Enhanced data export - tracks who references whom.
		/// </summary>
		private static byte[] BuildCrossRefsSection(List<CodeLabel> labels) {
			using var ms = new MemoryStream();
			using var writer = new BinaryWriter(ms);

			// For now, create cross-refs for labeled subroutines
			// Future: Could query actual disassembly for JSR/JMP/branch targets
			List<(uint From, uint To, byte Type)> xrefs = [];

			// Placeholder - in a full implementation, we would scan the disassembly
			// for instructions that reference labeled addresses
			writer.Write((uint)xrefs.Count);

			foreach (var xref in xrefs) {
				writer.Write(xref.From);    // Source address (4 bytes)
				writer.Write(xref.To);      // Target address (4 bytes)
				writer.Write(xref.Type);    // Type: 1=Call, 2=Jump, 3=Read, 4=Write
				writer.Write((byte)0);      // Memory type from
				writer.Write((byte)0);      // Memory type to
				writer.Write((byte)0);      // Flags
			}

			return ms.ToArray();
		}

		/// <summary>
		/// Build enhanced cross-references section by analyzing CDL and disassembly.
		/// Phase 3: Extracts actual cross-references from code analysis.
		/// </summary>
		private static byte[] BuildEnhancedCrossRefsSection(List<CodeLabel> labels, byte[]? cdlData, uint[] functions, uint[] jumpTargets, CpuType cpuType, MemoryType memType) {
			using var ms = new MemoryStream();
			using var writer = new BinaryWriter(ms);

			if (cdlData is null or { Length: 0 }) {
				writer.Write((uint)0);
				return ms.ToArray();
			}

			List<(uint From, uint To, CrossRefType Type, byte MemTypeFrom, byte MemTypeTo)> xrefs = [];

			// Build a set of known targets for quick lookup
			var functionSet = new HashSet<uint>(functions);
			var jumpTargetSet = new HashSet<uint>(jumpTargets);
			var labelAddresses = new HashSet<uint>(labels.Select(l => (uint)l.Address));

			// Scan CDL data for code regions and extract references
			// CDL flags: 0x01 = Code, 0x02 = Data, 0x04 = JumpTarget, 0x08 = SubEntryPoint
			for (int i = 0; i < cdlData.Length; i++) {
				byte flags = cdlData[i];
				bool isCode = (flags & 0x01) != 0;

				if (isCode) {
					// Try to get disassembly at this address to find the target
					try {
						var output = DebugApi.GetDisassemblyOutput(cpuType, (uint)i, 1);
						if (output.Length > 0) {
							var line = output[0];

							// Check if this instruction references another address
							if (line.EffectiveAddress >= 0 && line.EffectiveAddress != i) {
								uint targetAddr = (uint)line.EffectiveAddress;

								// Determine xref type based on instruction and target
								CrossRefType xrefType;
								if (functionSet.Contains(targetAddr)) {
									xrefType = CrossRefType.Call;
								} else if (jumpTargetSet.Contains(targetAddr)) {
									// Check if it's a branch (short jump) or full jump
									xrefType = IsBranchInstruction(line.ByteCode, cpuType) ? CrossRefType.Branch : CrossRefType.Jump;
								} else if ((cdlData.Length > targetAddr && (cdlData[targetAddr] & 0x02) != 0)) {
									// Target is data - this is a read/write
									xrefType = IsWriteInstruction(line.ByteCode, cpuType) ? CrossRefType.Write : CrossRefType.Read;
								} else {
									xrefType = CrossRefType.Jump; // Default
								}

								xrefs.Add(((uint)i, targetAddr, xrefType, (byte)memType, (byte)line.EffectiveAddressType));
							}
						}
					} catch {
						// Skip addresses that fail to disassemble
					}

					// Skip ahead based on instruction length (we'll still catch overlapping refs)
				}
			}

			// Deduplicate and write
			var uniqueXrefs = xrefs.DistinctBy(x => (x.From, x.To)).ToList();
			writer.Write((uint)uniqueXrefs.Count);

			foreach (var xref in uniqueXrefs) {
				writer.Write(xref.From);                 // Source address (4 bytes)
				writer.Write(xref.To);                   // Target address (4 bytes)
				writer.Write((byte)xref.Type);           // Type (1 byte)
				writer.Write(xref.MemTypeFrom);          // Memory type from (1 byte)
				writer.Write(xref.MemTypeTo);            // Memory type to (1 byte)
				writer.Write((byte)0);                   // Flags (1 byte)
			}

			return ms.ToArray();
		}

		/// <summary>
		/// Check if the bytecode represents a branch instruction (conditional jump).
		/// </summary>
		private static bool IsBranchInstruction(byte[] byteCode, CpuType cpuType) {
			if (byteCode is null or { Length: 0 }) return false;

			byte opcode = byteCode[0];

			return cpuType switch {
				// 6502/65816: BCC, BCS, BEQ, BMI, BNE, BPL, BVC, BVS, BRA, BRL
				CpuType.Nes or CpuType.Snes => opcode is 0x10 or 0x30 or 0x50 or 0x70 or 0x90 or 0xB0 or 0xD0 or 0xF0 or 0x80 or 0x82,

				// Game Boy (Z80-like): JR cc, nn
				CpuType.Gameboy => opcode is 0x18 or 0x20 or 0x28 or 0x30 or 0x38,

				// GBA (ARM/THUMB): Would need more complex detection
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
				// 6502/65816: STA, STX, STY, STZ
				CpuType.Nes or CpuType.Snes => opcode is
					0x85 or 0x95 or 0x8D or 0x9D or 0x99 or 0x81 or 0x91 or // STA
					0x86 or 0x96 or 0x8E or // STX
					0x84 or 0x94 or 0x8C or // STY
					0x64 or 0x74 or 0x9C or 0x9E, // STZ (65816)

				// Game Boy: LD (address), reg
				CpuType.Gameboy => opcode is 0x02 or 0x12 or 0x22 or 0x32 or 0x70 or 0x71 or 0x72 or 0x73 or 0x74 or 0x75 or 0x77 or 0xE0 or 0xE2 or 0xEA,

				_ => false
			};
		}

		private sealed class SectionInfo {
			public ushort Type;
			public uint Offset;
			public uint CompressedSize;
			public uint UncompressedSize;
		}
	}
}
