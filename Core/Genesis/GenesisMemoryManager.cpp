#include "pch.h"
#include "Genesis/GenesisMemoryManager.h"
#include "Genesis/GenesisConsole.h"
#include "Genesis/GenesisM68k.h"
#include "Genesis/GenesisVdp.h"
#include "Genesis/GenesisControlManager.h"
#include "Genesis/GenesisPsg.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"
#include "Shared/BatteryManager.h"
#include "Shared/MessageManager.h"
#include "Utilities/Serializer.h"
#include <algorithm>
#include <cctype>
#include <filesystem>

namespace {
	__forceinline uint32_t ReadBe32(const vector<uint8_t>& data, size_t offset) {
		size_t effectiveOffset = offset;
		uint32_t byte0 = (uint32_t)data[effectiveOffset];
		uint32_t byte1 = (uint32_t)data[effectiveOffset + 1];
		uint32_t byte2 = (uint32_t)data[effectiveOffset + 2];
		uint32_t byte3 = (uint32_t)data[effectiveOffset + 3];
		uint32_t value = (byte0 << 24)
			| (byte1 << 16)
			| (byte2 << 8)
			| byte3;
		return value;
	}

	__forceinline bool IsZ80BusReqAddress(uint32_t addr) {
		uint32_t effectiveAddr = addr;
		bool isBusReqAddress = (effectiveAddr & 0xFFFF00) == 0xA11100;
		return isBusReqAddress;
	}

	__forceinline bool IsZ80ResetAddress(uint32_t addr) {
		uint32_t effectiveAddr = addr;
		bool isResetAddress = (effectiveAddr & 0xFFFF00) == 0xA11200;
		return isResetAddress;
	}

	__forceinline bool IsYm2612Address(uint32_t addr) {
		uint32_t effectiveAddr = addr & 0xFFFFFF;
		bool isYm2612Address = effectiveAddr >= 0xA04000 && effectiveAddr <= 0xA04003;
		return isYm2612Address;
	}

	__forceinline uint8_t GetZ80BusAckStatusBit(bool busAck) {
		// BUSACK bit is low when 68k currently owns the Z80 bus.
		uint8_t ackStatus = busAck ? 0x00 : 0x01;
		return ackStatus;
	}

	__forceinline bool IsTmssAddress(uint32_t addr) {
		uint32_t effectiveAddr = addr;
		bool isTmssAddress = effectiveAddr >= 0xA14000 && effectiveAddr <= 0xA14003;
		return isTmssAddress;
	}

	__forceinline bool IsTmssCartAddress(uint32_t addr) {
		uint32_t effectiveAddr = addr;
		return effectiveAddr == 0xA14101;
	}

	__forceinline bool IsLegacyBridgePassThroughAddress(uint32_t addr) {
		return (addr >= 0xA13000 && addr <= 0xA1301F)
			|| (addr >= 0xA14000 && addr <= 0xA1401F);
	}

	__forceinline bool IsBridgeControlReadbackAddress(uint32_t addr) {
		return (addr >= 0xA12002 && addr <= 0xA12005)
			|| (addr >= 0xA12012 && addr <= 0xA12015)
			|| addr == 0xA15012 || addr == 0xA15013 || addr == 0xA15014
			|| addr == 0xA15016 || addr == 0xA15017
			|| (addr >= 0xA15008 && addr <= 0xA1500B)
			|| (addr >= 0xA16012 && addr <= 0xA16015)
			|| (addr >= 0xA18008 && addr <= 0xA1800B);
	}

	__forceinline bool IsBridgeModeledWriteAddress(uint32_t addr) {
		return addr == 0xA12000 || addr == 0xA12001
			|| IsBridgeControlReadbackAddress(addr)
			|| IsLegacyBridgePassThroughAddress(addr);
	}

	__forceinline uint8_t BuildVersionRegister(ConsoleRegion region) {
		uint8_t versionByte = 0xA0; // Base hardware profile
		if (region == ConsoleRegion::NtscJapan) {
			versionByte &= (uint8_t)~0x80; // Domestic
		} else {
			versionByte |= 0x80; // Overseas
		}

		if (region == ConsoleRegion::Pal) {
			versionByte |= 0x40;
		} else {
			versionByte &= (uint8_t)~0x40;
		}
		return versionByte;
	}

	static std::string sNexenWramTracePath = "reference/cpu_ram_trace.log";
	static std::string sNexenStartupTracePath = "reference/genesis_startup_trace.log";
	static FILE* sNexenWramTraceFile = nullptr;
	static FILE* sNexenStartupTraceFile = nullptr;
	static uint32_t sNexenWramTraceLines = 0;
	static uint32_t sNexenStartupTraceLines = 0;
	static bool sNexenWramTraceConfigLoaded = false;
	static bool sNexenStartupTraceConfigLoaded = false;
	static uint32_t sNexenWramTraceFrameStart = 0u;
	static uint32_t sNexenWramTraceFrameEnd = 50u;
	static uint32_t sNexenWramTraceAddrStart = 0xFFCC00u;
	static uint32_t sNexenWramTraceAddrEnd = 0xFFCFFFu;
	static uint32_t sNexenWramTraceMaxLines = 300000u;
	static bool sNexenStartupTraceEnabled = true;
	static uint32_t sNexenStartupTraceFrameEnd = 600u;
	static uint32_t sNexenStartupTraceMaxLines = 50000u;
	static bool sNexenGenesisTmssStrictMode = false;
	static std::string sNexenGenesisStartupProfile = "logo-compat";
	static uint32_t sNexenGenesisStartupWindowFrames = 16u;
	static uint32_t sNexenGenesisStartupCheckpointIntervalFrames = 1u;
	static uint32_t sNexenGenesisStartupCheckpointEndFrame = 600u;
	static uint16_t sNexenGenesisZ80BusReqAckDelayMclk = 7u;
	static uint16_t sNexenGenesisZ80BusResumeDelayMclk = 7u;
	static bool sNexenGenesisZ80LatchOnlyHighByteWrites = true;
	static bool sNexenGenesisPreferMesenBusHandoff = true;
	static bool sNexenGenesisPowerOnZ80ResetAsserted = true;

	static bool TryGetNexenTracePathFromEnv(const char* name, std::string& outPath) {
		const char* raw = std::getenv(name);
		if (!raw || !*raw) {
			return false;
		}

		outPath = raw;
		return !outPath.empty();
	}

	static void EnsureNexenTraceDirectoryForPath(const std::string& tracePath) {
		if (tracePath.empty()) {
			return;
		}

		std::filesystem::path path = std::filesystem::path(tracePath);
		std::filesystem::path parent = path.parent_path();
		if (parent.empty()) {
			return;
		}

		std::error_code fsError;
		std::filesystem::create_directories(parent, fsError);
	}

	static bool TryParseNexenTraceEnvU32AutoBase(const char* name, uint32_t minValue, uint32_t maxValue, uint32_t& outValue) {
		const char* raw = std::getenv(name);
		if (!raw || !*raw) {
			return false;
		}

		char* end = nullptr;
		unsigned long parsed = std::strtoul(raw, &end, 0);
		if (end == raw || *end != '\0' || parsed < minValue || parsed > maxValue) {
			return false;
		}

		outValue = (uint32_t)parsed;
		return true;
	}

	static bool TryParseNexenTraceEnvBool(const char* name, bool& outValue) {
		uint32_t value = 0;
		if (!TryParseNexenTraceEnvU32AutoBase(name, 0u, 1u, value)) {
			return false;
		}

		outValue = value != 0;
		return true;
	}

	static bool TryGetNexenTraceEnvLowerString(const char* name, std::string& outValue) {
		const char* raw = std::getenv(name);
		if (!raw || !*raw) {
			return false;
		}

		outValue = raw;
		std::transform(outValue.begin(), outValue.end(), outValue.begin(), [](unsigned char c) {
			return (char)std::tolower(c);
		});
		return !outValue.empty();
	}

	static void LoadNexenWramTraceConfigFromEnv() {
		if (sNexenWramTraceConfigLoaded) {
			return;
		}
		sNexenWramTraceConfigLoaded = true;

		uint32_t value = 0;
		if (TryParseNexenTraceEnvU32AutoBase("NEXEN_WRAM_FRAME_START", 0u, 0xFFFFFFFFu, value)) {
			sNexenWramTraceFrameStart = value;
		}
		if (TryParseNexenTraceEnvU32AutoBase("NEXEN_WRAM_FRAME_END", 0u, 0xFFFFFFFFu, value)) {
			sNexenWramTraceFrameEnd = value;
		}
		if (TryParseNexenTraceEnvU32AutoBase("NEXEN_WRAM_ADDR_START", 0u, 0xFFFFFFu, value)) {
			sNexenWramTraceAddrStart = value;
		}
		if (TryParseNexenTraceEnvU32AutoBase("NEXEN_WRAM_ADDR_END", 0u, 0xFFFFFFu, value)) {
			sNexenWramTraceAddrEnd = value;
		}
		if (TryParseNexenTraceEnvU32AutoBase("NEXEN_WRAM_MAX_LINES", 1u, 0xFFFFFFFFu, value)) {
			sNexenWramTraceMaxLines = value;
		}
		std::string tracePath;
		if (TryGetNexenTracePathFromEnv("NEXEN_WRAM_TRACE_PATH", tracePath)) {
			sNexenWramTracePath = tracePath;
		}

		if (sNexenWramTraceFrameStart > sNexenWramTraceFrameEnd) {
			std::swap(sNexenWramTraceFrameStart, sNexenWramTraceFrameEnd);
		}
		if (sNexenWramTraceAddrStart > sNexenWramTraceAddrEnd) {
			std::swap(sNexenWramTraceAddrStart, sNexenWramTraceAddrEnd);
		}
	}

	static void LoadNexenStartupTraceConfigFromEnv() {
		sNexenStartupTraceConfigLoaded = true;

		// Profile defaults: tuned for startup/logo validation in the first 10 seconds.
		sNexenGenesisStartupProfile = "logo-compat";
		sNexenGenesisStartupWindowFrames = 16u;
		sNexenGenesisStartupCheckpointIntervalFrames = 1u;
		sNexenGenesisStartupCheckpointEndFrame = 600u;
		sNexenGenesisZ80BusReqAckDelayMclk = 7u;
		sNexenGenesisZ80BusResumeDelayMclk = 7u;
		sNexenGenesisZ80LatchOnlyHighByteWrites = true;
		sNexenGenesisPreferMesenBusHandoff = true;
		sNexenGenesisPowerOnZ80ResetAsserted = true;

		uint32_t value = 0;
		if (TryParseNexenTraceEnvU32AutoBase("NEXEN_GENESIS_STARTUP_TRACE", 0u, 1u, value)) {
			sNexenStartupTraceEnabled = value != 0;
		}
		if (TryParseNexenTraceEnvU32AutoBase("NEXEN_GENESIS_STARTUP_TRACE_FRAME_END", 0u, 0xFFFFFFFFu, value)) {
			sNexenStartupTraceFrameEnd = value;
		}
		if (TryParseNexenTraceEnvU32AutoBase("NEXEN_GENESIS_STARTUP_TRACE_MAX_LINES", 1u, 0xFFFFFFFFu, value)) {
			sNexenStartupTraceMaxLines = value;
		}
		if (TryParseNexenTraceEnvU32AutoBase("NEXEN_GENESIS_TMSS_STRICT", 0u, 1u, value)) {
			sNexenGenesisTmssStrictMode = value != 0;
		}

		std::string startupProfile;
		if (TryGetNexenTraceEnvLowerString("NEXEN_GENESIS_STARTUP_PROFILE", startupProfile)) {
			sNexenGenesisStartupProfile = startupProfile;
		}

		if (sNexenGenesisStartupProfile == "strict" || sNexenGenesisStartupProfile == "strict-startup") {
			sNexenGenesisStartupWindowFrames = 0u;
			sNexenGenesisStartupCheckpointIntervalFrames = 2u;
			sNexenGenesisStartupCheckpointEndFrame = 120u;
			sNexenGenesisPreferMesenBusHandoff = false;
			sNexenGenesisTmssStrictMode = true;
		} else if (sNexenGenesisStartupProfile == "mesen" || sNexenGenesisStartupProfile == "mesen-startup") {
			sNexenGenesisStartupWindowFrames = 10u;
			sNexenGenesisStartupCheckpointIntervalFrames = 1u;
			sNexenGenesisStartupCheckpointEndFrame = 600u;
			sNexenGenesisPreferMesenBusHandoff = true;
		} else {
			// logo-compat and unknown values fall back to compatibility-first behavior.
			sNexenGenesisStartupProfile = "logo-compat";
		}

		if (TryParseNexenTraceEnvU32AutoBase("NEXEN_GENESIS_STARTUP_WINDOW_FRAMES", 0u, 120u, value)) {
			sNexenGenesisStartupWindowFrames = value;
		}
		if (TryParseNexenTraceEnvU32AutoBase("NEXEN_GENESIS_STARTUP_CHECKPOINT_INTERVAL_FRAMES", 1u, 120u, value)) {
			sNexenGenesisStartupCheckpointIntervalFrames = value;
		}
		if (TryParseNexenTraceEnvU32AutoBase("NEXEN_GENESIS_STARTUP_CHECKPOINT_END_FRAME", 0u, 1800u, value)) {
			sNexenGenesisStartupCheckpointEndFrame = value;
		}
		if (TryParseNexenTraceEnvU32AutoBase("NEXEN_GENESIS_Z80_BUSREQ_ACK_DELAY_MCLK", 0u, 255u, value)) {
			sNexenGenesisZ80BusReqAckDelayMclk = (uint16_t)value;
		}
		if (TryParseNexenTraceEnvU32AutoBase("NEXEN_GENESIS_Z80_BUSRESUME_DELAY_MCLK", 0u, 255u, value)) {
			sNexenGenesisZ80BusResumeDelayMclk = (uint16_t)value;
		}
		TryParseNexenTraceEnvBool("NEXEN_GENESIS_Z80_LATCH_HIGH_BYTE_ONLY", sNexenGenesisZ80LatchOnlyHighByteWrites);
		TryParseNexenTraceEnvBool("NEXEN_GENESIS_PREFER_MESEN_BUS_HANDOFF", sNexenGenesisPreferMesenBusHandoff);
		TryParseNexenTraceEnvBool("NEXEN_GENESIS_POWERON_Z80_RESET_ASSERTED", sNexenGenesisPowerOnZ80ResetAsserted);

		std::string tracePath;
		if (TryGetNexenTracePathFromEnv("NEXEN_GENESIS_STARTUP_TRACE_PATH", tracePath)) {
			sNexenStartupTracePath = tracePath;
		}

		if (sNexenStartupTraceFrameEnd < sNexenGenesisStartupCheckpointEndFrame) {
			sNexenStartupTraceFrameEnd = sNexenGenesisStartupCheckpointEndFrame;
		}
	}

	static void EnsureNexenWramTraceOpen() {
		if (sNexenWramTraceFile) {
			return;
		}

		LoadNexenWramTraceConfigFromEnv();
		EnsureNexenTraceDirectoryForPath(sNexenWramTracePath);
		sNexenWramTraceFile = fopen(sNexenWramTracePath.c_str(), "w");
		if (sNexenWramTraceFile) {
			fprintf(sNexenWramTraceFile, "# CPU work-RAM write trace\n");
			fprintf(sNexenWramTraceFile, "# tracePath=%s\n", sNexenWramTracePath.c_str());
			fprintf(sNexenWramTraceFile, "# frameRange=%u-%u addrRange=%06X-%06X maxLines=%u\n",
				sNexenWramTraceFrameStart,
				sNexenWramTraceFrameEnd,
				(unsigned)sNexenWramTraceAddrStart,
				(unsigned)sNexenWramTraceAddrEnd,
				sNexenWramTraceMaxLines);
			fprintf(sNexenWramTraceFile, "F0000 L000 WRAM_BOOT addr=000000 data=00 pc=000000 mclk=0\n");
			sNexenWramTraceLines++;
			fflush(sNexenWramTraceFile);
		}
	}

	static void EnsureNexenStartupTraceOpen() {
		if (sNexenStartupTraceFile) {
			return;
		}

		LoadNexenStartupTraceConfigFromEnv();
		if (!sNexenStartupTraceEnabled) {
			return;
		}

		EnsureNexenTraceDirectoryForPath(sNexenStartupTracePath);
		sNexenStartupTraceFile = fopen(sNexenStartupTracePath.c_str(), "w");
		if (sNexenStartupTraceFile) {
			fprintf(sNexenStartupTraceFile, "# Genesis startup trace\n");
			fprintf(sNexenStartupTraceFile, "# tracePath=%s\n", sNexenStartupTracePath.c_str());
			fprintf(sNexenStartupTraceFile, "# frameEnd=%u maxLines=%u\n",
				sNexenStartupTraceFrameEnd,
				sNexenStartupTraceMaxLines);
			fflush(sNexenStartupTraceFile);
		}
	}

	static bool ShouldLogNexenWramTrace(uint32_t frame, uint32_t address) {
		EnsureNexenWramTraceOpen();
		if (!sNexenWramTraceFile) {
			return false;
		}
		if (sNexenWramTraceLines >= sNexenWramTraceMaxLines) {
			return false;
		}
		if (frame < sNexenWramTraceFrameStart || frame > sNexenWramTraceFrameEnd) {
			return false;
		}
		if (address < sNexenWramTraceAddrStart || address > sNexenWramTraceAddrEnd) {
			return false;
		}
		return true;
	}

	static void LogNexenWramTrace(uint32_t frame, uint16_t line, uint32_t address, uint8_t data, uint32_t programCounter, uint64_t masterClock) {
		if (!sNexenWramTraceFile) {
			return;
		}

		fprintf(sNexenWramTraceFile, "F%04u L%03u WRAM addr=%06X data=%02X pc=%06X mclk=%llu\n",
			(unsigned)frame,
			(unsigned)line,
			(unsigned)address,
			(unsigned)data,
			(unsigned)programCounter,
			(unsigned long long)masterClock);
		sNexenWramTraceLines++;
		if ((sNexenWramTraceLines & 0x3FFu) == 0u) {
			fflush(sNexenWramTraceFile);
		}
	}

	static bool ShouldLogNexenStartupTrace(uint32_t frame) {
		EnsureNexenStartupTraceOpen();
		if (!sNexenStartupTraceFile) {
			return false;
		}
		if (sNexenStartupTraceLines >= sNexenStartupTraceMaxLines) {
			return false;
		}
		return frame <= sNexenStartupTraceFrameEnd;
	}

	static bool ShouldTraceStartupLoopPoll(uint32_t frame, uint32_t pc) {
		if (pc < 0x071f80u || pc > 0x072040u) {
			return false;
		}

		static uint32_t sLoopPollTraceFrame = 0xFFFFFFFFu;
		static uint32_t sLoopPollTraceCount = 0u;
		if (sLoopPollTraceFrame != frame) {
			sLoopPollTraceFrame = frame;
			sLoopPollTraceCount = 0u;
		}

		if (sLoopPollTraceCount >= 96u) {
			return false;
		}

		sLoopPollTraceCount++;
		return true;
	}

	static bool ShouldTraceStartupPreLoopBranch(uint32_t frame, uint32_t pc) {
		if (pc < 0x071f80u || pc > 0x071fa0u) {
			return false;
		}

		static uint32_t sPreLoopTraceFrame = 0xFFFFFFFFu;
		static uint32_t sPreLoopTraceCount = 0u;
		if (sPreLoopTraceFrame != frame) {
			sPreLoopTraceFrame = frame;
			sPreLoopTraceCount = 0u;
		}

		if (sPreLoopTraceCount >= 64u) {
			return false;
		}

		sPreLoopTraceCount++;
		return true;
	}

	static void LogNexenStartupTrace(uint32_t frame, uint16_t line, const char* tag, uint32_t address, uint16_t value, uint16_t auxValue, uint32_t pc, uint64_t mclk) {
		if (!sNexenStartupTraceFile) {
			return;
		}

		fprintf(sNexenStartupTraceFile, "F%04u L%03u %s addr=%06X data=%04X aux=%u pc=%06X mclk=%llu\n",
			(unsigned)frame,
			(unsigned)line,
			tag,
			(unsigned)(address & 0x00FFFFFFu),
			(unsigned)value,
			(unsigned)auxValue,
			(unsigned)(pc & 0x00FFFFFFu),
			(unsigned long long)mclk);
		sNexenStartupTraceLines++;
		// Flush every line so traces survive force-stop smoke runs.
		fflush(sNexenStartupTraceFile);
	}

	static uint16_t ComputeStartupPaletteDigest(const uint16_t* cram) {
		if (!cram) {
			return 0;
		}

		uint16_t digest = 0x4d47;
		for (uint32_t i = 0; i < 16; i++) {
			digest = (uint16_t)((digest << 3) | (digest >> 13));
			digest ^= cram[i];
			digest = (uint16_t)(digest * 33u + (uint16_t)i);
		}

		return digest;
	}

	static bool StartupTagEquals(const char* tag, const char* literal) {
		return std::strcmp(tag, literal) == 0;
	}

	static bool ShouldEmitMesenProfileStartupTag(const char* tag) {
		return StartupTagEquals(tag, "STARTUP_BOOT")
			|| StartupTagEquals(tag, "STARTUP_CHECKPOINT")
			|| StartupTagEquals(tag, "STARTUP_Z80")
			|| StartupTagEquals(tag, "STARTUP_PAL")
			|| StartupTagEquals(tag, "STARTUP_VDP")
			|| StartupTagEquals(tag, "VDP_DISP_TGL")
			|| StartupTagEquals(tag, "Z80_RUN_TGL")
			|| StartupTagEquals(tag, "Z80_BUSREQ")
			|| StartupTagEquals(tag, "Z80_RESET")
			|| StartupTagEquals(tag, "VDP_REG_W")
			|| StartupTagEquals(tag, "VDP_STAT_W")
			|| StartupTagEquals(tag, "TMSS_UNLOCK");
	}
}

