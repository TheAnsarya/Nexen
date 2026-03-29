#include "pch.h"
#include "Lynx/LynxConsole.h"
#include "Lynx/LynxTypes.h"
#include "Lynx/LynxCpu.h"
#include "Lynx/LynxMikey.h"
#include "Lynx/LynxSuzy.h"
#include "Lynx/LynxMemoryManager.h"
#include "Lynx/LynxCart.h"
#include "Lynx/LynxControlManager.h"
#include "Lynx/LynxApu.h"
#include "Lynx/LynxEeprom.h"
#include "Lynx/LynxGameDatabase.h"
#include "Lynx/AtariLynxFormat.h"
#include "Utilities/CRC32.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"
#include "Shared/MessageManager.h"
#include "Shared/BatteryManager.h"
#include "Shared/FirmwareHelper.h"
#include "Utilities/Serializer.h"
#include "Utilities/VirtualFile.h"

// EEPROM CRC32 lookup now handled by LynxGameDatabase

LynxConsole::LynxConsole(Emulator* emu) {
	_emu = emu;
}

LynxConsole::~LynxConsole() {
}

LoadRomResult LynxConsole::LoadRom(VirtualFile& romFile) {

	vector<uint8_t> romData;
	if (!romFile.ReadFile(romData) || romData.size() < 32) {
		return LoadRomResult::Failure;
	}

	MessageManager::Log("------------------------------");

	// Detect format and extract ROM data + metadata
	// Three supported formats: .atari-lynx (LYNXROM magic), LNX (LYNX magic), raw (LYX/O)
	bool hasLnxHeader = false;
	bool hasAtariLynxHeader = false;
	uint32_t romOffset = 0;
	AtariLynxMetadata atariLynxMeta;
	bool hasAtariLynxMeta = false;

	if (AtariLynxFormat::IsAtariLynxFormat(romData.data(), romData.size())) {
		// .atari-lynx format — parse with the format reader
		hasAtariLynxHeader = true;
		auto result = AtariLynxFormat::Load(romData.data(), romData.size());
		if (!result.Success) {
			MessageManager::Log(std::format("Failed to load .atari-lynx file: {}", result.Error));
			return LoadRomResult::Failure;
		}

		// Replace romData with extracted ROM data for the rest of the pipeline
		romData = std::move(result.RomData);
		romOffset = 0;
		atariLynxMeta = std::move(result.Metadata);
		hasAtariLynxMeta = result.HasMetadata;

		// Apply rotation from metadata
		switch (atariLynxMeta.Rotation) {
			case 1: _rotation = LynxRotation::Left; break;
			case 2: _rotation = LynxRotation::Right; break;
			default: _rotation = LynxRotation::None; break;
		}

		MessageManager::Log(".atari-lynx format:");
		if (hasAtariLynxMeta && !atariLynxMeta.CartName.empty()) {
			MessageManager::Log(std::format("  Cart Name: {}", atariLynxMeta.CartName));
		}
		if (hasAtariLynxMeta && !atariLynxMeta.Manufacturer.empty()) {
			MessageManager::Log(std::format("  Manufacturer: {}", atariLynxMeta.Manufacturer));
		}
	} else if (AtariLynxFormat::IsLnxFormat(romData.data(), romData.size())) {
		// LNX format — 64-byte header
		hasLnxHeader = true;
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
		// Headerless .lyx/.o format — raw ROM data
		MessageManager::Log("Headerless ROM (raw .lyx/.o format)");
		_rotation = LynxRotation::None;
	}

	// Extract ROM data
	_prgRomSize = static_cast<uint32_t>(romData.size() - romOffset);
	if (_prgRomSize == 0) {
		return LoadRomResult::Failure;
	}

	_prgRom = std::make_unique<uint8_t[]>(_prgRomSize);
	memcpy(_prgRom.get(), romData.data() + romOffset, _prgRomSize);
	_emu->RegisterMemory(MemoryType::LynxPrgRom, _prgRom.get(), _prgRomSize);

	MessageManager::Log(std::format("ROM Size: {} KB", _prgRomSize / 1024));

	// Allocate work RAM (64 KB)
	_workRamSize = LynxConstants::WorkRamSize;
	_workRam = std::make_unique<uint8_t[]>(_workRamSize);
	std::fill_n(_workRam.get(), _workRamSize, uint8_t{0});
	_emu->RegisterMemory(MemoryType::LynxWorkRam, _workRam.get(), _workRamSize);

	// Boot ROM — optional, loaded from firmware
	vector<uint8_t> bootRomData;
	if (FirmwareHelper::LoadLynxBootRom(_emu, bootRomData)) {
		_bootRomSize = static_cast<uint32_t>(bootRomData.size());
		_bootRom = std::make_unique<uint8_t[]>(_bootRomSize);
		memcpy(_bootRom.get(), bootRomData.data(), _bootRomSize);
		_emu->RegisterMemory(MemoryType::LynxBootRom, _bootRom.get(), _bootRomSize);
		MessageManager::Log("Boot ROM loaded successfully.");
	} else {
		// No boot ROM — use HLE (High-Level Emulation) fallback.
		// Skip the boot animation and set up the post-boot hardware state
		// so games can run without the real boot ROM.
		_bootRomSize = 0;
		_bootRom.reset();
		MessageManager::Log("No boot ROM found — using HLE fallback.");
	}

	// Save RAM (EEPROM) — managed by LynxEeprom, not through _saveRam
	_saveRamSize = 0;
	_saveRam.reset();

	MessageManager::Log(std::format("Work RAM: {} KB", _workRamSize / 1024));

	// Look up game in ROM database for auto-detection
	uint32_t prgCrc32 = CRC32::GetCRC(_prgRom.get(), _prgRomSize);
	const LynxGameDatabase::Entry* dbEntry = LynxGameDatabase::Lookup(prgCrc32);
	if (dbEntry) {
		MessageManager::Log(std::format("ROM Database: {} (CRC32: {:08x})", dbEntry->Name, prgCrc32));
	}

	MessageManager::Log("------------------------------");

	// Create components
	_controlManager = std::make_unique<LynxControlManager>(_emu, this);
	_memoryManager = std::make_unique<LynxMemoryManager>();
	_cart = std::make_unique<LynxCart>();
	_mikey = std::make_unique<LynxMikey>();
	_suzy = std::make_unique<LynxSuzy>();

	// Initialize memory manager first (it needs RAM, boot ROM)
	_memoryManager->Init(_emu, this, _workRam.get(), _bootRom.get(), _bootRomSize);

	// Initialize cart
	LynxCartInfo cartInfo = {};
	if (hasAtariLynxHeader && hasAtariLynxMeta) {
		// .atari-lynx format — populate from parsed metadata
		cartInfo.PageSizeBank0 = atariLynxMeta.Bank0PageSize;
		cartInfo.PageSizeBank1 = atariLynxMeta.Bank1PageSize;
		cartInfo.RomSize = _prgRomSize;
		strncpy_s(cartInfo.Name, sizeof(cartInfo.Name), atariLynxMeta.CartName.c_str(), 32);
		cartInfo.Name[32] = '\0';
		strncpy_s(cartInfo.Manufacturer, sizeof(cartInfo.Manufacturer), atariLynxMeta.Manufacturer.c_str(), 16);
		cartInfo.Manufacturer[16] = '\0';
		cartInfo.Rotation = static_cast<LynxRotation>(atariLynxMeta.Rotation);
		if (atariLynxMeta.EepromType >= 1 && atariLynxMeta.EepromType <= 5) {
			cartInfo.HasEeprom = true;
			cartInfo.EepromType = static_cast<LynxEepromType>(atariLynxMeta.EepromType);
		}
	} else if (hasLnxHeader) {
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

		// Byte 59: AUDIN (reserved, unused for now)
		// Byte 60: EEPROM type (BLL/LNX standard)
		uint8_t eepromByte = romData[60];
		// Mask off SD flag (bit 6) and 8-bit org flag (bit 7) — we only care about chip type
		uint8_t eepromChip = eepromByte & 0x0f;
		if (eepromChip >= 1 && eepromChip <= 5) {
			cartInfo.HasEeprom = true;
			cartInfo.EepromType = static_cast<LynxEepromType>(eepromChip);
			MessageManager::Log(std::format("  EEPROM: 93C{} ({} bytes)",
				eepromChip == 1 ? "46" : eepromChip == 2 ? "56" : eepromChip == 3 ? "66" :
				eepromChip == 4 ? "76" : "86",
				(1 << (eepromChip + 6))));  // 128, 256, 512, 1024, 2048
		} else {
			cartInfo.HasEeprom = false;
			cartInfo.EepromType = LynxEepromType::None;
		}
	} else {
		cartInfo.PageSizeBank0 = static_cast<uint16_t>(_prgRomSize / 256);
		cartInfo.PageSizeBank1 = 0;
		cartInfo.RomSize = _prgRomSize;
		cartInfo.HasEeprom = false;
		cartInfo.EepromType = LynxEepromType::None;

		// For headerless ROMs, apply database overrides for rotation and EEPROM
		if (dbEntry) {
			_rotation = dbEntry->Rotation;
			cartInfo.Rotation = dbEntry->Rotation;
			if (dbEntry->EepromType != LynxEepromType::None) {
				cartInfo.HasEeprom = true;
				cartInfo.EepromType = dbEntry->EepromType;
				MessageManager::Log(std::format("  EEPROM auto-detected from database: 93C46"));
			}
			// Copy game title from database
			strncpy_s(cartInfo.Name, sizeof(cartInfo.Name), dbEntry->Name, 32);
			cartInfo.Name[32] = 0;
		}
	}
	_cart->Init(_emu, this, cartInfo);

	// Initialize Suzy (sprite engine, math, joystick)
	_suzy->Init(_emu, this, _memoryManager.get(), _cart.get());

	// Initialize CPU — needs memory manager
	_cpu = std::make_unique<LynxCpu>(_emu, this, _memoryManager.get());

	// Initialize Mikey (timers, display, IRQs) — needs CPU reference for IRQ line
	// MUST be called before HLE boot state, as Init() zeroes all Mikey state
	_mikey->Init(_emu, this, _cpu.get(), _memoryManager.get());

	// Wire cart to Mikey for SYSCTL1 bank strobe
	_mikey->SetCart(_cart.get());

	// Initialize APU (audio channels, integrated into Mikey)
	_apu = std::make_unique<LynxApu>(_emu, this);
	_apu->Init();
	_mikey->SetApu(_apu.get());

	// Initialize EEPROM (serial protocol for battery-backed save data)
	_eeprom = std::make_unique<LynxEeprom>(_emu, this);
	if (cartInfo.HasEeprom) {
		_eeprom->Init(cartInfo.EepromType);
	}
	// Note: Database EEPROM detection for headerless ROMs is already handled
	// above in the headerless ROM loading path (sets cartInfo.HasEeprom = true).

	// Wire EEPROM to Mikey for IODAT register access
	_mikey->SetEeprom(_eeprom.get());

	// Wire memory manager to Mikey and Suzy for dispatching
	_memoryManager->SetMikey(_mikey.get());
	_memoryManager->SetSuzy(_suzy.get());

	// Load battery save if applicable
	LoadBattery();

	// HLE fallback: When no boot ROM is present, the reset vector at $FFFC-$FFFD
	// reads from RAM (which is all zeros), so PC = $0000. We need to set up the
	// post-boot state that the boot ROM would have configured:
	//  - Stack pointer initialized
	//  - Display timing configured (Mikey timers 0 and 2)
	//  - Display address set
	//  - MAPCTL configured to show ROM/Mikey/Suzy
	//  - Jump to cart entry point
	// NOTE: This must be called AFTER Mikey::Init() (which zeroes all state)
	// and after all subsystems are wired up.
	if (!_bootRom) {
		ApplyHleBootState();
	}

	// Initialize frame buffer to black
	std::fill_n(_frameBuffer, LynxConstants::PixelCount, 0u);

	return LoadRomResult::Success;
}

