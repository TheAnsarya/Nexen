#pragma once
#include "pch.h"
#include "Shared/Interfaces/IConsole.h"

class Emulator;
class BaseControlManager;
class Atari2600CpuAdapter;
class Atari2600Riot;
class Atari2600Tia;
class Atari2600Mapper;
class Atari2600Bus;

struct Atari2600RiotState {
	uint8_t PortA = 0;
	uint8_t PortB = 0;
	uint8_t PortADirection = 0;
	uint8_t PortBDirection = 0;
	uint8_t PortAInput = 0;
	uint8_t PortBInput = 0;
	uint16_t Timer = 0;
	uint16_t TimerDivider = 1;
	uint16_t TimerDividerCounter = 1;
	bool TimerUnderflow = false;
	bool InterruptFlag = false;
	uint32_t InterruptEdgeCount = 0;
	uint64_t CpuCycles = 0;
};

struct Atari2600TiaState {
	uint32_t FrameCount = 0;
	uint32_t Scanline = 0;
	uint32_t ColorClock = 0;
	bool WsyncHold = false;
	uint32_t WsyncCount = 0;
	bool HmovePending = false;
	bool HmoveDelayToNextScanline = false;
	uint32_t HmoveStrobeCount = 0;
	uint32_t HmoveApplyCount = 0;
	uint8_t ColorBackground = 0;
	uint8_t ColorPlayfield = 0;
	uint8_t ColorPlayer0 = 0;
	uint8_t ColorPlayer1 = 0;
	uint8_t Playfield0 = 0;
	uint8_t Playfield1 = 0;
	uint8_t Playfield2 = 0;
	bool PlayfieldReflect = false;
	bool PlayfieldScoreMode = false;
	bool PlayfieldPriority = false;
	uint8_t Nusiz0 = 0;
	uint8_t Nusiz1 = 0;
	uint8_t Player0Graphics = 0;
	uint8_t Player1Graphics = 0;
	bool Missile0Enabled = false;
	bool Missile1Enabled = false;
	bool BallEnabled = false;
	bool VdelPlayer0 = false;
	bool VdelPlayer1 = false;
	bool VdelBall = false;
	uint8_t DelayedPlayer0Graphics = 0;
	uint8_t DelayedPlayer1Graphics = 0;
	bool DelayedBallEnabled = false;
	uint8_t Player0X = 24;
	uint8_t Player1X = 96;
	uint8_t Missile0X = 32;
	uint8_t Missile1X = 104;
	uint8_t BallX = 80;
	int8_t MotionPlayer0 = 0;
	int8_t MotionPlayer1 = 0;
	int8_t MotionMissile0 = 0;
	int8_t MotionMissile1 = 0;
	int8_t MotionBall = 0;
	uint8_t CollisionCxm0p = 0;
	uint8_t CollisionCxm1p = 0;
	uint8_t CollisionCxp0fb = 0;
	uint8_t CollisionCxp1fb = 0;
	uint8_t CollisionCxm0fb = 0;
	uint8_t CollisionCxm1fb = 0;
	uint8_t CollisionCxblpf = 0;
	uint8_t CollisionCxppmm = 0;
	uint32_t RenderRevision = 0;
	uint8_t AudioControl0 = 0;
	uint8_t AudioControl1 = 0;
	uint8_t AudioFrequency0 = 0;
	uint8_t AudioFrequency1 = 0;
	uint8_t AudioVolume0 = 0;
	uint8_t AudioVolume1 = 0;
	uint16_t AudioCounter0 = 1;
	uint16_t AudioCounter1 = 1;
	uint8_t AudioPhase0 = 0;
	uint8_t AudioPhase1 = 0;
	uint8_t LastMixedSample = 0;
	uint64_t AudioMixAccumulator = 0;
	uint64_t AudioSampleCount = 0;
	uint32_t AudioRevision = 0;
	uint64_t TotalColorClocks = 0;
};

