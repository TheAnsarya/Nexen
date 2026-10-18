#pragma once
#include "pch.h"
#include <ctime>

class Emulator;
struct RenderedFrame;

//Which feature created a save; encoded as a filename prefix
enum class SaveStateOrigin
{
	Manual,
	Auto,
	RecentPlay
};

struct SaveStateInfo
{
	string filepath;
	string romName;
	time_t timestamp; //Local time of the save, parsed from the filename
	uint32_t fileSize;
};

class SaveStateManager
{
private:
	static constexpr uint32_t MaxIndex = 10;
	static constexpr uint32_t MaxAutoSaves = 20; //Oldest saves of each origin are deleted beyond these counts
	static constexpr uint32_t MaxRecentPlaySaves = 5;

	atomic<uint32_t> _lastIndex;
	Emulator* _emu;

	string GetStateFilepath(int stateIndex);
	string GetRomSaveStateDirectory();
	string GetTimestampedFilepath(const char* prefix = "");
	void PruneOrigin(SaveStateOrigin origin);
	static time_t ParseTimestampFromFilename(const string& filename);
	void SaveVideoData(ostream& stream);
	bool GetVideoData(vector<uint8_t>& out, RenderedFrame& frame, istream& stream);

	void WriteValue(ostream& stream, uint32_t value);
	uint32_t ReadValue(istream& stream);

public:
	static constexpr uint32_t FileFormatVersion = 4;
	static constexpr uint32_t MinimumSupportedVersion = 3;
	static constexpr uint32_t AutoSaveStateIndex = 11;

	SaveStateManager(Emulator* emu);

	void SaveState();
	bool LoadState();

	void GetSaveStateHeader(ostream & stream);

	void SaveState(ostream &stream);
	bool SaveState(string filepath, bool showSuccessMessage = true);
	void SaveState(int stateIndex, bool displayMessage = true);
	bool LoadState(istream &stream);
	bool LoadState(string filepath, bool showSuccessMessage = true);
	bool LoadState(int stateIndex);

	void SaveRecentGame(string romName, string romPath, string patchPath);
	void LoadRecentGame(string filename, bool resetGame);

	int32_t GetSaveStatePreview(string saveStatePath, uint8_t* pngData);

	void SelectSaveSlot(int slotIndex);
	void MoveToNextSlot();
	void MoveToPreviousSlot();

	//Timestamped (unlimited) saves, stored as {SaveStates}/{RomName}/yyyy-MM-dd_HH-mm-ss.mss
	string SaveTimestampedState();
	string SaveOriginState(SaveStateOrigin origin);
	static const char* GetOriginPrefix(SaveStateOrigin origin);
	static SaveStateOrigin GetOrigin(const string& filepath);
	vector<SaveStateInfo> GetSaveStateList();
	bool DeleteSaveState(const string& filepath);
	uint32_t GetSaveStateCount();
};