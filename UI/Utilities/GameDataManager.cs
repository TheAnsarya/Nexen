using System;
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
}