GenesisMemoryManager::GenesisMemoryManager() {
}

GenesisMemoryManager::~GenesisMemoryManager() {
}

void GenesisMemoryManager::Init(Emulator* emu, GenesisConsole* console, vector<uint8_t>& romData, GenesisVdp* vdp, GenesisControlManager* controlManager, GenesisPsg* psg) {
	_emu = emu;
	_console = console;
	_vdp = vdp;
	_controlManager = controlManager;
	_psg = psg;
	EnsureNexenWramTraceOpen();
	EnsureNexenStartupTraceOpen();
	TraceStartupEvent("STARTUP_BOOT", 0x000000, 0, 0);

	// Register and allocate ROM
	_prgRomSize = (uint32_t)romData.size();
	_prgRom = new uint8_t[_prgRomSize];
	memcpy(_prgRom, romData.data(), _prgRomSize);
	_emu->RegisterMemory(MemoryType::GenesisPrgRom, _prgRom, _prgRomSize);

	// Register and allocate work RAM
	_workRam = new uint8_t[WorkRamSize];
	memset(_workRam, 0, WorkRamSize);
	_emu->RegisterMemory(MemoryType::GenesisWorkRam, _workRam, WorkRamSize);

	// Register and allocate Z80 RAM
	_z80Ram = new uint8_t[Z80RamSize];
	memset(_z80Ram, 0, Z80RamSize);

	// I/O defaults
	memset(&_ioState, 0, sizeof(_ioState));
	_ioState.CtrlPort[0] = 0;
	_ioState.CtrlPort[1] = 0;
	_ioState.CtrlPort[2] = 0;
	memset(_segaCdBridgeA120, 0, sizeof(_segaCdBridgeA120));
	memset(_segaCdBridgeA130, 0, sizeof(_segaCdBridgeA130));
	memset(_segaCdBridgeA140, 0, sizeof(_segaCdBridgeA140));
	memset(_segaCdBridgeA150, 0, sizeof(_segaCdBridgeA150));
	memset(_segaCdBridgeA160, 0, sizeof(_segaCdBridgeA160));
	memset(_segaCdBridgeA180, 0, sizeof(_segaCdBridgeA180));

	_z80BusRequest = false;
	_z80Reset = sNexenGenesisPowerOnZ80ResetAsserted;
	_z80BusAck = false;
	_z80BusReqDelayMclk = 0;
	_z80ResumeDelayMclk = 0;
	_z80RuntimeRunning = false;
	_z80RuntimeRunnableCycles = 0;
	_z80RuntimeStalledCycles = 0;
	_z80RuntimeTransitionCount = 0;
	_z80RuntimeStateEpoch = 0;
	_z80RuntimeLastTransitionClock = 0;
	ApplyStartupEnvironmentProfile();
	_z80Reset = sNexenGenesisPowerOnZ80ResetAsserted;
	_startupLastDisplayEnabled = _vdp ? ((_vdp->GetState().Registers[VdpReg::ModeSet2] & 0x40) != 0) : false;
	UpdateZ80RuntimeState(false, 0, 0, "init");
	_tmssEnabled = _emu->GetSettings()->GetGenesisConfig().EnableTmss;
	_tmssUnlocked = false;
	_tmssVdpBlockLogged = false;
	_segaCdSubCpuRunning = false;
	_segaCdSubCpuBusRequest = false;
	_segaCdSubCpuTransitionCount = 0;
	_segaCdPcmLeft = 0;
	_segaCdPcmRight = 0;
	_segaCdCddaLeft = 0;
	_segaCdCddaRight = 0;
	_segaCdMixedLeft = 0;
	_segaCdMixedRight = 0;
	_segaCdAudioCheckpointCount = 0;
	_segaCdToolingDebuggerSignal = 0;
	_segaCdToolingTasSignal = 0;
	_segaCdToolingSaveStateSignal = 0;
	_segaCdToolingCheatSignal = 0;
	_segaCdToolingEventCount = 0;
	_segaCdToolingDigest = 0;
	_m32xMasterSh2Running = false;
	_m32xSlaveSh2Running = false;
	_m32xSh2SyncPhase = 0;
	_m32xSh2Milestone = 0;
	_m32xSh2SyncEpoch = 0;
	_m32xSh2Digest = 0;
	_m32xCompositionBlend = 0;
	_m32xFrameSyncMarker = 0;
	_m32xFrameSyncEpoch = 0;
	_m32xCompositionDigest = 0;
	_m32xToolingDebuggerSignal = 0;
	_m32xToolingTasSignal = 0;
	_m32xToolingSaveStateSignal = 0;
	_m32xToolingCheatSignal = 0;
	_m32xToolingEventCount = 0;
	_m32xToolingDigest = 0;
	_m32xCoprocMasterSignal = 0;
	_m32xCoprocSlaveSignal = 0;
	_m32xCoprocPhaseSignal = 0;
	_m32xCoprocFenceSignal = 0;
	_m32xCoprocEventCount = 0;
	_m32xCoprocDigest = 0;
	_m32xCoprocEdgeCount = 0;
	_m32xCoprocPhaseEpoch = 0;
	_m32xCoprocFenceEpoch = 0;
	_m32xCoprocArbiterLatch = 0;
	_m32xHostDebuggerSignal = 0;
	_m32xHostTasSignal = 0;
	_m32xHostSaveStateSignal = 0;
	_m32xHostCheatSignal = 0;
	_m32xHostEventCount = 0;
	_m32xHostDigest = 0;
	_m32xHostEdgeCount = 0;
	_m32xHostCommandNonce = 0;
	_m32xHostAckToken = 0;
	_m32xHostDeterminismLatch = 0;

	_hasSram = false;
	_sramStart = 0;
	_sramEnd = 0;
	_ioState.DebugTranscriptLaneCount = 0;
	_ioState.DebugTranscriptLaneDigest = 0;
	_ioState.RomReadHeartbeat = 0;
	_ioState.TmssEnabled = _tmssEnabled ? 1 : 0;
	_ioState.TmssUnlocked = _tmssUnlocked ? 1 : 0;
	for (int i = 0; i < 4; i++) {
		_ioState.DebugTranscriptEntryAddress[i] = 0;
		_ioState.DebugTranscriptEntryValue[i] = 0;
		_ioState.DebugTranscriptEntryFlags[i] = 0;
	}
	_sramEvenBytes = true;
	_sramOddBytes = true;
	_saveRam = nullptr;
	_saveRamSize = 0;

	if (romData.size() >= 0x1BC && romData[0x1B0] == 'R' && romData[0x1B1] == 'A') {
		uint32_t start = ReadBe32(romData, 0x1B4) & 0xFFFFFF;
		uint32_t end = ReadBe32(romData, 0x1B8) & 0xFFFFFF;
		uint8_t type = romData[0x1B2];

		if (end >= start) {
			_sramStart = start;
			_sramEnd = end;
			_sramEvenBytes = true;
			_sramOddBytes = true;

			if (type == 0xB0) {
				_sramOddBytes = false;
			} else if (type == 0xB8) {
				_sramEvenBytes = false;
			}

			if (!_sramEvenBytes && !_sramOddBytes) {
				_sramEvenBytes = true;
				_sramOddBytes = true;
			}

			if (_sramEvenBytes && _sramOddBytes) {
				_saveRamSize = (_sramEnd - _sramStart) + 1;
			} else {
				_saveRamSize = ((_sramEnd - _sramStart) >> 1) + 1;
			}

			if (_saveRamSize > 0) {
				_saveRam = new uint8_t[_saveRamSize];
				memset(_saveRam, 0xFF, _saveRamSize);
				_hasSram = true;
			}
		}
	}

	ResetRomBankMapper();
}

void GenesisMemoryManager::ResetRomBankMapper() {
	uint32_t bankCount = (_prgRomSize + MapperWindowSize - 1) / MapperWindowSize;
	if (bankCount == 0) {
		bankCount = 1;
	}

	_romBankMapperEnabled = _prgRomSize > MapperWindowSize;
	for (uint32_t i = 0; i < MapperBankWindowCount; i++) {
		_romBankRegisters[i] = (uint8_t)((i + 1) % bankCount);
	}
}

void GenesisMemoryManager::UpdateExecutionHeartbeat(uint32_t instructionProgramCounter, uint64_t cycleCount) {
	EnsureNexenWramTraceOpen();
	EnsureNexenStartupTraceOpen();

	_ioState.CpuProgramCounterHeartbeat = instructionProgramCounter & 0x00ffffff;
	_ioState.CpuCycleHeartbeat = cycleCount;
	_ioState.CpuInstructionHeartbeat++;
	if (_cpu && _vdp) {
		uint32_t frame = _vdp->GetFrameCount();
		uint32_t pc = instructionProgramCounter & 0x00ffffffu;
		GenesisM68kState cpuState = _cpu->GetState();
		if (ShouldTraceStartupPreLoopBranch(frame, pc)) {
			uint16_t d0 = (uint16_t)(cpuState.D[0] & 0xFFFFu);
			uint16_t sr = cpuState.SR;
			TraceStartupEvent("CPU_PRELOOP", instructionProgramCounter, d0, sr);
		}
		if (ShouldTraceStartupLoopPoll(frame, pc)) {
			uint16_t d0 = (uint16_t)(cpuState.D[0] & 0xFFFFu);
			uint16_t d7 = (uint16_t)(cpuState.D[7] & 0xFFFFu);
			TraceStartupEvent("CPU_LOOP", instructionProgramCounter, d0, d7);
		}
	}
	if ((_ioState.CpuInstructionHeartbeat & 0xFFu) == 0u) {
		TraceStartupEvent("CPU_HB", instructionProgramCounter, (uint16_t)(cycleCount & 0xFFFFu), (uint16_t)(_ioState.CpuInstructionHeartbeat & 0xFFFFu));
	}
}

void GenesisMemoryManager::ApplyStartupEnvironmentProfile() {
	LoadNexenStartupTraceConfigFromEnv();
	_startupWindowFrames = sNexenGenesisStartupWindowFrames;
	_startupCheckpointIntervalFrames = std::max<uint32_t>(1u, sNexenGenesisStartupCheckpointIntervalFrames);
	_startupCheckpointEndFrame = sNexenGenesisStartupCheckpointEndFrame;
	_startupNextCheckpointFrame = 0;
	_startupDisplayTransitionCount = 0;
	_startupHasLastDisplayState = false;
	_startupHasLastZ80RunState = false;
	_startupLastZ80Running = false;
	_startupHasLastZ80BusReqState = false;
	_startupLastZ80BusReq = false;
	_startupHasLastZ80ResetState = false;
	_startupLastZ80Reset = false;
	_startupHasLastVdpRegs = false;
	memset(_startupLastVdpRegs, 0, sizeof(_startupLastVdpRegs));
	_startupLastVdpStatus = 0;
	_startupProfilePreferMesenBusHandoff = sNexenGenesisPreferMesenBusHandoff;
	_z80BusReqAckDelayMclkSetting = sNexenGenesisZ80BusReqAckDelayMclk;
	_z80BusResumeDelayMclkSetting = sNexenGenesisZ80BusResumeDelayMclk;
	_z80LatchOnlyHighByteWrites = sNexenGenesisZ80LatchOnlyHighByteWrites;
	_tmssStrictMode = sNexenGenesisTmssStrictMode;
}

void GenesisMemoryManager::EmitStartupTransitionMarkers() {
	if (!_vdp) {
		return;
	}

	uint32_t frame = _vdp->GetFrameCount();
	if (!ShouldLogNexenStartupTrace(frame)) {
		return;
	}

	GenesisVdpState state = _vdp->GetState();
	bool displayEnabled = (state.Registers[VdpReg::ModeSet2] & 0x40) != 0;
	bool z80Running = _z80RuntimeRunning;
	bool z80BusReq = _z80BusRequest;
	bool z80Reset = _z80Reset;

	if (!_startupHasLastDisplayState) {
		_startupHasLastDisplayState = true;
		_startupLastDisplayEnabled = displayEnabled;
	} else if (displayEnabled != _startupLastDisplayEnabled) {
		_startupLastDisplayEnabled = displayEnabled;
		_startupDisplayTransitionCount++;
		TraceStartupEvent("VDP_DISP_TGL", 0xC00004, (uint16_t)state.Registers[VdpReg::ModeSet2], (uint16_t)_startupDisplayTransitionCount);
	}

	if (!_startupHasLastZ80RunState) {
		_startupHasLastZ80RunState = true;
		_startupLastZ80Running = z80Running;
	} else if (z80Running != _startupLastZ80Running) {
		_startupLastZ80Running = z80Running;
		TraceStartupEvent("Z80_RUN_TGL", 0xA11100, z80Running ? 1u : 0u, (uint16_t)(_z80BusAck ? 1u : 0u));
	}

	if (!_startupHasLastZ80BusReqState) {
		_startupHasLastZ80BusReqState = true;
		_startupLastZ80BusReq = z80BusReq;
	} else if (z80BusReq != _startupLastZ80BusReq) {
		_startupLastZ80BusReq = z80BusReq;
		TraceStartupEvent("Z80_BUSREQ", 0xA11100, z80BusReq ? 1u : 0u, (uint16_t)(_z80BusAck ? 1u : 0u));
	}

	if (!_startupHasLastZ80ResetState) {
		_startupHasLastZ80ResetState = true;
		_startupLastZ80Reset = z80Reset;
	} else if (z80Reset != _startupLastZ80Reset) {
		_startupLastZ80Reset = z80Reset;
		TraceStartupEvent("Z80_RESET", 0xA11200, z80Reset ? 1u : 0u, (uint16_t)(z80BusReq ? 1u : 0u));
	}

	if (!_startupHasLastVdpRegs) {
		_startupHasLastVdpRegs = true;
		memcpy(_startupLastVdpRegs, state.Registers, sizeof(_startupLastVdpRegs));
		_startupLastVdpStatus = state.StatusRegister;
	} else {
		for (uint32_t i = 0; i < (uint32_t)sizeof(_startupLastVdpRegs); i++) {
			uint8_t currentValue = state.Registers[i];
			if (_startupLastVdpRegs[i] != currentValue) {
				uint16_t packed = (uint16_t)(((uint16_t)_startupLastVdpRegs[i] << 8) | currentValue);
				TraceStartupEvent("VDP_REG_W", 0xC00004, packed, (uint16_t)i);
				_startupLastVdpRegs[i] = currentValue;
			}
		}

		if (_startupLastVdpStatus != state.StatusRegister) {
			TraceStartupEvent("VDP_STAT_W", 0xC00004, state.StatusRegister, (uint16_t)(_startupLastVdpStatus ^ state.StatusRegister));
			_startupLastVdpStatus = state.StatusRegister;
		}
	}
}

void GenesisMemoryManager::EmitStartupCheckpointIfNeeded(const char* sourceTag) {
	if (!_vdp) {
		return;
	}

	uint32_t frame = _vdp->GetFrameCount();
	if (frame == 0u && _startupNextCheckpointFrame == 0u) {
		_startupNextCheckpointFrame = 1u;
		return;
	}

	if (frame > _startupCheckpointEndFrame || frame < _startupNextCheckpointFrame) {
		return;
	}

	// Align startup checkpoints to end-of-frame timing to reduce early line-0 noise.
	// This keeps cross-emulator comparisons focused on stable per-frame state.
	uint16_t totalLines = _vdp->GetTotalLines();
	uint16_t scanline = _vdp->GetScanline();
	if (totalLines > 0u && scanline < (uint16_t)(totalLines - 1u)) {
		return;
	}

	GenesisVdpState state = _vdp->GetState();
	bool displayEnabled = (state.Registers[VdpReg::ModeSet2] & 0x40) != 0;
	if (frame == 0u && _startupNextCheckpointFrame == 0u) {
		_startupLastDisplayEnabled = displayEnabled;
	}

	uint16_t startupFlags = 0;
	startupFlags |= _tmssEnabled ? 0x0001 : 0x0000;
	startupFlags |= _tmssUnlocked ? 0x0002 : 0x0000;
	startupFlags |= _z80BusRequest ? 0x0004 : 0x0000;
	startupFlags |= _z80Reset ? 0x0008 : 0x0000;
	startupFlags |= _z80BusAck ? 0x0010 : 0x0000;
	startupFlags |= _z80RuntimeRunning ? 0x0020 : 0x0000;
	startupFlags |= displayEnabled ? 0x0040 : 0x0000;
	startupFlags |= _startupProfilePreferMesenBusHandoff ? 0x0080 : 0x0000;

	uint16_t startupValue = (uint16_t)(((frame & 0x3FFu) << 6) | (state.VCounter & 0x003Fu));
	TraceStartupEvent("STARTUP_CHECKPOINT", 0xC00004, startupValue, startupFlags);
	TraceStartupEvent("STARTUP_Z80", 0xA11100, _z80BusReqDelayMclk, _z80ResumeDelayMclk);
	uint16_t paletteDigest = ComputeStartupPaletteDigest(_vdp->GetCramPointer());
	TraceStartupEvent("STARTUP_PAL", 0xC00000, paletteDigest, (uint16_t)(state.Registers[VdpReg::ModeSet2]));
	TraceStartupEvent("STARTUP_VDP", 0xC00004, state.StatusRegister, (uint16_t)(state.Registers[VdpReg::ModeSet1] | ((uint16_t)state.Registers[VdpReg::ModeSet2] << 8)));

	if (displayEnabled != _startupLastDisplayEnabled) {
		_startupDisplayTransitionCount++;
		TraceStartupEvent("VDP_DISP_TGL", 0xC00004, (uint16_t)state.Registers[VdpReg::ModeSet2], (uint16_t)_startupDisplayTransitionCount);
		_startupLastDisplayEnabled = displayEnabled;
	}

	if (_startupCheckpointIntervalFrames == 0) {
		_startupCheckpointIntervalFrames = 1;
	}
	_startupNextCheckpointFrame = frame + _startupCheckpointIntervalFrames;

	if (_startupTraceSequence <= 16u || (_startupTraceSequence % 512u) == 0u) {
		MessageManager::Log(std::format("[Genesis][MMU] startup checkpoint src={} frame={} v={} r1=${:02x} tmss={} unlocked={} z80Req={} z80Reset={} z80Ack={} running={} reqDelay={} resumeDelay={} displayTransitions={}",
			sourceTag,
			frame,
			state.VCounter,
			state.Registers[VdpReg::ModeSet2],
			_tmssEnabled ? 1 : 0,
			_tmssUnlocked ? 1 : 0,
			_z80BusRequest ? 1 : 0,
			_z80Reset ? 1 : 0,
			_z80BusAck ? 1 : 0,
			_z80RuntimeRunning ? 1 : 0,
			_z80BusReqDelayMclk,
			_z80ResumeDelayMclk,
			_startupDisplayTransitionCount));
	}
}

bool GenesisMemoryManager::IsTmssVdpLockEnforced() const {
	return _tmssEnabled && _tmssStrictMode && !_tmssUnlocked;
}

bool GenesisMemoryManager::IsStartupWindowActive() const {
	return _vdp && _vdp->GetFrameCount() < _startupWindowFrames;
}

bool GenesisMemoryManager::IsTmssLockedVdpReadAllowed(uint32_t addr) const {
	if (!IsTmssVdpLockEnforced()) {
		return true;
	}

	uint32_t port = addr & 0x1F;
	if (port >= 0x04 && port < 0x10) {
		// Allow status/HV polling while locked to stabilize startup loops.
		return true;
	}

	if (_startupProfilePreferMesenBusHandoff && port < 0x04) {
		// Compatibility path: allow early data/control reads while logo sequence initializes.
		return IsStartupWindowActive();
	}

	if (IsStartupWindowActive()) {
		// Startup compatibility window: allow early VDP reads during first frames.
		return true;
	}

	return false;
}

bool GenesisMemoryManager::IsTmssLockedVdpWriteAllowed(uint32_t addr) const {
	if (!IsTmssVdpLockEnforced()) {
		return true;
	}

	uint32_t port = addr & 0x1F;
	if (port >= 0x04 && port < 0x08) {
		// Allow register/control setup while TMSS is still settling.
		return true;
	}

	if (_startupProfilePreferMesenBusHandoff && port < 0x04) {
		// Compatibility path: allow data/control writes during startup logo setup.
		return IsStartupWindowActive();
	}

	if (IsStartupWindowActive()) {
		// Startup compatibility window: permit early writes in first frames.
		return true;
	}

	return false;
}

void GenesisMemoryManager::TraceStartupEvent(const char* tag, uint32_t addr, uint16_t value, uint16_t auxValue) {
	if (!_vdp) {
		return;
	}

	uint32_t frame = _vdp->GetFrameCount();
	if (!ShouldLogNexenStartupTrace(frame)) {
		return;
	}

	if (_startupProfilePreferMesenBusHandoff) {
		if (!ShouldEmitMesenProfileStartupTag(tag)) {
			return;
		}

		// Keep frame-0 emission limited to bootstrap to better match Mesen startup chronology.
		if (frame == 0u && !StartupTagEquals(tag, "STARTUP_BOOT")) {
			return;
		}
	}

	GenesisVdpState vdpState = _vdp->GetState();
	uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffff) : 0x000000;
	for (const uint8_t* p = (const uint8_t*)tag; *p; p++) {
		_startupTraceDigest ^= *p;
		_startupTraceDigest *= 1099511628211ull;
	}
	_startupTraceDigest ^= addr;
	_startupTraceDigest *= 1099511628211ull;
	_startupTraceDigest ^= value;
	_startupTraceDigest *= 1099511628211ull;
	_startupTraceDigest ^= auxValue;
	_startupTraceDigest *= 1099511628211ull;

	uint16_t line = _vdp->GetScanline();
	LogNexenStartupTrace(frame, line, tag, addr, value, auxValue, pc, _masterClock);
	_startupTraceSequence++;
}

