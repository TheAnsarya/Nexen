#pragma once
#include "pch.h"
#include "PCE/PceTypes.h"
#include "PCE/IPceMapper.h"
#include "Shared/Emulator.h"
#include "Shared/CheatManager.h"
#include "Utilities/HexUtilities.h"
#include "Utilities/ISerializable.h"
#include "Shared/MemoryOperationType.h"

class PceConsole;
class PceVpc;
class PceVce;
class PcePsg;
class PceControlManager;
class PceCdRom;
class PceTimer;

/// <summary>
/// PC Engine memory manager with HuC6280 MMU support.
/// Handles memory mapping, I/O, and bus access.
/// </summary>
/// <remarks>
/// **Memory Map (21-bit physical):**
/// - $000000-$0FFFFF: HuCard ROM (up to 1MB)
/// - $100000-$10FFFF: CD-ROM System Card
/// - $1F0000-$1F7FFF: Work RAM (8KB standard, 32KB+ CD-ROM)
/// - $1FE000-$1FFFFF: Hardware registers (bank $FF)
///
/// **MMU (Memory Paging Registers):**
/// - 8 MPR registers select 8KB banks ($FFF0-$FFF7 for TAM/TMA)
/// - Each MPR value maps to physical address (value * $2000)
/// - $FF selects hardware I/O page
///
/// **Register Addresses (within bank $FF):**
/// - $0000-$03FF: VDC (Video Display Controller)
/// - $0400-$07FF: VCE (Video Color Encoder)
/// - $0800-$0BFF: PSG (Programmable Sound Generator)
/// - $0C00-$0FFF: Timer
/// - $1000-$13FF: I/O Port (Controllers)
/// - $1400-$17FF: IRQ Control
/// - $1800-$1BFF: CD-ROM (if present)
///
/// **Speed Modes:**
/// - High (7.16 MHz): Fast execution
/// - Low (1.79 MHz): Required for VDC access timing
/// </remarks>
class PceMemoryManager final : public ISerializable {
private:
	/// <summary>Emulator instance.</summary>
	Emulator* _emu = nullptr;

	/// <summary>Cheat manager for applying codes.</summary>
	CheatManager* _cheatManager = nullptr;

	/// <summary>Console instance.</summary>
	PceConsole* _console = nullptr;

	/// <summary>Video Priority Controller.</summary>
	PceVpc* _vpc = nullptr;

	/// <summary>Video Color Encoder.</summary>
	PceVce* _vce = nullptr;

	/// <summary>Programmable Sound Generator.</summary>
	PcePsg* _psg = nullptr;

	/// <summary>Controller manager.</summary>
	PceControlManager* _controlManager = nullptr;

	/// <summary>CD-ROM interface (if present).</summary>
	PceCdRom* _cdrom = nullptr;

	/// <summary>Hardware timer.</summary>
	PceTimer* _timer = nullptr;

	/// <summary>HuCard/SuperGrafx mapper.</summary>
	IPceMapper* _mapper = nullptr;

	/// <summary>Execution function pointer type.</summary>
	typedef void (PceMemoryManager::*Func)();

	/// <summary>Current execution handler.</summary>
	Func _exec = nullptr;

	/// <summary>Fast cycle execution handler.</summary>
	Func _fastExec = nullptr;

	/// <summary>Memory manager state (MPR values, IRQs).</summary>
	PceMemoryManagerState _state = {};

	/// <summary>Program ROM data.</summary>
	uint8_t* _prgRom = nullptr;

	/// <summary>Program ROM size in bytes.</summary>
	uint32_t _prgRomSize = 0;

	/// <summary>Read bank pointers (256 possible banks).</summary>
	uint8_t* _readBanks[0x100] = {};

	/// <summary>Write bank pointers (256 possible banks).</summary>
	uint8_t* _writeBanks[0x100] = {};

	/// <summary>Memory type for each bank.</summary>
	MemoryType _bankMemType[0x100] = {};

	/// <summary>Work RAM (8KB standard, larger with CD-ROM).</summary>
	uint8_t* _workRam = nullptr;

	/// <summary>Work RAM size in bytes.</summary>
	uint32_t _workRamSize = 0;

	/// <summary>Optional HuCard RAM.</summary>
	uint8_t* _cardRam = nullptr;

	/// <summary>Card RAM size in bytes.</summary>
	uint32_t _cardRamSize = 0;

	/// <summary>Start bank for card RAM.</summary>
	uint32_t _cardRamStartBank = 0;

	/// <summary>End bank for card RAM.</summary>
	uint32_t _cardRamEndBank = 0;

	/// <summary>Unmapped/open bus bank.</summary>
	uint8_t* _unmappedBank = nullptr;

	/// <summary>Battery-backed save RAM.</summary>
	uint8_t* _saveRam = nullptr;

	/// <summary>CD-ROM RAM buffer.</summary>
	uint8_t* _cdromRam = nullptr;

	/// <summary>Whether CD-ROM unit is present.</summary>
	bool _cdromUnitEnabled = false;

public:
	/// <summary>
	/// Constructs memory manager with all hardware components.
	/// </summary>
	PceMemoryManager(Emulator* emu, PceConsole* console, PceVpc* vpc, PceVce* vce, PceControlManager* controlManager, PcePsg* psg, PceTimer* timer, IPceMapper* mapper, PceCdRom* cdrom, vector<uint8_t>& romData, uint32_t cardRamSize, bool cdromUnitEnabled);
	~PceMemoryManager();

