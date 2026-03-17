#include "pch.h"
#include "Atari2600/Atari2600Console.h"
#include "Atari2600/Atari2600DefaultVideoFilter.h"
#include "Shared/BaseControlManager.h"
#include "Shared/CpuType.h"
#include "Shared/MemoryType.h"
#include "Utilities/VirtualFile.h"
#include "Utilities/Serializer.h"
#include <functional>
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
				// Write-driven modes; read-side hotspots are intentionally ignored.
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
};

class Atari2600Riot {
	private:
		Atari2600RiotState _state = {};

		void ConfigureTimer(uint8_t value, uint16_t divider) {
			_state.Timer = value;
			_state.TimerDivider = std::max<uint16_t>(1, divider);
			_state.TimerDividerCounter = _state.TimerDivider;
			_state.TimerUnderflow = false;
			_state.InterruptFlag = false;
		}

		[[nodiscard]] uint8_t ReadPortValue(uint8_t outputLatch, uint8_t inputLatch, uint8_t directionMask) const {
			uint8_t outputBits = (uint8_t)(outputLatch & directionMask);
			uint8_t inputBits = (uint8_t)(inputLatch & (uint8_t)~directionMask);
			return (uint8_t)(outputBits | inputBits);
		}

	public:
		void Reset() {
			_state = {};
			_state.TimerDivider = 1;
			_state.TimerDividerCounter = 1;
		}

		void StepCpuCycles(uint32_t cycles) {
			for (uint32_t i = 0; i < cycles; i++) {
				_state.CpuCycles++;

				if (_state.TimerDividerCounter > 0) {
					_state.TimerDividerCounter--;
				}

				if (_state.TimerDividerCounter == 0) {
					_state.TimerDividerCounter = _state.TimerDivider;

					if (_state.Timer == 0) {
						_state.Timer = 0x00FF;
						_state.TimerUnderflow = true;
						_state.InterruptFlag = true;
						_state.InterruptEdgeCount++;
					} else {
						_state.Timer--;
					}
				}
			}
		}

		uint8_t ReadRegister(uint16_t addr) const {
			switch (addr & 0x07) {
				case 0x00: return ReadPortValue(_state.PortA, _state.PortAInput, _state.PortADirection);
				case 0x01: return ReadPortValue(_state.PortB, _state.PortBInput, _state.PortBDirection);
				case 0x02: return _state.PortADirection;
				case 0x03: return _state.PortBDirection;
				case 0x04: return (uint8_t)(_state.Timer & 0xFF);
				case 0x05: return (uint8_t)((_state.Timer >> 8) & 0xFF);
				case 0x06: return _state.InterruptFlag ? 1 : 0;
				case 0x07: return _state.TimerUnderflow ? 1 : 0;
				default: return 0;
			}
		}

		void WriteRegister(uint16_t addr, uint8_t value) {
			switch (addr & 0x07) {
				case 0x00:
					_state.PortA = value;
					break;
				case 0x01:
					_state.PortB = value;
					break;
				case 0x02:
					_state.PortADirection = value;
					break;
				case 0x03:
					_state.PortBDirection = value;
					break;
				case 0x04:
					ConfigureTimer(value, 1);
					break;
				case 0x05:
					ConfigureTimer(value, 8);
					break;
				case 0x06:
					ConfigureTimer(value, 64);
					break;
				case 0x07:
					ConfigureTimer(value, 1024);
					break;
			}
		}

