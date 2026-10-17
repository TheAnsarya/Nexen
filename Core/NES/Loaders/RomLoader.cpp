#include "pch.h"
#include <algorithm>
#include <cctype>
#include <ranges>
#include <unordered_set>
#include "Utilities/FolderUtilities.h"
#include "Utilities/CRC32.h"
#include "Utilities/ArchiveReader.h"
#include "Utilities/VirtualFile.h"

#include "NES/Loaders/RomLoader.h"
#include "NES/Loaders/iNesLoader.h"
#include "NES/Loaders/FdsLoader.h"
#include "NES/Loaders/NsfLoader.h"
#include "NES/Loaders/NsfeLoader.h"
#include "NES/Loaders/UnifLoader.h"
#include "NES/Loaders/StudyBoxLoader.h"
#include "NES/NesHeader.h"
#include "NES/GameDatabase.h"

bool RomLoader::LoadFile(VirtualFile& romFile, RomData& romData, bool databaseEnabled) {
	if (!romFile.IsValid()) {
		return false;
	}

	romData = {};

	vector<uint8_t>& fileData = romData.RawData;
	(void)romFile.ReadFile(fileData);
	if (fileData.size() < 15) {
		return false;
	}

	string filename = romFile.GetFileName();
	string romName = FolderUtilities::GetFilename(filename, true);

	uint32_t crc = CRC32::GetCRC(fileData.data(), fileData.size());
	romData.Info.Hash.Crc32 = crc;

	MessageManager::Log("");
	MessageManager::Log("Loading rom: " + romName);
	MessageManager::Log("File CRC32: 0x" + std::format("{:08X}", crc));

	if (memcmp(fileData.data(), "NES\x1a", 4) == 0) {
		iNesLoader loader;
		loader.LoadRom(romData, fileData, nullptr, databaseEnabled);
	} else if (memcmp(fileData.data(), "FDS\x1a", 4) == 0 || memcmp(fileData.data(), "\x1*NINTENDO-HVC*", 15) == 0) {
		FdsLoader loader;
		loader.LoadRom(romData, fileData);
	} else if (memcmp(fileData.data(), "NESM\x1a", 5) == 0) {
		NsfLoader loader;
		loader.LoadRom(romData, fileData);
	} else if (memcmp(fileData.data(), "NSFE", 4) == 0) {
		NsfeLoader loader;
		loader.LoadRom(romData, fileData);
	} else if (memcmp(fileData.data(), "UNIF", 4) == 0) {
		UnifLoader loader;
		loader.LoadRom(romData, fileData, databaseEnabled);
	} else if (memcmp(fileData.data(), "STBX", 4) == 0) {
		StudyBoxLoader loader;
		loader.LoadRom(romData, fileData, romFile.GetFilePath());
	} else {
		NesHeader header = {};
		if (GameDatabase::GetiNesHeader(crc, header)) {
			MessageManager::Log("[DB] Headerless ROM file found - using game database data.");
			iNesLoader loader;
			loader.LoadRom(romData, fileData, &header, true);
			romData.Info.IsHeaderlessRom = true;
		} else {
			MessageManager::Log("Invalid rom file.");
			romData.Error = true;
		}
	}

	romData.Info.RomName = romName;
	romData.Info.Filename = filename;

	if (romData.Info.System == GameSystem::Unknown) {
		// Use filename to detect PAL/VS system games
		string name = romData.Info.Filename;
		std::ranges::transform(name, name.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		if (name.contains("(e)") || name.contains("(australia)") || name.contains("(europe)") ||
		    name.contains("(germany)") || name.contains("(spain)")) {
			romData.Info.System = GameSystem::NesPal;
		} else if (name.contains("(vs)")) {
			romData.Info.System = GameSystem::VsSystem;
		} else {
			romData.Info.System = GameSystem::NesNtsc;
		}
	}

	return !romData.Error;
}
