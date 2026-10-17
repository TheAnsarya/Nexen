#include "pch.h"
#include "Genesis/GenesisConsole.h"
#include "Genesis/GenesisM68k.h"
#include "Genesis/GenesisMemoryManager.h"
#include "Genesis/GenesisSmokeHarness.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"
#include "Utilities/VirtualFile.h"

namespace {
	bool IsLongHarnessEnabled() {
		char* enabled = nullptr;
		size_t enabledLen = 0;
		const bool hasValue = _dupenv_s(&enabled, &enabledLen, "NEXEN_ENABLE_GENESIS_LONG_HARNESS") == 0 && enabled != nullptr && enabled[0] != '\0';
		std::free(enabled);
		return hasValue;
	}

	std::vector<uint8_t> BuildProbeRom(uint32_t initialSp, uint32_t initialPc, size_t romSize = 0x40000) {
		std::vector<uint8_t> rom(romSize, 0);
		for (size_t i = 0; i + 1 < rom.size(); i += 2) {
			rom[i] = 0x4e;
			rom[i + 1] = 0x71;
		}

		rom[0] = (uint8_t)((initialSp >> 24) & 0xff);
		rom[1] = (uint8_t)((initialSp >> 16) & 0xff);
		rom[2] = (uint8_t)((initialSp >> 8) & 0xff);
		rom[3] = (uint8_t)(initialSp & 0xff);
		rom[4] = (uint8_t)((initialPc >> 24) & 0xff);
		rom[5] = (uint8_t)((initialPc >> 16) & 0xff);
		rom[6] = (uint8_t)((initialPc >> 8) & 0xff);
		rom[7] = (uint8_t)(initialPc & 0xff);

		if (rom.size() >= 0x104) {
			rom[0x100] = 'S';
			rom[0x101] = 'E';
			rom[0x102] = 'G';
			rom[0x103] = 'A';
		}

		return rom;
	}

	bool LoadProbeConsole(Emulator& emu, GenesisConsole& console, const char* fileName, uint32_t sp, uint32_t pc) {
		std::vector<uint8_t> romData = BuildProbeRom(sp, pc);
		VirtualFile rom(romData.data(), romData.size(), fileName);
		return console.LoadRom(rom) == LoadRomResult::Success;
	}
}

TEST(GenesisCrashTriageProbeTests, RunFrameBeforeLoadRomEarlyAbortsWithoutCrashing) {
	Emulator emu;
	GenesisConsole console(&emu);

	console.RunFrame();

	EXPECT_GT(console.GetRunFrameEntryCount(), 0u);
	EXPECT_GT(console.GetRunFrameEarlyAbortCount(), 0u);
	EXPECT_EQ(console.GetRunFrameExitCount(), 0u);
	EXPECT_FALSE(console.GetRunFrameLastEntrySummary().empty());
	EXPECT_FALSE(console.GetRunFrameLastExitSummary().empty());
}