		Atari2600RiotState GetState() const {
			return _state;
		}
	};

	class Atari2600Tia {
	private:
		Atari2600TiaState _state = {};
		static constexpr uint32_t HmoveBlankColorClocks = 8;

		[[nodiscard]] static uint16_t NormalizeRegisterAddress(uint16_t addr) {
			return addr & 0x3F;
		}

		void MarkRenderDirty() {
			_state.RenderRevision++;
		}

		void MarkAudioDirty() {
			_state.AudioRevision++;
		}

		[[nodiscard]] static uint8_t ComputeChannelSample(uint8_t control, uint8_t phase, uint8_t volume) {
			if (volume == 0) {
				return 0;
			}

			uint8_t mode = (uint8_t)(control & 0x03);
			switch (mode) {
				case 0:
					return volume;
				case 1:
					return (phase & 0x01) ? volume : 0;
				case 2:
					return (phase & 0x03) < 2 ? volume : 0;
				default:
					return (phase & 0x07) == 0 ? volume : 0;
			}
		}

		void StepAudio() {
			auto advanceChannel = [](uint16_t& counter, uint8_t frequency, uint8_t& phase, uint8_t control) {
				if (counter > 0) {
					counter--;
				}
				if (counter == 0) {
					counter = (uint16_t)(frequency + 1);
					phase = (uint8_t)((phase + 1 + (control & 0x01)) & 0x0F);
				}
			};

			advanceChannel(_state.AudioCounter0, _state.AudioFrequency0, _state.AudioPhase0, _state.AudioControl0);
			advanceChannel(_state.AudioCounter1, _state.AudioFrequency1, _state.AudioPhase1, _state.AudioControl1);

			uint8_t sample0 = ComputeChannelSample(_state.AudioControl0, _state.AudioPhase0, _state.AudioVolume0);
			uint8_t sample1 = ComputeChannelSample(_state.AudioControl1, _state.AudioPhase1, _state.AudioVolume1);
			_state.LastMixedSample = (uint8_t)std::min<uint16_t>(31, (uint16_t)sample0 + sample1);
			_state.AudioMixAccumulator += _state.LastMixedSample;
			_state.AudioSampleCount++;
		}

		void AdvanceScanline() {
			_state.ColorClock = 0;
			_state.Scanline++;
			if (_state.Scanline >= 262) {
				_state.Scanline = 0;
				_state.FrameCount++;
			}
		}

		void StepColorClocks(uint32_t colorClocks) {
			for (uint32_t i = 0; i < colorClocks; i++) {
				_state.TotalColorClocks++;
				_state.ColorClock++;
				if (_state.ColorClock >= 228) {
					AdvanceScanline();
				}
			}
		}

	public:
		void Reset() {
			_state = {};
			_state.Player0X = 24;
			_state.Player1X = 96;
			_state.BallX = 80;
			_state.AudioCounter0 = 1;
			_state.AudioCounter1 = 1;
		}

		void StepCpuCycles(uint32_t cpuCycles) {
			for (uint32_t i = 0; i < cpuCycles; i++) {
				if (_state.WsyncHold) {
					_state.WsyncHold = false;
					AdvanceScanline();
				}
				StepColorClocks(3);
				StepAudio();
				if (_state.HmovePending) {
					_state.HmovePending = false;
					_state.HmoveApplyCount++;
					StepColorClocks(HmoveBlankColorClocks);
				}
			}
		}

		void RequestWsync() {
			_state.WsyncHold = true;
			_state.WsyncCount++;
		}

		void RequestHmove() {
			_state.HmovePending = true;
			_state.HmoveStrobeCount++;
		}

		uint8_t ReadRegister(uint16_t addr) const {
			switch (NormalizeRegisterAddress(addr)) {
				case 0x06: return _state.ColorPlayer0;
				case 0x07: return _state.ColorPlayer1;
				case 0x08: return _state.ColorPlayfield;
				case 0x09: return _state.ColorBackground;
				case 0x0A: return _state.PlayfieldReflect ? 0x01 : 0x00;
				case 0x0D: return _state.Playfield0;
				case 0x0E: return _state.Playfield1;
				case 0x0F: return _state.Playfield2;
				case 0x1B: return _state.Player0Graphics;
				case 0x1C: return _state.Player1Graphics;
				case 0x1D: return _state.Missile0Enabled ? 0x02 : 0x00;
				case 0x1E: return _state.Missile1Enabled ? 0x02 : 0x00;
				case 0x1F: return _state.BallEnabled ? 0x02 : 0x00;
				case 0x15: return _state.AudioControl0;
				case 0x16: return _state.AudioControl1;
				case 0x17: return _state.AudioFrequency0;
				case 0x18: return _state.AudioFrequency1;
				case 0x19: return _state.AudioVolume0;
				case 0x1A: return _state.AudioVolume1;
				default: return 0;
			}
		}

		void WriteRegister(uint16_t addr, uint8_t value) {
			switch (NormalizeRegisterAddress(addr)) {
				case 0x02:
					RequestWsync();
					break;

				case 0x2A:
					RequestHmove();
					break;

				case 0x06:
					_state.ColorPlayer0 = value;
					MarkRenderDirty();
					break;

				case 0x07:
					_state.ColorPlayer1 = value;
					MarkRenderDirty();
					break;

				case 0x08:
					_state.ColorPlayfield = value;
					MarkRenderDirty();
					break;

				case 0x09:
					_state.ColorBackground = value;
					MarkRenderDirty();
					break;

				case 0x0A:
					_state.PlayfieldReflect = (value & 0x01) != 0;
					MarkRenderDirty();
					break;

				case 0x0D:
					_state.Playfield0 = value;
					MarkRenderDirty();
					break;

				case 0x0E:
					_state.Playfield1 = value;
					MarkRenderDirty();
					break;

				case 0x0F:
					_state.Playfield2 = value;
					MarkRenderDirty();
					break;

				case 0x10:
					_state.Player0X = (uint8_t)(_state.ColorClock % Atari2600Console::ScreenWidth);
					MarkRenderDirty();
					break;

				case 0x11:
					_state.Player1X = (uint8_t)(_state.ColorClock % Atari2600Console::ScreenWidth);
					MarkRenderDirty();
					break;

				case 0x14:
					_state.BallX = (uint8_t)(_state.ColorClock % Atari2600Console::ScreenWidth);
					MarkRenderDirty();
					break;

				case 0x15:
					_state.AudioControl0 = value;
					MarkAudioDirty();
					break;

				case 0x16:
					_state.AudioControl1 = value;
					MarkAudioDirty();
					break;

				case 0x17:
					_state.AudioFrequency0 = (uint8_t)(value & 0x1F);
					_state.AudioCounter0 = (uint16_t)(_state.AudioFrequency0 + 1);
					MarkAudioDirty();
					break;

				case 0x18:
					_state.AudioFrequency1 = (uint8_t)(value & 0x1F);
					_state.AudioCounter1 = (uint16_t)(_state.AudioFrequency1 + 1);
					MarkAudioDirty();
					break;

				case 0x19:
					_state.AudioVolume0 = (uint8_t)(value & 0x0F);
					MarkAudioDirty();
					break;

				case 0x1A:
					_state.AudioVolume1 = (uint8_t)(value & 0x0F);
					MarkAudioDirty();
					break;

				case 0x1B:
					_state.Player0Graphics = value;
					MarkRenderDirty();
					break;

				case 0x1C:
					_state.Player1Graphics = value;
					MarkRenderDirty();
					break;

				case 0x1D:
					_state.Missile0Enabled = (value & 0x02) != 0;
					MarkRenderDirty();
					break;

				case 0x1E:
					_state.Missile1Enabled = (value & 0x02) != 0;
					MarkRenderDirty();
					break;

				case 0x1F:
					_state.BallEnabled = (value & 0x02) != 0;
					MarkRenderDirty();
					break;

				default:
					break;
			}
		}

		Atari2600TiaState GetState() const {
			return _state;
		}

		void ResetAudioHistory() {
			_state.AudioCounter0 = (uint16_t)(_state.AudioFrequency0 + 1);
			_state.AudioCounter1 = (uint16_t)(_state.AudioFrequency1 + 1);
			_state.AudioPhase0 = 0;
			_state.AudioPhase1 = 0;
			_state.LastMixedSample = 0;
			_state.AudioMixAccumulator = 0;
			_state.AudioSampleCount = 0;
		}
	};

	class Atari2600Bus {
	private:
		Atari2600Riot* _riot = nullptr;
		Atari2600Tia* _tia = nullptr;
		Atari2600Mapper* _mapper = nullptr;

	public:
		void Attach(Atari2600Riot* riot, Atari2600Tia* tia, Atari2600Mapper* mapper) {
			_riot = riot;
			_tia = tia;
			_mapper = mapper;
		}

		uint8_t Read(uint16_t addr) const {
			addr &= 0x1FFF;
			if ((addr & 0x1000) == 0x1000 && _mapper) {
				return _mapper->Read(addr);
			}
			if ((addr & 0x1080) == 0x0080 && _riot) {
				return _riot->ReadRegister(addr);
			}
			if ((addr & 0x1080) == 0x0000 && _tia) {
				return _tia->ReadRegister(addr);
			}
			return 0;
		}

		void Write(uint16_t addr, uint8_t value) {
			addr &= 0x1FFF;
			if (_mapper) {
				_mapper->Write(addr, value);
			}

			if ((addr & 0x1080) == 0x0080 && _riot) {
				_riot->WriteRegister(addr, value);
				return;
			}
			if ((addr & 0x1080) == 0x0000 && _tia) {
				_tia->WriteRegister(addr, value);
			}
		}
	};

	class Atari2600CpuAdapter {
	private:
		std::function<uint8_t(uint16_t)> _read;
		std::function<void(uint16_t, uint8_t)> _write;
		uint16_t _pc = 0x1000;
		uint64_t _cycleCount = 0;
		uint8_t _a = 0;
		uint8_t _x = 0;
		uint8_t _y = 0;
		uint8_t _sp = 0xFD;
		uint8_t _status = 0x24;
		uint8_t _instructionCyclesRemaining = 0;

		static constexpr uint8_t FlagCarry = 0x01;
		static constexpr uint8_t FlagZero = 0x02;
		static constexpr uint8_t FlagInterruptDisable = 0x04;
		static constexpr uint8_t FlagDecimal = 0x08;
		static constexpr uint8_t FlagBreak = 0x10;
		static constexpr uint8_t FlagUnused = 0x20;
		static constexpr uint8_t FlagOverflow = 0x40;
		static constexpr uint8_t FlagNegative = 0x80;

		[[nodiscard]] uint8_t Read(uint16_t addr) const {
			return _read ? _read(addr & 0x1FFF) : 0xFF;
		}

		void Write(uint16_t addr, uint8_t value) const {
			if (_write) {
				_write(addr & 0x1FFF, value);
			}
		}

		[[nodiscard]] uint8_t FetchByte() {
			uint8_t value = Read(_pc);
			_pc = (_pc + 1) & 0x1FFF;
			return value;
		}

		[[nodiscard]] uint16_t FetchWord() {
			uint16_t low = FetchByte();
			uint16_t high = FetchByte();
			return (uint16_t)((high << 8) | low);
		}

		void SetFlag(uint8_t flag, bool enabled) {
			if (enabled) {
				_status |= flag;
			} else {
				_status &= (uint8_t)~flag;
			}
		}

		[[nodiscard]] bool GetFlag(uint8_t flag) const {
			return (_status & flag) != 0;
		}

		void UpdateZeroNegative(uint8_t value) {
			SetFlag(FlagZero, value == 0);
			SetFlag(FlagNegative, (value & 0x80) != 0);
		}

		[[nodiscard]] uint8_t AddWithCarry(uint8_t left, uint8_t right, bool subtract) {
			uint16_t operand = subtract ? (uint16_t)(right ^ 0xFF) : right;
			uint16_t carryIn = GetFlag(FlagCarry) ? 1 : 0;
			uint16_t sum = (uint16_t)left + operand + carryIn;
			uint8_t result = (uint8_t)sum;

			SetFlag(FlagCarry, sum > 0xFF);
			SetFlag(FlagOverflow, ((~(left ^ operand) & (left ^ result)) & 0x80) != 0);
			UpdateZeroNegative(result);
			return result;
		}

		[[nodiscard]] uint8_t Compare(uint8_t left, uint8_t right) {
			uint16_t diff = (uint16_t)left - right;
			SetFlag(FlagCarry, left >= right);
			SetFlag(FlagZero, left == right);
			SetFlag(FlagNegative, (diff & 0x80) != 0);
			return 2;
		}

		[[nodiscard]] uint8_t BranchIf(bool condition) {
			int8_t offset = (int8_t)FetchByte();
			if (!condition) {
				return 2;
			}

			uint16_t sourcePc = _pc;
			uint16_t targetPc = (uint16_t)(_pc + offset);
			_pc = targetPc & 0x1FFF;

			uint8_t cycles = 3;
			if (((sourcePc ^ targetPc) & 0xFF00) != 0) {
				cycles++;
			}
			return cycles;
		}

		[[nodiscard]] uint8_t ExecuteInstruction() {
			uint8_t opcode = FetchByte();

			switch (opcode) {
				case 0x00:
				case 0xEA:
					return 2;

				case 0xA9:
					_a = FetchByte();
					UpdateZeroNegative(_a);
					return 2;

				case 0xA2:
					_x = FetchByte();
					UpdateZeroNegative(_x);
					return 2;

				case 0xA0:
					_y = FetchByte();
					UpdateZeroNegative(_y);
					return 2;

				case 0x8D: {
					uint16_t addr = FetchWord();
					Write(addr, _a);
					return 4;
				}

				case 0x8E: {
					uint16_t addr = FetchWord();
					Write(addr, _x);
					return 4;
				}

				case 0x8C: {
					uint16_t addr = FetchWord();
					Write(addr, _y);
					return 4;
				}

				case 0xAA:
					_x = _a;
					UpdateZeroNegative(_x);
					return 2;

				case 0xE8:
					_x++;
					UpdateZeroNegative(_x);
					return 2;

				case 0xCA:
					_x--;
					UpdateZeroNegative(_x);
					return 2;

				case 0xC8:
					_y++;
					UpdateZeroNegative(_y);
					return 2;

				case 0x88:
					_y--;
					UpdateZeroNegative(_y);
					return 2;

				case 0x18:
					SetFlag(FlagCarry, false);
					return 2;

				case 0x38:
					SetFlag(FlagCarry, true);
					return 2;

				case 0x69:
					_a = AddWithCarry(_a, FetchByte(), false);
					return 2;

				case 0xE9:
					_a = AddWithCarry(_a, FetchByte(), true);
					return 2;

				case 0xC9:
					return Compare(_a, FetchByte());

				case 0x4C:
					_pc = FetchWord() & 0x1FFF;
					return 3;

				case 0xF0:
					return BranchIf(GetFlag(FlagZero));

				case 0xD0:
					return BranchIf(!GetFlag(FlagZero));

				case 0x10:
					return BranchIf(!GetFlag(FlagNegative));

				case 0x30:
					return BranchIf(GetFlag(FlagNegative));

				default:
					return 2;
			}
		}

	public:
		void Reset() {
			_pc = 0x1000;
			_cycleCount = 0;
			_a = 0;
			_x = 0;
			_y = 0;
			_sp = 0xFD;
			_status = FlagUnused | FlagInterruptDisable;
			_instructionCyclesRemaining = 0;
		}

		void SetReadCallback(std::function<uint8_t(uint16_t)> cb) {
			_read = std::move(cb);
		}

		void SetWriteCallback(std::function<void(uint16_t, uint8_t)> cb) {
			_write = std::move(cb);
		}

		void StepCycles(uint32_t cycles) {
			for (uint32_t i = 0; i < cycles; i++) {
				if (_instructionCyclesRemaining == 0) {
					_instructionCyclesRemaining = ExecuteInstruction();
					if (_instructionCyclesRemaining == 0) {
						_instructionCyclesRemaining = 1;
					}
				}

				_instructionCyclesRemaining--;
				_cycleCount++;
			}
		}

		uint64_t GetCycleCount() const {
			return _cycleCount;
		}

		uint16_t GetProgramCounter() const {
			return _pc;
		}
	};

	class Atari2600ControlManager final : public BaseControlManager {
	public:
		explicit Atari2600ControlManager(Emulator* emu)
			: BaseControlManager(emu, CpuType::Nes) {
		}

		shared_ptr<BaseControlDevice> CreateControllerDevice(ControllerType type, uint8_t port) override {
			(void)type;
			(void)port;
			return nullptr;
		}

		void UpdateInputState() override {
			SetInputReadFlag();
		}
	};

