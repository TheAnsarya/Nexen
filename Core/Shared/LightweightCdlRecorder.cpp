#include "pch.h"
#include "Shared/LightweightCdlRecorder.h"
#include "Shared/Interfaces/IConsole.h"
#include "Utilities/VirtualFile.h"
#include "Utilities/FolderUtilities.h"
#include "Shared/MessageManager.h"
#include <fstream>

LightweightCdlRecorder::LightweightCdlRecorder(IConsole* console, MemoryType prgRomType, uint32_t prgRomSize, CpuType cpuType, uint32_t romCrc32) {
	_console = console;
	_prgRomType = prgRomType;
	_cdlSize = prgRomSize;
	_cpuType = cpuType;
	_romCrc32 = romCrc32;
	_cdlData = std::make_unique<uint8_t[]>(prgRomSize);
	Reset();
}

LightweightCdlRecorder::~LightweightCdlRecorder() = default;

void LightweightCdlRecorder::Reset() {
	memset(_cdlData.get(), 0, _cdlSize);
}

AddressInfo LightweightCdlRecorder::GetPcAbsoluteAddress() {
	return _console->GetPcAbsoluteAddress();
}

CdlStatistics LightweightCdlRecorder::GetStatistics() {
	uint32_t codeSize = 0;
	uint32_t dataSize = 0;
	uint32_t bothSize = 0;

	for (uint32_t i = 0; i < _cdlSize; i++) {
		uint32_t isCode = (uint32_t)(_cdlData[i] & CdlFlags::Code);
		uint32_t isData = (uint32_t)(_cdlData[i] & CdlFlags::Data) >> 1;
		codeSize += isCode;
		dataSize += isData;
		bothSize += isCode & isData;
	}

	dataSize -= bothSize;

	CdlStatistics stats = {};
	stats.CodeBytes = codeSize;
	stats.DataBytes = dataSize;
	stats.TotalBytes = _cdlSize;
	return stats;
}

bool LightweightCdlRecorder::LoadCdlFile(string cdlFilepath) {
	VirtualFile cdlFile = cdlFilepath;
	if (cdlFile.IsValid()) {
		vector<uint8_t>& cdlData = cdlFile.GetData();
		uint32_t fileSize = (uint32_t)cdlData.size();

		if (fileSize >= _cdlSize && fileSize >= (uint32_t)HeaderSize) {
			Reset();

			if (memcmp(cdlData.data(), "CDLv2", 5) == 0) {
				uint32_t savedCrc = cdlData[5] | (cdlData[6] << 8) | (cdlData[7] << 16) | (cdlData[8] << 24);
				if (savedCrc == _romCrc32 && fileSize >= _cdlSize + HeaderSize) {
					memcpy(_cdlData.get(), cdlData.data() + HeaderSize, _cdlSize);
				}
			} else {
				// Older headerless CDL file
				MessageManager::Log("[Warning] CDL file doesn't contain header/CRC, may be incompatible.");
				memcpy(_cdlData.get(), cdlData.data(), _cdlSize);
			}

			return true;
		}
	}
	return false;
}

bool LightweightCdlRecorder::SaveCdlFile(string cdlFilepath) {
	ofstream cdlFile(cdlFilepath, ios::out | ios::binary);
	if (cdlFile) {
		cdlFile.write("CDLv2", 5);
		cdlFile.put(_romCrc32 & 0xFF);
		cdlFile.put((_romCrc32 >> 8) & 0xFF);
		cdlFile.put((_romCrc32 >> 16) & 0xFF);
		cdlFile.put((_romCrc32 >> 24) & 0xFF);
		cdlFile.write((char*)_cdlData.get(), _cdlSize);
		cdlFile.close();
		return true;
	}
	return false;
}

void LightweightCdlRecorder::GetCdlData(uint32_t offset, uint32_t length, uint8_t* cdlData) {
	if (offset + length <= _cdlSize) {
		memcpy(cdlData, _cdlData.get() + offset, length);
	}
}

void LightweightCdlRecorder::SetCdlData(uint8_t* cdlData, uint32_t length) {
	if (length <= _cdlSize) {
		memcpy(_cdlData.get(), cdlData, length);
	}
}

uint8_t LightweightCdlRecorder::GetFlags(uint32_t addr) {
	if (addr < _cdlSize) {
		return _cdlData[addr];
	}
	return 0;
}
