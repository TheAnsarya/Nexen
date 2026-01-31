#pragma once
#include "pch.h"
#include "WS/WsCpu.h"
#include "WS/APU/WsApu.h"
#include "WS/WsPpu.h"
#include "WS/WsTypes.h"
#include "Utilities/ISerializable.h"

class WsConsole;
class WsTimer;
class WsControlManager;
class WsCart;
class WsSerial;
class WsDmaController;
class WsEeprom;
class Emulator;

/// <summary>
/// WonderSwan / WonderSwan Color memory manager implementation.
/// Handles address decoding, I/O port access, and memory banking.
/// </summary>
/// <remarks>
/// The WonderSwan has a segmented memory architecture:
/// - 64KB Work RAM (WonderSwan Color) / 16KB (original)
/// - Up to 16MB ROM space via banking
/// - 64KB SRAM for save data
/// - Memory-mapped I/O for hardware registers
///
/// **Address Space:**
/// - $00000-$0FFFF: Work RAM (mirrored on original WS)
/// - $10000-$FFFFF: ROM banks (16 x 64KB banks)
///
/// **I/O Ports ($00-$FF):**
/// - Display, sound, timer, serial, cartridge registers
/// - Some ports have wait states affecting timing
/// </remarks>
class WsMemoryManager final : public ISerializable {
private:
	/// <summary>Console instance for system info.</summary>
	WsConsole* _console = nullptr;

	/// <summary>CPU for cycle counting.</summary>
	WsCpu* _cpu = nullptr;

	/// <summary>PPU for rendering sync.</summary>
	WsPpu* _ppu = nullptr;

	/// <summary>APU for audio sync.</summary>
	WsApu* _apu = nullptr;

	/// <summary>Control manager for input.</summary>
	WsControlManager* _controlManager = nullptr;

	/// <summary>Cartridge for ROM/SRAM banking.</summary>
	WsCart* _cart = nullptr;

	/// <summary>Hardware timer.</summary>
	WsTimer* _timer = nullptr;

	/// <summary>Serial port for multiplayer.</summary>
	WsSerial* _serial = nullptr;

	/// <summary>DMA controller for VRAM transfers.</summary>
	WsDmaController* _dmaController = nullptr;

	/// <summary>Internal EEPROM for system settings.</summary>
	WsEeprom* _eeprom = nullptr;

	/// <summary>Emulator instance.</summary>
	Emulator* _emu = nullptr;

	/// <summary>Cartridge ROM data.</summary>
	uint8_t* _prgRom = nullptr;

	/// <summary>Cartridge ROM size.</summary>
	uint32_t _prgRomSize = 0;

	/// <summary>Cartridge SRAM data.</summary>
	uint8_t* _saveRam = nullptr;

	/// <summary>Cartridge SRAM size.</summary>
	uint32_t _saveRamSize = 0;

	/// <summary>Boot ROM data (splash screen).</summary>
	uint8_t* _bootRom = nullptr;

	/// <summary>Boot ROM size.</summary>
	uint32_t _bootRomSize = 0;

	/// <summary>Work RAM size (16KB or 64KB).</summary>
	uint32_t _workRamSize = 0;

	/// <summary>Memory manager state (banking, color mode, etc.).</summary>
	WsMemoryManagerState _state = {};

	/// <summary>Read handler table (4KB pages).</summary>
	uint8_t* _reads[256] = {};

	/// <summary>Write handler table (4KB pages).</summary>
	uint8_t* _writes[256] = {};

	/// <summary>Checks if port is 16-bit (word access).</summary>
	bool IsWordPort(uint16_t port);

	/// <summary>Gets wait states for port access.</summary>
	uint8_t GetPortWaitStates(uint16_t port);

	/// <summary>Checks if port is unmapped.</summary>
	bool IsUnmappedPort(uint16_t port);

public:
	WsMemoryManager() {}

	/// <summary>
	/// Initializes memory manager with all hardware references.
	/// </summary>
	void Init(Emulator* emu, WsConsole* console, WsCpu* cpu, WsPpu* ppu, WsControlManager* controlManager, WsCart* cart, WsTimer* timer, WsDmaController* dmaController, WsEeprom* eeprom, WsApu* apu, WsSerial* serial);

	/// <summary>Gets reference to memory manager state.</summary>
	WsMemoryManagerState& GetState() { return _state; }

	/// <summary>Updates memory mappings based on current state.</summary>
	void RefreshMappings();

	/// <summary>Gets value for unmapped port reads.</summary>
	uint8_t GetUnmappedPort();

	/// <summary>Maps a memory region to a handler.</summary>
	void Map(uint32_t start, uint32_t end, MemoryType type, uint32_t offset, bool readonly);

	/// <summary>Unmaps a memory region.</summary>
	void Unmap(uint32_t start, uint32_t end);