void GenesisMemoryManager::EvaluateTmssUnlockState(bool allowLog, uint32_t addr, uint32_t value, bool isWrite) {
	bool signatureMatch = _segaCdBridgeA140[0] == 'S'
		&& _segaCdBridgeA140[1] == 'E'
		&& _segaCdBridgeA140[2] == 'G'
		&& _segaCdBridgeA140[3] == 'A';

	if (!_tmssEnabled) {
		_tmssUnlockPending = false;
		_tmssUnlockDelayMclk = 0;
		_tmssUnlocked = signatureMatch;
		_tmssVdpBlockLogged = false;
		_tmssStartupBypassLogged = false;
		_ioState.TmssEnabled = 0;
		_ioState.TmssUnlocked = _tmssUnlocked ? 1 : 0;
		return;
	}

	if (signatureMatch) {
		_tmssUnlockPending = false;
		_tmssUnlockDelayMclk = 0;
		if (!_tmssUnlocked) {
			_tmssUnlocked = true;
			_tmssVdpBlockLogged = false;
			_tmssStartupBypassLogged = false;
			if (allowLog) {
				MessageManager::Log(std::format("[Genesis][MMU] TMSS signature complete; unlock immediate ({} ${:06x}=${:02x})",
					isWrite ? "write" : "read",
					addr,
					value & 0xFF));
			}
			TraceStartupEvent("TMSS_UNLOCK", addr, (uint16_t)(value & 0xFFFF), 0);
		}
	} else {
		_tmssUnlocked = false;
		_tmssUnlockPending = false;
		_tmssUnlockDelayMclk = 0;
		_tmssVdpBlockLogged = false;
		_tmssStartupBypassLogged = false;
	}

	_ioState.TmssEnabled = _tmssEnabled ? 1 : 0;
	_ioState.TmssUnlocked = _tmssUnlocked ? 1 : 0;
	MessageManager::Log(std::format("[Genesis][MMU] TMSS mode enable={} strictLocking={} (strict toggled by NEXEN_GENESIS_TMSS_STRICT)",
		_tmssEnabled ? 1 : 0,
		_tmssStrictMode ? 1 : 0));
}

void GenesisMemoryManager::UpdateTmssUnlockWindow(uint32_t masterClocks) {
	if (!IsTmssVdpLockEnforced() || !_tmssUnlockPending) {
		return;
	}

	if (masterClocks >= _tmssUnlockDelayMclk) {
		_tmssUnlockDelayMclk = 0;
		_tmssUnlockPending = false;
		_tmssUnlocked = true;
		_tmssVdpBlockLogged = false;
		_tmssStartupBypassLogged = false;
		_ioState.TmssEnabled = _tmssEnabled ? 1 : 0;
		_ioState.TmssUnlocked = _tmssUnlocked ? 1 : 0;
		MessageManager::Log("[Genesis][MMU] TMSS unlock delay elapsed - VDP access enabled");
		TraceStartupEvent("TMSS_UNLOCK", 0xA14000, 0x5345, 0x4741);
	} else {
		_tmssUnlockDelayMclk = (uint16_t)(_tmssUnlockDelayMclk - masterClocks);
	}
}

bool GenesisMemoryManager::TryGetRomBankRegisterSlot(uint32_t addr, uint8_t& slot) const {
	if ((addr & 0x01) == 0) {
		return false;
	}

	if (addr < 0xA130F3 || addr > 0xA130FF) {
		return false;
	}

	slot = (uint8_t)((addr - 0xA130F3) >> 1);
	return slot < MapperBankWindowCount;
}

bool GenesisMemoryManager::IsRamControlRegister(uint32_t addr) const {
	return (addr & 0xFFFFFF) == 0xA130F1;
}

uint8_t GenesisMemoryManager::GetRamControlRegisterValue() const {
	return (uint8_t)((_ramEnable ? 0x01 : 0x00) | (_ramWritable ? 0x00 : 0x02));
}

void GenesisMemoryManager::WriteRamControlRegister(uint8_t value) {
	_ramEnable = (value & 0x01) != 0;
	_ramWritable = (value & 0x02) == 0;
}

bool GenesisMemoryManager::TryWriteRomBankRegister(uint32_t addr, uint8_t value) {
	uint8_t slot = 0;
	if (!TryGetRomBankRegisterSlot(addr, slot)) {
		return false;
	}

	uint8_t effectiveValue = (uint8_t)(value & 0x3F);
	_romBankRegisters[slot] = effectiveValue;
	return true;
}

uint32_t GenesisMemoryManager::TranslateRomAddress(uint32_t addr) const {
	if (_prgRomSize == 0) {
		return 0;
	}

	uint32_t effectiveAddr = addr & 0x3FFFFF;
	uint32_t bankCount = (_prgRomSize + MapperWindowSize - 1) / MapperWindowSize;
	if (bankCount == 0) {
		bankCount = 1;
	}

	if (_romBankMapperEnabled && effectiveAddr >= 0x080000 && effectiveAddr < 0x400000) {
		uint32_t windowOffset = effectiveAddr - 0x080000;
		uint32_t slot = windowOffset / MapperWindowSize;
		if (slot < MapperBankWindowCount) {
			uint32_t bank = _romBankRegisters[slot] % bankCount;
			uint32_t mappedAddress = bank * MapperWindowSize + (windowOffset % MapperWindowSize);
			return mappedAddress % _prgRomSize;
		}
	}

	return effectiveAddr % _prgRomSize;
}

bool GenesisMemoryManager::IsZ80BusGranted() const {
	// 68k can access the Z80 window when BUSACK is asserted or when reset is held.
	return _z80BusAck || _z80Reset;
}

void GenesisMemoryManager::AdvanceZ80BusArbitration(uint32_t masterClocks) {
	if (masterClocks == 0) {
		return;
	}

	if (_z80Reset) {
		_z80BusReqDelayMclk = 0;
		_z80ResumeDelayMclk = 0;
		_z80BusAck = _z80BusRequest;
		return;
	}

	if (_z80BusRequest) {
		_z80ResumeDelayMclk = 0;
		if (!_z80BusAck && _z80BusReqDelayMclk > 0) {
			if (masterClocks >= _z80BusReqDelayMclk) {
				_z80BusReqDelayMclk = 0;
				_z80BusAck = true;
			} else {
				_z80BusReqDelayMclk = (uint16_t)(_z80BusReqDelayMclk - masterClocks);
			}
		}
	} else {
		_z80BusReqDelayMclk = 0;
		if (_z80ResumeDelayMclk > 0) {
			if (masterClocks >= _z80ResumeDelayMclk) {
				_z80ResumeDelayMclk = 0;
			} else {
				_z80ResumeDelayMclk = (uint16_t)(_z80ResumeDelayMclk - masterClocks);
			}
		}
	}
}

void GenesisMemoryManager::SetZ80BusRequest(bool request, bool allowTransitionLog, uint32_t addr, uint32_t pc, const char* sourceTag) {
	bool oldBusReq = _z80BusRequest;
	_z80BusRequest = request;
	if (request) {
		_z80ResumeDelayMclk = 0;
		if (_z80Reset) {
			_z80BusAck = true;
			_z80BusReqDelayMclk = 0;
		} else if (!_z80BusAck && _z80BusReqDelayMclk == 0) {
			_z80BusReqDelayMclk = _z80BusReqAckDelayMclkSetting;
		}
	} else {
		_z80BusAck = false;
		_z80BusReqDelayMclk = 0;
		_z80ResumeDelayMclk = _z80Reset ? 0 : _z80BusResumeDelayMclkSetting;
	}

	if (allowTransitionLog && oldBusReq != _z80BusRequest) {
		MessageManager::Log(std::format("[Genesis][MMU] Z80 busreq latch src={} addr=${:06x} pc=${:06x} oldReq={} newReq={} reset={} ack={} reqDelay={} resumeDelay={}",
			sourceTag,
			addr & 0x00ffffff,
			pc & 0x00ffffff,
			oldBusReq ? 1 : 0,
			_z80BusRequest ? 1 : 0,
			_z80Reset ? 1 : 0,
			_z80BusAck ? 1 : 0,
			_z80BusReqDelayMclk,
			_z80ResumeDelayMclk));
		TraceStartupEvent("Z80_BUSREQ", addr, (uint16_t)((oldBusReq ? 0x100 : 0) | (_z80BusRequest ? 0x001 : 0)), (uint16_t)(_z80BusAck ? 1 : 0));
	}

	UpdateZ80RuntimeState(allowTransitionLog, addr, pc, sourceTag);
}

void GenesisMemoryManager::SetZ80Reset(bool resetAsserted, bool allowTransitionLog, uint32_t addr, uint32_t pc, const char* sourceTag) {
	bool oldReset = _z80Reset;
	_z80Reset = resetAsserted;
	if (resetAsserted) {
		_z80BusReqDelayMclk = 0;
		_z80ResumeDelayMclk = 0;
		_z80BusAck = _z80BusRequest;
	} else if (_z80BusRequest) {
		_z80BusAck = false;
		_z80BusReqDelayMclk = _z80BusReqAckDelayMclkSetting;
		_z80ResumeDelayMclk = 0;
	} else {
		_z80BusAck = false;
		_z80BusReqDelayMclk = 0;
		_z80ResumeDelayMclk = 0;
	}

	if (allowTransitionLog && oldReset != _z80Reset) {
		MessageManager::Log(std::format("[Genesis][MMU] Z80 reset latch src={} addr=${:06x} pc=${:06x} oldReset={} newReset={} req={} ack={} reqDelay={} resumeDelay={}",
			sourceTag,
			addr & 0x00ffffff,
			pc & 0x00ffffff,
			oldReset ? 1 : 0,
			_z80Reset ? 1 : 0,
			_z80BusRequest ? 1 : 0,
			_z80BusAck ? 1 : 0,
			_z80BusReqDelayMclk,
			_z80ResumeDelayMclk));
		TraceStartupEvent("Z80_RESET", addr, (uint16_t)((oldReset ? 0x100 : 0) | (_z80Reset ? 0x001 : 0)), (uint16_t)(_z80BusRequest ? 1 : 0));
	}

	UpdateZ80RuntimeState(allowTransitionLog, addr, pc, sourceTag);
}

bool GenesisMemoryManager::ComputeZ80RuntimeRunning() const {
	return !_z80Reset && !_z80BusAck && _z80ResumeDelayMclk == 0;
}

void GenesisMemoryManager::UpdateZ80RuntimeState(bool allowTransitionLog, uint32_t addr, uint32_t pc, const char* sourceTag) {
	bool nextRunning = ComputeZ80RuntimeRunning();
	if (_z80RuntimeRunning != nextRunning) {
		_z80RuntimeTransitionCount++;
		_z80RuntimeStateEpoch++;
		_z80RuntimeLastTransitionClock = _masterClock;
		TraceStartupEvent("Z80_RUN_TGL", 0xA11100, nextRunning ? 1 : 0, (uint16_t)(_z80RuntimeTransitionCount & 0xFFFFu));
		if (allowTransitionLog) {
			MessageManager::Log(std::format("[Genesis][MMU] Z80 runtime transition #{} epoch={} src={} addr=${:06x} pc=${:06x} oldRunning={} newRunning={} busReq={} reset={} runnableCycles={} stalledCycles={} masterClock={}",
				_z80RuntimeTransitionCount,
				_z80RuntimeStateEpoch,
				sourceTag,
				addr & 0x00ffffff,
				pc & 0x00ffffff,
				_z80RuntimeRunning ? 1 : 0,
				nextRunning ? 1 : 0,
				_z80BusRequest ? 1 : 0,
				_z80Reset ? 1 : 0,
				_z80RuntimeRunnableCycles,
				_z80RuntimeStalledCycles,
				_masterClock));
		}
	}
	_z80RuntimeRunning = nextRunning;
}

bool GenesisMemoryManager::TryGetSegaCdBridgeSlot(uint32_t addr, uint8_t*& slot, uint32_t& slotIndex) {
	if (addr >= 0xA12000 && addr <= 0xA1201F) {
		slot = &_segaCdBridgeA120[0];
		slotIndex = addr & 0x1F;
		return true;
	}

	if (addr >= 0xA13000 && addr <= 0xA1301F) {
		slot = &_segaCdBridgeA130[0];
		slotIndex = addr & 0x1F;
		return true;
	}

	if (addr >= 0xA14000 && addr <= 0xA1401F) {
		slot = &_segaCdBridgeA140[0];
		slotIndex = addr & 0x1F;
		return true;
	}

	if (addr >= 0xA15000 && addr <= 0xA1501F) {
		slot = &_segaCdBridgeA150[0];
		slotIndex = addr & 0x1F;
		return true;
	}

	if (addr >= 0xA16000 && addr <= 0xA1601F) {
		slot = &_segaCdBridgeA160[0];
		slotIndex = addr & 0x1F;
		return true;
	}

	if (addr >= 0xA18000 && addr <= 0xA1801F) {
		slot = &_segaCdBridgeA180[0];
		slotIndex = addr & 0x1F;
		return true;
	}

	return false;
}

bool GenesisMemoryManager::IsSegaCdSubCpuControlAddress(uint32_t addr) const {
	return addr == 0xA12000 || addr == 0xA12001;
}

bool GenesisMemoryManager::IsSegaCdAudioDataAddress(uint32_t addr) const {
	return addr >= 0xA12002 && addr <= 0xA12005;
}

bool GenesisMemoryManager::IsSegaCdAudioStatusAddress(uint32_t addr) const {
	return addr == 0xA12010 || addr == 0xA12011;
}

bool GenesisMemoryManager::IsSegaCdToolingControlAddress(uint32_t addr) const {
	return addr >= 0xA12012 && addr <= 0xA12015;
}

bool GenesisMemoryManager::IsSegaCdToolingStatusAddress(uint32_t addr) const {
	return addr >= 0xA12016 && addr <= 0xA1201F;
}

bool GenesisMemoryManager::Is32xSh2ControlAddress(uint32_t addr) const {
	return addr == 0xA15012 || addr == 0xA15013 || addr == 0xA15014;
}

bool GenesisMemoryManager::Is32xSh2StatusAddress(uint32_t addr) const {
	return addr == 0xA1501A || addr == 0xA1501B;
}

bool GenesisMemoryManager::Is32xCompositionControlAddress(uint32_t addr) const {
	return addr == 0xA15016 || addr == 0xA15017;
}

bool GenesisMemoryManager::Is32xCompositionStatusAddress(uint32_t addr) const {
	return addr == 0xA1501C || addr == 0xA1501D;
}

bool GenesisMemoryManager::Is32xToolingControlAddress(uint32_t addr) const {
	return addr >= 0xA15008 && addr <= 0xA1500B;
}

bool GenesisMemoryManager::Is32xToolingStatusAddress(uint32_t addr) const {
	return addr >= 0xA15018 && addr <= 0xA1501F;
}

bool GenesisMemoryManager::Is32xCoprocControlAddress(uint32_t addr) const {
	return addr >= 0xA16012 && addr <= 0xA16015;
}

bool GenesisMemoryManager::Is32xCoprocStatusAddress(uint32_t addr) const {
	return addr >= 0xA1601A && addr <= 0xA1601F;
}

bool GenesisMemoryManager::Is32xHostToolingControlAddress(uint32_t addr) const {
	return addr >= 0xA18008 && addr <= 0xA1800B;
}

bool GenesisMemoryManager::Is32xHostToolingStatusAddress(uint32_t addr) const {
	return addr >= 0xA18018 && addr <= 0xA1801F;
}

uint8_t GenesisMemoryManager::Normalize32xCoprocControlValue(uint32_t addr, uint8_t value) const {
	if (addr == 0xA16012 || addr == 0xA16013) {
		return (uint8_t)(value & 0x01);
	}
	if (addr == 0xA16014) {
		return (uint8_t)(value & 0x0F);
	}
	if (addr == 0xA16015) {
		return (uint8_t)(value & 0x03);
	}
	return value;
}

uint8_t GenesisMemoryManager::Normalize32xHostControlValue(uint32_t addr, uint8_t value) const {
	if (addr >= 0xA18008 && addr <= 0xA1800B) {
		return (uint8_t)(value & 0x01);
	}
	return value;
}

void GenesisMemoryManager::Recompute32xCoprocDigest() {
	uint8_t digest = _m32xToolingDigest;
	digest ^= _m32xCoprocMasterSignal;
	digest ^= (uint8_t)(_m32xCoprocSlaveSignal << 1);
	digest ^= (uint8_t)(_m32xCoprocPhaseSignal << 2);
	digest ^= (uint8_t)(_m32xCoprocFenceSignal << 5);
	digest ^= (uint8_t)(_m32xCoprocEventCount & 0xFF);
	digest ^= (uint8_t)(_m32xCoprocEdgeCount & 0xFF);
	digest ^= (uint8_t)(_m32xCoprocPhaseEpoch & 0xFF);
	digest ^= (uint8_t)(_m32xCoprocFenceEpoch << 4);
	digest ^= _m32xCoprocArbiterLatch;
	_m32xCoprocDigest = digest;
}

void GenesisMemoryManager::Recompute32xHostDigest() {
	uint8_t digest = _m32xCoprocDigest;
	digest ^= _m32xHostDebuggerSignal;
	digest ^= (uint8_t)(_m32xHostTasSignal << 1);
	digest ^= (uint8_t)(_m32xHostSaveStateSignal << 2);
	digest ^= (uint8_t)(_m32xHostCheatSignal << 3);
	digest ^= (uint8_t)(_m32xHostEventCount & 0xFF);
	digest ^= (uint8_t)(_m32xHostEdgeCount & 0xFF);
	digest ^= _m32xHostCommandNonce;
	digest ^= _m32xHostAckToken;
	digest ^= _m32xHostDeterminismLatch;
	_m32xHostDigest = digest;
}

void GenesisMemoryManager::UpdateSegaCdSubCpuControl(uint8_t value) {
	uint8_t effectiveValue = value;
	bool nextRunning = (effectiveValue & 0x01) != 0;
	bool nextBusRequest = (effectiveValue & 0x02) != 0;
	if (nextRunning != _segaCdSubCpuRunning || nextBusRequest != _segaCdSubCpuBusRequest) {
		_segaCdSubCpuTransitionCount++;
	}
	_segaCdSubCpuRunning = nextRunning;
	_segaCdSubCpuBusRequest = nextBusRequest;
}

uint8_t GenesisMemoryManager::GetSegaCdSubCpuStatusByte() const {
	uint8_t statusByte = 0;
	if (_segaCdSubCpuRunning) {
		statusByte |= 0x01;
	}
	if (_segaCdSubCpuBusRequest) {
		statusByte |= 0x02;
	}
	statusByte |= (uint8_t)((_segaCdSubCpuTransitionCount & 0x0F) << 4);
	return statusByte;
}

void GenesisMemoryManager::UpdateSegaCdAudioPath(uint32_t addr, uint8_t value) {
	uint8_t effectiveValue = value;
	if (addr == 0xA12002) {
		_segaCdPcmLeft = effectiveValue;
	} else if (addr == 0xA12003) {
		_segaCdPcmRight = effectiveValue;
	} else if (addr == 0xA12004) {
		_segaCdCddaLeft = effectiveValue;
	} else if (addr == 0xA12005) {
		_segaCdCddaRight = effectiveValue;
	} else {
		return;
	}

	int16_t mixedLeft = (int16_t)(int8_t)_segaCdPcmLeft + (int16_t)(int8_t)_segaCdCddaLeft;
	int16_t mixedRight = (int16_t)(int8_t)_segaCdPcmRight + (int16_t)(int8_t)_segaCdCddaRight;
	mixedLeft = std::clamp<int16_t>(mixedLeft, -128, 127);
	mixedRight = std::clamp<int16_t>(mixedRight, -128, 127);
	_segaCdMixedLeft = (uint8_t)(int8_t)mixedLeft;
	_segaCdMixedRight = (uint8_t)(int8_t)mixedRight;
	_segaCdAudioCheckpointCount++;
}

uint8_t GenesisMemoryManager::GetSegaCdAudioStatusByte(uint32_t addr) const {
	if (addr == 0xA12010) {
		uint8_t statusByte = _segaCdMixedLeft;
		return statusByte;
	}
	if (addr == 0xA12011) {
		uint8_t statusByte = _segaCdMixedRight;
		return statusByte;
	}
	uint8_t statusByte = 0;
	return statusByte;
}

void GenesisMemoryManager::UpdateSegaCdToolingContract(uint32_t addr, uint8_t value) {
	uint8_t* target = nullptr;
	if (addr == 0xA12012) {
		target = &_segaCdToolingDebuggerSignal;
	} else if (addr == 0xA12013) {
		target = &_segaCdToolingTasSignal;
	} else if (addr == 0xA12014) {
		target = &_segaCdToolingSaveStateSignal;
	} else if (addr == 0xA12015) {
		target = &_segaCdToolingCheatSignal;
	}

	if (!target) {
		return;
	}

	uint8_t effectiveValue = value;
	if (*target != effectiveValue) {
		*target = effectiveValue;
		_segaCdToolingEventCount++;
	}

	uint8_t digest = 0x0F;
	digest ^= _segaCdToolingDebuggerSignal;
	digest ^= (uint8_t)(_segaCdToolingTasSignal << 1);
	digest ^= (uint8_t)(_segaCdToolingSaveStateSignal << 2);
	digest ^= (uint8_t)(_segaCdToolingCheatSignal << 3);
	digest ^= (uint8_t)(_segaCdToolingEventCount & 0xFF);
	_segaCdToolingDigest = digest;
}

