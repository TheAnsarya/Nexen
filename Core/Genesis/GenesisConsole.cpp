#include "pch.h"
#include "Genesis/GenesisConsole.h"
#include "Genesis/GenesisM68k.h"
#include "Genesis/GenesisVdp.h"
#include "Genesis/GenesisControlManager.h"
#include "Genesis/GenesisMemoryManager.h"
#include "Genesis/GenesisPsg.h"
#include "Genesis/GenesisTypes.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"
#include "Genesis/GenesisDefaultVideoFilter.h"
#include "Shared/Video/BaseVideoFilter.h"
#include "Utilities/Serializer.h"

GenesisConsole::GenesisConsole(Emulator* emu) {
	_emu = emu;
}

GenesisConsole::~GenesisConsole() {
}

LoadRomResult GenesisConsole::LoadRom(VirtualFile& romFile) {
	vector<uint8_t> romData;
	(void)romFile.ReadFile(romData);

	if (romData.size() < 0x200) {
		return LoadRomResult::Failure;
	}

	// Handle SMD interleaved format
	string ext = romFile.GetFileExtension();
	if (ext == ".smd" && romData.size() > 0x200) {
		// Strip 512-byte header
		vector<uint8_t> deinterleaved;
		size_t dataSize = romData.size() - 0x200;
		deinterleaved.resize(dataSize);

		for (size_t i = 0; i < dataSize / 0x4000; i++) {
			size_t srcOffset = 0x200 + i * 0x4000;
			size_t dstOffset = i * 0x4000;

			// De-interleave: odd bytes first, then even bytes
			for (size_t j = 0; j < 0x2000; j++) {
				deinterleaved[dstOffset + j * 2 + 1] = romData[srcOffset + j];
				deinterleaved[dstOffset + j * 2] = romData[srcOffset + 0x2000 + j];
			}
		}
		romData = std::move(deinterleaved);
	}

	// Pad ROM to power of 2
	uint32_t power = (uint32_t)std::log2(romData.size());
	if (romData.size() > ((uint64_t)1 << power)) {
		uint32_t newSize = 1 << (power + 1);
		romData.insert(romData.end(), newSize - romData.size(), 0);
	}

	// Detect region from ROM header
	if (romData.size() >= 0x1F2) {
		char regionChar = (char)romData[0x1F0];
		if (regionChar == 'E') {
			_region = ConsoleRegion::Pal;
		} else {
			_region = ConsoleRegion::Ntsc;
		}
	}

	// Create all hardware components
	_vdp = std::make_unique<GenesisVdp>();
	_memoryManager = std::make_unique<GenesisMemoryManager>();
	_controlManager = std::make_unique<GenesisControlManager>(_emu, this);
	_psg = std::make_unique<GenesisPsg>(_emu, this);
	_cpu = std::make_unique<GenesisM68k>();

	_memoryManager->Init(_emu, this, romData, _vdp.get(), _controlManager.get(), _psg.get());
	_vdp->Init(_emu, this, _cpu.get(), _memoryManager.get());
	_memoryManager->SetCpu(_cpu.get());
	_cpu->Init(_emu, this, _memoryManager.get());
	_cpu->Reset(false);
	_memoryManager->LoadBattery();

	_vdp->SetRegion(_region == ConsoleRegion::Pal);

	return LoadRomResult::Success;
}

void GenesisConsole::Reset() {
	_cpu->Reset(true);
	if (_psg) {
		_psg->Reset();
	}
}

void GenesisConsole::RunFrame() {
	uint32_t frame = _vdp->GetFrameCount();
	while (frame == _vdp->GetFrameCount()) {
		_cpu->Exec();
	}
}

void GenesisConsole::ProcessEndOfFrame() {
	_controlManager->UpdateControlDevices();
	_controlManager->UpdateInputState();
}

void GenesisConsole::SaveBattery() {
	if (_memoryManager) {
		_memoryManager->SaveBattery();
	}
}

BaseControlManager* GenesisConsole::GetControlManager() {
	return _controlManager.get();
}

ConsoleRegion GenesisConsole::GetRegion() {
	return _region;
}

ConsoleType GenesisConsole::GetConsoleType() {
	return ConsoleType::Genesis;
}

vector<CpuType> GenesisConsole::GetCpuTypes() {
	return {CpuType::Genesis};
}

RomFormat GenesisConsole::GetRomFormat() {
	return RomFormat::Genesis;
}

double GenesisConsole::GetFps() {
	return (_region == ConsoleRegion::Pal) ? 50.0 : 59.922743;
}

uint64_t GenesisConsole::GetMasterClock() {
	return _memoryManager->GetMasterClock();
}

uint32_t GenesisConsole::GetMasterClockRate() {
	// M68000 @ 7.670454 MHz (NTSC), 7.600489 MHz (PAL)
	return (_region == ConsoleRegion::Pal) ? 7600489 : 7670454;
}

PpuFrameInfo GenesisConsole::GetPpuFrame() {
	PpuFrameInfo frame = {};
	frame.FirstScanline = 0;
	frame.FrameCount = _vdp->GetFrameCount();
	frame.Width = _vdp->GetScreenWidth();
	frame.Height = _vdp->GetScreenHeight();
	frame.ScanlineCount = (_region == ConsoleRegion::Pal) ? 313 : 262;
	frame.CycleCount = 488; // Cycles per scanline at 68k rate
	frame.FrameBufferSize = frame.Width * frame.Height * sizeof(uint16_t);
	frame.FrameBuffer = (uint8_t*)_vdp->GetScreenBuffer(false);
	return frame;
}

BaseVideoFilter* GenesisConsole::GetVideoFilter(bool getDefaultFilter) {
	return new GenesisDefaultVideoFilter(_emu, this);
}

AudioTrackInfo GenesisConsole::GetAudioTrackInfo() {
	return AudioTrackInfo();
}

void GenesisConsole::ProcessAudioPlayerAction(AudioPlayerActionParams p) {
}

AddressInfo GenesisConsole::GetAbsoluteAddress(AddressInfo& relAddress) {
	return _memoryManager->GetAbsoluteAddress(relAddress.Address);
}

AddressInfo GenesisConsole::GetPcAbsoluteAddress() {
	return _memoryManager->GetAbsoluteAddress(_cpu->GetState().PC);
}

AddressInfo GenesisConsole::GetRelativeAddress(AddressInfo& absAddress, CpuType cpuType) {
	return {_memoryManager->GetRelativeAddress(absAddress), MemoryType::GenesisMemory};
}

GenesisState GenesisConsole::GetState() {
	GenesisState state;
	state.Cpu = _cpu->GetState();
	state.Vdp = _vdp->GetState();
	state.Io = _memoryManager->GetIoState();
	if (_psg) {
		state.Psg = _psg->GetState();
	}
	return state;
}

void GenesisConsole::GetConsoleState(BaseState& state, ConsoleType consoleType) {
	(GenesisState&)state = GetState();
}

void GenesisConsole::InitializeRam(void* data, uint32_t length) {
	EmuSettings* settings = _emu->GetSettings();
	settings->InitializeRam(settings->GetGenesisConfig().RamPowerOnState, data, length);
}

void GenesisConsole::Serialize(Serializer& s) {
	SV(_cpu);
	SV(_vdp);
	SV(_controlManager);
	SV(_memoryManager);
	SV(_psg);
}
