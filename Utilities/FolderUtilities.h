#pragma once

#include "pch.h"
#include <unordered_set>

/// <summary>
/// Cross-platform folder and file path management for NEXEN.
/// Handles save folders, screenshots, firmware, HD packs, and game directories.
/// </summary>
/// <remarks>
/// Folder hierarchy:
/// - Home folder: Root directory for all NEXEN data
/// - Save folder: Battery saves (.sav files)
/// - Save state folder: Save states (.mst files)
/// - Screenshot folder: Screenshots (.png files)
/// - Firmware folder: Console firmware/BIOS files
/// - HD Pack folder: High-definition texture packs
/// - Debugger folder: Debug symbols and labels
/// - Recent games folder: Recently played games list
///
/// All folders can be overridden via SetFolderOverrides().
/// Game folders track known ROM locations for quick access.
/// </remarks>
class FolderUtilities {
private:
	static string _homeFolder;               ///< Root folder for all NEXEN data
	static string _saveFolderOverride;       ///< Custom save folder (empty = default)
	static string _saveStateFolderOverride;  ///< Custom save state folder
	static string _firmwareFolderOverride;   ///< Custom firmware/BIOS folder
	static string _screenshotFolderOverride; ///< Custom screenshot folder
	static vector<string> _gameFolders;      ///< List of known ROM directories

public:
	/// <summary>Set NEXEN home folder (root for all data)</summary>
	static void SetHomeFolder(string homeFolder);

	/// <summary>Get current home folder path</summary>
	[[nodiscard]] static string GetHomeFolder();

	/// <summary>
	/// Override default folder locations.
	/// </summary>
	/// <param name="saveFolder">Custom save folder (empty = use default)</param>
	/// <param name="saveStateFolder">Custom save state folder</param>
	/// <param name="screenshotFolder">Custom screenshot folder</param>
	/// <param name="firmwareFolder">Custom firmware/BIOS folder</param>
	static void SetFolderOverrides(string saveFolder, string saveStateFolder, string screenshotFolder, string firmwareFolder);

	/// <summary>Add folder to known game directory list</summary>
	static void AddKnownGameFolder(string gameFolder);

	/// <summary>Get list of all known game directories</summary>
	[[nodiscard]] static vector<string> GetKnownGameFolders();

	/// <summary>Get battery save folder path</summary>
	[[nodiscard]] static string GetSaveFolder();

	/// <summary>Get firmware/BIOS folder path</summary>
	[[nodiscard]] static string GetFirmwareFolder();

	/// <summary>Get save state folder path</summary>
	[[nodiscard]] static string GetSaveStateFolder();

	/// <summary>Get screenshot folder path</summary>
	[[nodiscard]] static string GetScreenshotFolder();

	/// <summary>Get HD texture pack folder path</summary>
	[[nodiscard]] static string GetHdPackFolder();

	/// <summary>Get debugger symbols/labels folder path</summary>
	[[nodiscard]] static string GetDebuggerFolder();

	/// <summary>Get recent games list folder path</summary>
	[[nodiscard]] static string GetRecentGamesFolder();

	/// <summary>Get list of all subdirectories in folder</summary>
	/// <param name="rootFolder">Root folder to scan</param>
	/// <returns>List of subdirectory paths</returns>
	[[nodiscard]] static vector<string> GetFolders(string rootFolder);

	/// <summary>
	/// Get list of files in folder matching extensions.
	/// </summary>
	/// <param name="rootFolder">Root folder to scan</param>
	/// <param name="extensions">Set of file extensions to match (e.g., {".nes", ".sfc"})</param>
	/// <param name="recursive">Recursively scan subdirectories if true</param>
	/// <returns>List of matching file paths</returns>
	[[nodiscard]] static vector<string> GetFilesInFolder(string rootFolder, std::unordered_set<string> extensions, bool recursive);

	/// <summary>Extract filename from full path</summary>
	/// <param name="filepath">Full file path</param>
	/// <param name="includeExtension">Include file extension in result</param>
	/// <returns>Filename with or without extension</returns>
	[[nodiscard]] static string GetFilename(string filepath, bool includeExtension);

	/// <summary>Get file extension from path</summary>
	[[nodiscard]] static string GetExtension(string filename);

	/// <summary>Get folder name from full path (excluding filename)</summary>
	[[nodiscard]] static string GetFolderName(string filepath);

	/// <summary>Create directory (and parent directories if needed)</summary>
	static void CreateFolder(string folder);

	[[nodiscard]] static string CombinePath(string folder, string filename);
};