uint8_t GenesisMemoryManager::GetSegaCdToolingStatusByte(uint32_t addr) const {
	if (addr == 0xA12016) {
		return (uint8_t)(_segaCdToolingEventCount & 0xFF);
	}
	if (addr == 0xA12017) {
		return (uint8_t)((_segaCdToolingEventCount >> 8) & 0xFF);
	}
	if (addr == 0xA12018) {
		return (uint8_t)(_ioState.DebugTranscriptLaneCount & 0xFF);
	}
	if (addr == 0xA12019) {
		return (uint8_t)(_ioState.DebugTranscriptLaneDigest & 0xFF);
	}
	if (addr == 0xA1201A) {
		uint8_t statusByte = 0x0F;
		return statusByte;
	}
	if (addr == 0xA1201B) {
		uint8_t statusByte = _segaCdToolingDigest;
		return statusByte;
	}
	if (addr == 0xA1201C) {
		uint8_t statusByte = _controlManager ? _controlManager->GetDeterministicPortCapabilities(0) : 0;
		return statusByte;
	}
	if (addr == 0xA1201D) {
		uint8_t statusByte = _controlManager ? _controlManager->GetDeterministicPortDigest(0) : 0;
		return statusByte;
	}
	if (addr == 0xA1201E) {
		uint8_t statusByte = _controlManager ? _controlManager->GetDeterministicPortCapabilities(1) : 0;
		return statusByte;
	}
	if (addr == 0xA1201F) {
		uint8_t statusByte = _controlManager ? _controlManager->GetDeterministicPortDigest(1) : 0;
		return statusByte;
	}
	uint8_t statusByte = 0;
	return statusByte;
}

void GenesisMemoryManager::Update32xSh2Staging(uint32_t addr, uint8_t value) {
	uint8_t effectiveValue = value;
	bool changed = false;
	if (addr == 0xA15012) {
		bool nextMaster = (effectiveValue & 0x01) != 0;
		bool nextSlave = (effectiveValue & 0x02) != 0;
		changed = nextMaster != _m32xMasterSh2Running || nextSlave != _m32xSlaveSh2Running;
		_m32xMasterSh2Running = nextMaster;
		_m32xSlaveSh2Running = nextSlave;
	} else if (addr == 0xA15013) {
		uint8_t phase = (uint8_t)(effectiveValue & 0x0F);
		changed = phase != _m32xSh2SyncPhase;
		_m32xSh2SyncPhase = phase;
	} else if (addr == 0xA15014) {
		changed = effectiveValue != _m32xSh2Milestone;
		_m32xSh2Milestone = effectiveValue;
	}

	if (changed) {
		_m32xSh2SyncEpoch++;
	}

	uint8_t digest = 0;
	digest |= _m32xMasterSh2Running ? 0x01 : 0x00;
	digest |= _m32xSlaveSh2Running ? 0x02 : 0x00;
	digest ^= (uint8_t)(_m32xSh2SyncPhase << 2);
	digest ^= _m32xSh2Milestone;
	digest ^= (uint8_t)(_m32xSh2SyncEpoch & 0xFF);
	_m32xSh2Digest = digest;
}

uint8_t GenesisMemoryManager::Get32xSh2StatusByte(uint32_t addr) const {
	if (addr == 0xA1501A) {
		uint8_t status = 0;
		if (_m32xMasterSh2Running) {
			status |= 0x01;
		}
		if (_m32xSlaveSh2Running) {
			status |= 0x02;
		}
		status |= (uint8_t)((_m32xSh2SyncPhase & 0x0F) << 2);
		return status;
	}
	if (addr == 0xA1501B) {
		uint8_t statusByte = _m32xSh2Digest;
		return statusByte;
	}
	uint8_t statusByte = 0;
	return statusByte;
}

void GenesisMemoryManager::Update32xCompositionStaging(uint32_t addr, uint8_t value) {
	uint8_t effectiveValue = value;
	bool changed = false;
	if (addr == 0xA15016) {
		uint8_t blend = (uint8_t)(effectiveValue & 0x0F);
		changed = blend != _m32xCompositionBlend;
		_m32xCompositionBlend = blend;
	} else if (addr == 0xA15017) {
		uint8_t nextMarker = effectiveValue;
		changed = nextMarker != _m32xFrameSyncMarker;
		_m32xFrameSyncMarker = nextMarker;
	}

	if (changed) {
		_m32xFrameSyncEpoch++;
	}

	uint8_t digest = _m32xSh2Digest;
	digest ^= (uint8_t)(_m32xCompositionBlend << 1);
	digest ^= _m32xFrameSyncMarker;
	digest ^= (uint8_t)(_m32xFrameSyncEpoch & 0xFF);
	_m32xCompositionDigest = digest;
}

uint8_t GenesisMemoryManager::Get32xCompositionStatusByte(uint32_t addr) const {
	if (addr == 0xA1501C) {
		uint8_t status = (uint8_t)(_m32xCompositionBlend & 0x0F);
		status |= (uint8_t)((_m32xFrameSyncEpoch & 0x03) << 6);
		return status;
	}
	if (addr == 0xA1501D) {
		uint8_t statusByte = _m32xCompositionDigest;
		return statusByte;
	}
	uint8_t statusByte = 0;
	return statusByte;
}

void GenesisMemoryManager::Update32xToolingContract(uint32_t addr, uint8_t value) {
	uint8_t* target = nullptr;
	if (addr == 0xA15008) {
		target = &_m32xToolingDebuggerSignal;
	} else if (addr == 0xA15009) {
		target = &_m32xToolingTasSignal;
	} else if (addr == 0xA1500A) {
		target = &_m32xToolingSaveStateSignal;
	} else if (addr == 0xA1500B) {
		target = &_m32xToolingCheatSignal;
	}

	if (!target) {
		return;
	}

	uint8_t effectiveValue = value;
	if (*target != effectiveValue) {
		*target = effectiveValue;
		_m32xToolingEventCount++;
	}

	uint8_t digest = _m32xCompositionDigest;
	digest ^= _m32xToolingDebuggerSignal;
	digest ^= (uint8_t)(_m32xToolingTasSignal << 1);
	digest ^= (uint8_t)(_m32xToolingSaveStateSignal << 2);
	digest ^= (uint8_t)(_m32xToolingCheatSignal << 3);
	digest ^= (uint8_t)(_m32xToolingEventCount & 0xFF);
	_m32xToolingDigest = digest;
}

uint8_t GenesisMemoryManager::Get32xToolingStatusByte(uint32_t addr) const {
	if (addr == 0xA15018) {
		uint8_t statusByte = (uint8_t)(_m32xToolingEventCount & 0xFF);
		return statusByte;
	}
	if (addr == 0xA15019) {
		uint8_t statusByte = (uint8_t)((_m32xToolingEventCount >> 8) & 0xFF);
		return statusByte;
	}
	if (addr == 0xA1501E) {
		uint8_t statusByte = 0x0F;
		return statusByte;
	}
	if (addr == 0xA1501F) {
		uint8_t statusByte = _m32xToolingDigest;
		return statusByte;
	}
	uint8_t statusByte = 0;
	return statusByte;
}

void GenesisMemoryManager::Update32xCoprocContract(uint32_t addr, uint8_t value) {
	uint8_t* target = nullptr;
	if (addr == 0xA16012) {
		target = &_m32xCoprocMasterSignal;
	} else if (addr == 0xA16013) {
		target = &_m32xCoprocSlaveSignal;
	} else if (addr == 0xA16014) {
		target = &_m32xCoprocPhaseSignal;
	} else if (addr == 0xA16015) {
		target = &_m32xCoprocFenceSignal;
	}

	if (!target) {
		return;
	}

	uint8_t effectiveValue = Normalize32xCoprocControlValue(addr, value);
	uint8_t previousValue = *target;
	if (previousValue != effectiveValue) {
		*target = effectiveValue;
		_m32xCoprocEventCount++;
		_m32xCoprocEdgeCount++;

		if (addr == 0xA16014) {
			uint8_t previousPhase = (uint8_t)(previousValue & 0x0F);
			uint8_t nextPhase = (uint8_t)(effectiveValue & 0x0F);
			uint8_t phaseDelta = (uint8_t)((nextPhase - previousPhase) & 0x0F);
			if (phaseDelta == 0) {
				phaseDelta = 1;
			}
			_m32xCoprocPhaseEpoch = (uint16_t)(_m32xCoprocPhaseEpoch + phaseDelta);
		}

		if (addr == 0xA16015) {
			uint8_t previousFence = (uint8_t)(previousValue & 0x03);
			uint8_t nextFence = (uint8_t)(effectiveValue & 0x03);
			if (((previousFence ^ nextFence) & 0x01) != 0) {
				_m32xCoprocFenceEpoch++;
			}
		}
	}

	uint8_t arbiterLatch = 0;
	arbiterLatch |= (_m32xCoprocMasterSignal & 0x01);
	arbiterLatch |= (uint8_t)((_m32xCoprocSlaveSignal & 0x01) << 1);
	arbiterLatch |= (uint8_t)((_m32xCoprocPhaseEpoch & 0x03) << 2);
	arbiterLatch |= (uint8_t)((_m32xCoprocFenceEpoch & 0x03) << 4);
	if ((_m32xCoprocMasterSignal & _m32xCoprocSlaveSignal & 0x01) != 0) {
		arbiterLatch |= 0x40;
	}
	if ((_m32xCoprocEdgeCount & 0x01) != 0) {
		arbiterLatch |= 0x80;
	}
	_m32xCoprocArbiterLatch = arbiterLatch;

	Recompute32xCoprocDigest();
}

uint8_t GenesisMemoryManager::Get32xCoprocStatusByte(uint32_t addr) const {
	if (addr == 0xA1601A) {
		return (uint8_t)(_m32xCoprocEventCount & 0xFF);
	}
	if (addr == 0xA1601B) {
		return (uint8_t)((_m32xCoprocEventCount >> 8) & 0xFF);
	}
	if (addr == 0xA1601C) {
		uint8_t status = 0;
		status |= (_m32xCoprocMasterSignal & 0x01);
		status |= (uint8_t)((_m32xCoprocSlaveSignal & 0x01) << 1);
		status |= (uint8_t)((_m32xCoprocPhaseSignal & 0x03) << 2);
		status |= (uint8_t)((_m32xCoprocFenceSignal & 0x03) << 4);
		status |= (uint8_t)(_m32xCoprocArbiterLatch & 0xC0);
		return status;
	}
	if (addr == 0xA1601D) {
		return _m32xCoprocDigest;
	}
	if (addr == 0xA1601E) {
		return (uint8_t)(_m32xCoprocEdgeCount & 0xFF);
	}
	if (addr == 0xA1601F) {
		return (uint8_t)((_m32xCoprocPhaseEpoch & 0x0F) | ((_m32xCoprocFenceEpoch & 0x0F) << 4));
	}
	return 0;
}

void GenesisMemoryManager::Update32xHostToolingContract(uint32_t addr, uint8_t value) {
	uint8_t* target = nullptr;
	if (addr == 0xA18008) {
		target = &_m32xHostDebuggerSignal;
	} else if (addr == 0xA18009) {
		target = &_m32xHostTasSignal;
	} else if (addr == 0xA1800A) {
		target = &_m32xHostSaveStateSignal;
	} else if (addr == 0xA1800B) {
		target = &_m32xHostCheatSignal;
	}

	if (!target) {
		return;
	}

	uint8_t effectiveValue = Normalize32xHostControlValue(addr, value);
	uint8_t previousValue = *target;
	if (previousValue != effectiveValue) {
		*target = effectiveValue;
		_m32xHostEventCount++;
		_m32xHostEdgeCount++;

		if (addr == 0xA18008) {
			_m32xHostCommandNonce ^= effectiveValue ? 0x11 : 0x1D;
			_m32xHostAckToken = (uint8_t)(_m32xHostAckToken + 0x03);
		} else if (addr == 0xA18009) {
			_m32xHostCommandNonce = (uint8_t)(_m32xHostCommandNonce + (effectiveValue ? 0x17 : 0x05));
			_m32xHostAckToken ^= 0x05;
		} else if (addr == 0xA1800A) {
			_m32xHostCommandNonce = (uint8_t)((_m32xHostCommandNonce << 1) | (_m32xHostCommandNonce >> 7));
			_m32xHostAckToken = (uint8_t)(_m32xHostAckToken + (effectiveValue ? 0x2B : 0x09));
		} else if (addr == 0xA1800B) {
			_m32xHostCommandNonce ^= 0x3C;
			_m32xHostAckToken = (uint8_t)(_m32xHostAckToken + (effectiveValue ? 0x33 : 0x0F));
		}
	}

	uint8_t determinismLatch = 0;
	determinismLatch |= (_m32xHostDebuggerSignal & 0x01);
	determinismLatch |= (uint8_t)((_m32xHostTasSignal & 0x01) << 1);
	determinismLatch |= (uint8_t)((_m32xHostSaveStateSignal & 0x01) << 2);
	determinismLatch |= (uint8_t)((_m32xHostCheatSignal & 0x01) << 3);
	determinismLatch |= (uint8_t)((_m32xHostEdgeCount & 0x0F) << 4);
	_m32xHostDeterminismLatch = determinismLatch;

	Recompute32xHostDigest();
}

uint8_t GenesisMemoryManager::Get32xHostToolingStatusByte(uint32_t addr) const {
	if (addr == 0xA18018) {
		return (uint8_t)(_m32xHostEventCount & 0xFF);
	}
	if (addr == 0xA18019) {
		return (uint8_t)((_m32xHostEventCount >> 8) & 0xFF);
	}
	if (addr == 0xA1801A) {
		return _m32xHostDeterminismLatch;
	}
	if (addr == 0xA1801B) {
		return _m32xHostDigest;
	}
	if (addr == 0xA1801C) {
		return _m32xHostCommandNonce;
	}
	if (addr == 0xA1801D) {
		return _m32xHostAckToken;
	}
	if (addr == 0xA1801E) {
		uint8_t capability = _controlManager ? _controlManager->GetDeterministicPortCapabilities(0) : 0;
		return (uint8_t)(capability ^ _m32xHostDeterminismLatch);
	}
	if (addr == 0xA1801F) {
		uint8_t digest = _controlManager ? _controlManager->GetDeterministicPortDigest(0) : 0;
		return (uint8_t)(digest ^ _m32xHostDigest);
	}
	return 0;
}

void GenesisMemoryManager::TrackSegaCdTranscript(uint32_t addr, bool isWrite, uint8_t value) {
	uint8_t effectiveValue = value;
	uint8_t effectiveRoleFlags = 0;
	if ((addr & 0x10) != 0) {
		effectiveRoleFlags |= 0x02;
	}
	if (addr >= 0xA13000 && addr <= 0xA1301F) {
		effectiveRoleFlags |= 0x04;
	} else if (addr >= 0xA14000 && addr <= 0xA1401F) {
		effectiveRoleFlags |= 0x08;
	} else if (addr >= 0xA15000 && addr <= 0xA1501F) {
		effectiveRoleFlags |= 0x10;
	} else if (addr >= 0xA16000 && addr <= 0xA1601F) {
		effectiveRoleFlags |= 0x20;
	} else if (addr >= 0xA18000 && addr <= 0xA1801F) {
		effectiveRoleFlags |= 0x40;
	}

	TrackTranscriptEntry(addr, isWrite, effectiveValue, effectiveRoleFlags);
}

void GenesisMemoryManager::TrackSegaCdHandshakeTranscript(uint32_t addr, bool isWrite, uint8_t value) {
	uint8_t effectiveValue = value;
	uint8_t effectiveRoleFlags = 0x80;
	if (IsZ80ResetAddress(addr)) {
		effectiveRoleFlags |= 0x04;
	}
	if (!isWrite) {
		effectiveRoleFlags |= 0x02;
	}

	TrackTranscriptEntry(addr, isWrite, effectiveValue, effectiveRoleFlags);
}

void GenesisMemoryManager::TrackTranscriptEntry(uint32_t addr, bool isWrite, uint8_t value, uint8_t roleFlags) {
	static constexpr uint64_t FnvOffsetBasis = 1469598103934665603ull;
	static constexpr uint64_t FnvPrime = 1099511628211ull;
	uint8_t effectiveValue = value;
	uint8_t effectiveRoleFlags = roleFlags;

	if (isWrite) {
		effectiveRoleFlags |= 0x01;
	}

	uint64_t hash = _ioState.TranscriptLaneDigest == 0 ? FnvOffsetBasis : _ioState.TranscriptLaneDigest;
	hash ^= (addr & 0xFFFFFF);
	hash *= FnvPrime;
	hash ^= effectiveValue;
	hash *= FnvPrime;
	hash ^= effectiveRoleFlags;
	hash *= FnvPrime;
	_ioState.TranscriptLaneDigest = hash;

	uint32_t index = _ioState.TranscriptLaneCount % 4;
	_ioState.TranscriptEntryAddress[index] = addr & 0xFFFFFF;
	_ioState.TranscriptEntryValue[index] = effectiveValue;
	_ioState.TranscriptEntryFlags[index] = effectiveRoleFlags;
	_ioState.TranscriptLaneCount++;
}

void GenesisMemoryManager::TrackDebugTranscriptEntry(uint32_t addr, bool isWrite, uint8_t value, uint8_t roleFlags) {
	static constexpr uint64_t FnvOffsetBasis = 1469598103934665603ull;
	static constexpr uint64_t FnvPrime = 1099511628211ull;
	uint8_t effectiveValue = value;
	uint8_t effectiveRoleFlags = roleFlags;

	if (isWrite) {
		effectiveRoleFlags |= 0x01;
	}
	effectiveRoleFlags |= 0x40;

	uint64_t hash = _ioState.DebugTranscriptLaneDigest == 0 ? FnvOffsetBasis : _ioState.DebugTranscriptLaneDigest;
	hash ^= (addr & 0xFFFFFF);
	hash *= FnvPrime;
	hash ^= effectiveValue;
	hash *= FnvPrime;
	hash ^= effectiveRoleFlags;
	hash *= FnvPrime;
	_ioState.DebugTranscriptLaneDigest = hash;

	uint32_t index = _ioState.DebugTranscriptLaneCount % 4;
	_ioState.DebugTranscriptEntryAddress[index] = addr & 0xFFFFFF;
	_ioState.DebugTranscriptEntryValue[index] = effectiveValue;
	_ioState.DebugTranscriptEntryFlags[index] = effectiveRoleFlags;
	_ioState.DebugTranscriptLaneCount++;
}

void GenesisMemoryManager::ClearDebugTranscriptLane() {
	_ioState.DebugTranscriptLaneCount = 0;
	_ioState.DebugTranscriptLaneDigest = 0;
	for (uint32_t i = 0; i < 4; i++) {
		_ioState.DebugTranscriptEntryAddress[i] = 0;
		_ioState.DebugTranscriptEntryValue[i] = 0;
		_ioState.DebugTranscriptEntryFlags[i] = 0;
	}
}

bool GenesisMemoryManager::IsSramAddress(uint32_t addr) const {
	return HasSaveRam() && _ramEnable && addr >= _sramStart && addr <= _sramEnd;
}

bool GenesisMemoryManager::TryGetSramOffset(uint32_t addr, uint32_t& offset) const {
	uint32_t effectiveAddr = addr;
	if (!IsSramAddress(effectiveAddr)) {
		return false;
	}

	if ((effectiveAddr & 0x01) == 0) {
		if (!_sramEvenBytes) {
			return false;
		}
	} else if (!_sramOddBytes) {
		return false;
	}

	if (_sramEvenBytes && _sramOddBytes) {
		offset = effectiveAddr - _sramStart;
	} else {
		offset = (effectiveAddr - _sramStart) >> 1;
	}

	return offset < _saveRamSize;
}

// =============================================
// Genesis 68000 memory map (24-bit, big-endian)
// =============================================
// $000000-$3FFFFF : Cartridge ROM (up to 4MB)
// $400000-$7FFFFF : Reserved / expansion
// $A00000-$A0FFFF : Z80 address space (8KB RAM + mirrored)
// $A10000-$A1001F : I/O registers
// $A11100         : Z80 bus request
// $A11200         : Z80 reset
// $C00000-$C0001F : VDP ports
// $E00000-$FFFFFF : 68000 work RAM (64KB, mirrored)

