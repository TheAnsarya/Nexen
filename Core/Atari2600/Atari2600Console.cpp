#include "pch.h"
#include "Atari2600/Atari2600Console.h"
#include "Atari2600/Atari2600Mapper.h"
#include "Atari2600/Atari2600Riot.h"
#include "Atari2600/Atari2600Tia.h"
#include "Atari2600/Atari2600Bus.h"
#include "Atari2600/Atari2600CpuAdapter.h"
#include "Atari2600/Atari2600ControlManager.h"
#include "Atari2600/Atari2600DefaultVideoFilter.h"
#include "Shared/Audio/SoundMixer.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"
#include "Shared/MemoryType.h"
#include "Utilities/VirtualFile.h"
#include "Utilities/Serializer.h"

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
	const vector<int16_t>& audioBuffer = _tia->GetAudioBuffer();
	if (!audioBuffer.empty()) {
		_emu->GetSoundMixer()->PlayAudioBuffer(const_cast<int16_t*>(audioBuffer.data()), (uint32_t)audioBuffer.size() / 2, GetMasterClockRate());
	}
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

void Atari2600Console::SetTiaState(const Atari2600TiaState& state) {
	_tia->SetState(state);
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

void Atari2600Console::DebugRenderFrame() {
	RenderDebugFrame();
}

void Atari2600Console::RenderDebugFrame() {
	// Convert TIA color register (0-255) to palette index (0-127)
	auto toPaletteIndex = [](uint8_t tiaColor) -> uint16_t {
		return (tiaColor >> 1) & 0x7f;
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
		uint16_t colorBackground = toPaletteIndex(scanlineState.ColorBackground);
		uint16_t colorPlayfield = toPaletteIndex(scanlineState.ColorPlayfield);
		uint16_t colorPlayer0 = toPaletteIndex(scanlineState.ColorPlayer0);
		uint16_t colorPlayer1 = toPaletteIndex(scanlineState.ColorPlayer1);

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
