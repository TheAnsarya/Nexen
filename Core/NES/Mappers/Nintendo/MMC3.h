#pragma once

#include "pch.h"
#include "NES/BaseMapper.h"
#include "NES/Mappers/A12Watcher.h"
#include "NES/NesConsole.h"
#include "NES/NesCpu.h"
#include "NES/BaseNesPpu.h"

/// <summary>
/// Nintendo MMC3 mapper (iNES mapper 4) - Most common NES mapper.
/// Used by hundreds of games including SMB3, Mega Man 3-6, Double Dragon II/III.
/// </summary>
/// <remarks>
/// Features:
/// - 8KB switchable PRG-ROM (2 banks), 8KB fixed to second-to-last
/// - 1KB/2KB switchable CHR-ROM (6 banks total)
/// - Scanline counter IRQ (A12 rising edge detection)
/// - Horizontal/vertical mirroring control
/// - Optional 8KB PRG-RAM with write protection
///
/// **Banking Modes (controlled by $8000 bit 6 and 7):**
/// - PRG mode 0: $8000-$9FFF swappable, $C000-$DFFF fixed to second-last
/// - PRG mode 1: $C000-$DFFF swappable, $8000-$9FFF fixed to second-last
/// - CHR mode 0: 2KB banks at $0000/$0800, 1KB banks at $1000-$1C00
/// - CHR mode 1: 2KB banks at $1000/$1800, 1KB banks at $0000-$0C00
///
/// **IRQ Operation:**
/// Counter decrements on A12 rising edge (sprite/BG fetch transition).
/// When counter reaches 0 with IRQs enabled, IRQ is asserted.
/// Write to $C000 sets reload value, $C001 triggers reload, $E000 disables, $E001 enables.
///
/// **Revisions:**
/// - MMC3A: Counter reload behavior differs (reload on $C001 or when counter=0)
/// - MMC3B/C: Standard reload behavior
/// - Sharp MMC3B: Same as MMC3B
/// - MMC6: Adds PRG-RAM enable bits per 1KB
/// </remarks>
class MMC3 : public BaseMapper {
private:
	uint8_t _currentRegister = 0;  ///< Bank register to write (0-7)

	bool _wramEnabled = false;        ///< PRG-RAM at $6000-$7FFF enabled
	bool _wramWriteProtected = false; ///< PRG-RAM write-protected

	uint64_t _a12LowClock = 0;  ///< Last cycle A12 was low (IRQ edge detection)

	bool _forceMmc3RevAIrqs = false;  ///< Use MMC3A IRQ behavior

	struct Mmc3State {
		uint8_t Reg8000;  ///< Control register (bank select, modes)
		uint8_t RegA000;  ///< Mirroring register
		uint8_t RegA001;  ///< PRG-RAM protect register
	} _state = {};

protected:
	uint8_t _irqReloadValue = 0;  ///< IRQ counter reload value ($C000)
	uint8_t _irqCounter = 0;      ///< Current IRQ counter
	bool _irqReload = false;      ///< Reload counter on next clock
	bool _irqEnabled = false;     ///< IRQ generation enabled
	uint8_t _prgMode = 0;         ///< PRG banking mode (0 or 1)
	uint8_t _chrMode = 0;         ///< CHR banking mode (0 or 1)
	uint8_t _registers[8] = {};   ///< Bank registers R0-R7

	uint8_t GetCurrentRegister() {
		return _currentRegister;
	}

	Mmc3State GetState() {
		return _state;
	}

	uint8_t GetChrMode() {
		return _chrMode;
	}

