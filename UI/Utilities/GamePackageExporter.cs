using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Threading.Tasks;
using Nexen.Config;
using Nexen.Debugger.Labels;
using Nexen.Debugger.Utilities;
using Nexen.Interop;

namespace Nexen.Utilities;

/// <summary>
/// Exports a complete game package (.nexen-pack.zip) containing the ROM,
/// save states, CDL data, Pansy metadata, battery saves, screenshots, and
/// other per-game files into a single portable archive.
/// </summary>
/// <remarks>
/// Package contents (when present):
/// <list type="bullet">
///   <item>ROM file (original)</item>
///   <item>Save states (.nexen-save) — all found in per-ROM folder</item>
///   <item>Battery/SRAM saves (.sav, .srm, etc.)</item>
///   <item>CDL data (.cdl) — from debug folder</item>
///   <item>Pansy metadata (.pansy) — from debug folder</item>
///   <item>Labels (.nexen-labels) — from debug folder</item>
///   <item>Debug workspace (.json) — from debugger folder</item>
///   <item>Cheats (.json) — from cheat folder</item>
///   <item>Game config (.json) — from game config folder</item>
///   <item>Movie files (.nexen-movie) — any found for this ROM</item>
/// </list>
///
/// Archive structure:
/// <code>
/// {RomName}/
///     ROM/                    — Original ROM file
///     SaveStates/             — All save state files
///     Saves/                  — Battery/SRAM saves
///     Debug/                  — CDL, Pansy, labels
///     Movies/                 — Movie/TAS files
///     Config/                 — Cheats, game config, debug workspace
///     manifest.txt            — Package metadata
/// </code>
///
/// Output filename: <c>{RomName} ({yyyy-MM-dd-HH-mm-ss}).nexen-pack.zip</c>
/// Output folder: <c>{HomeFolder}/GamePacks/</c>
/// </remarks>
public static class GamePackageExporter {
	/// <summary>Default output folder name under HomeFolder</summary>
	private const string GamePacksFolderName = "GamePacks";

	/// <summary>File extension for game package archives</summary>
	public const string PackageExtension = ".nexen-pack.zip";

	/// <summary>
	/// Gets the output folder for game packages.
	/// Creates the directory if it doesn't exist.
	/// </summary>
	public static string GamePacksFolder {
		get {
			string path = Path.Combine(ConfigManager.HomeFolder, GamePacksFolderName);
			if (!Directory.Exists(path)) {
				Directory.CreateDirectory(path);
			}
			return path;
		}
	}

	/// <summary>
	/// Exports a complete game package for the currently loaded ROM.
	/// Runs the file collection and compression on a background thread.
	/// </summary>
	/// <returns>
	/// The full path to the created .nexen-pack.zip file,
	/// or null if no ROM is loaded or export failed.
	/// </returns>
	public static async Task<string?> ExportAsync() {
		RomInfo romInfo = EmuApi.GetRomInfo();
		if (string.IsNullOrEmpty(romInfo.RomPath)) {
			return null;
		}

		string romName = romInfo.GetRomName();
		string timestamp = DateTime.Now.ToString("yyyy-MM-dd-HH-mm-ss");
		string packageName = $"{romName} ({timestamp}){PackageExtension}";
		string outputPath = Path.Combine(GamePacksFolder, packageName);
		string archivePrefix = romName + "/";

		// Trigger a Pansy export before packaging so the file is up to date
		try {
			BackgroundPansyExporter.ForceExport();
		} catch (Exception ex) {
			// Non-fatal — continue without fresh Pansy export
			Debug.WriteLine($"GamePackageExporter: Pansy export failed: {ex.Message}");
		}

		// Collect all files on a background thread to avoid blocking UI
		return await Task.Run(() => {
			try {
				var filesToPack = CollectFiles(romInfo, romName);

				if (filesToPack.Count == 0) {
					return null;
				}

				using var stream = new FileStream(outputPath, FileMode.Create, FileAccess.Write);
				using var archive = new ZipArchive(stream, ZipArchiveMode.Create);

				// Add manifest first
				AddManifest(archive, archivePrefix, romInfo, romName, filesToPack);

				// Add all collected files
				foreach (var (sourcePath, archivePath) in filesToPack) {
					try {
						if (File.Exists(sourcePath)) {
							archive.CreateEntryFromFile(sourcePath, archivePrefix + archivePath, CompressionLevel.Optimal);
						}
					} catch (Exception ex) {
						// Skip individual files that can't be read (e.g., locked)
						Debug.WriteLine($"GamePackageExporter: Skipping file '{sourcePath}': {ex.Message}");
					}
				}

				return outputPath;
			} catch (Exception ex) {
				// Clean up partial file on failure
				Debug.WriteLine($"GamePackageExporter: Export failed: {ex.Message}");
				try {
					if (File.Exists(outputPath)) {
						File.Delete(outputPath);
					}
				} catch (Exception cleanupEx) {
					Debug.WriteLine($"GamePackageExporter: Cleanup failed: {cleanupEx.Message}");
				}
				return null;
			}
		});
	}