void LynxConsole::RunFrame() {
	// Log key emulation state for first 3 frames to help debug display issues
	if (_frameCount < 3) {
		auto& cpuState = _cpu->GetState();
		auto& t0 = _mikey->GetTimerState(0);
		auto& t2 = _mikey->GetTimerState(2);
		MessageManager::Log(std::format("Frame {}: PC=${:04x} SP=${:02x} A=${:02x} PS=${:02x} MAPCTL=${:02x}",
			_frameCount, cpuState.PC, cpuState.SP, cpuState.A, cpuState.PS, _memoryManager->GetMapctl()));
		MessageManager::Log(std::format("  T0: CTLA=${:02x} Count=${:02x} Backup=${:02x} Done={}  T2: CTLA=${:02x} Count=${:02x} Done={}",
			t0.ControlA, t0.Count, t0.BackupValue, t0.TimerDone,
			t2.ControlA, t2.Count, t2.TimerDone));
		MessageManager::Log(std::format("  DISPCTL=${:02x} DISPADR=${:04x} Scanline={}",
			_mikey->GetDisplayControl(), _mikey->GetDisplayAddress(), _mikey->GetCurrentScanline()));
	}

	// Update input state BEFORE frame execution to avoid 1-frame input lag.
	// On real hardware, Suzy latches joystick state when the CPU reads $FCB0/$FCB1
	// mid-frame, so input must be current before execution begins.
	_controlManager->UpdateInputState();
	_suzy->SetJoystick(_controlManager->ReadJoystick());
	_suzy->SetSwitches(_controlManager->ReadSwitches());

	// Run CPU instructions for one frame's worth of cycles
	uint32_t targetCycles = LynxConstants::CpuCyclesPerFrame;
	uint64_t startCycle = _cpu->GetCycleCount();

	while (_cpu->GetCycleCount() - startCycle < targetCycles) {
		_cpu->Exec();
		// Cache cycle count to avoid redundant pointer-chasing per iteration
		uint64_t cycle = _cpu->GetCycleCount();
		// Tick Mikey timers based on CPU cycle count
		_mikey->Tick(cycle);
		// Tick audio — _apu is always initialized after LoadRom()
		_apu->Tick(cycle);
	}

	// Flush remaining audio samples
	_apu->EndFrame();

	// Copy Mikey's frame buffer to output
	memcpy(_frameBuffer, _mikey->GetFrameBuffer(), sizeof(_frameBuffer));
	_frameCount++;
}

