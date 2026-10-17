#include "pch.h"
#include "Shared/LightweightCdlRecorder.h"
#include "Shared/Interfaces/IConsole.h"
#include "Debugger/DebugUtilities.h"
#include "Utilities/VirtualFile.h"
#include "Utilities/FolderUtilities.h"
#include "Shared/MessageManager.h"
#include <fstream>

LightweightCdlRecorder::LightweightCdlRecorder(IConsole* console, MemoryType prgRomType, uint32_t prgRomSize, CpuType cpuType, uint32_t romCrc32) {
	_console = console;
	_prgRomType = prgRomType;
	_cdlSize = prgRomSize;
	_cpuType = cpuType;
	_cpuMemType = DebugUtilities::GetCpuMemoryType(cpuType);
	_romCrc32 = romCrc32;
	_cdlData = std::make_unique<uint8_t[]>(prgRomSize);
	Reset();
	BuildPageCache();
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

bool LightweightCdlRecorder::LoadCdlFile(const string& cdlFilepath) {
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

bool LightweightCdlRecorder::SaveCdlFile(const string& cdlFilepath) {
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

uint32_t LightweightCdlRecorder::GetAddressSpaceSize(CpuType type) {
	int hexDigits = DebugUtilities::GetProgramCounterSize(type);
	switch (hexDigits) {
		case 4: return 0x10000;       // 16-bit: 64KB (NES, GB, SMS, Lynx, Atari2600, ChannelF)
		case 5: return 0x100000;      // 20-bit: 1MB (WS)
		case 6: return 0x1000000;     // 24-bit: 16MB (SNES, SA1, GSU, Genesis, PCE, NecDsp, Cx4)
		case 8: return 0x1000000;     // 32-bit: cap at 16MB for page cache (GBA, ST018)
		default: return 0;
	}
}

void LightweightCdlRecorder::BuildPageCache() {
	if (!_console) {
		_pageCacheSize = 0;
		return;
	}

	uint32_t addressSpaceSize = GetAddressSpaceSize(_cpuType);
	if (addressSpaceSize == 0) {
		_pageCacheSize = 0;
		return;
	}

	uint32_t pageCount = addressSpaceSize >> PageShift;
	if (pageCount > MaxCacheablePages) {
		pageCount = MaxCacheablePages;
	}

	_pageCache = std::make_unique<CdlPageEntry[]>(pageCount);
	_pageCacheSize = pageCount;

	for (uint32_t page = 0; page < pageCount; page++) {
		uint32_t pageAddr = page << PageShift;
		AddressInfo relAddr = { (int32_t)pageAddr, _cpuMemType };
		AddressInfo absAddr = _console->GetAbsoluteAddress(relAddr);

		if (absAddr.Address >= 0 && absAddr.Type == _prgRomType) {
			// Verify linear mapping: check that the last byte of the page
			// maps to baseOffset + PageMask (i.e., the page is contiguous in ROM)
			uint32_t lastAddr = pageAddr | PageMask;
			AddressInfo lastRel = { (int32_t)lastAddr, _cpuMemType };
			AddressInfo lastAbs = _console->GetAbsoluteAddress(lastRel);

			if (lastAbs.Type == _prgRomType &&
				lastAbs.Address == absAddr.Address + (int32_t)PageMask) {
				// Linear mapping confirmed — cache it
				_pageCache[page].baseOffset = absAddr.Address;
			} else {
				// Non-linear mapping — fall back to virtual call for this page
				_pageCache[page].baseOffset = -1;
			}
		} else {
			_pageCache[page].baseOffset = -1;
		}
	}

	MessageManager::Log("[LightweightCDL] Page cache built: " + std::to_string(pageCount) + " pages");
}

void LightweightCdlRecorder::RebuildPageCache() {
	BuildPageCache();
}
