#pragma once
#include "pch.h"
#include "Genesis/GenesisTypes.h"
#include "Genesis/GenesisVdp.h"
#include "Shared/Emulator.h"
#include "Shared/MemoryOperationType.h"
#include "Debugger/AddressInfo.h"
#include "Utilities/ISerializable.h"
#include "Utilities/Serializer.h"

class Emulator;
class GenesisConsole;
class GenesisM68k;
class GenesisVdp;
class GenesisControlManager;
class GenesisPsg;

class GenesisMemoryManager final : public ISerializable {
private:
	static constexpr uint32_t WorkRamSize = 0x10000;   // 64KB work RAM
	static constexpr uint32_t Z80RamSize = 0x2000;     // 8KB Z80 RAM

	Emulator* _emu = nullptr;
	GenesisConsole* _console = nullptr;
	GenesisM68k* _cpu = nullptr;
	GenesisVdp* _vdp = nullptr;
	GenesisControlManager* _controlManager = nullptr;
	GenesisPsg* _psg = nullptr;

	uint8_t* _prgRom = nullptr;
	uint32_t _prgRomSize = 0;

	uint8_t* _workRam = nullptr;
	uint8_t* _z80Ram = nullptr;
	uint8_t* _saveRam = nullptr;
	uint32_t _saveRamSize = 0;
	uint32_t _sramStart = 0;
	uint32_t _sramEnd = 0;
	bool _hasSram = false;
	bool _sramEvenBytes = true;
	bool _sramOddBytes = true;

	GenesisIoState _ioState = {};

	uint64_t _masterClock = 0;
	uint8_t _openBus = 0;

	// Z80 bus
	bool _z80BusRequest = false;
	bool _z80Reset = true;

	// TMSS (Trademark Security System)
	bool _tmssEnabled = false;
	bool _tmssUnlocked = false;

	bool IsSramAddress(uint32_t addr) const;
	bool TryGetSramOffset(uint32_t addr, uint32_t& offset) const;

public:
	GenesisMemoryManager();
	~GenesisMemoryManager();

	void Init(Emulator* emu, GenesisConsole* console, vector<uint8_t>& romData, GenesisVdp* vdp, GenesisControlManager* controlManager, GenesisPsg* psg);
	void LoadBattery();
	void SaveBattery();

	void SetCpu(GenesisM68k* cpu) { _cpu = cpu; }

	// Memory access (24-bit address space)
	uint8_t Read8(uint32_t addr);
	uint16_t Read16(uint32_t addr);
	void Write8(uint32_t addr, uint8_t value);
	void Write16(uint32_t addr, uint16_t value);

	// Debug access
	uint8_t DebugRead8(uint32_t addr);
	void DebugWrite8(uint32_t addr, uint8_t value);

	// VDP access
	uint16_t ReadVdpPort(uint32_t addr);
	void WriteVdpPort(uint32_t addr, uint16_t value);

	// I/O access
	uint8_t ReadIo(uint32_t addr);
	void WriteIo(uint32_t addr, uint8_t value);

	// Clock
	__forceinline void Exec(uint32_t cycles) {
		_masterClock += cycles;
		_vdp->Run(_masterClock);
	}

	uint64_t GetMasterClock() const { return _masterClock; }
	GenesisIoState GetIoState() const { return _ioState; }
	bool HasSaveRam() const { return _hasSram && _saveRam && _saveRamSize > 0; }

	// Address info
	AddressInfo GetAbsoluteAddress(uint32_t addr);
	int32_t GetRelativeAddress(AddressInfo& absAddress);

	uint8_t GetOpenBus() const { return _openBus; }

	void Serialize(Serializer& s) override;
};
