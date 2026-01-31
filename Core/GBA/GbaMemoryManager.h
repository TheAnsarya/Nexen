#pragma once
#include "pch.h"
#include "GBA/GbaTypes.h"
#include "GBA/GbaPpu.h"
#include "GBA/GbaTimer.h"
#include "GBA/GbaDmaController.h"
#include "GBA/GbaWaitStates.h"
#include "GBA/GbaRomPrefetch.h"
#include "Debugger/AddressInfo.h"
#include "Utilities/ISerializable.h"

class Emulator;
class GbaConsole;
class GbaPpu;
class GbaDmaController;
class GbaControlManager;
class GbaTimer;
class GbaApu;
class GbaCart;
class GbaSerial;
class MgbaLogHandler;

/// <summary>
/// Pending IRQ with delay counter.
/// IRQs are not instant - they have a small propagation delay.
/// </summary>
struct GbaPendingIrq {
	GbaIrqSource Source;  ///< IRQ source type
	uint8_t Delay;        ///< Cycles until IRQ triggers
};

/// <summary>
/// Game Boy Advance memory manager - unified 32-bit bus system.
/// Handles all memory access, wait states, and DMA coordination.
/// </summary>
/// <remarks>
/// **Memory Map (32-bit address space):**
/// - $00000000-$00003FFF: BIOS ROM (16KB, protected after boot)
/// - $02000000-$0203FFFF: External Work RAM (256KB, 2-cycle wait)
/// - $03000000-$03007FFF: Internal Work RAM (32KB, no wait)
/// - $04000000-$040003FF: I/O Registers
/// - $05000000-$050003FF: Palette RAM (1KB)
/// - $06000000-$06017FFF: VRAM (96KB)
/// - $07000000-$070003FF: OAM (1KB)
/// - $08000000-$09FFFFFF: Game Pak ROM/FlashROM (Wait State 0)
/// - $0A000000-$0BFFFFFF: Game Pak ROM/FlashROM (Wait State 1)
/// - $0C000000-$0DFFFFFF: Game Pak ROM/FlashROM (Wait State 2)
/// - $0E000000-$0E00FFFF: Game Pak SRAM (64KB max)
///
/// **Wait State System:**
/// - Configurable via WAITCNT register ($04000204)
/// - Separate wait states for ROM/SRAM
/// - Sequential vs non-sequential access timing
/// - Prefetch buffer reduces ROM access time
///
/// **DMA Interaction:**
/// - CPU halts during DMA transfers
/// - Some idle cycles can run in parallel with DMA
/// - DMA has priority over CPU bus access
///
/// **BIOS Protection:**
/// - BIOS readable only during BIOS execution
/// - Returns open bus when accessed from game code
///
/// **VRAM Stalling:**
/// - CPU stalls when accessing VRAM during PPU drawing
/// - Wait until PPU enters H-blank or V-blank
/// </remarks>
class GbaMemoryManager final : public ISerializable {
private:
	/// <summary>Emulator instance.</summary>
	Emulator* _emu = nullptr;

	/// <summary>Console instance.</summary>
	GbaConsole* _console = nullptr;

	/// <summary>Pixel Processing Unit.</summary>
	GbaPpu* _ppu = nullptr;

	/// <summary>DMA Controller.</summary>
	GbaDmaController* _dmaController = nullptr;

	/// <summary>Controller manager.</summary>
	GbaControlManager* _controlManager = nullptr;

	/// <summary>Timer system (4 timers).</summary>
	GbaTimer* _timer;

	/// <summary>Audio Processing Unit.</summary>
	GbaApu* _apu;

	/// <summary>Cartridge handler.</summary>
	GbaCart* _cart;

	/// <summary>Serial port (link cable).</summary>
	GbaSerial* _serial;

	/// <summary>ROM prefetch buffer.</summary>
	GbaRomPrefetch* _prefetch;

	/// <summary>Wait state calculator.</summary>
	GbaWaitStates _waitStates;

	/// <summary>mGBA debug log handler.</summary>
	unique_ptr<MgbaLogHandler> _mgbaLog;

	/// <summary>Master clock cycle counter.</summary>
	uint64_t _masterClock = 0;

	/// <summary>Pending updates need processing.</summary>
	bool _hasPendingUpdates = false;

	/// <summary>Late updates need processing.</summary>
	bool _hasPendingLateUpdates = false;

	/// <summary>Memory manager state.</summary>
	GbaMemoryManagerState _state = {};

	/// <summary>Program ROM size.</summary>
	uint32_t _prgRomSize = 0;

	/// <summary>Program ROM data (up to 32MB).</summary>
	uint8_t* _prgRom = nullptr;

