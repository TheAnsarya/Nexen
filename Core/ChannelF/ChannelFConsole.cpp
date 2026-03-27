#include "pch.h"
#include "ChannelF/ChannelFConsole.h"
#include "Shared/Emulator.h"
#include "Shared/MemoryType.h"
#include "Utilities/VirtualFile.h"

ChannelFConsole::ChannelFConsole(Emulator* emu)
	: _emu(emu),
	  _core(ChannelFBiosVariant::Unknown),
	  _cpu(std::make_unique<ChannelFCpu>()),
	  _memoryManager(std::make_unique<ChannelFMemoryManager>()),
	  _frameBuffer(ScreenWidth * ScreenHeight, 0) {
	_controlManager = std::make_unique<ChannelFControlManager>(emu);

	// Wire CPU callbacks to memory manager
	_cpu->SetReadCallback([this](uint16_t addr) {
		return _memoryManager->ReadMemory(addr);
	});
	_cpu->SetWriteCallback([this](uint16_t addr, uint8_t value) {
		_memoryManager->WriteMemory(addr, value);
	});
	_cpu->SetReadPortCallback([this](uint8_t port) {
		return _memoryManager->ReadPort(port);
	});
	_cpu->SetWritePortCallback([this](uint8_t port, uint8_t value) {
		_memoryManager->WritePort(port, value);
	});
}

ChannelFConsole::~ChannelFConsole() {
}

LoadRomResult ChannelFConsole::LoadRom(VirtualFile& romFile) {
	vector<uint8_t> romData;
	if (!romFile.ReadFile(romData) || romData.empty()) {
		return LoadRomResult::Failure;
	}

	_romSha1 = romFile.GetSha1Hash();
	_core.DetectVariantFromHashes(_romSha1, "");
	_romFormat = RomFormat::ChannelF;

	// Load cartridge ROM into memory manager
	_memoryManager->LoadCart(romData.data(), (uint32_t)romData.size());

	// Register memory regions with the emulator for debugger
	_emu->RegisterMemory(MemoryType::ChannelFBiosRom,
		const_cast<uint8_t*>(_memoryManager->GetBiosData()),
		_memoryManager->GetBiosSize());
	_emu->RegisterMemory(MemoryType::ChannelFCartRom,
		const_cast<uint8_t*>(_memoryManager->GetCartData()),
		_memoryManager->GetCartSize());
	_emu->RegisterMemory(MemoryType::ChannelFVideoRam,
		_memoryManager->GetVramData(),
		ChannelFMemoryManager::VramSize);

	_romLoaded = true;
	Reset();
	return LoadRomResult::Success;
}

void ChannelFConsole::RunFrame() {
	if (_controlManager) {
		_controlManager->UpdateControlDevices();
		_controlManager->UpdateInputState();
	}

	if (_romLoaded) {
		// Execute CPU for one frame's worth of cycles
		_cpu->StepCycles(CyclesPerFrame);

		// Convert VRAM (2-bit color indices) to frame buffer (palette indices)
		const uint8_t* vram = _memoryManager->GetVram();
		for (uint32_t i = 0; i < ScreenWidth * ScreenHeight; i++) {
			_frameBuffer[i] = vram[i] & 0x03;
		}
	} else {
		// Scaffold mode when no ROM loaded
		_core.RunFrame();
		uint8_t pixel = (uint8_t)(_core.GetDeterministicState() & 0x7f);
		std::fill(_frameBuffer.begin(), _frameBuffer.end(), pixel);
	}

	_frameCount++;

	if (_controlManager) {
		_controlManager->ProcessEndOfFrame();
	}
}

void ChannelFConsole::Reset() {
	_core.Reset();
	if (_cpu) _cpu->Reset();
	if (_memoryManager) _memoryManager->Reset();
	_frameCount = 0;
	std::fill(_frameBuffer.begin(), _frameBuffer.end(), uint16_t{0});
}

void ChannelFConsole::SaveBattery() {
}

BaseControlManager* ChannelFConsole::GetControlManager() {
	return _controlManager.get();
}

ConsoleRegion ChannelFConsole::GetRegion() {
	return ConsoleRegion::Ntsc;
}

