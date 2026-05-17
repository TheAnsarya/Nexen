#include "pch.h"
#include "Genesis/GenesisConsole.h"
#include "Genesis/GenesisM68k.h"
#include "Genesis/GenesisVdp.h"
#include "Genesis/GenesisControlManager.h"
#include "Genesis/GenesisMemoryManager.h"
#include "Genesis/GenesisPsg.h"
#include "Genesis/GenesisTypes.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"
#include "Shared/MessageManager.h"
#include "Genesis/GenesisDefaultVideoFilter.h"
#include "Debugger/DebugTypes.h"
#include "Shared/Video/BaseVideoFilter.h"
#include "Shared/RenderedFrame.h"
#include "Shared/Video/VideoDecoder.h"
#include "Shared/RewindManager.h"
#include "Utilities/Serializer.h"
#include "Shared/EventType.h"
#include <cstdlib>
#ifdef _WIN32
#include <excpt.h>
#endif

namespace {
	bool HasSegaHeader(const vector<uint8_t>& romData) {
		if (romData.size() < 0x104) {
			return false;
		}

		return romData[0x100] == 'S' && romData[0x101] == 'E' && romData[0x102] == 'G' && romData[0x103] == 'A';
	}

	bool IsLikelySmdImage(const vector<uint8_t>& romData, const string& extension) {
		if (romData.size() <= 0x200) {
			return false;
		}

		if (extension == ".smd") {
			return true;
		}

		// Typical SMD layout is a 512-byte header plus 16KB interleaved payload blocks.
		if ((romData.size() & 0x3fff) != 0x200) {
			return false;
		}

		// Avoid false positives when the raw image already looks like linear Genesis ROM.
		return !HasSegaHeader(romData);
	}

	void DecodeSmdToLinear(vector<uint8_t>& romData) {
		if (romData.size() <= 0x200) {
			return;
		}

		size_t payloadSize = romData.size() - 0x200;
		if ((payloadSize & 0x3fff) != 0) {
			// Irregular payload: safest fallback is to strip the copier header only.
			romData.erase(romData.begin(), romData.begin() + 0x200);
			return;
		}

		vector<uint8_t> decoded(payloadSize);
		const uint8_t* src = romData.data() + 0x200;

		for (size_t block = 0; block < payloadSize; block += 0x4000) {
			const uint8_t* in = src + block;
			uint8_t* out = decoded.data() + block;
			for (size_t i = 0; i < 0x2000; i++) {
				out[(i << 1)] = in[0x2000 + i];
				out[(i << 1) + 1] = in[i];
			}
		}

		romData.swap(decoded);
	}

	ConsoleRegion ResolveConfiguredGenesisRegion(ConsoleRegion configuredRegion) {
		switch (configuredRegion) {
			case ConsoleRegion::Pal:
				return ConsoleRegion::Pal;

			case ConsoleRegion::NtscJapan:
				return ConsoleRegion::NtscJapan;

			case ConsoleRegion::Ntsc:
			case ConsoleRegion::Dendy:
				return ConsoleRegion::Ntsc;

			case ConsoleRegion::Auto:
			default:
				return ConsoleRegion::Auto;
		}
	}

	ConsoleRegion DetectGenesisRegionFromHeader(const vector<uint8_t>& romData) {
		if (romData.size() < 0x1F0) {
			return ConsoleRegion::Ntsc;
		}

		uint32_t regionMask = 0;
		bool hasPalMarker = false;
		bool hasNtscMarker = false;
		bool hasJapanMarker = false;
		bool hasOverseasNtscMarker = false;
		bool foundAnyRegionMarker = false;
		uint32_t endOffset = (uint32_t)std::min<size_t>(romData.size(), 0x200);
		for (uint32_t i = 0x1F0; i < endOffset; i++) {
			char marker = (char)std::toupper((unsigned char)romData[i]);
			if (marker == 0 || marker == ' ') {
				continue;
			}

			if (marker == 'E') {
				foundAnyRegionMarker = true;
				hasPalMarker = true;
			} else if (marker == 'U' || marker == 'J') {
				foundAnyRegionMarker = true;
				hasNtscMarker = true;
				if (marker == 'J') {
					hasJapanMarker = true;
				}
				if (marker == 'U') {
					hasOverseasNtscMarker = true;
				}
			} else if (std::isxdigit((unsigned char)marker)) {
				foundAnyRegionMarker = true;
				if (marker >= '0' && marker <= '9') {
					regionMask |= (uint32_t)(marker - '0');
				} else {
					regionMask |= (uint32_t)(10 + marker - 'A');
				}
			}
		}

		if (regionMask != 0) {
			hasPalMarker = hasPalMarker || ((regionMask & 0x08) != 0);
			hasNtscMarker = hasNtscMarker || ((regionMask & 0x01) != 0) || ((regionMask & 0x04) != 0);
			hasJapanMarker = hasJapanMarker || ((regionMask & 0x01) != 0);
			hasOverseasNtscMarker = hasOverseasNtscMarker || ((regionMask & 0x04) != 0);
		}

		if (!foundAnyRegionMarker) {
			return ConsoleRegion::Ntsc;
		}

		if (hasPalMarker && !hasNtscMarker) {
			return ConsoleRegion::Pal;
		}

		if (hasNtscMarker && !hasPalMarker && hasJapanMarker && !hasOverseasNtscMarker) {
			return ConsoleRegion::NtscJapan;
		}

		return ConsoleRegion::Ntsc;
	}

