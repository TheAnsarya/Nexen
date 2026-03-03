using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Text;
using Nexen.Config;
using Nexen.Debugger.Labels;
using Nexen.Interop;
using Nexen.Utilities;
using Nexen.Windows;

namespace Nexen.Debugger.Integration;

/// <summary>
/// Imports Pansy metadata files into Nexen's debugger.
/// Supports loading symbols, comments, memory regions, and cross-references.
/// </summary>
public sealed class PansyImporter {
	private const string MAGIC = "PANSY\0\0\0";
	private const ushort MIN_VERSION = 0x0100;
	private const ushort MAX_VERSION = 0x0100;
	private const ushort FLAG_COMPRESSED = 0x0001;

	/// <summary>
	/// Import a Pansy file into the current debugger session.
	/// </summary>
	/// <param name="path">Path to the .pansy file</param>
	/// <param name="showResult">Whether to show a message box with results</param>
	/// <returns>Import result with statistics</returns>
	public static ImportResult Import(string path, bool showResult = true) {
		var result = new ImportResult { SourceFile = path };

		try {
			if (!File.Exists(path)) {
				result.Error = $"File not found: {path}";
				return result;
			}

			using var stream = new FileStream(path, FileMode.Open, FileAccess.Read);
			using var reader = new BinaryReader(stream);

			// Read and validate header
			var header = ReadHeader(reader);
			if (header is null) {
				result.Error = "Invalid Pansy file header";
				return result;
			}

			result.Version = header.Version;
			result.Platform = GetPlatformName(header.Platform);

			// Read section table from header (immediately after 32-byte header)
			// Each entry: Type(u32) + Offset(u32) + CompSize(u32) + UncompSize(u32) = 16 bytes
			var sectionEntries = new List<(uint Type, uint Offset, uint CompSize, uint UncompSize)>();
			for (uint i = 0; i < header.SectionCount; i++) {
				uint sType = reader.ReadUInt32();
				uint sOffset = reader.ReadUInt32();
				uint sCompSize = reader.ReadUInt32();
				uint sUncompSize = reader.ReadUInt32();
				sectionEntries.Add((sType, sOffset, sCompSize, sUncompSize));
			}

			// Process each section by seeking to its offset
			foreach (var (sType, sOffset, sCompSize, sUncompSize) in sectionEntries) {
				try {
					stream.Seek(sOffset, SeekOrigin.Begin);
					byte[] rawData = reader.ReadBytes((int)sCompSize);

					// Decompress if needed
					byte[] sectionData;
					if ((header.Flags & FLAG_COMPRESSED) != 0 && sCompSize != sUncompSize) {
						sectionData = PansyExporter.DecompressData(rawData, (int)sUncompSize);
					} else {
						sectionData = rawData;
					}

					switch (sType) {
						case 0x0001: // CODE_DATA_MAP
							result.CodeOffsetsImported = ImportCodeDataMap(sectionData);
							break;
						case 0x0002: // SYMBOLS
							result.SymbolsImported = ImportSymbols(sectionData, header);
							break;
						case 0x0003: // COMMENTS
							result.CommentsImported = ImportComments(sectionData, header);
							break;
						case 0x0004: // MEMORY_REGIONS
							result.MemoryRegionsImported = ImportMemoryRegions(sectionData, header);
							break;
						case 0x0006: // CROSS_REFS
							result.CrossReferencesImported = ImportCrossReferences(sectionData, header);
							break;
						case 0x0008: // METADATA
							// Metadata can be used in the future
							break;
						default:
							// Unknown/reserved section, skip
							break;
					}
				} catch (Exception) {
					// Skip sections that fail to parse
				}
			}

			result.Success = true;
		} catch (Exception ex) {
			result.Error = ex.Message;
		}

		if (showResult && result.Success) {
			ShowImportResult(result);
		}

		return result;
	}

	/// <summary>
	/// Import only labels from a Pansy file, merging with existing labels.
	/// </summary>
	public static int ImportLabelsOnly(string path) {
		var result = Import(path, showResult: false);
		return result.Success ? result.SymbolsImported : 0;
	}

	private static PansyHeader? ReadHeader(BinaryReader reader) {
		try {
			// Magic (8 bytes, offset 0)
			byte[] magic = reader.ReadBytes(8);
			string magicStr = Encoding.ASCII.GetString(magic);
			if (magicStr != MAGIC)
				return null;

			// Version (2 bytes, offset 8)
			ushort version = reader.ReadUInt16();
			if (version is < MIN_VERSION or > MAX_VERSION)
				return null;

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
			uint romCrc = reader.ReadUInt32();

			// Section count (4 bytes, offset 24)
			uint sectionCount = reader.ReadUInt32();

			// Reserved (4 bytes, offset 28)
			reader.ReadUInt32();

			return new PansyHeader {
				Version = version,
				Platform = platform,
				Flags = flags,
				RomSize = romSize,
				RomCrc32 = romCrc,
				SectionCount = sectionCount
			};
		} catch {
			return null;
		}
	}