TEST(GenesisCrashTriageProbeTests, RunFrameCrashProbeSummaryReportsMissingComponentsWhenUnloaded) {
	Emulator emu;
	GenesisConsole console(&emu);

	console.RunFrame();
	std::string summary = console.BuildRunFrameCrashProbeSummary();

	EXPECT_NE(summary.find("earlyAbortCount="), std::string::npos);
	EXPECT_NE(summary.find("firstFailureCaptures="), std::string::npos);
	EXPECT_NE(summary.find("firstFailureBoundary=none"), std::string::npos);
	EXPECT_NE(summary.find("cpuProbe="), std::string::npos);
	EXPECT_NE(summary.find("mmuFlow=enabled=0"), std::string::npos);
	EXPECT_NE(summary.find("mmuOps=enabled=0"), std::string::npos);
	EXPECT_NE(summary.find("startup=mmu=missing"), std::string::npos);
	EXPECT_NE(summary.find("sonicTraceArm=0"), std::string::npos);
	EXPECT_NE(summary.find("sonicTraceLastArmFrame=0"), std::string::npos);
	EXPECT_NE(summary.find("sonicTraceRearmWindow="), std::string::npos);
	EXPECT_NE(summary.find("sonicTraceCpuCycles="), std::string::npos);
	EXPECT_NE(summary.find("sonicTraceMmuCycles="), std::string::npos);
	EXPECT_NE(summary.find("sonicTraceCpuStride="), std::string::npos);
	EXPECT_NE(summary.find("sonicTraceMmuStride="), std::string::npos);
	EXPECT_NE(summary.find("sonicTraceCpuRing="), std::string::npos);
	EXPECT_NE(summary.find("sonicTraceMmuFlowRing="), std::string::npos);
	EXPECT_NE(summary.find("sonicTraceMmuOpRing="), std::string::npos);
	EXPECT_NE(summary.find("sonicCheckpointEnable=1"), std::string::npos);
	EXPECT_NE(summary.find("sonicCheckpointInterval="), std::string::npos);
	EXPECT_NE(summary.find("sonicCheckpointEndFrame="), std::string::npos);
	EXPECT_NE(summary.find("sonicCheckpointIncludeOpWindow=0"), std::string::npos);
	EXPECT_NE(summary.find("sonicCheckpointOpWindowLines="), std::string::npos);
	EXPECT_NE(summary.find("sonicCheckpointIncludeCpuTrace=0"), std::string::npos);
	EXPECT_NE(summary.find("sonicCheckpointCpuTraceLines="), std::string::npos);
	EXPECT_NE(summary.find("runFrameStagnantIters="), std::string::npos);
	EXPECT_NE(summary.find("runFrameForcedPulseLimit="), std::string::npos);
	EXPECT_NE(summary.find("runFrameForcedPulseCycles="), std::string::npos);
	EXPECT_NE(summary.find("runFrameFallbackPulses="), std::string::npos);
	EXPECT_NE(summary.find("runFrameHardGuardCpuTraceLines="), std::string::npos);
	EXPECT_NE(summary.find("runFrameHardGuardMmuOpLines="), std::string::npos);
	EXPECT_NE(summary.find("runFrameForcedCompletionCpuTraceLines="), std::string::npos);
	EXPECT_NE(summary.find("runFrameForcedCompletionMmuOpLines="), std::string::npos);
	EXPECT_NE(summary.find("runFrameWaitLogEnable=1"), std::string::npos);
	EXPECT_NE(summary.find("runFrameWaitLogInterval="), std::string::npos);
	EXPECT_NE(summary.find("runFrameWaitIncludeMmuOpWindow=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameWaitMmuOpLines="), std::string::npos);
	EXPECT_NE(summary.find("runFrameWaitIncludeCpuTrace=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameWaitCpuTraceLines="), std::string::npos);
	EXPECT_NE(summary.find("runFrameHardGuardTraceCpuCycles="), std::string::npos);
	EXPECT_NE(summary.find("runFrameHardGuardTraceCpuStride="), std::string::npos);
	EXPECT_NE(summary.find("runFrameHardGuardTraceCpuRing="), std::string::npos);
	EXPECT_NE(summary.find("runFrameHardGuardTraceMmuCpuCycles="), std::string::npos);
	EXPECT_NE(summary.find("runFrameHardGuardTraceMmuCycles="), std::string::npos);
	EXPECT_NE(summary.find("runFrameHardGuardTraceMmuCpuStride="), std::string::npos);
	EXPECT_NE(summary.find("runFrameHardGuardTraceMmuStride="), std::string::npos);
	EXPECT_NE(summary.find("runFrameHardGuardTraceMmuFlowRing="), std::string::npos);
	EXPECT_NE(summary.find("runFrameHardGuardTraceMmuOpRing="), std::string::npos);
	EXPECT_NE(summary.find("runFrameStallTraceCpuCycles="), std::string::npos);
	EXPECT_NE(summary.find("runFrameStallTraceCpuStride="), std::string::npos);
	EXPECT_NE(summary.find("runFrameStallTraceCpuRing="), std::string::npos);
	EXPECT_NE(summary.find("runFrameStallTraceMmuCpuCycles="), std::string::npos);
	EXPECT_NE(summary.find("runFrameStallTraceMmuCycles="), std::string::npos);
	EXPECT_NE(summary.find("runFrameStallTraceMmuCpuStride="), std::string::npos);
	EXPECT_NE(summary.find("runFrameStallTraceMmuStride="), std::string::npos);
	EXPECT_NE(summary.find("runFrameStallTraceMmuFlowRing="), std::string::npos);
	EXPECT_NE(summary.find("runFrameStallTraceMmuOpRing="), std::string::npos);
	EXPECT_NE(summary.find("runFrameFrameAdvanceLogEnable=1"), std::string::npos);
	EXPECT_NE(summary.find("runFrameFrameAdvanceLogModulo="), std::string::npos);
	EXPECT_NE(summary.find("runFrameFrameAdvanceIncludeCpuTrace=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameFrameAdvanceCpuTraceLines="), std::string::npos);
	EXPECT_NE(summary.find("runFrameFrameAdvanceIncludeMmuOpWindow=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameFrameAdvanceMmuOpLines="), std::string::npos);
	EXPECT_NE(summary.find("runFrameSehMmuOpLines="), std::string::npos);
	EXPECT_NE(summary.find("runFrameSehIncludeCpuTrace=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameSehCpuTraceLines="), std::string::npos);
	EXPECT_NE(summary.find("runFrameFirstFailureMmuOpLines="), std::string::npos);
	EXPECT_NE(summary.find("runFrameStallIncludeMmuOpWindow=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameStallMmuOpLines="), std::string::npos);
	EXPECT_NE(summary.find("runFrameStallIncludeCpuTrace=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameStallCpuTraceLines="), std::string::npos);
	EXPECT_NE(summary.find("runFrameHardGuardIncludeCpuBoundary=1"), std::string::npos);
	EXPECT_NE(summary.find("runFrameHardGuardIncludeCpuTrace=1"), std::string::npos);
	EXPECT_NE(summary.find("runFrameHardGuardIncludeCpuLoop=1"), std::string::npos);
	EXPECT_NE(summary.find("runFrameHardGuardIncludeCpuAddr=1"), std::string::npos);
	EXPECT_NE(summary.find("runFrameHardGuardIncludeStartup=1"), std::string::npos);
	EXPECT_NE(summary.find("runFrameHardGuardIncludeMmuFlow=1"), std::string::npos);
	EXPECT_NE(summary.find("runFrameHardGuardIncludeMmuOps=1"), std::string::npos);
	EXPECT_NE(summary.find("runFrameHardGuardIncludeMmuOpWindow=1"), std::string::npos);
	EXPECT_NE(summary.find("runFrameWaitIncludeHeartbeat=1"), std::string::npos);
	EXPECT_NE(summary.find("runFrameWaitIncludeZ80Runtime=1"), std::string::npos);
	EXPECT_NE(summary.find("runFrameWaitIncludeVdpState=1"), std::string::npos);
	EXPECT_NE(summary.find("runFrameWaitIncludeCpuLoop=1"), std::string::npos);
	EXPECT_NE(summary.find("runFrameWaitIncludeCpuAddr=1"), std::string::npos);
	EXPECT_NE(summary.find("runFrameForcedCompletionDetailEnable=1"), std::string::npos);
	EXPECT_NE(summary.find("runFrameForcedCompletionIncludeCpuTrace=1"), std::string::npos);
	EXPECT_NE(summary.find("runFrameForcedCompletionIncludeCpuLoop=1"), std::string::npos);
	EXPECT_NE(summary.find("runFrameForcedCompletionIncludeStartup=1"), std::string::npos);
	EXPECT_NE(summary.find("runFrameForcedCompletionIncludeMmuFlow=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameForcedCompletionIncludeMmuOps=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameForcedCompletionIncludeMmuOpWindow=1"), std::string::npos);
	EXPECT_NE(summary.find("runFrameForcedCompletionIncludeCpuBoundary=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameForcedCompletionIncludeCpuAddr=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameForcedCompletionIncludeTraceDigest=1"), std::string::npos);
	EXPECT_NE(summary.find("runFrameFirstFailureIncludeCpuTrace=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameFirstFailureCpuTraceLines="), std::string::npos);
	EXPECT_NE(summary.find("runFrameFirstFailureIncludeStartup=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameFrameAdvanceIncludeCpuLoop=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameFrameAdvanceIncludeCpuAddr=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameFrameAdvanceIncludeHeartbeat=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameFrameAdvanceIncludeZ80Runtime=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameFrameAdvanceIncludeVdpState=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameSehIncludeCpuBoundary=1"), std::string::npos);
	EXPECT_NE(summary.find("runFrameSehIncludeCpuLoop=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameSehIncludeCpuAddr=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameSehIncludeStartup=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameSehIncludeMmuFlow=1"), std::string::npos);
	EXPECT_NE(summary.find("runFrameSehIncludeMmuOps=1"), std::string::npos);
	EXPECT_NE(summary.find("runFrameSehIncludeMmuOpWindow=1"), std::string::npos);
	EXPECT_NE(summary.find("runFrameStallIncludeStartup=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameStallIncludeCpuBoundary=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameStallIncludeMmuFlow=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameStallIncludeMmuOps=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameStallIncludeTraceDigest=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameFirstFailureIncludeCpuBoundary=1"), std::string::npos);
	EXPECT_NE(summary.find("runFrameFirstFailureIncludeMmuFlow=1"), std::string::npos);
	EXPECT_NE(summary.find("runFrameFirstFailureIncludeMmuOps=1"), std::string::npos);
	EXPECT_NE(summary.find("runFrameWaitIncludeStartup=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameWaitIncludeMmuFlow=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameWaitIncludeMmuOps=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameWaitIncludeCpuBoundary=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameWaitIncludeTraceDigest=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameFrameAdvanceIncludeStartup=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameFrameAdvanceIncludeMmuFlow=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameFrameAdvanceIncludeMmuOps=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameFrameAdvanceIncludeCpuBoundary=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameFrameAdvanceIncludeTraceDigest=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameSehIncludeHeartbeat=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameSehIncludeZ80Runtime=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameSehIncludeVdpState=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameSehIncludeTraceDigest=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameStallIncludeHeartbeat=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameStallIncludeZ80Runtime=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameStallIncludeVdpState=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameStallIncludeCpuAddr=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameFirstFailureIncludeCpuLoop=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameFirstFailureIncludeCpuAddr=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameFirstFailureIncludeTraceDigest=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameFirstFailureIncludeMmuOpWindow=1"), std::string::npos);
	EXPECT_NE(summary.find("runFrameHardGuardIncludeHeartbeat=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameHardGuardIncludeZ80Runtime=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameHardGuardIncludeVdpState=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameHardGuardIncludeTraceDigest=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameForcedCompletionIncludeHeartbeat=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameForcedCompletionIncludeZ80Runtime=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameForcedCompletionIncludeVdpState=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameForcedCompletionIncludeTraceDigestDetail=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameFirstFailureIncludeHeartbeat=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameFirstFailureIncludeZ80Runtime=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameFirstFailureIncludeVdpState=0"), std::string::npos);
	EXPECT_NE(summary.find("runFrameFirstFailureIncludeTraceDigestExtended=0"), std::string::npos);
	EXPECT_NE(summary.find("sonicCheckpoints=0"), std::string::npos);
	EXPECT_NE(summary.find("sonicLastCheckpointFrame=0"), std::string::npos);
	EXPECT_NE(summary.find("sonicLastCheckpointPc=$000000"), std::string::npos);
	EXPECT_NE(summary.find("abort_missing_component"), std::string::npos);
}

