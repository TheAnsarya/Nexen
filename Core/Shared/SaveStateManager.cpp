#include "pch.h"
#include "Utilities/FolderUtilities.h"
#include "Utilities/ZipWriter.h"
#include "Utilities/ZipReader.h"
#include "Utilities/PNGHelper.h"
#include "Shared/SaveStateManager.h"
#include "Shared/MessageManager.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"
#include "Shared/Movies/MovieManager.h"
#include "Shared/RenderedFrame.h"
#include "Shared/EventType.h"
#include "Debugger/Debugger.h"
#include "Netplay/GameClient.h"
#include "Shared/Video/VideoDecoder.h"
#include "Shared/Video/VideoRenderer.h"
#include "Shared/Video/BaseVideoFilter.h"
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <algorithm>

SaveStateManager::SaveStateManager(Emulator* emu) {
	_emu = emu;
	_lastIndex = 1;
	_recentPlaySlot = 0;
	_lastRecentPlayTime = 0;
}

string SaveStateManager::GetStateFilepath(int stateIndex) {
	string romFile = _emu->GetRomInfo().RomFile.GetFileName();
	string folder = FolderUtilities::GetSaveStateFolder();
	string filename = FolderUtilities::GetFilename(romFile, false) + "_" + std::to_string(stateIndex) + ".nexen-save";
	return FolderUtilities::CombinePath(folder, filename);
}

string SaveStateManager::GetRomSaveStateDirectory() {
	// Use per-ROM directory override if set by C# GameDataManager
	if (!_perRomSaveStateDir.empty()) {
		FolderUtilities::CreateFolder(_perRomSaveStateDir);
		return _perRomSaveStateDir;
	}

	// Fallback: legacy path {SaveStateFolder}/{RomName}/
	string romName = FolderUtilities::GetFilename(_emu->GetRomInfo().RomFile.GetFileName(), false);
	string folder = FolderUtilities::CombinePath(FolderUtilities::GetSaveStateFolder(), romName);
	FolderUtilities::CreateFolder(folder);
	return folder;
}

string SaveStateManager::GetTimestampedFilepath() {
	string romName = FolderUtilities::GetFilename(_emu->GetRomInfo().RomFile.GetFileName(), false);
	string folder = GetRomSaveStateDirectory();

	// Generate timestamp: YYYY-MM-DD_HH-mm-ss
	auto now = std::chrono::system_clock::now();
	auto time = std::chrono::system_clock::to_time_t(now);
	std::tm tm;
#ifdef _WIN32
	localtime_s(&tm, &time);
#else
	localtime_r(&time, &tm);
#endif

	std::ostringstream oss;
	oss << romName << "_"
		<< std::setfill('0') << std::setw(4) << (tm.tm_year + 1900) << "-"
		<< std::setw(2) << (tm.tm_mon + 1) << "-"
		<< std::setw(2) << tm.tm_mday << "_"
		<< std::setw(2) << tm.tm_hour << "-"
		<< std::setw(2) << tm.tm_min << "-"
		<< std::setw(2) << tm.tm_sec << ".nexen-save";

	return FolderUtilities::CombinePath(folder, oss.str());
}

time_t SaveStateManager::ParseTimestampFromFilename(const string& filename) {
	// Expected format: {RomName}_{YYYY}-{MM}-{DD}_{HH}-{mm}-{ss}.nexen-save or .mss
	// Find the date/time portion by looking for the pattern _YYYY-MM-DD_HH-mm-ss before the extension

	size_t extPos = filename.rfind(".nexen-save");
	if (extPos == string::npos) {
		extPos = filename.rfind(".mss");
	}
	if (extPos == string::npos || extPos < 20) {
		return 0; // Not a valid timestamped filename
	}

	// Extract the timestamp portion (19 chars before ext: _YYYY-MM-DD_HH-mm-ss)
	size_t tsStart = extPos - 20;
	if (filename[tsStart] != '_') {
		return 0;
	}

	string tsStr = filename.substr(tsStart + 1, 19); // YYYY-MM-DD_HH-mm-ss

	std::tm tm = {};
	std::istringstream ss(tsStr);
	ss >> std::get_time(&tm, "%Y-%m-%d_%H-%M-%S");

	if (ss.fail()) {
		return 0;
	}

	return std::mktime(&tm);
}