Atari2600Console::Atari2600Console(Emulator* emu)
	: _emu(emu),
	  _cpu(std::make_unique<Atari2600CpuAdapter>()),
	  _riot(std::make_unique<Atari2600Riot>()),
	  _tia(std::make_unique<Atari2600Tia>()),
	  _mapper(std::make_unique<Atari2600Mapper>()),
	  _bus(std::make_unique<Atari2600Bus>()),
	  _frameBuffer(ScreenWidth * ScreenHeight, 0) {
	_controlManager = std::make_unique<Atari2600ControlManager>(emu);
	_bus->Attach(_riot.get(), _tia.get(), _mapper.get());
	_cpu->SetReadCallback([this](uint16_t addr) {
		return _bus->Read(addr);
	});
	_cpu->SetWriteCallback([this](uint16_t addr, uint8_t value) {
		_bus->Write(addr, value);
	});
}

Atari2600Console::~Atari2600Console() {
}

LoadRomResult Atari2600Console::LoadRom(VirtualFile& romFile) {
	vector<uint8_t> romData;
	if (!romFile.ReadFile(romData) || romData.size() < 1024) {
		return LoadRomResult::Failure;
	}

	_mapper->LoadRom(romData, romFile.GetFileName());
	_romLoaded = true;
	Reset();
	return LoadRomResult::Success;
}