	bool IsGenesisSehDiagGuardEnabled() {
		static int cached = -1;
		if (cached >= 0) {
			return cached == 1;
		}

		cached = 0;
		const char* raw = std::getenv("NEXEN_GENESIS_SEH_DIAG_GUARD");
		if (raw && (*raw == '1' || *raw == 'y' || *raw == 'Y' || *raw == 't' || *raw == 'T')) {
			cached = 1;
		}
		return cached == 1;
	}

	uint32_t GetGenesisRunFrameInstructionCap() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = 5000000;
		const char* raw = std::getenv("NEXEN_GENESIS_RUNFRAME_INSTR_CAP");
		if (!raw || !*raw) {
			return cached;
		}

		char* end = nullptr;
		unsigned long parsed = std::strtoul(raw, &end, 0);
		if (end == raw || *end != '\0') {
			return cached;
		}

		if (parsed < 50000ul) {
			parsed = 50000ul;
		} else if (parsed > 50000000ul) {
			parsed = 50000000ul;
		}
		cached = (uint32_t)parsed;
		return cached;
	}

	bool IsGenesisAutoTraceDisabled() {
		const char* raw = std::getenv("NEXEN_GENESIS_DISABLE_AUTO_TRACE");
		return raw && (*raw == '1' || *raw == 'y' || *raw == 'Y' || *raw == 't' || *raw == 'T');
	}

	uint32_t GetGenesisSonicStartupCheckpointIntervalFrames() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = 30;
		const char* raw = std::getenv("NEXEN_GENESIS_SONIC_CHECKPOINT_INTERVAL_FRAMES");
		if (!raw || !*raw) {
			return cached;
		}

		char* end = nullptr;
		unsigned long parsed = std::strtoul(raw, &end, 0);
		if (end == raw || *end != '\0') {
			return cached;
		}

		if (parsed < 1ul) {
			parsed = 1ul;
		} else if (parsed > 300ul) {
			parsed = 300ul;
		}
		cached = (uint32_t)parsed;
		return cached;
	}

	uint32_t GetGenesisSonicStartupCheckpointEndFrame() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = 900;
		const char* raw = std::getenv("NEXEN_GENESIS_SONIC_CHECKPOINT_END_FRAME");
		if (!raw || !*raw) {
			return cached;
		}

		char* end = nullptr;
		unsigned long parsed = std::strtoul(raw, &end, 0);
		if (end == raw || *end != '\0') {
			return cached;
		}

		if (parsed < 60ul) {
			parsed = 60ul;
		} else if (parsed > 3600ul) {
			parsed = 3600ul;
		}
		cached = (uint32_t)parsed;
		return cached;
	}

	bool IsGenesisSonicStartupCheckpointEnabled() {
		const char* raw = std::getenv("NEXEN_GENESIS_SONIC_CHECKPOINT_ENABLE");
		if (!raw || !*raw) {
			return true;
		}

		return *raw == '1' || *raw == 'y' || *raw == 'Y' || *raw == 't' || *raw == 'T';
	}

	uint32_t GetGenesisSonicTraceRearmFrameWindow() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = 180;
		const char* raw = std::getenv("NEXEN_GENESIS_SONIC_TRACE_REARM_FRAME_WINDOW");
		if (!raw || !*raw) {
			return cached;
		}

		char* end = nullptr;
		unsigned long parsed = std::strtoul(raw, &end, 0);
		if (end == raw || *end != '\0') {
			return cached;
		}

		if (parsed < 30ul) {
			parsed = 30ul;
		} else if (parsed > 1800ul) {
			parsed = 1800ul;
		}
		cached = (uint32_t)parsed;
		return cached;
	}

	const char* GenesisStartupTitleClassName(uint8_t value) {
		switch (value) {
			case 1: return "sonic";
			case 2: return "sonic1";
			case 3: return "sonic2";
			case 4: return "sonic3";
			case 5: return "sonic-and-knuckles";
			case 6: return "sonic-spinball";
			default: return "unknown";
		}
	}

	bool IsSonicStartupTitleClass(uint8_t value) {
		return value >= 1 && value <= 6;
	}

	string BuildGenesisStartupRuntimeSummary(const GenesisMemoryManager* memoryManager) {
		if (!memoryManager) {
			return "mmu=missing";
		}

		return std::format("titleClass={}({}) title='{}' product='{}' hint={} autotune={} dynamic={} startupFrame={} window={} logoEnd={} strictStart={} displayTransitions={} startupSeq={} startupDigest={:016x} arbDigest={:02x} arbEpoch={} arbMclk={}",
			memoryManager->GetStartupTitleClassValue(),
			GenesisStartupTitleClassName(memoryManager->GetStartupTitleClassValue()),
			memoryManager->GetStartupDetectedTitle(),
			memoryManager->GetStartupDetectedProductCode(),
			memoryManager->GetStartupTitleHintUsed() ? 1 : 0,
			memoryManager->GetStartupTitleAutotuneApplied() ? 1 : 0,
			memoryManager->GetStartupUseDynamicBusTiming() ? 1 : 0,
			memoryManager->GetStartupFrameForDiagnostics(),
			memoryManager->GetStartupWindowFrames(),
			memoryManager->GetStartupLogoPhaseEndFrame(),
			memoryManager->GetStartupStrictPhaseStartFrame(),
			memoryManager->GetStartupDisplayTransitionCount(),
			memoryManager->GetStartupTraceSequence(),
			memoryManager->GetStartupTraceDigest(),
			memoryManager->GetStartupArbitrationDigest(),
			memoryManager->GetStartupArbitrationEpoch(),
			memoryManager->GetStartupLastArbitrationMclk());
	}

	bool TryExecWithSehGuard(GenesisM68k* cpu, uint32_t& sehCode) {
		if (!cpu) {
			return false;
		}

		sehCode = 0;
		#ifdef _WIN32
		__try {
			cpu->Exec();
			return true;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			sehCode = (uint32_t)_exception_code();
			return false;
		}
		#else
		cpu->Exec();
		return true;
		#endif
	}
}