	/// <summary>Boot ROM (16KB BIOS).</summary>
	uint8_t* _bootRom = nullptr;

	/// <summary>Internal Work RAM (32KB).</summary>
	uint8_t* _intWorkRam = nullptr;

	/// <summary>External Work RAM (256KB).</summary>
	uint8_t* _extWorkRam = nullptr;

	/// <summary>Video RAM (96KB).</summary>
	uint8_t* _vram = nullptr;

	/// <summary>Object Attribute Memory (1KB).</summary>
	uint8_t* _oam = nullptr;

	/// <summary>Palette RAM (1KB).</summary>
	uint8_t* _palette = nullptr;

	/// <summary>Save RAM/EEPROM/Flash.</summary>
	uint8_t* _saveRam = nullptr;

	/// <summary>Save RAM size.</summary>
	uint32_t _saveRamSize = 0;

	/// <summary>Queue of pending IRQs with delays.</summary>
	vector<GbaPendingIrq> _pendingIrqs;

	/// <summary>Whether HALT mode has been used.</summary>
	bool _haltModeUsed = false;

	/// <summary>Whether BIOS is now locked (after boot).</summary>
	bool _biosLocked = false;

	/// <summary>HALT delay counter.</summary>
	uint8_t _haltDelay = 0;

	/// <summary>IRQ line state at first access cycle.</summary>
	uint8_t _irqFirstAccessCycle = 0;

	/// <summary>DMA IRQ counter.</summary>
	uint8_t _dmaIrqCounter = 0;

	/// <summary>Pending DMA IRQs.</summary>
	uint16_t _dmaIrqPending = 0;

	/// <summary>DMA IRQ line state.</summary>
	uint16_t _dmaIrqLine = 0;

	/// <summary>OBJ enable delay counter.</summary>
	uint8_t _objEnableDelay = 0;

	/// <summary>Processes wait states for memory access.</summary>
	__forceinline void ProcessWaitStates(GbaAccessModeVal mode, uint32_t addr);

	/// <summary>Processes VRAM access with stalling.</summary>
	__noinline void ProcessVramAccess(GbaAccessModeVal mode, uint32_t addr);

	/// <summary>Handles VRAM stalling during PPU access.</summary>
	__noinline void ProcessVramStalling(uint8_t memType);

	/// <summary>Updates open bus value based on access width.</summary>
	template <uint8_t width>
	void UpdateOpenBus(uint32_t addr, uint32_t value);

	/// <summary>Rotates misaligned read value.</summary>
	template <bool debug = false>
	uint32_t RotateValue(GbaAccessModeVal mode, uint32_t addr, uint32_t value, bool isSigned);

	/// <summary>Internal memory read.</summary>
	__forceinline uint8_t InternalRead(GbaAccessModeVal mode, uint32_t addr, uint32_t readAddr);

	/// <summary>Internal memory write.</summary>
	__forceinline void InternalWrite(GbaAccessModeVal mode, uint32_t addr, uint8_t value, uint32_t writeAddr, uint32_t fullValue);

	/// <summary>Reads from I/O register.</summary>
	uint32_t ReadRegister(uint32_t addr);

	/// <summary>Writes to I/O register.</summary>
	void WriteRegister(GbaAccessModeVal mode, uint32_t addr, uint8_t value);

	/// <summary>Triggers IRQ update processing.</summary>
	void TriggerIrqUpdate();

	/// <summary>Processes pending updates (IRQ, timers, etc.).</summary>
	__noinline void ProcessPendingUpdates(bool allowStartDma);

	/// <summary>Processes late updates.</summary>
	__noinline void ProcessPendingLateUpdates();

	/// <summary>Processes idle cycle during DMA.</summary>
	void ProcessParallelIdleCycle();

	/// <summary>Runs ROM prefetch buffer.</summary>
	__forceinline void RunPrefetch();

public:
	/// <summary>Constructs memory manager with all components.</summary>
	GbaMemoryManager(Emulator* emu, GbaConsole* console, GbaPpu* ppu, GbaDmaController* dmaController, GbaControlManager* controlManager, GbaTimer* timer, GbaApu* apu, GbaCart* cart, GbaSerial* serial, GbaRomPrefetch* prefetch);
	~GbaMemoryManager();

	/// <summary>Gets memory manager state.</summary>
	GbaMemoryManagerState& GetState() { return _state; }

	/// <summary>Gets current master clock value.</summary>
	[[nodiscard]] uint64_t GetMasterClock() { return _masterClock; }

	/// <summary>Gets wait state calculator.</summary>
	GbaWaitStates* GetWaitStates() { return &_waitStates; }