	/// <summary>
	/// Collects all per-game files that should be included in the package.
	/// </summary>
	/// <param name="romInfo">Current ROM info</param>
	/// <param name="romName">Sanitized ROM name</param>
	/// <returns>List of (source filesystem path, relative archive path) tuples</returns>
	private static List<(string SourcePath, string ArchivePath)> CollectFiles(RomInfo romInfo, string romName) {
		var files = new List<(string SourcePath, string ArchivePath)>();

		// 1. ROM file
		if (File.Exists(romInfo.RomPath)) {
			files.Add((romInfo.RomPath, "ROM/" + Path.GetFileName(romInfo.RomPath)));
		}

		// 2. Patch file (if any)
		if (!string.IsNullOrEmpty(romInfo.PatchPath) && File.Exists(romInfo.PatchPath)) {
			files.Add((romInfo.PatchPath, "ROM/" + Path.GetFileName(romInfo.PatchPath)));
		}

		// 3. Save states — from per-ROM GameData folder
		CollectFromFolder(files, "SaveStates/",
			GameDataManager.GetSaveStatesFolder(romInfo),
			"*.nexen-save");

		// Also check legacy save state folder
		CollectMatchingFiles(files, "SaveStates/",
			ConfigManager.SaveStateFolder,
			$"{romName}_*.nexen-save");
		CollectMatchingFiles(files, "SaveStates/",
			ConfigManager.SaveStateFolder,
			$"{romName}_*.mss");

		// 4. Battery/SRAM saves — from per-ROM GameData folder
		CollectFromFolder(files, "Saves/",
			GameDataManager.GetSavesFolder(romInfo),
			"*.*");

		// Also check legacy save folder
		string[] saveExtensions = ["*.sav", "*.srm", "*.rtc", "*.eeprom", "*.ieeprom", "*.chr.sav"];
		foreach (string ext in saveExtensions) {
			CollectMatchingFiles(files, "Saves/", ConfigManager.SaveFolder, $"{romName}{ext.TrimStart('*')}");
		}

		// 5. Debug folder — CDL, Pansy, Labels from GameData
		CollectFromFolder(files, "Debug/",
			GameDataManager.GetDebugFolder(romInfo),
			"*.*");

		// Also check legacy debugger folder
		string debuggerFile = Path.Combine(ConfigManager.DebuggerFolder, romName + ".cdl");
		if (File.Exists(debuggerFile)) {
			files.Add((debuggerFile, "Debug/" + Path.GetFileName(debuggerFile)));
		}
		debuggerFile = Path.Combine(ConfigManager.DebuggerFolder, romName + ".pansy");
		if (File.Exists(debuggerFile)) {
			files.Add((debuggerFile, "Debug/" + Path.GetFileName(debuggerFile)));
		}

		// 6. Debug workspace
		string workspacePath = Path.Combine(ConfigManager.DebuggerFolder, romName + ".json");
		if (File.Exists(workspacePath)) {
			files.Add((workspacePath, "Config/debug-workspace.json"));
		}

		// 7. Cheats
		string cheatPath = Path.Combine(ConfigManager.CheatFolder, romName + ".json");
		if (File.Exists(cheatPath)) {
			files.Add((cheatPath, "Config/cheats.json"));
		}

		// 8. Game config
		string gameConfigPath = Path.Combine(ConfigManager.GameConfigFolder, romName + ".json");
		if (File.Exists(gameConfigPath)) {
			files.Add((gameConfigPath, "Config/game-config.json"));
		}

		// 9. Movie files
		CollectMatchingFiles(files, "Movies/", ConfigManager.MovieFolder, $"{romName}*.nexen-movie");
		CollectMatchingFiles(files, "Movies/", ConfigManager.MovieFolder, $"{romName}*.mmo");

		// Deduplicate by archive path (prefer first occurrence)
		return files
			.GroupBy(f => f.ArchivePath, StringComparer.OrdinalIgnoreCase)
			.Select(g => g.First())
			.ToList();
	}

