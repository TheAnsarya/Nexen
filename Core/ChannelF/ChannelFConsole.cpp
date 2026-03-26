#include "pch.h"
#include "ChannelF/ChannelFConsole.h"
#include "Shared/Emulator.h"
#include "Shared/MemoryType.h"
#include "Utilities/VirtualFile.h"

ChannelFConsole::ChannelFConsole(Emulator* emu)
	: _emu(emu),
	  _core(ChannelFBiosVariant::Unknown),
	  _frameBuffer(ScreenWidth * ScreenHeight, 0) {
	_controlManager = std::make_unique<ChannelFControlManager>(emu);
}

ChannelFConsole::~ChannelFConsole() {
}

LoadRomResult ChannelFConsole::LoadRom(VirtualFile& romFile) {
	vector<uint8_t> romData;
	if (!romFile.ReadFile(romData) || romData.empty()) {
		return LoadRomResult::Failure;
	}

	_romSha1 = romFile.GetSha1Hash();
	_core.DetectVariantFromHashes(_romSha1, "");
	_romFormat = RomFormat::ChannelF;

	Reset();
	return LoadRomResult::Success;
}

void ChannelFConsole::RunFrame() {
	if (_controlManager) {
		_controlManager->UpdateControlDevices();
		_controlManager->UpdateInputState();
	}

	_core.RunFrame();
	_frameCount++;

	uint8_t pixel = (uint8_t)(_core.GetDeterministicState() & 0x7f);
	std::fill(_frameBuffer.begin(), _frameBuffer.end(), pixel);

	if (_controlManager) {
		_controlManager->ProcessEndOfFrame();
	}
}

void ChannelFConsole::Reset() {
	_core.Reset();
	_frameCount = 0;
	std::fill(_frameBuffer.begin(), _frameBuffer.end(), uint16_t{0});
}

void ChannelFConsole::SaveBattery() {
}

BaseControlManager* ChannelFConsole::GetControlManager() {
	return _controlManager.get();
}

ConsoleRegion ChannelFConsole::GetRegion() {
	return ConsoleRegion::Ntsc;
}

ConsoleType ChannelFConsole::GetConsoleType() {
	return ConsoleType::ChannelF;
}

vector<CpuType> ChannelFConsole::GetCpuTypes() {
	// Placeholder CPU identity until dedicated F8 debugger plumbing lands.
	return {CpuType::Atari2600};
}

uint64_t ChannelFConsole::GetMasterClock() {
	return _core.GetMasterClock();
}

uint32_t ChannelFConsole::GetMasterClockRate() {
	return ChannelFCoreScaffold::CpuClockHz;
}

double ChannelFConsole::GetFps() {
	return ChannelFCoreScaffold::Fps;
}

BaseVideoFilter* ChannelFConsole::GetVideoFilter([[maybe_unused]] bool getDefaultFilter) {
	return new ChannelFDefaultVideoFilter(_emu);
}

PpuFrameInfo ChannelFConsole::GetPpuFrame() {
	PpuFrameInfo frame = {};
	frame.FrameBuffer = (uint8_t*)_frameBuffer.data();
	frame.Width = ScreenWidth;
	frame.Height = ScreenHeight;
	frame.FrameBufferSize = (uint32_t)(_frameBuffer.size() * sizeof(uint16_t));
	frame.FrameCount = _frameCount;
	frame.ScanlineCount = ScreenHeight;
	frame.FirstScanline = 0;
	frame.CycleCount = ChannelFCoreScaffold::CyclesPerFrame;
	return frame;
}

RomFormat ChannelFConsole::GetRomFormat() {
	return _romFormat;
}

AudioTrackInfo ChannelFConsole::GetAudioTrackInfo() {
	return {};
}

void ChannelFConsole::ProcessAudioPlayerAction([[maybe_unused]] AudioPlayerActionParams p) {
}

AddressInfo ChannelFConsole::GetAbsoluteAddress([[maybe_unused]] AddressInfo& relAddress) {
	return {-1, MemoryType::None};
}

AddressInfo ChannelFConsole::GetPcAbsoluteAddress() {
	return {-1, MemoryType::None};
}

AddressInfo ChannelFConsole::GetRelativeAddress([[maybe_unused]] AddressInfo& absAddress, [[maybe_unused]] CpuType cpuType) {
	return {-1, MemoryType::None};
}

void ChannelFConsole::GetConsoleState([[maybe_unused]] BaseState& state, [[maybe_unused]] ConsoleType consoleType) {
}

string ChannelFConsole::GetHash(HashType hashType) {
	if (hashType == HashType::Sha1 || hashType == HashType::Sha1Cheat) {
		return _romSha1;
	}
	return {};
}

void ChannelFConsole::Serialize(Serializer& s) {
	_core.Serialize(s);
	s.Stream(_frameCount, "frameCount");
	s.Stream(_romFormat, "romFormat");
}