GenesisConsole::GenesisConsole(Emulator* emu) {
	_emu = emu;
}

GenesisConsole::~GenesisConsole() {
}

string GenesisConsole::BuildRunFrameCrashProbeSummary() const {
	string cpuSummary = _cpu ? _cpu->BuildCrashProbeSummary() : "cpu=missing";
	string cpuBoundarySummary = _cpu ? _cpu->BuildDispatchBoundaryProbeSummary() : "cpuBoundary=missing";
	string mmuFlowSummary = _memoryManager ? _memoryManager->BuildRuntimeFlowTraceSummary() : "enabled=0";
	string mmuOpSummary = _memoryManager ? _memoryManager->BuildRuntimeOpTraceSummary() : "enabled=0";
	string startupSummary = BuildGenesisStartupRuntimeSummary(_memoryManager.get());
	bool sonicCheckpointEnabled = IsGenesisSonicStartupCheckpointEnabled();
	uint32_t sonicCheckpointInterval = GetGenesisSonicStartupCheckpointIntervalFrames();
	uint32_t sonicCheckpointEndFrame = GetGenesisSonicStartupCheckpointEndFrame();
	uint32_t sonicTraceRearmWindow = GetGenesisSonicTraceRearmFrameWindow();
	return std::format(
		"entryCount={} exitCount={} earlyAbortCount={} firstFailureCaptures={} firstFailureBoundary={} lastGuard={} stalls={} forcedAdvances={} stallSummary={} entrySummary={} exitSummary={} cpuProbe={} cpuBoundaryProbe={} mmuFlow={} mmuOps={} startup={} sonicTraceArm={} sonicTraceArms={} sonicTraceLastArmFrame={} sonicTraceRearmWindow={} sonicCheckpointEnable={} sonicCheckpointInterval={} sonicCheckpointEndFrame={} sonicCheckpoints={} sonicLastCheckpointFrame={} sonicLastCheckpointPc=${:06x}",
		_runFrameEntryCount,
		_runFrameExitCount,
		_runFrameEarlyAbortCount,
		_runFrameFirstFailureBoundaryCaptureCount,
		_runFrameFirstFailureBoundarySummary.empty() ? "none" : _runFrameFirstFailureBoundarySummary,
		_runFrameLastGuardIterations,
		_runFrameStallEventCount,
		_runFrameForcedAdvanceCount,
		_runFrameLastStallSummary.empty() ? "none" : _runFrameLastStallSummary,
		_runFrameLastEntrySummary.empty() ? "none" : _runFrameLastEntrySummary,
		_runFrameLastExitSummary.empty() ? "none" : _runFrameLastExitSummary,
		cpuSummary,
		cpuBoundarySummary,
		mmuFlowSummary,
		mmuOpSummary,
		startupSummary,
		_sonicTraceEscalationArmed ? 1 : 0,
		_sonicTraceEscalationCount,
		_sonicTraceLastArmStartupFrame,
		sonicTraceRearmWindow,
		sonicCheckpointEnabled ? 1 : 0,
		sonicCheckpointInterval,
		sonicCheckpointEndFrame,
		_sonicStartupCheckpointCount,
		_sonicStartupLastCheckpointFrame,
		_sonicStartupLastCheckpointPc & 0x00ffffff);
}

