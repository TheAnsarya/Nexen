using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using Nexen.Config;
using Nexen.Debugger.Utilities;
using Nexen.Interop;
using Nexen.Utilities;

namespace Nexen.Debugger.Labels;
/// <summary>
/// Manages folder-based debug data storage for ROMs.
/// Each ROM gets a dedicated folder containing:
/// - metadata.pansy (universal format)
/// - labels.mlb (native Nexen labels)
/// - coverage.cdl (Code Data Logger)
/// - config.json (per-ROM configuration)
/// </summary>
public static class DebugFolderManager {
	private const string DEBUG_FOLDER_NAME = "Debug";
	private const string PANSY_FILENAME = "metadata.pansy";
	private const string MLB_FILENAME = "labels.mlb";
	private const string CDL_FILENAME = "coverage.cdl";
	private const string CONFIG_FILENAME = "config.json";
	private const string HISTORY_FOLDER = "history";

	/// <summary>
	/// Regex to match characters invalid in folder names.
	/// </summary>
	private static readonly Regex InvalidCharsRegex = new(
		$"[{Regex.Escape(new string(Path.GetInvalidFileNameChars()))}]",
		RegexOptions.Compiled
	);

	/// <summary>
	/// Get the base debug folder path (inside Nexen data directory).
	/// </summary>
	public static string GetDebugBasePath() {
		var config = ConfigManager.Config.Debug.Integration;
		if (!string.IsNullOrWhiteSpace(config.DebugFolderPath) && Directory.Exists(config.DebugFolderPath)) {
			return config.DebugFolderPath;
		}

		return Path.Combine(ConfigManager.HomeFolder, DEBUG_FOLDER_NAME);
	}

	/// <summary>
	/// Generate a safe folder name from the ROM name and CRC32.
	/// </summary>
	/// <param name="romInfo">ROM information</param>
	/// <returns>Sanitized folder name in format "RomName_CRC32"</returns>
	public static string GetRomFolderName(RomInfo romInfo) {
		// Get base name without extension
		string baseName = Path.GetFileNameWithoutExtension(romInfo.RomPath);

		// Sanitize the name (replace invalid chars with underscore)
		baseName = InvalidCharsRegex.Replace(baseName, "_");

		// Trim to reasonable length (max 100 chars)
		if (baseName.Length > 100) {
			baseName = baseName.Substring(0, 100);
		}

		// Calculate or get CRC32
		uint crc32 = PansyExporter.CalculateRomCrc32(romInfo);

		// Format: RomName_XXXXXXXX (8 hex digits)
		return $"{baseName}_{crc32:x8}";
	}

	/// <summary>
	/// Get the full path to the debug folder for a specific ROM.
	/// </summary>
	/// <param name="romInfo">ROM information</param>
	/// <returns>Full path to the ROM's debug folder</returns>
	public static string GetRomDebugFolder(RomInfo romInfo) {
		string basePath = GetDebugBasePath();
		string folderName = GetRomFolderName(romInfo);
		return Path.Combine(basePath, folderName);
	}

	/// <summary>
	/// Ensure the debug folder exists for the given ROM.
	/// Creates all necessary directories if they don't exist.
	/// </summary>
	/// <param name="romInfo">ROM information</param>
	/// <returns>Path to the created/existing folder</returns>
	public static string EnsureFolderExists(RomInfo romInfo) {
		string folderPath = GetRomDebugFolder(romInfo);

		if (!Directory.Exists(folderPath)) {
			Directory.CreateDirectory(folderPath);
			System.Diagnostics.Debug.WriteLine($"[DebugFolderManager] Created debug folder: {folderPath}");
		}

		return folderPath;
	}

	/// <summary>
	/// Get the path to the Pansy metadata file for a ROM.
	/// </summary>
	public static string GetPansyPath(RomInfo romInfo) {
		return Path.Combine(GetRomDebugFolder(romInfo), PANSY_FILENAME);
	}