void SaveStateManager::SelectSaveSlot(int slotIndex) {
	_lastIndex = slotIndex;
	MessageManager::DisplayMessage("SaveStates", "SaveStateSlotSelected", std::to_string(_lastIndex));
}

void SaveStateManager::MoveToNextSlot() {
	_lastIndex = (_lastIndex % MaxIndex) + 1;
	MessageManager::DisplayMessage("SaveStates", "SaveStateSlotSelected", std::to_string(_lastIndex));
}

void SaveStateManager::MoveToPreviousSlot() {
	_lastIndex = (_lastIndex == 1 ? SaveStateManager::MaxIndex : (_lastIndex - 1));
	MessageManager::DisplayMessage("SaveStates", "SaveStateSlotSelected", std::to_string(_lastIndex));
}

void SaveStateManager::SaveState() {
	SaveState(_lastIndex);
}

bool SaveStateManager::LoadState() {
	return LoadState(_lastIndex);
}

void SaveStateManager::GetSaveStateHeader(ostream& stream) {
	uint32_t emuVersion = _emu->GetSettings()->GetVersion();
	uint32_t formatVersion = SaveStateManager::FileFormatVersion;
	stream.write("MSS", 3);
	WriteValue(stream, emuVersion);
	WriteValue(stream, formatVersion);

	WriteValue(stream, (uint32_t)_emu->GetConsoleType());

	SaveVideoData(stream);

	RomInfo romInfo = _emu->GetRomInfo();
	string romName = FolderUtilities::GetFilename(romInfo.RomFile.GetFileName(), true);
	WriteValue(stream, (uint32_t)romName.size());
	stream.write(romName.c_str(), romName.size());
}

void SaveStateManager::SaveState(ostream& stream) {
	GetSaveStateHeader(stream);
	_emu->Serialize(stream, false);
}

bool SaveStateManager::SaveState(string filepath, bool showSuccessMessage) {
	ofstream file(filepath, ios::out | ios::binary);

	if (file) {
		{
			auto lock = _emu->AcquireLock();
			SaveState(file);
			_emu->ProcessEvent(EventType::StateSaved);
		}
		file.close();

		if (showSuccessMessage) {
			MessageManager::DisplayMessage("SaveStates", "SaveStateSavedFile", filepath);
		}
		return true;
	}
	return false;
}

void SaveStateManager::SaveState(int stateIndex, bool displayMessage) {
	// Use correct filepath based on slot type
	string filepath;
	if (stateIndex == AutoSaveStateIndex) {
		filepath = GetAutoSaveFilepath();
	} else {
		filepath = GetStateFilepath(stateIndex);
	}

	if (SaveState(filepath, false)) {
		if (displayMessage) {
			MessageManager::DisplayMessage("SaveStates", "SaveStateSaved", std::to_string(stateIndex));
		}
	}
}

void SaveStateManager::SaveVideoData(ostream& stream) {
	PpuFrameInfo frame = _emu->GetPpuFrame();
	WriteValue(stream, frame.FrameBufferSize);
	WriteValue(stream, frame.Width);
	WriteValue(stream, frame.Height);
	WriteValue(stream, (uint32_t)(_emu->GetVideoDecoder()->GetLastFrameScale() * 100));

	unsigned long compressedSize = compressBound(frame.FrameBufferSize);
	vector<uint8_t> compressedData(compressedSize, 0);
	compress2(compressedData.data(), &compressedSize, (const unsigned char*)frame.FrameBuffer, frame.FrameBufferSize, MZ_DEFAULT_LEVEL);

	WriteValue(stream, (uint32_t)compressedSize);
	stream.write((char*)compressedData.data(), (uint32_t)compressedSize);
}