void Atari2600Console::Reset() {
	_cpu->Reset();
	_riot->Reset();
	_tia->Reset();
	_lastFrameSummary = {};
	std::fill(_frameBuffer.begin(), _frameBuffer.end(), 0);
}

void Atari2600Console::StepCpuCycles(uint32_t cpuCycles) {
	_cpu->StepCycles(cpuCycles);
	_riot->StepCpuCycles(cpuCycles);
	_tia->StepCpuCycles(cpuCycles);
}

void Atari2600Console::RunFrame() {
	if (!_romLoaded) {
		return;
	}

	uint64_t startCycles = _cpu->GetCycleCount();
	StepCpuCycles(CpuCyclesPerFrame);
	Atari2600TiaState tiaState = _tia->GetState();
	_lastFrameSummary.FrameCount = tiaState.FrameCount;
	_lastFrameSummary.CpuCyclesThisFrame = (uint32_t)(_cpu->GetCycleCount() - startCycles);
	_lastFrameSummary.ScanlineAtFrameEnd = tiaState.Scanline;
	_lastFrameSummary.ColorClockAtFrameEnd = tiaState.ColorClock;
	RenderDebugFrame();
	if (_controlManager) {
		_controlManager->UpdateControlDevices();
		_controlManager->UpdateInputState();
		_controlManager->ProcessEndOfFrame();
	}
}