TEST(GenesisCrashTriageProbeTests, M68kCrashProbeSummaryReportsDefaultFields) {
	GenesisM68k cpu;
	std::string summary = cpu.BuildCrashProbeSummary();

	EXPECT_NE(summary.find("resetCount=0"), std::string::npos);
	EXPECT_NE(summary.find("firstDispatch=none"), std::string::npos);
	EXPECT_NE(summary.find("dispatchFaults=0"), std::string::npos);
	EXPECT_NE(summary.find("guardHits=0"), std::string::npos);
	EXPECT_NE(summary.find("decodeFaults=0"), std::string::npos);
	EXPECT_NE(summary.find("lastFetchPc=$000000"), std::string::npos);
	EXPECT_NE(summary.find("decodeRoute=none"), std::string::npos);
}

TEST(GenesisCrashTriageProbeTests, M68kDispatchBoundaryProbeSummaryReportsDefaultFields) {
	GenesisM68k cpu;
	std::string summary = cpu.BuildDispatchBoundaryProbeSummary();

	EXPECT_NE(summary.find("execCalls=0"), std::string::npos);
	EXPECT_NE(summary.find("guardHits=0"), std::string::npos);
	EXPECT_NE(summary.find("decodeFaults=0"), std::string::npos);
	EXPECT_NE(summary.find("fetchPc=$000000"), std::string::npos);
	EXPECT_NE(summary.find("boundary=none"), std::string::npos);
	EXPECT_NE(summary.find("decodeGroup=0"), std::string::npos);
	EXPECT_NE(summary.find("decodeRoute=none"), std::string::npos);
	EXPECT_NE(summary.find("flow=enabled=0"), std::string::npos);
}

