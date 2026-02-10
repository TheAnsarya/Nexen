using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text.RegularExpressions;
using Nexen.Config;
using Nexen.Interop;

namespace Nexen.Utilities;

/// <summary>
/// Manages per-game data folder organization.
/// Provides a central service for resolving per-game folder paths based on
/// ROM metadata (system, name, CRC32).
/// </summary>
/// <remarks>
/// Folder structure:
/// <code>
/// {HomeFolder}/GameData/{System}/{RomName}_{crc32:x8}/
///     SaveStates/
///     Debug/
///     Saves/
/// </code>
/// </remarks>
public static class GameDataManager {
	private const string GameDataFolderName = "GameData";
	private const string SaveStatesFolderName = "SaveStates";
	private const string DebugFolderName = "Debug";
	private const string SavesFolderName = "Saves";

	/// <summary>
	/// Maximum length for sanitized ROM names in folder names.
	/// </summary>
	public const int MaxRomNameLength = 100;

	/// <summary>
	/// Regex to match characters invalid in file/folder names.
	/// </summary>
	private static readonly Regex InvalidCharsRegex = new(
		$"[{Regex.Escape(new string(Path.GetInvalidFileNameChars()))}]",
		RegexOptions.Compiled
	);

	/// <summary>
	/// Regex to collapse multiple consecutive underscores.
	/// </summary>
	private static readonly Regex MultipleUnderscoresRegex = new(
		@"_{2,}",
		RegexOptions.Compiled
	);

	/// <summary>
	/// Maps <see cref="ConsoleType"/> values to short system folder names
	/// matching the standard abbreviations (NES, SNES, GB, GBA, SMS, PCE, WS).
	/// </summary>
	/// <param name="system">The console type</param>
	/// <returns>A short uppercase system folder name</returns>
	public static string GetSystemFolderName(ConsoleType system) => system switch {
		ConsoleType.Snes => "SNES",
		ConsoleType.Gameboy => "GB",
		ConsoleType.Nes => "NES",
		ConsoleType.PcEngine => "PCE",
		ConsoleType.Sms => "SMS",
		ConsoleType.Gba => "GBA",
		ConsoleType.Ws => "WS",
		_ => system.ToString().ToUpperInvariant()
	};

	/// <summary>
	/// Sanitizes a ROM name for use as a folder name component.
	/// Removes invalid filesystem characters, collapses underscores,
	/// and truncates to <see cref="MaxRomNameLength"/> characters.
	/// </summary>
	/// <param name="romName">The raw ROM name (may include file extension)</param>
	/// <returns>A sanitized name safe for use in folder paths</returns>
	public static string SanitizeRomName(string romName) {
		if (string.IsNullOrWhiteSpace(romName))
			return "Unknown";

		// Remove file extension if present
		string name = Path.GetFileNameWithoutExtension(romName);

		// Replace invalid characters with underscore
		name = InvalidCharsRegex.Replace(name, "_");

		// Collapse multiple consecutive underscores
		name = MultipleUnderscoresRegex.Replace(name, "_");

		// Trim leading/trailing underscores and whitespace
		name = name.Trim('_', ' ');

		// Truncate to max length
		if (name.Length > MaxRomNameLength)
			name = name[..MaxRomNameLength];

		return string.IsNullOrEmpty(name) ? "Unknown" : name;
	}

	/// <summary>
	/// Formats a game folder name from a sanitized ROM name and CRC32 value.
	/// Format: <c>RomName_crc32hex</c> (8 lowercase hex digits).
	/// </summary>
	/// <param name="sanitizedRomName">A pre-sanitized ROM name</param>
	/// <param name="crc32">The ROM's CRC32 value</param>
	/// <returns>A folder name like <c>SuperMario_a1b2c3d4</c></returns>
	public static string FormatGameFolderName(string sanitizedRomName, uint crc32) {
		return $"{sanitizedRomName}_{crc32:x8}";
	}

	/// <summary>
	/// Gets the base path for all per-game data folders.
	/// Default: <c>{HomeFolder}/GameData/</c>
	/// </summary>
	public static string GameDataBasePath =>
		ConfigManager.GetFolder(Path.Combine(ConfigManager.HomeFolder, GameDataFolderName), null, false);

