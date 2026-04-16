#include "pch.h"
#include <ranges>
#include "Shared/Movies/NexenMovie.h"
#include "Shared/Movies/MovieTypes.h"
#include "Shared/Movies/MovieManager.h"
#include "Shared/MessageManager.h"
#include "Shared/BaseControlManager.h"
#include "Shared/BaseControlDevice.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"
#include "Shared/SaveStateManager.h"
#include "Shared/NotificationManager.h"
#include "Shared/BatteryManager.h"
#include "Shared/CheatManager.h"
#include "Utilities/ZipReader.h"
#include "Utilities/StringUtilities.h"
#include "Utilities/HexUtilities.h"
#include "Utilities/VirtualFile.h"
#include "Utilities/magic_enum.hpp"
#include "Utilities/Serializer.h"

NexenMovie::NexenMovie(Emulator* emu, bool forTest) {
	_emu = emu;
	_forTest = forTest;
}

NexenMovie::~NexenMovie() {
	_emu->UnregisterInputProvider(this);
}

void NexenMovie::Stop() {
	if (_playing) {
		bool isEndOfMovie = _lastPollCounter >= _inputData.size();

		if (!_forTest) {
			MessageManager::DisplayMessage("Movies", isEndOfMovie ? "MovieEnded" : "MovieStopped");
		}

		EmuSettings* settings = _emu->GetSettings();
		if (isEndOfMovie && settings->GetPreferences().PauseOnMovieEnd) {
			_emu->PauseOnNextFrame();
		}

		_emu->GetCheatManager()->SetCheats(_originalCheats);

		Serializer backup(0, false);
		backup.LoadFrom(_emuSettingsBackup);
		backup.Stream(*settings, "", -1);

		_playing = false;
	}

	_emu->UnregisterInputProvider(this);
	_controlManager = nullptr;
}

bool NexenMovie::SetInput(BaseControlDevice* device) {
	uint32_t inputRowIndex = _controlManager->GetPollCounter();

	if (_lastPollCounter != inputRowIndex) {
		_lastPollCounter = inputRowIndex;
		assert(_deviceIndex == 0);
		_deviceIndex = 0;
	}

	if (_inputData.size() > inputRowIndex && _deviceCount > _deviceIndex) {
		device->SetTextState(StringUtilities::GetNthSegmentView(_inputData[inputRowIndex], '|', _deviceIndex));

		_deviceIndex++;
		if (_deviceIndex >= _deviceCount) {
			// Move to the next frame's data
			_deviceIndex = 0;
		}
	} else {
		// End of input data reached (movie end)
		_emu->GetMovieManager()->Stop();
	}
	return true;
}

bool NexenMovie::IsPlaying() {
	return _playing;
}

vector<uint8_t> NexenMovie::LoadBattery(const string& extension) {
	vector<uint8_t> batteryData;
	_reader->ExtractFile("Battery" + extension, batteryData);
	return batteryData;
}

void NexenMovie::ProcessNotification(ConsoleNotificationType type, void* parameter) {
	if (type == ConsoleNotificationType::GameLoaded) {
		_emu->RegisterInputProvider(this);
		shared_ptr<IConsole> console = _emu->GetConsole();
		if (console) {
			console->GetControlManager()->SetPollCounter(_lastPollCounter);
		}
	}
}

bool NexenMovie::Play(VirtualFile& file) {
	_movieFile = file;

	std::stringstream ss;
	(void)file.ReadFile(ss);

	_reader = std::make_unique<ZipReader>();
	_reader->LoadArchive(ss);

	stringstream settingsData, inputData;
	if (!_reader->GetStream("GameSettings.txt", settingsData)) {
		MessageManager::Log("[Movie] File not found: GameSettings.txt");
		return false;
	}
	if (!_reader->GetStream("Input.txt", inputData)) {
		MessageManager::Log("[Movie] File not found: Input.txt");
		return false;
	}

	// Pre-allocate using fast newline count from raw data (avoids double-pass getline)
	{
		const string& rawInput = inputData.str();
		size_t lineEstimate = std::ranges::count(rawInput, '\n') + 1;
		_inputData.reserve(lineEstimate);
	}

	string line;
	while (std::getline(inputData, line)) {
		if (line.starts_with("|")) {
			// Store the frame line (minus leading '|') as a single flat string:
			// e.g. "UDLRSsBA|UDLRSsBA" for a 2-controller NES game
			string_view frameData = string_view(line).substr(1);
			if (_deviceCount == 0) {
				_deviceCount = StringUtilities::CountSegments(frameData, '|');
			}
			_inputData.emplace_back(frameData);
		}
	}

	_deviceIndex = 0;

	ParseSettings(settingsData);

	string version = LoadString(_settings, MovieKeys::NexenVersion);
	if (version.size() < 2 || version.starts_with("0.") || version.starts_with("1.")) {
		// Prevent loading movies from Nexen/Nexen-S version 0.x.x or 1.x.x
		MessageManager::DisplayMessage("Movies", "MovieIncompatibleVersion");
		return false;
	}

	if (LoadInt(_settings, MovieKeys::MovieFormatVersion, 0) < 2) {
		MessageManager::DisplayMessage("Movies", "MovieIncompatibleVersion");
		return false;
	}

	auto emuLock = _emu->AcquireLock(false);

	if (!ApplySettings(settingsData)) {
		return false;
	}

	_emu->GetBatteryManager()->SetBatteryProvider(shared_from_this());
	_emu->GetNotificationManager()->RegisterNotificationListener(shared_from_this());

	_emu->PowerCycle();

	// Re-apply settings - power cycling can alter some (e.g auto-configure input types, etc.)
	ApplySettings(settingsData);

	_originalCheats = _emu->GetCheatManager()->GetCheats();

	_controlManager = _emu->GetConsole()->GetControlManager();

	LoadCheats();

	stringstream saveStateData;
	// Try new Nexen format first, then legacy Mesen format for backward compatibility
	if (_reader->GetStream("SaveState.nexen-save", saveStateData) || _reader->GetStream("SaveState.mss", saveStateData)) {
		if (!_emu->GetSaveStateManager()->LoadState(saveStateData)) {
			return false;
		}
	}

	_controlManager->UpdateControlDevices();
	_controlManager->SetPollCounter(0);
	_playing = true;

	return true;
}

