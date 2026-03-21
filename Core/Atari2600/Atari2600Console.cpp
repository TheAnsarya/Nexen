#include "pch.h"
#include "Atari2600/Atari2600Console.h"
#include "Atari2600/Atari2600Controller.h"
#include "Atari2600/Atari2600Paddle.h"
#include "Atari2600/Atari2600Keypad.h"
#include "Atari2600/Atari2600DrivingController.h"
#include "Atari2600/Atari2600BoosterGrip.h"
#include "Atari2600/Atari2600DefaultVideoFilter.h"
#include "Shared/BaseControlManager.h"
#include "Shared/CpuType.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"
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

class Atari2600Riot {
	private:
		Atari2600RiotState _state = {};
		uint8_t _ram[128] = {};

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
			memset(_ram, 0, sizeof(_ram));
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

		uint8_t ReadRegister(uint16_t addr) {
			if (!(addr & 0x0200)) {
				return _ram[addr & 0x7F];
			}
			switch (addr & 0x07) {
				case 0x00: return ReadPortValue(_state.PortA, _state.PortAInput, _state.PortADirection);
				case 0x01: return ReadPortValue(_state.PortB, _state.PortBInput, _state.PortBDirection);
				case 0x02: return _state.PortADirection;
				case 0x03: return _state.PortBDirection;
				case 0x04: {
					uint8_t timerValue = (uint8_t)(_state.Timer & 0xFF);
					_state.TimerUnderflow = false;
					return timerValue;
				}
				case 0x05: return (uint8_t)((_state.Timer >> 8) & 0xFF);
				case 0x06: {
					uint8_t interruptValue = _state.InterruptFlag ? 1 : 0;
					_state.InterruptFlag = false;
					return interruptValue;
				}
				case 0x07: {
					uint8_t underflowValue = _state.TimerUnderflow ? 1 : 0;
					_state.TimerUnderflow = false;
					return underflowValue;
				}
				default: return 0;
			}
		}

		void WriteRegister(uint16_t addr, uint8_t value) {
			if (!(addr & 0x0200)) {
				_ram[addr & 0x7F] = value;
				return;
			}
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

		void SetState(const Atari2600RiotState& state) {
			_state = state;
			if (_state.TimerDivider == 0) {
				_state.TimerDivider = 1;
			}
			if (_state.TimerDividerCounter == 0) {
				_state.TimerDividerCounter = _state.TimerDivider;
			}
		}

		uint8_t* GetRamData() {
			return _ram;
		}

		static constexpr uint32_t RamSize = 128;
	};

	class Atari2600Tia {
	private:
		Atari2600TiaState _state = {};
		std::array<uint8_t, Atari2600Console::ScreenHeight> _hmoveBlankScanlines = {};
		std::array<Atari2600ScanlineRenderState, Atari2600Console::ScreenHeight> _scanlineRenderState = {};
		static constexpr uint32_t HmoveBlankColorClocks = 8;
		static constexpr uint32_t HmoveLateCycleThreshold = 73;

		// Input ports for TIA reads (INPT0-INPT5)
		// INPT0-3: Paddle/pot, keypad column, or booster grip button inputs
		// INPT4-5: Fire buttons (bit 7: 0=pressed, 1=released)
		uint8_t _inputPort[4] = { 0x80, 0x80, 0x80, 0x80 }; // INPT0-3
		uint8_t _inputPort4 = 0x80; // P0 fire (not pressed)
		uint8_t _inputPort5 = 0x80; // P1 fire (not pressed)

		[[nodiscard]] static uint16_t NormalizeRegisterAddress(uint16_t addr) {
			return addr & 0x3F;
		}

		void MarkRenderDirty() {
			_state.RenderRevision++;
		}

		void MarkAudioDirty() {
			_state.AudioRevision++;
		}

		void ClearCollisionLatches() {
			_state.CollisionCxm0p = 0;
			_state.CollisionCxm1p = 0;
			_state.CollisionCxp0fb = 0;
			_state.CollisionCxp1fb = 0;
			_state.CollisionCxm0fb = 0;
			_state.CollisionCxm1fb = 0;
			_state.CollisionCxblpf = 0;
			_state.CollisionCxppmm = 0;
		}

		[[nodiscard]] static int8_t DecodeMotionNibble(uint8_t value) {
			int8_t motion = (int8_t)((value >> 4) & 0x0F);
			if ((motion & 0x08) != 0) {
				motion = (int8_t)(motion - 16);
			}
			return motion;
		}

		[[nodiscard]] static uint8_t DecodeBallSize(uint8_t controlPlayfield) {
			return (uint8_t)(1u << ((controlPlayfield >> 4) & 0x03));
		}

		[[nodiscard]] static uint8_t EncodeBallSizeBits(uint8_t ballSize) {
			switch (ballSize) {
				case 2: return 0x10;
				case 4: return 0x20;
				case 8: return 0x30;
				default: return 0x00;
			}
		}

		[[nodiscard]] static uint8_t WrapHorizontal(int32_t value) {
			int32_t wrapped = value % (int32_t)Atari2600Console::ScreenWidth;
			if (wrapped < 0) {
				wrapped += Atari2600Console::ScreenWidth;
			}
			return (uint8_t)wrapped;
		}

		void SyncLockedMissilesToPlayers() {
			if (_state.Missile0ResetToPlayer) {
				_state.Missile0X = _state.Player0X;
			}
			if (_state.Missile1ResetToPlayer) {
				_state.Missile1X = _state.Player1X;
			}
		}

		void ApplyHmoveDisplacements() {
			_state.Player0X = WrapHorizontal((int32_t)_state.Player0X + _state.MotionPlayer0);
			_state.Player1X = WrapHorizontal((int32_t)_state.Player1X + _state.MotionPlayer1);
			_state.Missile0X = WrapHorizontal((int32_t)_state.Missile0X + _state.MotionMissile0);
			_state.Missile1X = WrapHorizontal((int32_t)_state.Missile1X + _state.MotionMissile1);
			_state.BallX = WrapHorizontal((int32_t)_state.BallX + _state.MotionBall);
			SyncLockedMissilesToPlayers();
			CaptureCurrentScanlineState();
		}

		[[nodiscard]] Atari2600ScanlineRenderState BuildScanlineRenderState() const {
			Atari2600ScanlineRenderState scanline = {};
			scanline.ColorBackground = _state.ColorBackground;
			scanline.ColorPlayfield = _state.ColorPlayfield;
			scanline.ColorPlayer0 = _state.ColorPlayer0;
			scanline.ColorPlayer1 = _state.ColorPlayer1;
			scanline.Playfield0 = _state.Playfield0;
			scanline.Playfield1 = _state.Playfield1;
			scanline.Playfield2 = _state.Playfield2;
			scanline.PlayfieldReflect = _state.PlayfieldReflect;
			scanline.PlayfieldScoreMode = _state.PlayfieldScoreMode;
			scanline.PlayfieldPriority = _state.PlayfieldPriority;
			scanline.BallSize = _state.BallSize;
			scanline.Nusiz0 = _state.Nusiz0;
			scanline.Nusiz1 = _state.Nusiz1;
			scanline.Player0Graphics = _state.VdelPlayer0 ? _state.DelayedPlayer0Graphics : _state.Player0Graphics;
			scanline.Player1Graphics = _state.VdelPlayer1 ? _state.DelayedPlayer1Graphics : _state.Player1Graphics;
			scanline.Player0Reflect = _state.Player0Reflect;
			scanline.Player1Reflect = _state.Player1Reflect;
			scanline.Missile0Enabled = _state.Missile0Enabled;
			scanline.Missile1Enabled = _state.Missile1Enabled;
			scanline.BallEnabled = _state.VdelBall ? _state.DelayedBallEnabled : _state.BallEnabled;
			scanline.Player0X = _state.Player0X;
			scanline.Player1X = _state.Player1X;
			scanline.Missile0X = _state.Missile0X;
			scanline.Missile1X = _state.Missile1X;
			scanline.BallX = _state.BallX;
			return scanline;
		}

		void CaptureScanlineState(uint32_t scanline) {
			if (scanline < Atari2600Console::ScreenHeight) {
				_scanlineRenderState[scanline] = BuildScanlineRenderState();
			}
		}

		void CaptureCurrentScanlineState() {
			CaptureScanlineState(_state.Scanline);
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
			_state.DelayedPlayer0Graphics = _state.Player0Graphics;
			_state.DelayedPlayer1Graphics = _state.Player1Graphics;
			_state.DelayedBallEnabled = _state.BallEnabled;
			_state.ColorClock = 0;
			_state.Scanline++;
			_state.HmoveDelayToNextScanline = false;
			if (_state.Scanline >= 262) {
				_state.Scanline = 0;
				_state.FrameCount++;
			}
			CaptureCurrentScanlineState();
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
			_hmoveBlankScanlines.fill(0);
			_scanlineRenderState.fill({});
			_state.Player0X = 24;
			_state.Player1X = 96;
			_state.Missile0X = 32;
			_state.Missile1X = 104;
			_state.BallX = 80;
			_state.AudioCounter0 = 1;
			_state.AudioCounter1 = 1;
			ClearCollisionLatches();
			CaptureCurrentScanlineState();
		}

		void SetFireButtonState(uint8_t port0Fire, uint8_t port1Fire) {
			_inputPort4 = port0Fire;
			_inputPort5 = port1Fire;
		}

		void SetInptState(uint8_t inpt0, uint8_t inpt1, uint8_t inpt2, uint8_t inpt3) {
			_inputPort[0] = inpt0;
			_inputPort[1] = inpt1;
			_inputPort[2] = inpt2;
			_inputPort[3] = inpt3;
		}