	/// <summary>
	/// Gets the root folder for a specific game's data.
	/// Path format: <c>{GameDataBasePath}/{System}/{RomName}_{crc32:x8}/</c>
	/// Creates the directory if it doesn't exist.
	/// </summary>
	/// <param name="system">The console type</param>
	/// <param name="romName">The ROM name (will be sanitized)</param>
	/// <param name="crc32">The ROM's CRC32 checksum</param>
	/// <returns>Full path to the game's data root folder</returns>
	public static string GetGameDataRoot(ConsoleType system, string romName, uint crc32) {
		string systemFolder = GetSystemFolderName(system);
		string gameFolder = FormatGameFolderName(SanitizeRomName(romName), crc32);
		string path = Path.Combine(GameDataBasePath, systemFolder, gameFolder);
		EnsureDirectoryExists(path);
		return path;
	}

	/// <summary>
	/// Gets the root folder for a specific game's data using ROM info.
	/// CRC32 is computed via <see cref="RomHashService"/>.
	/// </summary>
	/// <param name="romInfo">ROM information for the currently loaded game</param>
	/// <returns>Full path to the game's data root folder</returns>
	public static string GetGameDataRoot(RomInfo romInfo) {
		uint crc32 = RomHashService.ComputeRomHashes(romInfo).Crc32Value;
		return GetGameDataRoot(romInfo.ConsoleType, romInfo.GetRomName(), crc32);
	}

	/// <summary>
	/// Gets the save states subfolder for a specific game.
	/// Path format: <c>{GameDataRoot}/SaveStates/</c>
	/// </summary>
	public static string GetSaveStatesFolder(ConsoleType system, string romName, uint crc32) {
		string path = Path.Combine(GetGameDataRoot(system, romName, crc32), SaveStatesFolderName);
		EnsureDirectoryExists(path);
		return path;
	}

	/// <inheritdoc cref="GetSaveStatesFolder(ConsoleType, string, uint)"/>
	public static string GetSaveStatesFolder(RomInfo romInfo) {
		uint crc32 = RomHashService.ComputeRomHashes(romInfo).Crc32Value;
		return GetSaveStatesFolder(romInfo.ConsoleType, romInfo.GetRomName(), crc32);
	}

	/// <summary>
	/// Gets the debug data subfolder for a specific game.
	/// Path format: <c>{GameDataRoot}/Debug/</c>
	/// Contains Pansy metadata, labels, CDL files, etc.
	/// </summary>
	public static string GetDebugFolder(ConsoleType system, string romName, uint crc32) {
		string path = Path.Combine(GetGameDataRoot(system, romName, crc32), DebugFolderName);
		EnsureDirectoryExists(path);
		return path;
	}

	/// <inheritdoc cref="GetDebugFolder(ConsoleType, string, uint)"/>
	public static string GetDebugFolder(RomInfo romInfo) {
		uint crc32 = RomHashService.ComputeRomHashes(romInfo).Crc32Value;
		return GetDebugFolder(romInfo.ConsoleType, romInfo.GetRomName(), crc32);
	}

	/// <summary>
	/// Gets the battery/SRAM saves subfolder for a specific game.
	/// Path format: <c>{GameDataRoot}/Saves/</c>
	/// </summary>
	public static string GetSavesFolder(ConsoleType system, string romName, uint crc32) {
		string path = Path.Combine(GetGameDataRoot(system, romName, crc32), SavesFolderName);
		EnsureDirectoryExists(path);
		return path;
	}

	/// <inheritdoc cref="GetSavesFolder(ConsoleType, string, uint)"/>
	public static string GetSavesFolder(RomInfo romInfo) {
		uint crc32 = RomHashService.ComputeRomHashes(romInfo).Crc32Value;
		return GetSavesFolder(romInfo.ConsoleType, romInfo.GetRomName(), crc32);
	}