uint8_t GenesisMemoryManager::Read8(uint32_t addr) {
	addr &= 0xFFFFFF;
	AdvanceZ80BusArbitration(7);
	UpdateZ80RuntimeState(false, addr, _cpu ? (_cpu->GetState().PC & 0x00ffffff) : 0xffffffff, "read8");
	uint32_t sramOffset = 0;
	if (addr == 0xA11000 || addr == 0xA11001) [[unlikely]] {
		uint8_t effectiveValue = 0x00;
		_openBus = effectiveValue;
		return effectiveValue;
	}
	if (addr == 0xA14100) [[unlikely]] {
		uint8_t effectiveValue = 0xFF;
		TraceStartupEvent("TMSS_CART_R8", addr, effectiveValue, _tmssUnlocked ? 1 : 0);
		_openBus = effectiveValue;
		return effectiveValue;
	}
	if (IsTmssCartAddress(addr)) [[unlikely]] {
		uint8_t effectiveValue = 0xFF;
		TraceStartupEvent("TMSS_CART_R8", addr, effectiveValue, _tmssUnlocked ? 1 : 0);
		_openBus = effectiveValue;
		return effectiveValue;
	}
	if (IsTmssAddress(addr)) [[unlikely]] {
		uint8_t effectiveValue = 0xFF;
		TraceStartupEvent("TMSS_R8", addr, effectiveValue, _tmssUnlocked ? 1 : 0);
		_openBus = effectiveValue;
		return effectiveValue;
	}

	if (TryGetSramOffset(addr, sramOffset)) [[unlikely]] {
		uint8_t effectiveValue = _saveRam[sramOffset];
		_emu->ProcessMemoryRead<CpuType::Genesis>(addr, effectiveValue, MemoryOperationType::Read);
		_openBus = effectiveValue;
		return effectiveValue;
	}

	if (addr < 0x400000) [[likely]] {
		// Cartridge ROM
		uint32_t mappedAddr = TranslateRomAddress(addr);
		uint8_t effectiveValue = _prgRom[mappedAddr];
		_ioState.RomReadHeartbeat++;
		_emu->ProcessMemoryRead<CpuType::Genesis>(mappedAddr, effectiveValue, MemoryOperationType::Read);
		if (_vdp) {
			uint32_t frame = _vdp->GetFrameCount();
			uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffffu) : 0xffffffffu;
			if (ShouldTraceStartupLoopPoll(frame, pc)) {
				TraceStartupEvent("LOOP_POLL8", addr, effectiveValue, (uint16_t)(pc & 0xFFFFu));
			}
		}
		_openBus = effectiveValue;
		return effectiveValue;
	}

	if (addr >= 0xE00000) [[likely]] {
		// Work RAM
		uint32_t offset = addr & 0xFFFF;
		uint8_t effectiveValue = _workRam[offset];
		_emu->ProcessMemoryRead<CpuType::Genesis>(addr, effectiveValue, MemoryOperationType::Read);
		if (_vdp) {
			uint32_t frame = _vdp->GetFrameCount();
			uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffffu) : 0xffffffffu;
			if (ShouldTraceStartupLoopPoll(frame, pc)) {
				TraceStartupEvent("LOOP_POLL8", addr, effectiveValue, (uint16_t)(pc & 0xFFFFu));
			}
		}
		_openBus = effectiveValue;
		return effectiveValue;
	}

	if (addr >= 0xC00000 && addr <= 0xC0001F) [[unlikely]] {
		if (IsTmssVdpLockEnforced()) {
			if (!IsTmssLockedVdpReadAllowed(addr)) {
				if (!_tmssVdpBlockLogged) {
					_tmssVdpBlockLogged = true;
					MessageManager::Log(std::format("[Genesis][MMU] TMSS is locking VDP read8 access at ${:06x}", addr));
				}
				uint8_t effectiveValue = _openBus;
				TraceStartupEvent("TMSS_VDP_R8_BLOCK", addr, effectiveValue, _tmssUnlocked ? 1 : 0);
				_openBus = effectiveValue;
				return effectiveValue;
			}

			if (!_tmssStartupBypassLogged) {
				_tmssStartupBypassLogged = true;
				MessageManager::Log(std::format("[Genesis][MMU] TMSS startup compatibility allows VDP read8 at ${:06x}", addr));
			}
			TraceStartupEvent("TMSS_VDP_R8_ALLOW", addr, 0, IsStartupWindowActive() ? 1 : 0);
		}
		// Use byte-accurate VDP semantics to avoid duplicating word-read side effects.
		uint8_t effectiveValue = _vdp ? _vdp->ReadPortByte(addr) : 0xFF;
		if (_vdp) {
			uint32_t port = addr & 0x1F;
			if (port < 0x04) {
				TraceStartupEvent("VDP_DATA_R", addr, effectiveValue, (uint16_t)port);
			} else if (port < 0x08) {
				GenesisVdpState stateAfterRead = _vdp->GetState();
				if (stateAfterRead.FrameCount > 0u) {
					TraceStartupEvent("VDP_CTRL_R", addr, effectiveValue, (uint16_t)port);
				}
			} else if (port < 0x10) {
				TraceStartupEvent("VDP_HV_R", addr, effectiveValue, (uint16_t)port);
			}

			uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffffu) : 0xffffffffu;
			uint32_t frame = _vdp->GetFrameCount();
			if (ShouldTraceStartupLoopPoll(frame, pc)) {
				TraceStartupEvent("LOOP_POLL8", addr, effectiveValue, (uint16_t)(pc & 0xFFFFu));
			}
		}
		_openBus = effectiveValue;
		return effectiveValue;
	}

	if (addr >= 0xA00000 && addr <= 0xA0FFFF) [[unlikely]] {
		if (IsYm2612Address(addr)) {
			// YM2612 register window: even lanes are status ports, odd lanes are data ports.
			// Until the MMU is wired to the FM core status API, keep status reads as not-busy.
			uint8_t effectiveValue = 0x00;
			_openBus = effectiveValue;
			return effectiveValue;
		}

		// Z80 address space
		static uint64_t z80WindowReadCount = 0;
		z80WindowReadCount++;
		if (IsZ80BusGranted()) {
			uint32_t z80Addr = addr & 0x1FFF;
			uint8_t effectiveValue = _z80Ram[z80Addr];
			bool traceAccess = z80WindowReadCount <= 256 || (z80WindowReadCount % 4096) == 0 || (z80Addr >= 0x1ff0);
			if (traceAccess) {
				uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffff) : 0xffffffff;
				MessageManager::Log(std::format("[Genesis][MMU] Z80 read #{} addr=${:06x} z80=${:04x} val=${:02x} pc=${:06x} busReq={} reset={} gate=allow",
					z80WindowReadCount,
					addr,
					z80Addr,
					effectiveValue,
					pc,
					_z80BusRequest ? 1 : 0,
					_z80Reset ? 1 : 0));
			}
			_openBus = effectiveValue;
			return effectiveValue;
		}
		uint8_t effectiveValue = 0xFF;
		if (z80WindowReadCount <= 256 || (z80WindowReadCount % 4096) == 0) {
			uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffff) : 0xffffffff;
			MessageManager::Log(std::format("[Genesis][MMU] Z80 read #{} addr=${:06x} val=${:02x} pc=${:06x} busReq={} reset={} gate=blocked",
				z80WindowReadCount,
				addr,
				effectiveValue,
				pc,
				_z80BusRequest ? 1 : 0,
				_z80Reset ? 1 : 0));
		}
		_openBus = effectiveValue;
		return effectiveValue;
	}

	if (addr >= 0xA10000 && addr <= 0xA1001F) [[unlikely]] {
		return ReadIo(addr);
	}
	if (addr >= 0xA13000 && addr <= 0xA130FF) [[unlikely]] {
		uint8_t bankSlot = 0;
		uint8_t effectiveValue = 0x00;
		if (IsRamControlRegister(addr)) {
			effectiveValue = GetRamControlRegisterValue();
		} else if (TryGetRomBankRegisterSlot(addr, bankSlot)) {
			effectiveValue = _romBankRegisters[bankSlot];
		}
		_openBus = effectiveValue;
		return effectiveValue;
	}

	uint8_t bankSlot = 0;
	if (IsRamControlRegister(addr)) [[unlikely]] {
		uint8_t effectiveValue = GetRamControlRegisterValue();
		_openBus = effectiveValue;
		return effectiveValue;
	}

	if (TryGetRomBankRegisterSlot(addr, bankSlot)) [[unlikely]] {
		uint8_t effectiveValue = _romBankRegisters[bankSlot];
		_openBus = effectiveValue;
		return effectiveValue;
	}

	uint8_t* bridgeSlot = nullptr;
	uint32_t bridgeIndex = 0;
	if (TryGetSegaCdBridgeSlot(addr, bridgeSlot, bridgeIndex)) [[unlikely]] {
		uint8_t effectiveValue = 0x00;
		if (IsSegaCdSubCpuControlAddress(addr)) {
			effectiveValue = GetSegaCdSubCpuStatusByte();
		} else if (IsBridgeControlReadbackAddress(addr)) {
			effectiveValue = bridgeSlot[bridgeIndex];
		} else if (IsSegaCdAudioStatusAddress(addr)) {
			effectiveValue = GetSegaCdAudioStatusByte(addr);
		} else if (IsSegaCdToolingStatusAddress(addr)) {
			effectiveValue = GetSegaCdToolingStatusByte(addr);
		} else if (Is32xSh2StatusAddress(addr)) {
			effectiveValue = Get32xSh2StatusByte(addr);
		} else if (Is32xCompositionStatusAddress(addr)) {
			effectiveValue = Get32xCompositionStatusByte(addr);
		} else if (Is32xToolingStatusAddress(addr)) {
			effectiveValue = Get32xToolingStatusByte(addr);
		} else if (Is32xCoprocStatusAddress(addr)) {
			effectiveValue = Get32xCoprocStatusByte(addr);
		} else if (Is32xHostToolingStatusAddress(addr)) {
			effectiveValue = Get32xHostToolingStatusByte(addr);
		} else if (IsLegacyBridgePassThroughAddress(addr)) {
			effectiveValue = bridgeSlot[bridgeIndex];
		}
		TrackSegaCdTranscript(addr, false, effectiveValue);
		_openBus = effectiveValue;
		return effectiveValue;
	}

	if (IsZ80BusReqAddress(addr)) [[unlikely]] {
		// Z80 bus request: bit 0 indicates bus grant, mirrored on both byte lanes.
		uint8_t ackStatus = GetZ80BusAckStatusBit(_z80BusAck);
		uint8_t effectiveValue = (uint8_t)(0xFE | ackStatus);
		static uint64_t z80BusReqReadCount = 0;
		z80BusReqReadCount++;
		if (z80BusReqReadCount <= 256 || (z80BusReqReadCount % 2048) == 0) {
			uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffff) : 0xffffffff;
			MessageManager::Log(std::format("[Genesis][MMU] Z80 busreq read #{} addr=${:06x} val=${:02x} ack={} pc=${:06x} busReq={} reset={}",
				z80BusReqReadCount,
				addr,
				effectiveValue,
				ackStatus,
				pc,
				_z80BusRequest ? 1 : 0,
				_z80Reset ? 1 : 0));
		}
		TrackSegaCdHandshakeTranscript(addr, false, effectiveValue);
		if (_vdp) {
			uint32_t frame = _vdp->GetFrameCount();
			uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffffu) : 0xffffffffu;
			if (ShouldTraceStartupLoopPoll(frame, pc)) {
				TraceStartupEvent("LOOP_POLL8", addr, effectiveValue, (uint16_t)(pc & 0xFFFFu));
			}
		}
		_openBus = effectiveValue;
		return effectiveValue;
	}

	if (IsZ80ResetAddress(addr)) [[unlikely]] {
		uint8_t resetStatus = _z80Reset ? 0x01 : 0x00;
		uint8_t effectiveValue = resetStatus;
		static uint64_t z80ResetReadCount = 0;
		z80ResetReadCount++;
		if (z80ResetReadCount <= 256 || (z80ResetReadCount % 2048) == 0) {
			uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffff) : 0xffffffff;
			MessageManager::Log(std::format("[Genesis][MMU] Z80 reset read #{} addr=${:06x} val=${:02x} status={} pc=${:06x} busReq={} reset={}",
				z80ResetReadCount,
				addr,
				effectiveValue,
				resetStatus,
				pc,
				_z80BusRequest ? 1 : 0,
				_z80Reset ? 1 : 0));
		}
		TrackSegaCdHandshakeTranscript(addr, false, effectiveValue);
		if (_vdp) {
			uint32_t frame = _vdp->GetFrameCount();
			uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffffu) : 0xffffffffu;
			if (ShouldTraceStartupLoopPoll(frame, pc)) {
				TraceStartupEvent("LOOP_POLL8", addr, effectiveValue, (uint16_t)(pc & 0xFFFFu));
			}
		}
		_openBus = effectiveValue;
		return effectiveValue;
	}

	uint8_t effectiveValue = _openBus;
	_openBus = effectiveValue;
	return effectiveValue;
}

uint16_t GenesisMemoryManager::Read16(uint32_t addr) {
	addr &= 0xFFFFFE;
	AdvanceZ80BusArbitration(7);
	UpdateZ80RuntimeState(false, addr, _cpu ? (_cpu->GetState().PC & 0x00ffffff) : 0xffffffff, "read16");
	if (addr == 0xA11000) [[unlikely]] {
		uint16_t effectiveValue = 0x0000;
		_openBus = 0x00;
		return effectiveValue;
	}
	if (addr == 0xA14100) [[unlikely]] {
		uint16_t effectiveValue = 0xFFFF;
		_openBus = 0xFF;
		TraceStartupEvent("TMSS_CART_R16", addr, effectiveValue, 0);
		return effectiveValue;
	}
	if (IsTmssAddress(addr)) [[unlikely]] {
		uint16_t effectiveValue = 0xFFFF;
		_openBus = 0xFF;
		TraceStartupEvent("TMSS_R16", addr, effectiveValue, _tmssUnlocked ? 1 : 0);
		return effectiveValue;
	}
	if (HasSaveRam() && addr >= _sramStart && addr <= _sramEnd) [[unlikely]] {
		uint8_t hi = Read8(addr);
		uint8_t lo = Read8(addr + 1);
		return ((uint16_t)hi << 8) | lo;
	}

	if (addr < 0x400000) [[likely]] {
		uint32_t mappedAddrHi = TranslateRomAddress(addr);
		uint32_t mappedAddrLo = TranslateRomAddress(addr + 1);
		uint8_t effectiveHighByte = _prgRom[mappedAddrHi];
		uint8_t effectiveLowByte = _prgRom[mappedAddrLo];
		uint16_t effectiveValue = ((uint16_t)effectiveHighByte << 8) | effectiveLowByte;
		_ioState.RomReadHeartbeat += 2;
		_emu->ProcessMemoryRead<CpuType::Genesis>(mappedAddrHi, effectiveHighByte, MemoryOperationType::Read);
		if (_vdp) {
			uint32_t frame = _vdp->GetFrameCount();
			uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffffu) : 0xffffffffu;
			if (ShouldTraceStartupLoopPoll(frame, pc)) {
				TraceStartupEvent("LOOP_POLL16", addr, effectiveValue, (uint16_t)(pc & 0xFFFFu));
			}
		}
		_openBus = (uint8_t)(effectiveValue & 0xFF);
		return effectiveValue;
	}

	if (addr >= 0xE00000) [[likely]] {
		uint32_t offset = addr & 0xFFFF;
		uint8_t effectiveHighByte = _workRam[offset];
		uint8_t effectiveLowByte = _workRam[(offset + 1) & 0xFFFF];
		uint16_t effectiveValue = ((uint16_t)effectiveHighByte << 8) | effectiveLowByte;
		_emu->ProcessMemoryRead<CpuType::Genesis>(addr, effectiveHighByte, MemoryOperationType::Read);
		if (_vdp) {
			uint32_t frame = _vdp->GetFrameCount();
			uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffffu) : 0xffffffffu;
			if (ShouldTraceStartupLoopPoll(frame, pc)) {
				TraceStartupEvent("LOOP_POLL16", addr, effectiveValue, (uint16_t)(pc & 0xFFFFu));
			}
		}
		_openBus = effectiveLowByte;
		return effectiveValue;
	}

	if (addr >= 0xC00000 && addr <= 0xC0001F) [[unlikely]] {
		if (IsTmssVdpLockEnforced()) {
			if (!IsTmssLockedVdpReadAllowed(addr)) {
				if (!_tmssVdpBlockLogged) {
					_tmssVdpBlockLogged = true;
					MessageManager::Log(std::format("[Genesis][MMU] TMSS is locking VDP read16 access at ${:06x}", addr));
				}
				uint16_t effectiveValue = (uint16_t)((_openBus << 8) | _openBus);
				_openBus = (uint8_t)(effectiveValue & 0xFF);
				TraceStartupEvent("TMSS_VDP_R16_BLOCK", addr, effectiveValue, _tmssUnlocked ? 1 : 0);
				return effectiveValue;
			}

			if (!_tmssStartupBypassLogged) {
				_tmssStartupBypassLogged = true;
				MessageManager::Log(std::format("[Genesis][MMU] TMSS startup compatibility allows VDP read16 at ${:06x}", addr));
			}
			TraceStartupEvent("TMSS_VDP_R16_ALLOW", addr, 0, IsStartupWindowActive() ? 1 : 0);
		}
		uint16_t effectiveValue = ReadVdpPort(addr);
		if (_vdp) {
			uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffffu) : 0xffffffffu;
			uint32_t frame = _vdp->GetFrameCount();
			if (ShouldTraceStartupLoopPoll(frame, pc)) {
				TraceStartupEvent("LOOP_POLL16", addr, effectiveValue, (uint16_t)(pc & 0xFFFFu));
			}
		}
		_openBus = (uint8_t)(effectiveValue & 0xFF);
		return effectiveValue;
	}

	if (addr >= 0xA00000 && addr <= 0xA0FFFF) [[unlikely]] {
		if (IsYm2612Address(addr)) {
			uint8_t effectiveHighByte = 0x00;
			uint8_t effectiveLowByte = 0x00;
			uint16_t effectiveValue = (uint16_t)(((uint16_t)effectiveHighByte << 8) | effectiveLowByte);
			_openBus = effectiveLowByte;
			return effectiveValue;
		}

		if (IsZ80BusGranted()) {
			uint32_t z80Addr = addr & 0x1FFF;
			uint8_t effectiveHighByte = _z80Ram[z80Addr];
			uint8_t effectiveLowByte = _z80Ram[(z80Addr + 1) & 0x1FFF];
			uint16_t effectiveValue = (uint16_t)(((uint16_t)effectiveHighByte << 8) | effectiveLowByte);
			_openBus = (uint8_t)(effectiveValue & 0xFF);
			return effectiveValue;
		}
		uint16_t effectiveValue = 0xFFFF;
		_openBus = (uint8_t)(effectiveValue & 0xFF);
		return effectiveValue;
	}

	if (addr >= 0xA10000 && addr <= 0xA1001F) [[unlikely]] {
		// 68k word access targets even addresses; Genesis I/O registers are byte-mapped on odd addresses.
		// Keep the high byte neutral and read the register from addr+1.
		uint8_t effectiveHi = 0x00;
		uint8_t effectiveLo = ReadIo(addr + 1);
		uint16_t effectiveValue = ((uint16_t)effectiveHi << 8) | effectiveLo;
		_openBus = effectiveLo;
		return effectiveValue;
	}

	if (addr >= 0xA13000 && addr <= 0xA130FE) [[unlikely]] {
		uint8_t effectiveHi = Read8(addr);
		uint8_t effectiveLo = Read8(addr + 1);
		return ((uint16_t)effectiveHi << 8) | effectiveLo;
	}

	uint8_t* bridgeSlot = nullptr;
	uint32_t bridgeIndex = 0;
	if (TryGetSegaCdBridgeSlot(addr, bridgeSlot, bridgeIndex)) [[unlikely]] {
		uint8_t effectiveHi = Read8(addr);
		uint8_t effectiveLo = Read8(addr + 1);
		return ((uint16_t)effectiveHi << 8) | effectiveLo;
	}

	if (IsZ80BusReqAddress(addr)) [[unlikely]] {
		uint8_t ackStatus = GetZ80BusAckStatusBit(_z80BusAck);
		uint8_t busStatus = (uint8_t)(0xFE | ackStatus);
		uint16_t effectiveValue = (uint16_t)(((uint16_t)busStatus << 8) | busStatus);
		uint8_t effectiveHighByte = (uint8_t)(effectiveValue >> 8);
		TrackSegaCdHandshakeTranscript(addr, false, effectiveHighByte);
		if (_vdp) {
			uint32_t frame = _vdp->GetFrameCount();
			uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffffu) : 0xffffffffu;
			if (ShouldTraceStartupLoopPoll(frame, pc)) {
				TraceStartupEvent("LOOP_POLL16", addr, effectiveValue, (uint16_t)(pc & 0xFFFFu));
			}
		}
		_openBus = busStatus;
		return effectiveValue;
	}

	if (IsZ80ResetAddress(addr)) [[unlikely]] {
		uint16_t effectiveValue = _z80Reset ? 0x0101 : 0x0000;
		uint8_t effectiveHighByte = (uint8_t)(effectiveValue >> 8);
		TrackSegaCdHandshakeTranscript(addr, false, effectiveHighByte);
		if (_vdp) {
			uint32_t frame = _vdp->GetFrameCount();
			uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffffu) : 0xffffffffu;
			if (ShouldTraceStartupLoopPoll(frame, pc)) {
				TraceStartupEvent("LOOP_POLL16", addr, effectiveValue, (uint16_t)(pc & 0xFFFFu));
			}
		}
		_openBus = (uint8_t)(effectiveValue & 0xFF);
		return effectiveValue;
	}

	uint16_t effectiveValue = (uint16_t)((_openBus << 8) | _openBus);
	_openBus = (uint8_t)(effectiveValue & 0xFF);
	return effectiveValue;
}