void LynxConsole::Reset() {
	// The Lynx has no reset button — behave like power cycle
	_emu->ReloadRom(true);
}

void LynxConsole::SaveBattery() {
	if (_saveRam && _saveRamSize > 0) {
		_emu->GetBatteryManager()->SaveBattery(".sav", std::span<const uint8_t>(_saveRam.get(), _saveRamSize));
	}
	if (_eeprom) {
		_eeprom->SaveBattery();
	}
}

void LynxConsole::LoadBattery() {
	if (_eeprom) {
		_eeprom->LoadBattery();
	}
	if (_saveRam && _saveRamSize > 0) {
		_emu->GetBatteryManager()->LoadBattery(".sav", std::span<uint8_t>(_saveRam.get(), _saveRamSize));
	}
}

void LynxConsole::ApplyHleBootState() {
	// High-Level Emulation of the Lynx boot ROM.
	// When no boot ROM file is provided, we simulate the post-boot hardware
	// state so the cart can execute immediately.
	//
	// The real boot ROM ($FE00-$FFFF, 512 bytes) does:
	// 1. Initializes hardware registers (MAPCTL, timers, display)
	// 2. Reads the first 256 bytes from cart and decrypts with RSA
	// 3. The decrypted block contains: dest address, size, entry point
	// 4. Reads 'size' more bytes from cart into RAM at dest address
	// 5. Jumps to the entry point
	//
	// Without the RSA key, we can't decrypt commercial game headers.
	// We support two HLE modes:
	// A) BLL (Bastard Lynx Loader) format: first byte $80, unencrypted header
	// B) Fallback: copy ROM data to $0200 and start there

	// --- CPU state ---
	auto& cpuState = _cpu->GetState();
	cpuState.SP = 0xff;
	cpuState.PS = 0x04 | LynxPSFlags::Reserved; // IRQs disabled, Reserved always set

	// --- Memory map: all overlays enabled ---
	_memoryManager->Write(0xfff9, 0x00, MemoryOperationType::Write);

	// --- Display: Configure Mikey for 160×102 display ---
	_mikey->WriteRegister(0x00, 0x9e); // Timer 0 BACKUP = 158
	_mikey->WriteRegister(0x01, 0x18); // Timer 0 CTLA = $18 (enable, prescaler 0)
	_mikey->WriteRegister(0x08, 0x68); // Timer 2 BACKUP = 104
	_mikey->WriteRegister(0x09, 0x1f); // Timer 2 CTLA = $1F (enable, linked)
	_mikey->WriteRegister(0x92, 0x09); // DISPCTL = $09 (DMA enabled, color)
	_mikey->WriteRegister(0x94, 0x00); // DISPADR low
	_mikey->WriteRegister(0x95, 0xc0); // DISPADR high = $C000

	_mikey->WriteRegister(0x80, 0x00); // Clear pending IRQs

	// --- Load cart data into RAM ---
	// The boot ROM reads cart data into RAM so the game can execute.
	// Without this step, RAM is all zeros and the CPU just hits BRK.
	uint16_t entryPoint = 0x0200; // Default Lynx loader address
	uint8_t* rom = _prgRom.get();
	uint32_t romSize = _prgRomSize;

	if (romSize >= 10 && rom[0] == 0x80) {
		// BLL (Bastard Lynx Loader) format — unencrypted header
		// Byte 0: $80 (magic)
		// Bytes 1-2: Destination address in RAM (little-endian)
		// Bytes 3-4: Number of bytes to load (little-endian)
		// Bytes 5-6: Entry point (little-endian, 0 = use dest address)
		uint16_t destAddr = rom[1] | (static_cast<uint16_t>(rom[2]) << 8);
		uint16_t loadSize = rom[3] | (static_cast<uint16_t>(rom[4]) << 8);
		uint16_t entryAddr = rom[5] | (static_cast<uint16_t>(rom[6]) << 8);

		if (entryAddr == 0) {
			entryAddr = destAddr;
		}

		// Copy ROM data (after 10-byte BLL header) into RAM at destAddr
		uint32_t srcOffset = 10;
		uint32_t bytesToCopy = std::min(static_cast<uint32_t>(loadSize), romSize - srcOffset);
		bytesToCopy = std::min(bytesToCopy, static_cast<uint32_t>(0x10000 - destAddr));

		for (uint32_t i = 0; i < bytesToCopy; i++) {
			_workRam[(destAddr + i) & 0xffff] = rom[srcOffset + i];
		}

		entryPoint = entryAddr;
		MessageManager::Log(std::format("HLE boot: BLL header — load {} bytes to ${:04x}, entry ${:04x}",
			bytesToCopy, destAddr, entryPoint));
	} else {
		// No BLL header — commercial game with encrypted loader.
		// Without the boot ROM's RSA key, we can't decrypt the first block.
		// Best-effort: copy ROM data to $0200 (standard loader address).
		uint32_t bytesToCopy = std::min(romSize, static_cast<uint32_t>(0xfbff - 0x0200));
		for (uint32_t i = 0; i < bytesToCopy; i++) {
			_workRam[0x0200 + i] = rom[i];
		}
		entryPoint = 0x0200;
		MessageManager::Log("HLE boot: No BLL header — copying ROM to $0200 (may not work without boot ROM)");
	}

	cpuState.PC = entryPoint;
	MessageManager::Log(std::format("HLE boot: PC=${:04x}, SP=$FF", entryPoint));
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

BaseVideoFilter* LynxConsole::GetVideoFilter([[maybe_unused]] bool getDefaultFilter) {
	// Lynx only supports the default video filter (no NTSC, bisqwit, etc.),
	// so the getDefaultFilter parameter is intentionally unused.
	return new LynxDefaultVideoFilter(_emu, this);
}

PpuFrameInfo LynxConsole::GetPpuFrame() {
	PpuFrameInfo frame = {};
	frame.FirstScanline = 0;
	frame.FrameCount = _frameCount;
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

AddressInfo LynxConsole::GetPcAbsoluteAddress() {
	if (_memoryManager) {
		return _memoryManager->GetAbsoluteAddress(_cpu->GetState().PC);
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
	reinterpret_cast<LynxState&>(state) = GetState();
}

LynxState LynxConsole::GetState() {
	LynxState state = {};
	state.Model = _model;
	if (_cpu) state.Cpu = _cpu->GetState();
	if (_mikey) state.Mikey = _mikey->GetState();
	if (_suzy) state.Suzy = _suzy->GetState();
	if (_memoryManager) state.MemoryManager = _memoryManager->GetState();
	if (_controlManager) state.ControlManager = _controlManager->GetState();
	// APU state is inside Mikey.Apu (populated by _mikey->GetState() above)
	if (_cart) state.Cart = _cart->GetState();
	if (_eeprom) state.Eeprom = _eeprom->GetState();

	// PPU state is synthesized from Mikey display registers and console frame count
	state.Ppu.FrameCount = _frameCount;
	if (_mikey) {
		// Use already-populated state.Mikey (from GetState() call above) to
		// avoid calling _mikey->GetState() again and copying the full struct.
		state.Ppu.Scanline = state.Mikey.CurrentScanline;
		state.Ppu.DisplayAddress = state.Mikey.DisplayAddress;
		state.Ppu.DisplayControl = state.Mikey.DisplayControl;
		state.Ppu.LcdEnabled = (state.Mikey.DisplayControl & 0x01) != 0;
	}
	return state;
}

void LynxConsole::Serialize(Serializer& s) {
	SV(_model);
	SV(_rotation);
	SV(_frameCount);

	if (_cpu) _cpu->Serialize(s);
	if (_mikey) _mikey->Serialize(s);
	if (_suzy) _suzy->Serialize(s);
	if (_apu) _apu->Serialize(s);
	if (_eeprom) _eeprom->Serialize(s);
	if (_cart) _cart->Serialize(s);
	if (_memoryManager) _memoryManager->Serialize(s);
	if (_controlManager) _controlManager->Serialize(s);

	SVArray(_workRam.get(), _workRamSize);
	if (_saveRam && _saveRamSize > 0) {
		SVArray(_saveRam.get(), _saveRamSize);
	}
}
