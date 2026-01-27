using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using Mesen.Config;
using Mesen.Debugger.Labels;
using Mesen.Interop;

namespace Mesen.Debugger.Integration;

/// <summary>
/// Converts debug information from various assembler formats to Pansy metadata.
/// Supports ca65/cc65 DBG, WLA-DX symbols, RGBDS symbols, and SDCC symbols.
/// </summary>
public static class DbgToPansyConverter {
	/// <summary>
	/// Debug format types supported for conversion.
	/// </summary>
	public enum DebugFormat {
		Unknown,
		Ca65Dbg,      // ca65/cc65 debug format
		WlaDx,        // WLA-DX symbol file
		Rgbds,        // RGBDS symbol file
		Sdcc,         // SDCC symbol file
		Elf,          // ELF with symbols
		MesenMlb      // Mesen label file
	}

	/// <summary>
	/// Result of a conversion operation.
	/// </summary>
	public class ConversionResult {
		public bool Success { get; set; }
		public int SymbolsConverted { get; set; }
		public int CommentsExtracted { get; set; }
		public int SourceFilesFound { get; set; }
		public int MemoryRegionsFound { get; set; }
		public List<string> Warnings { get; } = new();
		public string? ErrorMessage { get; set; }
	}

	/// <summary>
	/// Convert a debug file to Pansy format and save it.
	/// </summary>
	/// <param name="debugFilePath">Path to the debug file (DBG, SYM, etc.)</param>
	/// <param name="pansyOutputPath">Path for the output Pansy file</param>
	/// <param name="romInfo">ROM information for context</param>
	/// <param name="memoryType">Memory type for address mapping</param>
	/// <returns>Result of the conversion</returns>
	public static ConversionResult ConvertAndExport(
		string debugFilePath,
		string pansyOutputPath,
		RomInfo romInfo,
		MemoryType memoryType) {
		var result = new ConversionResult();

		try {
			// Detect format
			var format = DetectFormat(debugFilePath);
			if (format == DebugFormat.Unknown) {
				result.ErrorMessage = $"Unknown debug file format: {Path.GetExtension(debugFilePath)}";
				return result;
			}

			// Load the debug file into the label manager
			// (The existing importers already do this well)
			bool imported = ImportDebugFile(debugFilePath, format, romInfo, result);
			if (!imported) {
				result.ErrorMessage = "Failed to import debug file";
				return result;
			}

			// Now export to Pansy format
			var options = new PansyExportOptions {
				IncludeMemoryRegions = true,
				IncludeCrossReferences = true,
				IncludeDataBlocks = true,
				UseCompression = ConfigManager.Config.Debug.Integration.PansyUseCompression
			};

			bool exported = PansyExporter.Export(pansyOutputPath, romInfo, memoryType, 0, options);
			if (!exported) {
				result.ErrorMessage = "Failed to export Pansy file";
				return result;
			}

			result.Success = true;
		} catch (Exception ex) {
			result.ErrorMessage = ex.Message;
		}

		return result;
	}

	/// <summary>
	/// Detect the debug file format from extension and content.
	/// </summary>
	public static DebugFormat DetectFormat(string filePath) {
		string ext = Path.GetExtension(filePath).ToLowerInvariant();

		switch (ext) {
			case ".dbg":
				return DebugFormat.Ca65Dbg;

			case ".sym":
				// Need to check content to distinguish WLA-DX vs RGBDS
				return DetectSymFormat(filePath);

			case ".mlb":
				return DebugFormat.MesenMlb;

			case ".elf":
				return DebugFormat.Elf;

			case ".cdb":
				return DebugFormat.Sdcc;

			default:
				return DebugFormat.Unknown;
		}
	}

	/// <summary>
	/// Distinguish between WLA-DX and RGBDS symbol formats.
	/// </summary>
	private static DebugFormat DetectSymFormat(string filePath) {
		try {
			string[] lines = File.ReadLines(filePath).Take(20).ToArray();

			foreach (string line in lines) {
				// RGBDS format starts with SECTION or defines like "DEF name EQU $value"
				if (line.StartsWith(";") || line.StartsWith("[")) {
					continue;
				}

				// WLA-DX has sections like [labels], [definitions], etc.
				if (line.StartsWith("[labels]") || line.StartsWith("[definitions]")) {
					return DebugFormat.WlaDx;
				}

				// RGBDS format: "SECTION: name bank addr"
				if (line.Contains(" SECTION:")) {
					return DebugFormat.Rgbds;
				}

				// WLA-DX format: "XX:XXXX symbolname"
				if (System.Text.RegularExpressions.Regex.IsMatch(line, @"^[0-9a-fA-F]{2,4}:[0-9a-fA-F]{4}\s+\w")) {
					return DebugFormat.WlaDx;
				}
			}

			// Default to WLA-DX if unclear
			return DebugFormat.WlaDx;
		} catch {
			return DebugFormat.Unknown;
		}
	}

	/// <summary>
	/// Import a debug file using the appropriate importer.
	/// </summary>
	private static bool ImportDebugFile(string filePath, DebugFormat format, RomInfo romInfo, ConversionResult result) {
		try {
			switch (format) {
				case DebugFormat.Ca65Dbg:
					return ImportCa65Dbg(filePath, romInfo, result);

				case DebugFormat.WlaDx:
					return ImportWlaDx(filePath, romInfo, result);

				case DebugFormat.MesenMlb:
					return ImportMesenMlb(filePath, romInfo, result);

				case DebugFormat.Sdcc:
					return ImportSdcc(filePath, romInfo, result);

				default:
					result.Warnings.Add($"Unsupported format: {format}");
					return false;
			}
		} catch (Exception ex) {
			result.Warnings.Add($"Import failed: {ex.Message}");
			return false;
		}
	}