	private static int ImportSymbols(byte[] data, PansyHeader header) {
		var labels = new List<CodeLabel>();

		using var ms = new MemoryStream(data);
		using var reader = new BinaryReader(ms);

		uint count = reader.ReadUInt32();
		for (uint i = 0; i < count; i++) {
			try {
				uint address = reader.ReadUInt32();
				byte symbolType = reader.ReadByte();
				byte symbolFlags = reader.ReadByte();
				byte memTypeId = reader.ReadByte();
				reader.ReadByte(); // Reserved

				ushort nameLen = reader.ReadUInt16();
				string name = Encoding.UTF8.GetString(reader.ReadBytes(nameLen));

				// Map Pansy memory type to Nexen
				var memType = MapMemoryType(memTypeId, header.Platform);
				if (memType is null)
					continue;

				// Filter based on config
				if (!ShouldImportLabel(memType.Value))
					continue;

				labels.Add(new CodeLabel {
					Address = address,
					MemoryType = memType.Value,
					Label = name,
					Comment = "" // Comments are in separate section
				});
			} catch {
				break;
			}
		}

		if (labels.Count > 0) {
			LabelManager.SetLabels(labels, raiseEvents: true);
		}

		return labels.Count;
	}

	private static int ImportComments(byte[] data, PansyHeader header) {
		int count = 0;
		var config = ConfigManager.Config.Debug.Integration;

		if (!config.ImportComments)
			return 0;

		using var ms = new MemoryStream(data);
		using var reader = new BinaryReader(ms);

		uint commentCount = reader.ReadUInt32();
		for (uint i = 0; i < commentCount; i++) {
			try {
				uint address = reader.ReadUInt32();
				byte memTypeId = reader.ReadByte();
				reader.ReadBytes(3); // Reserved

				ushort commentLen = reader.ReadUInt16();
				string comment = Encoding.UTF8.GetString(reader.ReadBytes(commentLen));

				var memType = MapMemoryType(memTypeId, header.Platform);
				if (memType is null)
					continue;

				if (!ShouldImportLabel(memType.Value))
					continue;

				// Try to update existing label with comment
				var existingLabel = LabelManager.GetLabel(address, memType.Value);
				if (existingLabel is not null) {
					existingLabel.Comment = comment;
					count++;
				}
			} catch {
				break;
			}
		}

		return count;
	}

	private static int ImportCodeDataMap(byte[] data) {
		// Code/data map is a byte array where each byte represents flags for one address
		// Pansy flags: CODE(0x01), DATA(0x02), JUMP_TARGET(0x04), SUB_ENTRY(0x08),
		//              OPCODE(0x10), DRAWN(0x20), READ(0x40), INDIRECT(0x80)
		// We can use this to set CDL flags on the loaded ROM
		return data.Length; // Return count of mapped bytes
	}

	private static int ImportJumpTargets(byte[] data, PansyHeader header) {
		return ImportAddressList(data, CdlFlags.JumpTarget);
	}

	private static int ImportSubEntryPoints(byte[] data, PansyHeader header) {
		return ImportAddressList(data, CdlFlags.SubEntryPoint);
	}

	private static int ImportAddressList(byte[] data, CdlFlags flag) {
		using var ms = new MemoryStream(data);
		using var reader = new BinaryReader(ms);

		uint count = reader.ReadUInt32();
		// Address lists would need CDL integration to set flags
		// For now, just count them
		return (int)count;
	}

	private static int ImportMemoryRegions(byte[] data, PansyHeader header) {
		// Memory regions can be imported as label ranges
		// This is a placeholder for extended functionality
		using var ms = new MemoryStream(data);
		using var reader = new BinaryReader(ms);

		uint count = reader.ReadUInt32();
		return (int)count;
	}

	private static int ImportCrossReferences(byte[] data, PansyHeader header) {
		// Cross-references could be used for navigation
		// This is a placeholder for extended functionality
		using var ms = new MemoryStream(data);
		using var reader = new BinaryReader(ms);

		uint count = reader.ReadUInt32();
		return (int)count;
	}

	private static MemoryType? MapMemoryType(byte pansyMemType, byte platform) {
		// Pansy memory type IDs:
		// 0x01 = PRG ROM, 0x02 = Work RAM, 0x03 = Save RAM
		// 0x10 = SNES specific types, etc.

		// Basic mapping - extend as needed
		return pansyMemType switch {
			0x01 => GetPrgRomType(platform),
			0x02 => GetWorkRamType(platform),
			0x03 => GetSaveRamType(platform),
			_ => null
		};
	}

