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
#include "Shared/MessageManager.h"
#include "Genesis/GenesisDefaultVideoFilter.h"
#include "Debugger/DebugTypes.h"
#include "Shared/Video/BaseVideoFilter.h"
#include "Shared/RenderedFrame.h"
#include "Shared/Video/VideoDecoder.h"
#include "Shared/RewindManager.h"
#include "Utilities/Serializer.h"
#include "Shared/EventType.h"

namespace {
	bool HasSegaHeader(const vector<uint8_t>& romData) {
		if (romData.size() < 0x104) {
			return false;
		}

		return romData[0x100] == 'S' && romData[0x101] == 'E' && romData[0x102] == 'G' && romData[0x103] == 'A';
	}

	bool IsLikelySmdImage(const vector<uint8_t>& romData, const string& extension) {
		if (romData.size() <= 0x200) {
			return false;
		}

		if (extension == ".smd") {
			return true;
		}

		// Typical SMD layout is a 512-byte header plus 16KB interleaved payload blocks.
		if ((romData.size() & 0x3fff) != 0x200) {
			return false;
		}

		// Avoid false positives when the raw image already looks like linear Genesis ROM.
		return !HasSegaHeader(romData);
	}

	void DecodeSmdToLinear(vector<uint8_t>& romData) {
		if (romData.size() <= 0x200) {
			return;
		}

		size_t payloadSize = romData.size() - 0x200;
		if ((payloadSize & 0x3fff) != 0) {
			// Irregular payload: safest fallback is to strip the copier header only.
			romData.erase(romData.begin(), romData.begin() + 0x200);
			return;
		}

		vector<uint8_t> decoded(payloadSize);
		const uint8_t* src = romData.data() + 0x200;

		for (size_t block = 0; block < payloadSize; block += 0x4000) {
			const uint8_t* in = src + block;
			uint8_t* out = decoded.data() + block;
			for (size_t i = 0; i < 0x2000; i++) {
				out[(i << 1)] = in[0x2000 + i];
				out[(i << 1) + 1] = in[i];
			}
		}

		romData.swap(decoded);
	}

	ConsoleRegion ResolveConfiguredGenesisRegion(ConsoleRegion configuredRegion) {
		switch (configuredRegion) {
			case ConsoleRegion::Pal:
				return ConsoleRegion::Pal;

			case ConsoleRegion::NtscJapan:
			case ConsoleRegion::Ntsc:
			case ConsoleRegion::Dendy:
				return ConsoleRegion::Ntsc;

			case ConsoleRegion::Auto:
			default:
				return ConsoleRegion::Auto;
		}
	}

	ConsoleRegion DetectGenesisRegionFromHeader(const vector<uint8_t>& romData) {
		if (romData.size() < 0x1F0) {
			return ConsoleRegion::Ntsc;
		}

		uint32_t regionMask = 0;
		bool hasPalMarker = false;
		bool hasNtscMarker = false;
		bool foundAnyRegionMarker = false;
		uint32_t endOffset = (uint32_t)std::min<size_t>(romData.size(), 0x200);
		for (uint32_t i = 0x1F0; i < endOffset; i++) {
			char marker = (char)std::toupper((unsigned char)romData[i]);
			if (marker == 0 || marker == ' ') {
				continue;
			}

			if (marker == 'E') {
				foundAnyRegionMarker = true;
				hasPalMarker = true;
			} else if (marker == 'U' || marker == 'J') {
				foundAnyRegionMarker = true;
				hasNtscMarker = true;
			} else if (std::isxdigit((unsigned char)marker)) {
				foundAnyRegionMarker = true;
				if (marker >= '0' && marker <= '9') {
					regionMask |= (uint32_t)(marker - '0');
				} else {
					regionMask |= (uint32_t)(10 + marker - 'A');
				}
			}
		}

		if (regionMask != 0) {
			hasPalMarker = hasPalMarker || ((regionMask & 0x08) != 0);
			hasNtscMarker = hasNtscMarker || ((regionMask & 0x01) != 0) || ((regionMask & 0x04) != 0);
		}

		if (!foundAnyRegionMarker) {
			return ConsoleRegion::Ntsc;
		}

		if (hasPalMarker && !hasNtscMarker) {
			return ConsoleRegion::Pal;
		}

		return ConsoleRegion::Ntsc;
	}
}

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

	string ext = romFile.GetFileExtension();
	std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
		return (char)std::tolower(c);
	});

	if (IsLikelySmdImage(romData, ext)) {
		DecodeSmdToLinear(romData);
	}

	// Pad ROM to power of 2
	uint32_t power = (uint32_t)std::log2(romData.size());
	if (romData.size() > ((uint64_t)1 << power)) {
		uint32_t newSize = 1 << (power + 1);
		romData.insert(romData.end(), newSize - romData.size(), 0);
	}

	EmuSettings* settings = _emu->GetSettings();
	ConsoleRegion configuredRegion = ResolveConfiguredGenesisRegion(settings->GetGenesisConfig().Region);
	_region = configuredRegion == ConsoleRegion::Auto ? DetectGenesisRegionFromHeader(romData) : configuredRegion;

	// Create all hardware components
	_vdp = std::make_unique<GenesisVdp>();
	_memoryManager = std::make_unique<GenesisMemoryManager>();
	_controlManager = std::make_unique<GenesisControlManager>(_emu, this);
	_psg = std::make_unique<GenesisPsg>(_emu, this);
	_cpu = std::make_unique<GenesisM68k>();

	_memoryManager->Init(_emu, this, romData, _vdp.get(), _controlManager.get(), _psg.get());
	_vdp->Init(_emu, this, _cpu.get(), _memoryManager.get());
	_emu->RegisterMemory(MemoryType::GenesisVideoRam, _vdp->GetVramPointer(), 0x10000);
	_emu->RegisterMemory(MemoryType::GenesisPaletteRam, _vdp->GetCramPointer(), 128);
	_memoryManager->SetCpu(_cpu.get());
	_cpu->Init(_emu, this, _memoryManager.get());
	_cpu->Reset(false);
	_memoryManager->LoadBattery();

	_vdp->SetRegion(_region == ConsoleRegion::Pal);
	MessageManager::Log(std::format("[Genesis] ROM loaded: {} size={} region={} tmss={}",
		romFile.GetFileName(),
		romData.size(),
		_region == ConsoleRegion::Pal ? "PAL" : "NTSC",
		_memoryManager->GetIoState().TmssEnabled ? "on" : "off"));

	return LoadRomResult::Success;
}