	/// <summary>
	/// Get the path to the MLB labels file for a ROM.
	/// </summary>
	public static string GetMlbPath(RomInfo romInfo) {
		return Path.Combine(GetRomDebugFolder(romInfo), MLB_FILENAME);
	}

	/// <summary>
	/// Get the path to the CDL file for a ROM.
	/// </summary>
	public static string GetCdlPath(RomInfo romInfo) {
		return Path.Combine(GetRomDebugFolder(romInfo), CDL_FILENAME);
	}

	/// <summary>
	/// Get the path to the per-ROM config file.
	/// </summary>
	public static string GetConfigPath(RomInfo romInfo) {
		return Path.Combine(GetRomDebugFolder(romInfo), CONFIG_FILENAME);
	}

	/// <summary>
	/// Get the history folder path for version backups.
	/// </summary>
	public static string GetHistoryFolder(RomInfo romInfo) {
		return Path.Combine(GetRomDebugFolder(romInfo), HISTORY_FOLDER);
	}

	/// <summary>
	/// Export all debug files to the ROM's debug folder.
	/// </summary>
	/// <param name="romInfo">ROM information</param>
	/// <param name="memoryType">Memory type for CDL data</param>
	/// <returns>True if all exports succeeded</returns>
	public static bool ExportAllFiles(RomInfo romInfo, MemoryType memoryType) {
		var config = ConfigManager.Config.Debug.Integration;
		if (!config.UseFolderStorage) {
			// Fall back to single-file export (legacy behavior)
			Log.Info("[DebugFolderManager] UseFolderStorage=false, using legacy export");
			return ExportPansyOnly(romInfo, memoryType);
		}

		try {
			string folder = EnsureFolderExists(romInfo);
			Log.Info($"[DebugFolderManager] Exporting to folder: {folder}");

			// Create version backup if enabled
			if (config.KeepVersionHistory) {
				CreateVersionBackup(romInfo);
			}

			bool success = true;

			// 1. Export Pansy file
			string pansyPath = GetPansyPath(romInfo);
			Log.Info($"[DebugFolderManager] Exporting Pansy to: {pansyPath}");
			var options = new PansyExportOptions {
				IncludeMemoryRegions = config.PansyIncludeMemoryRegions,
				IncludeCrossReferences = config.PansyIncludeCrossReferences,
				IncludeDataBlocks = config.PansyIncludeDataBlocks,
				UseCompression = config.PansyUseCompression
			};
			success &= PansyExporter.Export(pansyPath, romInfo, memoryType, 0, options);
			Log.Info($"[DebugFolderManager] Pansy export result: {success}");

			// 2. Export MLB file if enabled
			if (config.SyncMlbFiles) {
				string mlbPath = GetMlbPath(romInfo);
				try {
					NexenLabelFile.Export(mlbPath);
					Log.Info($"[DebugFolderManager] MLB exported to: {mlbPath}");
				} catch (Exception ex) {
					Log.Error(ex, "[DebugFolderManager] MLB export failed");
					success = false;
				}
			}

			// 3. Export CDL file if enabled
			if (config.SyncCdlFiles) {
				string cdlPath = GetCdlPath(romInfo);
				success &= ExportCdlFile(cdlPath, memoryType);
				Log.Info($"[DebugFolderManager] CDL exported to: {cdlPath}");
			}

			// 4. Update config file with timestamp
			UpdateConfigFile(romInfo);

			Log.Info($"[DebugFolderManager] All exports complete. Success={success}");
			return success;
		} catch (Exception ex) {
			Log.Error(ex, "[DebugFolderManager] ExportAllFiles failed");
			return false;
		}
	}