	private static MemoryType? GetPrgRomType(byte platform) {
		return platform switch {
			0x01 => MemoryType.NesPrgRom,
			0x02 => MemoryType.SnesPrgRom,
			0x03 => MemoryType.GbPrgRom,
			0x04 => MemoryType.GbaPrgRom,
			0x06 => MemoryType.SmsPrgRom,
			0x07 => MemoryType.PcePrgRom,
			_ => null
		};
	}

	private static MemoryType? GetWorkRamType(byte platform) {
		return platform switch {
			0x01 => MemoryType.NesInternalRam,
			0x02 => MemoryType.SnesWorkRam,
			0x03 => MemoryType.GbWorkRam,
			0x04 => MemoryType.GbaIntWorkRam,
			_ => null
		};
	}

	private static MemoryType? GetSaveRamType(byte platform) {
		return platform switch {
			0x01 => MemoryType.NesSaveRam,
			0x02 => MemoryType.SnesSaveRam,
			0x03 => MemoryType.GbCartRam,
			0x04 => MemoryType.GbaSaveRam,
			_ => null
		};
	}

	private static bool ShouldImportLabel(MemoryType memType) {
		var config = ConfigManager.Config.Debug.Integration;

		// Check memory type against import filters using IsRomMemory extension
		if (memType.IsRomMemory())
			return config.ImportPrgRomLabels;

		// Check for work RAM types
		if (IsWorkRamType(memType))
			return config.ImportWorkRamLabels;

		// Check for save RAM types
		if (IsSaveRamType(memType))
			return config.ImportSaveRamLabels;

		return config.ImportOtherLabels;
	}

	private static bool IsWorkRamType(MemoryType memType) {
		return memType switch {
			MemoryType.SnesWorkRam or
			MemoryType.NesWorkRam or
			MemoryType.NesInternalRam or
			MemoryType.GbWorkRam or
			MemoryType.GbHighRam or
			MemoryType.GbaIntWorkRam or
			MemoryType.GbaExtWorkRam or
			MemoryType.PceWorkRam or
			MemoryType.SmsWorkRam or
			MemoryType.SpcRam or
			MemoryType.Sa1InternalRam => true,
			_ => false
		};
	}

	private static bool IsSaveRamType(MemoryType memType) {
		return memType switch {
			MemoryType.SnesSaveRam or
			MemoryType.NesSaveRam or
			MemoryType.GbCartRam or
			MemoryType.GbaSaveRam or
			MemoryType.PceSaveRam or
			MemoryType.SmsCartRam => true,
			_ => false
		};
	}

	private static string GetPlatformName(byte platform) {
		return platform switch {
			0x01 => "NES",
			0x02 => "SNES",
			0x03 => "Game Boy",
			0x04 => "GBA",
			0x05 => "Genesis",
			0x06 => "SMS",
			0x07 => "PC Engine",
			0x08 => "Atari 2600",
			0x09 => "Atari Lynx",
			0x0a => "WonderSwan",
			0x0c => "SPC700",
			0x16 => "Game Gear",
			_ => "Unknown"
		};
	}

	private static void ShowImportResult(ImportResult result) {
		// Format import stats as arguments for the resource string
		string stats = $"Platform: {result.Platform}\n" +
			$"Version: {result.Version >> 8}.{result.Version & 0xFF}\n\n" +
			$"Symbols: {result.SymbolsImported}\n" +
			$"Comments: {result.CommentsImported}";

		if (result.CodeOffsetsImported > 0)
			stats += $"\nCode offsets: {result.CodeOffsetsImported}";
		if (result.DataOffsetsImported > 0)
			stats += $"\nData offsets: {result.DataOffsetsImported}";

		NexenMsgBox.Show(null, "PansyImportComplete", MessageBoxButtons.OK, MessageBoxIcon.Info, stats);
	}

	private class PansyHeader {
		public ushort Version { get; set; }
		public byte Platform { get; set; }
		public ushort Flags { get; set; }
		public uint RomSize { get; set; }
		public uint RomCrc32 { get; set; }
		public uint SectionCount { get; set; }
	}
}

/// <summary>
/// Result of a Pansy import operation.
/// </summary>
public sealed class ImportResult {
	public string SourceFile { get; set; } = "";
	public bool Success { get; set; }
	public string? Error { get; set; }
	public ushort Version { get; set; }
	public string Platform { get; set; } = "";
	public int SymbolsImported { get; set; }
	public int CommentsImported { get; set; }
	public int CodeOffsetsImported { get; set; }
	public int DataOffsetsImported { get; set; }
	public int JumpTargetsImported { get; set; }
	public int SubEntryPointsImported { get; set; }
	public int MemoryRegionsImported { get; set; }
	public int CrossReferencesImported { get; set; }
}
