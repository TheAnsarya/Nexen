#pragma once

#include "pch.h"
#include "Utilities/VirtualFile.h"
#include "Shared/BatteryManager.h"
#include "Shared/Interfaces/INotificationListener.h"
#include "Shared/Movies/MovieManager.h"

class ZipReader;
class Emulator;
class BaseControlManager;
struct CheatCode;

class NexenMovie final : public IMovie, public INotificationListener, public IBatteryProvider, public std::enable_shared_from_this<NexenMovie> {
private:
	Emulator* _emu = nullptr;

	BaseControlManager* _controlManager = nullptr;

	VirtualFile _movieFile;
	unique_ptr<ZipReader> _reader;
	bool _playing = false;
	size_t _deviceIndex = 0;
	uint32_t _lastPollCounter = 0;
	vector<string> _inputData;
	size_t _deviceCount = 0;	vector<string> _cheats;
	vector<CheatCode> _originalCheats;
	stringstream _emuSettingsBackup;
	unordered_map<string, string> _settings;
	string _filename;
	bool _forTest = false;

private:
	void ParseSettings(stringstream& data);
	bool ApplySettings(istream& settingsData);

	[[nodiscard]] uint32_t LoadInt(std::unordered_map<string, string>& settings, const string& name, uint32_t defaultValue = 0);
	[[nodiscard]] bool LoadBool(std::unordered_map<string, string>& settings, const string& name);
	[[nodiscard]] string LoadString(std::unordered_map<string, string>& settings, const string& name);

	void LoadCheats();
	bool LoadCheat(const string& cheatData, CheatCode& code);

public:
	NexenMovie(Emulator* emu, bool silent);
	virtual ~NexenMovie();

	[[nodiscard]] bool Play(VirtualFile& file) override;
	void Stop() override;

	[[nodiscard]] bool SetInput(BaseControlDevice* device) override;
	[[nodiscard]] bool IsPlaying() override;

	// Inherited via IBatteryProvider
	vector<uint8_t> LoadBattery(const string& extension) override;

	// Inherited via INotificationListener
	void ProcessNotification(ConsoleNotificationType type, void* parameter) override;
};
