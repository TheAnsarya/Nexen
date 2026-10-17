#include "pch.h"
#include "ChannelF/ChannelFConsole.h"
#include "ChannelF/ChannelFTypes.h"
#include "Shared/Emulator.h"
#include "Shared/MemoryType.h"
#include "Shared/FirmwareHelper.h"
#include "Shared/Audio/SoundMixer.h"
#include "Shared/EmuSettings.h"
#include "Utilities/VirtualFile.h"

namespace {
	ChannelFBiosVariant MapConfiguredVariant(ChannelFConsoleVariant variant, ChannelFBiosVariant detectedVariant) {
		switch (variant) {
			case ChannelFConsoleVariant::SystemI: return ChannelFBiosVariant::SystemI;
			case ChannelFConsoleVariant::SystemII:
			case ChannelFConsoleVariant::SystemII_Luxor:
			case ChannelFConsoleVariant::Clone: return ChannelFBiosVariant::SystemII;
			case ChannelFConsoleVariant::Auto:
			default:
				return detectedVariant == ChannelFBiosVariant::Unknown ? ChannelFBiosVariant::SystemI : detectedVariant;
		}
	}

	ConsoleRegion MapConfiguredRegion(const ChannelFConfig& config) {
		if (config.Region != ConsoleRegion::Auto) {
			return config.Region;
		}

		// Known Luxor and clone profiles default to PAL timing in auto mode.
		if (config.ConsoleVariant == ChannelFConsoleVariant::SystemII_Luxor || config.ConsoleVariant == ChannelFConsoleVariant::Clone) {
			return ConsoleRegion::Pal;
		}

		return ConsoleRegion::Ntsc;
	}
}

ChannelFConsole::ChannelFConsole(Emulator* emu)
	: _emu(emu),
	  _core(ChannelFBiosVariant::Unknown),
	  _cpu(std::make_unique<ChannelFCpu>()),
	  _memoryManager(std::make_unique<ChannelFMemoryManager>()),
	  _frameBuffer(ScreenWidth * ScreenHeight, 0) {
	_controlManager = std::make_unique<ChannelFControlManager>(emu);

	// Wire CPU directly to memory manager (no std::function overhead)
	_cpu->Init(_memoryManager.get(), _emu);

	// Wire 3853 SMI interrupt vector updates from memory manager to CPU
	_memoryManager->SetCpu(_cpu.get());
}

ChannelFConsole::~ChannelFConsole() {
}