ConsoleType ChannelFConsole::GetConsoleType() {
	return ConsoleType::ChannelF;
}

vector<CpuType> ChannelFConsole::GetCpuTypes() {
	return {CpuType::ChannelF};
}

uint64_t ChannelFConsole::GetMasterClock() {
	if (_cpu) return _cpu->GetCycleCount();
	return _core.GetMasterClock();
}

uint32_t ChannelFConsole::GetMasterClockRate() {
	return CpuClockHz;
}

double ChannelFConsole::GetFps() {
	return Fps;
}

BaseVideoFilter* ChannelFConsole::GetVideoFilter([[maybe_unused]] bool getDefaultFilter) {
	return new ChannelFDefaultVideoFilter(_emu);
}

PpuFrameInfo ChannelFConsole::GetPpuFrame() {
	PpuFrameInfo frame = {};
	frame.FrameBuffer = (uint8_t*)_frameBuffer.data();
	frame.Width = ScreenWidth;
	frame.Height = ScreenHeight;
	frame.FrameBufferSize = (uint32_t)(_frameBuffer.size() * sizeof(uint16_t));
	frame.FrameCount = _frameCount;
	frame.ScanlineCount = ScreenHeight;
	frame.FirstScanline = 0;
	frame.CycleCount = CyclesPerFrame;
	return frame;
}

RomFormat ChannelFConsole::GetRomFormat() {
	return _romFormat;
}

AudioTrackInfo ChannelFConsole::GetAudioTrackInfo() {
	return {};
}

void ChannelFConsole::ProcessAudioPlayerAction([[maybe_unused]] AudioPlayerActionParams p) {
}

AddressInfo ChannelFConsole::GetAbsoluteAddress([[maybe_unused]] AddressInfo& relAddress) {
	return {-1, MemoryType::None};
}

AddressInfo ChannelFConsole::GetPcAbsoluteAddress() {
	if (_cpu) {
		uint16_t pc = _cpu->GetPc();
		if (pc < ChannelFMemoryManager::BiosSize) {
			return {(int32_t)pc, MemoryType::ChannelFBiosRom};
		}
		return {(int32_t)(pc - ChannelFMemoryManager::BiosSize), MemoryType::ChannelFCartRom};
	}
	return {-1, MemoryType::None};
}

AddressInfo ChannelFConsole::GetRelativeAddress([[maybe_unused]] AddressInfo& absAddress, [[maybe_unused]] CpuType cpuType) {
	return {-1, MemoryType::None};
}

void ChannelFConsole::GetConsoleState([[maybe_unused]] BaseState& state, [[maybe_unused]] ConsoleType consoleType) {
}

string ChannelFConsole::GetHash(HashType hashType) {
	if (hashType == HashType::Sha1 || hashType == HashType::Sha1Cheat) {
		return _romSha1;
	}
	return {};
}

void ChannelFConsole::Serialize(Serializer& s) {
	_core.Serialize(s);
	s.Stream(_frameCount, "frameCount");
	s.Stream(_romFormat, "romFormat");
	s.Stream(_romLoaded, "romLoaded");

	// Serialize CPU state
	if (_cpu) {
		uint8_t a, w, isar;
		uint16_t pc0, pc1, dc0, dc1;
		uint64_t cycleCount;
		bool interruptsEnabled;
		uint8_t scratchpad[64];

		if (s.IsSaving()) {
			_cpu->ExportState(a, w, isar, pc0, pc1, dc0, dc1, cycleCount, interruptsEnabled, scratchpad);
		}

		s.Stream(a, "cpuA");
		s.Stream(w, "cpuW");
		s.Stream(isar, "cpuIsar");
		s.Stream(pc0, "cpuPc0");
		s.Stream(pc1, "cpuPc1");
		s.Stream(dc0, "cpuDc0");
		s.Stream(dc1, "cpuDc1");
		s.Stream(cycleCount, "cpuCycleCount");
		s.Stream(interruptsEnabled, "cpuInterruptsEnabled");
		s.StreamArray(scratchpad, 64, "cpuScratchpad");

		if (!s.IsSaving()) {
			_cpu->ImportState(a, w, isar, pc0, pc1, dc0, dc1, cycleCount, interruptsEnabled, scratchpad);
		}
	}
}