LoadRomResult GenesisConsole::LoadRom(VirtualFile& romFile) {
	vector<uint8_t> romData;
	(void)romFile.ReadFile(romData);

	if (romData.size() < 0x200) {
		return LoadRomResult::Failure;
	}

	string ext = romFile.GetFileExtension();
	std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
		return (char)std::tolower(c);
	});

	if (IsLikelySmdImage(romData, ext)) {
		DecodeSmdToLinear(romData);
	}

	// Pad ROM to power of 2
	uint32_t power = (uint32_t)std::log2(romData.size());
	if (romData.size() > ((uint64_t)1 << power)) {
		uint32_t newSize = 1 << (power + 1);
		romData.insert(romData.end(), newSize - romData.size(), 0);
	}

	EmuSettings* settings = _emu->GetSettings();
	ConsoleRegion configuredRegion = ResolveConfiguredGenesisRegion(settings->GetGenesisConfig().Region);
	_region = configuredRegion == ConsoleRegion::Auto ? DetectGenesisRegionFromHeader(romData) : configuredRegion;

	// Create all hardware components
	_vdp = std::make_unique<GenesisVdp>();
	_memoryManager = std::make_unique<GenesisMemoryManager>();
	_controlManager = std::make_unique<GenesisControlManager>(_emu, this);
	_psg = std::make_unique<GenesisPsg>(_emu, this);
	_cpu = std::make_unique<GenesisM68k>();

	_memoryManager->Init(_emu, this, romData, _vdp.get(), _controlManager.get(), _psg.get());
	_vdp->Init(_emu, this, _cpu.get(), _memoryManager.get());
	_emu->RegisterMemory(MemoryType::GenesisVideoRam, _vdp->GetVramPointer(), 0x10000);
	_emu->RegisterMemory(MemoryType::GenesisPaletteRam, _vdp->GetCramPointer(), 128);
	_memoryManager->SetCpu(_cpu.get());
	_cpu->Init(_emu, this, _memoryManager.get());
	_sonicTraceEscalationArmed = false;
	_sonicTraceEscalationCount = 0;
	_sonicTraceLastArmStartupFrame = 0;
	_sonicStartupCheckpointCount = 0;
	_sonicStartupLastCheckpointFrame = 0;
	_sonicStartupLastCheckpointPc = 0;
	if (!IsGenesisAutoTraceDisabled()) {
		_cpu->SetInstructionTraceCapacity(8192);
		_cpu->SetInstructionTraceEnabled(true);
	}
	_cpu->Reset(false);
	_memoryManager->LoadBattery();

	_vdp->SetRegion(_region == ConsoleRegion::Pal);
	const char* regionName = "NTSC";
	if (_region == ConsoleRegion::Pal) {
		regionName = "PAL";
	} else if (_region == ConsoleRegion::NtscJapan) {
		regionName = "NTSC-J";
	}

	MessageManager::Log(std::format("[Genesis] ROM loaded: {} size={} region={} tmss={}",
		romFile.GetFileName(),
		romData.size(),
		regionName,
		_memoryManager->GetIoState().TmssEnabled ? "on" : "off"));

	return LoadRomResult::Success;
}

void GenesisConsole::Reset() {
	static uint64_t resetCount = 0;
	resetCount++;
	if (resetCount <= 64 || (resetCount % 1024) == 0) {
		uint32_t pcBeforeReset = _cpu ? (_cpu->GetState().PC & 0x00ffffff) : 0;
		MessageManager::Log(std::format("[Genesis] Console Reset #{} pcBefore=${:06x}", resetCount, pcBeforeReset));
	}

	_cpu->Reset(true);
	if (_vdp) {
		_vdp->Reset(false);
		_vdp->SetRegion(_region == ConsoleRegion::Pal);
	}
	if (_controlManager) {
		_controlManager->ResetRuntimeState();
	}
	if (_memoryManager) {
		_memoryManager->ResetRuntimeState(true);
	}
	if (_psg) {
		_psg->Reset();
	}
	_sonicTraceEscalationArmed = false;
	_sonicTraceLastArmStartupFrame = 0;
	_sonicStartupCheckpointCount = 0;
	_sonicStartupLastCheckpointFrame = 0;
	_sonicStartupLastCheckpointPc = 0;
}

