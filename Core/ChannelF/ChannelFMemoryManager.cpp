#include "pch.h"
#include "ChannelF/ChannelFMemoryManager.h"
#include <algorithm>
#include <limits>

namespace {
	constexpr uint32_t CartBankWindowSize = 0x2000;
	constexpr uint32_t ToneFrequencies[4] = {
		0,
		1000,
		500,
		120
	};

	bool HasSignature(const uint8_t* data, uint32_t size, const char* signature) {
		if (data == nullptr || signature == nullptr) {
			return false;
		}

		uint32_t sigLen = (uint32_t)strlen(signature);
		if (sigLen == 0 || size < sigLen) {
			return false;
		}

		for (uint32_t i = 0; i + sigLen <= size; i++) {
			if (memcmp(data + i, signature, sigLen) == 0) {
				return true;
			}
		}
		return false;
	}

	ChannelFMemoryManager::CartBoardType DetectBoardType(const uint8_t* data, uint32_t size) {
		if (size > CartBankWindowSize) {
			return ChannelFMemoryManager::CartBoardType::BankedRom;
		}

		if (HasSignature(data, size, "SCHACH")
			|| HasSignature(data, size, "CHESS")
			|| HasSignature(data, size, "VIDEOCART-10")
			|| HasSignature(data, size, "MAZE")) {
			return ChannelFMemoryManager::CartBoardType::RomWithRam;
		}

		return ChannelFMemoryManager::CartBoardType::StandardRom;
	}
}

ChannelFMemoryManager::ChannelFMemoryManager()
	: _biosRom(BiosSize, 0xff),
	  _cartRom(MaxCartSize, 0xff),
	  _cartRam(CartRamSize, 0x00) {
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

	_cartBoardType = DetectBoardType(data, _cartSize);
	_activeCartBank = 0;
	if (_cartBoardType != CartBoardType::RomWithRam) {
		memset(_cartRam.data(), 0x00, _cartRam.size());
	}
}

void ChannelFMemoryManager::Reset() {
	memset(_vram, 0, sizeof(_vram));
	memset(_portLatch, 0, sizeof(_portLatch));
	_videoColor = 0;
	_backgroundColor = 0;
	_videoX = 0;
	_videoY = 0;
	_pendingVideoWrites.clear();
	_videoCycle = 0;
	_soundToneSelect = 0;
	_soundVolume = 0x0f;
	_audioBuffer.clear();
	_audioCounter = 0;
	_audioOutput = false;
	_activeCartBank = 0;
	_controller1 = 0xff;
	_controller2 = 0xff;
	_consoleButtons = 0xff;
	_interruptVectorHigh = 0;
	_interruptVectorLow = 0;
	_smiTimerLatch = 0;
	_smiTimerCounter = 0;
	_smiTimerControl = 0;
	_smiTimerEnabled = false;
	if (_onSmiIrqLineChanged) {
		_onSmiIrqLineChanged(false);
	}
	if (_cartBoardType == CartBoardType::RomWithRam) {
		memset(_cartRam.data(), 0x00, _cartRam.size());
	}
}

uint8_t ChannelFMemoryManager::ReadMemory(uint16_t addr) const {
	if (_cartBoardType == CartBoardType::RomWithRam && addr >= CartRamStartAddr && addr <= CartRamEndAddr) {
		return _cartRam[addr - CartRamStartAddr];
	}

	if (addr < BiosSize) {
		return _biosRom[addr];
	}
	// Cartridge ROM at $0800+
	uint32_t cartAddr = addr - BiosSize;
	if (_cartBoardType == CartBoardType::BankedRom && cartAddr < CartBankWindowSize) {
		uint32_t bankedOffset = (uint32_t)_activeCartBank * CartBankWindowSize + cartAddr;
		if (bankedOffset < _cartSize) {
			return _cartRom[bankedOffset];
		}
		return 0xff;
	}

	if (cartAddr < _cartSize) {
		return _cartRom[cartAddr];
	}
	return 0xff;
}