bool SaveStateManager::GetVideoData(vector<uint8_t>& out, RenderedFrame& frame, istream& stream) {
	uint32_t frameBufferSize = ReadValue(stream);
	frame.Width = ReadValue(stream);
	frame.Height = ReadValue(stream);
	frame.Scale = ReadValue(stream) / 100.0;

	uint32_t compressedSize = ReadValue(stream);
	if (compressedSize > 1024 * 1024 * 2) {
		// Data is larger than 2mb, this is probably invalid
		return false;
	}

	vector<uint8_t> compressedData(compressedSize, 0);
	stream.read((char*)compressedData.data(), compressedSize);

	out = vector<uint8_t>(frameBufferSize, 0);
	unsigned long decompSize = frameBufferSize;
	if (uncompress(out.data(), &decompSize, compressedData.data(), (unsigned long)compressedData.size()) == MZ_OK) {
		return true;
	}
	return false;
}

bool SaveStateManager::LoadState(istream& stream) {
	if (!_emu->IsRunning()) {
		// Can't load a state if no game is running
		return false;
	} else if (_emu->GetGameClient()->Connected()) {
		MessageManager::DisplayMessage("Netplay", "NetplayNotAllowed");
		return false;
	}

	char header[3];
	stream.read(header, 3);
	if (memcmp(header, "MSS", 3) == 0) {
		uint32_t emuVersion = ReadValue(stream);
		if (emuVersion > _emu->GetSettings()->GetVersion()) {
			MessageManager::DisplayMessage("SaveStates", "SaveStateNewerVersion");
			return false;
		}

		uint32_t fileFormatVersion = ReadValue(stream);
		if (fileFormatVersion < SaveStateManager::MinimumSupportedVersion) {
			MessageManager::DisplayMessage("SaveStates", "SaveStateIncompatibleVersion");
			return false;
		}

		if (fileFormatVersion <= 3) {
			// Skip over old SHA1 field
			stream.seekg(40, ios::cur);
		}

		ConsoleType stateConsoleType = (ConsoleType)ReadValue(stream);

		RenderedFrame frame;
		vector<uint8_t> frameData;
		if (GetVideoData(frameData, frame, stream)) {
			frame.FrameBuffer = frameData.data();
		} else {
			MessageManager::DisplayMessage("SaveStates", "SaveStateInvalidFile");
			return false;
		}

		uint32_t nameLength = ReadValue(stream);

		vector<char> nameBuffer(nameLength);
		stream.read(nameBuffer.data(), nameBuffer.size());
		string romName(nameBuffer.data(), nameLength);

		DeserializeResult result = _emu->Deserialize(stream, fileFormatVersion, false, stateConsoleType);

		if (result == DeserializeResult::Success) {
			// Stop any movie that might have been playing/recording if a state is loaded
			//(Note: Loading a state is disabled in the UI while a movie is playing/recording)
			_emu->GetMovieManager()->Stop();

			if (_emu->IsPaused() && !_emu->GetVideoRenderer()->IsRecording()) {
				// Only send the saved frame if the emulation is paused and no avi recording is in progress
				// Otherwise the avi recorder will receive an extra frame that has no sound, which will
				// create a video vs audio desync in the avi file.
				_emu->GetVideoDecoder()->UpdateFrame(frame, true, false);
			}
			return true;
		} else if (result == DeserializeResult::SpecificError) {
			return false;
		}
	}

	MessageManager::DisplayMessage("SaveStates", "SaveStateInvalidFile");
	return false;
}

bool SaveStateManager::LoadState(string filepath, bool showSuccessMessage) {
	ifstream file(filepath, ios::in | ios::binary);
	bool result = false;

	if (file.good()) {
		{
			auto lock = _emu->AcquireLock();
			result = LoadState(file);
			if (result) {
				_emu->ProcessEvent(EventType::StateLoaded);
			}
		}
		file.close();

		if (result) {
			if (showSuccessMessage) {
				MessageManager::DisplayMessage("SaveStates", "SaveStateLoadedFile", filepath);
			}
		}
	} else {
		MessageManager::DisplayMessage("SaveStates", "SaveStateEmpty");
	}

	return result;
}

bool SaveStateManager::LoadState(int stateIndex) {
	string filepath = SaveStateManager::GetStateFilepath(stateIndex);
	if (LoadState(filepath, false)) {
		MessageManager::DisplayMessage("SaveStates", "SaveStateLoaded", std::to_string(stateIndex));
		return true;
	}
	return false;
}

