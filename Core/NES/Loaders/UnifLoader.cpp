#include "pch.h"
#include "NES/Loaders/UnifLoader.h"
#include "NES/Loaders/UnifBoards.h"
#include "NES/RomData.h"
#include "NES/GameDatabase.h"
#include "Shared/MessageManager.h"
#include "Utilities/CRC32.h"
#include "Utilities/md5.h"
#include "Utilities/HexUtilities.h"

void UnifLoader::Read(uint8_t*& data, uint8_t& dest) {
	dest = data[0];
	data++;
}

void UnifLoader::Read(uint8_t*& data, uint32_t& dest) {
	dest = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
	data += 4;
}

void UnifLoader::Read(uint8_t*& data, uint8_t* dest, size_t len) {
	memcpy(dest, data, len);
	data += len;
}

string UnifLoader::ReadString(uint8_t*& data, uint8_t* chunkEnd) {
	stringstream ss;
	while (data < chunkEnd) {
		if (data[0] == 0) {
			// end of string
			data = chunkEnd;
			break;
		} else {
			if (data[0] != ' ') {
				// Ignore spaces
				ss << (char)data[0];
			}
		}
		data++;
	}

	return ss.str();
}

string UnifLoader::ReadFourCC(uint8_t*& data) {
	stringstream ss;
	for (int i = 0; i < 4; i++) {
		ss << (char)data[i];
	}
	data += 4;
	return ss.str();
}

bool UnifLoader::ReadChunk(uint8_t*& data, uint8_t* dataEnd, RomData& romData) {
	if (data + 8 > dataEnd) {
		return false;
	}

	string fourCC = ReadFourCC(data);

	uint32_t length;
	Read(data, length);

	uint8_t* chunkEnd = data + length;
	if (chunkEnd > dataEnd) {
		return false;
	}

	if (fourCC.compare("MAPR") == 0) {
		_mapperName = ReadString(data, chunkEnd);
		if (_mapperName.size() > 0) {
			romData.Info.MapperID = GetMapperID(_mapperName);
			if (romData.Info.MapperID == UnifBoards::UnknownBoard) {
				Log("[UNIF] Error: Unknown board");
			}
		} else {
			romData.Error = true;
			return false;
		}
	} else if (fourCC.starts_with("PRG")) {
		uint32_t chunkNumber;
		std::stringstream ss;
		ss << std::hex << fourCC[3];
		ss >> chunkNumber;

		_prgChunks[chunkNumber].resize(length);
		Read(data, _prgChunks[chunkNumber].data(), length);
	} else if (fourCC.starts_with("CHR")) {
		uint32_t chunkNumber;
		std::stringstream ss;
		ss << std::hex << fourCC[3];
		ss >> chunkNumber;

		_chrChunks[chunkNumber].resize(length);
		Read(data, _chrChunks[chunkNumber].data(), length);
	} else if (fourCC.compare("TVCI") == 0) {
		uint8_t value;
		Read(data, value);
		romData.Info.System = value == 1 ? GameSystem::NesPal : GameSystem::NesNtsc;
	} else if (fourCC.compare("CTRL") == 0) {
		// not supported
	} else if (fourCC.compare("BATR") == 0) {
		uint8_t value;
		Read(data, value);
		romData.Info.HasBattery = value > 0;
	} else if (fourCC.compare("MIRR") == 0) {
		uint8_t value;
		Read(data, value);

		switch (value) {
			default:
			case 0:
				romData.Info.Mirroring = MirroringType::Horizontal;
				break;
			case 1:
				romData.Info.Mirroring = MirroringType::Vertical;
				break;
			case 2:
				romData.Info.Mirroring = MirroringType::ScreenAOnly;
				break;
			case 3:
				romData.Info.Mirroring = MirroringType::ScreenBOnly;
				break;
			case 4:
				romData.Info.Mirroring = MirroringType::FourScreens;
				break;
		}
	} else {
		// Unsupported/unused FourCCs: PCKn, CCKn, NAME, WRTR, READ, DINF, VROR
	}

	data = chunkEnd;

	return true;
}

int32_t UnifLoader::GetMapperID(const string& mapperName) {
	string name = mapperName;
	if (name.starts_with("NES-") || name.starts_with("UNL-") || name.starts_with("HVC-") || name.starts_with("BTL-") || name.starts_with("BMC-")) {
		name = name.substr(4);
	}

	auto it = std::lower_bound(_boardMappings.begin(), _boardMappings.end(), name,
		[](const BoardEntry& entry, const string& key) { return entry.first < key; });
	if (it != _boardMappings.end() && it->first == name) {
		return it->second;
	}

	return UnifBoards::UnknownBoard;
}

void UnifLoader::LoadRom(RomData& romData, vector<uint8_t>& romFile, bool databaseEnabled) {
	// Skip header, version & null bytes, start reading at first chunk
	uint8_t* data = romFile.data() + 32;
	uint8_t* endOfFile = romFile.data() + romFile.size();

	while (ReadChunk(data, endOfFile, romData)) {
		// Read all chunks
	}

	for (int i = 0; i < 16; i++) {
		romData.PrgRom.insert(romData.PrgRom.end(), _prgChunks[i].begin(), _prgChunks[i].end());
		romData.ChrRom.insert(romData.ChrRom.end(), _chrChunks[i].begin(), _chrChunks[i].end());
	}

	if (romData.PrgRom.size() == 0 || _mapperName.empty()) {
		romData.Error = true;
	} else {
		vector<uint8_t> fullRom;
		fullRom.insert(fullRom.end(), romData.PrgRom.begin(), romData.PrgRom.end());
		fullRom.insert(fullRom.end(), romData.ChrRom.begin(), romData.ChrRom.end());

		romData.Info.Format = RomFormat::Unif;
		romData.Info.Hash.PrgCrc32 = CRC32::GetCRC(romData.PrgRom.data(), romData.PrgRom.size());
		romData.Info.Hash.PrgChrCrc32 = CRC32::GetCRC(fullRom.data(), fullRom.size());

		Log("PRG+CHR CRC32: 0x" + HexUtilities::ToHex(romData.Info.Hash.PrgChrCrc32));
		Log("[UNIF] Board Name: " + _mapperName);
		Log("[UNIF] PRG ROM: " + std::to_string(romData.PrgRom.size() / 1024) + " KB");
		Log("[UNIF] CHR ROM: " + std::to_string(romData.ChrRom.size() / 1024) + " KB");
		if (romData.ChrRom.size() == 0) {
			Log("[UNIF] CHR RAM: 8 KB");
		}

		string mirroringType;
		switch (romData.Info.Mirroring) {
			case MirroringType::Horizontal:
				mirroringType = "Horizontal";
				break;
			case MirroringType::Vertical:
				mirroringType = "Vertical";
				break;
			case MirroringType::ScreenAOnly:
				mirroringType = "1-Screen (A)";
				break;
			case MirroringType::ScreenBOnly:
				mirroringType = "1-Screen (B)";
				break;
			case MirroringType::FourScreens:
				mirroringType = "Four Screens";
				break;
		}

		Log("[UNIF] Mirroring: " + mirroringType);
		Log("[UNIF] Battery: " + string(romData.Info.HasBattery ? "Yes" : "No"));

		GameDatabase::SetGameInfo(romData.Info.Hash.PrgChrCrc32, romData, databaseEnabled, false);

		if (romData.Info.MapperID == UnifBoards::UnknownBoard) {
			MessageManager::DisplayMessage("Error", "UnsupportedMapper", "UNIF: " + _mapperName);
			romData.Error = true;
		}
	}
}
