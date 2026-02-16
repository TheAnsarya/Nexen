#include "pch.h"
#include "Lynx/LynxConsole.h"
#include "Lynx/LynxTypes.h"
#include "Lynx/LynxCpu.h"
#include "Lynx/LynxMikey.h"
#include "Lynx/LynxSuzy.h"
#include "Lynx/LynxMemoryManager.h"
#include "Lynx/LynxCart.h"
#include "Lynx/LynxControlManager.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"
#include "Shared/MessageManager.h"
#include "Shared/BatteryManager.h"
#include "Utilities/Serializer.h"
#include "Utilities/VirtualFile.h"

LynxConsole::LynxConsole(Emulator* emu) {
	_emu = emu;
}

LynxConsole::~LynxConsole() {
	delete[] _workRam;
	delete[] _prgRom;
	delete[] _bootRom;
	delete[] _saveRam;
}

LoadRomResult LynxConsole::LoadRom(VirtualFile& romFile) {
	vector<uint8_t> romData;
	romFile.ReadFile(romData);

	if (romData.size() < 64) {
		return LoadRomResult::Failure;
	}

	// Check for LNX header: magic bytes "LYNX" at offset 0
	bool hasLnxHeader = (romData[0] == 'L' && romData[1] == 'Y' && romData[2] == 'N' && romData[3] == 'X');
	uint32_t romOffset = 0;

	MessageManager::Log("------------------------------");

	if (hasLnxHeader) {
		// LNX header is 64 bytes
		// Bytes 0-3: "LYNX" magic
		// Bytes 4-5: Bank 0 page size (little-endian, in 256-byte pages)
		// Bytes 6-7: Bank 1 page size (little-endian, in 256-byte pages)
		// Bytes 8-9: Version (usually 1)
		// Bytes 10-41: Cart name (32 bytes, null-terminated)
		// Bytes 42-57: Manufacturer name (16 bytes, null-terminated)
		// Byte 58: Rotation (0=none, 1=left, 2=right)
		// Bytes 59-63: Reserved

		uint16_t bank0Pages = romData[4] | (romData[5] << 8);
		uint16_t bank1Pages = romData[6] | (romData[7] << 8);
		uint16_t version = romData[8] | (romData[9] << 8);

		string cartName(reinterpret_cast<char*>(&romData[10]), 32);
		cartName = cartName.c_str(); // Trim at null terminator
		string manufacturer(reinterpret_cast<char*>(&romData[42]), 16);
		manufacturer = manufacturer.c_str();

		uint8_t rotation = romData[58];
		switch (rotation) {
			case 1: _rotation = LynxRotation::Left; break;
			case 2: _rotation = LynxRotation::Right; break;
			default: _rotation = LynxRotation::None; break;
		}

		MessageManager::Log("LNX Header:");
		MessageManager::Log(std::format("  Cart Name: {}", cartName));
		MessageManager::Log(std::format("  Manufacturer: {}", manufacturer));
		MessageManager::Log(std::format("  Version: {}", version));
		MessageManager::Log(std::format("  Bank 0 Pages: {} ({} KB)", bank0Pages, bank0Pages * 256 / 1024));
		MessageManager::Log(std::format("  Bank 1 Pages: {} ({} KB)", bank1Pages, bank1Pages * 256 / 1024));
		MessageManager::Log(std::format("  Rotation: {}", rotation == 0 ? "None" : (rotation == 1 ? "Left" : "Right")));

		romOffset = 64; // Skip header
	} else {
		// Headerless .o format — raw ROM data
		MessageManager::Log("Headerless ROM (raw .o format)");
		_rotation = LynxRotation::None;
	}

	// Extract ROM data
	_prgRomSize = (uint32_t)(romData.size() - romOffset);
	if (_prgRomSize == 0) {
		return LoadRomResult::Failure;
	}

	_prgRom = new uint8_t[_prgRomSize];
	memcpy(_prgRom, romData.data() + romOffset, _prgRomSize);
	_emu->RegisterMemory(MemoryType::LynxPrgRom, _prgRom, _prgRomSize);

	MessageManager::Log(std::format("ROM Size: {} KB", _prgRomSize / 1024));

	// Allocate work RAM (64 KB)
	_workRamSize = LynxConstants::WorkRamSize;
	_workRam = new uint8_t[_workRamSize];
	memset(_workRam, 0, _workRamSize);
	_emu->RegisterMemory(MemoryType::LynxWorkRam, _workRam, _workRamSize);

	// Boot ROM — optional, loaded from firmware
	// TODO: Load boot ROM via FirmwareHelper when implemented
	// For now, no boot ROM
	_bootRomSize = 0;
	_bootRom = nullptr;

	// Save RAM (EEPROM) — size determined by ROM database or header
	// TODO: Determine EEPROM size from cart info
	_saveRamSize = 0;
	_saveRam = nullptr;

	MessageManager::Log(std::format("Work RAM: {} KB", _workRamSize / 1024));
	MessageManager::Log("------------------------------");

	// Create components
	_controlManager.reset(new LynxControlManager(_emu, this));
	_memoryManager.reset(new LynxMemoryManager());
	_cart.reset(new LynxCart());
	_mikey.reset(new LynxMikey());
	_suzy.reset(new LynxSuzy());

	// Initialize memory manager first (it needs RAM, boot ROM)
	_memoryManager->Init(_emu, this, _workRam, _bootRom, _bootRomSize);

	// Initialize cart
	LynxCartInfo cartInfo = {};
	if (hasLnxHeader) {
		uint16_t bank0Pages = romData[4] | (romData[5] << 8);
		uint16_t bank1Pages = romData[6] | (romData[7] << 8);
		cartInfo.PageSizeBank0 = bank0Pages;
		cartInfo.PageSizeBank1 = bank1Pages;
		cartInfo.RomSize = _prgRomSize;
		// Copy name/manufacturer from header
		memcpy(cartInfo.Name, &romData[10], 32);
		cartInfo.Name[32] = 0;
		memcpy(cartInfo.Manufacturer, &romData[42], 16);
		cartInfo.Manufacturer[16] = 0;
		cartInfo.Rotation = static_cast<LynxRotation>(romData[58]);
	} else {
		cartInfo.PageSizeBank0 = (uint16_t)(_prgRomSize / 256);
		cartInfo.PageSizeBank1 = 0;
		cartInfo.RomSize = _prgRomSize;
	}
	_cart->Init(_emu, this, cartInfo);

	// Initialize Suzy (sprite engine, math, joystick)
	_suzy->Init(_emu, this, _memoryManager.get());

	// Initialize CPU — needs memory manager
	_cpu.reset(new LynxCpu(_emu, this, _memoryManager.get()));

	// Initialize Mikey (timers, display, IRQs) — needs CPU reference for IRQ line
	_mikey->Init(_emu, this, _cpu.get(), _memoryManager.get());

	// Wire memory manager to Mikey and Suzy for dispatching
	_memoryManager->SetMikey(_mikey.get());
	_memoryManager->SetSuzy(_suzy.get());

	// Load battery save if applicable
	LoadBattery();

	// Initialize frame buffer to black
	memset(_frameBuffer, 0, sizeof(_frameBuffer));

	return LoadRomResult::Success;
}