void Atari2600Console::RequestWsync() {
	_tia->RequestWsync();
}

void Atari2600Console::RequestHmove() {
	_tia->RequestHmove();
}

Atari2600RiotState Atari2600Console::GetRiotState() const {
	return _riot->GetState();
}

Atari2600TiaState Atari2600Console::GetTiaState() const {
	return _tia->GetState();
}

uint8_t Atari2600Console::DebugReadCartridge(uint16_t addr) const {
	if (!_bus) {
		return 0xFF;
	}
	return _bus->Read(addr);
}

void Atari2600Console::DebugWriteCartridge(uint16_t addr, uint8_t value) {
	if (_bus) {
		_bus->Write(addr, value);
	}
}

uint8_t Atari2600Console::DebugGetMapperBankIndex() const {
	return _mapper ? _mapper->GetActiveBank() : 0;
}

string Atari2600Console::DebugGetMapperMode() const {
	return _mapper ? _mapper->GetModeName() : "none";
}

void Atari2600Console::RenderDebugFrame() {
	Atari2600TiaState tiaState = _tia->GetState();
	auto toRgb565 = [](uint8_t tiaColor) {
		uint8_t hue = (uint8_t)((tiaColor >> 4) & 0x0F);
		uint8_t lum = (uint8_t)(tiaColor & 0x0F);
		uint8_t red = (uint8_t)((hue * 3 + lum) & 0x1F);
		uint8_t green = (uint8_t)((hue * 5 + lum * 2) & 0x3F);
		uint8_t blue = (uint8_t)((hue * 7 + lum) & 0x1F);
		return (uint16_t)((red << 11) | (green << 5) | blue);
	};

	auto getPlayfieldBit = [&tiaState](uint32_t index) {
		index %= 20;
		if (index < 4) {
			return ((tiaState.Playfield0 >> (4 + index)) & 0x01) != 0;
		}
		if (index < 12) {
			return ((tiaState.Playfield1 >> (11 - index)) & 0x01) != 0;
		}
		return ((tiaState.Playfield2 >> (19 - index)) & 0x01) != 0;
	};

	auto isPlayerPixel = [](uint8_t graphics, uint32_t x, uint32_t originX) {
		if (x < originX || x >= originX + 8) {
			return false;
		}
		uint32_t bit = 7 - (x - originX);
		return ((graphics >> bit) & 0x01) != 0;
	};

	uint16_t colorBackground = toRgb565(tiaState.ColorBackground);
	uint16_t colorPlayfield = toRgb565(tiaState.ColorPlayfield);
	uint16_t colorPlayer0 = toRgb565(tiaState.ColorPlayer0);
	uint16_t colorPlayer1 = toRgb565(tiaState.ColorPlayer1);

	for (uint32_t y = 0; y < ScreenHeight; y++) {
		for (uint32_t x = 0; x < ScreenWidth; x++) {
			uint16_t pixel = colorBackground;

			uint32_t coarsePixel = (x / 4);
			uint32_t halfIndex = coarsePixel % 20;
			if (coarsePixel >= 20 && tiaState.PlayfieldReflect) {
				halfIndex = 19 - halfIndex;
			}

			if (getPlayfieldBit(halfIndex)) {
				pixel = colorPlayfield;
			}

			if (tiaState.Missile0Enabled && x == (uint32_t)((tiaState.Player0X + 8) % ScreenWidth)) {
				pixel = colorPlayfield;
			}
			if (tiaState.Missile1Enabled && x == (uint32_t)((tiaState.Player1X + 8) % ScreenWidth)) {
				pixel = colorPlayfield;
			}
			if (tiaState.BallEnabled && x >= tiaState.BallX && x < (uint32_t)(tiaState.BallX + 4)) {
				pixel = colorPlayfield;
			}

			if (isPlayerPixel(tiaState.Player0Graphics, x, tiaState.Player0X)) {
				pixel = colorPlayer0;
			}
			if (isPlayerPixel(tiaState.Player1Graphics, x, tiaState.Player1X)) {
				pixel = colorPlayer1;
			}

			_frameBuffer[y * ScreenWidth + x] = pixel;
		}
	}
}