void GenesisConsole::Reset() {
	static uint64_t resetCount = 0;
	resetCount++;
	if (resetCount <= 64 || (resetCount % 1024) == 0) {
		uint32_t pcBeforeReset = _cpu ? (_cpu->GetState().PC & 0x00ffffff) : 0;
		MessageManager::Log(std::format("[Genesis] Console Reset #{} pcBefore=${:06x}", resetCount, pcBeforeReset));
	}

	_cpu->Reset(true);
	if (_vdp) {
		_vdp->Reset(false);
		_vdp->SetRegion(_region == ConsoleRegion::Pal);
	}
	if (_controlManager) {
		_controlManager->ResetRuntimeState();
	}
	if (_memoryManager) {
		_memoryManager->ResetRuntimeState(true);
	}
	if (_psg) {
		_psg->Reset();
	}
}

void GenesisConsole::RunFrame() {
	_emu->ProcessEvent(EventType::StartFrame, CpuType::Genesis);

	uint32_t frame = _vdp->GetFrameCount();
	uint64_t startClock = _memoryManager->GetMasterClock();
	uint32_t guard = 0;
	while (frame == _vdp->GetFrameCount()) {
		_cpu->Exec();
		guard++;
		if ((guard % 2000000) == 0) {
			MessageManager::Log(std::format("[Genesis] RunFrame waiting frame={} guard={} pc=${:06x} cycles={} masterClock={}",
				frame,
				guard,
				_cpu->GetState().PC & 0x00ffffff,
				_cpu->GetState().CycleCount,
				_memoryManager->GetMasterClock()));
		}
	}

	uint32_t nextFrame = _vdp->GetFrameCount();
	if ((nextFrame % 120) == 0) {
		GenesisVdpState vdpState = _vdp->GetState();
		MessageManager::Log(std::format("[Genesis] Frame advanced to {} (deltaClock={}) display={} R1=${:02x} tmssUnlocked={}",
			nextFrame,
			_memoryManager->GetMasterClock() - startClock,
			(vdpState.Registers[1] & 0x40) ? "on" : "off",
			vdpState.Registers[1],
			_memoryManager->GetIoState().TmssUnlocked ? "true" : "false"));
	}

	_emu->ProcessEvent(EventType::EndFrame, CpuType::Genesis);

	if (_emu && _emu->IsEmulationThread() && _vdp && _controlManager) {
		uint16_t* frameBuffer = _vdp->GetScreenBuffer(true);
		if (frameBuffer) {
			bool rewinding = _emu->GetRewindManager()->IsRewinding();
			RenderedFrame renderedFrame(
				frameBuffer,
				_vdp->GetScreenWidth(),
				_vdp->GetScreenHeight(),
				1.0,
				nextFrame,
				_controlManager->GetPortStates());
			_emu->GetVideoDecoder()->UpdateFrame(renderedFrame, rewinding, rewinding);
		}
	}

	ProcessEndOfFrame();
}

void GenesisConsole::ProcessEndOfFrame() {
	if (!_controlManager) {
		return;
	}

	if (_emu && _emu->IsEmulationThread()) {
		_controlManager->UpdateControlDevices();
		_controlManager->UpdateInputState();
		_emu->ProcessEndOfFrame();
	} else {
		// Keep headless frame tests deterministic without requiring host input stack initialization.
		_controlManager->SetPollCounter(_controlManager->GetPollCounter() + 1);
		_controlManager->ProcessEndOfFrame();
	}
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
	frame.FrameBuffer = (uint8_t*)_vdp->GetScreenBuffer(true);
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