	/// <summary>
	/// Builds a game data path from components without creating directories or
	/// requiring ConfigManager. Useful for testing and path validation.
	/// </summary>
	/// <param name="basePath">The base folder path (e.g., HomeFolder/GameData)</param>
	/// <param name="system">The console type</param>
	/// <param name="romName">The ROM name (will be sanitized)</param>
	/// <param name="crc32">The ROM's CRC32 checksum</param>
	/// <param name="subfolder">Optional subfolder name (SaveStates, Debug, Saves)</param>
	/// <returns>The composed path</returns>
	public static string BuildPath(string basePath, ConsoleType system, string romName, uint crc32, string? subfolder = null) {
		string systemFolder = GetSystemFolderName(system);
		string gameFolder = FormatGameFolderName(SanitizeRomName(romName), crc32);
		string path = Path.Combine(basePath, systemFolder, gameFolder);
		return subfolder is not null ? Path.Combine(path, subfolder) : path;
	}

	/// <summary>
	/// Creates the directory if it doesn't exist.
	/// Logs a warning on failure but does not throw.
	/// </summary>
	private static void EnsureDirectoryExists(string path) {
		try {
			if (!Directory.Exists(path))
				Directory.CreateDirectory(path);
		} catch (Exception ex) {
			Debug.WriteLine($"[GameDataManager] Failed to create directory '{path}': {ex.Message}");
		}
	}

	/// <summary>
	/// Migrates save state files from the legacy flat folder to the per-game folder.
	/// Copies (not moves) files so legacy paths still work as fallback.
	/// </summary>
	/// <param name="romInfo">ROM information for the currently loaded game.</param>
	/// <returns>Number of files migrated.</returns>
	/// <remarks>
	/// <para>
	/// Legacy location: <c>{HomeFolder}/SaveStates/{RomName}/</c>
	/// New location: <c>{GameDataRoot}/SaveStates/</c>
	/// </para>
	/// <para>
	/// Only copies files that don't already exist at the destination.
	/// Supports both .nexen-save and legacy .mss extensions.
	/// </para>
	/// </remarks>
	public static int MigrateSaveStates(RomInfo romInfo) {
		try {
			string romName = romInfo.GetRomName();
			string legacyFolder = Path.Combine(ConfigManager.HomeFolder, "SaveStates", romName);

			if (!Directory.Exists(legacyFolder))
				return 0;

			string newFolder = GetSaveStatesFolder(romInfo);
			int migrated = 0;

			foreach (string sourceFile in Directory.GetFiles(legacyFolder)) {
				string ext = Path.GetExtension(sourceFile).ToLowerInvariant();
				if (ext is not ".nexen-save" and not ".mss")
					continue;

				string destFile = Path.Combine(newFolder, Path.GetFileName(sourceFile));
				if (File.Exists(destFile))
					continue;

				try {
					File.Copy(sourceFile, destFile, overwrite: false);
					migrated++;
				} catch (Exception ex) {
					Log.Warn($"[GameDataManager] Failed to migrate save state '{sourceFile}': {ex.Message}");
				}
			}

			if (migrated > 0) {
				Log.Info($"[GameDataManager] Migrated {migrated} save state(s) for '{romName}' to per-game folder");
			}

			return migrated;
		} catch (Exception ex) {
			Log.Warn($"[GameDataManager] Save state migration failed: {ex.Message}");
			return 0;
		}
	}

