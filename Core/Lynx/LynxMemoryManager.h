#pragma once
#include "pch.h"
#include "Lynx/LynxTypes.h"
#include "Debugger/AddressInfo.h"
#include "Shared/MemoryOperationType.h"
#include "Utilities/ISerializable.h"

class LynxConsole;
class LynxCpu;
class LynxMikey;
class LynxSuzy;
class Emulator;

/// <summary>
/// Lynx memory manager — handles the 64KB flat address space with
/// MAPCTL-based overlays for Suzy, Mikey, ROM, and vector space.
///
/// Memory map (all addresses 16-bit, flat):
///   $0000-$FBFF  — always RAM
///   $FC00-$FCFF  — Suzy registers (when MAPCTL bit 0 = 0) or RAM
///   $FD00-$FDFF  — Mikey registers (when MAPCTL bit 1 = 0) or RAM
///   $FE00-$FFF7  — Boot ROM (when MAPCTL bit 2 = 0) or RAM
///   $FFF8         — reserved
///   $FFF9         — MAPCTL register (always writable)
///   $FFFA-$FFFF  — ROM vectors (when MAPCTL bit 3 = 0) or RAM
///
/// MAPCTL ($FFF9) bits:
///   Bit 0: 0 = Suzy space visible, 1 = RAM at $FC00-$FCFF
///   Bit 1: 0 = Mikey space visible, 1 = RAM at $FD00-$FDFF
///   Bit 2: 0 = ROM space visible, 1 = RAM at $FE00-$FFF7
///   Bit 3: 0 = Vector space (ROM) visible, 1 = RAM at $FFFA-$FFFF
///   Bit 4: Sequential cart access disable
///   Bit 5: CPU bus request held until next timer expire
/// </summary>
class LynxMemoryManager final : public ISerializable {
private:
	LynxConsole* _console = nullptr;
	Emulator* _emu = nullptr;

	// Component pointers (set via Init)
	LynxMikey* _mikey = nullptr;
	LynxSuzy* _suzy = nullptr;

	// Memory regions (owned by LynxConsole)
	uint8_t* _workRam = nullptr;
	uint32_t _workRamSize = 0;
	uint8_t* _bootRom = nullptr;
	uint32_t _bootRomSize = 0;

	LynxMemoryManagerState _state = {};

public:
	LynxMemoryManager() {}

	void Init(Emulator* emu, LynxConsole* console, uint8_t* workRam, uint8_t* bootRom, uint32_t bootRomSize);

	void SetMikey(LynxMikey* mikey) { _mikey = mikey; }
	void SetSuzy(LynxSuzy* suzy) { _suzy = suzy; }

	[[nodiscard]] LynxMemoryManagerState& GetState() { return _state; }

	/// <summary>CPU read — dispatches through MAPCTL overlays</summary>
	[[nodiscard]] uint8_t Read(uint16_t addr, MemoryOperationType opType = MemoryOperationType::Read);

	/// <summary>CPU write — dispatches through MAPCTL overlays</summary>
	void Write(uint16_t addr, uint8_t value, MemoryOperationType opType = MemoryOperationType::Write);

	/// <summary>Debug read — no side effects</summary>
	[[nodiscard]] uint8_t DebugRead(uint16_t addr);

	/// <summary>Debug write — no side effects</summary>
	void DebugWrite(uint16_t addr, uint8_t value);

	/// <summary>Get absolute address for debugger</summary>
	[[nodiscard]] AddressInfo GetAbsoluteAddress(uint16_t relAddr);

	/// <summary>Get relative address for debugger</summary>
	[[nodiscard]] int32_t GetRelativeAddress(AddressInfo& absAddress);

	void Serialize(Serializer& s) override;

private:
	/// <summary>Update derived MAPCTL state from raw register value</summary>
	void UpdateMapctl(uint8_t value);

	/// <summary>Check if address falls in Suzy register space and overlay is active</summary>
	[[nodiscard]] __forceinline bool IsSuzyAddress(uint16_t addr) const {
		return !_state.SuzySpaceVisible ? false : (addr >= LynxConstants::SuzyBase && addr <= LynxConstants::SuzyEnd);
	}

	/// <summary>Check if address falls in Mikey register space and overlay is active</summary>
	[[nodiscard]] __forceinline bool IsMikeyAddress(uint16_t addr) const {
		return !_state.MikeySpaceVisible ? false : (addr >= LynxConstants::MikeyBase && addr <= LynxConstants::MikeyEnd);
	}

	/// <summary>Check if address falls in ROM space and overlay is active</summary>
	[[nodiscard]] __forceinline bool IsRomAddress(uint16_t addr) const {
		return !_state.RomSpaceVisible ? false : (addr >= LynxConstants::BootRomBase && addr <= 0xfff7);
	}

	/// <summary>Check if address falls in vector space and overlay is active</summary>
	[[nodiscard]] __forceinline bool IsVectorAddress(uint16_t addr) const {
		return !_state.VectorSpaceVisible ? false : (addr >= 0xfffa);
	}
};