	/// Initialize MMC3 state with power-on values
	void ResetMmc3() {
		_state.Reg8000 = GetPowerOnByte();
		_state.RegA000 = GetPowerOnByte();
		_state.RegA001 = GetPowerOnByte();

		_chrMode = GetPowerOnByte() & 0x01;
		_prgMode = GetPowerOnByte() & 0x01;

		_currentRegister = GetPowerOnByte();

		_registers[0] = GetPowerOnByte(0);
		_registers[1] = GetPowerOnByte(2);
		_registers[2] = GetPowerOnByte(4);
		_registers[3] = GetPowerOnByte(5);
		_registers[4] = GetPowerOnByte(6);
		_registers[5] = GetPowerOnByte(7);
		_registers[6] = GetPowerOnByte(0);
		_registers[7] = GetPowerOnByte(1);

		_irqCounter = GetPowerOnByte();
		_irqReloadValue = GetPowerOnByte();
		_irqReload = GetPowerOnByte() & 0x01;
		_irqEnabled = GetPowerOnByte() & 0x01;

		_wramEnabled = GetPowerOnByte() & 0x01;
		_wramWriteProtected = GetPowerOnByte() & 0x01;
	}

	virtual bool ForceMmc3RevAIrqs() { return _forceMmc3RevAIrqs; }

	/// Update nametable mirroring based on $A000 register
	virtual void UpdateMirroring() {
		if (GetMirroringType() != MirroringType::FourScreens) {
			SetMirroringType(((_state.RegA000 & 0x01) == 0x01) ? MirroringType::Horizontal : MirroringType::Vertical);
		}
	}

	virtual void UpdateChrMapping() {
		if (_chrMode == 0) {
			SelectChrPage(0, _registers[0] & 0xFE);
			SelectChrPage(1, _registers[0] | 0x01);
			SelectChrPage(2, _registers[1] & 0xFE);
			SelectChrPage(3, _registers[1] | 0x01);

			SelectChrPage(4, _registers[2]);
			SelectChrPage(5, _registers[3]);
			SelectChrPage(6, _registers[4]);
			SelectChrPage(7, _registers[5]);
		} else if (_chrMode == 1) {
			SelectChrPage(0, _registers[2]);
			SelectChrPage(1, _registers[3]);
			SelectChrPage(2, _registers[4]);
			SelectChrPage(3, _registers[5]);

			SelectChrPage(4, _registers[0] & 0xFE);
			SelectChrPage(5, _registers[0] | 0x01);
			SelectChrPage(6, _registers[1] & 0xFE);
			SelectChrPage(7, _registers[1] | 0x01);
		}
	}

	virtual void UpdatePrgMapping() {
		if (_prgMode == 0) {
			SelectPrgPage(0, _registers[6]);
			SelectPrgPage(1, _registers[7]);
			SelectPrgPage(2, -2);
			SelectPrgPage(3, -1);
		} else if (_prgMode == 1) {
			SelectPrgPage(0, -2);
			SelectPrgPage(1, _registers[7]);
			SelectPrgPage(2, _registers[6]);
			SelectPrgPage(3, -1);
		}
	}

	bool CanWriteToWorkRam() {
		return _wramEnabled && !_wramWriteProtected;
	}

