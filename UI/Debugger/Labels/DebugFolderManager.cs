using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
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
/// - labels.nexen-labels (native Nexen labels)
/// - coverage.cdl (Code Data Logger)
/// - config.json (per-ROM configuration)
/// </summary>
/// <remarks>
/// Path resolution order:
/// <list type="number">
///   <item>User-configured custom DebugFolderPath (if set)</item>
///   <item>New GameDataManager path: GameData/{System}/{RomName}_{CRC32}/Debug/</item>
///   <item>Legacy path (read-only fallback): Debug/{RomName}_{CRC32}/</item>
/// </list>
/// </remarks>
public static class DebugFolderManager {
	private const string LegacyDebugFolderName = "Debug";
	private const string PansyFilename = "metadata.pansy";
	private const string LabelsFilename = "labels.nexen-labels";
	private const string LegacyMlbFilename = "labels.mlb";
	private const string CdlFilename = "coverage.cdl";
	private const string ConfigFilename = "config.json";
	private const string HistoryFolderName = "history";

	/// <summary>
	/// Regex to match characters invalid in folder names.
	/// Used only for legacy folder name generation.
	/// </summary>
	private static readonly Regex InvalidCharsRegex = new(
		$"[{Regex.Escape(new string(Path.GetInvalidFileNameChars()))}]",
		RegexOptions.Compiled
	);

	/// <summary>
	/// Checks if the user has configured a custom debug folder path override.
	/// </summary>
	private static bool HasCustomDebugPath {
		get {
			var config = ConfigManager.Config.Debug.Integration;
			return !string.IsNullOrWhiteSpace(config.DebugFolderPath)
				&& Directory.Exists(config.DebugFolderPath);
		}
	}

	/// <summary>
	/// Get the legacy debug base path (pre-GameDataManager).
	/// Used for backwards-compatible reads and custom override paths.
	/// </summary>
	public static string GetLegacyDebugBasePath() {
		var config = ConfigManager.Config.Debug.Integration;
		if (HasCustomDebugPath) {
			return config.DebugFolderPath!;
		}

		return Path.Combine(ConfigManager.HomeFolder, LegacyDebugFolderName);
	}

	/// <summary>
	/// Generate a safe folder name from the ROM name and CRC32.
	/// Delegates to <see cref="GameDataManager.SanitizeRomName"/> for consistency.
	/// </summary>
	/// <param name="romInfo">ROM information</param>
	/// <returns>Sanitized folder name in format "RomName_CRC32"</returns>
	public static string GetRomFolderName(RomInfo romInfo) {
		string sanitizedName = GameDataManager.SanitizeRomName(romInfo.GetRomName());
		uint crc32 = RomHashService.ComputeRomHashes(romInfo).Crc32Value;
		return GameDataManager.FormatGameFolderName(sanitizedName, crc32);
	}

	/// <summary>
	/// Get the legacy debug folder path for a ROM (pre-GameDataManager layout).
	/// Path format: {LegacyDebugBase}/{RomName}_{CRC32}/
	/// </summary>
	private static string GetLegacyRomDebugFolder(RomInfo romInfo) {
		string basePath = GetLegacyDebugBasePath();
		string folderName = GetRomFolderName(romInfo);
		return Path.Combine(basePath, folderName);
	}

	/// <summary>
	/// Get the full path to the debug folder for a specific ROM.
	/// Uses GameDataManager path for default storage, custom path if configured.
	/// </summary>
	/// <param name="romInfo">ROM information</param>
	/// <returns>Full path to the ROM's debug folder</returns>
	public static string GetRomDebugFolder(RomInfo romInfo) {
		// If user has a custom debug path, use the legacy structure under it
		if (HasCustomDebugPath) {
			return GetLegacyRomDebugFolder(romInfo);
		}

		// Use new GameDataManager path: GameData/{System}/{RomName}_{CRC32}/Debug/
		return GameDataManager.GetDebugFolder(romInfo);
	}

	/// <summary>
	/// Resolve a debug file path, checking the new GameDataManager location first,
	/// then falling back to the legacy location for backwards compatibility.
	/// </summary>
	/// <param name="romInfo">ROM information</param>
	/// <param name="filename">The filename to resolve (e.g., "metadata.pansy")</param>
	/// <returns>Path in the new location (for writes), or legacy path if file exists there</returns>
	private static string ResolveDebugFilePath(RomInfo romInfo, string filename) {
		// Primary path (GameDataManager or custom)
		string primaryPath = Path.Combine(GetRomDebugFolder(romInfo), filename);

		// If file exists at primary location, use it
		if (File.Exists(primaryPath)) {
			return primaryPath;
		}

		// Check legacy location (only if not using custom path)
		if (!HasCustomDebugPath) {
			string legacyPath = Path.Combine(GetLegacyRomDebugFolder(romInfo), filename);
			if (File.Exists(legacyPath)) {
				return legacyPath;
			}
		}

		// Default to primary path (for new writes)
		return primaryPath;
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
		return Path.Combine(GetRomDebugFolder(romInfo), PansyFilename);
	}