void SaveStateManager::SaveRecentGame(string romName, string romPath, string patchPath) {
	if (_emu->GetSettings()->CheckFlag(EmulationFlags::ConsoleMode) || _emu->GetSettings()->CheckFlag(EmulationFlags::TestMode)) {
		// Skip saving the recent game file when running in testrunner/CLI console mode
		return;
	}

	string filename = FolderUtilities::GetFilename(_emu->GetRomInfo().RomFile.GetFileName(), false) + ".rgd";
	ZipWriter writer;
	writer.Initialize(FolderUtilities::CombinePath(FolderUtilities::GetRecentGamesFolder(), filename));

	std::stringstream pngStream;
	_emu->GetVideoDecoder()->TakeScreenshot(pngStream);
	writer.AddFile(pngStream, "Screenshot.png");

	std::stringstream stateStream;
	SaveStateManager::SaveState(stateStream);
	writer.AddFile(stateStream, "Savestate.mss");

	std::stringstream romInfoStream;
	romInfoStream << romName << std::endl;
	romInfoStream << romPath << std::endl;
	romInfoStream << patchPath << std::endl;

	FrameInfo baseFrameSize = _emu->GetVideoDecoder()->GetBaseFrameInfo(true);
	double aspectRatio = _emu->GetSettings()->GetAspectRatio(_emu->GetRegion(), baseFrameSize);
	if (aspectRatio > 0) {
		romInfoStream << "aspectratio=" << aspectRatio << std::endl;
	}

	writer.AddFile(romInfoStream, "RomInfo.txt");
	writer.Save();
}

void SaveStateManager::LoadRecentGame(string filename, bool resetGame) {
	VirtualFile file(filename);
	if (!file.IsValid()) {
		MessageManager::DisplayMessage("Error", "CouldNotLoadFile", file.GetFileName());
		return;
	}

	ZipReader reader;
	reader.LoadArchive(filename);

	stringstream romInfoStream, stateStream;
	reader.GetStream("RomInfo.txt", romInfoStream);
	reader.GetStream("Savestate.mss", stateStream);

	string romName, romPath, patchPath;
	std::getline(romInfoStream, romName);
	std::getline(romInfoStream, romPath);
	std::getline(romInfoStream, patchPath);

	try {
		if (_emu->LoadRom(romPath, patchPath)) {
			if (!resetGame) {
				auto lock = _emu->AcquireLock();
				SaveStateManager::LoadState(stateStream);
			}
		}
	} catch (std::exception&) {
		_emu->Stop(true);
	}
}

int32_t SaveStateManager::GetSaveStatePreview(string saveStatePath, uint8_t* pngData) {
	ifstream stream(saveStatePath, ios::binary);

	if (!stream) {
		return -1;
	}

	char header[3];
	stream.read(header, 3);
	if (memcmp(header, "MSS", 3) == 0) {
		uint32_t emuVersion = ReadValue(stream);
		if (emuVersion > _emu->GetSettings()->GetVersion() || emuVersion <= 0x10000) {
			// Prevent loading files created with a newer version of Nexen or with Nexen 0.9.x or lower.
			return -1;
		}

		uint32_t fileFormatVersion = ReadValue(stream);
		if (fileFormatVersion < SaveStateManager::MinimumSupportedVersion) {
			return -1;
		}

		// Skip console type field
		stream.seekg(4, ios::cur);

		vector<uint8_t> frameData;
		RenderedFrame frame;
		if (GetVideoData(frameData, frame, stream)) {
			FrameInfo baseFrameInfo;
			baseFrameInfo.Width = frame.Width;
			baseFrameInfo.Height = frame.Height;

			unique_ptr<BaseVideoFilter> filter(_emu->GetVideoFilter(true));
			filter->SetBaseFrameInfo(baseFrameInfo);
			FrameInfo frameInfo = filter->SendFrame((uint16_t*)frameData.data(), 0, 0, nullptr);

			std::stringstream pngStream;
			PNGHelper::WritePNG(pngStream, filter->GetOutputBuffer(), frameInfo.Width, frameInfo.Height);

			string data = pngStream.str();
			memcpy(pngData, data.c_str(), data.size());

			return (int32_t)frameData.size();
		}
	}
	return -1;
}