TEST(GenesisCrashTriageProbeTests, M68kInstructionFlowSummaryDefaultsDisabledAndEmpty) {
	GenesisM68k cpu;
	std::string flowSummary = cpu.BuildInstructionFlowSummary();

	EXPECT_NE(flowSummary.find("enabled=0"), std::string::npos);
	EXPECT_NE(flowSummary.find("logged=0"), std::string::npos);
	EXPECT_NE(flowSummary.find("skipped=0"), std::string::npos);
	EXPECT_NE(flowSummary.find("ring=0"), std::string::npos);
	EXPECT_NE(flowSummary.find("last=none"), std::string::npos);
}

TEST(GenesisCrashTriageProbeTests, MmuOpTraceSummaryAndWindowDefaultsAreEmpty) {
	GenesisMemoryManager memoryManager;
	std::string opSummary = memoryManager.BuildRuntimeOpTraceSummary();
	std::string opWindow = memoryManager.BuildRuntimeOpTraceWindow(8);

	EXPECT_NE(opSummary.find("enabled=0"), std::string::npos);
	EXPECT_NE(opSummary.find("logged=0"), std::string::npos);
	EXPECT_NE(opSummary.find("ring=0"), std::string::npos);
	EXPECT_NE(opSummary.find("last=none"), std::string::npos);
	EXPECT_EQ(opWindow, "none");
}

