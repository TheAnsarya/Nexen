#pragma once
#include "pch.h"
#include <cctype>

class Atari2600Mapper {
private:
	enum class MapperMode {
		None,
		Fixed2K,
		Fixed4K,
		F8,
		F6,
		F4,
		FE,
		E0,
		Mapper3F,
		RareFallback,
		Unknown
	};

	vector<uint8_t> _rom;
	string _romName;
	MapperMode _mode = MapperMode::None;
	uint8_t _activeBank = 0;
	uint8_t _segmentBanks[3] = {4, 5, 6};
	uint8_t _fixedSegmentBank = 7;
	size_t _fallbackBankSize = 0x1000;
	uint8_t _fallbackBankCount = 1;

	[[nodiscard]] uint8_t ClampBankCount(size_t bankCount) const {
		return (uint8_t)std::min<size_t>(255, std::max<size_t>(1, bankCount));
	}

	[[nodiscard]] bool HasHint(std::string_view token) const {
		return _romName.find(token) != string::npos;
	}

	[[nodiscard]] static uint8_t SelectBankFromValue(uint8_t value, uint8_t bankCount) {
		if (bankCount <= 1) {
			return 0;
		}

		if ((bankCount & (uint8_t)(bankCount - 1)) == 0) {
			return (uint8_t)(value & (bankCount - 1));
		}
		return (uint8_t)(value % bankCount);
	}

	void SelectMode() {
		if (_rom.empty()) {
			_mode = MapperMode::None;
			_activeBank = 0;
			_segmentBanks[0] = 4;
			_segmentBanks[1] = 5;
			_segmentBanks[2] = 6;
			_fixedSegmentBank = 7;
			_fallbackBankSize = 0x1000;
			_fallbackBankCount = 1;
			return;
		}

		if (HasHint("-f4") || HasHint("_f4") || HasHint("mapperf4")) {
			_mode = MapperMode::F4;
			_activeBank = 0;
			return;
		}

		if (HasHint("-fe") || HasHint("_fe") || HasHint("mapperfe")) {
			_mode = MapperMode::FE;
			_activeBank = 0;
			return;
		}

		if (HasHint("-e0") || HasHint("_e0") || HasHint("mappere0")) {
			_mode = MapperMode::E0;
			_segmentBanks[0] = 4;
			_segmentBanks[1] = 5;
			_segmentBanks[2] = 6;
			_fixedSegmentBank = 7;
			return;
		}

		if (HasHint("-3f") || HasHint("_3f") || HasHint("mapper3f") || HasHint("tigervision")) {
			_mode = MapperMode::Mapper3F;
			_activeBank = 0;
			_fallbackBankSize = 0x0800;
			_fallbackBankCount = ClampBankCount(_rom.size() / _fallbackBankSize);
			return;
		}

		if (HasHint("rare") || HasHint("homebrew") || HasHint("mapper-unknown") || HasHint("mapperunknown")) {
			_mode = MapperMode::RareFallback;
			_activeBank = 0;
			_fallbackBankSize = 0x1000;
			_fallbackBankCount = ClampBankCount(_rom.size() / _fallbackBankSize);
			return;
		}

		size_t size = _rom.size();
		if (size <= 2048) {
			_mode = MapperMode::Fixed2K;
			_activeBank = 0;
		} else if (size <= 4096) {
			_mode = MapperMode::Fixed4K;
			_activeBank = 0;
		} else if (size == 8192) {
			_mode = MapperMode::F8;
			_activeBank = 1;
		} else if (size == 16384) {
			_mode = MapperMode::F6;
			_activeBank = 0;
		} else if (size == 32768) {
			_mode = MapperMode::F4;
			_activeBank = 0;
		} else {
			_mode = MapperMode::RareFallback;
			_activeBank = 0;
			_fallbackBankSize = 0x1000;
			_fallbackBankCount = ClampBankCount(_rom.size() / _fallbackBankSize);
		}
	}

	void HandleBankswitch(uint16_t addr) {
		addr &= 0x1FFF;

		switch (_mode) {
			case MapperMode::F8:
				if (addr == 0x1FF8) {
					_activeBank = 0;
				} else if (addr == 0x1FF9) {
					_activeBank = 1;
				}
				break;

			case MapperMode::F6:
				if (addr >= 0x1FF6 && addr <= 0x1FF9) {
					_activeBank = (uint8_t)(addr - 0x1FF6);
				}
				break;

			case MapperMode::F4:
				if (addr >= 0x1FF4 && addr <= 0x1FFB) {
					_activeBank = (uint8_t)(addr - 0x1FF4);
				}
				break;

			case MapperMode::FE:
				if (addr == 0x1FFE) {
					_activeBank = 0;
				} else if (addr == 0x1FFF) {
					_activeBank = 1;
				}
				break;

			case MapperMode::E0:
				if (addr >= 0x1FE0 && addr <= 0x1FE7) {
					_segmentBanks[0] = (uint8_t)(addr & 0x07);
				} else if (addr >= 0x1FE8 && addr <= 0x1FEF) {
					_segmentBanks[1] = (uint8_t)(addr & 0x07);
				} else if (addr >= 0x1FF0 && addr <= 0x1FF7) {
					_segmentBanks[2] = (uint8_t)(addr & 0x07);
				}
				break;

			case MapperMode::Mapper3F:
			case MapperMode::RareFallback:
				break;

			default:
				break;
		}
	}