void LynxConsole::RunFrame() {
	// Run CPU instructions for one frame's worth of cycles
	uint32_t targetCycles = LynxConstants::CpuCyclesPerFrame;
	uint64_t startCycle = _cpu->GetCycleCount();

	while (_cpu->GetCycleCount() - startCycle < targetCycles) {
		_cpu->Exec();
		// Tick Mikey timers based on CPU cycle count
		_mikey->Tick(_cpu->GetCycleCount());
	}

	// Copy Mikey's frame buffer to output
	memcpy(_frameBuffer, _mikey->GetFrameBuffer(), sizeof(_frameBuffer));

	// Update input state
	_controlManager->UpdateInputState();
	_suzy->SetJoystick(_controlManager->ReadJoystick());
	_suzy->SetSwitches(_controlManager->ReadSwitches());
}

void LynxConsole::Reset() {
	// The Lynx has no reset button — behave like power cycle
	_emu->ReloadRom(true);
}

void LynxConsole::SaveBattery() {
	if (_saveRam && _saveRamSize > 0) {
		_emu->GetBatteryManager()->SaveBattery(".sav", std::span<const uint8_t>(_saveRam, _saveRamSize));
	}
}

void LynxConsole::LoadBattery() {
	if (_saveRam && _saveRamSize > 0) {
		_emu->GetBatteryManager()->LoadBattery(".sav", std::span<uint8_t>(_saveRam, _saveRamSize));
	}
}