struct Atari2600ScanlineRenderState {
	uint8_t ColorBackground = 0;
	uint8_t ColorPlayfield = 0;
	uint8_t ColorPlayer0 = 0;
	uint8_t ColorPlayer1 = 0;
	uint8_t Playfield0 = 0;
	uint8_t Playfield1 = 0;
	uint8_t Playfield2 = 0;
	bool PlayfieldReflect = false;
	bool PlayfieldScoreMode = false;
	bool PlayfieldPriority = false;
	uint8_t Nusiz0 = 0;
	uint8_t Nusiz1 = 0;
	uint8_t Player0Graphics = 0;
	uint8_t Player1Graphics = 0;
	bool Missile0Enabled = false;
	bool Missile1Enabled = false;
	bool BallEnabled = false;
	uint8_t Player0X = 24;
	uint8_t Player1X = 96;
	uint8_t Missile0X = 32;
	uint8_t Missile1X = 104;
	uint8_t BallX = 80;
};

struct Atari2600FrameStepSummary {
	uint32_t FrameCount = 0;
	uint32_t CpuCyclesThisFrame = 0;
	uint32_t ScanlineAtFrameEnd = 0;
	uint32_t ColorClockAtFrameEnd = 0;
};

class Atari2600Console final : public IConsole {
private:
	Emulator* _emu = nullptr;
	unique_ptr<Atari2600CpuAdapter> _cpu;
	unique_ptr<Atari2600Riot> _riot;
	unique_ptr<Atari2600Tia> _tia;
	unique_ptr<Atari2600Mapper> _mapper;
	unique_ptr<Atari2600Bus> _bus;
	unique_ptr<BaseControlManager> _controlManager;
	vector<uint16_t> _frameBuffer;
	Atari2600FrameStepSummary _lastFrameSummary = {};
	bool _romLoaded = false;

	void RenderDebugFrame();

public:
	static constexpr uint32_t ScreenWidth = 160;
	static constexpr uint32_t ScreenHeight = 192;
	static constexpr uint32_t CpuCyclesPerFrame = 19912;

	[[nodiscard]] static vector<string> GetSupportedExtensions() { return {".a26"}; }
	[[nodiscard]] static vector<string> GetSupportedSignatures() { return {}; }

	explicit Atari2600Console(Emulator* emu);
	~Atari2600Console() override;

	LoadRomResult LoadRom(VirtualFile& romFile) override;
	void Reset() override;
	void RunFrame() override;
	void SaveBattery() override;

	BaseControlManager* GetControlManager() override;
	ConsoleRegion GetRegion() override;
	ConsoleType GetConsoleType() override;
	vector<CpuType> GetCpuTypes() override;
	uint64_t GetMasterClock() override;
	uint32_t GetMasterClockRate() override;
	double GetFps() override;
	BaseVideoFilter* GetVideoFilter(bool getDefaultFilter) override;
	PpuFrameInfo GetPpuFrame() override;
	RomFormat GetRomFormat() override;
	AudioTrackInfo GetAudioTrackInfo() override;
	void ProcessAudioPlayerAction(AudioPlayerActionParams p) override;
	AddressInfo GetAbsoluteAddress(AddressInfo& relAddress) override;
	AddressInfo GetPcAbsoluteAddress() override;
	AddressInfo GetRelativeAddress(AddressInfo& absAddress, CpuType cpuType) override;
	void GetConsoleState(BaseState& state, ConsoleType consoleType) override;
	void Serialize(Serializer& s) override;

	void StepCpuCycles(uint32_t cpuCycles);
	void RequestWsync();
	void RequestHmove();
	Atari2600RiotState GetRiotState() const;
	Atari2600TiaState GetTiaState() const;
	Atari2600FrameStepSummary GetLastFrameSummary() const { return _lastFrameSummary; }
	uint8_t DebugReadCartridge(uint16_t addr) const;
	void DebugWriteCartridge(uint16_t addr, uint8_t value);
	Atari2600ScanlineRenderState DebugGetScanlineRenderState(uint32_t scanline) const;
	uint8_t DebugGetMapperBankIndex() const;
	string DebugGetMapperMode() const;
};