	/// <summary>
	/// Get the path to the native .nexen-labels file for a ROM.
	/// </summary>
	public static string GetLabelsPath(RomInfo romInfo) {
		return Path.Combine(GetRomDebugFolder(romInfo), LabelsFilename);
	}

	/// <summary>
	/// Get the path to the legacy .mlb labels file for a ROM (read-only fallback).
	/// </summary>
	public static string GetLegacyMlbPath(RomInfo romInfo) {
		return Path.Combine(GetRomDebugFolder(romInfo), LegacyMlbFilename);
	}

	/// <summary>
	/// Get the path to the CDL file for a ROM.
	/// </summary>
	public static string GetCdlPath(RomInfo romInfo) {
		return Path.Combine(GetRomDebugFolder(romInfo), CdlFilename);
	}

	/// <summary>
	/// Get the path to the per-ROM config file.
	/// </summary>
	public static string GetConfigPath(RomInfo romInfo) {
		return Path.Combine(GetRomDebugFolder(romInfo), ConfigFilename);
	}

	/// <summary>
	/// Get the history folder path for version backups.
	/// </summary>
	public static string GetHistoryFolder(RomInfo romInfo) {
		return Path.Combine(GetRomDebugFolder(romInfo), HistoryFolderName);
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

			// 2. Export label file if enabled
			if (config.SyncLabelFiles) {
				string labelsPath = GetLabelsPath(romInfo);
				try {
					NexenLabelFile.Export(labelsPath, romInfo);
					Log.Info($"[DebugFolderManager] Labels exported to: {labelsPath}");
				} catch (Exception ex) {
					Log.Error(ex, "[DebugFolderManager] Label export failed");
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

		// Check if any debug folder exists (primary or legacy)
		string primaryFolder = GetRomDebugFolder(romInfo);
		string legacyFolder = HasCustomDebugPath ? "" : GetLegacyRomDebugFolder(romInfo);
		bool hasPrimary = Directory.Exists(primaryFolder);
		bool hasLegacy = !string.IsNullOrEmpty(legacyFolder) && Directory.Exists(legacyFolder);

		if (!hasPrimary && !hasLegacy) {
			return false;
		}

		bool imported = false;

		// 1. Import label file if exists and enabled
		if (config.AutoLoadLabelFiles && config.SyncLabelFiles) {
			// Try nexen-labels first, fall back to legacy MLB
			string labelsPath = ResolveDebugFilePath(romInfo, LabelsFilename);
			if (File.Exists(labelsPath)) {
				NexenLabelFile.Import(labelsPath, showResult: false);
				imported = true;
			} else {
				string mlbPath = ResolveDebugFilePath(romInfo, LegacyMlbFilename);
				if (File.Exists(mlbPath)) {
					NexenLabelFile.Import(mlbPath, showResult: false);
					imported = true;
				}
			}
		}

		// 2. Import CDL file if exists and enabled
		if (config.AutoLoadCdlFiles && config.SyncCdlFiles) {
			string cdlPath = ResolveDebugFilePath(romInfo, CdlFilename);
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
	/// Check if a debug folder exists for the given ROM (in either new or legacy location).
	/// </summary>
	public static bool DebugFolderExists(RomInfo romInfo) {
		if (Directory.Exists(GetRomDebugFolder(romInfo))) {
			return true;
		}

		// Also check legacy location
		if (!HasCustomDebugPath && Directory.Exists(GetLegacyRomDebugFolder(romInfo))) {
			return true;
		}

		return false;
	}

	/// <summary>
	/// Get a list of all ROMs with debug folders.
	/// </summary>
	public static List<string> GetAllDebugFolders() {
		var folders = new List<string>();

		// Scan legacy debug folder
		string legacyPath = GetLegacyDebugBasePath();
		if (Directory.Exists(legacyPath)) {
			folders.AddRange(Directory.GetDirectories(legacyPath));
		}

		// Scan GameData system folders for Debug subdirectories
		string gameDataRoot = GameDataManager.GameDataBasePath;
		if (Directory.Exists(gameDataRoot)) {
			foreach (string systemDir in Directory.GetDirectories(gameDataRoot)) {
				foreach (string romDir in Directory.GetDirectories(systemDir)) {
					string debugDir = Path.Combine(romDir, "Debug");
					if (Directory.Exists(debugDir)) {
						folders.Add(debugDir);
					}
				}
			}
		}

		return folders;
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