	virtual void UpdateState() {
		_currentRegister = _state.Reg8000 & 0x07;
		_chrMode = (_state.Reg8000 & 0x80) >> 7;
		_prgMode = (_state.Reg8000 & 0x40) >> 6;

		if (_romInfo.MapperID == 4 && _romInfo.SubMapperID == 1) {
			// MMC6
			bool wramEnabled = (_state.Reg8000 & 0x20) == 0x20;

			uint8_t firstBankAccess = (_state.RegA001 & 0x10 ? MemoryAccessType::Write : 0) | (_state.RegA001 & 0x20 ? MemoryAccessType::Read : 0);
			uint8_t lastBankAccess = (_state.RegA001 & 0x40 ? MemoryAccessType::Write : 0) | (_state.RegA001 & 0x80 ? MemoryAccessType::Read : 0);
			if (!wramEnabled) {
				firstBankAccess = MemoryAccessType::NoAccess;
				lastBankAccess = MemoryAccessType::NoAccess;
			}

			for (int i = 0; i < 4; i++) {
				SetCpuMemoryMapping(0x7000 + i * 0x400, 0x71FF + i * 0x400, 0, PrgMemoryType::SaveRam, firstBankAccess);
				SetCpuMemoryMapping(0x7200 + i * 0x400, 0x73FF + i * 0x400, 1, PrgMemoryType::SaveRam, lastBankAccess);
			}
		} else {
			_wramEnabled = (_state.RegA001 & 0x80) == 0x80;
			_wramWriteProtected = (_state.RegA001 & 0x40) == 0x40;

			if (_romInfo.SubMapperID == 0) {
				MemoryAccessType access;
				if (_wramEnabled) {
					access = CanWriteToWorkRam() ? MemoryAccessType::ReadWrite : MemoryAccessType::Read;
				} else {
					access = MemoryAccessType::NoAccess;
				}
				if ((HasBattery() && _saveRamSize > 0) || (!HasBattery() && _workRamSize > 0)) {
					SetCpuMemoryMapping(0x6000, 0x7FFF, 0, HasBattery() ? PrgMemoryType::SaveRam : PrgMemoryType::WorkRam, access);
				} else {
					RemoveCpuMemoryMapping(0x6000, 0x7FFF);
				}
			}
		}

		UpdatePrgMapping();
		UpdateChrMapping();
	}

	void Serialize(Serializer& s) override {
		BaseMapper::Serialize(s);
		SVArray(_registers, 8);
		SV(_a12LowClock);
		SV(_state.Reg8000);
		SV(_state.RegA000);
		SV(_state.RegA001);
		SV(_currentRegister);
		SV(_chrMode);
		SV(_prgMode);
		SV(_irqReloadValue);
		SV(_irqCounter);
		SV(_irqReload);
		SV(_irqEnabled);
		SV(_wramEnabled);
		SV(_wramWriteProtected);
	}

	uint16_t GetPrgPageSize() override { return 0x2000; }
	uint16_t GetChrPageSize() override { return 0x0400; }
	uint32_t GetSaveRamPageSize() override { return _romInfo.SubMapperID == 1 ? 0x200 : 0x2000; }
	uint32_t GetSaveRamSize() override { return _romInfo.SubMapperID == 1 ? 0x400 : 0x2000; }
	bool EnableVramAddressHook() override { return true; }

	void InitMapper() override {
		// Force MMC3A irqs for boards that are known to use the A revision.
		// Some MMC3B boards also have the A behavior, but currently no way to tell them apart.
		_forceMmc3RevAIrqs = _romInfo.DatabaseInfo.Chip.starts_with("MMC3A");

		ResetMmc3();
		SetCpuMemoryMapping(0x6000, 0x7FFF, 0, HasBattery() ? PrgMemoryType::SaveRam : PrgMemoryType::WorkRam);
		UpdateState();
		UpdateMirroring();
	}

	void WriteRegister(uint16_t addr, uint8_t value) override {
		switch (addr & 0xE001) {
			case 0x8000:
				_state.Reg8000 = value;
				UpdateState();
				break;

			case 0x8001:
				if (_currentRegister <= 1) {
					//"Writes to registers 0 and 1 always ignore bit 0"
					value &= ~0x01;
				}
				_registers[_currentRegister] = value;
				UpdateState();
				break;

			case 0xA000:
				_state.RegA000 = value;
				UpdateMirroring();
				break;

			case 0xA001:
				_state.RegA001 = value;
				UpdateState();
				break;

			case 0xC000:
				_irqReloadValue = value;
				break;

			case 0xC001:
				_irqCounter = 0;
				_irqReload = true;
				break;

			case 0xE000:
				_irqEnabled = false;
				_console->GetCpu()->ClearIrqSource(IRQSource::External);
				break;

			case 0xE001:
				_irqEnabled = true;
				break;
		}
	}

	virtual void TriggerIrq() {
		_console->GetCpu()->SetIrqSource(IRQSource::External);
	}