LoadRomResult ChannelFConsole::LoadRom(VirtualFile& romFile) {
	vector<uint8_t> romData;
	if (!romFile.ReadFile(romData) || romData.empty()) {
		return LoadRomResult::Failure;
	}

	// Channel F carts are bounded in size and should not claim larger BIN images.
	if (romData.size() > ChannelFMemoryManager::MaxCartSize) {
		return LoadRomResult::UnknownType;
	}

	// Genesis/Mega Drive ROMs typically expose the "SEGA" marker at 0x100.
	if (romData.size() >= 0x104 && memcmp(romData.data() + 0x100, "SEGA", 4) == 0) {
		return LoadRomResult::UnknownType;
	}

	_romSha1 = romFile.GetSha1Hash();
	_core.DetectVariantFromHashes(_romSha1, "");
	ChannelFConfig& config = _emu->GetSettings()->GetChannelFConfig();
	_activeVariant = MapConfiguredVariant(config.ConsoleVariant, _core.GetVariant());
	_activeRegion = MapConfiguredRegion(config);
	_romFormat = RomFormat::ChannelF;

	// Load BIOS firmware
	vector<uint8_t> biosData;
	try {
		if (FirmwareHelper::LoadChannelFBios(_emu, biosData)) {
			_memoryManager->LoadBios(biosData.data(), (uint32_t)biosData.size());
		}
	} catch (std::exception&) {
		// Keep default built-in BIOS when firmware paths are not configured (e.g., tests).
	}

	// Load cartridge ROM into memory manager
	_memoryManager->LoadCart(romData.data(), (uint32_t)romData.size());
	_memoryManager->SetMasterClockRate(GetMasterClockRate());

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

		// Feed controller state to memory manager for port reads
		uint8_t ctrl1 = _controlManager->GetController1Value();
		uint8_t ctrl2 = _controlManager->GetController2Value();
		uint8_t console = _controlManager->GetConsoleButtonValue();
		_memoryManager->SetControllerState(ctrl1, ctrl2, console);
	}

	if (_romLoaded) {
		// Clear audio buffer for this frame
		_memoryManager->BeginFrameCapture();

		uint32_t totalScanlines = GetScanlineCount();

		// Run CPU scanline-by-scanline for accurate timing
		for (_scanline = 0; _scanline < totalScanlines; _scanline++) {
			// Assert VBLANK interrupt when visible area ends (line 64)
			if (_scanline == ChannelFConstants::VblankStartLine) {
				_cpu->SetIrqLine(true);
			}

			// Execute CPU cycles for this scanline
			_scanlineCycle = 0;
			while (_scanlineCycle < ChannelFConstants::CyclesPerScanline) {
				uint8_t instrCycles = _cpu->StepOne();
				for (uint8_t i = 0; i < instrCycles; i++) {
					_memoryManager->StepAudio();
				}
				_scanlineCycle += instrCycles;
			}

			// Render visible scanlines as they complete (captures mid-frame palette changes)
			if (_scanline < ChannelFConstants::ScreenHeight) {
				RenderScanline(_scanline);
			}
		}

		// Reset scanline state for debugger queries between frames
		_scanline = 0;
		_scanlineCycle = 0;

		// Send audio buffer to SoundMixer for playback
		const vector<int16_t>& audioBuffer = _memoryManager->GetAudioBuffer();
		if (!audioBuffer.empty()) {
			_emu->GetSoundMixer()->PlayAudioBuffer(
				const_cast<int16_t*>(audioBuffer.data()),
				(uint32_t)audioBuffer.size() / 2,
				GetMasterClockRate());
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
	if (_memoryManager) {
		_memoryManager->Reset();
		_memoryManager->SetMasterClockRate(GetMasterClockRate());
	}
	_frameCount = 0;
	_scanline = 0;
	_scanlineCycle = 0;
	std::fill(_frameBuffer.begin(), _frameBuffer.end(), uint16_t{0});
}

void ChannelFConsole::SaveBattery() {
	// Channel F has no battery-backed SRAM - cart RAM is volatile
}

BaseControlManager* ChannelFConsole::GetControlManager() {
	return _controlManager.get();
}

ConsoleRegion ChannelFConsole::GetRegion() {
	return _activeRegion;
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
	return _activeRegion == ConsoleRegion::Pal ? PalCpuClockHz : NtscCpuClockHz;
}

double ChannelFConsole::GetFps() {
	return _activeRegion == ConsoleRegion::Pal ? PalFps : NtscFps;
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
	frame.ScanlineCount = _activeRegion == ConsoleRegion::Pal ? PalScanlineCount : NtscScanlineCount;
	frame.FirstScanline = 0;
	frame.CycleCount = GetCyclesPerFrame();
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
	if (relAddress.Address < 0) {
		return {-1, MemoryType::None};
	}

	// Pass-through for absolute memory types
	if (relAddress.Type == MemoryType::ChannelFBiosRom || relAddress.Type == MemoryType::ChannelFCartRom ||
		relAddress.Type == MemoryType::ChannelFVideoRam) {
		return relAddress;
	}

	// Map CPU address space to absolute memory type
	uint16_t addr = (uint16_t)relAddress.Address;
	if (addr < ChannelFMemoryManager::BiosSize) {
		return {(int32_t)addr, MemoryType::ChannelFBiosRom};
	}

	uint32_t cartAddr = addr - ChannelFMemoryManager::BiosSize;
	if (cartAddr < _memoryManager->GetCartSize()) {
		return {(int32_t)cartAddr, MemoryType::ChannelFCartRom};
	}

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
	if (absAddress.Address < 0) {
		return {-1, MemoryType::None};
	}

	if (absAddress.Type == MemoryType::ChannelFBiosRom) {
		return {(int32_t)absAddress.Address, MemoryType::ChannelFBiosRom};
	}

	if (absAddress.Type == MemoryType::ChannelFCartRom) {
		return {(int32_t)(absAddress.Address + ChannelFMemoryManager::BiosSize), MemoryType::ChannelFCartRom};
	}

	return absAddress;
}

void ChannelFConsole::GetConsoleState([[maybe_unused]] BaseState& state, [[maybe_unused]] ConsoleType consoleType) {
	ChannelFState& s = reinterpret_cast<ChannelFState&>(state);

	// Fill CPU state
	if (_cpu) {
		uint8_t scratchpad[64];
		_cpu->ExportState(
			s.Cpu.A, s.Cpu.W, s.Cpu.ISAR,
			s.Cpu.PC0, s.Cpu.PC1, s.Cpu.DC0, s.Cpu.DC1,
			s.Cpu.CycleCount, s.Cpu.InterruptsEnabled, scratchpad
		);
		memcpy(s.Cpu.Scratchpad, scratchpad, 64);
		s.Cpu.IrqLine = _cpu->GetIrqLine();
		s.Cpu.InterruptVector = _cpu->GetInterruptVector();
	}

	// Fill video/audio/port state from memory manager
	if (_memoryManager) {
		s.Video = _memoryManager->GetVideoState();
		s.Video.Scanline = _scanline;
		s.Video.Cycle = _scanlineCycle;
		s.Audio = _memoryManager->GetAudioState();
		s.Ports = _memoryManager->GetPortState();
	}
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

	// CPU state
	uint8_t a = 0, w = 0, isar = 0;
	uint16_t pc0 = 0, pc1 = 0, dc0 = 0, dc1 = 0;
	uint64_t cycleCount = 0;
	bool interruptsEnabled = false;
	bool irqLine = false;
	uint16_t interruptVector = 0;
	uint8_t scratchpad[64] = {};

	// Video/Audio/Port state
	ChannelFVideoState videoState = {};
	ChannelFAudioState audioState = {};
	ChannelFPortState portState = {};
	uint8_t vram[ChannelFConstants::VramSize] = {};
	uint8_t cartRam[ChannelFMemoryManager::CartRamSize] = {};

	if (s.IsSaving()) {
		if (_cpu) {
			_cpu->ExportState(a, w, isar, pc0, pc1, dc0, dc1, cycleCount, interruptsEnabled, scratchpad);
			irqLine = _cpu->GetIrqLine();
			interruptVector = _cpu->GetInterruptVector();
		}
		if (_memoryManager) {
			videoState = _memoryManager->GetVideoState();
			videoState.Scanline = _scanline;
			videoState.Cycle = _scanlineCycle;
			audioState = _memoryManager->GetAudioState();
			portState = _memoryManager->GetPortState();
			memcpy(vram, _memoryManager->GetVram(), ChannelFConstants::VramSize);
			if (_memoryManager->HasCartRam()) {
				memcpy(cartRam, _memoryManager->GetCartRamData(), ChannelFMemoryManager::CartRamSize);
			}
		}
	}

	// CPU registers
	SV(a); SV(w); SV(isar);
	SV(pc0); SV(pc1); SV(dc0); SV(dc1);
	SV(cycleCount); SV(interruptsEnabled);
	SV(irqLine); SV(interruptVector);
	s.StreamArray(scratchpad, 64, "scratchpad");

	// Video state
	SV(videoState.Color);
	SV(videoState.X);
	SV(videoState.Y);
	SV(videoState.BackgroundColor);
	SV(videoState.Scanline);
	SV(videoState.Cycle);
	SV(videoState.PendingWrites);

	// Audio state
	SV(audioState.ToneSelect);
	SV(audioState.Volume);
	SV(audioState.SoundEnabled);
	SV(audioState.HalfPeriodCycles);
	SV(audioState.CycleCounter);
	SV(audioState.OutputHigh);

	// Port state
	SV(portState.Port0);
	SV(portState.Port1);
	SV(portState.Port4);
	SV(portState.Port5);

	// VRAM
	s.StreamArray(vram, ChannelFConstants::VramSize, "vram");
	s.StreamArray(cartRam, ChannelFMemoryManager::CartRamSize, "cartRam");

	if (!s.IsSaving()) {
		if (_cpu) {
			_cpu->ImportState(a, w, isar, pc0, pc1, dc0, dc1, cycleCount, interruptsEnabled, scratchpad);
			_cpu->SetIrqLine(irqLine);
			_cpu->SetInterruptVector(interruptVector);
		}
		if (_memoryManager) {
			_memoryManager->SetVideoState(videoState);
			_memoryManager->SetAudioState(audioState);
			_memoryManager->SetPortState(portState);
			memcpy(_memoryManager->GetVramData(), vram, ChannelFConstants::VramSize);
			if (_memoryManager->HasCartRam()) {
				memcpy(_memoryManager->GetCartRamMutable(), cartRam, ChannelFMemoryManager::CartRamSize);
			}
		}
		_scanline = videoState.Scanline;
		_scanlineCycle = videoState.Cycle;
	}
}

ChannelFCpuState ChannelFConsole::GetCpuState() {
	ChannelFCpuState state = {};
	if (_cpu) {
		_cpu->ExportState(
			state.A, state.W, state.ISAR,
			state.PC0, state.PC1, state.DC0, state.DC1,
			state.CycleCount, state.InterruptsEnabled, state.Scratchpad
		);
		state.IrqLine = _cpu->GetIrqLine();
		state.InterruptVector = _cpu->GetInterruptVector();
	}
	return state;
}

void ChannelFConsole::SetCpuState(ChannelFCpuState& state) {
	if (_cpu) {
		_cpu->ImportState(
			state.A, state.W, state.ISAR,
			state.PC0, state.PC1, state.DC0, state.DC1,
			state.CycleCount, state.InterruptsEnabled, state.Scratchpad
		);
		_cpu->SetIrqLine(state.IrqLine);
		_cpu->SetInterruptVector(state.InterruptVector);
	}
}

uint8_t ChannelFConsole::DebugRead(uint16_t addr) {
	if (_memoryManager) {
		return _memoryManager->ReadMemory(addr);
	}
	return 0xff;
}

void ChannelFConsole::DebugRenderFrame() {
	// Re-convert VRAM to frame buffer for debug display
	if (_memoryManager) {
		const uint8_t* vram = _memoryManager->GetVram();
		for (uint32_t i = 0; i < ScreenWidth * ScreenHeight; i++) {
			_frameBuffer[i] = vram[i] & 0x03;
		}
	}
}

uint32_t ChannelFConsole::GetFrameCount() {
	return _frameCount;
}

uint32_t ChannelFConsole::GetCyclesPerFrame() const {
	double fps = _activeRegion == ConsoleRegion::Pal ? PalFps : NtscFps;
	uint32_t clockRate = _activeRegion == ConsoleRegion::Pal ? PalCpuClockHz : NtscCpuClockHz;
	return (uint32_t)(clockRate / fps);
}

uint32_t ChannelFConsole::GetScanlineCount() const {
	return _activeRegion == ConsoleRegion::Pal ? PalScanlineCount : NtscScanlineCount;
}

void ChannelFConsole::RenderScanline(uint16_t line) {
	if (!_memoryManager || line >= ScreenHeight) return;

	const uint8_t* vram = _memoryManager->GetVram();
	ChannelFVideoState videoState = _memoryManager->GetVideoState();
	uint32_t rowBase = line * ScreenWidth;
	uint8_t reg1 = vram[rowBase + 125] & 0x03;
	uint8_t reg2 = vram[rowBase + 126] & 0x03;
	uint16_t paletteOffset = static_cast<uint16_t>(((reg2 & 0x02) | (reg1 >> 1)) << 2);
	uint16_t backgroundColor = static_cast<uint16_t>((videoState.BackgroundColor & 0x03) << 2);
	for (uint32_t x = 0; x < ScreenWidth; x++) {
		uint16_t color = (uint16_t)(vram[rowBase + x] & 0x03);
		_frameBuffer[rowBase + x] = color == 0 ? backgroundColor : (uint16_t)(paletteOffset + color);
	}
}