	/// <summary>
	/// Migrates legacy battery save files to the new per-game folder structure.
	/// </summary>
	/// <param name="romInfo">ROM information for locating the game folder.</param>
	/// <returns>Number of files migrated.</returns>
	/// <remarks>
	/// <para>
	/// Copies (non-destructive) battery saves from the legacy <c>{HomeFolder}/Saves/</c> folder
	/// to the new <c>GameData/{System}/{RomName}_{CRC32}/Saves/</c> structure.
	/// Only copies files that don't already exist at the destination.
	/// Supports all battery save extensions: .sav, .srm, .rtc, .eeprom, .ieeprom, .bs, .chr.sav
	/// </para>
	/// </remarks>
	public static int MigrateBatterySaves(RomInfo romInfo) {
		try {
			string romName = romInfo.GetRomName();
			string legacyFolder = Path.Combine(ConfigManager.HomeFolder, "Saves");

			if (!Directory.Exists(legacyFolder))
				return 0;

			string newFolder = GetSavesFolder(romInfo);
			int migrated = 0;

			// Battery save extensions used across all console cores
			HashSet<string> batteryExtensions = [".sav", ".srm", ".rtc", ".eeprom", ".ieeprom", ".bs"];

			foreach (string sourceFile in Directory.GetFiles(legacyFolder)) {
				string fileName = Path.GetFileName(sourceFile);

				// Battery saves are stored as {RomName}{ext} — check if this file belongs to our ROM
				// Handle compound extensions like .chr.sav
				bool matchesRom = false;
				string ext = "";

				if (fileName.StartsWith(romName, StringComparison.OrdinalIgnoreCase)) {
					ext = fileName[romName.Length..].ToLowerInvariant();
					matchesRom = batteryExtensions.Contains(ext) || ext == ".chr.sav";
				}

				if (!matchesRom)
					continue;

				string destFile = Path.Combine(newFolder, fileName);
				if (File.Exists(destFile))
					continue;

				try {
					File.Copy(sourceFile, destFile, overwrite: false);
					migrated++;
				} catch (Exception ex) {
					Log.Warn($"[GameDataManager] Failed to migrate battery save '{sourceFile}': {ex.Message}");
				}
			}

			if (migrated > 0) {
				Log.Info($"[GameDataManager] Migrated {migrated} battery save(s) for '{romName}' to per-game folder");
			}

			return migrated;
		} catch (Exception ex) {
			Log.Warn($"[GameDataManager] Battery save migration failed: {ex.Message}");
			return 0;
		}
	}

	/// <summary>
	/// Gets the count of legacy files that have already been migrated to per-game folders
	/// and can be cleaned up. Scans all system folders in GameData/ and checks for
	/// corresponding legacy files.
	/// </summary>
	/// <returns>A tuple of (cleanable legacy file count, total migrated game count).</returns>
	public static (int LegacyFileCount, int GameCount) GetCleanupStatus() {
		try {
			string gameDataBase = GameDataBasePath;
			if (!Directory.Exists(gameDataBase))
				return (0, 0);

			int legacyFiles = 0;
			int gameCount = 0;

			// Iterate all system folders (NES, SNES, GB, etc.)
			foreach (string systemDir in Directory.GetDirectories(gameDataBase)) {
				// Iterate all per-game folders ({RomName}_{crc32})
				foreach (string gameDir in Directory.GetDirectories(systemDir)) {
					string gameFolderName = Path.GetFileName(gameDir);

					// Extract ROM name from folder name (everything before last _xxxxxxxx)
					int lastUnderscore = gameFolderName.LastIndexOf('_');
					if (lastUnderscore < 0)
						continue;

					string romName = gameFolderName[..lastUnderscore];
					bool hasLegacyFiles = false;

					// Check legacy SaveStates folder
					string legacySaveStates = Path.Combine(ConfigManager.HomeFolder, "SaveStates", romName);
					if (Directory.Exists(legacySaveStates)) {
						string newSaveStates = Path.Combine(gameDir, SaveStatesFolderName);
						if (Directory.Exists(newSaveStates)) {
							foreach (string file in Directory.GetFiles(legacySaveStates)) {
								string ext = Path.GetExtension(file).ToLowerInvariant();
								if (ext is ".nexen-save" or ".mss") {
									string destFile = Path.Combine(newSaveStates, Path.GetFileName(file));
									if (File.Exists(destFile)) {
										legacyFiles++;
										hasLegacyFiles = true;
									}
								}
							}
						}
					}

					// Check legacy Saves folder for battery files
					string legacySaves = Path.Combine(ConfigManager.HomeFolder, "Saves");
					if (Directory.Exists(legacySaves)) {
						HashSet<string> batteryExtensions = [".sav", ".srm", ".rtc", ".eeprom", ".ieeprom", ".bs"];
						string newSaves = Path.Combine(gameDir, SavesFolderName);
						if (Directory.Exists(newSaves)) {
							foreach (string file in Directory.GetFiles(legacySaves)) {
								string fileName = Path.GetFileName(file);
								if (fileName.StartsWith(romName, StringComparison.OrdinalIgnoreCase)) {
									string ext = fileName[romName.Length..].ToLowerInvariant();
									if (batteryExtensions.Contains(ext) || ext == ".chr.sav") {
										string destFile = Path.Combine(newSaves, fileName);
										if (File.Exists(destFile)) {
											legacyFiles++;
											hasLegacyFiles = true;
										}
									}
								}
							}
						}
					}

					if (hasLegacyFiles)
						gameCount++;
				}
			}

			return (legacyFiles, gameCount);
		} catch (Exception ex) {
			Log.Warn($"[GameDataManager] Failed to get cleanup status: {ex.Message}");
			return (0, 0);
		}
	}