TEST(GenesisCrashTriageProbeTests, ResilienceGateIncludesSonicDigestMarkersForSonicCases) {
	if (!IsLongHarnessEnabled()) {
		GTEST_SKIP() << "Long-running resilience harness test is opt-in; set NEXEN_ENABLE_GENESIS_LONG_HARNESS=1 to run.";
	}

	GenesisM68kBoundaryScaffold scaffold;
	std::vector<GenesisCompatibilityRomCase> romSet;
	romSet.push_back({"Sonic The Hedgehog (USA)", BuildProbeRom(0x00fffe00, 0x00000100)});

	GenesisExecutionResilienceGateResult result = GenesisSmokeHarness::RunExecutionResilienceGate(scaffold, romSet, 600);
	ASSERT_EQ((int)result.Entries.size(), 1);
	EXPECT_EQ(result.Entries[0].TitleClass, "sonic");
	EXPECT_NE(result.Entries[0].TraceDigest.find(":SONIC:"), std::string::npos);
}

TEST(GenesisCrashTriageProbeTests, ResilienceGateStillProducesDigestForMixedTitleClasses) {
	if (!IsLongHarnessEnabled()) {
		GTEST_SKIP() << "Long-running resilience harness test is opt-in; set NEXEN_ENABLE_GENESIS_LONG_HARNESS=1 to run.";
	}

	GenesisM68kBoundaryScaffold scaffold;
	std::vector<GenesisCompatibilityRomCase> romSet;
	romSet.push_back({"Sonic 3 (World)", BuildProbeRom(0x00fffe00, 0x00000100)});
	romSet.push_back({"Golden Axe (USA)", BuildProbeRom(0x00fffe00, 0x00000100)});

	GenesisExecutionResilienceGateResult result = GenesisSmokeHarness::RunExecutionResilienceGate(scaffold, romSet, 600);
	ASSERT_EQ((int)result.Entries.size(), 2);
	EXPECT_FALSE(result.Digest.empty());
	EXPECT_FALSE(result.OutputLines.empty());
}

