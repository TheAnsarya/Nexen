#include "pch.h"
#include "ChannelF/ChannelFMemoryManager.h"

ChannelFMemoryManager::ChannelFMemoryManager()
	: _biosRom(BiosSize, 0xff),
	  _cartRom(MaxCartSize, 0xff) {
}

void ChannelFMemoryManager::LoadBios(const uint8_t* data, uint32_t size) {
	if (size > BiosSize) {
		size = BiosSize;
	}
	memcpy(_biosRom.data(), data, size);
}

void ChannelFMemoryManager::LoadCart(const uint8_t* data, uint32_t size) {
	_cartSize = size;
	if (size > MaxCartSize) {
		_cartSize = MaxCartSize;
	}
	memset(_cartRom.data(), 0xff, MaxCartSize);
	memcpy(_cartRom.data(), data, _cartSize);
}

void ChannelFMemoryManager::Reset() {
	memset(_vram, 0, sizeof(_vram));
	memset(_portLatch, 0, sizeof(_portLatch));
	_videoColor = 0;
	_videoX = 0;
	_videoY = 0;
	_soundTone = 0;
	_soundFreq = 0;
	_audioBuffer.clear();
	_audioCounter = 0;
	_audioOutput = false;
	_controller1 = 0xff;
	_controller2 = 0xff;
	_consoleButtons = 0xff;
}

uint8_t ChannelFMemoryManager::ReadMemory(uint16_t addr) const {
	if (addr < BiosSize) {
		return _biosRom[addr];
	}
	// Cartridge ROM at $0800+
	uint32_t cartAddr = addr - BiosSize;
	if (cartAddr < _cartSize) {
		return _cartRom[cartAddr];
	}
	return 0xff;
}

void ChannelFMemoryManager::WriteMemory([[maybe_unused]] uint16_t addr, [[maybe_unused]] uint8_t value) {
	// Channel F has no writable program memory in the standard configuration.
	// Some cartridge mappers might add RAM, but for now this is a no-op.
}

uint8_t ChannelFMemoryManager::ReadPort(uint8_t port) const {
	switch (port) {
		case 0:
			// Port 0: Console buttons
			return _consoleButtons;
		case 1:
			// Port 1: Video/misc status
			return _portLatch[1];
		case 4:
			// Port 4: Right controller
			return _controller2;
		case 5:
			// Port 5: Left controller
			return _controller1;
		default:
			return _portLatch[port];
	}
}

void ChannelFMemoryManager::WritePort(uint8_t port, uint8_t value) {
	_portLatch[port] = value;

	switch (port) {
		case 0:
			// Port 0 write: bits 0-1 = color, bit 5 = sound tone, bit 6 = sound freq
			_videoColor = value & 0x03;
			_soundTone = (value >> 5) & 0x01;
			_soundFreq = (value >> 6) & 0x03;
			break;

		case 1: {
			// Port 1 write: video column and row control
			// Bits 0-6 = column (X), bit 7 = write-enable
			uint8_t col = value & 0x7f;
			if (value & 0x80) {
				// Write pixel at current (col, _videoY) with _videoColor
				if (col < ScreenWidth && _videoY < ScreenHeight) {
					_vram[(uint32_t)_videoY * ScreenWidth + col] = _videoColor;
				}
			}
			_videoX = col;
			break;
		}

		case 4:
			// Port 4: Row latch for video
			_videoY = value & 0x3f;
			break;

		case 5:
			// Port 5 write: sound output
			_soundTone = value & 0x3f;
			break;

		default:
			break;
	}
}

void ChannelFMemoryManager::SetControllerState(uint8_t ctrl1, uint8_t ctrl2, uint8_t console) {
	_controller1 = ctrl1;
	_controller2 = ctrl2;
	_consoleButtons = console;
}

ChannelFVideoState ChannelFMemoryManager::GetVideoState() const {
	ChannelFVideoState state = {};
	state.Color = _videoColor;
	state.X = _videoX;
	state.Y = _videoY;
	return state;
}

ChannelFAudioState ChannelFMemoryManager::GetAudioState() const {
	ChannelFAudioState state = {};
	state.Tone = _soundTone;
	state.Frequency = _soundFreq;
	state.SoundEnabled = (_soundTone != 0);
	return state;
}

ChannelFPortState ChannelFMemoryManager::GetPortState() const {
	ChannelFPortState state = {};
	state.Port0 = _portLatch[0];
	state.Port1 = _portLatch[1];
	state.Port4 = _controller2;
	state.Port5 = _controller1;
	return state;
}

void ChannelFMemoryManager::SetVideoState(const ChannelFVideoState& state) {
	_videoColor = state.Color;
	_videoX = state.X;
	_videoY = state.Y;
}

void ChannelFMemoryManager::SetAudioState(const ChannelFAudioState& state) {
	_soundTone = state.Tone;
	_soundFreq = state.Frequency;
}

void ChannelFMemoryManager::SetPortState(const ChannelFPortState& state) {
	_portLatch[0] = state.Port0;
	_portLatch[1] = state.Port1;
	_controller2 = state.Port4;
	_controller1 = state.Port5;
}

void ChannelFMemoryManager::BeginFrameCapture() {
	_audioBuffer.clear();
}

void ChannelFMemoryManager::StepAudio() {
	// Channel F PSG: single-channel square wave
	// _soundTone (6-bit from port 5 bits 0-5) controls the period divider
	// When _soundTone == 0: silence
	// When _soundTone > 0: square wave with period = _soundTone * 2 CPU cycles
	int16_t sample = 0;
	if (_soundTone != 0) {
		_audioCounter++;
		if (_audioCounter >= _soundTone) {
			_audioCounter = 0;
			_audioOutput = !_audioOutput;
		}
		// Square wave: +/- amplitude (~25% of int16_t range for reasonable volume)
		sample = _audioOutput ? (int16_t)8192 : (int16_t)-8192;
	} else {
		_audioCounter = 0;
		_audioOutput = false;
	}
	// Stereo: push left + right (mono duplicated)
	_audioBuffer.push_back(sample);
	_audioBuffer.push_back(sample);
}