void Atari2600Console::SaveBattery() {
}

BaseControlManager* Atari2600Console::GetControlManager() {
	return _controlManager.get();
}

ConsoleRegion Atari2600Console::GetRegion() {
	return ConsoleRegion::Ntsc;
}

ConsoleType Atari2600Console::GetConsoleType() {
	return ConsoleType::Atari2600;
}

vector<CpuType> Atari2600Console::GetCpuTypes() {
	return {CpuType::Nes};
}

uint64_t Atari2600Console::GetMasterClock() {
	return _cpu->GetCycleCount();
}

uint32_t Atari2600Console::GetMasterClockRate() {
	return 1193191;
}

double Atari2600Console::GetFps() {
	return 60.0;
}

BaseVideoFilter* Atari2600Console::GetVideoFilter(bool getDefaultFilter) {
	(void)getDefaultFilter;
	if (!_emu) {
		return nullptr;
	}
	return new Atari2600DefaultVideoFilter(_emu);
}

PpuFrameInfo Atari2600Console::GetPpuFrame() {
	PpuFrameInfo frame = {};
	frame.FrameBuffer = reinterpret_cast<uint8_t*>(_frameBuffer.data());
	frame.Width = ScreenWidth;
	frame.Height = ScreenHeight;
	frame.FrameBufferSize = ScreenWidth * ScreenHeight * sizeof(uint16_t);
	frame.FrameCount = _lastFrameSummary.FrameCount;
	frame.ScanlineCount = 262;
	frame.FirstScanline = 0;
	frame.CycleCount = _lastFrameSummary.CpuCyclesThisFrame;
	return frame;
}