	[[nodiscard]] size_t GetOffset(uint16_t addr) const {
		if (_rom.empty() || addr < 0x1000) {
			return 0;
		}

		uint16_t cartAddr = (uint16_t)(addr - 0x1000);
		switch (_mode) {
			case MapperMode::Fixed2K:
				return cartAddr & 0x07FF;

			case MapperMode::Fixed4K:
				return cartAddr & 0x0FFF;

			case MapperMode::F8:
				return ((size_t)_activeBank * 0x1000) + (cartAddr & 0x0FFF);

			case MapperMode::F6:
				return ((size_t)_activeBank * 0x1000) + (cartAddr & 0x0FFF);

			case MapperMode::F4:
				return ((size_t)_activeBank * 0x1000) + (cartAddr & 0x0FFF);

			case MapperMode::FE:
				return ((size_t)_activeBank * 0x1000) + (cartAddr & 0x0FFF);

			case MapperMode::E0:
				if (cartAddr < 0x0400) {
					return ((size_t)_segmentBanks[0] * 0x0400) + (cartAddr & 0x03FF);
				} else if (cartAddr < 0x0800) {
					return ((size_t)_segmentBanks[1] * 0x0400) + (cartAddr & 0x03FF);
				} else if (cartAddr < 0x0C00) {
					return ((size_t)_segmentBanks[2] * 0x0400) + (cartAddr & 0x03FF);
				}
				return ((size_t)_fixedSegmentBank * 0x0400) + (cartAddr & 0x03FF);

			case MapperMode::Mapper3F: {
				size_t bankCount = std::max<size_t>(1, _fallbackBankCount);
				size_t fixedBank = bankCount - 1;
				if (cartAddr < 0x0800) {
					return ((size_t)(_activeBank % bankCount) * 0x0800) + (cartAddr & 0x07FF);
				}
				return (fixedBank * 0x0800) + (cartAddr & 0x07FF);
			}

			case MapperMode::RareFallback:
				return ((size_t)_activeBank * _fallbackBankSize) + (cartAddr & (uint16_t)(_fallbackBankSize - 1));

			case MapperMode::Unknown:
				return cartAddr % _rom.size();

			case MapperMode::None:
			default:
				return 0;
		}
	}

public:
	void LoadRom(const vector<uint8_t>& romData, string romName) {
		_rom = romData;
		_romName = std::move(romName);
		std::transform(_romName.begin(), _romName.end(), _romName.begin(), [](unsigned char c) {
			return (char)std::tolower(c);
		});
		SelectMode();
	}

	uint8_t Read(uint16_t addr) {
		if (_rom.empty() || addr < 0x1000) {
			return 0xFF;
		}

		HandleBankswitch(addr);
		size_t offset = GetOffset(addr);
		if (offset >= _rom.size()) {
			offset %= _rom.size();
		}
		return _rom[offset];
	}

	[[nodiscard]] bool TryGetRomOffset(uint16_t addr, int32_t& offset) const {
		addr &= 0x1FFF;
		if (_rom.empty() || addr < 0x1000) {
			offset = -1;
			return false;
		}

		size_t romOffset = GetOffset(addr);
		if (romOffset >= _rom.size()) {
			romOffset %= _rom.size();
		}

		offset = (int32_t)romOffset;
		return true;
	}

	void Write(uint16_t addr, uint8_t value) {
		if (_mode == MapperMode::Mapper3F && (addr & 0x003F) == 0x003F && _fallbackBankCount > 0) {
			_activeBank = SelectBankFromValue(value, _fallbackBankCount);
			return;
		}

		if (_mode == MapperMode::RareFallback && addr >= 0x1000 && addr <= 0x1FFF && _fallbackBankCount > 0) {
			_activeBank = SelectBankFromValue(value, _fallbackBankCount);
			return;
		}

		(void)value;
		HandleBankswitch(addr);
	}

	[[nodiscard]] uint8_t GetActiveBank() const {
		return _activeBank;
	}

	void ExportState(uint8_t& activeBank, uint8_t& segmentBank0, uint8_t& segmentBank1, uint8_t& segmentBank2, uint8_t& fixedSegmentBank) const {
		activeBank = _activeBank;
		segmentBank0 = _segmentBanks[0];
		segmentBank1 = _segmentBanks[1];
		segmentBank2 = _segmentBanks[2];
		fixedSegmentBank = _fixedSegmentBank;
	}

	void ImportState(uint8_t activeBank, uint8_t segmentBank0, uint8_t segmentBank1, uint8_t segmentBank2, uint8_t fixedSegmentBank) {
		_activeBank = activeBank;
		_segmentBanks[0] = segmentBank0;
		_segmentBanks[1] = segmentBank1;
		_segmentBanks[2] = segmentBank2;
		_fixedSegmentBank = fixedSegmentBank;
	}

	[[nodiscard]] string GetModeName() const {
		switch (_mode) {
			case MapperMode::Fixed2K: return "2k";
			case MapperMode::Fixed4K: return "4k";
			case MapperMode::F8: return "f8";
			case MapperMode::F6: return "f6";
			case MapperMode::F4: return "f4";
			case MapperMode::FE: return "fe";
			case MapperMode::E0: return "e0";
			case MapperMode::Mapper3F: return "3f";
			case MapperMode::RareFallback: return "fallback";
			case MapperMode::Unknown: return "unknown";
			default: return "none";
		}
	}

	[[nodiscard]] uint8_t* GetRomData() {
		return _rom.empty() ? nullptr : _rom.data();
	}

	[[nodiscard]] uint32_t GetRomSize() const {
		return (uint32_t)_rom.size();
	}
};
