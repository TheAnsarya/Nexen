#include "pch.h"
#include "Atari2600/Atari2600Console.h"
#include "Atari2600/Atari2600DefaultVideoFilter.h"
#include "Shared/BaseControlManager.h"
#include "Shared/CpuType.h"
#include "Shared/Emulator.h"
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

		uint8_t ReadRegister(uint16_t addr) const {
			if (!(addr & 0x0200)) {
				return _ram[addr & 0x7F];
			}
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

		[[nodiscard]] static uint8_t WrapHorizontal(int32_t value) {
			int32_t wrapped = value % (int32_t)Atari2600Console::ScreenWidth;
			if (wrapped < 0) {
				wrapped += Atari2600Console::ScreenWidth;
			}
			return (uint8_t)wrapped;
		}

		void ApplyHmoveDisplacements() {
			_state.Player0X = WrapHorizontal((int32_t)_state.Player0X + _state.MotionPlayer0);
			_state.Player1X = WrapHorizontal((int32_t)_state.Player1X + _state.MotionPlayer1);
			_state.Missile0X = WrapHorizontal((int32_t)_state.Missile0X + _state.MotionMissile0);
			_state.Missile1X = WrapHorizontal((int32_t)_state.Missile1X + _state.MotionMissile1);
			_state.BallX = WrapHorizontal((int32_t)_state.BallX + _state.MotionBall);
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
			scanline.Nusiz0 = _state.Nusiz0;
			scanline.Nusiz1 = _state.Nusiz1;
			scanline.Player0Graphics = _state.VdelPlayer0 ? _state.DelayedPlayer0Graphics : _state.Player0Graphics;
			scanline.Player1Graphics = _state.VdelPlayer1 ? _state.DelayedPlayer1Graphics : _state.Player1Graphics;
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
			switch (NormalizeRegisterAddress(addr)) {
				case 0x00: return _state.CollisionCxm0p;
				case 0x01: return _state.CollisionCxm1p;
				case 0x02: return _state.CollisionCxp0fb;
				case 0x03: return _state.CollisionCxp1fb;
				case 0x04: return _state.CollisionCxm0fb;
				case 0x05: return _state.CollisionCxm1fb;
				case 0x06: return _state.CollisionCxblpf;
				case 0x07: return _state.CollisionCxppmm;
				case 0x08: return _state.ColorPlayfield;
				case 0x09: return _state.ColorBackground;
				case 0x0A:
					return (uint8_t)((_state.PlayfieldReflect ? 0x01 : 0x00)
						| (_state.PlayfieldScoreMode ? 0x02 : 0x00)
						| (_state.PlayfieldPriority ? 0x04 : 0x00));
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
					MarkRenderDirty();
					CaptureCurrentScanlineState();
					break;

				case 0x11:
					_state.Player1X = (uint8_t)(_state.ColorClock % Atari2600Console::ScreenWidth);
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

Atari2600ScanlineRenderState Atari2600Console::DebugGetScanlineRenderState(uint32_t scanline) const {
	return _tia->GetScanlineRenderState(scanline);
}

uint8_t Atari2600Console::DebugGetMapperBankIndex() const {
	return _mapper ? _mapper->GetActiveBank() : 0;
}

string Atari2600Console::DebugGetMapperMode() const {
	return _mapper ? _mapper->GetModeName() : "none";
}

void Atari2600Console::RenderDebugFrame() {
	auto toRgb565 = [](uint8_t tiaColor) {
		uint8_t hue = (uint8_t)((tiaColor >> 4) & 0x0F);
		uint8_t lum = (uint8_t)(tiaColor & 0x0F);
		uint8_t red = (uint8_t)((hue * 3 + lum) & 0x1F);
		uint8_t green = (uint8_t)((hue * 5 + lum * 2) & 0x3F);
		uint8_t blue = (uint8_t)((hue * 7 + lum) & 0x1F);
		return (uint16_t)((red << 11) | (green << 5) | blue);
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

	auto isPlayerPixel = [&](uint8_t graphics, uint8_t nusiz, uint32_t x, uint32_t originX) {
		std::array<uint8_t, 3> offsets = {};
		uint32_t copyCount = getCopyOffsets(nusiz, offsets);
		uint32_t pixelScale = getPlayerScale(nusiz);
		uint32_t spriteWidth = 8 * pixelScale;

		for (uint32_t i = 0; i < copyCount; i++) {
			uint32_t copyOrigin = (originX + offsets[i]) % ScreenWidth;
			uint32_t relativeX = (x + ScreenWidth - copyOrigin) % ScreenWidth;
			if (relativeX < spriteWidth) {
				uint32_t bit = 7 - (relativeX / pixelScale);
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
			bool ballPixel = scanlineState.BallEnabled && ballOffset < 4;
			bool player0Pixel = isPlayerPixel(scanlineState.Player0Graphics, scanlineState.Nusiz0, x, scanlineState.Player0X);
			bool player1Pixel = isPlayerPixel(scanlineState.Player1Graphics, scanlineState.Nusiz1, x, scanlineState.Player1X);
			_tia->LatchCollisionPixel(missile0Pixel, missile1Pixel, player0Pixel, player1Pixel, ballPixel, playfieldPixel);

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
	return {(int32_t)_cpu->GetProgramCounter(), MemoryType::Atari2600PrgRom};
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
	SV(tiaState.Nusiz0);
	SV(tiaState.Nusiz1);
	SV(tiaState.Player0Graphics);
	SV(tiaState.Player1Graphics);
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