BaseControlManager* LynxConsole::GetControlManager() {
	return _controlManager.get();
}

ConsoleRegion LynxConsole::GetRegion() {
	return ConsoleRegion::Ntsc;
}

ConsoleType LynxConsole::GetConsoleType() {
	return ConsoleType::Lynx;
}

vector<CpuType> LynxConsole::GetCpuTypes() {
	return { CpuType::Lynx };
}

uint64_t LynxConsole::GetMasterClock() {
	return _cpu ? _cpu->GetCycleCount() : 0;
}

uint32_t LynxConsole::GetMasterClockRate() {
	return LynxConstants::MasterClockRate;
}

double LynxConsole::GetFps() {
	return LynxConstants::Fps;
}

BaseVideoFilter* LynxConsole::GetVideoFilter(bool getDefaultFilter) {
	// TODO: Implement LynxDefaultVideoFilter
	return nullptr;
}

PpuFrameInfo LynxConsole::GetPpuFrame() {
	PpuFrameInfo frame = {};
	frame.FirstScanline = 0;
	frame.FrameCount = 0; // TODO: Get from Mikey
	frame.Width = LynxConstants::ScreenWidth;
	frame.Height = LynxConstants::ScreenHeight;
	frame.ScanlineCount = LynxConstants::ScanlineCount;
	frame.CycleCount = LynxConstants::CpuCyclesPerScanline;
	frame.FrameBufferSize = frame.Width * frame.Height * sizeof(uint32_t);
	frame.FrameBuffer = (uint8_t*)_frameBuffer;
	return frame;
}

RomFormat LynxConsole::GetRomFormat() {
	return RomFormat::Lynx;
}

AudioTrackInfo LynxConsole::GetAudioTrackInfo() {
	return {};
}

void LynxConsole::ProcessAudioPlayerAction(AudioPlayerActionParams p) {
	// Not applicable for Lynx
}

AddressInfo LynxConsole::GetAbsoluteAddress(AddressInfo& relAddress) {
	if (_memoryManager) {
		return _memoryManager->GetAbsoluteAddress(relAddress.Address);
	}
	return { -1, MemoryType::None };
}

AddressInfo LynxConsole::GetRelativeAddress(AddressInfo& absAddress, CpuType cpuType) {
	if (_memoryManager) {
		int32_t relAddr = _memoryManager->GetRelativeAddress(absAddress);
		if (relAddr >= 0) {
			return { relAddr, MemoryType::LynxWorkRam };
		}
	}
	return { -1, MemoryType::None };
}

void LynxConsole::GetConsoleState(BaseState& state, ConsoleType consoleType) {
	(LynxState&)state = GetState();
}

LynxState LynxConsole::GetState() {
	LynxState state = {};
	state.Model = _model;
	if (_cpu) state.Cpu = _cpu->GetState();
	if (_mikey) state.Mikey = _mikey->GetState();
	if (_suzy) state.Suzy = _suzy->GetState();
	if (_memoryManager) state.MemoryManager = _memoryManager->GetState();
	if (_controlManager) state.ControlManager = _controlManager->GetState();
	return state;
}

void LynxConsole::Serialize(Serializer& s) {
	SV(_model);
	SV(_rotation);

	if (_cpu) _cpu->Serialize(s);
	if (_mikey) _mikey->Serialize(s);
	if (_suzy) _suzy->Serialize(s);
	if (_cart) _cart->Serialize(s);
	if (_memoryManager) _memoryManager->Serialize(s);
	if (_controlManager) _controlManager->Serialize(s);

	SVArray(_workRam, _workRamSize);
	if (_saveRam && _saveRamSize > 0) {
		SVArray(_saveRam, _saveRamSize);
	}
}
