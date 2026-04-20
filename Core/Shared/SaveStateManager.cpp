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
#include "Shared/Interfaces/IConsole.h"
#include "Shared/BaseControlManager.h"
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
	_lastRecentPlayTime = std::time(nullptr);
	_shutdownRequested = false;
	_writeThread = std::thread([this]() { BackgroundWriteLoop(); });
}

SaveStateManager::~SaveStateManager() {
	{
		std::lock_guard<std::mutex> lock(_writeMutex);
		_shutdownRequested = true;
	}
	_writeCv.notify_one();
	if (_writeThread.joinable()) {
		_writeThread.join();
	}
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

	std::string filename = std::format("{}_{:04d}-{:02d}-{:02d}_{:02d}-{:02d}-{:02d}.nexen-save",
		romName, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);

	return FolderUtilities::CombinePath(folder, filename);
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

	// Let mktime auto-detect DST; zero-init (tm_isdst=0) assumes standard time,
	// which produces a 1-hour offset when the timestamp was created during DST
	tm.tm_isdst = -1;
	return std::mktime(&tm);
}

string SaveStateManager::FormatSaveStateOsd(const string& badge) {
	auto now = std::chrono::system_clock::now();
	auto time = std::chrono::system_clock::to_time_t(now);
	std::tm tm;
#ifdef _WIN32
	localtime_s(&tm, &time);
#else
	localtime_r(&time, &tm);
#endif
	return std::format("{}\n{:04d}-{:02d}-{:02d}\n{:02d}:{:02d}:{:02d}",
		badge, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);
}

string SaveStateManager::FormatSaveStateOsdFromFile(const string& filepath, const string& action) {
	string filename = FolderUtilities::GetFilename(filepath, true);

	// Determine badge from filename pattern
	string badge;
	if (filename.find("_auto.") != string::npos || filename.find("_auto_") != string::npos) {
		badge = "Auto " + action;
	} else if (size_t pos = filename.find("_[slot"); pos != string::npos) {
		size_t end = filename.find(']', pos);
		if (end != string::npos) {
			string slotStr = filename.substr(pos + 6, end - pos - 6);
			badge = "Slot " + slotStr + " " + action;
		} else {
			badge = action;
		}
	} else {
		badge = action;
	}

	// Try to extract timestamp from filename
	time_t fileTime = ParseTimestampFromFilename(filepath);
	if (fileTime > 0) {
		std::tm tm;
#ifdef _WIN32
		localtime_s(&tm, &fileTime);
#else
		localtime_r(&fileTime, &tm);
#endif
		return std::format("{}\n{:04d}-{:02d}-{:02d}\n{:02d}:{:02d}:{:02d}",
			badge, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec);
	}

	// Fallback: use file modification time
	try {
		auto modTime = std::filesystem::last_write_time(filepath);
		auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
			modTime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
		);
		auto tt = std::chrono::system_clock::to_time_t(sctp);
		std::tm tm;
#ifdef _WIN32
		localtime_s(&tm, &tt);
#else
		localtime_r(&tt, &tm);
#endif
		return std::format("{}\n{:04d}-{:02d}-{:02d}\n{:02d}:{:02d}:{:02d}",
			badge, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec);
	} catch (...) {
	}

	// Last fallback: current time
	return FormatSaveStateOsd(badge);
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

	// v5+: write pause state after compressed state data
	char pauseByte = _emu->IsPaused() ? 1 : 0;
	stream.put(pauseByte);
}