	/// <summary>
	/// Import ca65/cc65 DBG format.
	/// </summary>
	private static bool ImportCa65Dbg(string filePath, RomInfo romInfo, ConversionResult result) {
		try {
			// Use the static Import method which creates the correct importer and imports
			var provider = DbgImporter.Import(
				romInfo.Format,
				filePath,
				importComments: true,
				showResult: false
			);

			if (provider != null) {
				var symbols = provider.GetSymbols();
				result.SymbolsConverted = symbols.Count;
				result.SourceFilesFound = provider.SourceFiles.Count;
				return symbols.Count > 0 || provider.SourceFiles.Count > 0;
			}

			result.Warnings.Add($"No DBG importer for format: {romInfo.Format}");
			return false;
		} catch (Exception ex) {
			result.Warnings.Add($"ca65 DBG import error: {ex.Message}");
			return false;
		}
	}

	/// <summary>
	/// Import WLA-DX symbol format.
	/// </summary>
	private static bool ImportWlaDx(string filePath, RomInfo romInfo, ConversionResult result) {
		var cpuType = romInfo.ConsoleType.GetMainCpuType();

		try {
			WlaDxImporter? provider = cpuType switch {
				CpuType.Snes or CpuType.Sa1 or CpuType.Spc => new SnesWlaDxImporter(),
				CpuType.Gameboy => new GbWlaDxImporter(),
				CpuType.Sms or CpuType.Gba => new SmsWlaDxImporter(),
				_ => null
			};

			if (provider != null) {
				provider.Import(filePath, showResult: false);
				var symbols = provider.GetSymbols();
				result.SymbolsConverted = symbols.Count;
				result.SourceFilesFound = provider.SourceFiles.Count;
				return symbols.Count > 0 || provider.SourceFiles.Count > 0;
			}

			result.Warnings.Add($"No WLA-DX importer for CPU type: {cpuType}");
			return false;
		} catch (Exception ex) {
			result.Warnings.Add($"WLA-DX import error: {ex.Message}");
			return false;
		}
	}

	/// <summary>
	/// Import Mesen MLB format.
	/// </summary>
	private static bool ImportMesenMlb(string filePath, RomInfo romInfo, ConversionResult result) {
		try {
			var cpuType = romInfo.ConsoleType.GetMainCpuType();
			int countBefore = LabelManager.GetLabels(cpuType).Count;
			MesenLabelFile.Import(filePath, showResult: false);
			int countAfter = LabelManager.GetLabels(cpuType).Count;
			result.SymbolsConverted = countAfter - countBefore;
			return true;
		} catch (Exception ex) {
			result.Warnings.Add($"MLB import error: {ex.Message}");
			return false;
		}
	}

	/// <summary>
	/// Import SDCC symbol format.
	/// </summary>
	private static bool ImportSdcc(string filePath, RomInfo romInfo, ConversionResult result) {
		try {
			var provider = SdccSymbolImporter.Import(romInfo.Format, filePath, showResult: false);

			if (provider != null) {
				var symbols = provider.GetSymbols();
				result.SymbolsConverted = symbols.Count;
				return symbols.Count > 0;
			}

			return false;
		} catch (Exception ex) {
			result.Warnings.Add($"SDCC import error: {ex.Message}");
			return false;
		}
	}

	/// <summary>
	/// Extract symbols from an ISymbolProvider and store them in Pansy-compatible format.
	/// </summary>
	public static List<PansySymbol> ExtractSymbols(ISymbolProvider provider) {
		var pansySymbols = new List<PansySymbol>();

		foreach (var symbol in provider.GetSymbols()) {
			var addr = provider.GetSymbolAddressInfo(symbol);
			if (addr.HasValue) {
				pansySymbols.Add(new PansySymbol {
					Name = symbol.Name ?? "",
					Address = (uint)addr.Value.Address,
					MemoryType = (byte)addr.Value.Type,
					Size = (uint)provider.GetSymbolSize(symbol)
				});
			}
		}

		return pansySymbols;
	}

	/// <summary>
	/// Represents a symbol in Pansy format.
	/// </summary>
	public class PansySymbol {
		public string Name { get; set; } = "";
		public uint Address { get; set; }
		public byte MemoryType { get; set; }
		public uint Size { get; set; }
	}

	/// <summary>
	/// Batch convert multiple debug files to Pansy format.
	/// </summary>
	/// <param name="debugFiles">List of debug file paths</param>
	/// <param name="outputFolder">Folder for output Pansy files</param>
	/// <param name="romInfo">ROM information</param>
	/// <param name="memoryType">Memory type for address mapping</param>
	/// <returns>List of conversion results</returns>
	public static List<ConversionResult> BatchConvert(
		IEnumerable<string> debugFiles,
		string outputFolder,
		RomInfo romInfo,
		MemoryType memoryType) {
		var results = new List<ConversionResult>();

		if (!Directory.Exists(outputFolder)) {
			Directory.CreateDirectory(outputFolder);
		}

		foreach (string debugFile in debugFiles) {
			string baseName = Path.GetFileNameWithoutExtension(debugFile);
			string pansyPath = Path.Combine(outputFolder, baseName + ".pansy");

			var result = ConvertAndExport(debugFile, pansyPath, romInfo, memoryType);
			results.Add(result);
		}

		return results;
	}
}
