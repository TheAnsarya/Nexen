using Mesen.Config;
using Mesen.Debugger.Labels;
using Mesen.Debugger.Utilities;
using Mesen.Interop;
using Mesen.Utilities;
using Mesen.Windows;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace Mesen.Debugger.Labels
{
	/// <summary>
	/// Pansy file header structure for reading existing files.
	/// </summary>
	public record PansyHeader(ushort Version, byte Platform, byte Flags, uint RomSize, uint RomCrc32, long Timestamp);

	/// <summary>
	/// Exports Mesen2 debugger data to Pansy metadata format.
	/// Pansy is a universal disassembly metadata format for retro game analysis.
	/// </summary>
	public static class PansyExporter
	{
		// Pansy file format constants
		private const string MAGIC = "PANSY\0\0\0";
		private const ushort VERSION = 0x0100; // v1.0

		// Section types
		private const ushort SECTION_CODE_DATA_MAP = 0x0001;
		private const ushort SECTION_SYMBOLS = 0x0002;
		private const ushort SECTION_COMMENTS = 0x0003;
		private const ushort SECTION_MEMORY_REGIONS = 0x0004;
		private const ushort SECTION_CROSS_REFS = 0x0006;
		private const ushort SECTION_JUMP_TARGETS = 0x0007;
		private const ushort SECTION_SUB_ENTRY_POINTS = 0x0008;

		// Platform IDs matching Pansy specification
		private static readonly Dictionary<RomFormat, byte> PlatformIds = new() {
			{ RomFormat.Sfc, 0x02 },    // SNES
			{ RomFormat.Spc, 0x0D },    // SPC700
			{ RomFormat.iNes, 0x01 },   // NES
			{ RomFormat.Fds, 0x0E },    // FDS
			{ RomFormat.Nsf, 0x01 },    // NSF (uses NES)
			{ RomFormat.Gb, 0x03 },     // Game Boy
			{ RomFormat.Gbs, 0x03 },    // GBS
			{ RomFormat.Gba, 0x04 },    // GBA
			{ RomFormat.Pce, 0x07 },    // PC Engine
			{ RomFormat.Sms, 0x06 },    // SMS
			{ RomFormat.GameGear, 0x06 }, // Game Gear (SMS compatible)
			{ RomFormat.Sg, 0x06 },     // SG-1000
			{ RomFormat.Ws, 0x0B },     // WonderSwan
		};

		// CRC32 lookup table (IEEE polynomial 0xEDB88320)
		private static readonly uint[] Crc32Table = InitCrc32Table();

		private static uint[] InitCrc32Table()
		{
			var table = new uint[256];
			for (uint i = 0; i < 256; i++) {
				uint crc = i;
				for (int j = 0; j < 8; j++) {
					crc = (crc & 1) != 0 ? (crc >> 1) ^ 0xEDB88320 : crc >> 1;
				}
				table[i] = crc;
			}
			return table;
		}

		private static uint ComputeCrc32(byte[] data)
		{
			uint crc = 0xFFFFFFFF;
			foreach (byte b in data) {
				crc = Crc32Table[(crc ^ b) & 0xFF] ^ (crc >> 8);
			}
			return ~crc;
		}

		/// <summary>
		/// Calculate CRC32 of the ROM data for integrity verification.
		/// </summary>
		/// <param name="romInfo">ROM information containing console type</param>
		/// <returns>CRC32 hash of the ROM data, or 0 if unavailable</returns>
		public static uint CalculateRomCrc32(RomInfo romInfo)
		{
			try {
				var cpuType = romInfo.ConsoleType.GetMainCpuType();
				var memType = cpuType.GetPrgRomMemoryType();
				byte[] romData = DebugApi.GetMemoryState(memType);
				if (romData == null || romData.Length == 0)
					return 0;
				return ComputeCrc32(romData);
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
		public static PansyHeader? ReadHeader(string path)
		{
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
		/// <returns>True if export succeeded</returns>
		public static bool Export(string path, RomInfo romInfo, MemoryType memoryType, uint romCrc32Override = 0)
		{
			try {
				using var stream = new FileStream(path, FileMode.Create, FileAccess.Write);
				using var writer = new BinaryWriter(stream);

				// Get all the data we need to export
				var labels = LabelManager.GetAllLabels();
				var cdlData = GetCdlData(memoryType);
				var functions = GetFunctions(memoryType);
				var jumpTargets = GetJumpTargets(memoryType);

				// Build sections
				var sections = new List<SectionInfo>();
				var sectionData = new List<byte[]>();

				// Section 1: CODE_DATA_MAP (CDL data)
				if (cdlData != null && cdlData.Length > 0) {
					sections.Add(new SectionInfo {
						Type = SECTION_CODE_DATA_MAP,
						UncompressedSize = (uint)cdlData.Length
					});
					sectionData.Add(cdlData);
				}

				// Section 2: SYMBOLS
				var symbolBytes = BuildSymbolSection(labels);
				if (symbolBytes.Length > 0) {
					sections.Add(new SectionInfo {
						Type = SECTION_SYMBOLS,
						UncompressedSize = (uint)symbolBytes.Length
					});
					sectionData.Add(symbolBytes);
				}

				// Section 3: COMMENTS
				var commentBytes = BuildCommentSection(labels);
				if (commentBytes.Length > 0) {
					sections.Add(new SectionInfo {
						Type = SECTION_COMMENTS,
						UncompressedSize = (uint)commentBytes.Length
					});
					sectionData.Add(commentBytes);
				}

				// Section 4: JUMP_TARGETS
				if (jumpTargets.Length > 0) {
					var jtBytes = BuildAddressListSection(jumpTargets);
					sections.Add(new SectionInfo {
						Type = SECTION_JUMP_TARGETS,
						UncompressedSize = (uint)jtBytes.Length
					});
					sectionData.Add(jtBytes);
				}

				// Section 5: SUB_ENTRY_POINTS
				if (functions.Length > 0) {
					var subBytes = BuildAddressListSection(functions);
					sections.Add(new SectionInfo {
						Type = SECTION_SUB_ENTRY_POINTS,
						UncompressedSize = (uint)subBytes.Length
					});
					sectionData.Add(subBytes);
				}

				// Calculate section offsets
				uint currentOffset = 32 + 4 + (uint)(sections.Count * 16); // Header + section count + section table
				for (int i = 0; i < sections.Count; i++) {
					sections[i].Offset = currentOffset;
					sections[i].CompressedSize = sections[i].UncompressedSize; // No compression for now
					currentOffset += sections[i].CompressedSize;
				}

				// Get ROM size and CRC
				var stats = DebugApi.GetCdlStatistics(memoryType);
				uint romSize = stats.TotalBytes;
				uint romCrc = romCrc32Override != 0 ? romCrc32Override : CalculateRomCrc32(romInfo);

				// Write header (32 bytes)
				writer.Write(Encoding.ASCII.GetBytes(MAGIC)); // 8 bytes
				writer.Write(VERSION);                         // 2 bytes
				writer.Write(GetPlatformId(romInfo.Format));   // 1 byte
				writer.Write((byte)0);                         // Flags (no compression) - 1 byte
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
		/// Auto-export Pansy file when CDL is saved.
		/// Called from debugger shutdown or explicit save.
		/// </summary>
		/// <param name="romInfo">Current ROM information</param>
		/// <param name="memoryType">Memory type for CDL</param>
		public static void AutoExport(RomInfo romInfo, MemoryType memoryType)
		{
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

			// Check if existing pansy file has matching CRC
			var existingHeader = ReadHeader(pansyPath);
			if (existingHeader != null) {
				System.Diagnostics.Debug.WriteLine($"[Pansy] Existing file CRC32: {existingHeader.RomCrc32:X8}");
				
				if (existingHeader.RomCrc32 != 0 && currentCrc != 0 && existingHeader.RomCrc32 != currentCrc) {
					// CRC mismatch - this is likely a hacked/translated ROM
					// Create a separate file with CRC suffix instead of overwriting
					string altPath = GetPansyFilePathWithCrc(romName, currentCrc);
					System.Diagnostics.Debug.WriteLine($"[Pansy] CRC MISMATCH! Creating separate file: {altPath}");
					bool success = Export(altPath, romInfo, memoryType, currentCrc);
					System.Diagnostics.Debug.WriteLine($"[Pansy] Export result: {success}");
					return;
				}
			}

			// Export normally (CRC matches or no existing file)
			bool exported = Export(pansyPath, romInfo, memoryType, currentCrc);
			System.Diagnostics.Debug.WriteLine($"[Pansy] Export result: {exported}");
		}

		/// <summary>
		/// Get the default Pansy file path for a ROM.
		/// </summary>
		public static string GetPansyFilePath(string romName)
		{
			string filename = Path.GetFileNameWithoutExtension(romName);
			return Path.Combine(ConfigManager.DebuggerFolder, $"{filename}.pansy");
		}

		/// <summary>
		/// Get Pansy file path with CRC suffix for mismatched ROMs.
		/// </summary>
		public static string GetPansyFilePathWithCrc(string romName, uint crc32)
		{
			string filename = Path.GetFileNameWithoutExtension(romName);
			return Path.Combine(ConfigManager.DebuggerFolder, $"{filename}_{crc32:x8}.pansy");
		}

		private static byte GetPlatformId(RomFormat format)
		{
			return PlatformIds.TryGetValue(format, out byte id) ? id : (byte)0xFF;
		}

		private static byte[]? GetCdlData(MemoryType memoryType)
		{
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

		private static uint[] GetFunctions(MemoryType memoryType)
		{
			try {
				return DebugApi.GetCdlFunctions(memoryType);
			} catch {
				return Array.Empty<uint>();
			}
		}

		private static uint[] GetJumpTargets(MemoryType memoryType)
		{
			try {
				// Get CDL data and extract jump targets
				byte[] cdl = GetCdlData(memoryType) ?? Array.Empty<byte>();
				var targets = new List<uint>();
				
				for (int i = 0; i < cdl.Length; i++) {
					if ((cdl[i] & 0x04) != 0) { // JumpTarget flag
						targets.Add((uint)i);
					}
				}
				
				return targets.ToArray();
			} catch {
				return Array.Empty<uint>();
			}
		}

		private static byte[] BuildSymbolSection(List<CodeLabel> labels)
		{
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

		private static byte[] BuildCommentSection(List<CodeLabel> labels)
		{
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

		private static byte[] BuildAddressListSection(uint[] addresses)
		{
			using var ms = new MemoryStream();
			using var writer = new BinaryWriter(ms);

			writer.Write((uint)addresses.Length);
			foreach (var addr in addresses) {
				writer.Write(addr);
			}

			return ms.ToArray();
		}

		private class SectionInfo
		{
			public ushort Type;
			public uint Offset;
			public uint CompressedSize;
			public uint UncompressedSize;
		}
	}
}