	/// <summary>
	/// Export only the Pansy file (legacy single-file mode).
	/// </summary>
	private static bool ExportPansyOnly(RomInfo romInfo, MemoryType memoryType) {
		var config = ConfigManager.Config.Debug.Integration;
		string pansyPath = GetLegacyPansyPath(romInfo);
		var options = new PansyExportOptions {
			IncludeMemoryRegions = config.PansyIncludeMemoryRegions,
			IncludeCrossReferences = config.PansyIncludeCrossReferences,
			IncludeDataBlocks = config.PansyIncludeDataBlocks,
			UseCompression = config.PansyUseCompression
		};
		return PansyExporter.Export(pansyPath, romInfo, memoryType, 0, options);
	}

	/// <summary>
	/// Get the legacy (non-folder) Pansy file path.
	/// </summary>
	private static string GetLegacyPansyPath(RomInfo romInfo) {
		string dir = Path.GetDirectoryName(romInfo.RomPath) ?? "";
		string baseName = Path.GetFileNameWithoutExtension(romInfo.RomPath);
		return Path.Combine(dir, baseName + ".pansy");
	}

	/// <summary>
	/// Export CDL data to a file.
	/// </summary>
	private static bool ExportCdlFile(string path, MemoryType memoryType) {
		try {
			int size = DebugApi.GetMemorySize(memoryType);
			if (size <= 0) {
				return false;
			}

			CdlFlags[] cdlFlags = DebugApi.GetCdlData(0, (uint)size, memoryType);

			// Convert CdlFlags[] to byte[] for file storage
			byte[] cdlData = new byte[cdlFlags.Length];
			for (int i = 0; i < cdlFlags.Length; i++) {
				cdlData[i] = (byte)cdlFlags[i];
			}

			File.WriteAllBytes(path, cdlData);
			return true;
		} catch (Exception ex) {
			System.Diagnostics.Debug.WriteLine($"[DebugFolderManager] CDL export failed: {ex.Message}");
			return false;
		}
	}

	/// <summary>
	/// Create a version backup of the current Pansy file.
	/// </summary>
	private static void CreateVersionBackup(RomInfo romInfo) {
		string pansyPath = GetPansyPath(romInfo);
		if (!File.Exists(pansyPath)) {
			return;
		}

		try {
			string historyFolder = GetHistoryFolder(romInfo);
			if (!Directory.Exists(historyFolder)) {
				Directory.CreateDirectory(historyFolder);
			}

			// Generate timestamp filename
			string timestamp = DateTime.Now.ToString("yyyy-MM-dd_HHmmss", CultureInfo.InvariantCulture);
			string backupPath = Path.Combine(historyFolder, $"{timestamp}.pansy");

			File.Copy(pansyPath, backupPath, overwrite: false);

			// Clean up old backups
			CleanupOldBackups(romInfo);
		} catch (Exception ex) {
			System.Diagnostics.Debug.WriteLine($"[DebugFolderManager] Backup creation failed: {ex.Message}");
		}
	}

	/// <summary>
	/// Remove old version backups exceeding the configured limit.
	/// </summary>
	private static void CleanupOldBackups(RomInfo romInfo) {
		var config = ConfigManager.Config.Debug.Integration;
		string historyFolder = GetHistoryFolder(romInfo);

		if (!Directory.Exists(historyFolder)) {
			return;
		}

		try {
			var files = Directory.GetFiles(historyFolder, "*.pansy")
				.Select(f => new FileInfo(f))
				.OrderByDescending(f => f.CreationTime)
				.Skip(config.MaxHistoryEntries)
				.ToList();

			foreach (var file in files) {
				file.Delete();
			}
		} catch (Exception ex) {
			System.Diagnostics.Debug.WriteLine($"[DebugFolderManager] Cleanup failed: {ex.Message}");
		}
	}