	/// <summary>
	/// Executes one memory cycle - advances CPU, PPU, and APU.
	/// </summary>
	__forceinline void Exec() {
		_cpu->IncCycleCount();  // Increment CPU cycle counter
		_ppu->Exec();           // Run PPU for one cycle
		_apu->Run();            // Run APU for one cycle
	}

	/// <summary>
	/// Reads a byte from memory without side effects for debugging.
	/// </summary>
	/// <param name="addr">20-bit address.</param>
	/// <returns>Byte value or open bus ($90).</returns>
	__forceinline uint8_t InternalRead(uint32_t addr) {
		uint8_t* handler = _reads[addr >> 12];
		uint8_t value = 0x90;  // Open bus default
		if (handler) {
			value = handler[addr & 0xFFF];
		}

		// TODOWS open bus
		return value;
	}

	/// <summary>
	/// Writes a byte to memory (internal, no side effects).
	/// </summary>
	/// <param name="addr">20-bit address.</param>
	/// <param name="value">Byte value.</param>
	__forceinline void InternalWrite(uint32_t addr, uint8_t value) {
		// TODOWS open bus
		uint8_t* handler = _writes[addr >> 12];
		if (handler) {
			handler[addr & 0xFFF] = value;
		}
	}

	uint8_t DebugRead(uint32_t addr);
	void DebugWrite(uint32_t addr, uint8_t value);

	template <typename T>
	T DebugCpuRead(uint16_t seg, uint16_t offset);

	template <typename T>
	__forceinline T Read(uint16_t seg, uint16_t offset, MemoryOperationType opType = MemoryOperationType::Read) {
		uint32_t addr = ((seg << 4) + offset) & 0xFFFFF;
		if constexpr (std::is_same<T, uint16_t>::value) {
			bool splitReads = !IsWordBus(addr) || (addr & 0x01);
			if (splitReads) {
				uint8_t lo = Read<uint8_t>(seg, offset);
				uint8_t hi = Read<uint8_t>(seg, offset + 1);
				return lo | (hi << 8);
			} else {
				Exec();
				uint8_t lo = InternalRead(addr);
				uint8_t hi = InternalRead(((seg << 4) + (uint16_t)(offset + 1)) & 0xFFFFF);
				uint16_t value = lo | (hi << 8);
				_emu->ProcessMemoryRead<CpuType::Ws, 2>(addr, value, opType);
				return value;
			}
		} else {
			Exec();
			uint8_t value = InternalRead(addr);
			_emu->ProcessMemoryRead<CpuType::Ws, 1>(addr, value, opType);
			return value;
		}
	}

	template <typename T>
	__forceinline void Write(uint16_t seg, uint16_t offset, T value, MemoryOperationType opType = MemoryOperationType::Write) {
		uint32_t addr = ((seg << 4) + offset) & 0xFFFFF;
		if constexpr (std::is_same<T, uint16_t>::value) {
			bool splitWrites = !IsWordBus(addr) || (addr & 0x01);
			if (splitWrites) {
				Write<uint8_t>(seg, offset, (uint8_t)value);
				Write<uint8_t>(seg, offset + 1, (uint8_t)(value >> 8));
			} else {
				Exec();
				if (_emu->ProcessMemoryWrite<CpuType::Ws, 2>(addr, value, opType)) {
					InternalWrite(addr, (uint8_t)value);
					InternalWrite(((seg << 4) + (uint16_t)(offset + 1)) & 0xFFFFF, value >> 8);
				}
			}
		} else {
			Exec();
			if (_emu->ProcessMemoryWrite<CpuType::Ws>(addr, value, opType)) {
				InternalWrite(addr, value);
			}
		}
	}

	template <typename T>
	T ReadPort(uint16_t port);
	template <typename T>
	void WritePort(uint16_t port, T value);
	uint8_t InternalReadPort(uint16_t port, bool isWordAccess);
	void InternalWritePort(uint16_t port, uint8_t value, bool isWordAccess);

	template <typename T>
	T DebugReadPort(uint16_t port);

	[[nodiscard]] bool IsPowerOffRequested() { return _state.PowerOffRequested; }
	[[nodiscard]] bool IsColorEnabled() { return _state.ColorEnabled; }

	[[nodiscard]] bool IsWordBus(uint32_t addr);
	[[nodiscard]] uint8_t GetWaitStates(uint32_t addr);

	void SetIrqSource(WsIrqSource src);
	void ClearIrqSource(WsIrqSource src);
	[[nodiscard]] uint8_t GetActiveIrqs();
	[[nodiscard]] uint8_t GetIrqVector();

	void OnBeforeBreak();

	[[nodiscard]] __forceinline bool HasPendingIrq() {
		return GetActiveIrqs() != 0;
	}

	[[nodiscard]] AddressInfo GetAbsoluteAddress(uint32_t relAddr);
	[[nodiscard]] int GetRelativeAddress(AddressInfo& absAddress);

	void Serialize(Serializer& s) override;
};
