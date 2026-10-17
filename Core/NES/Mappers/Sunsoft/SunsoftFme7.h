#pragma once
#include "pch.h"
#include "NES/BaseMapper.h"
#include "NES/NesCpu.h"
#include "NES/Mappers/Audio/Sunsoft5bAudio.h"

/// <summary>
/// Sunsoft FME-7/5A/5B mapper (iNES mapper 69).
/// Used by games like Batman: Return of the Joker, Gimmick!, Hebereke.
/// </summary>
/// <remarks>
/// Features:
/// - 8KB PRG banking (4 switchable banks)
/// - 1KB CHR banking (8 switchable banks)
/// - PRG-RAM at $6000-$7FFF (optional battery backup)
/// - Scanline counter IRQ
/// - Sunsoft 5B expansion audio (YM2149-compatible, 3 square wave channels)
///
/// Banking is command-based: write command to $8000, then data to $A000.
/// Commands 0-7: CHR banks, 8: PRG-RAM/ROM at $6000, 9-B: PRG banks, C: mirroring.
/// </remarks>
class SunsoftFme7 : public BaseMapper {
private:
	unique_ptr<Sunsoft5bAudio> _audio;  ///< Expansion audio (YM2149-style)
	uint8_t _command = 0;               ///< Current command register (0-15)
	uint8_t _workRamValue = 0;          ///< $6000-$7FFF configuration
	bool _irqEnabled = false;           ///< IRQ counter enabled
	bool _irqCounterEnabled = false;    ///< Counter decrement enabled
	uint16_t _irqCounter = 0;           ///< 16-bit IRQ counter

protected:
	uint16_t GetPrgPageSize() override { return 0x2000; }  ///< 8KB PRG pages
	uint16_t GetChrPageSize() override { return 0x400; }   ///< 1KB CHR pages
	uint32_t GetWorkRamSize() override { return 0x8000; }
	uint32_t GetWorkRamPageSize() override { return 0x2000; }
	uint32_t GetSaveRamSize() override { return 0x8000; }
	uint32_t GetSaveRamPageSize() override { return 0x2000; }
	bool EnableCpuClockHook() override { return true; }  ///< Need clock for IRQ counter

	void InitMapper() override {
		_audio = std::make_unique<Sunsoft5bAudio>(_console);

		_command = 0;
		_workRamValue = 0;
		_irqEnabled = false;
		_irqCounterEnabled = false;
		_irqCounter = 0;

		SelectPrgPage(3, -1);

		UpdateWorkRam();
	}

	void Serialize(Serializer& s) override {
		BaseMapper::Serialize(s);
		SV(_audio);
		SV(_command);
		SV(_workRamValue);
		SV(_irqEnabled);
		SV(_irqCounterEnabled);
		SV(_irqCounter);
		if (!s.IsSaving()) {
			UpdateWorkRam();
		}
	}

	void ProcessCpuClock() override {
		BaseProcessCpuClock();

		if (_irqCounterEnabled) {
			_irqCounter--;
			if (_irqCounter == 0xFFFF) {
				if (_irqEnabled) {
					_console->GetCpu()->SetIrqSource(IRQSource::External);
				}
			}
		}

		_audio->Clock();
	}

	void UpdateWorkRam() {
		if (_workRamValue & 0x40) {
			MemoryAccessType accessType = (_workRamValue & 0x80) ? MemoryAccessType::ReadWrite : MemoryAccessType::NoAccess;
			SetCpuMemoryMapping(0x6000, 0x7FFF, _workRamValue & 0x3F, HasBattery() ? PrgMemoryType::SaveRam : PrgMemoryType::WorkRam, accessType);
		} else {
			SetCpuMemoryMapping(0x6000, 0x7FFF, _workRamValue & 0x3F, PrgMemoryType::PrgRom);
		}
	}

	void WriteRegister(uint16_t addr, uint8_t value) override {
		switch (addr & 0xE000) {
			case 0x8000:
				_command = value & 0x0F;
				break;
			case 0xA000:
				switch (_command) {
					case 0:
					case 1:
					case 2:
					case 3:
					case 4:
					case 5:
					case 6:
					case 7:
						SelectChrPage(_command, value);
						break;

					case 8: {
						_workRamValue = value;
						UpdateWorkRam();
						break;
					}

					case 9:
					case 0xA:
					case 0xB:
						SelectPrgPage(_command - 9, value & 0x3F);
						break;

					case 0xC:
						switch (value & 0x03) {
							case 0:
								SetMirroringType(MirroringType::Vertical);
								break;
							case 1:
								SetMirroringType(MirroringType::Horizontal);
								break;
							case 2:
								SetMirroringType(MirroringType::ScreenAOnly);
								break;
							case 3:
								SetMirroringType(MirroringType::ScreenBOnly);
								break;
						}
						break;

					case 0xD:
						_irqEnabled = (value & 0x01) == 0x01;
						_irqCounterEnabled = (value & 0x80) == 0x80;
						_console->GetCpu()->ClearIrqSource(IRQSource::External);
						break;

					case 0xE:
						_irqCounter = (_irqCounter & 0xFF00) | value;
						break;

					case 0xF:
						_irqCounter = (_irqCounter & 0xFF) | (value << 8);
						break;
				}
				break;

			case 0xC000:
			case 0xE000:
				_audio->WriteRegister(addr, value);
				break;
		}
	}
};