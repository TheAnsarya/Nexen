#pragma once
#include "pch.h"
#include "Atari2600/Atari2600Types.h"

class Atari2600Tia {
private:
	Atari2600TiaState _state = {};
	std::array<uint8_t, Atari2600Constants::ScreenHeight> _hmoveBlankScanlines = {};
	std::array<Atari2600ScanlineRenderState, Atari2600Constants::ScreenHeight> _scanlineRenderState = {};
	vector<int16_t> _audioBuffer;
	static constexpr uint32_t HmoveBlankColorClocks = 8;
	static constexpr uint32_t HmoveLateCycleThreshold = 73;
	static constexpr uint32_t FrameAudioSampleCapacity = Atari2600Constants::CpuCyclesPerFrame;

	uint8_t _inputPort[4] = { 0x80, 0x80, 0x80, 0x80 };
	uint8_t _inputPort4 = 0x80;
	uint8_t _inputPort5 = 0x80;

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
		int32_t wrapped = value % (int32_t)Atari2600Constants::ScreenWidth;
		if (wrapped < 0) {
			wrapped += Atari2600Constants::ScreenWidth;
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
		if (scanline < Atari2600Constants::ScreenHeight) {
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
		auto toSignedSample = [](uint8_t sample5bit) {
			int32_t centered = (int32_t)sample5bit - 15;
			int32_t scaled = centered * 2048;
			return (int16_t)std::clamp(scaled, (int32_t)INT16_MIN, (int32_t)INT16_MAX);
		};

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

		int16_t outputSample = toSignedSample(_state.LastMixedSample);
		_audioBuffer.push_back(outputSample);
		_audioBuffer.push_back(outputSample);
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
	Atari2600Tia() {
		_audioBuffer.reserve(FrameAudioSampleCapacity * 2);
	}

	void Reset() {
		_state = {};
		_hmoveBlankScanlines.fill(0);
		_scanlineRenderState.fill({});
		_audioBuffer.clear();
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
		_audioBuffer.clear();
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
				if (_state.Scanline < Atari2600Constants::ScreenHeight) {
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
		switch (addr & 0x0f) {
			case 0x00: return _state.CollisionCxm0p;
			case 0x01: return _state.CollisionCxm1p;
			case 0x02: return _state.CollisionCxp0fb;
			case 0x03: return _state.CollisionCxp1fb;
			case 0x04: return _state.CollisionCxm0fb;
			case 0x05: return _state.CollisionCxm1fb;
			case 0x06: return _state.CollisionCxblpf;
			case 0x07: return _state.CollisionCxppmm;
			case 0x08: return _inputPort[0];
			case 0x09: return _inputPort[1];
			case 0x0a: return _inputPort[2];
			case 0x0b: return _inputPort[3];
			case 0x0c: return _inputPort4;
			case 0x0d: return _inputPort5;
			default: return 0;
		}
	}

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
				_state.Player0X = (uint8_t)(_state.ColorClock % Atari2600Constants::ScreenWidth);
				SyncLockedMissilesToPlayers();
				MarkRenderDirty();
				CaptureCurrentScanlineState();
				break;

			case 0x11:
				_state.Player1X = (uint8_t)(_state.ColorClock % Atari2600Constants::ScreenWidth);
				SyncLockedMissilesToPlayers();
				MarkRenderDirty();
				CaptureCurrentScanlineState();
				break;

			case 0x12:
				_state.Missile0X = (uint8_t)(_state.ColorClock % Atari2600Constants::ScreenWidth);
				MarkRenderDirty();
				CaptureCurrentScanlineState();
				break;

			case 0x13:
				_state.Missile1X = (uint8_t)(_state.ColorClock % Atari2600Constants::ScreenWidth);
				MarkRenderDirty();
				CaptureCurrentScanlineState();
				break;

			case 0x14:
				_state.BallX = (uint8_t)(_state.ColorClock % Atari2600Constants::ScreenWidth);
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
		_audioBuffer.clear();
	}

	[[nodiscard]] const vector<int16_t>& GetAudioBuffer() const {
		return _audioBuffer;
	}
};
