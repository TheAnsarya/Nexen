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
		if (raw && (*raw == '1' || *raw == 'y' || *raw == 'Y' || *raw == 't' || *raw == 'T' || *raw == 'o' || *raw == 'O')) {
			cached = 1;
		}
		return cached == 1;
	}

	bool ParseEnvEnabled(const char* key, bool defaultValue) {
		const char* raw = std::getenv(key);
		if (!raw || !*raw) {
			return defaultValue;
		}

		return *raw == '1' || *raw == 'y' || *raw == 'Y' || *raw == 't' || *raw == 'T' || *raw == 'o' || *raw == 'O';
	}

	uint32_t ParseEnvUInt32Clamped(const char* key, uint32_t defaultValue, uint32_t minValue, uint32_t maxValue) {
		char* end = nullptr;
		const char* raw = std::getenv(key);
		if (!raw || !*raw) {
			return defaultValue;
		}

		unsigned long parsed = std::strtoul(raw, &end, 0);
		if (end == raw || *end != '\0') {
			return defaultValue;
		}

		if (parsed < minValue) {
			parsed = minValue;
		} else if (parsed > maxValue) {
			parsed = maxValue;
		}
		return (uint32_t)parsed;
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
		return ParseEnvEnabled("NEXEN_GENESIS_DISABLE_AUTO_TRACE", false);
	}

	uint32_t GetGenesisSonicStartupCheckpointIntervalFrames() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_SONIC_CHECKPOINT_INTERVAL_FRAMES", 30, 1, 300);
		return cached;
	}

	uint32_t GetGenesisSonicStartupCheckpointEndFrame() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_SONIC_CHECKPOINT_END_FRAME", 900, 60, 3600);
		return cached;
	}

	bool IsGenesisSonicStartupCheckpointEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_SONIC_CHECKPOINT_ENABLE", true);
	}

	uint32_t GetGenesisSonicTraceRearmFrameWindow() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_SONIC_TRACE_REARM_FRAME_WINDOW", 180, 30, 1800);
		return cached;
	}

	uint32_t GetGenesisSonicTraceCpuCycles() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_SONIC_TRACE_CPU_CYCLES", 260000, 60000, 2000000);
		return cached;
	}

	uint32_t GetGenesisSonicTraceMmuCycles() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_SONIC_TRACE_MMU_CYCLES", 340000, 60000, 3000000);
		return cached;
	}

	uint32_t GetGenesisSonicTraceCpuStride() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_SONIC_TRACE_CPU_STRIDE", 1, 1, 16);
		return cached;
	}

	uint32_t GetGenesisSonicTraceMmuStride() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_SONIC_TRACE_MMU_STRIDE", 1, 1, 16);
		return cached;
	}

	uint32_t GetGenesisSonicTraceCpuRing() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_SONIC_TRACE_CPU_RING", 320, 32, 4096);
		return cached;
	}

	uint32_t GetGenesisSonicTraceMmuFlowRing() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_SONIC_TRACE_MMU_FLOW_RING", 256, 32, 4096);
		return cached;
	}

	uint32_t GetGenesisSonicTraceMmuOpRing() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_SONIC_TRACE_MMU_OP_RING", 320, 32, 4096);
		return cached;
	}

	bool IsGenesisSonicCheckpointIncludeOpWindowEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_SONIC_CHECKPOINT_INCLUDE_OP_WINDOW", false);
	}

	uint32_t GetGenesisSonicCheckpointOpWindowLines() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_SONIC_CHECKPOINT_OP_WINDOW_LINES", 6, 1, 32);
		return cached;
	}

	bool IsGenesisSonicCheckpointIncludeCpuTraceEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_SONIC_CHECKPOINT_INCLUDE_CPU_TRACE", false);
	}

	uint32_t GetGenesisSonicCheckpointCpuTraceLines() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_SONIC_CHECKPOINT_CPU_TRACE_LINES", 6, 1, 32);
		return cached;
	}

	uint32_t GetGenesisRunFrameStagnantIterationThreshold() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_STAGNANT_ITERS", 250000, 1000, 5000000);
		return cached;
	}

	uint32_t GetGenesisRunFrameForcedAdvancePulseLimit() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_FORCED_PULSE_LIMIT", 8, 1, 256);
		return cached;
	}

	uint32_t GetGenesisRunFrameForcedAdvancePulseCycles() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_FORCED_PULSE_CYCLES", 488, 32, 4096);
		return cached;
	}

	uint32_t GetGenesisRunFrameFallbackPulseIterations() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_FALLBACK_PULSES", 320, 1, 4096);
		return cached;
	}

	uint32_t GetGenesisRunFrameHardGuardCpuTraceLines() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_HARD_GUARD_CPU_TRACE_LINES", 10, 1, 64);
		return cached;
	}

	uint32_t GetGenesisRunFrameHardGuardMmuOpWindowLines() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_HARD_GUARD_MMU_OP_LINES", 10, 1, 64);
		return cached;
	}

	uint32_t GetGenesisRunFrameForcedCompletionCpuTraceLines() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_FORCED_COMPLETION_CPU_TRACE_LINES", 8, 1, 64);
		return cached;
	}

	uint32_t GetGenesisRunFrameForcedCompletionMmuOpWindowLines() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_FORCED_COMPLETION_MMU_OP_LINES", 8, 1, 64);
		return cached;
	}

	bool IsGenesisRunFrameWaitLogEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_WAIT_LOG_ENABLE", true);
	}

	uint32_t GetGenesisRunFrameWaitLogIntervalGuard() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_WAIT_LOG_INTERVAL", 50000, 1000, 1000000);
		return cached;
	}

	bool IsGenesisRunFrameWaitIncludeMmuOpWindowEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_WAIT_INCLUDE_MMU_OP_WINDOW", false);
	}

	uint32_t GetGenesisRunFrameWaitMmuOpWindowLines() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_WAIT_MMU_OP_LINES", 4, 1, 64);
		return cached;
	}

	bool IsGenesisRunFrameWaitIncludeCpuTraceEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_WAIT_INCLUDE_CPU_TRACE", false);
	}

	uint32_t GetGenesisRunFrameWaitCpuTraceLines() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_WAIT_CPU_TRACE_LINES", 4, 1, 64);
		return cached;
	}

	bool IsGenesisRunFrameFrameAdvanceLogEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FRAME_ADVANCE_LOG_ENABLE", true);
	}

	uint32_t GetGenesisRunFrameFrameAdvanceLogModulo() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_FRAME_ADVANCE_LOG_MODULO", 120, 1, 3600);
		return cached;
	}

	bool IsGenesisRunFrameFrameAdvanceIncludeCpuTraceEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FRAME_ADVANCE_INCLUDE_CPU_TRACE", false);
	}

	uint32_t GetGenesisRunFrameFrameAdvanceCpuTraceLines() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_FRAME_ADVANCE_CPU_TRACE_LINES", 4, 1, 64);
		return cached;
	}

	bool IsGenesisRunFrameFrameAdvanceIncludeMmuOpWindowEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FRAME_ADVANCE_INCLUDE_MMU_OP_WINDOW", false);
	}

	bool IsGenesisRunFrameFrameAdvanceIncludeCpuLoopEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FRAME_ADVANCE_INCLUDE_CPU_LOOP", false);
	}

	bool IsGenesisRunFrameFrameAdvanceIncludeCpuAddrEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FRAME_ADVANCE_INCLUDE_CPU_ADDR", false);
	}

	bool IsGenesisRunFrameFrameAdvanceIncludeHeartbeatEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FRAME_ADVANCE_INCLUDE_HEARTBEAT", false);
	}

	bool IsGenesisRunFrameFrameAdvanceIncludeZ80RuntimeEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FRAME_ADVANCE_INCLUDE_Z80_RUNTIME", false);
	}

	bool IsGenesisRunFrameFrameAdvanceIncludeVdpStateEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FRAME_ADVANCE_INCLUDE_VDP_STATE", false);
	}

	uint32_t GetGenesisRunFrameFrameAdvanceMmuOpWindowLines() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_FRAME_ADVANCE_MMU_OP_LINES", 4, 1, 64);
		return cached;
	}

	uint32_t GetGenesisRunFrameSehMmuOpWindowLines() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_SEH_MMU_OP_LINES", 12, 1, 64);
		return cached;
	}

	bool IsGenesisRunFrameSehIncludeCpuTraceEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_SEH_INCLUDE_CPU_TRACE", false);
	}

	bool IsGenesisRunFrameSehIncludeCpuBoundaryEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_SEH_INCLUDE_CPU_BOUNDARY", true);
	}

	bool IsGenesisRunFrameSehIncludeCpuLoopEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_SEH_INCLUDE_CPU_LOOP", false);
	}

	bool IsGenesisRunFrameSehIncludeCpuAddrEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_SEH_INCLUDE_CPU_ADDR", false);
	}

	bool IsGenesisRunFrameSehIncludeStartupEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_SEH_INCLUDE_STARTUP", false);
	}

	bool IsGenesisRunFrameSehIncludeMmuFlowEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_SEH_INCLUDE_MMU_FLOW", true);
	}

	bool IsGenesisRunFrameSehIncludeMmuOpsEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_SEH_INCLUDE_MMU_OPS", true);
	}

	bool IsGenesisRunFrameSehIncludeMmuOpWindowEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_SEH_INCLUDE_MMU_OP_WINDOW", true);
	}

	uint32_t GetGenesisRunFrameSehCpuTraceLines() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_SEH_CPU_TRACE_LINES", 6, 1, 64);
		return cached;
	}

	uint32_t GetGenesisRunFrameFirstFailureMmuOpWindowLines() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_FIRST_FAILURE_MMU_OP_LINES", 8, 1, 64);
		return cached;
	}

	bool IsGenesisRunFrameStallRecoveryIncludeMmuOpWindowEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_STALL_INCLUDE_MMU_OP_WINDOW", false);
	}

	uint32_t GetGenesisRunFrameStallRecoveryMmuOpWindowLines() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_STALL_MMU_OP_LINES", 4, 1, 64);
		return cached;
	}

	bool IsGenesisRunFrameStallRecoveryIncludeCpuTraceEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_STALL_INCLUDE_CPU_TRACE", false);
	}

	uint32_t GetGenesisRunFrameStallRecoveryCpuTraceLines() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_STALL_CPU_TRACE_LINES", 4, 1, 64);
		return cached;
	}

	bool IsGenesisRunFrameHardGuardIncludeCpuBoundaryEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_HARD_GUARD_INCLUDE_CPU_BOUNDARY", true);
	}

	bool IsGenesisRunFrameHardGuardIncludeCpuTraceEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_HARD_GUARD_INCLUDE_CPU_TRACE", true);
	}

	bool IsGenesisRunFrameHardGuardIncludeCpuLoopEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_HARD_GUARD_INCLUDE_CPU_LOOP", true);
	}

	bool IsGenesisRunFrameHardGuardIncludeCpuAddrEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_HARD_GUARD_INCLUDE_CPU_ADDR", true);
	}

	bool IsGenesisRunFrameHardGuardIncludeStartupEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_HARD_GUARD_INCLUDE_STARTUP", true);
	}

	bool IsGenesisRunFrameHardGuardIncludeMmuFlowEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_HARD_GUARD_INCLUDE_MMU_FLOW", true);
	}

	bool IsGenesisRunFrameHardGuardIncludeMmuOpsEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_HARD_GUARD_INCLUDE_MMU_OPS", true);
	}

	bool IsGenesisRunFrameHardGuardIncludeMmuOpWindowEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_HARD_GUARD_INCLUDE_MMU_OP_WINDOW", true);
	}

	bool IsGenesisRunFrameHardGuardIncludeHeartbeatEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_HARD_GUARD_INCLUDE_HEARTBEAT", false);
	}

	bool IsGenesisRunFrameHardGuardIncludeZ80RuntimeEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_HARD_GUARD_INCLUDE_Z80_RUNTIME", false);
	}

	bool IsGenesisRunFrameHardGuardIncludeVdpStateEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_HARD_GUARD_INCLUDE_VDP_STATE", false);
	}

	bool IsGenesisRunFrameHardGuardIncludeTraceDigestEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_HARD_GUARD_INCLUDE_TRACE_DIGEST", false);
	}

	bool IsGenesisRunFrameWaitIncludeHeartbeatEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_WAIT_INCLUDE_HEARTBEAT", true);
	}

	bool IsGenesisRunFrameWaitIncludeZ80RuntimeEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_WAIT_INCLUDE_Z80_RUNTIME", true);
	}

	bool IsGenesisRunFrameWaitIncludeVdpStateEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_WAIT_INCLUDE_VDP_STATE", true);
	}

	bool IsGenesisRunFrameWaitIncludeCpuLoopEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_WAIT_INCLUDE_CPU_LOOP", true);
	}

	bool IsGenesisRunFrameWaitIncludeCpuAddrEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_WAIT_INCLUDE_CPU_ADDR", true);
	}

	bool IsGenesisRunFrameWaitIncludeStartupEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_WAIT_INCLUDE_STARTUP", false);
	}

	bool IsGenesisRunFrameWaitIncludeMmuFlowEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_WAIT_INCLUDE_MMU_FLOW", false);
	}

	bool IsGenesisRunFrameWaitIncludeMmuOpsEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_WAIT_INCLUDE_MMU_OPS", false);
	}

	bool IsGenesisRunFrameWaitIncludeCpuBoundaryEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_WAIT_INCLUDE_CPU_BOUNDARY", false);
	}

	bool IsGenesisRunFrameWaitIncludeTraceDigestEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_WAIT_INCLUDE_TRACE_DIGEST", false);
	}

	bool IsGenesisRunFrameForcedCompletionDetailEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FORCED_COMPLETION_DETAIL_ENABLE", true);
	}

	bool IsGenesisRunFrameForcedCompletionIncludeCpuTraceEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FORCED_COMPLETION_INCLUDE_CPU_TRACE", true);
	}

	bool IsGenesisRunFrameForcedCompletionIncludeCpuLoopEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FORCED_COMPLETION_INCLUDE_CPU_LOOP", true);
	}

	bool IsGenesisRunFrameForcedCompletionIncludeStartupEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FORCED_COMPLETION_INCLUDE_STARTUP", true);
	}

	bool IsGenesisRunFrameForcedCompletionIncludeMmuFlowEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FORCED_COMPLETION_INCLUDE_MMU_FLOW", false);
	}

	bool IsGenesisRunFrameForcedCompletionIncludeMmuOpsEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FORCED_COMPLETION_INCLUDE_MMU_OPS", false);
	}

	bool IsGenesisRunFrameForcedCompletionIncludeMmuOpWindowEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FORCED_COMPLETION_INCLUDE_MMU_OP_WINDOW", true);
	}

	bool IsGenesisRunFrameForcedCompletionIncludeCpuBoundaryEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FORCED_COMPLETION_INCLUDE_CPU_BOUNDARY", false);
	}

	bool IsGenesisRunFrameForcedCompletionIncludeCpuAddrEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FORCED_COMPLETION_INCLUDE_CPU_ADDR", false);
	}

	bool IsGenesisRunFrameForcedCompletionIncludeHeartbeatEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FORCED_COMPLETION_INCLUDE_HEARTBEAT", false);
	}

	bool IsGenesisRunFrameForcedCompletionIncludeZ80RuntimeEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FORCED_COMPLETION_INCLUDE_Z80_RUNTIME", false);
	}

	bool IsGenesisRunFrameForcedCompletionIncludeVdpStateEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FORCED_COMPLETION_INCLUDE_VDP_STATE", false);
	}

	bool IsGenesisRunFrameForcedCompletionIncludeTraceDigestDetailEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FORCED_COMPLETION_INCLUDE_TRACE_DIGEST_DETAIL", false);
	}

	bool IsGenesisRunFrameForcedCompletionIncludeTraceDigestEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FORCED_COMPLETION_INCLUDE_TRACE_DIGEST", true);
	}

	bool IsGenesisRunFrameFirstFailureIncludeCpuTraceEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FIRST_FAILURE_INCLUDE_CPU_TRACE", false);
	}

	uint32_t GetGenesisRunFrameFirstFailureCpuTraceLines() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_FIRST_FAILURE_CPU_TRACE_LINES", 4, 1, 64);
		return cached;
	}

	bool IsGenesisRunFrameFirstFailureIncludeStartupEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FIRST_FAILURE_INCLUDE_STARTUP", false);
	}

	bool IsGenesisRunFrameFirstFailureIncludeCpuBoundaryEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FIRST_FAILURE_INCLUDE_CPU_BOUNDARY", true);
	}

	bool IsGenesisRunFrameFirstFailureIncludeMmuFlowEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FIRST_FAILURE_INCLUDE_MMU_FLOW", true);
	}

	bool IsGenesisRunFrameFirstFailureIncludeMmuOpsEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FIRST_FAILURE_INCLUDE_MMU_OPS", true);
	}

	bool IsGenesisRunFrameFirstFailureIncludeHeartbeatEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FIRST_FAILURE_INCLUDE_HEARTBEAT", false);
	}

	bool IsGenesisRunFrameFirstFailureIncludeZ80RuntimeEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FIRST_FAILURE_INCLUDE_Z80_RUNTIME", false);
	}

	bool IsGenesisRunFrameFirstFailureIncludeVdpStateEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FIRST_FAILURE_INCLUDE_VDP_STATE", false);
	}

	bool IsGenesisRunFrameFirstFailureIncludeTraceDigestExtendedEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FIRST_FAILURE_INCLUDE_TRACE_DIGEST_EXTENDED", false);
	}

	bool IsGenesisRunFrameStallRecoveryIncludeStartupEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_STALL_INCLUDE_STARTUP", false);
	}

	bool IsGenesisRunFrameStallRecoveryIncludeCpuBoundaryEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_STALL_INCLUDE_CPU_BOUNDARY", false);
	}

	bool IsGenesisRunFrameStallRecoveryIncludeMmuFlowEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_STALL_INCLUDE_MMU_FLOW", false);
	}

	bool IsGenesisRunFrameStallRecoveryIncludeMmuOpsEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_STALL_INCLUDE_MMU_OPS", false);
	}

	bool IsGenesisRunFrameStallRecoveryIncludeTraceDigestEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_STALL_INCLUDE_TRACE_DIGEST", false);
	}

	bool IsGenesisRunFrameStallRecoveryIncludeHeartbeatEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_STALL_INCLUDE_HEARTBEAT", false);
	}

	bool IsGenesisRunFrameStallRecoveryIncludeZ80RuntimeEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_STALL_INCLUDE_Z80_RUNTIME", false);
	}

	bool IsGenesisRunFrameStallRecoveryIncludeVdpStateEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_STALL_INCLUDE_VDP_STATE", false);
	}

	bool IsGenesisRunFrameStallRecoveryIncludeCpuAddrEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_STALL_INCLUDE_CPU_ADDR", false);
	}

	bool IsGenesisRunFrameFrameAdvanceIncludeStartupEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FRAME_ADVANCE_INCLUDE_STARTUP", false);
	}

	bool IsGenesisRunFrameFrameAdvanceIncludeMmuFlowEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FRAME_ADVANCE_INCLUDE_MMU_FLOW", false);
	}

	bool IsGenesisRunFrameFrameAdvanceIncludeMmuOpsEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FRAME_ADVANCE_INCLUDE_MMU_OPS", false);
	}

	bool IsGenesisRunFrameFrameAdvanceIncludeCpuBoundaryEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FRAME_ADVANCE_INCLUDE_CPU_BOUNDARY", false);
	}

	bool IsGenesisRunFrameFrameAdvanceIncludeTraceDigestEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FRAME_ADVANCE_INCLUDE_TRACE_DIGEST", false);
	}

	bool IsGenesisRunFrameSehIncludeHeartbeatEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_SEH_INCLUDE_HEARTBEAT", false);
	}

	bool IsGenesisRunFrameSehIncludeZ80RuntimeEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_SEH_INCLUDE_Z80_RUNTIME", false);
	}

	bool IsGenesisRunFrameSehIncludeVdpStateEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_SEH_INCLUDE_VDP_STATE", false);
	}

	bool IsGenesisRunFrameSehIncludeTraceDigestEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_SEH_INCLUDE_TRACE_DIGEST", false);
	}

	bool IsGenesisRunFrameFirstFailureIncludeCpuLoopEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FIRST_FAILURE_INCLUDE_CPU_LOOP", false);
	}

	bool IsGenesisRunFrameFirstFailureIncludeCpuAddrEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FIRST_FAILURE_INCLUDE_CPU_ADDR", false);
	}

	bool IsGenesisRunFrameFirstFailureIncludeTraceDigestEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FIRST_FAILURE_INCLUDE_TRACE_DIGEST", false);
	}

	bool IsGenesisRunFrameFirstFailureIncludeMmuOpWindowEnabled() {
		return ParseEnvEnabled("NEXEN_GENESIS_RUNFRAME_FIRST_FAILURE_INCLUDE_MMU_OP_WINDOW", true);
	}

	uint32_t GetGenesisRunFrameHardGuardTraceCpuCycles() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_HARD_GUARD_TRACE_CPU_CYCLES", 120000, 10000, 3000000);
		return cached;
	}

	uint32_t GetGenesisRunFrameHardGuardTraceCpuStride() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_HARD_GUARD_TRACE_CPU_STRIDE", 1, 1, 16);
		return cached;
	}

	uint32_t GetGenesisRunFrameHardGuardTraceCpuRing() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_HARD_GUARD_TRACE_CPU_RING", 192, 16, 4096);
		return cached;
	}

	uint32_t GetGenesisRunFrameHardGuardTraceMmuCpuCycles() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_HARD_GUARD_TRACE_MMU_CPU_CYCLES", 180000, 10000, 3000000);
		return cached;
	}

	uint32_t GetGenesisRunFrameHardGuardTraceMmuCycles() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_HARD_GUARD_TRACE_MMU_CYCLES", 260000, 10000, 4000000);
		return cached;
	}

	uint32_t GetGenesisRunFrameHardGuardTraceMmuCpuStride() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_HARD_GUARD_TRACE_MMU_CPU_STRIDE", 1, 1, 16);
		return cached;
	}

	uint32_t GetGenesisRunFrameHardGuardTraceMmuStride() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_HARD_GUARD_TRACE_MMU_STRIDE", 1, 1, 16);
		return cached;
	}

	uint32_t GetGenesisRunFrameHardGuardTraceMmuFlowRing() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_HARD_GUARD_TRACE_MMU_FLOW_RING", 192, 16, 4096);
		return cached;
	}

	uint32_t GetGenesisRunFrameHardGuardTraceMmuOpRing() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_HARD_GUARD_TRACE_MMU_OP_RING", 256, 16, 4096);
		return cached;
	}

	uint32_t GetGenesisRunFrameStallRecoveryTraceCpuCycles() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_STALL_TRACE_CPU_CYCLES", 90000, 10000, 3000000);
		return cached;
	}

	uint32_t GetGenesisRunFrameStallRecoveryTraceCpuStride() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_STALL_TRACE_CPU_STRIDE", 1, 1, 16);
		return cached;
	}

	uint32_t GetGenesisRunFrameStallRecoveryTraceCpuRing() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_STALL_TRACE_CPU_RING", 160, 16, 4096);
		return cached;
	}

	uint32_t GetGenesisRunFrameStallRecoveryTraceMmuCpuCycles() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_STALL_TRACE_MMU_CPU_CYCLES", 140000, 10000, 3000000);
		return cached;
	}

	uint32_t GetGenesisRunFrameStallRecoveryTraceMmuCycles() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_STALL_TRACE_MMU_CYCLES", 220000, 10000, 4000000);
		return cached;
	}

	uint32_t GetGenesisRunFrameStallRecoveryTraceMmuCpuStride() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_STALL_TRACE_MMU_CPU_STRIDE", 1, 1, 16);
		return cached;
	}

	uint32_t GetGenesisRunFrameStallRecoveryTraceMmuStride() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_STALL_TRACE_MMU_STRIDE", 1, 1, 16);
		return cached;
	}

	uint32_t GetGenesisRunFrameStallRecoveryTraceMmuFlowRing() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_STALL_TRACE_MMU_FLOW_RING", 160, 16, 4096);
		return cached;
	}

	uint32_t GetGenesisRunFrameStallRecoveryTraceMmuOpRing() {
		static uint32_t cached = 0;
		if (cached != 0) {
			return cached;
		}

		cached = ParseEnvUInt32Clamped("NEXEN_GENESIS_RUNFRAME_STALL_TRACE_MMU_OP_RING", 224, 16, 4096);
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
	uint32_t sonicTraceCpuCycles = GetGenesisSonicTraceCpuCycles();
	uint32_t sonicTraceMmuCycles = GetGenesisSonicTraceMmuCycles();
	uint32_t sonicTraceCpuStride = GetGenesisSonicTraceCpuStride();
	uint32_t sonicTraceMmuStride = GetGenesisSonicTraceMmuStride();
	uint32_t sonicTraceCpuRing = GetGenesisSonicTraceCpuRing();
	uint32_t sonicTraceMmuFlowRing = GetGenesisSonicTraceMmuFlowRing();
	uint32_t sonicTraceMmuOpRing = GetGenesisSonicTraceMmuOpRing();
	bool sonicCheckpointIncludeOpWindow = IsGenesisSonicCheckpointIncludeOpWindowEnabled();
	uint32_t sonicCheckpointOpWindowLines = GetGenesisSonicCheckpointOpWindowLines();
	bool sonicCheckpointIncludeCpuTrace = IsGenesisSonicCheckpointIncludeCpuTraceEnabled();
	uint32_t sonicCheckpointCpuTraceLines = GetGenesisSonicCheckpointCpuTraceLines();
	uint32_t runFrameStagnantIters = GetGenesisRunFrameStagnantIterationThreshold();
	uint32_t runFrameForcedPulseLimit = GetGenesisRunFrameForcedAdvancePulseLimit();
	uint32_t runFrameForcedPulseCycles = GetGenesisRunFrameForcedAdvancePulseCycles();
	uint32_t runFrameFallbackPulses = GetGenesisRunFrameFallbackPulseIterations();
	uint32_t runFrameHardGuardCpuTraceLines = GetGenesisRunFrameHardGuardCpuTraceLines();
	uint32_t runFrameHardGuardMmuOpLines = GetGenesisRunFrameHardGuardMmuOpWindowLines();
	uint32_t runFrameForcedCompletionCpuTraceLines = GetGenesisRunFrameForcedCompletionCpuTraceLines();
	uint32_t runFrameForcedCompletionMmuOpLines = GetGenesisRunFrameForcedCompletionMmuOpWindowLines();
	bool runFrameWaitLogEnabled = IsGenesisRunFrameWaitLogEnabled();
	uint32_t runFrameWaitLogInterval = GetGenesisRunFrameWaitLogIntervalGuard();
	bool runFrameWaitIncludeMmuOpWindow = IsGenesisRunFrameWaitIncludeMmuOpWindowEnabled();
	uint32_t runFrameWaitMmuOpLines = GetGenesisRunFrameWaitMmuOpWindowLines();
	bool runFrameWaitIncludeCpuTrace = IsGenesisRunFrameWaitIncludeCpuTraceEnabled();
	uint32_t runFrameWaitCpuTraceLines = GetGenesisRunFrameWaitCpuTraceLines();
	uint32_t runFrameHardGuardTraceCpuCycles = GetGenesisRunFrameHardGuardTraceCpuCycles();
	uint32_t runFrameHardGuardTraceCpuStride = GetGenesisRunFrameHardGuardTraceCpuStride();
	uint32_t runFrameHardGuardTraceCpuRing = GetGenesisRunFrameHardGuardTraceCpuRing();
	uint32_t runFrameHardGuardTraceMmuCpuCycles = GetGenesisRunFrameHardGuardTraceMmuCpuCycles();
	uint32_t runFrameHardGuardTraceMmuCycles = GetGenesisRunFrameHardGuardTraceMmuCycles();
	uint32_t runFrameHardGuardTraceMmuCpuStride = GetGenesisRunFrameHardGuardTraceMmuCpuStride();
	uint32_t runFrameHardGuardTraceMmuStride = GetGenesisRunFrameHardGuardTraceMmuStride();
	uint32_t runFrameHardGuardTraceMmuFlowRing = GetGenesisRunFrameHardGuardTraceMmuFlowRing();
	uint32_t runFrameHardGuardTraceMmuOpRing = GetGenesisRunFrameHardGuardTraceMmuOpRing();
	uint32_t runFrameStallTraceCpuCycles = GetGenesisRunFrameStallRecoveryTraceCpuCycles();
	uint32_t runFrameStallTraceCpuStride = GetGenesisRunFrameStallRecoveryTraceCpuStride();
	uint32_t runFrameStallTraceCpuRing = GetGenesisRunFrameStallRecoveryTraceCpuRing();
	uint32_t runFrameStallTraceMmuCpuCycles = GetGenesisRunFrameStallRecoveryTraceMmuCpuCycles();
	uint32_t runFrameStallTraceMmuCycles = GetGenesisRunFrameStallRecoveryTraceMmuCycles();
	uint32_t runFrameStallTraceMmuCpuStride = GetGenesisRunFrameStallRecoveryTraceMmuCpuStride();
	uint32_t runFrameStallTraceMmuStride = GetGenesisRunFrameStallRecoveryTraceMmuStride();
	uint32_t runFrameStallTraceMmuFlowRing = GetGenesisRunFrameStallRecoveryTraceMmuFlowRing();
	uint32_t runFrameStallTraceMmuOpRing = GetGenesisRunFrameStallRecoveryTraceMmuOpRing();
	bool runFrameFrameAdvanceLogEnabled = IsGenesisRunFrameFrameAdvanceLogEnabled();
	uint32_t runFrameFrameAdvanceLogModulo = GetGenesisRunFrameFrameAdvanceLogModulo();
	bool runFrameFrameAdvanceIncludeCpuTrace = IsGenesisRunFrameFrameAdvanceIncludeCpuTraceEnabled();
	uint32_t runFrameFrameAdvanceCpuTraceLines = GetGenesisRunFrameFrameAdvanceCpuTraceLines();
	bool runFrameFrameAdvanceIncludeMmuOpWindow = IsGenesisRunFrameFrameAdvanceIncludeMmuOpWindowEnabled();
	bool runFrameFrameAdvanceIncludeCpuLoop = IsGenesisRunFrameFrameAdvanceIncludeCpuLoopEnabled();
	bool runFrameFrameAdvanceIncludeCpuAddr = IsGenesisRunFrameFrameAdvanceIncludeCpuAddrEnabled();
	bool runFrameFrameAdvanceIncludeHeartbeat = IsGenesisRunFrameFrameAdvanceIncludeHeartbeatEnabled();
	bool runFrameFrameAdvanceIncludeZ80Runtime = IsGenesisRunFrameFrameAdvanceIncludeZ80RuntimeEnabled();
	bool runFrameFrameAdvanceIncludeVdpState = IsGenesisRunFrameFrameAdvanceIncludeVdpStateEnabled();
	uint32_t runFrameFrameAdvanceMmuOpLines = GetGenesisRunFrameFrameAdvanceMmuOpWindowLines();
	uint32_t runFrameSehMmuOpLines = GetGenesisRunFrameSehMmuOpWindowLines();
	bool runFrameSehIncludeCpuTrace = IsGenesisRunFrameSehIncludeCpuTraceEnabled();
	bool runFrameSehIncludeCpuBoundary = IsGenesisRunFrameSehIncludeCpuBoundaryEnabled();
	bool runFrameSehIncludeCpuLoop = IsGenesisRunFrameSehIncludeCpuLoopEnabled();
	bool runFrameSehIncludeCpuAddr = IsGenesisRunFrameSehIncludeCpuAddrEnabled();
	bool runFrameSehIncludeStartup = IsGenesisRunFrameSehIncludeStartupEnabled();
	bool runFrameSehIncludeMmuFlow = IsGenesisRunFrameSehIncludeMmuFlowEnabled();
	bool runFrameSehIncludeMmuOps = IsGenesisRunFrameSehIncludeMmuOpsEnabled();
	bool runFrameSehIncludeMmuOpWindow = IsGenesisRunFrameSehIncludeMmuOpWindowEnabled();
	uint32_t runFrameSehCpuTraceLines = GetGenesisRunFrameSehCpuTraceLines();
	uint32_t runFrameFirstFailureMmuOpLines = GetGenesisRunFrameFirstFailureMmuOpWindowLines();
	bool runFrameStallIncludeMmuOpWindow = IsGenesisRunFrameStallRecoveryIncludeMmuOpWindowEnabled();
	uint32_t runFrameStallMmuOpLines = GetGenesisRunFrameStallRecoveryMmuOpWindowLines();
	bool runFrameStallIncludeCpuTrace = IsGenesisRunFrameStallRecoveryIncludeCpuTraceEnabled();
	bool runFrameStallIncludeStartup = IsGenesisRunFrameStallRecoveryIncludeStartupEnabled();
	bool runFrameStallIncludeCpuBoundary = IsGenesisRunFrameStallRecoveryIncludeCpuBoundaryEnabled();
	bool runFrameStallIncludeMmuFlow = IsGenesisRunFrameStallRecoveryIncludeMmuFlowEnabled();
	bool runFrameStallIncludeMmuOps = IsGenesisRunFrameStallRecoveryIncludeMmuOpsEnabled();
	bool runFrameStallIncludeTraceDigest = IsGenesisRunFrameStallRecoveryIncludeTraceDigestEnabled();
	uint32_t runFrameStallCpuTraceLines = GetGenesisRunFrameStallRecoveryCpuTraceLines();
	bool runFrameHardGuardIncludeCpuBoundary = IsGenesisRunFrameHardGuardIncludeCpuBoundaryEnabled();
	bool runFrameHardGuardIncludeCpuTrace = IsGenesisRunFrameHardGuardIncludeCpuTraceEnabled();
	bool runFrameHardGuardIncludeCpuLoop = IsGenesisRunFrameHardGuardIncludeCpuLoopEnabled();
	bool runFrameHardGuardIncludeCpuAddr = IsGenesisRunFrameHardGuardIncludeCpuAddrEnabled();
	bool runFrameHardGuardIncludeStartup = IsGenesisRunFrameHardGuardIncludeStartupEnabled();
	bool runFrameHardGuardIncludeMmuFlow = IsGenesisRunFrameHardGuardIncludeMmuFlowEnabled();
	bool runFrameHardGuardIncludeMmuOps = IsGenesisRunFrameHardGuardIncludeMmuOpsEnabled();
	bool runFrameHardGuardIncludeMmuOpWindow = IsGenesisRunFrameHardGuardIncludeMmuOpWindowEnabled();
	bool runFrameHardGuardIncludeHeartbeat = IsGenesisRunFrameHardGuardIncludeHeartbeatEnabled();
	bool runFrameHardGuardIncludeZ80Runtime = IsGenesisRunFrameHardGuardIncludeZ80RuntimeEnabled();
	bool runFrameHardGuardIncludeVdpState = IsGenesisRunFrameHardGuardIncludeVdpStateEnabled();
	bool runFrameHardGuardIncludeTraceDigest = IsGenesisRunFrameHardGuardIncludeTraceDigestEnabled();
	bool runFrameWaitIncludeHeartbeat = IsGenesisRunFrameWaitIncludeHeartbeatEnabled();
	bool runFrameWaitIncludeZ80Runtime = IsGenesisRunFrameWaitIncludeZ80RuntimeEnabled();
	bool runFrameWaitIncludeVdpState = IsGenesisRunFrameWaitIncludeVdpStateEnabled();
	bool runFrameWaitIncludeCpuLoop = IsGenesisRunFrameWaitIncludeCpuLoopEnabled();
	bool runFrameWaitIncludeCpuAddr = IsGenesisRunFrameWaitIncludeCpuAddrEnabled();
	bool runFrameWaitIncludeStartup = IsGenesisRunFrameWaitIncludeStartupEnabled();
	bool runFrameWaitIncludeMmuFlow = IsGenesisRunFrameWaitIncludeMmuFlowEnabled();
	bool runFrameWaitIncludeMmuOps = IsGenesisRunFrameWaitIncludeMmuOpsEnabled();
	bool runFrameWaitIncludeCpuBoundary = IsGenesisRunFrameWaitIncludeCpuBoundaryEnabled();
	bool runFrameWaitIncludeTraceDigest = IsGenesisRunFrameWaitIncludeTraceDigestEnabled();
	bool runFrameForcedCompletionDetailEnabled = IsGenesisRunFrameForcedCompletionDetailEnabled();
	bool runFrameForcedCompletionIncludeCpuTrace = IsGenesisRunFrameForcedCompletionIncludeCpuTraceEnabled();
	bool runFrameForcedCompletionIncludeCpuLoop = IsGenesisRunFrameForcedCompletionIncludeCpuLoopEnabled();
	bool runFrameForcedCompletionIncludeStartup = IsGenesisRunFrameForcedCompletionIncludeStartupEnabled();
	bool runFrameForcedCompletionIncludeMmuFlow = IsGenesisRunFrameForcedCompletionIncludeMmuFlowEnabled();
	bool runFrameForcedCompletionIncludeMmuOps = IsGenesisRunFrameForcedCompletionIncludeMmuOpsEnabled();
	bool runFrameForcedCompletionIncludeMmuOpWindow = IsGenesisRunFrameForcedCompletionIncludeMmuOpWindowEnabled();
	bool runFrameForcedCompletionIncludeCpuBoundary = IsGenesisRunFrameForcedCompletionIncludeCpuBoundaryEnabled();
	bool runFrameForcedCompletionIncludeCpuAddr = IsGenesisRunFrameForcedCompletionIncludeCpuAddrEnabled();
	bool runFrameForcedCompletionIncludeHeartbeat = IsGenesisRunFrameForcedCompletionIncludeHeartbeatEnabled();
	bool runFrameForcedCompletionIncludeZ80Runtime = IsGenesisRunFrameForcedCompletionIncludeZ80RuntimeEnabled();
	bool runFrameForcedCompletionIncludeVdpState = IsGenesisRunFrameForcedCompletionIncludeVdpStateEnabled();
	bool runFrameForcedCompletionIncludeTraceDigestDetail = IsGenesisRunFrameForcedCompletionIncludeTraceDigestDetailEnabled();
	bool runFrameForcedCompletionIncludeTraceDigest = IsGenesisRunFrameForcedCompletionIncludeTraceDigestEnabled();
	bool runFrameFirstFailureIncludeCpuTrace = IsGenesisRunFrameFirstFailureIncludeCpuTraceEnabled();
	uint32_t runFrameFirstFailureCpuTraceLines = GetGenesisRunFrameFirstFailureCpuTraceLines();
	bool runFrameFirstFailureIncludeStartup = IsGenesisRunFrameFirstFailureIncludeStartupEnabled();
	bool runFrameFirstFailureIncludeCpuLoop = IsGenesisRunFrameFirstFailureIncludeCpuLoopEnabled();
	bool runFrameFirstFailureIncludeCpuAddr = IsGenesisRunFrameFirstFailureIncludeCpuAddrEnabled();
	bool runFrameFirstFailureIncludeTraceDigest = IsGenesisRunFrameFirstFailureIncludeTraceDigestEnabled();
	bool runFrameFirstFailureIncludeMmuOpWindow = IsGenesisRunFrameFirstFailureIncludeMmuOpWindowEnabled();
	bool runFrameFrameAdvanceIncludeStartup = IsGenesisRunFrameFrameAdvanceIncludeStartupEnabled();
	bool runFrameFrameAdvanceIncludeMmuFlow = IsGenesisRunFrameFrameAdvanceIncludeMmuFlowEnabled();
	bool runFrameFrameAdvanceIncludeMmuOps = IsGenesisRunFrameFrameAdvanceIncludeMmuOpsEnabled();
	bool runFrameFrameAdvanceIncludeCpuBoundary = IsGenesisRunFrameFrameAdvanceIncludeCpuBoundaryEnabled();
	bool runFrameFrameAdvanceIncludeTraceDigest = IsGenesisRunFrameFrameAdvanceIncludeTraceDigestEnabled();
	bool runFrameSehIncludeHeartbeat = IsGenesisRunFrameSehIncludeHeartbeatEnabled();
	bool runFrameSehIncludeZ80Runtime = IsGenesisRunFrameSehIncludeZ80RuntimeEnabled();
	bool runFrameSehIncludeVdpState = IsGenesisRunFrameSehIncludeVdpStateEnabled();
	bool runFrameSehIncludeTraceDigest = IsGenesisRunFrameSehIncludeTraceDigestEnabled();
	bool runFrameStallIncludeHeartbeat = IsGenesisRunFrameStallRecoveryIncludeHeartbeatEnabled();
	bool runFrameStallIncludeZ80Runtime = IsGenesisRunFrameStallRecoveryIncludeZ80RuntimeEnabled();
	bool runFrameStallIncludeVdpState = IsGenesisRunFrameStallRecoveryIncludeVdpStateEnabled();
	bool runFrameStallIncludeCpuAddr = IsGenesisRunFrameStallRecoveryIncludeCpuAddrEnabled();
	bool runFrameFirstFailureIncludeCpuBoundary = IsGenesisRunFrameFirstFailureIncludeCpuBoundaryEnabled();
	bool runFrameFirstFailureIncludeMmuFlow = IsGenesisRunFrameFirstFailureIncludeMmuFlowEnabled();
	bool runFrameFirstFailureIncludeMmuOps = IsGenesisRunFrameFirstFailureIncludeMmuOpsEnabled();
	bool runFrameFirstFailureIncludeHeartbeat = IsGenesisRunFrameFirstFailureIncludeHeartbeatEnabled();
	bool runFrameFirstFailureIncludeZ80Runtime = IsGenesisRunFrameFirstFailureIncludeZ80RuntimeEnabled();
	bool runFrameFirstFailureIncludeVdpState = IsGenesisRunFrameFirstFailureIncludeVdpStateEnabled();
	bool runFrameFirstFailureIncludeTraceDigestExtended = IsGenesisRunFrameFirstFailureIncludeTraceDigestExtendedEnabled();
	return std::format(
		"entryCount={} exitCount={} earlyAbortCount={} firstFailureCaptures={} firstFailureBoundary={} lastGuard={} stalls={} forcedAdvances={} stallSummary={} entrySummary={} exitSummary={} cpuProbe={} cpuBoundaryProbe={} mmuFlow={} mmuOps={} startup={} sonicTraceArm={} sonicTraceArms={} sonicTraceLastArmFrame={} sonicTraceRearmWindow={} sonicTraceCpuCycles={} sonicTraceMmuCycles={} sonicTraceCpuStride={} sonicTraceMmuStride={} sonicTraceCpuRing={} sonicTraceMmuFlowRing={} sonicTraceMmuOpRing={} sonicCheckpointEnable={} sonicCheckpointInterval={} sonicCheckpointEndFrame={} sonicCheckpointIncludeOpWindow={} sonicCheckpointOpWindowLines={} sonicCheckpointIncludeCpuTrace={} sonicCheckpointCpuTraceLines={} runFrameStagnantIters={} runFrameForcedPulseLimit={} runFrameForcedPulseCycles={} runFrameFallbackPulses={} runFrameHardGuardCpuTraceLines={} runFrameHardGuardMmuOpLines={} runFrameForcedCompletionCpuTraceLines={} runFrameForcedCompletionMmuOpLines={} runFrameWaitLogEnable={} runFrameWaitLogInterval={} runFrameWaitIncludeMmuOpWindow={} runFrameWaitMmuOpLines={} runFrameWaitIncludeCpuTrace={} runFrameWaitCpuTraceLines={} runFrameHardGuardTraceCpuCycles={} runFrameHardGuardTraceCpuStride={} runFrameHardGuardTraceCpuRing={} runFrameHardGuardTraceMmuCpuCycles={} runFrameHardGuardTraceMmuCycles={} runFrameHardGuardTraceMmuCpuStride={} runFrameHardGuardTraceMmuStride={} runFrameHardGuardTraceMmuFlowRing={} runFrameHardGuardTraceMmuOpRing={} runFrameStallTraceCpuCycles={} runFrameStallTraceCpuStride={} runFrameStallTraceCpuRing={} runFrameStallTraceMmuCpuCycles={} runFrameStallTraceMmuCycles={} runFrameStallTraceMmuCpuStride={} runFrameStallTraceMmuStride={} runFrameStallTraceMmuFlowRing={} runFrameStallTraceMmuOpRing={} runFrameFrameAdvanceLogEnable={} runFrameFrameAdvanceLogModulo={} runFrameFrameAdvanceIncludeCpuTrace={} runFrameFrameAdvanceCpuTraceLines={} runFrameFrameAdvanceIncludeMmuOpWindow={} runFrameFrameAdvanceMmuOpLines={} runFrameSehMmuOpLines={} runFrameSehIncludeCpuTrace={} runFrameSehCpuTraceLines={} runFrameFirstFailureMmuOpLines={} runFrameStallIncludeMmuOpWindow={} runFrameStallMmuOpLines={} runFrameStallIncludeCpuTrace={} runFrameStallCpuTraceLines={} runFrameHardGuardIncludeCpuBoundary={} runFrameHardGuardIncludeCpuTrace={} runFrameHardGuardIncludeCpuLoop={} runFrameHardGuardIncludeCpuAddr={} runFrameHardGuardIncludeStartup={} runFrameHardGuardIncludeMmuFlow={} runFrameHardGuardIncludeMmuOps={} runFrameHardGuardIncludeMmuOpWindow={} runFrameWaitIncludeHeartbeat={} runFrameWaitIncludeZ80Runtime={} runFrameWaitIncludeVdpState={} runFrameWaitIncludeCpuLoop={} runFrameWaitIncludeCpuAddr={} runFrameForcedCompletionDetailEnable={} runFrameForcedCompletionIncludeCpuTrace={} runFrameForcedCompletionIncludeCpuLoop={} runFrameForcedCompletionIncludeStartup={} runFrameForcedCompletionIncludeMmuFlow={} runFrameForcedCompletionIncludeMmuOps={} runFrameForcedCompletionIncludeMmuOpWindow={} runFrameForcedCompletionIncludeCpuBoundary={} runFrameForcedCompletionIncludeCpuAddr={} runFrameForcedCompletionIncludeTraceDigest={} runFrameFirstFailureIncludeCpuTrace={} runFrameFirstFailureCpuTraceLines={} runFrameFirstFailureIncludeStartup={} runFrameFrameAdvanceIncludeCpuLoop={} runFrameFrameAdvanceIncludeCpuAddr={} runFrameFrameAdvanceIncludeHeartbeat={} runFrameFrameAdvanceIncludeZ80Runtime={} runFrameFrameAdvanceIncludeVdpState={} runFrameSehIncludeCpuBoundary={} runFrameSehIncludeCpuLoop={} runFrameSehIncludeCpuAddr={} runFrameSehIncludeStartup={} runFrameSehIncludeMmuFlow={} runFrameSehIncludeMmuOps={} runFrameSehIncludeMmuOpWindow={} runFrameStallIncludeStartup={} runFrameStallIncludeCpuBoundary={} runFrameStallIncludeMmuFlow={} runFrameStallIncludeMmuOps={} runFrameStallIncludeTraceDigest={} runFrameFirstFailureIncludeCpuBoundary={} runFrameFirstFailureIncludeMmuFlow={} runFrameFirstFailureIncludeMmuOps={} sonicCheckpoints={} sonicLastCheckpointFrame={} sonicLastCheckpointPc=${:06x}",
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
		sonicTraceCpuCycles,
		sonicTraceMmuCycles,
		sonicTraceCpuStride,
		sonicTraceMmuStride,
		sonicTraceCpuRing,
		sonicTraceMmuFlowRing,
		sonicTraceMmuOpRing,
		sonicCheckpointEnabled ? 1 : 0,
		sonicCheckpointInterval,
		sonicCheckpointEndFrame,
		sonicCheckpointIncludeOpWindow ? 1 : 0,
		sonicCheckpointOpWindowLines,
		sonicCheckpointIncludeCpuTrace ? 1 : 0,
		sonicCheckpointCpuTraceLines,
		runFrameStagnantIters,
		runFrameForcedPulseLimit,
		runFrameForcedPulseCycles,
		runFrameFallbackPulses,
		runFrameHardGuardCpuTraceLines,
		runFrameHardGuardMmuOpLines,
		runFrameForcedCompletionCpuTraceLines,
		runFrameForcedCompletionMmuOpLines,
		runFrameWaitLogEnabled ? 1 : 0,
		runFrameWaitLogInterval,
		runFrameWaitIncludeMmuOpWindow ? 1 : 0,
		runFrameWaitMmuOpLines,
		runFrameWaitIncludeCpuTrace ? 1 : 0,
		runFrameWaitCpuTraceLines,
		runFrameHardGuardTraceCpuCycles,
		runFrameHardGuardTraceCpuStride,
		runFrameHardGuardTraceCpuRing,
		runFrameHardGuardTraceMmuCpuCycles,
		runFrameHardGuardTraceMmuCycles,
		runFrameHardGuardTraceMmuCpuStride,
		runFrameHardGuardTraceMmuStride,
		runFrameHardGuardTraceMmuFlowRing,
		runFrameHardGuardTraceMmuOpRing,
		runFrameStallTraceCpuCycles,
		runFrameStallTraceCpuStride,
		runFrameStallTraceCpuRing,
		runFrameStallTraceMmuCpuCycles,
		runFrameStallTraceMmuCycles,
		runFrameStallTraceMmuCpuStride,
		runFrameStallTraceMmuStride,
		runFrameStallTraceMmuFlowRing,
		runFrameStallTraceMmuOpRing,
		runFrameFrameAdvanceLogEnabled ? 1 : 0,
		runFrameFrameAdvanceLogModulo,
		runFrameFrameAdvanceIncludeCpuTrace ? 1 : 0,
		runFrameFrameAdvanceCpuTraceLines,
		runFrameFrameAdvanceIncludeMmuOpWindow ? 1 : 0,
		runFrameFrameAdvanceMmuOpLines,
		runFrameSehMmuOpLines,
		runFrameSehIncludeCpuTrace ? 1 : 0,
		runFrameSehCpuTraceLines,
		runFrameFirstFailureMmuOpLines,
		runFrameStallIncludeMmuOpWindow ? 1 : 0,
		runFrameStallMmuOpLines,
		runFrameStallIncludeCpuTrace ? 1 : 0,
		runFrameStallCpuTraceLines,
		runFrameHardGuardIncludeCpuBoundary ? 1 : 0,
		runFrameHardGuardIncludeCpuTrace ? 1 : 0,
		runFrameHardGuardIncludeCpuLoop ? 1 : 0,
		runFrameHardGuardIncludeCpuAddr ? 1 : 0,
		runFrameHardGuardIncludeStartup ? 1 : 0,
		runFrameHardGuardIncludeMmuFlow ? 1 : 0,
		runFrameHardGuardIncludeMmuOps ? 1 : 0,
		runFrameHardGuardIncludeMmuOpWindow ? 1 : 0,
		runFrameWaitIncludeHeartbeat ? 1 : 0,
		runFrameWaitIncludeZ80Runtime ? 1 : 0,
		runFrameWaitIncludeVdpState ? 1 : 0,
		runFrameWaitIncludeCpuLoop ? 1 : 0,
		runFrameWaitIncludeCpuAddr ? 1 : 0,
		runFrameForcedCompletionDetailEnabled ? 1 : 0,
		runFrameForcedCompletionIncludeCpuTrace ? 1 : 0,
		runFrameForcedCompletionIncludeCpuLoop ? 1 : 0,
		runFrameForcedCompletionIncludeStartup ? 1 : 0,
		runFrameForcedCompletionIncludeMmuFlow ? 1 : 0,
		runFrameForcedCompletionIncludeMmuOps ? 1 : 0,
		runFrameForcedCompletionIncludeMmuOpWindow ? 1 : 0,
		runFrameForcedCompletionIncludeCpuBoundary ? 1 : 0,
		runFrameForcedCompletionIncludeCpuAddr ? 1 : 0,
		runFrameForcedCompletionIncludeTraceDigest ? 1 : 0,
		runFrameFirstFailureIncludeCpuTrace ? 1 : 0,
		runFrameFirstFailureCpuTraceLines,
		runFrameFirstFailureIncludeStartup ? 1 : 0,
		runFrameFrameAdvanceIncludeCpuLoop ? 1 : 0,
		runFrameFrameAdvanceIncludeCpuAddr ? 1 : 0,
		runFrameFrameAdvanceIncludeHeartbeat ? 1 : 0,
		runFrameFrameAdvanceIncludeZ80Runtime ? 1 : 0,
		runFrameFrameAdvanceIncludeVdpState ? 1 : 0,
		runFrameSehIncludeCpuBoundary ? 1 : 0,
		runFrameSehIncludeCpuLoop ? 1 : 0,
		runFrameSehIncludeCpuAddr ? 1 : 0,
		runFrameSehIncludeStartup ? 1 : 0,
		runFrameSehIncludeMmuFlow ? 1 : 0,
		runFrameSehIncludeMmuOps ? 1 : 0,
		runFrameSehIncludeMmuOpWindow ? 1 : 0,
		runFrameStallIncludeStartup ? 1 : 0,
		runFrameStallIncludeCpuBoundary ? 1 : 0,
		runFrameStallIncludeMmuFlow ? 1 : 0,
		runFrameStallIncludeMmuOps ? 1 : 0,
		runFrameStallIncludeTraceDigest ? 1 : 0,
		runFrameFirstFailureIncludeCpuBoundary ? 1 : 0,
		runFrameFirstFailureIncludeMmuFlow ? 1 : 0,
		runFrameFirstFailureIncludeMmuOps ? 1 : 0,
		_sonicStartupCheckpointCount,
		_sonicStartupLastCheckpointFrame,
		_sonicStartupLastCheckpointPc & 0x00ffffff) + std::format(
		" runFrameWaitIncludeStartup={} runFrameWaitIncludeMmuFlow={} runFrameWaitIncludeMmuOps={} runFrameWaitIncludeCpuBoundary={} runFrameWaitIncludeTraceDigest={} runFrameFrameAdvanceIncludeStartup={} runFrameFrameAdvanceIncludeMmuFlow={} runFrameFrameAdvanceIncludeMmuOps={} runFrameFrameAdvanceIncludeCpuBoundary={} runFrameFrameAdvanceIncludeTraceDigest={} runFrameSehIncludeHeartbeat={} runFrameSehIncludeZ80Runtime={} runFrameSehIncludeVdpState={} runFrameSehIncludeTraceDigest={} runFrameStallIncludeHeartbeat={} runFrameStallIncludeZ80Runtime={} runFrameStallIncludeVdpState={} runFrameStallIncludeCpuAddr={} runFrameFirstFailureIncludeCpuLoop={} runFrameFirstFailureIncludeCpuAddr={} runFrameFirstFailureIncludeTraceDigest={} runFrameFirstFailureIncludeMmuOpWindow={}",
		runFrameWaitIncludeStartup ? 1 : 0,
		runFrameWaitIncludeMmuFlow ? 1 : 0,
		runFrameWaitIncludeMmuOps ? 1 : 0,
		runFrameWaitIncludeCpuBoundary ? 1 : 0,
		runFrameWaitIncludeTraceDigest ? 1 : 0,
		runFrameFrameAdvanceIncludeStartup ? 1 : 0,
		runFrameFrameAdvanceIncludeMmuFlow ? 1 : 0,
		runFrameFrameAdvanceIncludeMmuOps ? 1 : 0,
		runFrameFrameAdvanceIncludeCpuBoundary ? 1 : 0,
		runFrameFrameAdvanceIncludeTraceDigest ? 1 : 0,
		runFrameSehIncludeHeartbeat ? 1 : 0,
		runFrameSehIncludeZ80Runtime ? 1 : 0,
		runFrameSehIncludeVdpState ? 1 : 0,
		runFrameSehIncludeTraceDigest ? 1 : 0,
		runFrameStallIncludeHeartbeat ? 1 : 0,
		runFrameStallIncludeZ80Runtime ? 1 : 0,
		runFrameStallIncludeVdpState ? 1 : 0,
		runFrameStallIncludeCpuAddr ? 1 : 0,
		runFrameFirstFailureIncludeCpuLoop ? 1 : 0,
		runFrameFirstFailureIncludeCpuAddr ? 1 : 0,
		runFrameFirstFailureIncludeTraceDigest ? 1 : 0,
		runFrameFirstFailureIncludeMmuOpWindow ? 1 : 0) + std::format(
		" runFrameHardGuardIncludeHeartbeat={} runFrameHardGuardIncludeZ80Runtime={} runFrameHardGuardIncludeVdpState={} runFrameHardGuardIncludeTraceDigest={} runFrameForcedCompletionIncludeHeartbeat={} runFrameForcedCompletionIncludeZ80Runtime={} runFrameForcedCompletionIncludeVdpState={} runFrameForcedCompletionIncludeTraceDigestDetail={} runFrameFirstFailureIncludeHeartbeat={} runFrameFirstFailureIncludeZ80Runtime={} runFrameFirstFailureIncludeVdpState={} runFrameFirstFailureIncludeTraceDigestExtended={}",
		runFrameHardGuardIncludeHeartbeat ? 1 : 0,
		runFrameHardGuardIncludeZ80Runtime ? 1 : 0,
		runFrameHardGuardIncludeVdpState ? 1 : 0,
		runFrameHardGuardIncludeTraceDigest ? 1 : 0,
		runFrameForcedCompletionIncludeHeartbeat ? 1 : 0,
		runFrameForcedCompletionIncludeZ80Runtime ? 1 : 0,
		runFrameForcedCompletionIncludeVdpState ? 1 : 0,
		runFrameForcedCompletionIncludeTraceDigestDetail ? 1 : 0,
		runFrameFirstFailureIncludeHeartbeat ? 1 : 0,
		runFrameFirstFailureIncludeZ80Runtime ? 1 : 0,
		runFrameFirstFailureIncludeVdpState ? 1 : 0,
		runFrameFirstFailureIncludeTraceDigestExtended ? 1 : 0);
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
			uint32_t traceCpuCycles = GetGenesisSonicTraceCpuCycles();
			uint32_t traceMmuCycles = GetGenesisSonicTraceMmuCycles();
			uint32_t traceCpuStride = GetGenesisSonicTraceCpuStride();
			uint32_t traceMmuStride = GetGenesisSonicTraceMmuStride();
			uint32_t traceCpuRing = GetGenesisSonicTraceCpuRing();
			uint32_t traceMmuFlowRing = GetGenesisSonicTraceMmuFlowRing();
			uint32_t traceMmuOpRing = GetGenesisSonicTraceMmuOpRing();
			bool includeOpWindow = IsGenesisSonicCheckpointIncludeOpWindowEnabled();
			uint32_t opWindowLines = GetGenesisSonicCheckpointOpWindowLines();
			bool includeCpuTrace = IsGenesisSonicCheckpointIncludeCpuTraceEnabled();
			uint32_t cpuTraceLines = GetGenesisSonicCheckpointCpuTraceLines();
			bool shouldArm = !_sonicTraceEscalationArmed;
			if (!shouldArm && startupFrame >= (_sonicTraceLastArmStartupFrame + rearmWindow)) {
				shouldArm = true;
			}

			if (shouldArm) {
				_cpu->ArmAggressiveFlowTrace(traceCpuCycles, traceCpuStride, traceCpuRing);
				_memoryManager->ArmAggressiveTraceBurst(traceCpuCycles, traceMmuCycles, traceCpuStride, traceMmuStride, traceMmuFlowRing, traceMmuOpRing);
				_sonicTraceEscalationArmed = true;
				_sonicTraceEscalationCount++;
				_sonicTraceLastArmStartupFrame = startupFrame;
				MessageManager::Log(std::format("[Genesis] RunFrame sonic trace escalation arm #{} titleClass={} startupFrame={} rearmWindow={} cpuCycles={} mmuCycles={} cpuStride={} mmuStride={} cpuRing={} mmuFlowRing={} mmuOpRing={} startup={}",
					_sonicTraceEscalationCount,
					titleClass,
					startupFrame,
					rearmWindow,
					traceCpuCycles,
					traceMmuCycles,
					traceCpuStride,
					traceMmuStride,
					traceCpuRing,
					traceMmuFlowRing,
					traceMmuOpRing,
					BuildGenesisStartupRuntimeSummary(_memoryManager.get())));
			}

			if (IsGenesisSonicStartupCheckpointEnabled()) {
				uint32_t interval = GetGenesisSonicStartupCheckpointIntervalFrames();
				uint32_t endFrame = GetGenesisSonicStartupCheckpointEndFrame();
				if (startupFrame <= endFrame && (startupFrame % interval) == 0 && startupFrame != _sonicStartupLastCheckpointFrame) {
					_sonicStartupLastCheckpointFrame = startupFrame;
					_sonicStartupLastCheckpointPc = _cpu->GetState().PC & 0x00ffffff;
					_sonicStartupCheckpointCount++;
					MessageManager::Log(std::format("[Genesis] Sonic startup checkpoint #{} frame={} pc=${:06x} cycles={} loop={} startup={} mmuFlow={}{}{}",
						_sonicStartupCheckpointCount,
						startupFrame,
						_sonicStartupLastCheckpointPc,
						_cpu->GetState().CycleCount,
						_cpu->BuildSamePcLoopSummary(),
						BuildGenesisStartupRuntimeSummary(_memoryManager.get()),
						_memoryManager->BuildRuntimeFlowTraceSummary(),
						includeOpWindow ? std::format(" mmuOpsWindow={}", _memoryManager->BuildRuntimeOpTraceWindow(opWindowLines)) : "",
						includeCpuTrace ? std::format(" cpuTrace={}", _cpu->BuildInstructionTraceWindow(cpuTraceLines)) : ""));
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
	uint32_t stagnantIterationThreshold = GetGenesisRunFrameStagnantIterationThreshold();
	uint32_t forcedAdvancePulseLimit = GetGenesisRunFrameForcedAdvancePulseLimit();
	uint32_t forcedAdvancePulseCycles = GetGenesisRunFrameForcedAdvancePulseCycles();
	uint32_t fallbackPulseIterations = GetGenesisRunFrameFallbackPulseIterations();
	uint32_t hardGuardCpuTraceLines = GetGenesisRunFrameHardGuardCpuTraceLines();
	uint32_t hardGuardMmuOpWindowLines = GetGenesisRunFrameHardGuardMmuOpWindowLines();
	uint32_t forcedCompletionCpuTraceLines = GetGenesisRunFrameForcedCompletionCpuTraceLines();
	uint32_t forcedCompletionMmuOpWindowLines = GetGenesisRunFrameForcedCompletionMmuOpWindowLines();
	bool waitLogEnabled = IsGenesisRunFrameWaitLogEnabled();
	uint32_t waitLogIntervalGuard = GetGenesisRunFrameWaitLogIntervalGuard();
	bool waitIncludeMmuOpWindow = IsGenesisRunFrameWaitIncludeMmuOpWindowEnabled();
	uint32_t waitMmuOpWindowLines = GetGenesisRunFrameWaitMmuOpWindowLines();
	bool waitIncludeCpuTrace = IsGenesisRunFrameWaitIncludeCpuTraceEnabled();
	uint32_t waitCpuTraceLines = GetGenesisRunFrameWaitCpuTraceLines();
	bool waitIncludeHeartbeat = IsGenesisRunFrameWaitIncludeHeartbeatEnabled();
	bool waitIncludeZ80Runtime = IsGenesisRunFrameWaitIncludeZ80RuntimeEnabled();
	bool waitIncludeVdpState = IsGenesisRunFrameWaitIncludeVdpStateEnabled();
	bool waitIncludeCpuLoop = IsGenesisRunFrameWaitIncludeCpuLoopEnabled();
	bool waitIncludeCpuAddr = IsGenesisRunFrameWaitIncludeCpuAddrEnabled();
	bool waitIncludeStartup = IsGenesisRunFrameWaitIncludeStartupEnabled();
	bool waitIncludeMmuFlow = IsGenesisRunFrameWaitIncludeMmuFlowEnabled();
	bool waitIncludeMmuOps = IsGenesisRunFrameWaitIncludeMmuOpsEnabled();
	bool waitIncludeCpuBoundary = IsGenesisRunFrameWaitIncludeCpuBoundaryEnabled();
	bool waitIncludeTraceDigest = IsGenesisRunFrameWaitIncludeTraceDigestEnabled();
	bool frameAdvanceLogEnabled = IsGenesisRunFrameFrameAdvanceLogEnabled();
	uint32_t frameAdvanceLogModulo = GetGenesisRunFrameFrameAdvanceLogModulo();
	bool frameAdvanceIncludeCpuTrace = IsGenesisRunFrameFrameAdvanceIncludeCpuTraceEnabled();
	bool frameAdvanceIncludeCpuLoop = IsGenesisRunFrameFrameAdvanceIncludeCpuLoopEnabled();
	bool frameAdvanceIncludeCpuAddr = IsGenesisRunFrameFrameAdvanceIncludeCpuAddrEnabled();
	bool frameAdvanceIncludeHeartbeat = IsGenesisRunFrameFrameAdvanceIncludeHeartbeatEnabled();
	bool frameAdvanceIncludeZ80Runtime = IsGenesisRunFrameFrameAdvanceIncludeZ80RuntimeEnabled();
	bool frameAdvanceIncludeVdpState = IsGenesisRunFrameFrameAdvanceIncludeVdpStateEnabled();
	bool frameAdvanceIncludeStartup = IsGenesisRunFrameFrameAdvanceIncludeStartupEnabled();
	bool frameAdvanceIncludeMmuFlow = IsGenesisRunFrameFrameAdvanceIncludeMmuFlowEnabled();
	bool frameAdvanceIncludeMmuOps = IsGenesisRunFrameFrameAdvanceIncludeMmuOpsEnabled();
	bool frameAdvanceIncludeCpuBoundary = IsGenesisRunFrameFrameAdvanceIncludeCpuBoundaryEnabled();
	bool frameAdvanceIncludeTraceDigest = IsGenesisRunFrameFrameAdvanceIncludeTraceDigestEnabled();
	uint32_t frameAdvanceCpuTraceLines = GetGenesisRunFrameFrameAdvanceCpuTraceLines();
	bool frameAdvanceIncludeMmuOpWindow = IsGenesisRunFrameFrameAdvanceIncludeMmuOpWindowEnabled();
	uint32_t frameAdvanceMmuOpLines = GetGenesisRunFrameFrameAdvanceMmuOpWindowLines();
	uint32_t sehMmuOpLines = GetGenesisRunFrameSehMmuOpWindowLines();
	bool sehIncludeCpuTrace = IsGenesisRunFrameSehIncludeCpuTraceEnabled();
	bool sehIncludeCpuBoundary = IsGenesisRunFrameSehIncludeCpuBoundaryEnabled();
	bool sehIncludeCpuLoop = IsGenesisRunFrameSehIncludeCpuLoopEnabled();
	bool sehIncludeCpuAddr = IsGenesisRunFrameSehIncludeCpuAddrEnabled();
	bool sehIncludeStartup = IsGenesisRunFrameSehIncludeStartupEnabled();
	bool sehIncludeMmuFlow = IsGenesisRunFrameSehIncludeMmuFlowEnabled();
	bool sehIncludeMmuOps = IsGenesisRunFrameSehIncludeMmuOpsEnabled();
	bool sehIncludeMmuOpWindow = IsGenesisRunFrameSehIncludeMmuOpWindowEnabled();
	bool sehIncludeHeartbeat = IsGenesisRunFrameSehIncludeHeartbeatEnabled();
	bool sehIncludeZ80Runtime = IsGenesisRunFrameSehIncludeZ80RuntimeEnabled();
	bool sehIncludeVdpState = IsGenesisRunFrameSehIncludeVdpStateEnabled();
	bool sehIncludeTraceDigest = IsGenesisRunFrameSehIncludeTraceDigestEnabled();
	uint32_t sehCpuTraceLines = GetGenesisRunFrameSehCpuTraceLines();
	uint32_t firstFailureMmuOpLines = GetGenesisRunFrameFirstFailureMmuOpWindowLines();
	bool stallIncludeMmuOpWindow = IsGenesisRunFrameStallRecoveryIncludeMmuOpWindowEnabled();
	uint32_t stallMmuOpLines = GetGenesisRunFrameStallRecoveryMmuOpWindowLines();
	bool stallIncludeCpuTrace = IsGenesisRunFrameStallRecoveryIncludeCpuTraceEnabled();
	bool stallIncludeStartup = IsGenesisRunFrameStallRecoveryIncludeStartupEnabled();
	bool stallIncludeCpuBoundary = IsGenesisRunFrameStallRecoveryIncludeCpuBoundaryEnabled();
	bool stallIncludeMmuFlow = IsGenesisRunFrameStallRecoveryIncludeMmuFlowEnabled();
	bool stallIncludeMmuOps = IsGenesisRunFrameStallRecoveryIncludeMmuOpsEnabled();
	bool stallIncludeTraceDigest = IsGenesisRunFrameStallRecoveryIncludeTraceDigestEnabled();
	bool stallIncludeHeartbeat = IsGenesisRunFrameStallRecoveryIncludeHeartbeatEnabled();
	bool stallIncludeZ80Runtime = IsGenesisRunFrameStallRecoveryIncludeZ80RuntimeEnabled();
	bool stallIncludeVdpState = IsGenesisRunFrameStallRecoveryIncludeVdpStateEnabled();
	bool stallIncludeCpuAddr = IsGenesisRunFrameStallRecoveryIncludeCpuAddrEnabled();
	uint32_t stallCpuTraceLines = GetGenesisRunFrameStallRecoveryCpuTraceLines();
	bool hardGuardIncludeCpuBoundary = IsGenesisRunFrameHardGuardIncludeCpuBoundaryEnabled();
	bool hardGuardIncludeCpuTrace = IsGenesisRunFrameHardGuardIncludeCpuTraceEnabled();
	bool hardGuardIncludeCpuLoop = IsGenesisRunFrameHardGuardIncludeCpuLoopEnabled();
	bool hardGuardIncludeCpuAddr = IsGenesisRunFrameHardGuardIncludeCpuAddrEnabled();
	bool hardGuardIncludeStartup = IsGenesisRunFrameHardGuardIncludeStartupEnabled();
	bool hardGuardIncludeMmuFlow = IsGenesisRunFrameHardGuardIncludeMmuFlowEnabled();
	bool hardGuardIncludeMmuOps = IsGenesisRunFrameHardGuardIncludeMmuOpsEnabled();
	bool hardGuardIncludeMmuOpWindow = IsGenesisRunFrameHardGuardIncludeMmuOpWindowEnabled();
	bool hardGuardIncludeHeartbeat = IsGenesisRunFrameHardGuardIncludeHeartbeatEnabled();
	bool hardGuardIncludeZ80Runtime = IsGenesisRunFrameHardGuardIncludeZ80RuntimeEnabled();
	bool hardGuardIncludeVdpState = IsGenesisRunFrameHardGuardIncludeVdpStateEnabled();
	bool hardGuardIncludeTraceDigest = IsGenesisRunFrameHardGuardIncludeTraceDigestEnabled();
	bool forcedCompletionDetailEnabled = IsGenesisRunFrameForcedCompletionDetailEnabled();
	bool forcedCompletionIncludeCpuTrace = IsGenesisRunFrameForcedCompletionIncludeCpuTraceEnabled();
	bool forcedCompletionIncludeCpuLoop = IsGenesisRunFrameForcedCompletionIncludeCpuLoopEnabled();
	bool forcedCompletionIncludeStartup = IsGenesisRunFrameForcedCompletionIncludeStartupEnabled();
	bool forcedCompletionIncludeMmuFlow = IsGenesisRunFrameForcedCompletionIncludeMmuFlowEnabled();
	bool forcedCompletionIncludeMmuOps = IsGenesisRunFrameForcedCompletionIncludeMmuOpsEnabled();
	bool forcedCompletionIncludeMmuOpWindow = IsGenesisRunFrameForcedCompletionIncludeMmuOpWindowEnabled();
	bool forcedCompletionIncludeCpuBoundary = IsGenesisRunFrameForcedCompletionIncludeCpuBoundaryEnabled();
	bool forcedCompletionIncludeCpuAddr = IsGenesisRunFrameForcedCompletionIncludeCpuAddrEnabled();
	bool forcedCompletionIncludeHeartbeat = IsGenesisRunFrameForcedCompletionIncludeHeartbeatEnabled();
	bool forcedCompletionIncludeZ80Runtime = IsGenesisRunFrameForcedCompletionIncludeZ80RuntimeEnabled();
	bool forcedCompletionIncludeVdpState = IsGenesisRunFrameForcedCompletionIncludeVdpStateEnabled();
	bool forcedCompletionIncludeTraceDigestDetail = IsGenesisRunFrameForcedCompletionIncludeTraceDigestDetailEnabled();
	bool forcedCompletionIncludeTraceDigest = IsGenesisRunFrameForcedCompletionIncludeTraceDigestEnabled();
	bool firstFailureIncludeCpuTrace = IsGenesisRunFrameFirstFailureIncludeCpuTraceEnabled();
	uint32_t firstFailureCpuTraceLines = GetGenesisRunFrameFirstFailureCpuTraceLines();
	bool firstFailureIncludeStartup = IsGenesisRunFrameFirstFailureIncludeStartupEnabled();
	bool firstFailureIncludeCpuBoundary = IsGenesisRunFrameFirstFailureIncludeCpuBoundaryEnabled();
	bool firstFailureIncludeMmuFlow = IsGenesisRunFrameFirstFailureIncludeMmuFlowEnabled();
	bool firstFailureIncludeMmuOps = IsGenesisRunFrameFirstFailureIncludeMmuOpsEnabled();
	bool firstFailureIncludeHeartbeat = IsGenesisRunFrameFirstFailureIncludeHeartbeatEnabled();
	bool firstFailureIncludeZ80Runtime = IsGenesisRunFrameFirstFailureIncludeZ80RuntimeEnabled();
	bool firstFailureIncludeVdpState = IsGenesisRunFrameFirstFailureIncludeVdpStateEnabled();
	bool firstFailureIncludeTraceDigestExtended = IsGenesisRunFrameFirstFailureIncludeTraceDigestExtendedEnabled();
	bool firstFailureIncludeCpuLoop = IsGenesisRunFrameFirstFailureIncludeCpuLoopEnabled();
	bool firstFailureIncludeCpuAddr = IsGenesisRunFrameFirstFailureIncludeCpuAddrEnabled();
	bool firstFailureIncludeTraceDigest = IsGenesisRunFrameFirstFailureIncludeTraceDigestEnabled();
	bool firstFailureIncludeMmuOpWindow = IsGenesisRunFrameFirstFailureIncludeMmuOpWindowEnabled();
	uint32_t hardGuardTraceCpuCycles = GetGenesisRunFrameHardGuardTraceCpuCycles();
	uint32_t hardGuardTraceCpuStride = GetGenesisRunFrameHardGuardTraceCpuStride();
	uint32_t hardGuardTraceCpuRing = GetGenesisRunFrameHardGuardTraceCpuRing();
	uint32_t hardGuardTraceMmuCpuCycles = GetGenesisRunFrameHardGuardTraceMmuCpuCycles();
	uint32_t hardGuardTraceMmuCycles = GetGenesisRunFrameHardGuardTraceMmuCycles();
	uint32_t hardGuardTraceMmuCpuStride = GetGenesisRunFrameHardGuardTraceMmuCpuStride();
	uint32_t hardGuardTraceMmuStride = GetGenesisRunFrameHardGuardTraceMmuStride();
	uint32_t hardGuardTraceMmuFlowRing = GetGenesisRunFrameHardGuardTraceMmuFlowRing();
	uint32_t hardGuardTraceMmuOpRing = GetGenesisRunFrameHardGuardTraceMmuOpRing();
	uint32_t stallTraceCpuCycles = GetGenesisRunFrameStallRecoveryTraceCpuCycles();
	uint32_t stallTraceCpuStride = GetGenesisRunFrameStallRecoveryTraceCpuStride();
	uint32_t stallTraceCpuRing = GetGenesisRunFrameStallRecoveryTraceCpuRing();
	uint32_t stallTraceMmuCpuCycles = GetGenesisRunFrameStallRecoveryTraceMmuCpuCycles();
	uint32_t stallTraceMmuCycles = GetGenesisRunFrameStallRecoveryTraceMmuCycles();
	uint32_t stallTraceMmuCpuStride = GetGenesisRunFrameStallRecoveryTraceMmuCpuStride();
	uint32_t stallTraceMmuStride = GetGenesisRunFrameStallRecoveryTraceMmuStride();
	uint32_t stallTraceMmuFlowRing = GetGenesisRunFrameStallRecoveryTraceMmuFlowRing();
	uint32_t stallTraceMmuOpRing = GetGenesisRunFrameStallRecoveryTraceMmuOpRing();
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
				_cpu->ArmAggressiveFlowTrace(hardGuardTraceCpuCycles, hardGuardTraceCpuStride, hardGuardTraceCpuRing);
				_memoryManager->ArmAggressiveTraceBurst(hardGuardTraceMmuCpuCycles, hardGuardTraceMmuCycles, hardGuardTraceMmuCpuStride, hardGuardTraceMmuStride, hardGuardTraceMmuFlowRing, hardGuardTraceMmuOpRing);
				_runFrameStallEventCount++;
				_runFrameLastStallSummary = _cpu->BuildExecutionStallSummary();
				GenesisVdpState hardGuardVdpState = _vdp->GetState();
				GenesisIoState hardGuardIoState = _memoryManager->GetIoState();
				string hardGuardExtras;
				if (hardGuardIncludeHeartbeat) {
					hardGuardExtras += std::format(" heartbeatPc=${:06x} heartbeatCycles={} heartbeatInstr={}", hardGuardIoState.CpuProgramCounterHeartbeat, hardGuardIoState.CpuCycleHeartbeat, hardGuardIoState.CpuInstructionHeartbeat);
				}
				if (hardGuardIncludeZ80Runtime) {
					hardGuardExtras += std::format(" z80Running={} z80RunnableCycles={} z80StalledCycles={} z80Transitions={}", _memoryManager->GetZ80RuntimeRunning() ? 1 : 0, _memoryManager->GetZ80RuntimeRunnableCycles(), _memoryManager->GetZ80RuntimeStalledCycles(), _memoryManager->GetZ80RuntimeTransitionCount());
				}
				if (hardGuardIncludeVdpState) {
					hardGuardExtras += std::format(" vdpVc={} vdpHc={} vdpStatus=${:04x}", hardGuardVdpState.VCounter, hardGuardVdpState.HCounter, hardGuardVdpState.StatusRegister);
				}
				if (hardGuardIncludeTraceDigest) {
					hardGuardExtras += std::format(" traceDigest={}", _cpu->BuildInstructionTraceDigest());
				}
				if (_runFrameFirstFailureBoundarySummary.empty()) {
					_runFrameFirstFailureBoundarySummary = _cpu->BuildDispatchBoundaryProbeSummary();
					_runFrameFirstFailureBoundaryCaptureCount++;
				}
				MessageManager::Log(std::format("[Genesis] RunFrame hard-guard abort frame={} guard={} cap={} pc=${:06x} cycles={} stall={}{}{}{}{}{}{}{}{}{}",
					frame,
					guard,
					hardInstructionCap,
					_cpu->GetState().PC & 0x00ffffff,
					_cpu->GetState().CycleCount,
					_runFrameLastStallSummary,
					hardGuardIncludeCpuBoundary ? std::format(" cpuBoundary={}", _runFrameFirstFailureBoundarySummary) : "",
					hardGuardIncludeCpuTrace ? std::format(" cpuTrace={}", _cpu->BuildInstructionTraceWindow(hardGuardCpuTraceLines)) : "",
					hardGuardIncludeCpuLoop ? std::format(" cpuLoop={}", _cpu->BuildSamePcLoopSummary()) : "",
					hardGuardIncludeCpuAddr ? std::format(" cpuAddr={}", _cpu->BuildAddressErrorSummary()) : "",
					hardGuardIncludeStartup ? std::format(" startup={}", BuildGenesisStartupRuntimeSummary(_memoryManager.get())) : "",
					hardGuardIncludeMmuFlow ? std::format(" mmuFlow={}", _memoryManager->BuildRuntimeFlowTraceSummary()) : "",
					hardGuardIncludeMmuOps ? std::format(" mmuOps={}", _memoryManager->BuildRuntimeOpTraceSummary()) : "",
					hardGuardIncludeMmuOpWindow ? std::format(" mmuOpsWindow={}", _memoryManager->BuildRuntimeOpTraceWindow(hardGuardMmuOpWindowLines)) : "",
					hardGuardExtras));
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

			if (stagnantIterations >= stagnantIterationThreshold) {
				_cpu->ArmAggressiveFlowTrace(stallTraceCpuCycles, stallTraceCpuStride, stallTraceCpuRing);
				_memoryManager->ArmAggressiveTraceBurst(stallTraceMmuCpuCycles, stallTraceMmuCycles, stallTraceMmuCpuStride, stallTraceMmuStride, stallTraceMmuFlowRing, stallTraceMmuOpRing);
				_runFrameStallEventCount++;
				forcedAdvancePulses++;
				_cpu->ForceClockAdvance(forcedAdvancePulseCycles);
				_runFrameForcedAdvanceCount++;
				_runFrameLastStallSummary = _cpu->BuildExecutionStallSummary();
				GenesisVdpState stallVdpState = _vdp->GetState();
				GenesisIoState stallIoState = _memoryManager->GetIoState();
				string stallExtras;
				if (stallIncludeHeartbeat) {
					stallExtras += std::format(" heartbeatPc=${:06x} heartbeatCycles={} heartbeatInstr={}", stallIoState.CpuProgramCounterHeartbeat, stallIoState.CpuCycleHeartbeat, stallIoState.CpuInstructionHeartbeat);
				}
				if (stallIncludeZ80Runtime) {
					stallExtras += std::format(" z80Running={} z80RunnableCycles={} z80StalledCycles={} z80Transitions={}", _memoryManager->GetZ80RuntimeRunning() ? 1 : 0, _memoryManager->GetZ80RuntimeRunnableCycles(), _memoryManager->GetZ80RuntimeStalledCycles(), _memoryManager->GetZ80RuntimeTransitionCount());
				}
				if (stallIncludeVdpState) {
					stallExtras += std::format(" vdpVc={} vdpHc={} vdpStatus=${:04x}", stallVdpState.VCounter, stallVdpState.HCounter, stallVdpState.StatusRegister);
				}
				MessageManager::Log(std::format("[Genesis] RunFrame stall recovery #{} pulse={} frame={} guard={} summary={} loop={} masterClock={} cpuCycles={}{}{}{}{}{}{}{}{}{}",
					_runFrameStallEventCount,
					forcedAdvancePulses,
					frame,
					guard,
					_runFrameLastStallSummary,
					_cpu->BuildSamePcLoopSummary(),
					_memoryManager->GetMasterClock(),
					_cpu->GetState().CycleCount,
					stallIncludeMmuOpWindow ? std::format(" mmuOpsWindow={}", _memoryManager->BuildRuntimeOpTraceWindow(stallMmuOpLines)) : "",
					stallIncludeCpuTrace ? std::format(" cpuTrace={}", _cpu->BuildInstructionTraceWindow(stallCpuTraceLines)) : "",
					stallIncludeStartup ? std::format(" startup={}", BuildGenesisStartupRuntimeSummary(_memoryManager.get())) : "",
					stallIncludeCpuBoundary ? std::format(" cpuBoundary={}", _cpu->BuildDispatchBoundaryProbeSummary()) : "",
					stallIncludeCpuAddr ? std::format(" cpuAddr={}", _cpu->BuildAddressErrorSummary()) : "",
					stallIncludeMmuFlow ? std::format(" mmuFlow={}", _memoryManager->BuildRuntimeFlowTraceSummary()) : "",
					stallIncludeMmuOps ? std::format(" mmuOps={}", _memoryManager->BuildRuntimeOpTraceSummary()) : "",
					stallIncludeTraceDigest ? std::format(" traceDigest={}", _cpu->BuildInstructionTraceDigest()) : "",
					stallExtras));
				stagnantIterations = 0;
				lastProgressClock = _memoryManager->GetMasterClock();
				lastProgressCpuCycles = _cpu->GetState().CycleCount;

				if (forcedAdvancePulses >= forcedAdvancePulseLimit) {
					if (_runFrameFirstFailureBoundarySummary.empty()) {
						_runFrameFirstFailureBoundarySummary = _cpu->BuildDispatchBoundaryProbeSummary();
						_runFrameFirstFailureBoundaryCaptureCount++;
						GenesisVdpState firstFailureVdpState = _vdp->GetState();
						GenesisIoState firstFailureIoState = _memoryManager->GetIoState();
						string firstFailureExtras;
						if (firstFailureIncludeHeartbeat) {
							firstFailureExtras += std::format(" heartbeatPc=${:06x} heartbeatCycles={} heartbeatInstr={}", firstFailureIoState.CpuProgramCounterHeartbeat, firstFailureIoState.CpuCycleHeartbeat, firstFailureIoState.CpuInstructionHeartbeat);
						}
						if (firstFailureIncludeZ80Runtime) {
							firstFailureExtras += std::format(" z80Running={} z80RunnableCycles={} z80StalledCycles={} z80Transitions={}", _memoryManager->GetZ80RuntimeRunning() ? 1 : 0, _memoryManager->GetZ80RuntimeRunnableCycles(), _memoryManager->GetZ80RuntimeStalledCycles(), _memoryManager->GetZ80RuntimeTransitionCount());
						}
						if (firstFailureIncludeVdpState) {
							firstFailureExtras += std::format(" vdpVc={} vdpHc={} vdpStatus=${:04x}", firstFailureVdpState.VCounter, firstFailureVdpState.HCounter, firstFailureVdpState.StatusRegister);
						}
						if (firstFailureIncludeTraceDigestExtended) {
							firstFailureExtras += std::format(" traceDigestExt={}", _cpu->BuildInstructionTraceDigest());
						}
						MessageManager::Log(std::format("[Genesis] RunFrame first-failure pair{}{}{}{}{}{}{}{}{}",
							firstFailureIncludeCpuBoundary ? std::format(" cpuBoundary={}", _runFrameFirstFailureBoundarySummary) : "",
							firstFailureIncludeCpuLoop ? std::format(" cpuLoop={}", _cpu->BuildSamePcLoopSummary()) : "",
							firstFailureIncludeCpuAddr ? std::format(" cpuAddr={}", _cpu->BuildAddressErrorSummary()) : "",
							firstFailureIncludeMmuFlow ? std::format(" mmuFlow={}", _memoryManager->BuildRuntimeFlowTraceSummary()) : "",
							firstFailureIncludeMmuOps ? std::format(" mmuOps={}", _memoryManager->BuildRuntimeOpTraceSummary()) : "",
							firstFailureIncludeMmuOpWindow ? std::format(" mmuOpsWindow={}", _memoryManager->BuildRuntimeOpTraceWindow(firstFailureMmuOpLines)) : "",
							firstFailureIncludeCpuTrace ? std::format(" cpuTrace={}", _cpu->BuildInstructionTraceWindow(firstFailureCpuTraceLines)) : "",
							firstFailureIncludeStartup ? std::format(" startup={}", BuildGenesisStartupRuntimeSummary(_memoryManager.get())) : "",
							firstFailureIncludeTraceDigest ? std::format(" traceDigest={}", _cpu->BuildInstructionTraceDigest()) : "") + firstFailureExtras);
					}
					MessageManager::Log(std::format("[Genesis] RunFrame forced completion frame={} guard={} pulses={}{}",
						frame,
						guard,
						forcedAdvancePulses,
						forcedCompletionIncludeTraceDigest ? std::format(" traceDigest={}", _cpu->BuildInstructionTraceDigest()) : ""));
					if (forcedCompletionDetailEnabled) {
						GenesisVdpState forcedCompletionVdpState = _vdp->GetState();
						GenesisIoState forcedCompletionIoState = _memoryManager->GetIoState();
						string forcedCompletionExtras;
						if (forcedCompletionIncludeHeartbeat) {
							forcedCompletionExtras += std::format(" heartbeatPc=${:06x} heartbeatCycles={} heartbeatInstr={}", forcedCompletionIoState.CpuProgramCounterHeartbeat, forcedCompletionIoState.CpuCycleHeartbeat, forcedCompletionIoState.CpuInstructionHeartbeat);
						}
						if (forcedCompletionIncludeZ80Runtime) {
							forcedCompletionExtras += std::format(" z80Running={} z80RunnableCycles={} z80StalledCycles={} z80Transitions={}", _memoryManager->GetZ80RuntimeRunning() ? 1 : 0, _memoryManager->GetZ80RuntimeRunnableCycles(), _memoryManager->GetZ80RuntimeStalledCycles(), _memoryManager->GetZ80RuntimeTransitionCount());
						}
						if (forcedCompletionIncludeVdpState) {
							forcedCompletionExtras += std::format(" vdpVc={} vdpHc={} vdpStatus=${:04x}", forcedCompletionVdpState.VCounter, forcedCompletionVdpState.HCounter, forcedCompletionVdpState.StatusRegister);
						}
						if (forcedCompletionIncludeTraceDigestDetail) {
							forcedCompletionExtras += std::format(" traceDigestDetail={}", _cpu->BuildInstructionTraceDigest());
						}
						MessageManager::Log(std::format("[Genesis] RunFrame forced completion detail{}{}{}{}{}{}{}{}",
							forcedCompletionIncludeCpuTrace ? std::format(" cpuTrace={}", _cpu->BuildInstructionTraceWindow(forcedCompletionCpuTraceLines)) : "",
							forcedCompletionIncludeCpuLoop ? std::format(" cpuLoop={}", _cpu->BuildSamePcLoopSummary()) : "",
							forcedCompletionIncludeStartup ? std::format(" startup={}", BuildGenesisStartupRuntimeSummary(_memoryManager.get())) : "",
							forcedCompletionIncludeMmuFlow ? std::format(" mmuFlow={}", _memoryManager->BuildRuntimeFlowTraceSummary()) : "",
							forcedCompletionIncludeMmuOps ? std::format(" mmuOps={}", _memoryManager->BuildRuntimeOpTraceSummary()) : "",
							forcedCompletionIncludeMmuOpWindow ? std::format(" mmuOpsWindow={}", _memoryManager->BuildRuntimeOpTraceWindow(forcedCompletionMmuOpWindowLines)) : "",
							forcedCompletionIncludeCpuBoundary ? std::format(" cpuBoundary={}", _cpu->BuildDispatchBoundaryProbeSummary()) : "",
							forcedCompletionIncludeCpuAddr ? std::format(" cpuAddr={}", _cpu->BuildAddressErrorSummary()) : "") + forcedCompletionExtras);
					}
					break;
				}
			}

			if (waitLogEnabled && (guard % waitLogIntervalGuard) == 0) {
				GenesisVdpState vdpState = _vdp->GetState();
				GenesisIoState ioState = _memoryManager->GetIoState();
				MessageManager::Log(std::format("[Genesis] RunFrame waiting frame={} guard={} pc=${:06x} cycles={} masterClock={}{}{}{}{}{}{}{}{}{}{}{}{}",
					frame,
					guard,
					_cpu->GetState().PC & 0x00ffffff,
					_cpu->GetState().CycleCount,
					_memoryManager->GetMasterClock(),
					waitIncludeHeartbeat ? std::format(" heartbeatPc=${:06x} heartbeatCycles={} heartbeatInstr={}", ioState.CpuProgramCounterHeartbeat, ioState.CpuCycleHeartbeat, ioState.CpuInstructionHeartbeat) : "",
					waitIncludeZ80Runtime ? std::format(" z80Running={} z80RunnableCycles={} z80StalledCycles={} z80Transitions={} z80Epoch={} z80LastTransitionClock={}", _memoryManager->GetZ80RuntimeRunning() ? 1 : 0, _memoryManager->GetZ80RuntimeRunnableCycles(), _memoryManager->GetZ80RuntimeStalledCycles(), _memoryManager->GetZ80RuntimeTransitionCount(), _memoryManager->GetZ80RuntimeStateEpoch(), _memoryManager->GetZ80RuntimeLastTransitionClock()) : "",
					waitIncludeVdpState ? std::format(" vdpVc={} vdpHc={} vdpStatus=${:04x} r1=${:02x} dmaActive={} dmaMode={}", vdpState.VCounter, vdpState.HCounter, vdpState.StatusRegister, vdpState.Registers[1], vdpState.DmaActive ? 1 : 0, vdpState.DmaMode) : "",
					waitIncludeCpuLoop ? std::format(" cpuLoop={}", _cpu->BuildSamePcLoopSummary()) : "",
					waitIncludeCpuAddr ? std::format(" cpuAddr={}", _cpu->BuildAddressErrorSummary()) : "",
					waitIncludeMmuOpWindow ? std::format(" mmuOpsWindow={}", _memoryManager->BuildRuntimeOpTraceWindow(waitMmuOpWindowLines)) : "",
					waitIncludeCpuTrace ? std::format(" cpuTrace={}", _cpu->BuildInstructionTraceWindow(waitCpuTraceLines)) : "",
					waitIncludeStartup ? std::format(" startup={}", BuildGenesisStartupRuntimeSummary(_memoryManager.get())) : "",
					waitIncludeMmuFlow ? std::format(" mmuFlow={}", _memoryManager->BuildRuntimeFlowTraceSummary()) : "",
					waitIncludeMmuOps ? std::format(" mmuOps={}", _memoryManager->BuildRuntimeOpTraceSummary()) : "",
					waitIncludeCpuBoundary ? std::format(" cpuBoundary={}", _cpu->BuildDispatchBoundaryProbeSummary()) : "",
					waitIncludeTraceDigest ? std::format(" traceDigest={}", _cpu->BuildInstructionTraceDigest()) : ""));
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
		GenesisVdpState sehVdpState = _vdp->GetState();
		GenesisIoState sehIoState = _memoryManager->GetIoState();
		string sehExtras;
		if (sehIncludeHeartbeat) {
			sehExtras += std::format(" heartbeatPc=${:06x} heartbeatCycles={} heartbeatInstr={}", sehIoState.CpuProgramCounterHeartbeat, sehIoState.CpuCycleHeartbeat, sehIoState.CpuInstructionHeartbeat);
		}
		if (sehIncludeZ80Runtime) {
			sehExtras += std::format(" z80Running={} z80RunnableCycles={} z80StalledCycles={} z80Transitions={}", _memoryManager->GetZ80RuntimeRunning() ? 1 : 0, _memoryManager->GetZ80RuntimeRunnableCycles(), _memoryManager->GetZ80RuntimeStalledCycles(), _memoryManager->GetZ80RuntimeTransitionCount());
		}
		if (sehIncludeVdpState) {
			sehExtras += std::format(" vdpVc={} vdpHc={} vdpStatus=${:04x}", sehVdpState.VCounter, sehVdpState.HCounter, sehVdpState.StatusRegister);
		}
		if (sehIncludeTraceDigest) {
			sehExtras += std::format(" traceDigest={}", _cpu->BuildInstructionTraceDigest());
		}
		_runFrameLastExitSummary = std::format("seh_guard_abort code=${:08x} frame={} guard={} pc=${:06x} cycles={}{}{}{}{}{}{}{}{}{}",
			trappedSehCode,
			frame,
			guard,
			_cpu->GetState().PC & 0x00ffffff,
			_cpu->GetState().CycleCount,
			sehIncludeCpuBoundary ? std::format(" cpuBoundary={}", _runFrameFirstFailureBoundarySummary) : "",
			sehIncludeMmuFlow ? std::format(" mmuFlow={}", _memoryManager->BuildRuntimeFlowTraceSummary()) : "",
			sehIncludeMmuOps ? std::format(" mmuOps={}", _memoryManager->BuildRuntimeOpTraceSummary()) : "",
			sehIncludeMmuOpWindow ? std::format(" mmuOpsWindow={}", _memoryManager->BuildRuntimeOpTraceWindow(sehMmuOpLines)) : "",
			sehIncludeCpuTrace ? std::format(" cpuTrace={}", _cpu->BuildInstructionTraceWindow(sehCpuTraceLines)) : "",
			sehIncludeCpuLoop ? std::format(" cpuLoop={}", _cpu->BuildSamePcLoopSummary()) : "",
			sehIncludeCpuAddr ? std::format(" cpuAddr={}", _cpu->BuildAddressErrorSummary()) : "",
			sehIncludeStartup ? std::format(" startup={}", BuildGenesisStartupRuntimeSummary(_memoryManager.get())) : "",
			sehExtras);
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
		for (uint32_t i = 0; i < fallbackPulseIterations && nextFrame == frame; i++) {
			_cpu->ForceClockAdvance(forcedAdvancePulseCycles);
			nextFrame = _vdp->GetFrameCount();
			_runFrameForcedAdvanceCount++;
		}
	}
	if (frameAdvanceLogEnabled && (nextFrame % frameAdvanceLogModulo) == 0) {
		GenesisVdpState vdpState = _vdp->GetState();
		GenesisIoState ioState = _memoryManager->GetIoState();
		MessageManager::Log(std::format("[Genesis] Frame advanced to {} (deltaClock={}) display={} R1=${:02x} tmssUnlocked={}{}{}{}{}{}{}{}{}{}{}{}{}",
			nextFrame,
			_memoryManager->GetMasterClock() - startClock,
			(vdpState.Registers[1] & 0x40) ? "on" : "off",
			vdpState.Registers[1],
			_memoryManager->GetIoState().TmssUnlocked ? "true" : "false",
			frameAdvanceIncludeCpuTrace ? std::format(" cpuTrace={}", _cpu->BuildInstructionTraceWindow(frameAdvanceCpuTraceLines)) : "",
			frameAdvanceIncludeMmuOpWindow ? std::format(" mmuOpsWindow={}", _memoryManager->BuildRuntimeOpTraceWindow(frameAdvanceMmuOpLines)) : "",
			frameAdvanceIncludeCpuLoop ? std::format(" cpuLoop={}", _cpu->BuildSamePcLoopSummary()) : "",
			frameAdvanceIncludeCpuAddr ? std::format(" cpuAddr={}", _cpu->BuildAddressErrorSummary()) : "",
			frameAdvanceIncludeHeartbeat ? std::format(" heartbeatPc=${:06x} heartbeatCycles={} heartbeatInstr={}", ioState.CpuProgramCounterHeartbeat, ioState.CpuCycleHeartbeat, ioState.CpuInstructionHeartbeat) : "",
			frameAdvanceIncludeZ80Runtime ? std::format(" z80Running={} z80RunnableCycles={} z80StalledCycles={} z80Transitions={}", _memoryManager->GetZ80RuntimeRunning() ? 1 : 0, _memoryManager->GetZ80RuntimeRunnableCycles(), _memoryManager->GetZ80RuntimeStalledCycles(), _memoryManager->GetZ80RuntimeTransitionCount()) : "",
			frameAdvanceIncludeVdpState ? std::format(" vdpVc={} vdpHc={} vdpStatus=${:04x}", vdpState.VCounter, vdpState.HCounter, vdpState.StatusRegister) : "",
			frameAdvanceIncludeStartup ? std::format(" startup={}", BuildGenesisStartupRuntimeSummary(_memoryManager.get())) : "",
			frameAdvanceIncludeMmuFlow ? std::format(" mmuFlow={}", _memoryManager->BuildRuntimeFlowTraceSummary()) : "",
			frameAdvanceIncludeMmuOps ? std::format(" mmuOps={}", _memoryManager->BuildRuntimeOpTraceSummary()) : "",
			frameAdvanceIncludeCpuBoundary ? std::format(" cpuBoundary={}", _cpu->BuildDispatchBoundaryProbeSummary()) : "",
			frameAdvanceIncludeTraceDigest ? std::format(" traceDigest={}", _cpu->BuildInstructionTraceDigest()) : ""));
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
	AudioTrackInfo info = {};
	info.GameTitle = "Sega Genesis";
	info.TrackNumber = 1;
	info.TrackCount = 1;
	if (!_memoryManager || !_vdp) {
		info.SongTitle = "audio unavailable";
		info.Comment = "missing memory or video subsystem";
		return info;
	}

	uint8_t fmKeyOnMask = _memoryManager->GetYmKeyOnMask();
	int fmActiveChannels = 0;
	for (int i = 0; i < 6; i++) {
		fmActiveChannels += (fmKeyOnMask & (1u << i)) ? 1 : 0;
	}

	int psgActiveChannels = 0;
	if (_psg) {
		const GenesisPsgState& psg = _psg->GetState();
		for (int i = 0; i < 4; i++) {
			if (psg.Channels[i].Volume < 0x0F) {
				psgActiveChannels++;
			}
		}
	}

	info.SongTitle = std::format("FM {} /6 + PSG {} /4 active", fmActiveChannels, psgActiveChannels);
	info.Comment = std::format("ymBusy={} ymStatus=${:02x} ymKeyOnMask=${:02x} ymLastKeyOn=${:02x}",
		_memoryManager->GetYmBusy() ? 1 : 0,
		_memoryManager->GetYmStatusFlags() & 0x03,
		fmKeyOnMask,
		_memoryManager->GetYmLastKeyOnValue());
	info.Position = _vdp->GetFrameCount() / GetFps();
	info.Length = 0;
	info.FadeLength = 0;
	return info;
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