void GenesisConsole::RunFrame() {
	static uint64_t runFrameCallCount = 0;
	runFrameCallCount++;
	_runFrameEntryCount++;
	if (runFrameCallCount <= 256 || (runFrameCallCount % 2048) == 0) {
		uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffff) : 0xffffffff;
		uint16_t sr = _cpu ? _cpu->GetState().SR : 0;
		uint32_t frameBefore = _vdp ? _vdp->GetFrameCount() : 0;
		MessageManager::Log(std::format("[Genesis] RunFrame enter #{} frame={} pc=${:06x} sr=${:04x} masterClock={}",
			runFrameCallCount,
			frameBefore,
			pc,
			sr,
			_memoryManager ? _memoryManager->GetMasterClock() : 0));
	}

	if (!_cpu || !_vdp || !_memoryManager || !_controlManager) {
		_runFrameEarlyAbortCount++;
		if (_cpu && _runFrameFirstFailureBoundarySummary.empty()) {
			_runFrameFirstFailureBoundarySummary = _cpu->BuildDispatchBoundaryProbeSummary();
			_runFrameFirstFailureBoundaryCaptureCount++;
			if (_memoryManager) {
				MessageManager::Log(std::format("[Genesis] RunFrame first-failure pair cpuBoundary={} mmuFlow={} mmuOps={}",
					_runFrameFirstFailureBoundarySummary,
					_memoryManager->BuildRuntimeFlowTraceSummary(),
					_memoryManager->BuildRuntimeOpTraceSummary()));
			}
		}
		_runFrameLastEntrySummary = std::format("abort_missing_component cpu={} vdp={} mmu={} ctrl={}",
			_cpu ? 1 : 0,
			_vdp ? 1 : 0,
			_memoryManager ? 1 : 0,
			_controlManager ? 1 : 0);
		_runFrameLastExitSummary = _runFrameLastEntrySummary;
		MessageManager::Log(std::format("[Genesis] RunFrame early abort #{} {}", _runFrameEarlyAbortCount, BuildRunFrameCrashProbeSummary()));
		return;
	}

	_runFrameLastEntrySummary = std::format("entry={} frame={} pc=${:06x} sr=${:04x} clock={} cpuProbe={}",
		_runFrameEntryCount,
		_vdp->GetFrameCount(),
		_cpu->GetState().PC & 0x00ffffff,
		_cpu->GetState().SR,
		_memoryManager->GetMasterClock(),
		_cpu->BuildCrashProbeSummary());

	if (_memoryManager && _cpu) {
		uint8_t titleClass = _memoryManager->GetStartupTitleClassValue();
		if (IsSonicStartupTitleClass(titleClass)) {
			uint32_t startupFrame = _memoryManager->GetStartupFrameForDiagnostics();
			uint32_t rearmWindow = GetGenesisSonicTraceRearmFrameWindow();
			bool shouldArm = !_sonicTraceEscalationArmed;
			if (!shouldArm && startupFrame >= (_sonicTraceLastArmStartupFrame + rearmWindow)) {
				shouldArm = true;
			}

			if (shouldArm) {
				_cpu->ArmAggressiveFlowTrace(260000, 1, 320);
				_memoryManager->ArmAggressiveTraceBurst(260000, 340000, 1, 1, 256, 320);
				_sonicTraceEscalationArmed = true;
				_sonicTraceEscalationCount++;
				_sonicTraceLastArmStartupFrame = startupFrame;
				MessageManager::Log(std::format("[Genesis] RunFrame sonic trace escalation arm #{} titleClass={} startupFrame={} rearmWindow={} startup={}",
					_sonicTraceEscalationCount,
					titleClass,
					startupFrame,
					rearmWindow,
					BuildGenesisStartupRuntimeSummary(_memoryManager.get())));
			}

			if (IsGenesisSonicStartupCheckpointEnabled()) {
				uint32_t interval = GetGenesisSonicStartupCheckpointIntervalFrames();
				uint32_t endFrame = GetGenesisSonicStartupCheckpointEndFrame();
				if (startupFrame <= endFrame && (startupFrame % interval) == 0 && startupFrame != _sonicStartupLastCheckpointFrame) {
					_sonicStartupLastCheckpointFrame = startupFrame;
					_sonicStartupLastCheckpointPc = _cpu->GetState().PC & 0x00ffffff;
					_sonicStartupCheckpointCount++;
					MessageManager::Log(std::format("[Genesis] Sonic startup checkpoint #{} frame={} pc=${:06x} cycles={} loop={} startup={} mmuFlow={}",
						_sonicStartupCheckpointCount,
						startupFrame,
						_sonicStartupLastCheckpointPc,
						_cpu->GetState().CycleCount,
						_cpu->BuildSamePcLoopSummary(),
						BuildGenesisStartupRuntimeSummary(_memoryManager.get()),
						_memoryManager->BuildRuntimeFlowTraceSummary()));
				}
			}
		}
	}

	bool emitFrameEvents = _emu && _emu->IsEmulationThread();
	if (emitFrameEvents) {
		_emu->ProcessEvent(EventType::StartFrame, CpuType::Genesis);
	}

	uint32_t frame = _vdp->GetFrameCount();
	uint64_t startClock = _memoryManager->GetMasterClock();
	uint32_t guard = 0;
	uint64_t lastProgressClock = startClock;
	uint64_t lastProgressCpuCycles = _cpu->GetState().CycleCount;
	uint32_t stagnantIterations = 0;
	uint32_t forcedAdvancePulses = 0;
	bool trappedSehFault = false;
	bool hardGuardAbort = false;
	uint32_t trappedSehCode = 0;
	constexpr uint32_t StagnantIterationThreshold = 250000;
	constexpr uint32_t ForcedAdvancePulseLimit = 8;
	const uint32_t hardInstructionCap = GetGenesisRunFrameInstructionCap();
	bool sehGuardEnabled = IsGenesisSehDiagGuardEnabled();
	auto runFrameLoop = [&]() {
		while (frame == _vdp->GetFrameCount()) {
			if (sehGuardEnabled) {
				if (!TryExecWithSehGuard(_cpu.get(), trappedSehCode)) {
					trappedSehFault = true;
					break;
				}
			} else {
				_cpu->Exec();
			}
			guard++;
			if (guard >= hardInstructionCap) {
				hardGuardAbort = true;
				_cpu->ArmAggressiveFlowTrace(120000, 1, 192);
				_memoryManager->ArmAggressiveTraceBurst(180000, 260000, 1, 1, 192, 256);
				_runFrameStallEventCount++;
				_runFrameLastStallSummary = _cpu->BuildExecutionStallSummary();
				if (_runFrameFirstFailureBoundarySummary.empty()) {
					_runFrameFirstFailureBoundarySummary = _cpu->BuildDispatchBoundaryProbeSummary();
					_runFrameFirstFailureBoundaryCaptureCount++;
				}
				MessageManager::Log(std::format("[Genesis] RunFrame hard-guard abort frame={} guard={} cap={} pc=${:06x} cycles={} stall={} cpuBoundary={} cpuTrace={} cpuLoop={} cpuAddr={} startup={} mmuFlow={} mmuOps={} mmuOpsWindow={}",
					frame,
					guard,
					hardInstructionCap,
					_cpu->GetState().PC & 0x00ffffff,
					_cpu->GetState().CycleCount,
					_runFrameLastStallSummary,
					_runFrameFirstFailureBoundarySummary,
					_cpu->BuildInstructionTraceWindow(10),
					_cpu->BuildSamePcLoopSummary(),
					_cpu->BuildAddressErrorSummary(),
					BuildGenesisStartupRuntimeSummary(_memoryManager.get()),
					_memoryManager->BuildRuntimeFlowTraceSummary(),
					_memoryManager->BuildRuntimeOpTraceSummary(),
					_memoryManager->BuildRuntimeOpTraceWindow(10)));
				break;
			}

			uint64_t currentClock = _memoryManager->GetMasterClock();
			uint64_t currentCpuCycles = _cpu->GetState().CycleCount;
			if (currentClock == lastProgressClock && currentCpuCycles == lastProgressCpuCycles) {
				stagnantIterations++;
			} else {
				stagnantIterations = 0;
				lastProgressClock = currentClock;
				lastProgressCpuCycles = currentCpuCycles;
			}

			if (stagnantIterations >= StagnantIterationThreshold) {
				_cpu->ArmAggressiveFlowTrace(90000, 1, 160);
				_memoryManager->ArmAggressiveTraceBurst(140000, 220000, 1, 1, 160, 224);
				_runFrameStallEventCount++;
				forcedAdvancePulses++;
				_cpu->ForceClockAdvance(488);
				_runFrameForcedAdvanceCount++;
				_runFrameLastStallSummary = _cpu->BuildExecutionStallSummary();
				MessageManager::Log(std::format("[Genesis] RunFrame stall recovery #{} pulse={} frame={} guard={} summary={} loop={} masterClock={} cpuCycles={}",
					_runFrameStallEventCount,
					forcedAdvancePulses,
					frame,
					guard,
					_runFrameLastStallSummary,
					_cpu->BuildSamePcLoopSummary(),
					_memoryManager->GetMasterClock(),
					_cpu->GetState().CycleCount));
				stagnantIterations = 0;
				lastProgressClock = _memoryManager->GetMasterClock();
				lastProgressCpuCycles = _cpu->GetState().CycleCount;

				if (forcedAdvancePulses >= ForcedAdvancePulseLimit) {
					if (_runFrameFirstFailureBoundarySummary.empty()) {
						_runFrameFirstFailureBoundarySummary = _cpu->BuildDispatchBoundaryProbeSummary();
						_runFrameFirstFailureBoundaryCaptureCount++;
						MessageManager::Log(std::format("[Genesis] RunFrame first-failure pair cpuBoundary={} mmuFlow={} mmuOps={} mmuOpsWindow={}",
							_runFrameFirstFailureBoundarySummary,
							_memoryManager->BuildRuntimeFlowTraceSummary(),
							_memoryManager->BuildRuntimeOpTraceSummary(),
							_memoryManager->BuildRuntimeOpTraceWindow(8)));
					}
					MessageManager::Log(std::format("[Genesis] RunFrame forced completion frame={} guard={} pulses={} traceDigest={}",
						frame,
						guard,
						forcedAdvancePulses,
						_cpu->BuildInstructionTraceDigest()));
					MessageManager::Log(std::format("[Genesis] RunFrame forced completion detail cpuTrace={} cpuLoop={} startup={} mmuOpsWindow={}",
						_cpu->BuildInstructionTraceWindow(8),
						_cpu->BuildSamePcLoopSummary(),
						BuildGenesisStartupRuntimeSummary(_memoryManager.get()),
						_memoryManager->BuildRuntimeOpTraceWindow(8)));
					break;
				}
			}

			if ((guard % 50000) == 0) {
				GenesisVdpState vdpState = _vdp->GetState();
				GenesisIoState ioState = _memoryManager->GetIoState();
				MessageManager::Log(std::format("[Genesis] RunFrame waiting frame={} guard={} pc=${:06x} cycles={} masterClock={} heartbeatPc=${:06x} heartbeatCycles={} heartbeatInstr={} z80Running={} z80RunnableCycles={} z80StalledCycles={} z80Transitions={} z80Epoch={} z80LastTransitionClock={} vdpVc={} vdpHc={} vdpStatus=${:04x} r1=${:02x} dmaActive={} dmaMode={} cpuLoop={} cpuAddr={}",
					frame,
					guard,
					_cpu->GetState().PC & 0x00ffffff,
					_cpu->GetState().CycleCount,
					_memoryManager->GetMasterClock(),
					ioState.CpuProgramCounterHeartbeat,
					ioState.CpuCycleHeartbeat,
					ioState.CpuInstructionHeartbeat,
					_memoryManager->GetZ80RuntimeRunning() ? 1 : 0,
					_memoryManager->GetZ80RuntimeRunnableCycles(),
					_memoryManager->GetZ80RuntimeStalledCycles(),
					_memoryManager->GetZ80RuntimeTransitionCount(),
					_memoryManager->GetZ80RuntimeStateEpoch(),
					_memoryManager->GetZ80RuntimeLastTransitionClock(),
					vdpState.VCounter,
					vdpState.HCounter,
					vdpState.StatusRegister,
					vdpState.Registers[1],
					vdpState.DmaActive ? 1 : 0,
					vdpState.DmaMode,
					_cpu->BuildSamePcLoopSummary(),
					_cpu->BuildAddressErrorSummary()));
			}
		}
	};

	runFrameLoop();

	if (trappedSehFault) {
		_runFrameEarlyAbortCount++;
		if (_runFrameFirstFailureBoundarySummary.empty()) {
			_runFrameFirstFailureBoundarySummary = _cpu->BuildDispatchBoundaryProbeSummary();
			_runFrameFirstFailureBoundaryCaptureCount++;
		}
		_runFrameLastExitSummary = std::format("seh_guard_abort code=${:08x} frame={} guard={} pc=${:06x} cycles={} cpuBoundary={} mmuFlow={} mmuOps={} mmuOpsWindow={}",
			trappedSehCode,
			frame,
			guard,
			_cpu->GetState().PC & 0x00ffffff,
			_cpu->GetState().CycleCount,
			_runFrameFirstFailureBoundarySummary,
			_memoryManager->BuildRuntimeFlowTraceSummary(),
			_memoryManager->BuildRuntimeOpTraceSummary(),
			_memoryManager->BuildRuntimeOpTraceWindow(12));
		MessageManager::Log(std::format("[Genesis] RunFrame SEH trapped {}", _runFrameLastExitSummary));
		return;
	}
	if (hardGuardAbort) {
		_runFrameEarlyAbortCount++;
	}
	_runFrameLastGuardIterations = guard;

	uint32_t nextFrame = _vdp->GetFrameCount();
	if (nextFrame == frame) {
		// Last resort fallback: force enough master clocks to cross a full frame boundary.
		// One NTSC frame = 262 scanlines x 488 cycles = 127856 cycles; use 320 iterations
		// of 488 cycles (= 156160 cycles) to guarantee crossing both NTSC and PAL frames.
		for (uint32_t i = 0; i < 320 && nextFrame == frame; i++) {
			_cpu->ForceClockAdvance(488);
			nextFrame = _vdp->GetFrameCount();
			_runFrameForcedAdvanceCount++;
		}
	}
	if ((nextFrame % 120) == 0) {
		GenesisVdpState vdpState = _vdp->GetState();
		MessageManager::Log(std::format("[Genesis] Frame advanced to {} (deltaClock={}) display={} R1=${:02x} tmssUnlocked={}",
			nextFrame,
			_memoryManager->GetMasterClock() - startClock,
			(vdpState.Registers[1] & 0x40) ? "on" : "off",
			vdpState.Registers[1],
			_memoryManager->GetIoState().TmssUnlocked ? "true" : "false"));
	}

	if (emitFrameEvents) {
		_emu->ProcessEvent(EventType::EndFrame, CpuType::Genesis);
	}

	if (_emu && _emu->IsEmulationThread() && _vdp && _controlManager) {
		uint16_t* frameBuffer = _vdp->GetScreenBuffer(true);
		if (frameBuffer) {
			bool rewinding = _emu->GetRewindManager()->IsRewinding();
			RenderedFrame renderedFrame(
				frameBuffer,
				_vdp->GetScreenWidth(),
				_vdp->GetScreenHeight(),
				1.0,
				nextFrame,
				_controlManager->GetPortStates());
			_emu->GetVideoDecoder()->UpdateFrame(renderedFrame, rewinding, rewinding);
		}
	}

	ProcessEndOfFrame();
	_runFrameExitCount++;
	_runFrameLastExitSummary = std::format("exit={} frameBefore={} frameAfter={} guard={} hardGuardAbort={} stalls={} forcedAdvances={} sonicTraceArms={} sonicTraceLastArmFrame={} sonicCheckpoints={} sonicLastCheckpointFrame={} sonicLastCheckpointPc=${:06x} startupClass={} startupFrame={} pc=${:06x} cycles={} traceDigest={}",
		_runFrameExitCount,
		frame,
		nextFrame,
		guard,
		hardGuardAbort ? 1 : 0,
		_runFrameStallEventCount,
		_runFrameForcedAdvanceCount,
		_sonicTraceEscalationCount,
		_sonicTraceLastArmStartupFrame,
		_sonicStartupCheckpointCount,
		_sonicStartupLastCheckpointFrame,
		_sonicStartupLastCheckpointPc & 0x00ffffff,
		_memoryManager ? _memoryManager->GetStartupTitleClassValue() : 0,
		_memoryManager ? _memoryManager->GetStartupFrameForDiagnostics() : 0,
		_cpu->GetState().PC & 0x00ffffff,
		_cpu->GetState().CycleCount,
		_cpu->BuildInstructionTraceDigest());
}