void SaveStateManager::WriteValue(ostream& stream, uint32_t value) {
	stream.put(value & 0xFF);
	stream.put((value >> 8) & 0xFF);
	stream.put((value >> 16) & 0xFF);
	stream.put((value >> 24) & 0xFF);
}

uint32_t SaveStateManager::ReadValue(istream& stream) {
	char a = 0, b = 0, c = 0, d = 0;
	stream.get(a);
	stream.get(b);
	stream.get(c);
	stream.get(d);

	uint32_t result = (uint8_t)a | ((uint8_t)b << 8) | ((uint8_t)c << 16) | ((uint8_t)d << 24);
	return result;
}

// ========== Timestamped Save State Methods ==========

string SaveStateManager::SaveTimestampedState() {
	string filepath = GetTimestampedFilepath();

	if (SaveState(filepath, false)) {
		// Extract just the time portion for the message
		auto now = std::chrono::system_clock::now();
		auto time = std::chrono::system_clock::to_time_t(now);
		std::tm tm;
#ifdef _WIN32
		localtime_s(&tm, &time);
#else
		localtime_r(&time, &tm);
#endif

		std::ostringstream oss;
		oss << std::setfill('0') << std::setw(2) << tm.tm_hour << ":"
			<< std::setw(2) << tm.tm_min << ":"
			<< std::setw(2) << tm.tm_sec;

		MessageManager::DisplayMessage("SaveStates", "SaveStateSavedTime", oss.str());
		return filepath;
	}

	return "";
}

vector<SaveStateInfo> SaveStateManager::GetSaveStateList() {
	vector<SaveStateInfo> states;

	string romName = FolderUtilities::GetFilename(_emu->GetRomInfo().RomFile.GetFileName(), false);
	string folder = GetRomSaveStateDirectory();

	namespace fs = std::filesystem;

	try {
		if (!fs::exists(folder)) {
			return states;
		}

		for (const auto& entry : fs::directory_iterator(folder)) {
			if (!entry.is_regular_file()) {
				continue;
			}

			string filename = entry.path().filename().string();

		// Check if it's a save state file (.nexen-save or legacy .mss)
		bool isNexenSave = filename.size() > 11 && filename.substr(filename.size() - 11) == ".nexen-save";
		bool isMesenSave = filename.size() > 4 && filename.substr(filename.size() - 4) == ".mss";
		if (!isNexenSave && !isMesenSave) {
			continue;
		}

			// Check if it starts with the ROM name
			if (filename.find(romName) != 0) {
				continue;
			}

			SaveStateInfo info;
			info.filepath = entry.path().string();
			info.romName = romName;
			info.timestamp = ParseTimestampFromFilename(filename);
			info.fileSize = static_cast<uint32_t>(entry.file_size());

			// Detect origin from filename pattern
			// {RomName}_auto.nexen-save -> Auto
			// {RomName}_recent_{NN}.nexen-save -> Recent
			// {RomName}_lua_{timestamp}.nexen-save -> Lua
			// {RomName}_{YYYY-MM-DD}_{HH-mm-ss}.nexen-save -> Save
			if (filename.find("_auto.") != string::npos) {
				info.origin = SaveStateOrigin::Auto;
			} else if (filename.find("_recent_") != string::npos) {
				info.origin = SaveStateOrigin::Recent;
			} else if (filename.find("_lua_") != string::npos) {
				info.origin = SaveStateOrigin::Lua;
			} else {
				info.origin = SaveStateOrigin::Save;
			}

			// If timestamp parsing failed, use file modification time
			if (info.timestamp == 0) {
				auto ftime = fs::last_write_time(entry);
				auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
					ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
				);
				info.timestamp = std::chrono::system_clock::to_time_t(sctp);
			}

			states.push_back(info);
		}
	} catch (const std::exception&) {
		// Ignore filesystem errors
	}

	// Sort by timestamp, newest first
	std::sort(states.begin(), states.end(), [](const SaveStateInfo& a, const SaveStateInfo& b) {
		return a.timestamp > b.timestamp;
	});

	return states;
}

bool SaveStateManager::DeleteSaveState(const string& filepath) {
	try {
		namespace fs = std::filesystem;
		if (fs::exists(filepath) && fs::is_regular_file(filepath)) {
			return fs::remove(filepath);
		}
	} catch (const std::exception&) {
		// Ignore errors
	}
	return false;
}