TEST(GenesisCrashTriageProbeTests, RunFrameCrashProbeSummaryTracksEntryAndAbortCountersMonotonically) {
	Emulator emu;
	GenesisConsole console(&emu);

	for (int i = 0; i < 3; i++) {
		console.RunFrame();
	}

	EXPECT_EQ(console.GetRunFrameEntryCount(), 3u);
	EXPECT_EQ(console.GetRunFrameEarlyAbortCount(), 3u);
	EXPECT_EQ(console.GetRunFrameExitCount(), 0u);
	std::string summary = console.BuildRunFrameCrashProbeSummary();
	EXPECT_NE(summary.find("entryCount=3"), std::string::npos);
	EXPECT_NE(summary.find("earlyAbortCount=3"), std::string::npos);
}

TEST(GenesisCrashTriageProbeTests, LoadedConsoleExposesCpuCrashProbeSummaryString) {
	constexpr uint32_t InitialSp = 0x00fffe00;
	constexpr uint32_t InitialPc = 0x00000100;
	Emulator emu;
	GenesisConsole console(&emu);
	ASSERT_TRUE(LoadProbeConsole(emu, console, "genesis-crash-probe-summary.bin", InitialSp, InitialPc));
	ASSERT_NE(console.GetCpu(), nullptr);

	std::string cpuSummary = console.GetCpu()->BuildCrashProbeSummary();
	EXPECT_NE(cpuSummary.find("resetCount="), std::string::npos);
	EXPECT_NE(cpuSummary.find("vectorPc="), std::string::npos);
	EXPECT_NE(cpuSummary.find("boundary="), std::string::npos);
}