void GenesisConsole::ProcessEndOfFrame() {
	if (!_controlManager) {
		return;
	}

	if (_emu && _emu->IsEmulationThread()) {
		_controlManager->UpdateControlDevices();
		_controlManager->UpdateInputState();
		_emu->ProcessEndOfFrame();
	} else {
		// Keep headless frame tests deterministic without requiring host input stack initialization.
		_controlManager->SetPollCounter(_controlManager->GetPollCounter() + 1);
		_controlManager->ProcessEndOfFrame();
	}
}

void GenesisConsole::SaveBattery() {
	if (_memoryManager) {
		_memoryManager->SaveBattery();
	}
}

BaseControlManager* GenesisConsole::GetControlManager() {
	return _controlManager.get();
}

ConsoleRegion GenesisConsole::GetRegion() {
	return _region;
}

ConsoleType GenesisConsole::GetConsoleType() {
	return ConsoleType::Genesis;
}

vector<CpuType> GenesisConsole::GetCpuTypes() {
	return {CpuType::Genesis};
}

RomFormat GenesisConsole::GetRomFormat() {
	return RomFormat::Genesis;
}

double GenesisConsole::GetFps() {
	return (_region == ConsoleRegion::Pal) ? 50.0 : 59.922743;
}

uint64_t GenesisConsole::GetMasterClock() {
	return _memoryManager->GetMasterClock();
}