	/// <summary>
	/// Processes an idle CPU cycle.
	/// Idle cycles can run in parallel with DMA.
	/// </summary>
	__forceinline void ProcessIdleCycle() {
		if (_dmaController->HasPendingDma()) {
			_dmaController->RunPendingDma(true);
		}

		if (_dmaController->CanRunInParallelWithDma()) {
			// When DMA is running, CPU idle cycles (e.g from MUL or other instructions) can run in parallel
			// with the DMA. The CPU only stops once it tries to read or write to the bus.
			// This allows this idle cycle to run in "parallel" with the DMA
			ProcessParallelIdleCycle();
			return;
		}

		if (_prefetch->NeedExec(_state.PrefetchEnabled)) {
			_prefetch->Exec(1, _state.PrefetchEnabled);
		}

		ProcessInternalCycle<true>();
	}

	/// <summary>
	/// Processes an internal cycle (clock tick).
	/// </summary>
	/// <typeparam name="firstAccessCycle">True if this is the first cycle of a bus access.</typeparam>
	template <bool firstAccessCycle = false>
	__forceinline void ProcessInternalCycle() {
		if (_hasPendingUpdates) {
			ProcessPendingUpdates(firstAccessCycle);
		} else {
			_masterClock++;
			_ppu->Exec();
			_timer->Exec(_masterClock);
		}

		if constexpr (firstAccessCycle) {
			// The CPU appears to check the IRQ line on the first cycle in each read/write access
			// So a 4-cycle read to ROM will check the IRQ line's state after the first of these
			// 4 cycles and this will determine whether or not the CPU runs an extra instruction
			// before processing the IRQ or not.
			// This is needed to pass the Internal_Cycle_DMA_IRQ test
			_irqFirstAccessCycle = _state.IrqLine;
		}

		if (_hasPendingLateUpdates) {
			ProcessPendingLateUpdates();
		}
	}

	/// <summary>Processes DMA start request.</summary>
	void ProcessDmaStart();

	/// <summary>Runs pending DMA transfers.</summary>
	__forceinline void ProcessDma() {
		if (_dmaController->HasPendingDma()) {
			_dmaController->RunPendingDma(true);
		}
	}

	/// <summary>Triggers OBJ enable update.</summary>
	void TriggerObjEnableUpdate();

	/// <summary>Processes cycle in STOP mode.</summary>
	void ProcessStoppedCycle();

	/// <summary>Locks the bus (prevents CPU access).</summary>
	void LockBus() { _state.BusLocked = true; }

	/// <summary>Unlocks the bus.</summary>
	void UnlockBus() { _state.BusLocked = false; }

	/// <summary>Checks if bus is locked by DMA.</summary>
	[[nodiscard]] bool IsBusLocked() { return _state.BusLocked; }

	/// <summary>Checks if system is in STOP mode.</summary>
	[[nodiscard]] bool IsSystemStopped() { return _state.StopMode; }

	/// <summary>Checks if inline HALT can be used.</summary>
	bool UseInlineHalt();

	/// <summary>Sets flag for pending updates.</summary>
	void SetPendingUpdateFlag() { _hasPendingUpdates = true; }

	/// <summary>Sets flag for late updates.</summary>
	void SetPendingLateUpdateFlag() { _hasPendingLateUpdates = true; }

	/// <summary>Reads from memory.</summary>
	uint32_t Read(GbaAccessModeVal mode, uint32_t addr);

	/// <summary>Writes to memory.</summary>
	void Write(GbaAccessModeVal mode, uint32_t addr, uint32_t value);

	/// <summary>Sets IRQ source with delay.</summary>
	void SetDelayedIrqSource(GbaIrqSource source, uint8_t delay);

	/// <summary>Sets IRQ source immediately.</summary>
	void SetIrqSource(GbaIrqSource source);

	/// <summary>Checks if IRQ is pending.</summary>
	bool HasPendingIrq();

	/// <summary>Checks if HALT is complete.</summary>
	bool IsHaltOver();

	/// <summary>Gets open bus value for address.</summary>
	uint8_t GetOpenBus(uint32_t addr);

	/// <summary>Debug CPU read (no side effects).</summary>
	uint32_t DebugCpuRead(GbaAccessModeVal mode, uint32_t addr);

	/// <summary>Debug read (no side effects).</summary>
	uint8_t DebugRead(uint32_t addr);

	/// <summary>Debug write (no side effects).</summary>
	void DebugWrite(uint32_t addr, uint8_t value);

	/// <summary>Gets absolute address from GBA address.</summary>
	AddressInfo GetAbsoluteAddress(uint32_t addr);

	/// <summary>Gets relative address from absolute.</summary>
	int64_t GetRelativeAddress(AddressInfo& absAddress);

	/// <summary>Serializes state for save states.</summary>
	void Serialize(Serializer& s) override;
};