bool SaveStateManager::SaveState(const string& filepath, bool showSuccessMessage) {
	if (!_emu->IsRunning()) {
		return false;
	}

	SaveStateSnapshot snapshot;

	{
		auto lock = _emu->AcquireLock();

		// Capture state data under lock (fast: memcpy-like, no compression)
		snapshot.stateData = _emu->SerializeToBuffer();

		// Capture frame data under lock
		PpuFrameInfo frame = _emu->GetPpuFrame();
		snapshot.frameBufferSize = frame.FrameBufferSize;
		snapshot.frameWidth = frame.Width;
		snapshot.frameHeight = frame.Height;
		snapshot.frameScale100 = (uint32_t)(_emu->GetVideoDecoder()->GetLastFrameScale() * 100);
		if (frame.FrameBuffer && frame.FrameBufferSize > 0) {
			snapshot.frameBuffer.assign(
				(uint8_t*)frame.FrameBuffer,
				(uint8_t*)frame.FrameBuffer + frame.FrameBufferSize);
		}

		// Capture metadata under lock
		snapshot.emuVersion = _emu->GetSettings()->GetVersion();
		snapshot.consoleType = (uint32_t)_emu->GetConsoleType();
		RomInfo romInfo = _emu->GetRomInfo();
		snapshot.romName = FolderUtilities::GetFilename(romInfo.RomFile.GetFileName(), true);
		snapshot.isPaused = _emu->IsPaused();

		_emu->ProcessEvent(EventType::StateSaved);
	}

	snapshot.filepath = filepath;
	snapshot.showSuccessMessage = showSuccessMessage;

	// Enqueue for background compression + write (no lock held)
	{
		std::lock_guard<std::mutex> lock(_writeMutex);
		_writeQueue.push(std::move(snapshot));
	}
	_writeCv.notify_one();

	return true;
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
			string badge = (stateIndex == AutoSaveStateIndex) ? "Auto Saved" : "#" + std::to_string(stateIndex) + " Saved";
			MessageManager::DisplayMessage("SaveStates", FormatSaveStateOsd(badge));
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
	_compressBuffer.resize(compressedSize);
	compress2(_compressBuffer.data(), &compressedSize, (const unsigned char*)frame.FrameBuffer, frame.FrameBufferSize, MZ_BEST_SPEED);

	WriteValue(stream, (uint32_t)compressedSize);
	stream.write((char*)_compressBuffer.data(), (uint32_t)compressedSize);
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

	// Reuse persistent buffers to avoid ~150KB heap alloc/dealloc per load
	// (especially important for rewind which loads state every ~1 second)
	_compressedReadBuffer.resize(compressedSize);
	stream.read((char*)_compressedReadBuffer.data(), compressedSize);

	_decompressBuffer.resize(frameBufferSize);
	unsigned long decompSize = frameBufferSize;
	if (uncompress(_decompressBuffer.data(), &decompSize, _compressedReadBuffer.data(), (unsigned long)_compressedReadBuffer.size()) == MZ_OK) {
		// Copy into output instead of moving — preserves _decompressBuffer allocation
		// across calls (especially important during rewind: ~1 state load/sec)
		out.resize(frameBufferSize);
		memcpy(out.data(), _decompressBuffer.data(), frameBufferSize);
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

			// v5+: restore pause state from save state
			if (fileFormatVersion >= 5) {
				char pauseByte = 0;
				if (stream.get(pauseByte)) {
					if (pauseByte) {
						_emu->Pause();
					} else {
						_emu->Resume();
					}
				}
			}

			if (!_emu->GetVideoRenderer()->IsRecording()) {
				// Always push the saved frame to the renderer after a successful state load.
				// If running, the emulation loop overwrites on the next frame (harmless).
				// If paused, this ensures the loaded state's frame is displayed immediately.
				// Skip only during AVI recording to avoid audio desync from the extra soundless frame.
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

bool SaveStateManager::LoadState(const string& filepath, bool showSuccessMessage) {
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
				MessageManager::DisplayMessage("SaveStates", FormatSaveStateOsdFromFile(filepath, "Loaded"));
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
		MessageManager::DisplayMessage("SaveStates", FormatSaveStateOsdFromFile(filepath, "Loaded"));
		return true;
	}
	return false;
}

void SaveStateManager::SaveRecentGame(const string& romName, const string& romPath, const string& patchPath) {
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
	romInfoStream << romName << '\n';
	romInfoStream << romPath << '\n';
	romInfoStream << patchPath << '\n';

	FrameInfo baseFrameSize = _emu->GetVideoDecoder()->GetBaseFrameInfo(true);
	double aspectRatio = _emu->GetSettings()->GetAspectRatio(_emu->GetRegion(), baseFrameSize);
	if (aspectRatio > 0) {
		romInfoStream << "aspectratio=" << aspectRatio << '\n';
	}

	writer.AddFile(romInfoStream, "RomInfo.txt");
	writer.Save();
}

void SaveStateManager::LoadRecentGame(const string& filename, bool resetGame) {
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
				(void)SaveStateManager::LoadState(stateStream);
			}
		}
	} catch (std::exception&) {
		_emu->Stop(true);
	}
}