uint32_t GenesisConsole::GetMasterClockRate() {
	// M68000 @ 7.670454 MHz (NTSC), 7.600489 MHz (PAL)
	return (_region == ConsoleRegion::Pal) ? 7600489 : 7670454;
}

PpuFrameInfo GenesisConsole::GetPpuFrame() {
	PpuFrameInfo frame = {};
	frame.FirstScanline = 0;
	frame.FrameCount = _vdp->GetFrameCount();
	frame.Width = _vdp->GetScreenWidth();
	frame.Height = _vdp->GetScreenHeight();
	frame.ScanlineCount = (_region == ConsoleRegion::Pal) ? 313 : 262;
	frame.CycleCount = 488; // Cycles per scanline at 68k rate
	frame.FrameBufferSize = frame.Width * frame.Height * sizeof(uint16_t);
	frame.FrameBuffer = (uint8_t*)_vdp->GetScreenBuffer(true);
	return frame;
}

BaseVideoFilter* GenesisConsole::GetVideoFilter(bool getDefaultFilter) {
	return new GenesisDefaultVideoFilter(_emu, this);
}

AudioTrackInfo GenesisConsole::GetAudioTrackInfo() {
	return AudioTrackInfo();
}

void GenesisConsole::ProcessAudioPlayerAction(AudioPlayerActionParams p) {
}