template <typename T>
T FromString(string_view name, const vector<string>& enumNames, T defaultValue) {
	for (size_t i = 0; i < enumNames.size(); i++) {
		if (name == enumNames[i]) {
			return (T)i;
		}
	}
	return defaultValue;
}

void NexenMovie::ParseSettings(stringstream& data) {
	string line;
	while (std::getline(data, line)) {
		if (!line.empty()) {
			size_t index = line.find(' ');
			if (index != string::npos) {
				string name = line.substr(0, index);
				string value = line.substr(index + 1);

				if (name == "Cheat") {
					_cheats.push_back(std::move(value));
				} else {
					_settings.insert_or_assign(std::move(name), std::move(value));
				}
			}
		}
	}
}

bool NexenMovie::ApplySettings(istream& settingsData) {
	EmuSettings* settings = _emu->GetSettings();

	settingsData.clear();
	settingsData.seekg(0, std::ios::beg);
	Serializer s(0, false, SerializeFormat::Text);
	s.LoadFrom(settingsData);

	ConsoleType consoleType = {};
	s.Stream(consoleType, "emu.consoleType", -1);

	if (consoleType != _emu->GetConsoleType()) {
		MessageManager::DisplayMessage("Movies", "MovieIncorrectConsole", string(magic_enum::enum_name<ConsoleType>(consoleType)));
		return false;
	}

	Serializer backup(0, true);
	backup.Stream(*settings, "", -1);
	backup.SaveTo(_emuSettingsBackup);

	// Load settings
	s.Stream(*settings, "", -1);

	return true;
}

uint32_t NexenMovie::LoadInt(SettingsMap& settings, std::string_view name, uint32_t defaultValue) {
	auto result = settings.find(name);
	if (result != settings.end()) {
		uint32_t value;
		auto [ptr, ec] = std::from_chars(result->second.data(), result->second.data() + result->second.size(), value);
		if (ec == std::errc()) {
			return value;
		}
		MessageManager::Log("[Movies] Invalid value for tag: " + string(name));
		return defaultValue;
	} else {
		return defaultValue;
	}
}

bool NexenMovie::LoadBool(SettingsMap& settings, std::string_view name) {
	auto result = settings.find(name);
	if (result != settings.end()) {
		if (result->second == "true") {
			return true;
		} else if (result->second == "false") {
			return false;
		} else {
			MessageManager::Log("[Movies] Invalid value for tag: " + string(name));
			return false;
		}
	} else {
		return false;
	}
}

string NexenMovie::LoadString(SettingsMap& settings, std::string_view name) {
	auto result = settings.find(name);
	if (result != settings.end()) {
		return result->second;
	} else {
		return "";
	}
}

void NexenMovie::LoadCheats() {
	vector<CheatCode> cheats;
	for (const string& cheatData : _cheats) {
		CheatCode code;
		if (LoadCheat(cheatData, code)) {
			cheats.push_back(code);
		}
	}
	_emu->GetCheatManager()->SetCheats(cheats);
}

bool NexenMovie::LoadCheat(const string& cheatData, CheatCode& code) {
	if (StringUtilities::CountSegments(cheatData, ' ') == 2) {
		string_view typeStr = StringUtilities::GetNthSegmentView(cheatData, ' ', 0);
		string_view codeStr = StringUtilities::GetNthSegmentView(cheatData, ' ', 1);

		auto cheatType = magic_enum::enum_cast<CheatType>(typeStr);
		if (cheatType.has_value() && codeStr.size() <= 15) {
			code.Type = cheatType.value();
			memcpy(code.Code, codeStr.data(), codeStr.size());
			code.Code[codeStr.size()] = '\0';
			return true;
		}
	}

	MessageManager::Log("[Movie] Invalid cheat definition: " + cheatData);
	return false;
}
