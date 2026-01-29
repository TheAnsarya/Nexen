#pragma once

#include "pch.h"
#include <unordered_set>

class FolderUtilities {
private:
	static string _homeFolder;
	static string _saveFolderOverride;
	static string _saveStateFolderOverride;
	static string _firmwareFolderOverride;
	static string _screenshotFolderOverride;
	static vector<string> _gameFolders;

public:
	static void SetHomeFolder(string homeFolder);
	[[nodiscard]] static string GetHomeFolder();

	static void SetFolderOverrides(string saveFolder, string saveStateFolder, string screenshotFolder, string firmwareFolder);

	static void AddKnownGameFolder(string gameFolder);
	[[nodiscard]] static vector<string> GetKnownGameFolders();

	[[nodiscard]] static string GetSaveFolder();
	[[nodiscard]] static string GetFirmwareFolder();
	[[nodiscard]] static string GetSaveStateFolder();
	[[nodiscard]] static string GetScreenshotFolder();
	[[nodiscard]] static string GetHdPackFolder();
	[[nodiscard]] static string GetDebuggerFolder();
	[[nodiscard]] static string GetRecentGamesFolder();

	[[nodiscard]] static vector<string> GetFolders(string rootFolder);
	[[nodiscard]] static vector<string> GetFilesInFolder(string rootFolder, std::unordered_set<string> extensions, bool recursive);

	[[nodiscard]] static string GetFilename(string filepath, bool includeExtension);
	[[nodiscard]] static string GetExtension(string filename);
	[[nodiscard]] static string GetFolderName(string filepath);

	static void CreateFolder(string folder);

	[[nodiscard]] static string CombinePath(string folder, string filename);
};