	/// <summary>Gets current memory manager state.</summary>
	PceMemoryManagerState& GetState() { return _state; }

	/// <summary>Sets CPU speed mode (slow = 1.79 MHz).</summary>
	void SetSpeed(bool slow);

	/// <summary>Updates bank mappings from MPR values.</summary>
	void UpdateMappings(uint32_t bankOffsets[8]);

	/// <summary>Updates CD-ROM specific bank mappings.</summary>
	void UpdateCdRomBanks();

	/// <summary>Updates execution callback for current mode.</summary>
	void UpdateExecCallback();

	/// <summary>Templated execution for different configurations.</summary>
	template <bool hasCdRom, bool isSuperGrafx>
	void ExecTemplate();

	/// <summary>Slow-speed execution path.</summary>
	void ExecSlow();

	/// <summary>Executes one cycle.</summary>
	__forceinline void Exec() { (this->*_exec)(); }

	/// <summary>Executes one fast cycle.</summary>
	__forceinline void ExecFastCycle() { (this->*_fastExec)(); }

	/// <summary>Reads byte from memory.</summary>
	__forceinline uint8_t Read(uint16_t addr, MemoryOperationType type = MemoryOperationType::Read);

	/// <summary>Writes byte to memory.</summary>
	__forceinline void Write(uint16_t addr, uint8_t value, MemoryOperationType type);

	/// <summary>Reads from hardware register.</summary>
	uint8_t ReadRegister(uint16_t addr);

	/// <summary>Writes to hardware register.</summary>
	void WriteRegister(uint16_t addr, uint8_t value);

	/// <summary>Writes to VDC registers.</summary>
	void WriteVdc(uint16_t addr, uint8_t value);

	/// <summary>Debug read (no side effects).</summary>
	uint8_t DebugRead(uint16_t addr);

	/// <summary>Debug write (no side effects).</summary>
	void DebugWrite(uint16_t addr, uint8_t value);

	/// <summary>Sets MPR (Memory Paging Register) value.</summary>
	void SetMprValue(uint8_t regSelect, uint8_t value);

	/// <summary>Gets MPR value.</summary>
	uint8_t GetMprValue(uint8_t regSelect);

	/// <summary>Converts logical to absolute address.</summary>
	AddressInfo GetAbsoluteAddress(uint32_t relAddr);

	/// <summary>Converts absolute to logical address.</summary>
	AddressInfo GetRelativeAddress(AddressInfo absAddr, uint16_t pc);

	/// <summary>Sets an IRQ source active.</summary>
	void SetIrqSource(PceIrqSource source) { _state.ActiveIrqs |= (int)source; }

	/// <summary>Gets pending (unmasked) IRQs.</summary>
	__forceinline uint8_t GetPendingIrqs() { return (_state.ActiveIrqs & ~_state.DisabledIrqs); }

	/// <summary>Checks if specific IRQ source is pending.</summary>
	__forceinline bool HasIrqSource(PceIrqSource source) { return (_state.ActiveIrqs & ~_state.DisabledIrqs & (int)source) != 0; }

	/// <summary>Clears an IRQ source.</summary>
	void ClearIrqSource(PceIrqSource source) { _state.ActiveIrqs &= ~(int)source; }

	/// <summary>Serializes state for save states.</summary>
	void Serialize(Serializer& s);
};

__forceinline uint8_t PceMemoryManager::Read(uint16_t addr, MemoryOperationType type) {
	uint8_t bank = _state.Mpr[(addr & 0xE000) >> 13];
	uint8_t value;
	if (bank != 0xFF) {
		value = _readBanks[bank][addr & 0x1FFF];
	} else {
		value = ReadRegister(addr & 0x1FFF);
	}

	if (_mapper && _mapper->IsBankMapped(bank)) {
		value = _mapper->Read(bank, addr, value);
	}

	if (_cheatManager->HasCheats<CpuType::Pce>()) {
		_cheatManager->ApplyCheat<CpuType::Pce>((bank << 13) | (addr & 0x1FFF), value);
	}
	_emu->ProcessMemoryRead<CpuType::Pce>(addr, value, type);
	return value;
}

__forceinline void PceMemoryManager::Write(uint16_t addr, uint8_t value, MemoryOperationType type) {
	if (_emu->ProcessMemoryWrite<CpuType::Pce>(addr, value, type)) {
		uint8_t bank = _state.Mpr[(addr & 0xE000) >> 13];
		if (_mapper && _mapper->IsBankMapped(bank)) {
			_mapper->Write(bank, addr, value);
		}

		if (bank == 0xF7) {
			if (_writeBanks[bank] && (addr & 0x1FFF) <= 0x7FF) {
				// Only allow writes to the first 2kb - save RAM is not mirrored
				_writeBanks[bank][addr & 0x7FF] = value;
			}
		} else if (bank != 0xFF) {
			if (_writeBanks[bank]) {
				_writeBanks[bank][addr & 0x1FFF] = value;
			}
		} else {
			WriteRegister(addr & 0x1FFF, value);
		}
	}
}