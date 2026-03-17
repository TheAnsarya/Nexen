#include "pch.h"
#include "Atari2600/Atari2600Console.h"
#include "Atari2600/Atari2600DefaultVideoFilter.h"
#include "Shared/BaseControlManager.h"
#include "Shared/CpuType.h"
#include "Shared/MemoryType.h"
#include "Utilities/VirtualFile.h"
#include "Utilities/Serializer.h"
#include <functional>

class Atari2600Mapper {
private:
	enum class MapperMode {
		None,
		Fixed2K,
		Fixed4K,
		F8,
		F6,
		Unknown
	};

	vector<uint8_t> _rom;
	MapperMode _mode = MapperMode::None;
	uint8_t _activeBank = 0;

	void SelectMode() {
		if (_rom.empty()) {
			_mode = MapperMode::None;
			_activeBank = 0;
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
		} else {
			_mode = MapperMode::Unknown;
			_activeBank = 0;
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

			case MapperMode::Unknown:
				return cartAddr % _rom.size();

			case MapperMode::None:
			default:
				return 0;
		}
	}

public:
	void LoadRom(const vector<uint8_t>& romData) {
		_rom = romData;
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
			case MapperMode::Unknown: return "unknown";
			default: return "none";
		}
	}
};

class Atari2600Riot {
	private:
		Atari2600RiotState _state = {};

	public:
		void Reset() {
			_state = {};
		}

		void StepCpuCycles(uint32_t cycles) {
			for (uint32_t i = 0; i < cycles; i++) {
				_state.CpuCycles++;
				if (_state.Timer > 0) {
					_state.Timer--;
				} else {
					_state.TimerUnderflow = true;
				}
			}
		}

		uint8_t ReadRegister(uint16_t addr) const {
			switch (addr & 0x07) {
				case 0x00: return _state.PortA;
				case 0x01: return _state.PortB;
				case 0x04: return (uint8_t)(_state.Timer & 0xFF);
				case 0x05: return (uint8_t)((_state.Timer >> 8) & 0xFF);
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
				case 0x04:
					_state.Timer = value;
					_state.TimerUnderflow = false;
					break;
				case 0x05:
					_state.Timer = (uint16_t)value << 6;
					_state.TimerUnderflow = false;
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
		}

		void StepCpuCycles(uint32_t cpuCycles) {
			for (uint32_t i = 0; i < cpuCycles; i++) {
				if (_state.WsyncHold) {
					_state.WsyncHold = false;
					AdvanceScanline();
				}
				StepColorClocks(3);
			}
		}

		void RequestWsync() {
			_state.WsyncHold = true;
		}

		Atari2600TiaState GetState() const {
			return _state;
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
			return 0;
		}

		void Write(uint16_t addr, uint8_t value) {
			addr &= 0x1FFF;
			if ((addr & 0x1080) == 0x0080 && _riot) {
				_riot->WriteRegister(addr, value);
				return;
			}
			if ((addr & 0x1000) == 0x1000 && _mapper) {
				_mapper->Write(addr, value);
				return;
			}
			if (_tia && ((addr & 0x3F) == 0x02)) {
				_tia->RequestWsync();
			}
		}
	};

	class Atari2600CpuAdapter {
	private:
		std::function<uint8_t(uint16_t)> _read;
		std::function<void(uint16_t, uint8_t)> _write;
		uint16_t _pc = 0x1000;
		uint64_t _cycleCount = 0;

	public:
		void Reset() {
			_pc = 0x1000;
			_cycleCount = 0;
		}

		void SetReadCallback(std::function<uint8_t(uint16_t)> cb) {
			_read = std::move(cb);
		}

		void SetWriteCallback(std::function<void(uint16_t, uint8_t)> cb) {
			_write = std::move(cb);
		}

		void StepCycles(uint32_t cycles) {
			for (uint32_t i = 0; i < cycles; i++) {
				if (_read) {
					volatile uint8_t opcode = _read(_pc);
					(void)opcode;
				}
				_pc = (_pc + 1) & 0x1FFF;
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

	_mapper->LoadRom(romData);
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
	uint32_t frame = _lastFrameSummary.FrameCount;
	for (uint32_t y = 0; y < ScreenHeight; y++) {
		uint8_t shade = (uint8_t)((y + frame) & 0x1F);
		uint16_t pixel = (uint16_t)((shade << 11) | (shade << 6) | shade);
		for (uint32_t x = 0; x < ScreenWidth; x++) {
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
	return {};
}

void Atari2600Console::ProcessAudioPlayerAction(AudioPlayerActionParams p) {
	(void)p;
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