int32_t SaveStateManager::GetSaveStatePreview(const string& saveStatePath, uint8_t* pngData) {
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

			auto pngView = pngStream.view();

			// Safety bounds check — buffer is 512*478*4 = 978944 bytes on the managed side
			constexpr size_t maxBufferSize = 512 * 478 * 4;
			if (pngView.size() > maxBufferSize) {
				return -1;
			}

			memcpy(pngData, pngView.data(), pngView.size());

			return (int32_t)pngView.size();
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
		MessageManager::DisplayMessage("SaveStates", FormatSaveStateOsd("Saved"));
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
			bool isNexenSave = filename.ends_with(".nexen-save");
			bool isMesenSave = filename.ends_with(".mss");
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
			// {RomName}_auto.nexen-save -> Auto (legacy)
			// {RomName}_auto_{YYYY-MM-DD}_{HH-mm-ss}.nexen-save -> Auto
			// {RomName}_recent_{NN}.nexen-save -> Recent
			// {RomName}_lua_{timestamp}.nexen-save -> Lua
			// {RomName}_[slot{NN}]_{timestamp}.nexen-save -> Designated (new format)
			// {RomName}_designated_{N}_{timestamp}.nexen-save -> Designated (legacy)
			// {RomName}_{YYYY-MM-DD}_{HH-mm-ss}.nexen-save -> Save
			if (filename.find("_auto.") != string::npos || filename.find("_auto_") != string::npos) {
				info.origin = SaveStateOrigin::Auto;
			} else if (filename.find("_recent_") != string::npos) {
				info.origin = SaveStateOrigin::Recent;
			} else if (filename.find("_lua_") != string::npos) {
				info.origin = SaveStateOrigin::Lua;
			} else if (auto pos = filename.find("_[slot"); pos != string::npos) {
				info.origin = SaveStateOrigin::Designated;
				// Extract slot number from _[slotNN]_
				auto numStart = pos + 6; // skip "_[slot"
				auto endBracket = filename.find(']', numStart);
				if (endBracket != string::npos && endBracket > numStart) {
					try {
						info.slotNumber = static_cast<uint8_t>(std::stoi(filename.substr(numStart, endBracket - numStart)));
					} catch (...) {}
				}
			} else if (filename.find("_designated_") != string::npos) {
				info.origin = SaveStateOrigin::Designated;
				// Extract slot number from _designated_N_ or _designated_N.
				auto numStart = filename.find("_designated_") + 12;
				auto numEnd = numStart;
				while (numEnd < filename.size() && filename[numEnd] >= '0' && filename[numEnd] <= '9') numEnd++;
				if (numEnd > numStart) {
					try {
						info.slotNumber = static_cast<uint8_t>(std::stoi(filename.substr(numStart, numEnd - numStart)));
					} catch (...) {}
				}
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

			// v5+: read pause byte (last byte of file) to detect paused save states
			// Only v5+ saves have the pause byte appended — older formats' last byte is compressed data
			if (info.fileSize > 0) {
				try {
					ifstream file(info.filepath, ios::binary);
					if (file.good()) {
						// Read header to check format version
						char hdr[3] = {};
						file.read(hdr, 3);
						if (memcmp(hdr, "MSS", 3) == 0) {
							// Skip emuVersion (4 bytes)
							file.seekg(4, ios::cur);
							uint32_t fileFormatVersion = ReadValue(file);
							if (fileFormatVersion >= 5) {
								// Seek to last byte for pause flag
								file.seekg(-1, ios::end);
								char pauseByte = 0;
								if (file.get(pauseByte)) {
									info.isPaused = (pauseByte != 0);
								}
							}
						}
					}
				} catch (...) {
					// Ignore — isPaused defaults to false
				}
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

	// Format slot as 2-digit (01-36)
	string filename = std::format("{}_recent_{:02d}.nexen-save", romName, slotIndex + 1);

	return FolderUtilities::CombinePath(folder, filename);
}

string SaveStateManager::GetAutoSaveFilepath() {
	string folder = GetRomSaveStateDirectory();
	string romName = FolderUtilities::GetFilename(_emu->GetRomInfo().RomFile.GetFileName(), false);

	// Keep Auto saves as a persistent timestamped progress log.
	auto now = std::chrono::system_clock::now();
	auto time = std::chrono::system_clock::to_time_t(now);
	std::tm tm;
#ifdef _WIN32
	localtime_s(&tm, &time);
#else
	localtime_r(&time, &tm);
#endif

	string filename = std::format("{}_auto_{:04d}-{:02d}-{:02d}_{:02d}-{:02d}-{:02d}.nexen-save",
		romName, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);

	return FolderUtilities::CombinePath(folder, filename);
}

string SaveStateManager::SaveRecentPlayState() {
	if (_emu->GetRomInfo().RomFile.GetFileName().empty()) {
		return "";
	}

	// Get current slot filepath
	string filepath = GetRecentPlayFilepath(_recentPlaySlot);

	// Save state to file
	bool showNotification = _emu->GetSettings()->GetPreferences().ShowRecentPlayNotifications;
	if (SaveState(filepath, showNotification)) {
		// Update timestamp
		_lastRecentPlayTime = std::time(nullptr);

		// Advance to next slot (wraps 0-35)
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

string SaveStateManager::GetDesignatedSaveFilepath(uint32_t slot) {
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

	string filename = std::format("{}_[slot{:02d}]_{:04d}-{:02d}-{:02d}_{:02d}-{:02d}-{:02d}.nexen-save",
		romName, slot + 1, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);

	return FolderUtilities::CombinePath(folder, filename);
}

string SaveStateManager::FindLatestDesignatedSave(uint32_t slot) const {
	string romName = FolderUtilities::GetFilename(_emu->GetRomInfo().RomFile.GetFileName(), false);
	string folder = _perRomSaveStateDir.empty()
		? FolderUtilities::CombinePath(FolderUtilities::GetSaveStateFolder(), romName)
		: _perRomSaveStateDir;

	// Match new [slotNN] format: {RomName}_[slot{NN}]_{timestamp}.nexen-save
	// and legacy designated format: {RomName}_designated_{N}_{timestamp}.nexen-save
	// and legacy fixed format: {RomName}_designated_{N}.nexen-save
	string slotStr = std::format("{:02d}", slot + 1);
	string newPrefix = romName + "_[slot" + slotStr + "]_";
	string legacySlotStr = std::to_string(slot + 1);
	string legacyPrefix = romName + "_designated_" + legacySlotStr + "_";
	string legacyExact = romName + "_designated_" + legacySlotStr + ".nexen-save";

	namespace fs = std::filesystem;
	string latestPath;
	time_t latestTime = 0;

	try {
		if (!fs::exists(folder)) {
			return "";
		}

		for (const auto& entry : fs::directory_iterator(folder)) {
			if (!entry.is_regular_file()) {
				continue;
			}

			string filename = entry.path().filename().string();
			if (!filename.ends_with(".nexen-save")) {
				continue;
			}

			bool isMatch = false;
			if (filename == legacyExact) {
				isMatch = true;
			} else if (filename.starts_with(newPrefix)) {
				isMatch = true;
			} else if (filename.starts_with(legacyPrefix)) {
				isMatch = true;
			}

			if (isMatch) {
				time_t ts = ParseTimestampFromFilename(filename);
				// For legacy files without timestamp, use file modification time
				if (ts == 0) {
					auto ftime = fs::last_write_time(entry);
					auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
						ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
					);
					ts = std::chrono::system_clock::to_time_t(sctp);
				}

				if (ts > latestTime) {
					latestTime = ts;
					latestPath = entry.path().string();
				}
			}
		}
	} catch (...) {
	}

	return latestPath;
}

void SaveStateManager::SaveDesignatedState(uint32_t slot) {
	if (slot >= DesignatedSlotCount) {
		return;
	}

	string filepath = GetDesignatedSaveFilepath(slot);
	if (SaveState(filepath, false)) {
		MessageManager::DisplayMessage("SaveStates", FormatSaveStateOsd("Slot " + std::format("{:02d}", slot + 1) + " Saved"));
	}
}

bool SaveStateManager::LoadDesignatedState(uint32_t slot) {
	if (slot >= DesignatedSlotCount) {
		return false;
	}

	string filepath = FindLatestDesignatedSave(slot);
	if (filepath.empty()) {
		MessageManager::DisplayMessage("SaveStates", "NoDesignatedSave");
		return false;
	}

	return LoadState(filepath, true);
}

bool SaveStateManager::HasDesignatedSave(uint32_t slot) const {
	if (slot >= DesignatedSlotCount) {
		return false;
	}

	return !FindLatestDesignatedSave(slot).empty();
}

void SaveStateManager::SetDesignatedSave(const string& filepath) {
	// Legacy compatibility — ignored, designated slots now use file-based system
	(void)filepath;
}

string SaveStateManager::GetDesignatedSave() const {
	// Legacy compatibility — return latest slot 0 path if any exist
	return FindLatestDesignatedSave(0);
}

void SaveStateManager::ClearDesignatedSave() {
	MessageManager::DisplayMessage("SaveStates", "DesignatedSaveCleared");
}

// ========== Per-ROM Directory Override ==========

void SaveStateManager::SetPerRomSaveStateDirectory(const string& path) {
	_perRomSaveStateDir = path;
	if (!path.empty()) {
		FolderUtilities::CreateFolder(path);
	}
}

void SaveStateManager::BackgroundWriteLoop() {
	while (true) {
		SaveStateSnapshot snapshot;
		{
			std::unique_lock<std::mutex> lock(_writeMutex);
			_writeCv.wait(lock, [this] { return _shutdownRequested || !_writeQueue.empty(); });

			if (_shutdownRequested && _writeQueue.empty()) {
				return;
			}

			snapshot = std::move(_writeQueue.front());
			_writeQueue.pop();
		}

		WriteSnapshotToDisk(snapshot);
	}
}

void SaveStateManager::WriteSnapshotToDisk(SaveStateSnapshot& snapshot) {
	ofstream file(snapshot.filepath, ios::out | ios::binary);
	if (!file) {
		return;
	}

	// Write header: "MSS" + emu version + format version + console type
	file.write("MSS", 3);
	WriteValue(file, snapshot.emuVersion);
	WriteValue(file, SaveStateManager::FileFormatVersion);
	WriteValue(file, snapshot.consoleType);

	// Write video data (compress framebuffer)
	WriteValue(file, snapshot.frameBufferSize);
	WriteValue(file, snapshot.frameWidth);
	WriteValue(file, snapshot.frameHeight);
	WriteValue(file, snapshot.frameScale100);

	unsigned long compressedSize = compressBound(snapshot.frameBufferSize);
	_bgCompressFrameBuffer.resize(compressedSize);
	compress2(_bgCompressFrameBuffer.data(), &compressedSize, snapshot.frameBuffer.data(), snapshot.frameBufferSize, MZ_BEST_SPEED);

	WriteValue(file, (uint32_t)compressedSize);
	file.write((char*)_bgCompressFrameBuffer.data(), (uint32_t)compressedSize);

	// Write ROM name
	WriteValue(file, (uint32_t)snapshot.romName.size());
	file.write(snapshot.romName.c_str(), snapshot.romName.size());

	// Write compressed state data
	bool isCompressed = true;
	file.put((char)isCompressed);

	unsigned long stateCompSize = compressBound((unsigned long)snapshot.stateData.size());
	_bgCompressStateBuffer.resize(stateCompSize);
	compress2(_bgCompressStateBuffer.data(), &stateCompSize, snapshot.stateData.data(), (unsigned long)snapshot.stateData.size(), 1);

	uint32_t originalSize = (uint32_t)snapshot.stateData.size();
	uint32_t compSize = (uint32_t)stateCompSize;
	file.write((char*)&originalSize, sizeof(uint32_t));
	file.write((char*)&compSize, sizeof(uint32_t));
	file.write((char*)_bgCompressStateBuffer.data(), compSize);

	// v5+: write pause state after compressed state data
	char pauseByte = snapshot.isPaused ? 1 : 0;
	file.put(pauseByte);

	file.close();

	if (snapshot.showSuccessMessage) {
		MessageManager::DisplayMessage("SaveStates", FormatSaveStateOsdFromFile(snapshot.filepath, "Saved"));
	}
}

void SaveStateManager::FlushPendingWrites() {
	// Spin until write queue is empty
	while (true) {
		{
			std::lock_guard<std::mutex> lock(_writeMutex);
			if (_writeQueue.empty()) {
				return;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}