	/// <summary>
	/// Update the per-ROM config file with export timestamp.
	/// </summary>
	private static void UpdateConfigFile(RomInfo romInfo) {
		try {
			string configPath = GetConfigPath(romInfo);
			var config = new {
				LastExport = DateTime.UtcNow.ToString("o"),
				RomName = Path.GetFileName(romInfo.RomPath),
				Platform = romInfo.Format.ToString(),
				Version = "1.0"
			};
#pragma warning disable IL2026 // Using 'JsonSerializer.Serialize' may break functionality when trimming - Nexen does not use trimming
#pragma warning disable IL3050 // Using 'JsonSerializer.Serialize' may break functionality when AOT compiling - Nexen does not use AOT
			string json = System.Text.Json.JsonSerializer.Serialize(config, new System.Text.Json.JsonSerializerOptions {
				WriteIndented = true
			});
#pragma warning restore IL3050
#pragma warning restore IL2026
			File.WriteAllText(configPath, json);
		} catch (Exception ex) {
			System.Diagnostics.Debug.WriteLine($"[DebugFolderManager] Config update failed: {ex.Message}");
		}
	}

	/// <summary>
	/// Import all debug files from the ROM's debug folder.
	/// </summary>
	/// <param name="romInfo">ROM information</param>
	/// <returns>True if any files were imported</returns>
	public static bool ImportFromFolder(RomInfo romInfo) {
		var config = ConfigManager.Config.Debug.Integration;
		string folder = GetRomDebugFolder(romInfo);

		if (!Directory.Exists(folder)) {
			return false;
		}

		bool imported = false;

		// 1. Import MLB file if exists and enabled
		if (config.AutoLoadMlbFiles && config.SyncMlbFiles) {
			string mlbPath = GetMlbPath(romInfo);
			if (File.Exists(mlbPath)) {
				NexenLabelFile.Import(mlbPath, showResult: false);
				imported = true;
			}
		}

		// 2. Import CDL file if exists and enabled
		if (config.AutoLoadCdlFiles && config.SyncCdlFiles) {
			string cdlPath = GetCdlPath(romInfo);
			if (File.Exists(cdlPath)) {
				ImportCdlFile(cdlPath, romInfo);
				imported = true;
			}
		}

		return imported;
	}

	/// <summary>
	/// Import CDL data from a file.
	/// </summary>
	private static void ImportCdlFile(string path, RomInfo romInfo) {
		try {
			var cpuType = romInfo.ConsoleType.GetMainCpuType();
			var memType = cpuType.GetPrgRomMemoryType();

			byte[] cdlData = File.ReadAllBytes(path);
			int memSize = DebugApi.GetMemorySize(memType);

			if (cdlData.Length == memSize) {
				DebugApi.SetCdlData(memType, cdlData, cdlData.Length);
			} else {
				System.Diagnostics.Debug.WriteLine($"[DebugFolderManager] CDL size mismatch: expected {memSize}, got {cdlData.Length}");
			}
		} catch (Exception ex) {
			System.Diagnostics.Debug.WriteLine($"[DebugFolderManager] CDL import failed: {ex.Message}");
		}
	}

	/// <summary>
	/// Check if a debug folder exists for the given ROM.
	/// </summary>
	public static bool DebugFolderExists(RomInfo romInfo) {
		return Directory.Exists(GetRomDebugFolder(romInfo));
	}

	/// <summary>
	/// Get a list of all ROMs with debug folders.
	/// </summary>
	public static List<string> GetAllDebugFolders() {
		string basePath = GetDebugBasePath();
		if (!Directory.Exists(basePath)) {
			return [];
		}

		return Directory.GetDirectories(basePath).ToList();
	}

	/// <summary>
	/// Delete the debug folder for a ROM.
	/// </summary>
	public static bool DeleteDebugFolder(RomInfo romInfo) {
		string folder = GetRomDebugFolder(romInfo);
		if (Directory.Exists(folder)) {
			try {
				Directory.Delete(folder, recursive: true);
				return true;
			} catch (Exception ex) {
				System.Diagnostics.Debug.WriteLine($"[DebugFolderManager] Delete failed: {ex.Message}");
			}
		}

		return false;
	}
}