AddressInfo GenesisConsole::GetAbsoluteAddress(AddressInfo& relAddress) {
	return _memoryManager->GetAbsoluteAddress(relAddress.Address);
}

AddressInfo GenesisConsole::GetPcAbsoluteAddress() {
	return _memoryManager->GetAbsoluteAddress(_cpu->GetState().PC);
}

AddressInfo GenesisConsole::GetRelativeAddress(AddressInfo& absAddress, CpuType cpuType) {
	return {_memoryManager->GetRelativeAddress(absAddress), MemoryType::GenesisMemory};
}

GenesisState GenesisConsole::GetState() {
	GenesisState state;
	state.Cpu = _cpu->GetState();
	state.Vdp = _vdp->GetState();
	state.Io = _memoryManager->GetIoState();
	if (_psg) {
		state.Psg = _psg->GetState();
	}
	return state;
}

void GenesisConsole::GetConsoleState(BaseState& state, ConsoleType consoleType) {
	(GenesisState&)state = GetState();
}

void GenesisConsole::InitializeRam(void* data, uint32_t length) {
	EmuSettings* settings = _emu->GetSettings();
	settings->InitializeRam(settings->GetGenesisConfig().RamPowerOnState, data, length);
}

void GenesisConsole::Serialize(Serializer& s) {
	SV(_cpu);
	SV(_vdp);
	SV(_controlManager);
	SV(_memoryManager);
	SV(_psg);
}
