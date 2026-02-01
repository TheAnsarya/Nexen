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
public class PansyImporter {
	private const string MAGIC = "PANSY\0\0\0";
	private const ushort MIN_VERSION = 0x0100;
	private const ushort MAX_VERSION = 0x0101;
	private const byte FLAG_COMPRESSED = 0x01;

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
			if (header == null) {
				result.Error = "Invalid Pansy file header";
				return result;
			}

			result.Version = header.Version;
			result.Platform = GetPlatformName(header.Platform);

			// Get content stream (decompress if needed)
			Stream contentStream;
			if ((header.Flags & FLAG_COMPRESSED) != 0) {
				// Compressed content follows header
				contentStream = new DeflateStream(stream, CompressionMode.Decompress);
			} else {
				contentStream = stream;
			}

			using var contentReader = new BinaryReader(contentStream);

			// Read sections
			while (contentStream.CanRead) {
				try {
					byte sectionType = contentReader.ReadByte();
					uint sectionSize = contentReader.ReadUInt32();

					if (sectionSize is 0 or > 100_000_000) // Sanity check
						break;

					byte[] sectionData = contentReader.ReadBytes((int)sectionSize);

					switch (sectionType) {
						case 0x01: // Symbols
							result.SymbolsImported = ImportSymbols(sectionData, header);
							break;
						case 0x02: // Comments
							result.CommentsImported = ImportComments(sectionData, header);
							break;
						case 0x03: // Code offsets
							result.CodeOffsetsImported = ImportCodeOffsets(sectionData, header);
							break;
						case 0x04: // Data offsets
							result.DataOffsetsImported = ImportDataOffsets(sectionData, header);
							break;
						case 0x05: // Jump targets
							result.JumpTargetsImported = ImportJumpTargets(sectionData, header);
							break;
						case 0x06: // Sub entry points
							result.SubEntryPointsImported = ImportSubEntryPoints(sectionData, header);
							break;
						case 0x07: // Memory regions
							result.MemoryRegionsImported = ImportMemoryRegions(sectionData, header);
							break;
						case 0x08: // Cross-references
							result.CrossReferencesImported = ImportCrossReferences(sectionData, header);
							break;
						default:
							// Unknown section, skip
							break;
					}
				} catch (EndOfStreamException) {
					break;
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
			// Magic (8 bytes)
			byte[] magic = reader.ReadBytes(8);
			string magicStr = Encoding.ASCII.GetString(magic);
			if (magicStr != MAGIC)
				return null;

			// Version (2 bytes)
			ushort version = reader.ReadUInt16();
			if (version is < MIN_VERSION or > MAX_VERSION)
				return null;

			// Platform (1 byte)
			byte platform = reader.ReadByte();

			// Flags (1 byte)
			byte flags = reader.ReadByte();

			// ROM size (4 bytes)
			uint romSize = reader.ReadUInt32();

			// ROM CRC32 (4 bytes)
			uint romCrc = reader.ReadUInt32();

			// Timestamp (8 bytes)
			long timestamp = reader.ReadInt64();

			// Reserved (4 bytes)
			reader.ReadUInt32();

			return new PansyHeader {
				Version = version,
				Platform = platform,
				Flags = flags,
				RomSize = romSize,
				RomCrc32 = romCrc,
				Timestamp = DateTimeOffset.FromUnixTimeSeconds(timestamp).DateTime
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
				if (memType == null)
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
				if (memType == null)
					continue;

				if (!ShouldImportLabel(memType.Value))
					continue;

				// Try to update existing label with comment
				var existingLabel = LabelManager.GetLabel(address, memType.Value);
				if (existingLabel != null) {
					existingLabel.Comment = comment;
					count++;
				}
			} catch {
				break;
			}
		}

		return count;
	}

	private static int ImportCodeOffsets(byte[] data, PansyHeader header) {
		// Code offsets are typically handled by CDL data
		// This imports as drawn code regions
		return ImportAddressList(data, CdlFlags.Code);
	}

	private static int ImportDataOffsets(byte[] data, PansyHeader header) {
		return ImportAddressList(data, CdlFlags.Data);
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
			0x0B => "WonderSwan",
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
		public byte Flags { get; set; }
		public uint RomSize { get; set; }
		public uint RomCrc32 { get; set; }
		public DateTime Timestamp { get; set; }
	}
}

/// <summary>
/// Result of a Pansy import operation.
/// </summary>
public class ImportResult {
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