		void BeginFrameCapture() {
			_hmoveBlankScanlines.fill(0);
			Atari2600ScanlineRenderState currentState = BuildScanlineRenderState();
			_scanlineRenderState.fill(currentState);
			CaptureCurrentScanlineState();
		}

		void StepCpuCycles(uint32_t cpuCycles) {
			for (uint32_t i = 0; i < cpuCycles; i++) {
				if (_state.WsyncHold) {
					_state.WsyncHold = false;
					AdvanceScanline();
				}
				StepColorClocks(3);
				StepAudio();
				if (_state.HmovePending && !_state.HmoveDelayToNextScanline) {
					_state.HmovePending = false;
					_state.HmoveApplyCount++;
					ApplyHmoveDisplacements();
					if (_state.Scanline < Atari2600Console::ScreenHeight) {
						_hmoveBlankScanlines[_state.Scanline] = 1;
					}
					StepColorClocks(HmoveBlankColorClocks);
				}
			}
		}

		void RequestWsync() {
			_state.WsyncHold = true;
			_state.WsyncCount++;
		}

		void RequestHmove() {
			uint32_t cpuCycle = _state.ColorClock / 3;
			_state.HmoveDelayToNextScanline = cpuCycle > HmoveLateCycleThreshold;
			_state.HmovePending = true;
			_state.HmoveStrobeCount++;
		}

		void ClearHmove() {
			_state.HmovePending = false;
			_state.HmoveDelayToNextScanline = false;
			_state.MotionPlayer0 = 0;
			_state.MotionPlayer1 = 0;
			_state.MotionMissile0 = 0;
			_state.MotionMissile1 = 0;
			_state.MotionBall = 0;
		}

		[[nodiscard]] bool IsHmoveBlankScanline(uint32_t scanline) const {
			if (scanline >= _hmoveBlankScanlines.size()) {
				return false;
			}
			return _hmoveBlankScanlines[scanline] != 0;
		}

		[[nodiscard]] Atari2600ScanlineRenderState GetScanlineRenderState(uint32_t scanline) const {
			if (scanline >= _scanlineRenderState.size()) {
				return BuildScanlineRenderState();
			}
			return _scanlineRenderState[scanline];
		}

		void LatchCollisionPixel(bool missile0Pixel, bool missile1Pixel, bool player0Pixel, bool player1Pixel, bool ballPixel, bool playfieldPixel) {
			if (missile0Pixel && player1Pixel) {
				_state.CollisionCxm0p |= 0x80;
			}
			if (missile0Pixel && player0Pixel) {
				_state.CollisionCxm0p |= 0x40;
			}
			if (missile1Pixel && player0Pixel) {
				_state.CollisionCxm1p |= 0x80;
			}
			if (missile1Pixel && player1Pixel) {
				_state.CollisionCxm1p |= 0x40;
			}
			if (player0Pixel && playfieldPixel) {
				_state.CollisionCxp0fb |= 0x80;
			}
			if (player0Pixel && ballPixel) {
				_state.CollisionCxp0fb |= 0x40;
			}
			if (player1Pixel && playfieldPixel) {
				_state.CollisionCxp1fb |= 0x80;
			}
			if (player1Pixel && ballPixel) {
				_state.CollisionCxp1fb |= 0x40;
			}
			if (missile0Pixel && playfieldPixel) {
				_state.CollisionCxm0fb |= 0x80;
			}
			if (missile0Pixel && ballPixel) {
				_state.CollisionCxm0fb |= 0x40;
			}
			if (missile1Pixel && playfieldPixel) {
				_state.CollisionCxm1fb |= 0x80;
			}
			if (missile1Pixel && ballPixel) {
				_state.CollisionCxm1fb |= 0x40;
			}
			if (ballPixel && playfieldPixel) {
				_state.CollisionCxblpf |= 0x80;
			}
			if (player0Pixel && player1Pixel) {
				_state.CollisionCxppmm |= 0x80;
			}
			if (missile0Pixel && missile1Pixel) {
				_state.CollisionCxppmm |= 0x40;
			}
		}

		uint8_t ReadRegister(uint16_t addr) const {
			// TIA read registers only decode bits 0-3 of the address
			// Addresses 0x00-0x07: Collision latches
			// Addresses 0x08-0x0B: Pot/paddle inputs (INPT0-3)
			// Addresses 0x0C-0x0D: Fire button inputs (INPT4-5)
			switch (addr & 0x0f) {
				case 0x00: return _state.CollisionCxm0p;
				case 0x01: return _state.CollisionCxm1p;
				case 0x02: return _state.CollisionCxp0fb;
				case 0x03: return _state.CollisionCxp1fb;
				case 0x04: return _state.CollisionCxm0fb;
				case 0x05: return _state.CollisionCxm1fb;
				case 0x06: return _state.CollisionCxblpf;
				case 0x07: return _state.CollisionCxppmm;
				case 0x08: return _inputPort[0]; // INPT0 — paddle 0 / keypad col / booster
				case 0x09: return _inputPort[1]; // INPT1 — paddle 1 / keypad col / booster
				case 0x0a: return _inputPort[2]; // INPT2 — paddle 2 / keypad col / booster
				case 0x0b: return _inputPort[3]; // INPT3 — paddle 3 / keypad col / booster
				case 0x0c: return _inputPort4; // INPT4 — P0 fire button
				case 0x0d: return _inputPort5; // INPT5 — P1 fire button
				default: return 0;
			}
		}

		// Debug read that returns write register state for the register viewer
		uint8_t DebugReadWriteRegister(uint16_t addr) const {
			switch (NormalizeRegisterAddress(addr)) {
				case 0x06: return _state.ColorPlayer0;
				case 0x07: return _state.ColorPlayer1;
				case 0x08: return _state.ColorPlayfield;
				case 0x09: return _state.ColorBackground;
				case 0x0A:
					return (uint8_t)((_state.PlayfieldReflect ? 0x01 : 0x00)
						| (_state.PlayfieldScoreMode ? 0x02 : 0x00)
						| (_state.PlayfieldPriority ? 0x04 : 0x00)
						| EncodeBallSizeBits(_state.BallSize));
				case 0x0B: return _state.Player0Reflect ? 0x08 : 0x00;
				case 0x0C: return _state.Player1Reflect ? 0x08 : 0x00;
				case 0x0D: return _state.Playfield0;
				case 0x0E: return _state.Playfield1;
				case 0x0F: return _state.Playfield2;
				case 0x1B: return _state.Player0Graphics;
				case 0x1C: return _state.Player1Graphics;
				case 0x1D: return _state.Missile0Enabled ? 0x02 : 0x00;
				case 0x1E: return _state.Missile1Enabled ? 0x02 : 0x00;
				case 0x1F: return _state.BallEnabled ? 0x02 : 0x00;
				case 0x25: return _state.VdelPlayer0 ? 0x01 : 0x00;
				case 0x26: return _state.VdelPlayer1 ? 0x01 : 0x00;
				case 0x27: return _state.VdelBall ? 0x01 : 0x00;
				case 0x28: return _state.Missile0ResetToPlayer ? 0x02 : 0x00;
				case 0x29: return _state.Missile1ResetToPlayer ? 0x02 : 0x00;
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

				case 0x2B:
					ClearHmove();
					break;

				case 0x2C:
					ClearCollisionLatches();
					break;

				case 0x04:
					_state.Nusiz0 = value;
					MarkRenderDirty();
					CaptureCurrentScanlineState();
					break;

				case 0x05:
					_state.Nusiz1 = value;
					MarkRenderDirty();
					CaptureCurrentScanlineState();
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
					_state.PlayfieldScoreMode = (value & 0x02) != 0;
					_state.PlayfieldPriority = (value & 0x04) != 0;
					_state.BallSize = DecodeBallSize(value);
					MarkRenderDirty();
					CaptureCurrentScanlineState();
					break;

				case 0x0B:
					_state.Player0Reflect = (value & 0x08) != 0;
					MarkRenderDirty();
					CaptureCurrentScanlineState();
					break;

				case 0x0C:
					_state.Player1Reflect = (value & 0x08) != 0;
					MarkRenderDirty();
					CaptureCurrentScanlineState();
					break;

				case 0x0D:
					_state.Playfield0 = value;
					MarkRenderDirty();
					CaptureCurrentScanlineState();
					break;

				case 0x0E:
					_state.Playfield1 = value;
					MarkRenderDirty();
					CaptureCurrentScanlineState();
					break;

				case 0x0F:
					_state.Playfield2 = value;
					MarkRenderDirty();
					CaptureCurrentScanlineState();
					break;

				case 0x10:
					_state.Player0X = (uint8_t)(_state.ColorClock % Atari2600Console::ScreenWidth);
					SyncLockedMissilesToPlayers();
					MarkRenderDirty();
					CaptureCurrentScanlineState();
					break;

				case 0x11:
					_state.Player1X = (uint8_t)(_state.ColorClock % Atari2600Console::ScreenWidth);
					SyncLockedMissilesToPlayers();
					MarkRenderDirty();
					CaptureCurrentScanlineState();
					break;

				case 0x12:
					_state.Missile0X = (uint8_t)(_state.ColorClock % Atari2600Console::ScreenWidth);
					MarkRenderDirty();
					CaptureCurrentScanlineState();
					break;

				case 0x13:
					_state.Missile1X = (uint8_t)(_state.ColorClock % Atari2600Console::ScreenWidth);
					MarkRenderDirty();
					CaptureCurrentScanlineState();
					break;

				case 0x14:
					_state.BallX = (uint8_t)(_state.ColorClock % Atari2600Console::ScreenWidth);
					MarkRenderDirty();
					CaptureCurrentScanlineState();
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
					CaptureCurrentScanlineState();
					break;

				case 0x1C:
					_state.Player1Graphics = value;
					MarkRenderDirty();
					CaptureCurrentScanlineState();
					break;

				case 0x1D:
					_state.Missile0Enabled = (value & 0x02) != 0;
					MarkRenderDirty();
					CaptureCurrentScanlineState();
					break;

				case 0x1E:
					_state.Missile1Enabled = (value & 0x02) != 0;
					MarkRenderDirty();
					CaptureCurrentScanlineState();
					break;

				case 0x1F:
					_state.BallEnabled = (value & 0x02) != 0;
					MarkRenderDirty();
					CaptureCurrentScanlineState();
					break;

				case 0x20:
					_state.MotionPlayer0 = DecodeMotionNibble(value);
					break;

				case 0x21:
					_state.MotionPlayer1 = DecodeMotionNibble(value);
					break;

				case 0x22:
					_state.MotionMissile0 = DecodeMotionNibble(value);
					break;

				case 0x23:
					_state.MotionMissile1 = DecodeMotionNibble(value);
					break;

				case 0x24:
					_state.MotionBall = DecodeMotionNibble(value);
					break;

				case 0x25:
					_state.VdelPlayer0 = (value & 0x01) != 0;
					MarkRenderDirty();
					CaptureCurrentScanlineState();
					break;

				case 0x26:
					_state.VdelPlayer1 = (value & 0x01) != 0;
					MarkRenderDirty();
					CaptureCurrentScanlineState();
					break;

				case 0x27:
					_state.VdelBall = (value & 0x01) != 0;
					MarkRenderDirty();
					CaptureCurrentScanlineState();
					break;

				case 0x28:
					_state.Missile0ResetToPlayer = (value & 0x02) != 0;
					SyncLockedMissilesToPlayers();
					MarkRenderDirty();
					CaptureCurrentScanlineState();
					break;

				case 0x29:
					_state.Missile1ResetToPlayer = (value & 0x02) != 0;
					SyncLockedMissilesToPlayers();
					MarkRenderDirty();
					CaptureCurrentScanlineState();
					break;

				default:
					break;
			}
		}