	/// <summary>
	/// Removes legacy files that have already been successfully migrated to per-game folders.
	/// Only deletes legacy copies where the new per-game version exists.
	/// </summary>
	/// <returns>Number of legacy files removed.</returns>
	public static int CleanupMigratedLegacyFiles() {
		try {
			string gameDataBase = GameDataBasePath;
			if (!Directory.Exists(gameDataBase))
				return 0;

			int cleaned = 0;

			foreach (string systemDir in Directory.GetDirectories(gameDataBase)) {
				foreach (string gameDir in Directory.GetDirectories(systemDir)) {
					string gameFolderName = Path.GetFileName(gameDir);
					int lastUnderscore = gameFolderName.LastIndexOf('_');
					if (lastUnderscore < 0)
						continue;

					string romName = gameFolderName[..lastUnderscore];

					// Clean up legacy save states
					string legacySaveStates = Path.Combine(ConfigManager.HomeFolder, "SaveStates", romName);
					if (Directory.Exists(legacySaveStates)) {
						string newSaveStates = Path.Combine(gameDir, SaveStatesFolderName);
						if (Directory.Exists(newSaveStates)) {
							foreach (string file in Directory.GetFiles(legacySaveStates)) {
								string ext = Path.GetExtension(file).ToLowerInvariant();
								if (ext is not ".nexen-save" and not ".mss")
									continue;

								string destFile = Path.Combine(newSaveStates, Path.GetFileName(file));
								if (File.Exists(destFile)) {
									try {
										File.Delete(file);
										cleaned++;
									} catch (Exception ex) {
										Log.Warn($"[GameDataManager] Failed to clean up '{file}': {ex.Message}");
									}
								}
							}

							// Remove the legacy folder if it's now empty
							try {
								if (Directory.GetFiles(legacySaveStates).Length == 0 &&
									Directory.GetDirectories(legacySaveStates).Length == 0) {
									Directory.Delete(legacySaveStates);
								}
							} catch { /* ignore */ }
						}
					}

					// Clean up legacy battery saves
					string legacySaves = Path.Combine(ConfigManager.HomeFolder, "Saves");
					if (Directory.Exists(legacySaves)) {
						HashSet<string> batteryExtensions = [".sav", ".srm", ".rtc", ".eeprom", ".ieeprom", ".bs"];
						string newSaves = Path.Combine(gameDir, SavesFolderName);
						if (Directory.Exists(newSaves)) {
							foreach (string file in Directory.GetFiles(legacySaves)) {
								string fileName = Path.GetFileName(file);
								if (!fileName.StartsWith(romName, StringComparison.OrdinalIgnoreCase))
									continue;

								string ext = fileName[romName.Length..].ToLowerInvariant();
								if (!batteryExtensions.Contains(ext) && ext != ".chr.sav")
									continue;

								string destFile = Path.Combine(newSaves, fileName);
								if (File.Exists(destFile)) {
									try {
										File.Delete(file);
										cleaned++;
									} catch (Exception ex) {
										Log.Warn($"[GameDataManager] Failed to clean up '{file}': {ex.Message}");
									}
								}
							}
						}
					}
				}
			}

			if (cleaned > 0) {
				Log.Info($"[GameDataManager] Cleaned up {cleaned} legacy file(s)");
			}

			return cleaned;
		} catch (Exception ex) {
			Log.Warn($"[GameDataManager] Legacy cleanup failed: {ex.Message}");
			return 0;
		}
	}
}