	/// <summary>
	/// Collects all files matching a pattern from a folder.
	/// </summary>
	private static void CollectFromFolder(
		List<(string SourcePath, string ArchivePath)> files,
		string archiveSubfolder,
		string folderPath,
		string pattern) {
		if (!Directory.Exists(folderPath)) return;

		try {
			foreach (string file in Directory.GetFiles(folderPath, pattern, SearchOption.TopDirectoryOnly)) {
				files.Add((file, archiveSubfolder + Path.GetFileName(file)));
			}
		} catch (Exception ex) {
			// Folder access error
			Debug.WriteLine($"GamePackageExporter.CollectFromFolder: Failed reading '{folderPath}': {ex.Message}");
		}
	}

	/// <summary>
	/// Collects files matching a specific pattern from a folder.
	/// </summary>
	private static void CollectMatchingFiles(
		List<(string SourcePath, string ArchivePath)> files,
		string archiveSubfolder,
		string folderPath,
		string pattern) {
		if (!Directory.Exists(folderPath)) return;

		try {
			foreach (string file in Directory.GetFiles(folderPath, pattern, SearchOption.TopDirectoryOnly)) {
				files.Add((file, archiveSubfolder + Path.GetFileName(file)));
			}
		} catch (Exception ex) {
			Debug.WriteLine($"GamePackageExporter.CollectMatchingFiles: Failed reading '{folderPath}': {ex.Message}");
		}
	}

	/// <summary>
	/// Adds a manifest.txt file to the archive with package metadata.
	/// </summary>
	private static void AddManifest(
		ZipArchive archive,
		string prefix,
		RomInfo romInfo,
		string romName,
		List<(string SourcePath, string ArchivePath)> files) {
		var entry = archive.CreateEntry(prefix + "manifest.txt", CompressionLevel.Optimal);
		using var writer = new StreamWriter(entry.Open());

		writer.WriteLine($"Nexen Game Package");
		writer.WriteLine($"Created: {DateTime.Now:yyyy-MM-dd HH:mm:ss}");
		writer.WriteLine($"Emulator: Nexen {EmuApi.GetNexenVersion()}");
		writer.WriteLine($"ROM: {romName}");
		writer.WriteLine($"System: {romInfo.ConsoleType}");
		writer.WriteLine($"ROM CRC32: {RomHashService.ComputeRomHashes(romInfo).Crc32Value:x8}");
		writer.WriteLine($"ROM File: {Path.GetFileName(romInfo.RomPath)}");
		writer.WriteLine();
		writer.WriteLine($"Files ({files.Count}):");
		foreach (var (_, archivePath) in files) {
			writer.WriteLine($"  {archivePath}");
		}
	}

	/// <summary>
	/// Opens the GamePacks output folder in the system file explorer.
	/// </summary>
	public static void OpenGamePacksFolder() {
		try {
			string folder = GamePacksFolder;
			if (OperatingSystem.IsWindows()) {
				System.Diagnostics.Process.Start("explorer.exe", folder);
			} else if (OperatingSystem.IsLinux()) {
				System.Diagnostics.Process.Start("xdg-open", folder);
			} else if (OperatingSystem.IsMacOS()) {
				System.Diagnostics.Process.Start("open", folder);
			}
		} catch {
			// Non-fatal
		}
	}
}