	vector<MapperStateEntry> GetMapperStateEntries() override {
		bool isMmc6 = _romInfo.MapperID == 4 && _romInfo.SubMapperID == 1;

		vector<MapperStateEntry> entries;
		entries.emplace_back("$8000.0-2", "Current Register", _state.Reg8000 & 0x07, MapperStateValueType::Number8);
		if (isMmc6) {
			entries.emplace_back("$8000.5", "Work RAM Enabled", (_state.Reg8000 & 0x20) != 0);
		}
		entries.emplace_back("$8000.6", "PRG Banking Mode", (_state.Reg8000 & 0x40) != 0);
		entries.emplace_back("$8000.7", "CHR Banking Mode", (_state.Reg8000 & 0x80) != 0);

		entries.emplace_back("$A000.0", "Mirroring", _state.RegA000 & 0x01 ? "Horizontal" : "Vertical");

		if (isMmc6) {
			entries.emplace_back("$A001.4", "Work RAM Bank 0 Write Enabled", (_state.RegA001 & 0x10) != 0);
			entries.emplace_back("$A001.5", "Work RAM Bank 0 Read Enabled", (_state.RegA001 & 0x20) != 0);
			entries.emplace_back("$A001.6", "Work RAM Bank 1 Write Enabled", (_state.RegA001 & 0x40) != 0);
			entries.emplace_back("$A001.7", "Work RAM Bank 1 Read Enabled", (_state.RegA001 & 0x80) != 0);
		} else {
			entries.emplace_back("$A001.6", "Work RAM Write Protected", (_state.RegA001 & 0x40) != 0);
			entries.emplace_back("$A001.7", "Work RAM Enabled", (_state.RegA001 & 0x80) != 0);
		}

		entries.emplace_back("$C000", "IRQ Reload Value", _irqReloadValue, MapperStateValueType::Number8);
		entries.emplace_back("", "IRQ Counter", _irqCounter, MapperStateValueType::Number8);
		entries.emplace_back("", "IRQ Reload Flag", _irqReload);
		entries.emplace_back("$E000/1", "IRQ Enabled", _irqEnabled);

		entries.emplace_back("", "Register 0 (CHR)", _registers[0], MapperStateValueType::Number8);
		entries.emplace_back("", "Register 1 (CHR)", _registers[1], MapperStateValueType::Number8);
		entries.emplace_back("", "Register 2 (CHR)", _registers[2], MapperStateValueType::Number8);
		entries.emplace_back("", "Register 3 (CHR)", _registers[3], MapperStateValueType::Number8);
		entries.emplace_back("", "Register 4 (CHR)", _registers[4], MapperStateValueType::Number8);
		entries.emplace_back("", "Register 5 (CHR)", _registers[5], MapperStateValueType::Number8);
		entries.emplace_back("", "Register 6 (PRG)", _registers[6], MapperStateValueType::Number8);
		entries.emplace_back("", "Register 7 (PRG)", _registers[7], MapperStateValueType::Number8);

		return entries;
	}

	bool IsA12RisingEdge(uint16_t addr) {
		if (addr & 0x1000) {
			bool isRisingEdge = _a12LowClock > 0 && (_console->GetMasterClock() - _a12LowClock) >= 3;
			_a12LowClock = 0;
			return isRisingEdge;
		} else if (_a12LowClock == 0) {
			_a12LowClock = _console->GetMasterClock();
		}
		return false;
	}

public:
	void NotifyVramAddressChange(uint16_t addr) override {
		if (IsA12RisingEdge(addr)) {
			uint32_t count = _irqCounter;
			if (_irqCounter == 0 || _irqReload) {
				_irqCounter = _irqReloadValue;
			} else {
				_irqCounter--;
			}

			if (ForceMmc3RevAIrqs()) {
				// MMC3 Revision A behavior
				if ((count > 0 || _irqReload) && _irqCounter == 0 && _irqEnabled) {
					TriggerIrq();
				}
			} else {
				if (_irqCounter == 0 && _irqEnabled) {
					TriggerIrq();
				}
			}
			_irqReload = false;
		}
	}
};