uint32_t SaveStateManager::GetSaveStateCount() {
	return static_cast<uint32_t>(GetSaveStateList().size());
}

// ========== Recent Play Queue Implementation ==========

string SaveStateManager::GetRecentPlayFilepath(uint32_t slotIndex) {
	string folder = GetRomSaveStateDirectory();
	string romName = FolderUtilities::GetFilename(_emu->GetRomInfo().RomFile.GetFileName(), false);

	// Format slot as 2-digit (01-12)
	std::ostringstream oss;
	oss << romName << "_recent_" << std::setfill('0') << std::setw(2) << (slotIndex + 1) << ".nexen-save";

	return FolderUtilities::CombinePath(folder, oss.str());
}

string SaveStateManager::GetAutoSaveFilepath() {
	string folder = GetRomSaveStateDirectory();
	string romName = FolderUtilities::GetFilename(_emu->GetRomInfo().RomFile.GetFileName(), false);

	return FolderUtilities::CombinePath(folder, romName + "_auto.nexen-save");
}

string SaveStateManager::SaveRecentPlayState() {
	if (_emu->GetRomInfo().RomFile.GetFileName().empty()) {
		return "";
	}

	// Get current slot filepath
	string filepath = GetRecentPlayFilepath(_recentPlaySlot);

	// Save state to file
	if (SaveState(filepath, false)) {
		// Update timestamp
		_lastRecentPlayTime = std::time(nullptr);

		// Advance to next slot (wraps 0-11)
		_recentPlaySlot = (_recentPlaySlot + 1) % RecentPlayMaxSlots;

		return filepath;
	}

	return "";
}

bool SaveStateManager::ShouldSaveRecentPlay() {
	if (_emu->GetRomInfo().RomFile.GetFileName().empty()) {
		return false;
	}

	time_t now = std::time(nullptr);
	return (now - _lastRecentPlayTime) >= static_cast<time_t>(RecentPlayIntervalSec);
}

void SaveStateManager::ResetRecentPlayTimer() {
	_lastRecentPlayTime = std::time(nullptr);
	// Don't reset slot - allows continuing rotation across ROM loads
}

vector<SaveStateInfo> SaveStateManager::GetRecentPlayStates() {
	vector<SaveStateInfo> allStates = GetSaveStateList();
	vector<SaveStateInfo> recentStates;

	// Filter to only Recent origin saves
	for (const auto& state : allStates) {
		if (state.origin == SaveStateOrigin::Recent) {
			recentStates.push_back(state);
		}
	}

	return recentStates;
}

// ========== Designated Save Implementation ==========

void SaveStateManager::SetDesignatedSave(const string& filepath) {
	namespace fs = std::filesystem;

	// Validate the file exists
	if (!filepath.empty() && fs::exists(filepath) && fs::is_regular_file(filepath)) {
		_designatedSavePath = filepath;
		MessageManager::DisplayMessage("SaveStates", "DesignatedSaveSet");
	} else {
		MessageManager::DisplayMessage("SaveStates", "DesignatedSaveInvalid");
	}
}

string SaveStateManager::GetDesignatedSave() const {
	return _designatedSavePath;
}

bool SaveStateManager::LoadDesignatedState() {
	if (!HasDesignatedSave()) {
		MessageManager::DisplayMessage("SaveStates", "NoDesignatedSave");
		return false;
	}

	return LoadState(_designatedSavePath, true);
}

bool SaveStateManager::HasDesignatedSave() const {
	namespace fs = std::filesystem;
	return !_designatedSavePath.empty() &&
	       fs::exists(_designatedSavePath) &&
	       fs::is_regular_file(_designatedSavePath);
}

void SaveStateManager::ClearDesignatedSave() {
	_designatedSavePath.clear();
	MessageManager::DisplayMessage("SaveStates", "DesignatedSaveCleared");
}

// ========== Per-ROM Directory Override ==========

void SaveStateManager::SetPerRomSaveStateDirectory(const string& path) {
	_perRomSaveStateDir = path;
	if (!path.empty()) {
		FolderUtilities::CreateFolder(path);
	}
}