void GenesisMemoryManager::Write8(uint32_t addr, uint8_t value) {
	addr &= 0xFFFFFF;
	AdvanceZ80BusArbitration(7);
	UpdateZ80RuntimeState(false, addr, _cpu ? (_cpu->GetState().PC & 0x00ffffff) : 0xffffffff, "write8-pre");
	uint32_t sramOffset = 0;
	if (addr == 0xA11000 || addr == 0xA11001) [[unlikely]] {
		return;
	}
	if (addr >= 0xA13000 && addr <= 0xA130FF) [[unlikely]] {
		if (TryWriteRomBankRegister(addr, value)) {
			uint8_t effectiveValue = (uint8_t)(value & 0x3F);
			_openBus = effectiveValue;
			return;
		}
		if (IsRamControlRegister(addr)) {
			uint8_t effectiveValue = value;
			WriteRamControlRegister(effectiveValue);
			_openBus = effectiveValue;
			return;
		}
		return;
	}
	if ((addr & 0xFFFFFE) == 0xA14100) [[unlikely]] {
		// TMSS/cart byte writes are no-op on this path.
		TraceStartupEvent("TMSS_CART_W8", addr, value, 0);
		return;
	}
	if (IsTmssCartAddress(addr)) [[unlikely]] {
		// TMSS/cart byte writes are no-op on this path.
		TraceStartupEvent("TMSS_CART_W8", addr, value, 0);
		return;
	}
	if (IsTmssAddress(addr)) [[unlikely]] {
		uint8_t effectiveValue = value;
		uint32_t slot = addr & 0x03;
		_segaCdBridgeA140[slot] = effectiveValue;
		_openBus = effectiveValue;
		EvaluateTmssUnlockState(true, addr, effectiveValue, true);
		TraceStartupEvent("TMSS_W8", addr, effectiveValue, _tmssUnlocked ? 1 : (_tmssUnlockPending ? 2 : 0));
		if (_tmssEnabled) {
			MessageManager::Log(std::format("[Genesis][MMU] TMSS write ${:06x}=${:02x} state='{}{}{}{}' unlocked={} pending={} delay={}",
				addr,
				effectiveValue,
				(char)_segaCdBridgeA140[0],
				(char)_segaCdBridgeA140[1],
				(char)_segaCdBridgeA140[2],
				(char)_segaCdBridgeA140[3],
				_tmssUnlocked ? "true" : "false",
				_tmssUnlockPending ? 1 : 0,
				_tmssUnlockDelayMclk));
		}
		return;
	}

	if (TryGetSramOffset(addr, sramOffset)) [[unlikely]] {
		if (!_ramWritable) {
			_openBus = value;
			return;
		}

		uint8_t effectiveValue = value;
		_emu->ProcessMemoryWrite<CpuType::Genesis>(addr, effectiveValue, MemoryOperationType::Write);
		_saveRam[sramOffset] = effectiveValue;
		_openBus = effectiveValue;
		return;
	}

	if (addr < 0x400000) [[likely]] {
		uint8_t effectiveValue = value;
		_openBus = effectiveValue;
		TrackSegaCdTranscript(addr, true, effectiveValue);
		return;
	}

	if (addr >= 0xE00000) [[likely]] {
		uint32_t offset = addr & 0xFFFF;
		uint8_t effectiveValue = value;
		_emu->ProcessMemoryWrite<CpuType::Genesis>(addr, effectiveValue, MemoryOperationType::Write);
		_workRam[offset] = effectiveValue;
		if (_vdp) {
			uint32_t frame = _vdp->GetFrameCount();
			if (ShouldLogNexenWramTrace(frame, addr)) {
				uint16_t line = _vdp->GetState().VCounter;
				uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffff) : 0;
				LogNexenWramTrace(frame, line, addr, effectiveValue, pc, _masterClock);
			}
		}
		_openBus = effectiveValue;
		return;
	}

	if (addr >= 0xC00000 && addr <= 0xC0001F) [[unlikely]] {
		uint8_t effectiveValue = value;
		static uint64_t vdpWrite8GateCount = 0;
		if (vdpWrite8GateCount < 64) {
			uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffff) : 0xffffffff;
			MessageManager::Log(std::format("[Genesis][MMU] VDP write8 gate #{} addr=${:06x} val=${:02x} pc=${:06x}",
				vdpWrite8GateCount + 1,
				addr,
				effectiveValue,
				pc));
		}
		vdpWrite8GateCount++;
		if (IsTmssVdpLockEnforced()) {
			if (!IsTmssLockedVdpWriteAllowed(addr)) {
				if (!_tmssVdpBlockLogged) {
					_tmssVdpBlockLogged = true;
					MessageManager::Log(std::format("[Genesis][MMU] TMSS is locking VDP write8 access at ${:06x}", addr));
				}
				TraceStartupEvent("TMSS_VDP_W8_BLOCK", addr, effectiveValue, 0);
				_openBus = effectiveValue;
				return;
			}

			if (!_tmssStartupBypassLogged) {
				_tmssStartupBypassLogged = true;
				MessageManager::Log(std::format("[Genesis][MMU] TMSS startup compatibility allows VDP write8 at ${:06x}", addr));
			}
			TraceStartupEvent("TMSS_VDP_W8_ALLOW", addr, effectiveValue, IsStartupWindowActive() ? 1 : 0);
		}
		uint32_t port = addr & 0x1F;
		if (_vdp && port < 0x04) {
			TraceStartupEvent("VDP_DATA_W", addr, effectiveValue, (uint16_t)port);
			_vdp->WriteDataPortByte(effectiveValue, (addr & 1u) == 0u);
		} else if (_vdp && port < 0x08) {
			GenesisVdpState stateBeforeWrite = _vdp->GetState();
			TraceStartupEvent("VDP_CTRL_W", addr, effectiveValue, (uint16_t)port);
			_vdp->WriteControlPortByte(effectiveValue, (addr & 1u) == 0u);
			GenesisVdpState stateAfterWrite = _vdp->GetState();
			for (uint32_t reg = 0; reg < 24; reg++) {
				uint8_t oldValue = stateBeforeWrite.Registers[reg];
				uint8_t newValue = stateAfterWrite.Registers[reg];
				if (oldValue != newValue) {
					uint16_t packed = (uint16_t)(((uint16_t)oldValue << 8) | newValue);
					TraceStartupEvent("VDP_REG_W", 0xC00004, packed, (uint16_t)reg);
				}
			}
			if (stateBeforeWrite.StatusRegister != stateAfterWrite.StatusRegister) {
				TraceStartupEvent("VDP_STAT_W", 0xC00004, stateAfterWrite.StatusRegister, (uint16_t)(stateBeforeWrite.StatusRegister ^ stateAfterWrite.StatusRegister));
			}
		} else {
			// Non data/control VDP byte writes keep legacy behavior.
			WriteVdpPort(addr, (uint16_t)effectiveValue | ((uint16_t)effectiveValue << 8));
		}
		_openBus = effectiveValue;
		return;
	}

	if (addr >= 0xA00000 && addr <= 0xA0FFFF) [[unlikely]] {
		uint8_t effectiveValue = value;
		if (IsYm2612Address(addr)) {
			_openBus = effectiveValue;
			return;
		}

		static uint64_t z80WindowWriteCount = 0;
		z80WindowWriteCount++;
		if (IsZ80BusGranted()) {
			uint32_t z80Addr = addr & 0x1FFF;
			_z80Ram[z80Addr] = effectiveValue;
			bool traceAccess = z80WindowWriteCount <= 256 || (z80WindowWriteCount % 4096) == 0 || (z80Addr >= 0x1ff0);
			if (traceAccess) {
				uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffff) : 0xffffffff;
				MessageManager::Log(std::format("[Genesis][MMU] Z80 write #{} addr=${:06x} z80=${:04x} val=${:02x} pc=${:06x} busReq={} reset={} gate=allow",
					z80WindowWriteCount,
					addr,
					z80Addr,
					effectiveValue,
					pc,
					_z80BusRequest ? 1 : 0,
					_z80Reset ? 1 : 0));
			}
		} else if (z80WindowWriteCount <= 256 || (z80WindowWriteCount % 4096) == 0) {
			uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffff) : 0xffffffff;
			MessageManager::Log(std::format("[Genesis][MMU] Z80 write #{} addr=${:06x} val=${:02x} pc=${:06x} busReq={} reset={} gate=blocked",
				z80WindowWriteCount,
				addr,
				effectiveValue,
				pc,
				_z80BusRequest ? 1 : 0,
				_z80Reset ? 1 : 0));
		}
		_openBus = effectiveValue;
		return;
	}

	if (addr >= 0xA10000 && addr <= 0xA1001F) [[unlikely]] {
		uint8_t effectiveValue = value;
		WriteIo(addr, effectiveValue);
		return;
	}

	if (TryWriteRomBankRegister(addr, value)) [[unlikely]] {
		uint8_t effectiveValue = (uint8_t)(value & 0x3F);
		_openBus = effectiveValue;
		return;
	}

	if (IsRamControlRegister(addr)) [[unlikely]] {
		uint8_t effectiveValue = value;
		WriteRamControlRegister(effectiveValue);
		_openBus = effectiveValue;
		return;
	}

	uint8_t* bridgeSlot = nullptr;
	uint32_t bridgeIndex = 0;
	if (TryGetSegaCdBridgeSlot(addr, bridgeSlot, bridgeIndex)) [[unlikely]] {
		if (!IsBridgeModeledWriteAddress(addr)) {
			return;
		}

		uint8_t effectiveValue = value;
		if (Is32xCoprocControlAddress(addr)) {
			effectiveValue = Normalize32xCoprocControlValue(addr, effectiveValue);
		} else if (Is32xHostToolingControlAddress(addr)) {
			effectiveValue = Normalize32xHostControlValue(addr, effectiveValue);
		}
		bridgeSlot[bridgeIndex] = effectiveValue;
		_openBus = effectiveValue;
		if (IsSegaCdSubCpuControlAddress(addr)) {
			UpdateSegaCdSubCpuControl(effectiveValue);
		} else if (IsSegaCdAudioDataAddress(addr)) {
			UpdateSegaCdAudioPath(addr, effectiveValue);
		} else if (IsSegaCdToolingControlAddress(addr)) {
			UpdateSegaCdToolingContract(addr, effectiveValue);
		} else if (Is32xSh2ControlAddress(addr)) {
			Update32xSh2Staging(addr, effectiveValue);
		} else if (Is32xCompositionControlAddress(addr)) {
			Update32xCompositionStaging(addr, effectiveValue);
		} else if (Is32xToolingControlAddress(addr)) {
			Update32xToolingContract(addr, effectiveValue);
		} else if (Is32xCoprocControlAddress(addr)) {
			Update32xCoprocContract(addr, effectiveValue);
		} else if (Is32xHostToolingControlAddress(addr)) {
			Update32xHostToolingContract(addr, effectiveValue);
		}
		TrackSegaCdTranscript(addr, true, effectiveValue);
		return;
	}

	if (IsZ80BusReqAddress(addr)) [[unlikely]] {
		uint8_t effectiveValue = value;
		if (!_z80LatchOnlyHighByteWrites || !(addr & 0x01)) {
			static uint64_t z80BusReqWriteCount = 0;
			z80BusReqWriteCount++;
			uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffff) : 0xffffffff;
			bool request = (effectiveValue & 0x01) != 0;
			SetZ80BusRequest(request, true, addr, pc, "write8-busreq");
			if ((pc >= 0x071b00 && pc <= 0x071cff) || z80BusReqWriteCount <= 256 || (z80BusReqWriteCount % 2048) == 0) {
				MessageManager::Log(std::format("[Genesis][MMU] Z80 busreq write8 #{} addr=${:06x} val=${:02x} pc=${:06x} req={} reset={} ack={} reqDelay={} resumeDelay={}",
					z80BusReqWriteCount,
					addr,
					effectiveValue,
					pc,
					request ? 1 : 0,
					_z80Reset ? 1 : 0,
					_z80BusAck ? 1 : 0,
					_z80BusReqDelayMclk,
					_z80ResumeDelayMclk));
			}
		}
		TrackSegaCdHandshakeTranscript(addr, true, effectiveValue);
		return;
	}

	if (IsZ80ResetAddress(addr)) [[unlikely]] {
		uint8_t effectiveValue = value;
		if (!_z80LatchOnlyHighByteWrites || !(addr & 0x01)) {
			static uint64_t z80ResetWriteCount = 0;
			z80ResetWriteCount++;
			uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffff) : 0xffffffff;
			bool resetAsserted = !(effectiveValue & 0x01);
			SetZ80Reset(resetAsserted, true, addr, pc, "write8-reset");
			if ((pc >= 0x071b00 && pc <= 0x071cff) || z80ResetWriteCount <= 256 || (z80ResetWriteCount % 2048) == 0) {
				MessageManager::Log(std::format("[Genesis][MMU] Z80 reset write8 #{} addr=${:06x} val=${:02x} pc=${:06x} reset={} req={} ack={} reqDelay={} resumeDelay={}",
					z80ResetWriteCount,
					addr,
					effectiveValue,
					pc,
					resetAsserted ? 1 : 0,
					_z80BusRequest ? 1 : 0,
					_z80BusAck ? 1 : 0,
					_z80BusReqDelayMclk,
					_z80ResumeDelayMclk));
			}
		}
		TrackSegaCdHandshakeTranscript(addr, true, effectiveValue);
		return;
	}

	// Unmapped/ROM area — ignore writes (no mapper for now)
	uint8_t effectiveValue = value;
	_openBus = effectiveValue;
}

void GenesisMemoryManager::Write16(uint32_t addr, uint16_t value) {
	addr &= 0xFFFFFE;
	AdvanceZ80BusArbitration(7);
	UpdateZ80RuntimeState(false, addr, _cpu ? (_cpu->GetState().PC & 0x00ffffff) : 0xffffffff, "write16-pre");
	if (addr == 0xA11000) [[unlikely]] {
		return;
	}
	if (addr == 0xA14100) [[unlikely]] {
		// TMSS/cart word writes are no-op.
		TraceStartupEvent("TMSS_CART_W16", addr, value, 0);
		return;
	}
	if (IsTmssAddress(addr)) [[unlikely]] {
		uint8_t effectiveHighByte = (uint8_t)(value >> 8);
		uint8_t effectiveLowByte = (uint8_t)(value & 0xFF);
		Write8(addr, effectiveHighByte);
		Write8(addr + 1, effectiveLowByte);
		return;
	}
	if (HasSaveRam() && addr >= _sramStart && addr <= _sramEnd) [[unlikely]] {
		if (!_ramEnable || !_ramWritable) {
			uint8_t effectiveLowByte = (uint8_t)(value & 0xFF);
			_openBus = effectiveLowByte;
			return;
		}

		uint8_t effectiveHighByte = (uint8_t)(value >> 8);
		uint8_t effectiveLowByte = (uint8_t)(value & 0xFF);
		Write8(addr, effectiveHighByte);
		Write8(addr + 1, effectiveLowByte);
		return;
	}

	if (addr >= 0xA13000 && addr <= 0xA130FE) [[unlikely]] {
		uint8_t effectiveHighByte = (uint8_t)(value >> 8);
		uint8_t effectiveLowByte = (uint8_t)(value & 0xFF);
		Write8(addr, effectiveHighByte);
		Write8(addr + 1, effectiveLowByte);
		return;
	}

	if (addr < 0x400000) [[likely]] {
		uint16_t effectiveValue = value;
		uint8_t effectiveLowByte = (uint8_t)(effectiveValue & 0xFF);
		_openBus = effectiveLowByte;
		return;
	}

	if (addr >= 0xE00000) [[likely]] {
		uint32_t offset = addr & 0xFFFF;
		uint8_t effectiveHighByte = (uint8_t)(value >> 8);
		uint8_t effectiveLowByte = (uint8_t)(value & 0xFF);
		_emu->ProcessMemoryWrite<CpuType::Genesis>(addr, effectiveHighByte, MemoryOperationType::Write);
		_workRam[offset] = effectiveHighByte;
		_workRam[(offset + 1) & 0xFFFF] = effectiveLowByte;
		if (_vdp) {
			uint32_t frame = _vdp->GetFrameCount();
			if (ShouldLogNexenWramTrace(frame, addr)) {
				uint16_t line = _vdp->GetState().VCounter;
				uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffff) : 0;
				LogNexenWramTrace(frame, line, addr, effectiveHighByte, pc, _masterClock);
			}
			uint32_t lowAddress = (addr + 1) & 0xFFFFFF;
			if (ShouldLogNexenWramTrace(frame, lowAddress)) {
				uint16_t line = _vdp->GetState().VCounter;
				uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffff) : 0;
				LogNexenWramTrace(frame, line, lowAddress, effectiveLowByte, pc, _masterClock);
			}
		}
		_openBus = effectiveLowByte;
		return;
	}

	if (addr >= 0xC00000 && addr <= 0xC0001F) [[unlikely]] {
		uint16_t effectiveValue = value;
		uint8_t effectiveLowByte = (uint8_t)(effectiveValue & 0xFF);
		static uint64_t vdpWrite16GateCount = 0;
		if (vdpWrite16GateCount < 128) {
			uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffff) : 0xffffffff;
			MessageManager::Log(std::format("[Genesis][MMU] VDP write16 gate #{} addr=${:06x} val=${:04x} pc=${:06x}",
				vdpWrite16GateCount + 1,
				addr,
				effectiveValue,
				pc));
		}
		vdpWrite16GateCount++;
		if (IsTmssVdpLockEnforced()) {
			if (!IsTmssLockedVdpWriteAllowed(addr)) {
				TraceStartupEvent("TMSS_VDP_W16_BLOCK", addr, effectiveValue, 0);
				_openBus = effectiveLowByte;
				return;
			}

			if (!_tmssStartupBypassLogged) {
				_tmssStartupBypassLogged = true;
				MessageManager::Log(std::format("[Genesis][MMU] TMSS startup compatibility allows VDP write16 at ${:06x}", addr));
			}
			TraceStartupEvent("TMSS_VDP_W16_ALLOW", addr, effectiveValue, IsStartupWindowActive() ? 1 : 0);
		}
		WriteVdpPort(addr, effectiveValue);
		_openBus = effectiveLowByte;
		return;
	}

	if (addr >= 0xA00000 && addr <= 0xA0FFFF) [[unlikely]] {
		uint8_t effectiveHighByte = (uint8_t)(value >> 8);
		uint8_t effectiveLowByte = (uint8_t)(value & 0xFF);
		if (IsYm2612Address(addr)) {
			_openBus = effectiveLowByte;
			return;
		}

		if (IsZ80BusGranted()) {
			uint32_t z80Addr = addr & 0x1FFF;
			_z80Ram[z80Addr] = effectiveHighByte;
			_z80Ram[(z80Addr + 1) & 0x1FFF] = effectiveLowByte;
		}
		_openBus = effectiveLowByte;
		return;
	}

	if (addr >= 0xA10000 && addr <= 0xA1001F) [[unlikely]] {
		uint8_t effectiveLowByte = (uint8_t)(value & 0xFF);
		// 68k word writes hit even addresses; only the low byte maps to the odd-byte I/O register.
		WriteIo(addr + 1, effectiveLowByte);
		_openBus = effectiveLowByte;
		return;
	}

	uint8_t* bridgeSlot = nullptr;
	uint32_t bridgeIndex = 0;
	if (TryGetSegaCdBridgeSlot(addr, bridgeSlot, bridgeIndex)) [[unlikely]] {
		uint16_t effectiveValue = value;
		uint8_t effectiveHighByte = (uint8_t)(effectiveValue >> 8);
		uint8_t effectiveLowByte = (uint8_t)(effectiveValue & 0xFF);
		Write8(addr, effectiveHighByte);
		Write8(addr + 1, effectiveLowByte);
		return;
	}

	if (IsZ80BusReqAddress(addr)) [[unlikely]] {
		uint16_t effectiveValue = value;
		uint8_t effectiveHighByte = (uint8_t)(effectiveValue >> 8);
		bool request = (effectiveHighByte & 0x01) != 0;
		static uint64_t z80BusReqWrite16Count = 0;
		z80BusReqWrite16Count++;
		uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffff) : 0xffffffff;
		SetZ80BusRequest(request, true, addr, pc, "write16-busreq");
		if ((pc >= 0x071b00 && pc <= 0x071cff) || z80BusReqWrite16Count <= 256 || (z80BusReqWrite16Count % 2048) == 0) {
			MessageManager::Log(std::format("[Genesis][MMU] Z80 busreq write16 #{} addr=${:06x} val=${:04x} hi=${:02x} pc=${:06x} req={} reset={} ack={} reqDelay={} resumeDelay={}",
				z80BusReqWrite16Count,
				addr,
				effectiveValue,
				effectiveHighByte,
				pc,
				request ? 1 : 0,
				_z80Reset ? 1 : 0,
				_z80BusAck ? 1 : 0,
				_z80BusReqDelayMclk,
				_z80ResumeDelayMclk));
		}
		TrackSegaCdHandshakeTranscript(addr, true, effectiveHighByte);
		return;
	}

	if (IsZ80ResetAddress(addr)) [[unlikely]] {
		uint16_t effectiveValue = value;
		uint8_t effectiveHighByte = (uint8_t)(effectiveValue >> 8);
		uint8_t effectiveLowByte = (uint8_t)(effectiveValue & 0xFF);
		bool resetAsserted = !(effectiveHighByte & 0x01);
		static uint64_t z80ResetWrite16Count = 0;
		z80ResetWrite16Count++;
		uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffff) : 0xffffffff;
		SetZ80Reset(resetAsserted, true, addr, pc, "write16-reset");
		if ((pc >= 0x071b00 && pc <= 0x071cff) || z80ResetWrite16Count <= 256 || (z80ResetWrite16Count % 2048) == 0) {
			MessageManager::Log(std::format("[Genesis][MMU] Z80 reset write16 #{} addr=${:06x} val=${:04x} hi=${:02x} pc=${:06x} reset={} req={} ack={} reqDelay={} resumeDelay={}",
				z80ResetWrite16Count,
				addr,
				effectiveValue,
				effectiveHighByte,
				pc,
				resetAsserted ? 1 : 0,
				_z80BusRequest ? 1 : 0,
				_z80BusAck ? 1 : 0,
				_z80BusReqDelayMclk,
				_z80ResumeDelayMclk));
		}
		_openBus = effectiveLowByte;
		TrackSegaCdHandshakeTranscript(addr, true, effectiveHighByte);
		return;
	}

	uint8_t effectiveLowByte = (uint8_t)(value & 0xFF);
	_openBus = effectiveLowByte;
}

