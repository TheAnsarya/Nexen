using System;
using System.Collections.Generic;
using System.IO;
using Nexen.Config;
using Nexen.Debugger.Labels;
using Nexen.Interop;
using Nexen.Utilities;
using Nexen.Windows;
using Pansy.Core;

namespace Nexen.Debugger.Integration;

/// <summary>
/// Imports Pansy metadata files into Nexen's debugger.
/// Uses Pansy.Core.PansyLoader for canonical binary format reading.
/// </summary>
public sealed class PansyImporter {
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

			// Use PansyLoader for canonical binary format reading
			var loader = PansyLoader.Load(path);

			result.Version = loader.Version;
			result.Platform = GetPlatformName(loader.Platform);

			// Import symbols
			var symbolEntries = loader.SymbolEntries;
			if (symbolEntries.Count > 0) {
				result.SymbolsImported = ImportSymbols(symbolEntries, loader.Platform);
			}

			// Import comments
			var commentEntries = loader.CommentEntries;
			if (commentEntries.Count > 0) {
				result.CommentsImported = ImportComments(commentEntries, loader.Platform);
			}

			// Import code/data map
			var codeDataMap = loader.CodeDataMapBytes;
			if (codeDataMap is not null && codeDataMap.Length > 0) {
				result.CodeOffsetsImported = codeDataMap.Length;
			}

			// Import memory regions
			if (loader.MemoryRegions.Count > 0) {
				result.MemoryRegionsImported = loader.MemoryRegions.Count;
			}

			// Import cross-references
			if (loader.CrossReferences.Count > 0) {
				result.CrossReferencesImported = loader.CrossReferences.Count;
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

	private static int ImportSymbols(IReadOnlyDictionary<int, SymbolEntry> symbolEntries, byte platform) {
		var labels = new List<CodeLabel>();

		foreach (var (address, symbol) in symbolEntries) {
			// Map platform to default ROM memory type
			var memType = GetPrgRomType(platform);
			if (memType is null)
				continue;

			// Filter based on config
			if (!ShouldImportLabel(memType.Value))
				continue;

			labels.Add(new CodeLabel {
				Address = (uint)address,
				MemoryType = memType.Value,
				Label = symbol.Name,
				Comment = ""
			});
		}

		if (labels.Count > 0) {
			LabelManager.SetLabels(labels, raiseEvents: true);
		}

		return labels.Count;
	}

	private static int ImportComments(IReadOnlyDictionary<int, CommentEntry> commentEntries, byte platform) {
		var config = ConfigManager.Config.Debug.Integration;

		if (!config.ImportComments)
			return 0;

		int count = 0;

		foreach (var (address, comment) in commentEntries) {
			var memType = GetPrgRomType(platform);
			if (memType is null)
				continue;

			if (!ShouldImportLabel(memType.Value))
				continue;

			// Try to update existing label with comment
			var existingLabel = LabelManager.GetLabel((uint)address, memType.Value);
			if (existingLabel is not null) {
				existingLabel.Comment = comment.Text;
				count++;
			}
		}

		return count;
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