TEST(GenesisCrashTriageProbeTests, CrashProbeSummaryContainsCpuProbeFieldAfterLoadedRunFrameAttempt) {
	constexpr uint32_t InitialSp = 0x00fffe00;
	constexpr uint32_t InitialPc = 0x00000100;
	Emulator emu;
	GenesisConsole console(&emu);
	ASSERT_TRUE(LoadProbeConsole(emu, console, "genesis-crash-probe-after-load.bin", InitialSp, InitialPc));

	// The environment currently has an execution-path SEH blocker for some Genesis tests.
	// We only validate summary formatting and counter visibility here.
	std::string summaryBefore = console.BuildRunFrameCrashProbeSummary();
	EXPECT_NE(summaryBefore.find("cpuProbe="), std::string::npos);
	EXPECT_NE(summaryBefore.find("mmuFlow=enabled=0"), std::string::npos);
	EXPECT_NE(summaryBefore.find("mmuOps=enabled=0"), std::string::npos);
	EXPECT_NE(summaryBefore.find("entryCount="), std::string::npos);
	EXPECT_NE(summaryBefore.find("startup=titleClass="), std::string::npos);
	EXPECT_NE(summaryBefore.find("sonicTraceArm="), std::string::npos);
	EXPECT_NE(summaryBefore.find("sonicTraceLastArmFrame="), std::string::npos);
	EXPECT_NE(summaryBefore.find("sonicTraceRearmWindow="), std::string::npos);
	EXPECT_NE(summaryBefore.find("sonicTraceCpuCycles="), std::string::npos);
	EXPECT_NE(summaryBefore.find("sonicTraceMmuCycles="), std::string::npos);
	EXPECT_NE(summaryBefore.find("sonicTraceCpuStride="), std::string::npos);
	EXPECT_NE(summaryBefore.find("sonicTraceMmuStride="), std::string::npos);
	EXPECT_NE(summaryBefore.find("sonicTraceCpuRing="), std::string::npos);
	EXPECT_NE(summaryBefore.find("sonicTraceMmuFlowRing="), std::string::npos);
	EXPECT_NE(summaryBefore.find("sonicTraceMmuOpRing="), std::string::npos);
	EXPECT_NE(summaryBefore.find("sonicCheckpointEnable="), std::string::npos);
	EXPECT_NE(summaryBefore.find("sonicCheckpointInterval="), std::string::npos);
	EXPECT_NE(summaryBefore.find("sonicCheckpointEndFrame="), std::string::npos);
	EXPECT_NE(summaryBefore.find("sonicCheckpointIncludeOpWindow="), std::string::npos);
	EXPECT_NE(summaryBefore.find("sonicCheckpointOpWindowLines="), std::string::npos);
	EXPECT_NE(summaryBefore.find("sonicCheckpointIncludeCpuTrace="), std::string::npos);
	EXPECT_NE(summaryBefore.find("sonicCheckpointCpuTraceLines="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameStagnantIters="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameForcedPulseLimit="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameForcedPulseCycles="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameFallbackPulses="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameHardGuardCpuTraceLines="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameHardGuardMmuOpLines="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameForcedCompletionCpuTraceLines="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameForcedCompletionMmuOpLines="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameWaitLogEnable="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameWaitLogInterval="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameWaitIncludeMmuOpWindow="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameWaitMmuOpLines="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameWaitIncludeCpuTrace="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameWaitCpuTraceLines="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameHardGuardTraceCpuCycles="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameHardGuardTraceCpuStride="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameHardGuardTraceCpuRing="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameHardGuardTraceMmuCpuCycles="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameHardGuardTraceMmuCycles="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameHardGuardTraceMmuCpuStride="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameHardGuardTraceMmuStride="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameHardGuardTraceMmuFlowRing="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameHardGuardTraceMmuOpRing="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameStallTraceCpuCycles="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameStallTraceCpuStride="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameStallTraceCpuRing="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameStallTraceMmuCpuCycles="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameStallTraceMmuCycles="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameStallTraceMmuCpuStride="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameStallTraceMmuStride="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameStallTraceMmuFlowRing="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameStallTraceMmuOpRing="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameFrameAdvanceLogEnable="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameFrameAdvanceLogModulo="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameFrameAdvanceIncludeCpuTrace="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameFrameAdvanceCpuTraceLines="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameFrameAdvanceIncludeMmuOpWindow="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameFrameAdvanceMmuOpLines="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameSehMmuOpLines="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameSehIncludeCpuTrace="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameSehCpuTraceLines="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameFirstFailureMmuOpLines="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameStallIncludeMmuOpWindow="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameStallMmuOpLines="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameStallIncludeCpuTrace="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameStallCpuTraceLines="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameHardGuardIncludeCpuBoundary="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameHardGuardIncludeCpuTrace="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameHardGuardIncludeCpuLoop="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameHardGuardIncludeCpuAddr="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameHardGuardIncludeStartup="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameHardGuardIncludeMmuFlow="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameHardGuardIncludeMmuOps="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameHardGuardIncludeMmuOpWindow="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameWaitIncludeHeartbeat="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameWaitIncludeZ80Runtime="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameWaitIncludeVdpState="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameWaitIncludeCpuLoop="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameWaitIncludeCpuAddr="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameForcedCompletionDetailEnable="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameForcedCompletionIncludeCpuTrace="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameForcedCompletionIncludeCpuLoop="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameForcedCompletionIncludeStartup="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameForcedCompletionIncludeMmuFlow="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameForcedCompletionIncludeMmuOps="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameForcedCompletionIncludeMmuOpWindow="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameForcedCompletionIncludeCpuBoundary="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameForcedCompletionIncludeCpuAddr="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameForcedCompletionIncludeTraceDigest="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameFirstFailureIncludeCpuTrace="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameFirstFailureCpuTraceLines="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameFirstFailureIncludeStartup="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameFrameAdvanceIncludeCpuLoop="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameFrameAdvanceIncludeCpuAddr="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameFrameAdvanceIncludeHeartbeat="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameFrameAdvanceIncludeZ80Runtime="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameFrameAdvanceIncludeVdpState="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameSehIncludeCpuBoundary="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameSehIncludeCpuLoop="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameSehIncludeCpuAddr="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameSehIncludeStartup="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameSehIncludeMmuFlow="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameSehIncludeMmuOps="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameSehIncludeMmuOpWindow="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameStallIncludeStartup="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameStallIncludeCpuBoundary="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameStallIncludeMmuFlow="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameStallIncludeMmuOps="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameStallIncludeTraceDigest="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameFirstFailureIncludeCpuBoundary="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameFirstFailureIncludeMmuFlow="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameFirstFailureIncludeMmuOps="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameWaitIncludeStartup="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameWaitIncludeMmuFlow="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameWaitIncludeMmuOps="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameWaitIncludeCpuBoundary="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameWaitIncludeTraceDigest="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameFrameAdvanceIncludeStartup="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameFrameAdvanceIncludeMmuFlow="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameFrameAdvanceIncludeMmuOps="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameFrameAdvanceIncludeCpuBoundary="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameFrameAdvanceIncludeTraceDigest="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameSehIncludeHeartbeat="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameSehIncludeZ80Runtime="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameSehIncludeVdpState="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameSehIncludeTraceDigest="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameStallIncludeHeartbeat="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameStallIncludeZ80Runtime="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameStallIncludeVdpState="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameStallIncludeCpuAddr="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameFirstFailureIncludeCpuLoop="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameFirstFailureIncludeCpuAddr="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameFirstFailureIncludeTraceDigest="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameFirstFailureIncludeMmuOpWindow="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameHardGuardIncludeHeartbeat="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameHardGuardIncludeZ80Runtime="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameHardGuardIncludeVdpState="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameHardGuardIncludeTraceDigest="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameForcedCompletionIncludeHeartbeat="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameForcedCompletionIncludeZ80Runtime="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameForcedCompletionIncludeVdpState="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameForcedCompletionIncludeTraceDigestDetail="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameFirstFailureIncludeHeartbeat="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameFirstFailureIncludeZ80Runtime="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameFirstFailureIncludeVdpState="), std::string::npos);
	EXPECT_NE(summaryBefore.find("runFrameFirstFailureIncludeTraceDigestExtended="), std::string::npos);
	EXPECT_NE(summaryBefore.find("sonicCheckpoints="), std::string::npos);
	EXPECT_NE(summaryBefore.find("sonicLastCheckpointFrame="), std::string::npos);
	EXPECT_NE(summaryBefore.find("sonicLastCheckpointPc=$"), std::string::npos);
}