// VDP port access
uint16_t GenesisMemoryManager::ReadVdpPort(uint32_t addr) {
	uint32_t port = addr & 0x1F;
	if (port < 0x04) {
		uint16_t effectiveValue = _vdp->ReadDataPort();
		TraceStartupEvent("VDP_DATA_R", addr, effectiveValue, port);
		_openBus = (uint8_t)(effectiveValue & 0xFF);
		return effectiveValue;
	} else if (port < 0x08) {
		GenesisVdpState stateBeforeRead = _vdp->GetState();
		uint16_t effectiveValue = _vdp->ReadControlPort();
		static uint64_t controlPortReadCount = 0;
		controlPortReadCount++;
		if (controlPortReadCount <= 128 || (controlPortReadCount % 4096) == 0) {
			GenesisVdpState vdpState = _vdp->GetState();
			uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffff) : 0xffffffff;
			uint8_t statusLo = (uint8_t)(effectiveValue & 0xff);
			bool vblankBit = (statusLo & (uint8_t)VdpStatus::VBlankFlag) != 0;
			bool displayEnabled = (vdpState.Registers[1] & 0x40) != 0;
			MessageManager::Log(std::format("[Genesis][MMU] CtrlPortRead #{} addr=${:06x} pc=${:06x} status=${:04x} lo=${:02x} vb={} display={} vc={} hc={} r1=${:02x}",
				controlPortReadCount,
				addr & 0x00ffffff,
				pc & 0x00ffffff,
				effectiveValue,
				statusLo,
				vblankBit ? 1 : 0,
				displayEnabled ? 1 : 0,
				vdpState.VCounter,
				vdpState.HCounter,
				vdpState.Registers[1]));
			MessageManager::Log(std::format("[Genesis][MMU] CtrlPortReadState #{} beforeStatus=${:04x} afterStatus=${:04x} beforePending={} latchedPending={} afterPending={} beforeVc={} afterVc={} beforeHc={} afterHc={}",
				controlPortReadCount,
				stateBeforeRead.StatusRegister,
				vdpState.StatusRegister,
				(stateBeforeRead.StatusRegister & VdpStatus::VIntPending) ? 1 : 0,
				(effectiveValue & VdpStatus::VIntPending) ? 1 : 0,
				(vdpState.StatusRegister & VdpStatus::VIntPending) ? 1 : 0,
				stateBeforeRead.VCounter,
				vdpState.VCounter,
				stateBeforeRead.HCounter,
				vdpState.HCounter));
		}
		GenesisVdpState stateAfterRead = _vdp->GetState();
		if (stateAfterRead.FrameCount > 0u) {
			TraceStartupEvent("VDP_CTRL_R", addr, effectiveValue, (uint16_t)(stateBeforeRead.StatusRegister ^ stateAfterRead.StatusRegister));
		}
		_openBus = (uint8_t)(effectiveValue & 0xFF);
		return effectiveValue;
	} else if (port < 0x10) {
		uint16_t effectiveValue = _vdp->ReadHVCounter();
		TraceStartupEvent("VDP_HV_R", addr, effectiveValue, port);
		_openBus = (uint8_t)(effectiveValue & 0xFF);
		return effectiveValue;
	}
	uint16_t effectiveValue = _openBus;
	_openBus = (uint8_t)(effectiveValue & 0xFF);
	return effectiveValue;
}

void GenesisMemoryManager::WriteVdpPort(uint32_t addr, uint16_t value) {
	uint32_t port = addr & 0x1F;
	uint16_t effectiveValue = value;
	uint8_t effectiveHighByte = (uint8_t)(value >> 8);
	uint8_t effectiveLowByte = (uint8_t)(value & 0xFF);
	static uint64_t vdpPortWriteCount = 0;
	static bool loggedFirstNonZeroDataWrite = false;
	static bool loggedFirstNonZeroControlWrite = false;
	vdpPortWriteCount++;

	if (port < 0x04) {
		if (!loggedFirstNonZeroDataWrite && effectiveValue != 0) {
			loggedFirstNonZeroDataWrite = true;
			uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffff) : 0xffffffff;
			MessageManager::Log(std::format("[Genesis][MMU] First non-zero VDP data write #{} addr=${:06x} port=${:02x} word=${:04x} hi=${:02x} lo=${:02x} pc=${:06x}",
				vdpPortWriteCount,
				addr & 0x00ffffff,
				port,
				effectiveValue,
				effectiveHighByte,
				effectiveLowByte,
				pc));
		}
		TraceStartupEvent("VDP_DATA_W", addr, effectiveValue, port);
		_vdp->WriteDataPort(effectiveValue);
	} else if (port < 0x08) {
		GenesisVdpState stateBeforeWrite = _vdp->GetState();
		if (!loggedFirstNonZeroControlWrite && effectiveValue != 0) {
			loggedFirstNonZeroControlWrite = true;
			uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffff) : 0xffffffff;
			MessageManager::Log(std::format("[Genesis][MMU] First non-zero VDP control write #{} addr=${:06x} port=${:02x} word=${:04x} hi=${:02x} lo=${:02x} pc=${:06x}",
				vdpPortWriteCount,
				addr & 0x00ffffff,
				port,
				effectiveValue,
				effectiveHighByte,
				effectiveLowByte,
				pc));
		}
		TraceStartupEvent("VDP_CTRL_W", addr, effectiveValue, port);
		_vdp->WriteControlPort(effectiveValue);
		GenesisVdpState stateAfterWrite = _vdp->GetState();
		for (uint32_t reg = 0; reg < 24; reg++) {
			uint8_t oldValue = stateBeforeWrite.Registers[reg];
			uint8_t newValue = stateAfterWrite.Registers[reg];
			if (oldValue != newValue) {
				uint16_t packed = (uint16_t)(((uint16_t)oldValue << 8) | newValue);
				TraceStartupEvent("VDP_REG_W", 0xC00004, packed, (uint16_t)reg);
			}
		}
		if (stateBeforeWrite.StatusRegister != stateAfterWrite.StatusRegister) {
			TraceStartupEvent("VDP_STAT_W", 0xC00004, stateAfterWrite.StatusRegister, (uint16_t)(stateBeforeWrite.StatusRegister ^ stateAfterWrite.StatusRegister));
		}
	} else if (port >= 0x11 && port < 0x14) {
		// PSG write — SN76489 accepts byte writes via top byte of word
		if (_psg) {
			_psg->Write(effectiveHighByte);
		}
	}
	_openBus = effectiveLowByte;
}

// I/O registers ($A10001-$A1001F)
uint8_t GenesisMemoryManager::ReadIo(uint32_t addr) {
	if ((addr & 0x01) == 0) {
		uint8_t effectiveValue = 0x00;
		_openBus = effectiveValue;
		return effectiveValue;
	}

	uint32_t reg = addr & 0x1F;
	switch (reg) {
		case 0x01:
			{
				uint8_t effectiveValue = BuildVersionRegister(_console ? _console->GetRegion() : ConsoleRegion::Ntsc);
				_openBus = effectiveValue;
				return effectiveValue;
			}
		case 0x03:
			_ioState.DataPort[0] = _controlManager ? _controlManager->ReadDataPort(0) : 0x7F; // Controller 1 data
			_ioState.ThState[0] = _controlManager ? _controlManager->GetThState(0) : 0;
			_ioState.ThCount[0] = _controlManager ? _controlManager->GetThCount(0) : 0;
			{
				uint8_t effectiveValue = _ioState.DataPort[0];
				_openBus = effectiveValue;
				return effectiveValue;
			}
		case 0x05:
			_ioState.DataPort[1] = _controlManager ? _controlManager->ReadDataPort(1) : 0x7F; // Controller 2 data
			_ioState.ThState[1] = _controlManager ? _controlManager->GetThState(1) : 0;
			_ioState.ThCount[1] = _controlManager ? _controlManager->GetThCount(1) : 0;
			{
				uint8_t effectiveValue = _ioState.DataPort[1];
				_openBus = effectiveValue;
				return effectiveValue;
			}
		case 0x07:
			_ioState.DataPort[2] = 0xFF; // EXT port
			{
				uint8_t effectiveValue = _ioState.DataPort[2];
				_openBus = effectiveValue;
				return effectiveValue;
			}
		case 0x09:
			{
				uint8_t effectiveValue = _ioState.CtrlPort[0]; // Controller 1 ctrl
				_openBus = effectiveValue;
				return effectiveValue;
			}
		case 0x0B:
			{
				uint8_t effectiveValue = _ioState.CtrlPort[1]; // Controller 2 ctrl
				_openBus = effectiveValue;
				return effectiveValue;
			}
		case 0x0D:
			{
				uint8_t effectiveValue = _ioState.CtrlPort[2]; // EXT ctrl
				_openBus = effectiveValue;
				return effectiveValue;
			}
		default:
			{
				uint8_t effectiveValue = _openBus;
				_openBus = effectiveValue;
				return effectiveValue;
			}
	}
}

void GenesisMemoryManager::WriteIo(uint32_t addr, uint8_t value) {
	if ((addr & 0x01) == 0) {
		return;
	}

	uint32_t reg = addr & 0x1F;
	switch (reg) {
		case 0x03:
			{
				uint8_t effectiveValue = value;
				if (_controlManager) {
					_controlManager->WriteDataPort(0, effectiveValue);
					_ioState.DataPort[0] = _controlManager->GetDataPortWriteLatch(0);
					_ioState.ThState[0] = _controlManager->GetThState(0);
					_ioState.ThCount[0] = _controlManager->GetThCount(0);
				} else {
					_ioState.DataPort[0] = effectiveValue;
					_ioState.ThState[0] = 0;
					_ioState.ThCount[0] = 0;
				}
				break;
			}
		case 0x05:
			{
				uint8_t effectiveValue = value;
				if (_controlManager) {
					_controlManager->WriteDataPort(1, effectiveValue);
					_ioState.DataPort[1] = _controlManager->GetDataPortWriteLatch(1);
					_ioState.ThState[1] = _controlManager->GetThState(1);
					_ioState.ThCount[1] = _controlManager->GetThCount(1);
				} else {
					_ioState.DataPort[1] = effectiveValue;
					_ioState.ThState[1] = 0;
					_ioState.ThCount[1] = 0;
				}
				break;
			}
		case 0x09:
			{
				uint8_t effectiveValue = value;
				_ioState.CtrlPort[0] = effectiveValue;
				break;
			}
		case 0x0B:
			{
				uint8_t effectiveValue = value;
				_ioState.CtrlPort[1] = effectiveValue;
				break;
			}
		case 0x0D:
			{
				uint8_t effectiveValue = value;
				_ioState.CtrlPort[2] = effectiveValue;
				break;
			}
	}
	uint8_t effectiveValue = value;
	_openBus = effectiveValue;
}

uint8_t GenesisMemoryManager::DebugRead8(uint32_t addr) {
	addr &= 0xFFFFFF;
	AdvanceZ80BusArbitration(7);
	UpdateZ80RuntimeState(false, addr, _cpu ? (_cpu->GetState().PC & 0x00ffffff) : 0xffffffff, "debugread8");
	uint32_t effectiveAddr = addr;
	if (effectiveAddr == 0xA11000 || effectiveAddr == 0xA11001) {
		uint8_t effectiveValue = 0x00;
		_openBus = effectiveValue;
		TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x12);
		return effectiveValue;
	}
	if (effectiveAddr == 0xA14100) {
		uint8_t effectiveValue = 0xFF;
		_openBus = effectiveValue;
		TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x02);
		return effectiveValue;
	}
	if (IsTmssCartAddress(effectiveAddr)) {
		uint8_t effectiveValue = 0xFF;
		_openBus = effectiveValue;
		TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x02);
		return effectiveValue;
	}
	if (IsTmssAddress(effectiveAddr)) {
		uint8_t effectiveValue = 0xFF;
		_openBus = effectiveValue;
		TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x02);
		return effectiveValue;
	}
	if (effectiveAddr < 0x400000) {
		uint32_t mappedAddr = TranslateRomAddress(effectiveAddr);
		uint8_t effectiveValue = _prgRom[mappedAddr];
		_openBus = effectiveValue;
		TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x01);
		return effectiveValue;
	}
	if (effectiveAddr >= 0xE00000) {
		uint8_t effectiveValue = _workRam[effectiveAddr & 0xFFFF];
		_openBus = effectiveValue;
		TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x04);
		return effectiveValue;
	}
	if (effectiveAddr >= 0xA10000 && effectiveAddr <= 0xA1001F) {
		if ((effectiveAddr & 0x01) == 0) {
			uint8_t effectiveValue = 0x00;
			_openBus = effectiveValue;
			TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x10);
			return effectiveValue;
		}

		uint32_t reg = effectiveAddr & 0x1F;
		uint8_t effectiveValue = _openBus;
		switch (reg) {
			case 0x01:
				{
					uint8_t statusByte = BuildVersionRegister(_console ? _console->GetRegion() : ConsoleRegion::Ntsc);
					effectiveValue = statusByte;
				}
				break;
			case 0x03:
				_ioState.DataPort[0] = _controlManager ? _controlManager->ReadDataPort(0) : 0x7F;
				_ioState.ThState[0] = _controlManager ? _controlManager->GetThState(0) : 0;
				_ioState.ThCount[0] = _controlManager ? _controlManager->GetThCount(0) : 0;
				{
					uint8_t statusByte = _ioState.DataPort[0];
					effectiveValue = statusByte;
				}
				break;
			case 0x05:
				_ioState.DataPort[1] = _controlManager ? _controlManager->ReadDataPort(1) : 0x7F;
				_ioState.ThState[1] = _controlManager ? _controlManager->GetThState(1) : 0;
				_ioState.ThCount[1] = _controlManager ? _controlManager->GetThCount(1) : 0;
				{
					uint8_t statusByte = _ioState.DataPort[1];
					effectiveValue = statusByte;
				}
				break;
			case 0x07:
				{
					uint8_t statusByte = 0xFF;
					effectiveValue = statusByte;
				}
				break;
			case 0x09:
				{
					uint8_t statusByte = _ioState.CtrlPort[0];
					effectiveValue = statusByte;
				}
				break;
			case 0x0B:
				{
					uint8_t statusByte = _ioState.CtrlPort[1];
					effectiveValue = statusByte;
				}
				break;
			case 0x0D:
				{
					uint8_t statusByte = _ioState.CtrlPort[2];
					effectiveValue = statusByte;
				}
				break;
			default:
				{
					uint8_t statusByte = _openBus;
					effectiveValue = statusByte;
				}
				break;
		}
		_openBus = effectiveValue;
		TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x10);
		return effectiveValue;
	}
	if (effectiveAddr >= 0xA13000 && effectiveAddr <= 0xA130FF) {
		uint8_t bankSlot = 0;
		uint8_t effectiveValue = 0x00;
		if (IsRamControlRegister(effectiveAddr)) {
			effectiveValue = GetRamControlRegisterValue();
		} else if (TryGetRomBankRegisterSlot(effectiveAddr, bankSlot)) {
			effectiveValue = _romBankRegisters[bankSlot];
		}
		_openBus = effectiveValue;
		TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x11);
		return effectiveValue;
	}
	uint8_t bankSlot = 0;
	if (IsRamControlRegister(effectiveAddr)) {
		uint8_t effectiveValue = GetRamControlRegisterValue();
		_openBus = effectiveValue;
		TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x11);
		return effectiveValue;
	}

	if (TryGetRomBankRegisterSlot(effectiveAddr, bankSlot)) {
		uint8_t effectiveValue = _romBankRegisters[bankSlot];
		_openBus = effectiveValue;
		TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x11);
		return effectiveValue;
	}
	if (IsZ80BusReqAddress(effectiveAddr)) {
		uint8_t ackStatus = GetZ80BusAckStatusBit(_z80BusAck);
		uint8_t effectiveValue = (uint8_t)(0xFE | ackStatus);
		TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x82);
		return effectiveValue;
	}
	if (IsZ80ResetAddress(effectiveAddr)) {
		uint8_t effectiveValue = _z80Reset ? 0x01 : 0x00;
		TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x86);
		return effectiveValue;
	}
	if (effectiveAddr >= 0xA00000 && effectiveAddr <= 0xA0FFFF) {
		if (IsYm2612Address(effectiveAddr)) {
			uint8_t effectiveValue = 0x00;
			_openBus = effectiveValue;
			TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x20);
			return effectiveValue;
		}

		uint8_t effectiveValue = 0xFF;
		if (IsZ80BusGranted()) {
			effectiveValue = _z80Ram[effectiveAddr & 0x1FFF];
		}
		_openBus = effectiveValue;
		TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x20);
		return effectiveValue;
	}
	if (effectiveAddr >= 0xC00000 && effectiveAddr <= 0xC0001F) {
		if (IsTmssVdpLockEnforced()) {
			uint8_t effectiveValue = _openBus;
			_openBus = effectiveValue;
			TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x30);
			return effectiveValue;
		}
		uint8_t effectiveValue = _openBus;
		if (_vdp) {
			uint16_t effectiveWord = ReadVdpPort(effectiveAddr);
			if (effectiveAddr & 1) {
				effectiveValue = (uint8_t)(effectiveWord & 0xFF);
			} else {
				effectiveValue = (uint8_t)(effectiveWord >> 8);
			}
		}
		_openBus = effectiveValue;
		TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x30);
		return effectiveValue;
	}

	uint8_t* bridgeSlot = nullptr;
	uint32_t bridgeIndex = 0;
	if (TryGetSegaCdBridgeSlot(effectiveAddr, bridgeSlot, bridgeIndex)) {
		if (IsBridgeControlReadbackAddress(effectiveAddr)) {
			uint8_t effectiveValue = bridgeSlot[bridgeIndex];
			_openBus = effectiveValue;
			TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x02);
			return effectiveValue;
		}
		if (IsSegaCdSubCpuControlAddress(effectiveAddr)) {
			uint8_t effectiveValue = GetSegaCdSubCpuStatusByte();
			_openBus = effectiveValue;
			TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x02);
			return effectiveValue;
		}
		if (IsSegaCdAudioStatusAddress(effectiveAddr)) {
			uint8_t effectiveValue = GetSegaCdAudioStatusByte(effectiveAddr);
			_openBus = effectiveValue;
			TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x02);
			return effectiveValue;
		}
		if (IsSegaCdToolingStatusAddress(effectiveAddr)) {
			uint8_t effectiveValue = GetSegaCdToolingStatusByte(effectiveAddr);
			_openBus = effectiveValue;
			TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x02);
			return effectiveValue;
		}
		if (Is32xSh2StatusAddress(effectiveAddr)) {
			uint8_t effectiveValue = Get32xSh2StatusByte(effectiveAddr);
			_openBus = effectiveValue;
			TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x02);
			return effectiveValue;
		}
		if (Is32xCompositionStatusAddress(effectiveAddr)) {
			uint8_t effectiveValue = Get32xCompositionStatusByte(effectiveAddr);
			_openBus = effectiveValue;
			TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x02);
			return effectiveValue;
		}
		if (Is32xToolingStatusAddress(effectiveAddr)) {
			uint8_t effectiveValue = Get32xToolingStatusByte(effectiveAddr);
			_openBus = effectiveValue;
			TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x02);
			return effectiveValue;
		}
		if (Is32xCoprocStatusAddress(effectiveAddr)) {
			uint8_t effectiveValue = Get32xCoprocStatusByte(effectiveAddr);
			_openBus = effectiveValue;
			TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x02);
			return effectiveValue;
		}
		if (Is32xHostToolingStatusAddress(effectiveAddr)) {
			uint8_t effectiveValue = Get32xHostToolingStatusByte(effectiveAddr);
			_openBus = effectiveValue;
			TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x02);
			return effectiveValue;
		}
		uint8_t effectiveValue = 0x00;
		if (IsLegacyBridgePassThroughAddress(effectiveAddr)) {
			effectiveValue = bridgeSlot[bridgeIndex];
		}
		_openBus = effectiveValue;
		TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x02);
		return effectiveValue;
	}
	uint8_t effectiveValue = _openBus;
	_openBus = effectiveValue;
	TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x00);
	return effectiveValue;
}