void ChannelFMemoryManager::WriteMemory(uint16_t addr, uint8_t value) {
	if (_cartBoardType == CartBoardType::RomWithRam && addr >= CartRamStartAddr && addr <= CartRamEndAddr) {
		_cartRam[addr - CartRamStartAddr] = value;
		return;
	}

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
			case 0x20:
				return _smiTimerCounter;
			case 0x21:
				return _smiTimerControl;
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
		case 0x20:
		case 0x21:
		case 0x22:
		case 0x23:
		case 0x24:
		case 0x25:
		case 0x26:
		case 0x27:
			if (_cartBoardType == CartBoardType::BankedRom) {
				uint8_t requestedBank = (uint8_t)(port - 0x20);
				uint8_t maxBank = (uint8_t)std::max(1u, (_cartSize + CartBankWindowSize - 1) / CartBankWindowSize);
				_activeCartBank = (uint8_t)(requestedBank % maxBank);
			} else {
				// 3853 SMI ports: $20-$21 = timer, $22-$23 = interrupt vector
				switch (port) {
					case 0x20:
						_smiTimerLatch = value;
						if ((_smiTimerControl & 0x01) != 0) {
							_smiTimerCounter = _smiTimerLatch;
						}
						break;
					case 0x21:
						// bit0: enable, bit1: auto-reload
						_smiTimerControl = value;
						_smiTimerEnabled = (value & 0x01) != 0;
						if (_smiTimerEnabled) {
							_smiTimerCounter = _smiTimerLatch;
							if (_onSmiIrqLineChanged) {
								_onSmiIrqLineChanged(false);
							}
						}
						break;
					case 0x22:
						_interruptVectorHigh = value;
						if (_onInterruptVectorChanged) {
							_onInterruptVectorChanged(GetInterruptVector());
						}
						break;
					case 0x23:
						_interruptVectorLow = value;
						if (_onInterruptVectorChanged) {
							_onInterruptVectorChanged(GetInterruptVector());
						}
						break;
					// $20-$21: timer (not yet implemented)
					default:
						break;
				}
			}
			break;

		case 0:
			// Port 0 write: bits 0-1 = color, bits 5-6 = tone select
			// Tone select: 0=silence, 1=~1kHz, 2=~500Hz, 3=~120Hz
			_videoColor = value & 0x03;
			_backgroundColor = value & 0x03;
			_soundToneSelect = (value >> 5) & 0x03;
			break;

		case 1: {
			// Port 1 write: video column and row control
			// Bits 0-6 = column (X), bit 7 = write-enable
			uint8_t col = value & 0x7f;
			if (value & 0x80) {
				// Queue the write to land on the next cycle to model port-write video latency.
				if (col < ScreenWidth && _videoY < ScreenHeight) {
					_pendingVideoWrites.push_back({
						(uint16_t)col,
						_videoY,
						(uint8_t)(_videoColor & 0x03),
						_videoCycle + 1
					});
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
			// Port 5: controller/peripheral I/O (not tone select), low nibble used as output volume.
			_soundVolume = value & 0x0f;
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
	state.BackgroundColor = _backgroundColor;
	state.PendingWrites = (uint16_t)std::min<size_t>(_pendingVideoWrites.size(), std::numeric_limits<uint16_t>::max());
	return state;
}

ChannelFAudioState ChannelFMemoryManager::GetAudioState() const {
	ChannelFAudioState state = {};
	state.ToneSelect = _soundToneSelect;
	state.Volume = _soundVolume;
	state.SoundEnabled = (_soundToneSelect != 0);
	if (_soundToneSelect != 0) {
		uint32_t freq = ToneFrequencies[_soundToneSelect];
		state.HalfPeriodCycles = std::max(1u, _masterClockHz / (2u * freq));
	}
	state.CycleCounter = _audioCounter;
	state.OutputHigh = _audioOutput;
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
	_backgroundColor = state.BackgroundColor;
}

void ChannelFMemoryManager::SetAudioState(const ChannelFAudioState& state) {
	_soundToneSelect = state.ToneSelect;
	_soundVolume = state.Volume;
	_audioCounter = state.CycleCounter;
	_audioOutput = state.OutputHigh;
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
	_videoCycle++;
	ApplyDeferredVideoWrites();

	if (_smiTimerEnabled && _smiTimerCounter > 0) {
		_smiTimerCounter--;
		if (_smiTimerCounter == 0) {
			if (_onSmiIrqLineChanged) {
				_onSmiIrqLineChanged(true);
			}
			if ((_smiTimerControl & 0x02) != 0) {
				_smiTimerCounter = _smiTimerLatch;
			} else {
				_smiTimerEnabled = false;
			}
		}
	}

	// Channel F PSG: 4 discrete tones from port 0 bits 5-6
	// _soundToneSelect: 0=silence, 1=~1kHz, 2=~500Hz, 3=~120Hz
	// Half period is derived from active console clock (NTSC/PAL), coupling audio to frame timing.

	int16_t sample = 0;
	if (_soundToneSelect != 0) {
		uint32_t freq = ToneFrequencies[_soundToneSelect];
		uint32_t halfPeriod = std::max(1u, _masterClockHz / (2u * freq));
		_audioCounter++;
		if (_audioCounter >= halfPeriod) {
			_audioCounter = 0;
			_audioOutput = !_audioOutput;
		}
		int16_t amplitude = (int16_t)((8192 * _soundVolume) / 15);
		sample = _audioOutput ? amplitude : (int16_t)-amplitude;
	} else {
		_audioCounter = 0;
		_audioOutput = false;
	}
	// Stereo: push left + right (mono duplicated)
	_audioBuffer.push_back(sample);
	_audioBuffer.push_back(sample);
}

void ChannelFMemoryManager::ApplyDeferredVideoWrites() {
	if (_pendingVideoWrites.empty()) {
		return;
	}

	size_t writeIndex = 0;
	while (writeIndex < _pendingVideoWrites.size()) {
		const DeferredVideoWrite& write = _pendingVideoWrites[writeIndex];
		if (write.ApplyCycle > _videoCycle) {
			break;
		}

		if (write.X < ScreenWidth && write.Y < ScreenHeight) {
			_vram[(uint32_t)write.Y * ScreenWidth + write.X] = write.Color;
		}
		writeIndex++;
	}

	if (writeIndex > 0) {
		_pendingVideoWrites.erase(_pendingVideoWrites.begin(), _pendingVideoWrites.begin() + (ptrdiff_t)writeIndex);
	}
}