RomFormat Atari2600Console::GetRomFormat() {
	return RomFormat::Unknown;
}

AudioTrackInfo Atari2600Console::GetAudioTrackInfo() {
	Atari2600TiaState tiaState = _tia->GetState();
	AudioTrackInfo info = {};
	info.GameTitle = "Atari 2600";
	info.SongTitle = "TIA two-channel mixer";
	info.Comment = "sample=" + std::to_string(tiaState.LastMixedSample) + ", revision=" + std::to_string(tiaState.AudioRevision);
	info.Position = (double)tiaState.AudioSampleCount / GetMasterClockRate();
	info.Length = 0;
	info.FadeLength = 0;
	info.TrackNumber = 1;
	info.TrackCount = 1;
	return info;
}

void Atari2600Console::ProcessAudioPlayerAction(AudioPlayerActionParams p) {
	switch (p.Action) {
		case AudioPlayerAction::NextTrack:
		case AudioPlayerAction::PrevTrack:
		case AudioPlayerAction::SelectTrack:
			_tia->ResetAudioHistory();
			break;
	}
}

AddressInfo Atari2600Console::GetAbsoluteAddress(AddressInfo& relAddress) {
	return relAddress;
}

AddressInfo Atari2600Console::GetPcAbsoluteAddress() {
	return {(int32_t)_cpu->GetProgramCounter(), MemoryType::None};
}

AddressInfo Atari2600Console::GetRelativeAddress(AddressInfo& absAddress, CpuType cpuType) {
	(void)cpuType;
	return absAddress;
}

void Atari2600Console::GetConsoleState(BaseState& state, ConsoleType consoleType) {
	(void)state;
	(void)consoleType;
}

void Atari2600Console::Serialize(Serializer& s) {
	SV(_romLoaded);
	SV(_lastFrameSummary.FrameCount);
	SV(_lastFrameSummary.CpuCyclesThisFrame);
	SV(_lastFrameSummary.ScanlineAtFrameEnd);
	SV(_lastFrameSummary.ColorClockAtFrameEnd);
}