void GenesisMemoryManager::DebugWrite8(uint32_t addr, uint8_t value) {
	addr &= 0xFFFFFF;
	AdvanceZ80BusArbitration(7);
	UpdateZ80RuntimeState(false, addr, _cpu ? (_cpu->GetState().PC & 0x00ffffff) : 0xffffffff, "debugwrite8-pre");
	uint32_t effectiveAddr = addr;
	if (effectiveAddr == 0xA11000 || effectiveAddr == 0xA11001) {
		TrackDebugTranscriptEntry(effectiveAddr, true, _openBus, 0x12);
		return;
	}
	if (effectiveAddr >= 0xA13000 && effectiveAddr <= 0xA130FF) {
		if (TryWriteRomBankRegister(effectiveAddr, value)) {
			uint8_t effectiveValue = (uint8_t)(value & 0x3F);
			_openBus = effectiveValue;
			TrackDebugTranscriptEntry(effectiveAddr, true, effectiveValue, 0x11);
			return;
		}
		if (IsRamControlRegister(effectiveAddr)) {
			uint8_t effectiveValue = value;
			WriteRamControlRegister(effectiveValue);
			_openBus = effectiveValue;
			TrackDebugTranscriptEntry(effectiveAddr, true, effectiveValue, 0x11);
			return;
		}
		TrackDebugTranscriptEntry(effectiveAddr, true, _openBus, 0x11);
		return;
	}
	if ((effectiveAddr & 0xFFFFFE) == 0xA14100) {
		TrackDebugTranscriptEntry(effectiveAddr, true, _openBus, 0x02);
		return;
	}
	if (IsTmssCartAddress(effectiveAddr)) {
		TrackDebugTranscriptEntry(effectiveAddr, true, _openBus, 0x02);
		return;
	}
	if (IsTmssAddress(effectiveAddr)) {
		uint8_t effectiveValue = value;
		uint32_t slot = effectiveAddr & 0x03;
		_segaCdBridgeA140[slot] = effectiveValue;
		_openBus = effectiveValue;
		EvaluateTmssUnlockState(false, effectiveAddr, effectiveValue, true);
		TrackDebugTranscriptEntry(effectiveAddr, true, effectiveValue, 0x02);
		return;
	}
	if (effectiveAddr < 0x400000) {
		uint8_t effectiveValue = value;
		_openBus = effectiveValue;
		TrackDebugTranscriptEntry(effectiveAddr, true, effectiveValue, 0x01);
		return;
	}
	if (effectiveAddr >= 0xE00000) {
		uint8_t effectiveValue = value;
		_workRam[effectiveAddr & 0xFFFF] = effectiveValue;
		_openBus = effectiveValue;
		TrackDebugTranscriptEntry(effectiveAddr, true, effectiveValue, 0x04);
		return;
	}
	if (effectiveAddr >= 0xA10000 && effectiveAddr <= 0xA1001F) {
		if ((effectiveAddr & 0x01) == 0) {
			TrackDebugTranscriptEntry(effectiveAddr, true, _openBus, 0x10);
			return;
		}

		uint32_t reg = effectiveAddr & 0x1F;
		switch (reg) {
			case 0x03:
				{
					uint8_t ioWriteValue = value;
				if (_controlManager) {
					_controlManager->WriteDataPort(0, ioWriteValue);
					_ioState.DataPort[0] = _controlManager->GetDataPortWriteLatch(0);
					_ioState.ThState[0] = _controlManager->GetThState(0);
					_ioState.ThCount[0] = _controlManager->GetThCount(0);
				} else {
					_ioState.DataPort[0] = ioWriteValue;
					_ioState.ThState[0] = 0;
					_ioState.ThCount[0] = 0;
				}
				{
					uint8_t effectiveValue = ioWriteValue;
					_openBus = effectiveValue;
					TrackDebugTranscriptEntry(effectiveAddr, true, effectiveValue, 0x10);
				}
				return;
				}
			case 0x05:
				{
					uint8_t ioWriteValue = value;
				if (_controlManager) {
					_controlManager->WriteDataPort(1, ioWriteValue);
					_ioState.DataPort[1] = _controlManager->GetDataPortWriteLatch(1);
					_ioState.ThState[1] = _controlManager->GetThState(1);
					_ioState.ThCount[1] = _controlManager->GetThCount(1);
				} else {
					_ioState.DataPort[1] = ioWriteValue;
					_ioState.ThState[1] = 0;
					_ioState.ThCount[1] = 0;
				}
				{
					uint8_t effectiveValue = ioWriteValue;
					_openBus = effectiveValue;
					TrackDebugTranscriptEntry(effectiveAddr, true, effectiveValue, 0x10);
				}
				return;
				}
			case 0x09:
				{
					uint8_t effectiveValue = value;
					uint8_t controlValue = effectiveValue;
					_ioState.CtrlPort[0] = controlValue;
					_openBus = effectiveValue;
					TrackDebugTranscriptEntry(effectiveAddr, true, effectiveValue, 0x10);
				}
				return;
			case 0x0B:
				{
					uint8_t effectiveValue = value;
					uint8_t controlValue = effectiveValue;
					_ioState.CtrlPort[1] = controlValue;
					_openBus = effectiveValue;
					TrackDebugTranscriptEntry(effectiveAddr, true, effectiveValue, 0x10);
				}
				return;
			case 0x0D:
				{
					uint8_t effectiveValue = value;
					uint8_t controlValue = effectiveValue;
					_ioState.CtrlPort[2] = controlValue;
					_openBus = effectiveValue;
					TrackDebugTranscriptEntry(effectiveAddr, true, effectiveValue, 0x10);
				}
				return;
			default:
				{
					uint8_t ioWriteValue = value;
					_openBus = ioWriteValue;
					TrackDebugTranscriptEntry(effectiveAddr, true, ioWriteValue, 0x10);
				}
				return;
		}
	}
	if (TryWriteRomBankRegister(effectiveAddr, value)) {
		uint8_t effectiveValue = (uint8_t)(value & 0x3F);
		_openBus = effectiveValue;
		TrackDebugTranscriptEntry(effectiveAddr, true, effectiveValue, 0x11);
		return;
	}
	if (IsRamControlRegister(effectiveAddr)) {
		uint8_t effectiveValue = value;
		WriteRamControlRegister(effectiveValue);
		_openBus = effectiveValue;
		TrackDebugTranscriptEntry(effectiveAddr, true, effectiveValue, 0x11);
		return;
	}
	if (IsZ80BusReqAddress(effectiveAddr)) {
		uint8_t effectiveValue = value;
		if (!_z80LatchOnlyHighByteWrites || !(effectiveAddr & 0x01)) {
			bool request = (effectiveValue & 0x01) != 0;
			uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffff) : 0xffffffff;
			SetZ80BusRequest(request, false, effectiveAddr, pc, "debugwrite8-busreq");
		}
		TrackDebugTranscriptEntry(effectiveAddr, true, effectiveValue, 0x80);
		return;
	}
	if (IsZ80ResetAddress(effectiveAddr)) {
		uint8_t effectiveValue = value;
		if (!_z80LatchOnlyHighByteWrites || !(effectiveAddr & 0x01)) {
			bool resetAsserted = !(effectiveValue & 0x01);
			uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffff) : 0xffffffff;
			SetZ80Reset(resetAsserted, false, effectiveAddr, pc, "debugwrite8-reset");
		}
		TrackDebugTranscriptEntry(effectiveAddr, true, effectiveValue, 0x84);
		return;
	}
	if (effectiveAddr >= 0xA00000 && effectiveAddr <= 0xA0FFFF) {
		uint8_t effectiveValue = value;
		if (IsZ80BusGranted()) {
			_z80Ram[effectiveAddr & 0x1FFF] = effectiveValue;
		}
		_openBus = effectiveValue;
		TrackDebugTranscriptEntry(effectiveAddr, true, effectiveValue, 0x20);
		return;
	}
	if (effectiveAddr >= 0xC00000 && effectiveAddr <= 0xC0001F) {
		uint8_t effectiveValue = value;
		if (IsTmssVdpLockEnforced()) {
			_openBus = effectiveValue;
			TrackDebugTranscriptEntry(effectiveAddr, true, effectiveValue, 0x30);
			return;
		}
		if (_vdp) {
			uint32_t port = effectiveAddr & 0x1F;
			if (port < 0x04) {
				_vdp->WriteDataPortByte(effectiveValue, (effectiveAddr & 1u) == 0u);
			} else if (port < 0x08) {
				_vdp->WriteControlPortByte(effectiveValue, (effectiveAddr & 1u) == 0u);
			} else {
				WriteVdpPort(effectiveAddr, (uint16_t)effectiveValue | ((uint16_t)effectiveValue << 8));
			}
			_openBus = effectiveValue;
		}
		TrackDebugTranscriptEntry(effectiveAddr, true, effectiveValue, 0x30);
		return;
	}

	uint8_t* bridgeSlot = nullptr;
	uint32_t bridgeIndex = 0;
	if (TryGetSegaCdBridgeSlot(effectiveAddr, bridgeSlot, bridgeIndex)) {
		if (!IsBridgeModeledWriteAddress(effectiveAddr)) {
			TrackDebugTranscriptEntry(effectiveAddr, true, _openBus, 0x02);
			return;
		}

		uint8_t effectiveValue = value;
		if (Is32xCoprocControlAddress(effectiveAddr)) {
			effectiveValue = Normalize32xCoprocControlValue(effectiveAddr, effectiveValue);
		} else if (Is32xHostToolingControlAddress(effectiveAddr)) {
			effectiveValue = Normalize32xHostControlValue(effectiveAddr, effectiveValue);
		}
		bridgeSlot[bridgeIndex] = effectiveValue;
		_openBus = effectiveValue;
		if (IsSegaCdSubCpuControlAddress(effectiveAddr)) {
			UpdateSegaCdSubCpuControl(effectiveValue);
		} else if (IsSegaCdAudioDataAddress(effectiveAddr)) {
			UpdateSegaCdAudioPath(effectiveAddr, effectiveValue);
		} else if (IsSegaCdToolingControlAddress(effectiveAddr)) {
			UpdateSegaCdToolingContract(effectiveAddr, effectiveValue);
		} else if (Is32xSh2ControlAddress(effectiveAddr)) {
			Update32xSh2Staging(effectiveAddr, effectiveValue);
		} else if (Is32xCompositionControlAddress(effectiveAddr)) {
			Update32xCompositionStaging(effectiveAddr, effectiveValue);
		} else if (Is32xToolingControlAddress(effectiveAddr)) {
			Update32xToolingContract(effectiveAddr, effectiveValue);
		} else if (Is32xCoprocControlAddress(effectiveAddr)) {
			Update32xCoprocContract(effectiveAddr, effectiveValue);
		} else if (Is32xHostToolingControlAddress(effectiveAddr)) {
			Update32xHostToolingContract(effectiveAddr, effectiveValue);
		}
		TrackDebugTranscriptEntry(effectiveAddr, true, effectiveValue, 0x02);
		return;
	}

	uint8_t effectiveValue = value;
	_openBus = effectiveValue;
	TrackDebugTranscriptEntry(effectiveAddr, true, effectiveValue, 0x00);
}

AddressInfo GenesisMemoryManager::GetAbsoluteAddress(uint32_t addr) {
	addr &= 0xFFFFFF;
	uint32_t effectiveAddress = addr;
	AddressInfo info = {};
	if (effectiveAddress < 0x400000) {
		info.Address = TranslateRomAddress(effectiveAddress);
		info.Type = MemoryType::GenesisPrgRom;
	} else if (effectiveAddress >= 0xE00000) {
		info.Address = effectiveAddress & 0xFFFF;
		info.Type = MemoryType::GenesisWorkRam;
	} else {
		info.Address = -1;
		info.Type = MemoryType::None;
	}
	return info;
}

int32_t GenesisMemoryManager::GetRelativeAddress(AddressInfo& absAddress) {
	int32_t relativeAddress = -1;
	if (absAddress.Type == MemoryType::GenesisPrgRom) {
		relativeAddress = absAddress.Address;
	} else if (absAddress.Type == MemoryType::GenesisWorkRam) {
		relativeAddress = 0xE00000 + absAddress.Address;
	}
	return relativeAddress;
}

void GenesisMemoryManager::Serialize(Serializer& s) {
	SVArray(_workRam, WorkRamSize);
	SVArray(_z80Ram, Z80RamSize);
	SV(_hasSram);
	SV(_sramStart);
	SV(_sramEnd);
	SV(_sramEvenBytes);
	SV(_sramOddBytes);
	if (_saveRam && _saveRamSize > 0) {
		SVArray(_saveRam, _saveRamSize);
	}
	SV(_masterClock);
	SV(_openBus);
	SV(_z80BusRequest);
	SV(_z80Reset);
	SV(_z80BusAck);
	SV(_z80BusReqDelayMclk);
	SV(_z80ResumeDelayMclk);
	SV(_romBankMapperEnabled);
	SVArray(_romBankRegisters, (uint32_t)sizeof(_romBankRegisters));
	SV(_ramEnable);
	SV(_ramWritable);
	SV(_tmssEnabled);
	SV(_tmssStrictMode);
	SV(_tmssUnlocked);
	SV(_tmssStartupBypassLogged);
	SV(_tmssUnlockPending);
	SV(_tmssUnlockDelayMclk);
	SV(_tmssCartRegister);
	SV(_startupWindowFrames);
	SV(_startupTraceSequence);
	SV(_startupTraceDigest);
	SV(_startupCheckpointIntervalFrames);
	SV(_startupCheckpointEndFrame);
	SV(_startupNextCheckpointFrame);
	SV(_startupDisplayTransitionCount);
	SV(_startupLastDisplayEnabled);
	SV(_startupHasLastDisplayState);
	SV(_startupHasLastZ80RunState);
	SV(_startupLastZ80Running);
	SV(_startupHasLastZ80BusReqState);
	SV(_startupLastZ80BusReq);
	SV(_startupHasLastZ80ResetState);
	SV(_startupLastZ80Reset);
	SV(_startupHasLastVdpRegs);
	SVArray(_startupLastVdpRegs, (uint32_t)sizeof(_startupLastVdpRegs));
	SV(_startupLastVdpStatus);
	SV(_segaCdSubCpuRunning);
	SV(_segaCdSubCpuBusRequest);
	SV(_segaCdSubCpuTransitionCount);
	SV(_segaCdPcmLeft);
	SV(_segaCdPcmRight);
	SV(_segaCdCddaLeft);
	SV(_segaCdCddaRight);
	SV(_segaCdMixedLeft);
	SV(_segaCdMixedRight);
	SV(_segaCdAudioCheckpointCount);
	SV(_segaCdToolingDebuggerSignal);
	SV(_segaCdToolingTasSignal);
	SV(_segaCdToolingSaveStateSignal);
	SV(_segaCdToolingCheatSignal);
	SV(_segaCdToolingEventCount);
	SV(_segaCdToolingDigest);
	SV(_m32xMasterSh2Running);
	SV(_m32xSlaveSh2Running);
	SV(_m32xSh2SyncPhase);
	SV(_m32xSh2Milestone);
	SV(_m32xSh2SyncEpoch);
	SV(_m32xSh2Digest);
	SV(_m32xCompositionBlend);
	SV(_m32xFrameSyncMarker);
	SV(_m32xFrameSyncEpoch);
	SV(_m32xCompositionDigest);
	SV(_m32xToolingDebuggerSignal);
	SV(_m32xToolingTasSignal);
	SV(_m32xToolingSaveStateSignal);
	SV(_m32xToolingCheatSignal);
	SV(_m32xToolingEventCount);
	SV(_m32xToolingDigest);
	SV(_m32xCoprocMasterSignal);
	SV(_m32xCoprocSlaveSignal);
	SV(_m32xCoprocPhaseSignal);
	SV(_m32xCoprocFenceSignal);
	SV(_m32xCoprocEventCount);
	SV(_m32xCoprocDigest);
	SV(_m32xCoprocEdgeCount);
	SV(_m32xCoprocPhaseEpoch);
	SV(_m32xCoprocFenceEpoch);
	SV(_m32xCoprocArbiterLatch);
	SV(_m32xHostDebuggerSignal);
	SV(_m32xHostTasSignal);
	SV(_m32xHostSaveStateSignal);
	SV(_m32xHostCheatSignal);
	SV(_m32xHostEventCount);
	SV(_m32xHostDigest);
	SV(_m32xHostEdgeCount);
	SV(_m32xHostCommandNonce);
	SV(_m32xHostAckToken);
	SV(_m32xHostDeterminismLatch);
	SVArray(_segaCdBridgeA120, (uint32_t)sizeof(_segaCdBridgeA120));
	SVArray(_segaCdBridgeA130, (uint32_t)sizeof(_segaCdBridgeA130));
	SVArray(_segaCdBridgeA140, (uint32_t)sizeof(_segaCdBridgeA140));
	SVArray(_segaCdBridgeA150, (uint32_t)sizeof(_segaCdBridgeA150));
	SVArray(_segaCdBridgeA160, (uint32_t)sizeof(_segaCdBridgeA160));
	SVArray(_segaCdBridgeA180, (uint32_t)sizeof(_segaCdBridgeA180));
	SV(_ioState.DataPort[0]); SV(_ioState.DataPort[1]); SV(_ioState.DataPort[2]);
	SV(_ioState.CtrlPort[0]); SV(_ioState.CtrlPort[1]); SV(_ioState.CtrlPort[2]);
	SV(_ioState.TmssEnabled);
	SV(_ioState.TmssUnlocked);
	SV(_ioState.CpuProgramCounterHeartbeat);
	SV(_ioState.CpuCycleHeartbeat);
	SV(_ioState.CpuInstructionHeartbeat);
	SV(_ioState.TranscriptLaneCount);
	SV(_ioState.TranscriptLaneDigest);
	for (uint32_t i = 0; i < 4; i++) {
		SV(_ioState.TranscriptEntryAddress[i]);
		SV(_ioState.TranscriptEntryValue[i]);
		SV(_ioState.TranscriptEntryFlags[i]);
	}
	SV(_ioState.DebugTranscriptLaneCount);
	SV(_ioState.DebugTranscriptLaneDigest);
	for (uint32_t i = 0; i < 4; i++) {
		SV(_ioState.DebugTranscriptEntryAddress[i]);
		SV(_ioState.DebugTranscriptEntryValue[i]);
		SV(_ioState.DebugTranscriptEntryFlags[i]);
	}
	SV(_ioState.RomReadHeartbeat);
}

void GenesisMemoryManager::LoadBattery() {
	if (HasSaveRam()) {
		BatteryManager* batteryManager = _emu->GetBatteryManager();
		batteryManager->LoadBattery(".sav", std::span<uint8_t>(_saveRam, _saveRamSize));
	}
}

void GenesisMemoryManager::SaveBattery() {
	if (HasSaveRam()) {
		BatteryManager* batteryManager = _emu->GetBatteryManager();
		batteryManager->SaveBattery(".sav", std::span<const uint8_t>(_saveRam, _saveRamSize));
	}
}

void GenesisMemoryManager::ResetRuntimeState(bool hardReset) {
	(void)hardReset;
	EnsureNexenWramTraceOpen();
	EnsureNexenStartupTraceOpen();
	if (_emu && _emu->GetSettings()) {
		_tmssEnabled = _emu->GetSettings()->GetGenesisConfig().EnableTmss;
	}
	ApplyStartupEnvironmentProfile();

	bool nextZ80BusRequest = false;
	bool nextZ80Reset = sNexenGenesisPowerOnZ80ResetAsserted;
	uint8_t nextOpenBus = 0;
	bool nextTmssUnlocked = false;
	bool tmssEnabled = _tmssEnabled;
	_z80BusRequest = nextZ80BusRequest;
	_z80Reset = nextZ80Reset;
	_z80BusAck = false;
	_z80BusReqDelayMclk = 0;
	_z80ResumeDelayMclk = 0;
	_z80RuntimeRunning = false;
	_z80RuntimeRunnableCycles = 0;
	_z80RuntimeStalledCycles = 0;
	_z80RuntimeTransitionCount = 0;
	_z80RuntimeStateEpoch = 0;
	_z80RuntimeLastTransitionClock = 0;
	UpdateZ80RuntimeState(false, 0, 0, "reset");
	_openBus = nextOpenBus;
	_tmssUnlocked = nextTmssUnlocked;
	_tmssStartupBypassLogged = false;
	_tmssUnlockPending = false;
	_tmssUnlockDelayMclk = 0;
	_tmssCartRegister = 0;
	_startupTraceSequence = 0;
	_startupTraceDigest = 1469598103934665603ull;
	_startupDisplayTransitionCount = 0;
	_startupNextCheckpointFrame = 0;
	_startupHasLastDisplayState = false;
	_startupHasLastZ80RunState = false;
	_startupLastZ80Running = false;
	_startupHasLastZ80BusReqState = false;
	_startupLastZ80BusReq = false;
	_startupHasLastZ80ResetState = false;
	_startupLastZ80Reset = false;
	_startupHasLastVdpRegs = false;
	memset(_startupLastVdpRegs, 0, sizeof(_startupLastVdpRegs));
	_startupLastVdpStatus = 0;
	_startupLastDisplayEnabled = _vdp ? ((_vdp->GetState().Registers[VdpReg::ModeSet2] & 0x40) != 0) : false;
	_ramEnable = false;
	_ramWritable = true;
	_tmssVdpBlockLogged = false;

	if (tmssEnabled) {
		memset(_segaCdBridgeA140, 0, sizeof(_segaCdBridgeA140));
	}

	ResetRomBankMapper();

	memset(_ioState.DataPort, 0, sizeof(_ioState.DataPort));
	memset(_ioState.CtrlPort, 0, sizeof(_ioState.CtrlPort));
	memset(_ioState.TxData, 0, sizeof(_ioState.TxData));
	memset(_ioState.RxData, 0, sizeof(_ioState.RxData));
	memset(_ioState.SCtrl, 0, sizeof(_ioState.SCtrl));
	memset(_ioState.ThCount, 0, sizeof(_ioState.ThCount));
	memset(_ioState.ThState, 0, sizeof(_ioState.ThState));
	_ioState.CpuProgramCounterHeartbeat = 0;
	_ioState.CpuCycleHeartbeat = 0;
	_ioState.CpuInstructionHeartbeat = 0;
	uint32_t resetTranscriptLaneCount = 0;
	uint64_t resetTranscriptLaneDigest = 0;
	_ioState.TranscriptLaneCount = resetTranscriptLaneCount;
	_ioState.TranscriptLaneDigest = resetTranscriptLaneDigest;
	_ioState.RomReadHeartbeat = 0;
	for (uint32_t i = 0; i < 4; i++) {
		_ioState.TranscriptEntryAddress[i] = 0;
		_ioState.TranscriptEntryValue[i] = 0;
		_ioState.TranscriptEntryFlags[i] = 0;
	}

	uint8_t tmssEnabledValue = tmssEnabled ? 1 : 0;
	uint8_t tmssUnlockedValue = nextTmssUnlocked ? 1 : 0;
	_ioState.TmssEnabled = tmssEnabledValue;
	_ioState.TmssUnlocked = tmssUnlockedValue;

	_segaCdToolingDebuggerSignal = 0;
	_segaCdToolingTasSignal = 0;
	_segaCdToolingSaveStateSignal = 0;
	_segaCdToolingCheatSignal = 0;
	_segaCdToolingEventCount = 0;
	_segaCdToolingDigest = 0;

	_m32xToolingDebuggerSignal = 0;
	_m32xToolingTasSignal = 0;
	_m32xToolingSaveStateSignal = 0;
	_m32xToolingCheatSignal = 0;
	_m32xToolingEventCount = 0;
	_m32xToolingDigest = 0;
	_m32xCoprocMasterSignal = 0;
	_m32xCoprocSlaveSignal = 0;
	_m32xCoprocPhaseSignal = 0;
	_m32xCoprocFenceSignal = 0;
	_m32xCoprocEventCount = 0;
	_m32xCoprocDigest = 0;
	_m32xCoprocEdgeCount = 0;
	_m32xCoprocPhaseEpoch = 0;
	_m32xCoprocFenceEpoch = 0;
	_m32xCoprocArbiterLatch = 0;
	_m32xHostDebuggerSignal = 0;
	_m32xHostTasSignal = 0;
	_m32xHostSaveStateSignal = 0;
	_m32xHostCheatSignal = 0;
	_m32xHostEventCount = 0;
	_m32xHostDigest = 0;
	_m32xHostEdgeCount = 0;
	_m32xHostCommandNonce = 0;
	_m32xHostAckToken = 0;
	_m32xHostDeterminismLatch = 0;

	ClearDebugTranscriptLane();
}