		Atari2600TiaState GetState() const {
			return _state;
		}

		void SetState(const Atari2600TiaState& state) {
			_state = state;
			_hmoveBlankScanlines.fill(0);
			Atari2600ScanlineRenderState currentState = BuildScanlineRenderState();
			_scanlineRenderState.fill(currentState);
			CaptureCurrentScanlineState();
			if (_state.AudioCounter0 == 0) {
				_state.AudioCounter0 = (uint16_t)(_state.AudioFrequency0 + 1);
			}
			if (_state.AudioCounter1 == 0) {
				_state.AudioCounter1 = (uint16_t)(_state.AudioFrequency1 + 1);
			}
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

		uint8_t Read(uint16_t addr) {
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

		void Compare(uint8_t left, uint8_t right) {
			uint16_t diff = (uint16_t)left - right;
			SetFlag(FlagCarry, left >= right);
			SetFlag(FlagZero, left == right);
			SetFlag(FlagNegative, (diff & 0x80) != 0);
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

		void PushByte(uint8_t value) {
			Write(0x0100 | _sp, value);
			_sp--;
		}

		[[nodiscard]] uint8_t PullByte() {
			_sp++;
			return Read(0x0100 | _sp);
		}

		void PushWord(uint16_t value) {
			PushByte((uint8_t)(value >> 8));
			PushByte((uint8_t)(value & 0xff));
		}

		[[nodiscard]] uint16_t PullWord() {
			uint16_t low = PullByte();
			uint16_t high = PullByte();
			return (uint16_t)((high << 8) | low);
		}

		[[nodiscard]] uint16_t AddrZP() { return FetchByte(); }
		[[nodiscard]] uint16_t AddrZPX() { return (uint8_t)(FetchByte() + _x); }
		[[nodiscard]] uint16_t AddrZPY() { return (uint8_t)(FetchByte() + _y); }
		[[nodiscard]] uint16_t AddrAbs() { return FetchWord(); }

		[[nodiscard]] uint16_t AddrAbsX(bool& pageCrossed) {
			uint16_t base = FetchWord();
			uint16_t addr = (uint16_t)(base + _x);
			pageCrossed = ((base ^ addr) & 0xff00) != 0;
			return addr;
		}

		[[nodiscard]] uint16_t AddrAbsY(bool& pageCrossed) {
			uint16_t base = FetchWord();
			uint16_t addr = (uint16_t)(base + _y);
			pageCrossed = ((base ^ addr) & 0xff00) != 0;
			return addr;
		}

		[[nodiscard]] uint16_t AddrIndX() {
			uint8_t zp = (uint8_t)(FetchByte() + _x);
			uint16_t lo = Read(zp);
			uint16_t hi = Read((uint8_t)(zp + 1));
			return (uint16_t)((hi << 8) | lo);
		}

		[[nodiscard]] uint16_t AddrIndY(bool& pageCrossed) {
			uint8_t zp = FetchByte();
			uint16_t lo = Read(zp);
			uint16_t hi = Read((uint8_t)(zp + 1));
			uint16_t base = (uint16_t)((hi << 8) | lo);
			uint16_t addr = (uint16_t)(base + _y);
			pageCrossed = ((base ^ addr) & 0xff00) != 0;
			return addr;
		}

		[[nodiscard]] uint8_t DoASL(uint8_t v) {
			SetFlag(FlagCarry, (v & 0x80) != 0);
			v = (uint8_t)(v << 1);
			UpdateZeroNegative(v);
			return v;
		}

		[[nodiscard]] uint8_t DoLSR(uint8_t v) {
			SetFlag(FlagCarry, (v & 0x01) != 0);
			v = (uint8_t)(v >> 1);
			UpdateZeroNegative(v);
			return v;
		}

		[[nodiscard]] uint8_t DoROL(uint8_t v) {
			bool oldCarry = GetFlag(FlagCarry);
			SetFlag(FlagCarry, (v & 0x80) != 0);
			v = (uint8_t)((v << 1) | (oldCarry ? 1 : 0));
			UpdateZeroNegative(v);
			return v;
		}

		[[nodiscard]] uint8_t DoROR(uint8_t v) {
			bool oldCarry = GetFlag(FlagCarry);
			SetFlag(FlagCarry, (v & 0x01) != 0);
			v = (uint8_t)((v >> 1) | (oldCarry ? 0x80 : 0));
			UpdateZeroNegative(v);
			return v;
		}

		void DoBIT(uint8_t v) {
			SetFlag(FlagZero, (_a & v) == 0);
			SetFlag(FlagOverflow, (v & 0x40) != 0);
			SetFlag(FlagNegative, (v & 0x80) != 0);
		}

		[[nodiscard]] uint8_t ExecuteInstruction() {
			uint8_t opcode = FetchByte();
			bool pageCrossed = false;

			switch (opcode) {
				// BRK
				case 0x00: {
					_pc = (_pc + 1) & 0x1fff;
					PushWord(_pc);
					PushByte(_status | FlagBreak | FlagUnused);
					SetFlag(FlagInterruptDisable, true);
					uint16_t lo = Read(0x1ffe);
					uint16_t hi = Read(0x1fff);
					_pc = ((hi << 8) | lo) & 0x1fff;
					return 7;
				}

				// ORA
				case 0x01: _a |= Read(AddrIndX()); UpdateZeroNegative(_a); return 6;
				case 0x05: _a |= Read(AddrZP()); UpdateZeroNegative(_a); return 3;
				case 0x09: _a |= FetchByte(); UpdateZeroNegative(_a); return 2;
				case 0x0d: _a |= Read(AddrAbs()); UpdateZeroNegative(_a); return 4;
				case 0x11: _a |= Read(AddrIndY(pageCrossed)); UpdateZeroNegative(_a); return pageCrossed ? 6 : 5;
				case 0x15: _a |= Read(AddrZPX()); UpdateZeroNegative(_a); return 4;
				case 0x19: _a |= Read(AddrAbsY(pageCrossed)); UpdateZeroNegative(_a); return pageCrossed ? 5 : 4;
				case 0x1d: _a |= Read(AddrAbsX(pageCrossed)); UpdateZeroNegative(_a); return pageCrossed ? 5 : 4;

				// ASL
				case 0x06: { uint16_t a = AddrZP(); Write(a, DoASL(Read(a))); return 5; }
				case 0x0a: _a = DoASL(_a); return 2;
				case 0x0e: { uint16_t a = AddrAbs(); Write(a, DoASL(Read(a))); return 6; }
				case 0x16: { uint16_t a = AddrZPX(); Write(a, DoASL(Read(a))); return 6; }
				case 0x1e: { uint16_t a = AddrAbsX(pageCrossed); Write(a, DoASL(Read(a))); return 7; }

				// PHP
				case 0x08: PushByte(_status | FlagBreak | FlagUnused); return 3;

				// BPL
				case 0x10: return BranchIf(!GetFlag(FlagNegative));

				// CLC
				case 0x18: SetFlag(FlagCarry, false); return 2;

				// JSR
				case 0x20: {
					uint16_t target = FetchWord();
					PushWord((_pc - 1) & 0x1fff);
					_pc = target & 0x1fff;
					return 6;
				}

				// AND
				case 0x21: _a &= Read(AddrIndX()); UpdateZeroNegative(_a); return 6;
				case 0x25: _a &= Read(AddrZP()); UpdateZeroNegative(_a); return 3;
				case 0x29: _a &= FetchByte(); UpdateZeroNegative(_a); return 2;
				case 0x2d: _a &= Read(AddrAbs()); UpdateZeroNegative(_a); return 4;
				case 0x31: _a &= Read(AddrIndY(pageCrossed)); UpdateZeroNegative(_a); return pageCrossed ? 6 : 5;
				case 0x35: _a &= Read(AddrZPX()); UpdateZeroNegative(_a); return 4;
				case 0x39: _a &= Read(AddrAbsY(pageCrossed)); UpdateZeroNegative(_a); return pageCrossed ? 5 : 4;
				case 0x3d: _a &= Read(AddrAbsX(pageCrossed)); UpdateZeroNegative(_a); return pageCrossed ? 5 : 4;

				// BIT
				case 0x24: DoBIT(Read(AddrZP())); return 3;
				case 0x2c: DoBIT(Read(AddrAbs())); return 4;

				// ROL
				case 0x26: { uint16_t a = AddrZP(); Write(a, DoROL(Read(a))); return 5; }
				case 0x2a: _a = DoROL(_a); return 2;
				case 0x2e: { uint16_t a = AddrAbs(); Write(a, DoROL(Read(a))); return 6; }
				case 0x36: { uint16_t a = AddrZPX(); Write(a, DoROL(Read(a))); return 6; }
				case 0x3e: { uint16_t a = AddrAbsX(pageCrossed); Write(a, DoROL(Read(a))); return 7; }

				// PLP
				case 0x28: _status = PullByte(); SetFlag(FlagUnused, true); return 4;

				// BMI
				case 0x30: return BranchIf(GetFlag(FlagNegative));

				// SEC
				case 0x38: SetFlag(FlagCarry, true); return 2;

				// RTI
				case 0x40: {
					_status = PullByte();
					SetFlag(FlagUnused, true);
					_pc = PullWord() & 0x1fff;
					return 6;
				}

				// EOR
				case 0x41: _a ^= Read(AddrIndX()); UpdateZeroNegative(_a); return 6;
				case 0x45: _a ^= Read(AddrZP()); UpdateZeroNegative(_a); return 3;
				case 0x49: _a ^= FetchByte(); UpdateZeroNegative(_a); return 2;
				case 0x4d: _a ^= Read(AddrAbs()); UpdateZeroNegative(_a); return 4;
				case 0x51: _a ^= Read(AddrIndY(pageCrossed)); UpdateZeroNegative(_a); return pageCrossed ? 6 : 5;
				case 0x55: _a ^= Read(AddrZPX()); UpdateZeroNegative(_a); return 4;
				case 0x59: _a ^= Read(AddrAbsY(pageCrossed)); UpdateZeroNegative(_a); return pageCrossed ? 5 : 4;
				case 0x5d: _a ^= Read(AddrAbsX(pageCrossed)); UpdateZeroNegative(_a); return pageCrossed ? 5 : 4;

				// LSR
				case 0x46: { uint16_t a = AddrZP(); Write(a, DoLSR(Read(a))); return 5; }
				case 0x4a: _a = DoLSR(_a); return 2;
				case 0x4e: { uint16_t a = AddrAbs(); Write(a, DoLSR(Read(a))); return 6; }
				case 0x56: { uint16_t a = AddrZPX(); Write(a, DoLSR(Read(a))); return 6; }
				case 0x5e: { uint16_t a = AddrAbsX(pageCrossed); Write(a, DoLSR(Read(a))); return 7; }

				// PHA
				case 0x48: PushByte(_a); return 3;

				// JMP abs
				case 0x4c: _pc = FetchWord() & 0x1fff; return 3;

				// BVC
				case 0x50: return BranchIf(!GetFlag(FlagOverflow));

				// CLI
				case 0x58: SetFlag(FlagInterruptDisable, false); return 2;

				// RTS
				case 0x60: _pc = (PullWord() + 1) & 0x1fff; return 6;

				// ADC
				case 0x61: _a = AddWithCarry(_a, Read(AddrIndX()), false); return 6;
				case 0x65: _a = AddWithCarry(_a, Read(AddrZP()), false); return 3;
				case 0x69: _a = AddWithCarry(_a, FetchByte(), false); return 2;
				case 0x6d: _a = AddWithCarry(_a, Read(AddrAbs()), false); return 4;
				case 0x71: _a = AddWithCarry(_a, Read(AddrIndY(pageCrossed)), false); return pageCrossed ? 6 : 5;
				case 0x75: _a = AddWithCarry(_a, Read(AddrZPX()), false); return 4;
				case 0x79: _a = AddWithCarry(_a, Read(AddrAbsY(pageCrossed)), false); return pageCrossed ? 5 : 4;
				case 0x7d: _a = AddWithCarry(_a, Read(AddrAbsX(pageCrossed)), false); return pageCrossed ? 5 : 4;

				// ROR
				case 0x66: { uint16_t a = AddrZP(); Write(a, DoROR(Read(a))); return 5; }
				case 0x6a: _a = DoROR(_a); return 2;
				case 0x6e: { uint16_t a = AddrAbs(); Write(a, DoROR(Read(a))); return 6; }
				case 0x76: { uint16_t a = AddrZPX(); Write(a, DoROR(Read(a))); return 6; }
				case 0x7e: { uint16_t a = AddrAbsX(pageCrossed); Write(a, DoROR(Read(a))); return 7; }

				// PLA
				case 0x68: _a = PullByte(); UpdateZeroNegative(_a); return 4;

				// JMP indirect (with 6502 page-crossing bug)
				case 0x6c: {
					uint16_t ptr = FetchWord();
					uint16_t lo = Read(ptr);
					uint16_t hiAddr = (ptr & 0xff00) | ((ptr + 1) & 0x00ff);
					uint16_t hi = Read(hiAddr);
					_pc = ((hi << 8) | lo) & 0x1fff;
					return 5;
				}

				// BVS
				case 0x70: return BranchIf(GetFlag(FlagOverflow));

				// SEI
				case 0x78: SetFlag(FlagInterruptDisable, true); return 2;

				// STA
				case 0x81: Write(AddrIndX(), _a); return 6;
				case 0x85: Write(AddrZP(), _a); return 3;
				case 0x8d: Write(AddrAbs(), _a); return 4;
				case 0x91: Write(AddrIndY(pageCrossed), _a); return 6;
				case 0x95: Write(AddrZPX(), _a); return 4;
				case 0x99: Write(AddrAbsY(pageCrossed), _a); return 5;
				case 0x9d: Write(AddrAbsX(pageCrossed), _a); return 5;

				// STY
				case 0x84: Write(AddrZP(), _y); return 3;
				case 0x8c: Write(AddrAbs(), _y); return 4;
				case 0x94: Write(AddrZPX(), _y); return 4;

				// STX
				case 0x86: Write(AddrZP(), _x); return 3;
				case 0x8e: Write(AddrAbs(), _x); return 4;
				case 0x96: Write(AddrZPY(), _x); return 4;

				// DEY
				case 0x88: _y--; UpdateZeroNegative(_y); return 2;

				// TXA
				case 0x8a: _a = _x; UpdateZeroNegative(_a); return 2;

				// BCC
				case 0x90: return BranchIf(!GetFlag(FlagCarry));

				// TYA
				case 0x98: _a = _y; UpdateZeroNegative(_a); return 2;

				// TXS (does NOT set flags)
				case 0x9a: _sp = _x; return 2;

				// LDY
				case 0xa0: _y = FetchByte(); UpdateZeroNegative(_y); return 2;
				case 0xa4: _y = Read(AddrZP()); UpdateZeroNegative(_y); return 3;
				case 0xac: _y = Read(AddrAbs()); UpdateZeroNegative(_y); return 4;
				case 0xb4: _y = Read(AddrZPX()); UpdateZeroNegative(_y); return 4;
				case 0xbc: _y = Read(AddrAbsX(pageCrossed)); UpdateZeroNegative(_y); return pageCrossed ? 5 : 4;

				// LDA
				case 0xa1: _a = Read(AddrIndX()); UpdateZeroNegative(_a); return 6;
				case 0xa5: _a = Read(AddrZP()); UpdateZeroNegative(_a); return 3;
				case 0xa9: _a = FetchByte(); UpdateZeroNegative(_a); return 2;
				case 0xad: _a = Read(AddrAbs()); UpdateZeroNegative(_a); return 4;
				case 0xb1: _a = Read(AddrIndY(pageCrossed)); UpdateZeroNegative(_a); return pageCrossed ? 6 : 5;
				case 0xb5: _a = Read(AddrZPX()); UpdateZeroNegative(_a); return 4;
				case 0xb9: _a = Read(AddrAbsY(pageCrossed)); UpdateZeroNegative(_a); return pageCrossed ? 5 : 4;
				case 0xbd: _a = Read(AddrAbsX(pageCrossed)); UpdateZeroNegative(_a); return pageCrossed ? 5 : 4;

				// LDX
				case 0xa2: _x = FetchByte(); UpdateZeroNegative(_x); return 2;
				case 0xa6: _x = Read(AddrZP()); UpdateZeroNegative(_x); return 3;
				case 0xae: _x = Read(AddrAbs()); UpdateZeroNegative(_x); return 4;
				case 0xb6: _x = Read(AddrZPY()); UpdateZeroNegative(_x); return 4;
				case 0xbe: _x = Read(AddrAbsY(pageCrossed)); UpdateZeroNegative(_x); return pageCrossed ? 5 : 4;

				// TAY
				case 0xa8: _y = _a; UpdateZeroNegative(_y); return 2;

				// TAX
				case 0xaa: _x = _a; UpdateZeroNegative(_x); return 2;

				// BCS
				case 0xb0: return BranchIf(GetFlag(FlagCarry));

				// CLV
				case 0xb8: SetFlag(FlagOverflow, false); return 2;

				// TSX
				case 0xba: _x = _sp; UpdateZeroNegative(_x); return 2;

				// CPY
				case 0xc0: Compare(_y, FetchByte()); return 2;
				case 0xc4: Compare(_y, Read(AddrZP())); return 3;
				case 0xcc: Compare(_y, Read(AddrAbs())); return 4;

				// CMP
				case 0xc1: Compare(_a, Read(AddrIndX())); return 6;
				case 0xc5: Compare(_a, Read(AddrZP())); return 3;
				case 0xc9: Compare(_a, FetchByte()); return 2;
				case 0xcd: Compare(_a, Read(AddrAbs())); return 4;
				case 0xd1: Compare(_a, Read(AddrIndY(pageCrossed))); return pageCrossed ? 6 : 5;
				case 0xd5: Compare(_a, Read(AddrZPX())); return 4;
				case 0xd9: Compare(_a, Read(AddrAbsY(pageCrossed))); return pageCrossed ? 5 : 4;
				case 0xdd: Compare(_a, Read(AddrAbsX(pageCrossed))); return pageCrossed ? 5 : 4;

				// DEC
				case 0xc6: { uint16_t a = AddrZP(); uint8_t v = Read(a) - 1; Write(a, v); UpdateZeroNegative(v); return 5; }
				case 0xce: { uint16_t a = AddrAbs(); uint8_t v = Read(a) - 1; Write(a, v); UpdateZeroNegative(v); return 6; }
				case 0xd6: { uint16_t a = AddrZPX(); uint8_t v = Read(a) - 1; Write(a, v); UpdateZeroNegative(v); return 6; }
				case 0xde: { uint16_t a = AddrAbsX(pageCrossed); uint8_t v = Read(a) - 1; Write(a, v); UpdateZeroNegative(v); return 7; }

				// INY
				case 0xc8: _y++; UpdateZeroNegative(_y); return 2;

				// DEX
				case 0xca: _x--; UpdateZeroNegative(_x); return 2;

				// BNE
				case 0xd0: return BranchIf(!GetFlag(FlagZero));

				// CLD
				case 0xd8: SetFlag(FlagDecimal, false); return 2;

				// CPX
				case 0xe0: Compare(_x, FetchByte()); return 2;
				case 0xe4: Compare(_x, Read(AddrZP())); return 3;
				case 0xec: Compare(_x, Read(AddrAbs())); return 4;

				// SBC
				case 0xe1: _a = AddWithCarry(_a, Read(AddrIndX()), true); return 6;
				case 0xe5: _a = AddWithCarry(_a, Read(AddrZP()), true); return 3;
				case 0xe9: _a = AddWithCarry(_a, FetchByte(), true); return 2;
				case 0xed: _a = AddWithCarry(_a, Read(AddrAbs()), true); return 4;
				case 0xf1: _a = AddWithCarry(_a, Read(AddrIndY(pageCrossed)), true); return pageCrossed ? 6 : 5;
				case 0xf5: _a = AddWithCarry(_a, Read(AddrZPX()), true); return 4;
				case 0xf9: _a = AddWithCarry(_a, Read(AddrAbsY(pageCrossed)), true); return pageCrossed ? 5 : 4;
				case 0xfd: _a = AddWithCarry(_a, Read(AddrAbsX(pageCrossed)), true); return pageCrossed ? 5 : 4;

				// INC
				case 0xe6: { uint16_t a = AddrZP(); uint8_t v = Read(a) + 1; Write(a, v); UpdateZeroNegative(v); return 5; }
				case 0xee: { uint16_t a = AddrAbs(); uint8_t v = Read(a) + 1; Write(a, v); UpdateZeroNegative(v); return 6; }
				case 0xf6: { uint16_t a = AddrZPX(); uint8_t v = Read(a) + 1; Write(a, v); UpdateZeroNegative(v); return 6; }
				case 0xfe: { uint16_t a = AddrAbsX(pageCrossed); uint8_t v = Read(a) + 1; Write(a, v); UpdateZeroNegative(v); return 7; }

				// INX
				case 0xe8: _x++; UpdateZeroNegative(_x); return 2;

				// NOP
				case 0xea: return 2;

				// BEQ
				case 0xf0: return BranchIf(GetFlag(FlagZero));

				// SED
				case 0xf8: SetFlag(FlagDecimal, true); return 2;

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

		void ExportState(uint16_t& programCounter, uint64_t& cycleCount, uint8_t& a, uint8_t& x, uint8_t& y, uint8_t& sp, uint8_t& status, uint8_t& remainingCycles) const {
			programCounter = _pc;
			cycleCount = _cycleCount;
			a = _a;
			x = _x;
			y = _y;
			sp = _sp;
			status = _status;
			remainingCycles = _instructionCyclesRemaining;
		}

		void ImportState(uint16_t programCounter, uint64_t cycleCount, uint8_t a, uint8_t x, uint8_t y, uint8_t sp, uint8_t status, uint8_t remainingCycles) {
			_pc = programCounter & 0x1FFF;
			_cycleCount = cycleCount;
			_a = a;
			_x = x;
			_y = y;
			_sp = sp;
			_status = status;
			_instructionCyclesRemaining = remainingCycles;
		}
	};

	class Atari2600ControlManager final : public BaseControlManager {
	private:
		Atari2600Config _prevConfig = {};
		// Cached input state for hardware to read
		uint8_t _swcha = 0xff;     // Joystick/controller directions (active-low, all released)
		uint8_t _swchb = 0xff;     // Console switches
		uint8_t _fireP0 = 0x80;    // P0 fire button (bit 7: 0=pressed, 1=released)
		uint8_t _fireP1 = 0x80;    // P1 fire button
		uint8_t _inpt[4] = { 0x80, 0x80, 0x80, 0x80 }; // INPT0-3 state (paddles/keypad/booster)

	public:
		explicit Atari2600ControlManager(Emulator* emu)
			: BaseControlManager(emu, CpuType::Atari2600) {
		}

		shared_ptr<BaseControlDevice> CreateControllerDevice(ControllerType type, uint8_t port) override {
			shared_ptr<BaseControlDevice> device;
			Atari2600Config& cfg = _emu->GetSettings()->GetAtari2600Config();
			KeyMappingSet& keys = (port == 0) ? cfg.Port1.Keys : cfg.Port2.Keys;

			switch (type) {
				default:
				case ControllerType::None:
					break;
				case ControllerType::Atari2600Joystick:
					device = std::make_shared<Atari2600Controller>(_emu, port, keys);
					break;
				case ControllerType::Atari2600Paddle:
					// Create paddle A for this port (index 0 or 2)
					device = std::make_shared<Atari2600Paddle>(_emu, port, port * 2, keys);
					break;
				case ControllerType::Atari2600Keypad:
					device = std::make_shared<Atari2600Keypad>(_emu, port, keys);
					break;
				case ControllerType::Atari2600DrivingController:
					device = std::make_shared<Atari2600DrivingController>(_emu, port, keys);
					break;
				case ControllerType::Atari2600BoosterGrip:
					device = std::make_shared<Atari2600BoosterGrip>(_emu, port, keys);
					break;
			}
			return device;
		}

		void UpdateControlDevices() override {
			Atari2600Config cfg = _emu->GetSettings()->GetAtari2600Config();
			if (_emu->GetSettings()->IsEqual(_prevConfig, cfg) && _controlDevices.size() > 0) {
				return;
			}
			_prevConfig = cfg;

			auto lock = _deviceLock.AcquireSafe();

			// Rebuild device list: re-register valid system devices and add controllers.
			// ClearDevices() is not used here because _systemDevices may contain a
			// null entry when the Emulator was not fully initialized (e.g., unit tests
			// that create a bare Emulator without calling Initialize()).
			_controlDevices.clear();
			for (const shared_ptr<BaseControlDevice>& sysDevice : _systemDevices) {
				if (sysDevice) {
					RegisterControlDevice(sysDevice);
				}
			}

			// Port 1 controller
			shared_ptr<BaseControlDevice> dev0(CreateControllerDevice(cfg.Port1.Type, 0));
			if (dev0) {
				RegisterControlDevice(dev0);
			}

			// Port 2 controller
			shared_ptr<BaseControlDevice> dev1(CreateControllerDevice(cfg.Port2.Type, 1));
			if (dev1) {
				RegisterControlDevice(dev1);
			}
		}

		void UpdateInputState() override {
			BaseControlManager::UpdateInputState();

			uint8_t swcha = 0xff;
			_fireP0 = 0x80;
			_fireP1 = 0x80;
			_inpt[0] = 0x80;
			_inpt[1] = 0x80;
			_inpt[2] = 0x80;
			_inpt[3] = 0x80;

			for (shared_ptr<BaseControlDevice>& controller : _controlDevices) {
				uint8_t port = controller->GetPort();
				ControllerType type = controller->GetControllerType();

				switch (type) {
					case ControllerType::Atari2600Joystick: {
						auto* joystick = static_cast<Atari2600Controller*>(controller.get());
						uint8_t nibble = joystick->GetDirectionNibble();
						if (port == 0) {
							swcha = (uint8_t)((swcha & 0x0f) | (nibble << 4));
							_fireP0 = joystick->GetFireState();
						} else if (port == 1) {
							swcha = (uint8_t)((swcha & 0xf0) | nibble);
							_fireP1 = joystick->GetFireState();
						}
						break;
					}

					case ControllerType::Atari2600Paddle: {
						auto* paddle = static_cast<Atari2600Paddle*>(controller.get());
						uint8_t idx = paddle->GetPaddleIndex();
						if (idx < 4) {
							// Simplified: position maps directly to INPT threshold
							// Full accuracy would use scanline-based charge timing
							_inpt[idx] = paddle->GetInptState(128); // Mid-frame estimate
						}
						if (port == 0) {
							_fireP0 = paddle->GetFireState();
						} else if (port == 1) {
							_fireP1 = paddle->GetFireState();
						}
						break;
					}

					case ControllerType::Atari2600Keypad: {
						// Keypad INPT values are handled per-scanline via GetKeypadColumnState()
						// SWCHA is not driven by keypad (no direction nibble)
						break;
					}

					case ControllerType::Atari2600DrivingController: {
						auto* driving = static_cast<Atari2600DrivingController*>(controller.get());
						uint8_t nibble = driving->GetDirectionNibble();
						if (port == 0) {
							swcha = (uint8_t)((swcha & 0x0f) | (nibble << 4));
							_fireP0 = driving->GetFireState();
						} else if (port == 1) {
							swcha = (uint8_t)((swcha & 0xf0) | nibble);
							_fireP1 = driving->GetFireState();
						}
						break;
					}

					case ControllerType::Atari2600BoosterGrip: {
						auto* grip = static_cast<Atari2600BoosterGrip*>(controller.get());
						uint8_t nibble = grip->GetDirectionNibble();
						if (port == 0) {
							swcha = (uint8_t)((swcha & 0x0f) | (nibble << 4));
							_fireP0 = grip->GetFireState();
							_inpt[0] = grip->GetBoosterState();
							_inpt[1] = grip->GetTriggerState();
						} else if (port == 1) {
							swcha = (uint8_t)((swcha & 0xf0) | nibble);
							_fireP1 = grip->GetFireState();
							_inpt[2] = grip->GetBoosterState();
							_inpt[3] = grip->GetTriggerState();
						}
						break;
					}

					default:
						break;
				}
			}
			_swcha = swcha;

			// Build SWCHB from config
			// Bit 0: Reset (1=not pressed), Bit 1: Select (1=not pressed)
			// Bit 3: P0 Difficulty (1=B/Amateur, 0=A/Pro)
			// Bit 6: P1 Difficulty (1=B/Amateur, 0=A/Pro)
			// Bit 7: Color/BW (1=Color, 0=BW)
			Atari2600Config& cfg = _emu->GetSettings()->GetAtari2600Config();
			uint8_t swchb = 0x37; // Reset=1, Select=1, unused bits=1
			if (cfg.P0DifficultyB) swchb |= 0x08;
			if (cfg.P1DifficultyB) swchb |= 0x40;
			if (cfg.ColorMode) swchb |= 0x80;

			_swchb = swchb;
		}

		/// Returns INPT0-3 state for TIA read. Replaces hardcoded 0x80.
		/// For keypads, scans column based on current SWCHA row select.
		uint8_t GetInpt(uint8_t index) {
			if (index > 3) return 0x80;

			// Check if a keypad is connected on the corresponding port
			uint8_t keypadPort = (index < 2) ? 0 : 1; // INPT0-1 → port 0, INPT2-3 → port 1
			for (shared_ptr<BaseControlDevice>& controller : _controlDevices) {
				if (controller->GetControllerType() == ControllerType::Atari2600Keypad &&
					controller->GetPort() == keypadPort) {
					auto* keypad = static_cast<Atari2600Keypad*>(controller.get());
					// Determine which row is selected from SWCHA
					uint8_t portNibble = (keypadPort == 0) ? ((_swcha >> 4) & 0x0f) : (_swcha & 0x0f);
					// Find which row has its bit pulled low (active-low scan)
					uint8_t colIndex = index - (keypadPort * 2); // 0 or 1 within port
					// Note: keypad only uses 3 columns, but INPT0-3 only gives us
					// 2 per port (INPT0/1 for port 0, INPT2/3 for port 1)
					// Actual 2600 hardware: Port 0 keypad uses INPT0/1 for cols,
					// port 1 keypad uses INPT2/3 for cols
					for (uint8_t row = 0; row < 4; row++) {
						if (!(portNibble & (1 << row))) {
							// This row is selected (active low)
							return keypad->GetColumnState(row, colIndex);
						}
					}
					return 0x80; // No row selected
				}
			}

			return _inpt[index];
		}

		[[nodiscard]] uint8_t GetSwcha() const { return _swcha; }
		[[nodiscard]] uint8_t GetSwchb() const { return _swchb; }
		[[nodiscard]] uint8_t GetFireP0() const { return _fireP0; }
		[[nodiscard]] uint8_t GetFireP1() const { return _fireP1; }

		void Serialize(Serializer& s) override {
			BaseControlManager::Serialize(s);
			SV(_swcha);
			SV(_swchb);
			SV(_fireP0);
			SV(_fireP1);
			SVArray(_inpt, 4);
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

	_emu->RegisterMemory(MemoryType::Atari2600PrgRom, _mapper->GetRomData(), _mapper->GetRomSize());
	_emu->RegisterMemory(MemoryType::Atari2600Ram, _riot->GetRamData(), Atari2600Riot::RamSize);

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

	// Update controller devices and poll input BEFORE executing the frame
	if (_controlManager) {
		_controlManager->UpdateControlDevices();
		_controlManager->UpdateInputState();

		// Wire input state to hardware
		auto* ctrlMgr = static_cast<Atari2600ControlManager*>(_controlManager.get());
		Atari2600RiotState riotState = _riot->GetState();
		riotState.PortAInput = ctrlMgr->GetSwcha();
		riotState.PortBInput = ctrlMgr->GetSwchb();
		_riot->SetState(riotState);
		_tia->SetFireButtonState(ctrlMgr->GetFireP0(), ctrlMgr->GetFireP1());
		_tia->SetInptState(
			ctrlMgr->GetInpt(0), ctrlMgr->GetInpt(1),
			ctrlMgr->GetInpt(2), ctrlMgr->GetInpt(3)
		);
	}

	_tia->BeginFrameCapture();
	uint64_t startCycles = _cpu->GetCycleCount();
	StepCpuCycles(CpuCyclesPerFrame);
	Atari2600TiaState tiaState = _tia->GetState();
	_lastFrameSummary.FrameCount = tiaState.FrameCount;
	_lastFrameSummary.CpuCyclesThisFrame = (uint32_t)(_cpu->GetCycleCount() - startCycles);
	_lastFrameSummary.ScanlineAtFrameEnd = tiaState.Scanline;
	_lastFrameSummary.ColorClockAtFrameEnd = tiaState.ColorClock;
	RenderDebugFrame();
	if (_controlManager) {
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

uint8_t Atari2600Console::DebugReadCartridge(uint16_t addr) {
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

Atari2600ScanlineRenderState Atari2600Console::DebugGetScanlineRenderState(uint32_t scanline) const {
	return _tia->GetScanlineRenderState(scanline);
}

uint8_t Atari2600Console::DebugGetMapperBankIndex() const {
	return _mapper ? _mapper->GetActiveBank() : 0;
}

string Atari2600Console::DebugGetMapperMode() const {
	return _mapper ? _mapper->GetModeName() : "none";
}

Atari2600CpuState Atari2600Console::GetCpuState() const {
	Atari2600CpuState state;
	uint16_t pc = 0;
	uint64_t cycles = 0;
	uint8_t a = 0, x = 0, y = 0, sp = 0, ps = 0, remaining = 0;
	_cpu->ExportState(pc, cycles, a, x, y, sp, ps, remaining);
	state.PC = pc;
	state.CycleCount = cycles;
	state.A = a;
	state.X = x;
	state.Y = y;
	state.SP = sp;
	state.PS = ps;
	state.IRQFlag = 0;
	state.NmiFlag = false;
	return state;
}

void Atari2600Console::SetCpuState(const Atari2600CpuState& state) {
	_cpu->ImportState(state.PC, state.CycleCount, state.A, state.X, state.Y, state.SP, state.PS, 0);
}

uint8_t Atari2600Console::DebugRead(uint16_t addr) {
	if (!_bus) {
		return 0xFF;
	}
	return _bus->Read(addr & 0x1FFF);
}

void Atari2600Console::DebugWrite(uint16_t addr, uint8_t value) {
	if (_bus) {
		_bus->Write(addr & 0x1FFF, value);
	}
}

uint32_t Atari2600Console::GetFrameCount() const {
	return _tia ? _tia->GetState().FrameCount : 0;
}

uint32_t Atari2600Console::GetCurrentScanline() const {
	return _tia ? _tia->GetState().Scanline : 0;
}

uint32_t Atari2600Console::GetCurrentColorClock() const {
	return _tia ? _tia->GetState().ColorClock : 0;
}

uint32_t* Atari2600Console::GetFrameBuffer() {
	if (_frameBuffer.empty()) {
		return nullptr;
	}
	// Frame buffer is stored as uint16_t (RGB565), but the interface
	// expects uint32_t*. Reinterpret the underlying storage.
	return reinterpret_cast<uint32_t*>(_frameBuffer.data());
}

void Atari2600Console::RenderDebugFrame() {
	// Standard Stella NTSC TIA palette — 128 colors as 0x00RRGGBB
	// 16 hues × 8 luminance levels, indexed by (tiaColor >> 1)
	// Source: Stella emulator standard NTSC palette
	static constexpr uint32_t ntscPalette[128] = {
		// Hue 0  — Gray
		0x000000, 0x404040, 0x6c6c6c, 0x909090, 0xb0b0b0, 0xc8c8c8, 0xdcdcdc, 0xf4f4f4,
		// Hue 1  — Gold
		0x444400, 0x646410, 0x848424, 0xa0a034, 0xb8b840, 0xd0d050, 0xe8e85c, 0xfcfc68,
		// Hue 2  — Orange
		0x702800, 0x844414, 0x985c28, 0xac783c, 0xbc8c4c, 0xcca05c, 0xdcb468, 0xecc878,
		// Hue 3  — Red-Orange
		0x841800, 0x983418, 0xac5030, 0xc06848, 0xd0805c, 0xe09470, 0xeca880, 0xfcbc94,
		// Hue 4  — Red
		0x880000, 0x9c2020, 0xb03c3c, 0xc05858, 0xd07070, 0xe08888, 0xeca0a0, 0xfcb4b4,
		// Hue 5  — Purple-Red
		0x78005c, 0x8c2074, 0xa03c88, 0xb0589c, 0xc070b0, 0xd084c0, 0xdc9cd0, 0xecb0e0,
		// Hue 6  — Purple
		0x480078, 0x602090, 0x783ca4, 0x8c58b8, 0xa070cc, 0xb484dc, 0xc49cec, 0xd4b0fc,
		// Hue 7  — Blue-Purple
		0x140084, 0x302098, 0x4c3cac, 0x6858c0, 0x7c70d0, 0x9488e0, 0xa8a0ec, 0xbcb4fc,
		// Hue 8  — Blue
		0x000088, 0x1c209c, 0x3840b0, 0x505cc0, 0x6874d0, 0x7c8ce0, 0x90a4ec, 0xa4b8fc,
		// Hue 9  — Light Blue
		0x00187c, 0x1c3890, 0x3854a8, 0x5070bc, 0x6888cc, 0x7c9cdc, 0x90b4ec, 0xa4c8fc,
		// Hue 10 — Cyan
		0x002c5c, 0x1c4c78, 0x386890, 0x5084ac, 0x689cc0, 0x7cb4d4, 0x90cce8, 0xa4e0fc,
		// Hue 11 — Teal
		0x003c2c, 0x1c5c48, 0x387c64, 0x509c80, 0x68b494, 0x7cd0ac, 0x90e4c0, 0xa4fcd4,
		// Hue 12 — Green
		0x003c00, 0x205c20, 0x407c40, 0x5c9c5c, 0x74b474, 0x8cd08c, 0xa4e4a4, 0xb8fcb8,
		// Hue 13 — Yellow-Green
		0x143800, 0x345c1c, 0x507c38, 0x6c9850, 0x84b468, 0x9ccc7c, 0xb4e490, 0xc8fca4,
		// Hue 14 — Yellow-Green (warm)
		0x2c3000, 0x4c501c, 0x687034, 0x848c4c, 0x9ca864, 0xb4c078, 0xccd488, 0xe0ec9c,
		// Hue 15 — Dark Yellow
		0x442800, 0x644818, 0x846830, 0xa08444, 0xb89c58, 0xd0b46c, 0xe8cc7c, 0xfce08c,
	};

	// Convert TIA color index to RGB565 via NTSC palette lookup
	auto toRgb565 = [](uint8_t tiaColor) -> uint16_t {
		uint32_t c = ntscPalette[(tiaColor >> 1) & 0x7f];
		uint8_t r = (uint8_t)((c >> 16) & 0xff);
		uint8_t g = (uint8_t)((c >> 8) & 0xff);
		uint8_t b = (uint8_t)(c & 0xff);
		return (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
	};

	auto getPlayfieldBit = [](const Atari2600ScanlineRenderState& scanlineState, uint32_t index) {
		index %= 20;
		if (index < 4) {
			return ((scanlineState.Playfield0 >> (4 + index)) & 0x01) != 0;
		}
		if (index < 12) {
			return ((scanlineState.Playfield1 >> (11 - index)) & 0x01) != 0;
		}
		return ((scanlineState.Playfield2 >> (19 - index)) & 0x01) != 0;
	};

	auto getCopyOffsets = [](uint8_t nusiz, std::array<uint8_t, 3>& offsets) {
		switch (nusiz & 0x07) {
			case 1:
				offsets = {0, 16, 0};
				return 2u;
			case 2:
				offsets = {0, 32, 0};
				return 2u;
			case 3:
				offsets = {0, 16, 32};
				return 3u;
			case 4:
				offsets = {0, 64, 0};
				return 2u;
			case 6:
				offsets = {0, 32, 64};
				return 3u;
			default:
				offsets = {0, 0, 0};
				return 1u;
		}
	};

	auto getPlayerScale = [](uint8_t nusiz) {
		switch (nusiz & 0x07) {
			case 5: return 2u;
			case 7: return 4u;
			default: return 1u;
		}
	};

	auto getMissileWidth = [](uint8_t nusiz) {
		return 1u << ((nusiz >> 4) & 0x03);
	};

	auto isPlayerPixel = [&](uint8_t graphics, bool reflect, uint8_t nusiz, uint32_t x, uint32_t originX) {
		std::array<uint8_t, 3> offsets = {};
		uint32_t copyCount = getCopyOffsets(nusiz, offsets);
		uint32_t pixelScale = getPlayerScale(nusiz);
		uint32_t spriteWidth = 8 * pixelScale;

		for (uint32_t i = 0; i < copyCount; i++) {
			uint32_t copyOrigin = (originX + offsets[i]) % ScreenWidth;
			uint32_t relativeX = (x + ScreenWidth - copyOrigin) % ScreenWidth;
			if (relativeX < spriteWidth) {
				uint32_t bitIndex = relativeX / pixelScale;
				uint32_t bit = reflect ? bitIndex : (7 - bitIndex);
				if (((graphics >> bit) & 0x01) != 0) {
					return true;
				}
			}
		}
		return false;
	};

	auto isMissilePixel = [&](bool enabled, uint8_t nusiz, uint32_t x, uint32_t originX) {
		if (!enabled) {
			return false;
		}
		std::array<uint8_t, 3> offsets = {};
		uint32_t copyCount = getCopyOffsets(nusiz, offsets);
		uint32_t missileWidth = getMissileWidth(nusiz);

		for (uint32_t i = 0; i < copyCount; i++) {
			uint32_t copyOrigin = (originX + offsets[i]) % ScreenWidth;
			uint32_t relativeX = (x + ScreenWidth - copyOrigin) % ScreenWidth;
			if (relativeX < missileWidth) {
				return true;
			}
		}
		return false;
	};

	Atari2600Config& layerCfg = _emu->GetSettings()->GetAtari2600Config();

	for (uint32_t y = 0; y < ScreenHeight; y++) {
		Atari2600ScanlineRenderState scanlineState = _tia->GetScanlineRenderState(y);
		uint16_t colorBackground = toRgb565(scanlineState.ColorBackground);
		uint16_t colorPlayfield = toRgb565(scanlineState.ColorPlayfield);
		uint16_t colorPlayer0 = toRgb565(scanlineState.ColorPlayer0);
		uint16_t colorPlayer1 = toRgb565(scanlineState.ColorPlayer1);

		for (uint32_t x = 0; x < ScreenWidth; x++) {
			if (_tia->IsHmoveBlankScanline(y) && x < 8) {
				_frameBuffer[y * ScreenWidth + x] = 0;
				continue;
			}

			uint32_t coarsePixel = (x / 4);
			uint32_t halfIndex = coarsePixel % 20;
			if (coarsePixel >= 20 && scanlineState.PlayfieldReflect) {
				halfIndex = 19 - halfIndex;
			}

			bool playfieldPixel = getPlayfieldBit(scanlineState, halfIndex);
			bool missile0Pixel = isMissilePixel(scanlineState.Missile0Enabled, scanlineState.Nusiz0, x, scanlineState.Missile0X);
			bool missile1Pixel = isMissilePixel(scanlineState.Missile1Enabled, scanlineState.Nusiz1, x, scanlineState.Missile1X);
			uint32_t ballOffset = (x + ScreenWidth - scanlineState.BallX) % ScreenWidth;
			bool ballPixel = scanlineState.BallEnabled && ballOffset < scanlineState.BallSize;
			bool player0Pixel = isPlayerPixel(scanlineState.Player0Graphics, scanlineState.Player0Reflect, scanlineState.Nusiz0, x, scanlineState.Player0X);
			bool player1Pixel = isPlayerPixel(scanlineState.Player1Graphics, scanlineState.Player1Reflect, scanlineState.Nusiz1, x, scanlineState.Player1X);
			_tia->LatchCollisionPixel(missile0Pixel, missile1Pixel, player0Pixel, player1Pixel, ballPixel, playfieldPixel);

			// Apply layer visibility toggles (after collision detection to preserve accuracy)
			if (layerCfg.HidePlayfield) playfieldPixel = false;
			if (layerCfg.HidePlayer0) player0Pixel = false;
			if (layerCfg.HidePlayer1) player1Pixel = false;
			if (layerCfg.HideMissile0) missile0Pixel = false;
			if (layerCfg.HideMissile1) missile1Pixel = false;
			if (layerCfg.HideBall) ballPixel = false;

			uint16_t playfieldPixelColor = colorPlayfield;
			if (scanlineState.PlayfieldScoreMode) {
				playfieldPixelColor = x < (ScreenWidth / 2) ? colorPlayer0 : colorPlayer1;
			}

			uint16_t pixel = colorBackground;
			if (scanlineState.PlayfieldPriority) {
				if (missile0Pixel || player0Pixel) {
					pixel = colorPlayer0;
				}
				if (missile1Pixel || player1Pixel) {
					pixel = colorPlayer1;
				}
				if (playfieldPixel) {
					pixel = playfieldPixelColor;
				}
				if (ballPixel) {
					pixel = colorPlayfield;
				}
			} else {
				if (playfieldPixel) {
					pixel = playfieldPixelColor;
				}
				if (ballPixel) {
					pixel = colorPlayfield;
				}
				if (missile0Pixel || player0Pixel) {
					pixel = colorPlayer0;
				}
				if (missile1Pixel || player1Pixel) {
					pixel = colorPlayer1;
				}
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
	return {CpuType::Atari2600};
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
	return RomFormat::Atari2600;
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
	if (relAddress.Address < 0) {
		return {-1, MemoryType::None};
	}

	if (relAddress.Type == MemoryType::Atari2600PrgRom || relAddress.Type == MemoryType::Atari2600Ram || relAddress.Type == MemoryType::Atari2600TiaRegisters) {
		return relAddress;
	}

	if (relAddress.Type != MemoryType::Atari2600Memory && relAddress.Type != MemoryType::NesMemory) {
		return relAddress;
	}

	uint16_t addr = (uint16_t)relAddress.Address & 0x1FFF;
	if ((addr & 0x1000) == 0x1000) {
		int32_t romOffset = -1;
		if (_mapper && _mapper->TryGetRomOffset(addr, romOffset)) {
			return {romOffset, MemoryType::Atari2600PrgRom};
		}
		return {-1, MemoryType::Atari2600PrgRom};
	}

	if ((addr & 0x1080) == 0x0080) {
		if ((addr & 0x0200) == 0) {
			return {(int32_t)(addr & 0x7F), MemoryType::Atari2600Ram};
		}
		return {(int32_t)(addr & 0x07), MemoryType::Atari2600Ram};
	}

	if ((addr & 0x1080) == 0x0000) {
		return {(int32_t)(addr & 0x3F), MemoryType::Atari2600TiaRegisters};
	}

	return {(int32_t)addr, MemoryType::Atari2600Memory};
}

AddressInfo Atari2600Console::GetPcAbsoluteAddress() {
	int32_t romOffset = -1;
	if (_mapper && _mapper->TryGetRomOffset(_cpu->GetProgramCounter(), romOffset)) {
		return {romOffset, MemoryType::Atari2600PrgRom};
	}
	return {-1, MemoryType::Atari2600PrgRom};
}

AddressInfo Atari2600Console::GetRelativeAddress(AddressInfo& absAddress, CpuType cpuType) {
	(void)cpuType;

	if (absAddress.Type == MemoryType::Atari2600PrgRom) {
		if (absAddress.Address < 0) {
			return {-1, MemoryType::Atari2600Memory};
		}
		return {(int32_t)(0x1000 | (absAddress.Address & 0x0FFF)), MemoryType::Atari2600Memory};
	}

	if (absAddress.Type == MemoryType::Atari2600Ram) {
		return {(int32_t)(0x0080 | (absAddress.Address & 0x7F)), MemoryType::Atari2600Memory};
	}

	if (absAddress.Type == MemoryType::Atari2600TiaRegisters) {
		return {(int32_t)(absAddress.Address & 0x3F), MemoryType::Atari2600Memory};
	}

	return absAddress;
}

void Atari2600Console::GetConsoleState(BaseState& state, ConsoleType consoleType) {
	Atari2600State& s = reinterpret_cast<Atari2600State&>(state);
	s.Cpu = GetCpuState();
	s.Tia = _tia ? _tia->GetState() : Atari2600TiaState{};
	s.Riot = _riot ? _riot->GetState() : Atari2600RiotState{};
}

void Atari2600Console::Serialize(Serializer& s) {
	uint16_t cpuProgramCounter = 0;
	uint64_t cpuCycleCount = 0;
	uint8_t cpuA = 0;
	uint8_t cpuX = 0;
	uint8_t cpuY = 0;
	uint8_t cpuSp = 0;
	uint8_t cpuStatus = 0;
	uint8_t cpuRemainingCycles = 0;

	Atari2600RiotState riotState = {};
	Atari2600TiaState tiaState = {};

	uint8_t mapperActiveBank = 0;
	uint8_t mapperSegmentBank0 = 0;
	uint8_t mapperSegmentBank1 = 0;
	uint8_t mapperSegmentBank2 = 0;
	uint8_t mapperFixedSegmentBank = 0;

	if (s.IsSaving()) {
		_cpu->ExportState(cpuProgramCounter, cpuCycleCount, cpuA, cpuX, cpuY, cpuSp, cpuStatus, cpuRemainingCycles);
		riotState = _riot->GetState();
		tiaState = _tia->GetState();
		_mapper->ExportState(mapperActiveBank, mapperSegmentBank0, mapperSegmentBank1, mapperSegmentBank2, mapperFixedSegmentBank);
	}

	SV(_romLoaded);
	SV(_lastFrameSummary.FrameCount);
	SV(_lastFrameSummary.CpuCyclesThisFrame);
	SV(_lastFrameSummary.ScanlineAtFrameEnd);
	SV(_lastFrameSummary.ColorClockAtFrameEnd);

	SV(cpuProgramCounter);
	SV(cpuCycleCount);
	SV(cpuA);
	SV(cpuX);
	SV(cpuY);
	SV(cpuSp);
	SV(cpuStatus);
	SV(cpuRemainingCycles);

	SV(riotState.PortA);
	SV(riotState.PortB);
	SV(riotState.PortADirection);
	SV(riotState.PortBDirection);
	SV(riotState.PortAInput);
	SV(riotState.PortBInput);
	SV(riotState.Timer);
	SV(riotState.TimerDivider);
	SV(riotState.TimerDividerCounter);
	SV(riotState.TimerUnderflow);
	SV(riotState.InterruptFlag);
	SV(riotState.InterruptEdgeCount);
	SV(riotState.CpuCycles);

	SV(tiaState.FrameCount);
	SV(tiaState.Scanline);
	SV(tiaState.ColorClock);
	SV(tiaState.WsyncHold);
	SV(tiaState.WsyncCount);
	SV(tiaState.HmovePending);
	SV(tiaState.HmoveDelayToNextScanline);
	SV(tiaState.HmoveStrobeCount);
	SV(tiaState.HmoveApplyCount);
	SV(tiaState.ColorBackground);
	SV(tiaState.ColorPlayfield);
	SV(tiaState.ColorPlayer0);
	SV(tiaState.ColorPlayer1);
	SV(tiaState.Playfield0);
	SV(tiaState.Playfield1);
	SV(tiaState.Playfield2);
	SV(tiaState.PlayfieldReflect);
	SV(tiaState.PlayfieldScoreMode);
	SV(tiaState.PlayfieldPriority);
	SV(tiaState.BallSize);
	SV(tiaState.Nusiz0);
	SV(tiaState.Nusiz1);
	SV(tiaState.Player0Graphics);
	SV(tiaState.Player1Graphics);
	SV(tiaState.Player0Reflect);
	SV(tiaState.Player1Reflect);
	SV(tiaState.Missile0ResetToPlayer);
	SV(tiaState.Missile1ResetToPlayer);
	SV(tiaState.Missile0Enabled);
	SV(tiaState.Missile1Enabled);
	SV(tiaState.BallEnabled);
	SV(tiaState.VdelPlayer0);
	SV(tiaState.VdelPlayer1);
	SV(tiaState.VdelBall);
	SV(tiaState.DelayedPlayer0Graphics);
	SV(tiaState.DelayedPlayer1Graphics);
	SV(tiaState.DelayedBallEnabled);
	SV(tiaState.Player0X);
	SV(tiaState.Player1X);
	SV(tiaState.Missile0X);
	SV(tiaState.Missile1X);
	SV(tiaState.BallX);
	SV(tiaState.MotionPlayer0);
	SV(tiaState.MotionPlayer1);
	SV(tiaState.MotionMissile0);
	SV(tiaState.MotionMissile1);
	SV(tiaState.MotionBall);
	SV(tiaState.CollisionCxm0p);
	SV(tiaState.CollisionCxm1p);
	SV(tiaState.CollisionCxp0fb);
	SV(tiaState.CollisionCxp1fb);
	SV(tiaState.CollisionCxm0fb);
	SV(tiaState.CollisionCxm1fb);
	SV(tiaState.CollisionCxblpf);
	SV(tiaState.CollisionCxppmm);
	SV(tiaState.RenderRevision);
	SV(tiaState.AudioControl0);
	SV(tiaState.AudioControl1);
	SV(tiaState.AudioFrequency0);
	SV(tiaState.AudioFrequency1);
	SV(tiaState.AudioVolume0);
	SV(tiaState.AudioVolume1);
	SV(tiaState.AudioCounter0);
	SV(tiaState.AudioCounter1);
	SV(tiaState.AudioPhase0);
	SV(tiaState.AudioPhase1);
	SV(tiaState.LastMixedSample);
	SV(tiaState.AudioMixAccumulator);
	SV(tiaState.AudioSampleCount);
	SV(tiaState.AudioRevision);
	SV(tiaState.TotalColorClocks);

	SV(mapperActiveBank);
	SV(mapperSegmentBank0);
	SV(mapperSegmentBank1);
	SV(mapperSegmentBank2);
	SV(mapperFixedSegmentBank);

	if (!s.IsSaving()) {
		_cpu->ImportState(cpuProgramCounter, cpuCycleCount, cpuA, cpuX, cpuY, cpuSp, cpuStatus, cpuRemainingCycles);
		_riot->SetState(riotState);
		_tia->SetState(tiaState);
		_mapper->ImportState(mapperActiveBank, mapperSegmentBank0, mapperSegmentBank1, mapperSegmentBank2, mapperFixedSegmentBank);
	}
}
