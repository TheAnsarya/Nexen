#include "pch.h"
#include "Genesis/GenesisConsole.h"
#include "Genesis/GenesisM68k.h"
#include "Genesis/GenesisMemoryManager.h"
#include "Genesis/GenesisSmokeHarness.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"
#include "Utilities/VirtualFile.h"

namespace {
	std::vector<uint8_t> BuildTraceBootRom(uint32_t initialSp, uint32_t initialPc, size_t romSize = 0x40000) {
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

	bool LoadTraceConsole(Emulator& emu, GenesisConsole& console, const char* fileName, uint32_t sp, uint32_t pc) {
		std::vector<uint8_t> romData = BuildTraceBootRom(sp, pc);
		VirtualFile rom(romData.data(), romData.size(), fileName);
		return console.LoadRom(rom) == LoadRomResult::Success;
	}

	std::tuple<std::string, std::vector<GenesisInstructionTraceEntry>, GenesisIoState> RunTraceDigestCapture(int instructionCount, uint32_t capacity) {
		constexpr uint32_t InitialSp = 0x00fffe00;
		constexpr uint32_t InitialPc = 0x00000100;

		Emulator emu;
		GenesisConsole console(&emu);
		if (!LoadTraceConsole(emu, console, "genesis-trace-digest.bin", InitialSp, InitialPc) || !console.GetCpu() || !console.GetMemoryManager()) {
			return {};
		}

		GenesisM68k* cpu = console.GetCpu();
		cpu->SetInstructionTraceEnabled(true);
		cpu->SetInstructionTraceCapacity(capacity);
		cpu->ClearInstructionTrace();

		for (int i = 0; i < instructionCount; i++) {
			cpu->Exec();
		}

		return {cpu->BuildInstructionTraceDigest(), cpu->GetInstructionTraceSnapshot(), console.GetMemoryManager()->GetIoState()};
	}
}

TEST(GenesisExecutionTraceAndStallRecoveryTests, TraceRowsCaptureProgramCounterAndOpcodeFlow) {
	constexpr uint32_t InitialSp = 0x00fffe00;
	constexpr uint32_t InitialPc = 0x00000100;
	constexpr int InstructionCount = 64;

	Emulator emu;
	GenesisConsole console(&emu);
	ASSERT_TRUE(LoadTraceConsole(emu, console, "genesis-trace-flow.bin", InitialSp, InitialPc));
	ASSERT_NE(console.GetCpu(), nullptr);
	ASSERT_NE(console.GetMemoryManager(), nullptr);

	GenesisM68k* cpu = console.GetCpu();
	cpu->SetInstructionTraceEnabled(true);
	cpu->SetInstructionTraceCapacity(512);
	cpu->ClearInstructionTrace();

	for (int i = 0; i < InstructionCount; i++) {
		cpu->Exec();
	}

	std::vector<GenesisInstructionTraceEntry> snapshot = cpu->GetInstructionTraceSnapshot();
	ASSERT_EQ((int)snapshot.size(), InstructionCount);

	for (int i = 0; i < InstructionCount; i++) {
		EXPECT_EQ(snapshot[i].Opcode, 0x4e71u);
		EXPECT_EQ(snapshot[i].ProgramCounterBefore, InitialPc + (uint32_t)(i * 2));
		EXPECT_EQ(snapshot[i].ProgramCounterAfter, InitialPc + (uint32_t)((i + 1) * 2));
		EXPECT_GE(snapshot[i].InstructionCycleDelta, 4u);
		EXPECT_EQ(snapshot[i].ForcedCycleFloor, 0u);
	}

	GenesisIoState ioState = console.GetMemoryManager()->GetIoState();
	EXPECT_EQ(ioState.CpuInstructionHeartbeat, (uint64_t)InstructionCount);
	EXPECT_EQ(ioState.CpuProgramCounterHeartbeat, InitialPc + (uint32_t)((InstructionCount - 1) * 2));
}

TEST(GenesisExecutionTraceAndStallRecoveryTests, TraceDigestIsDeterministicAcrossIdenticalRuns) {
	auto runA = RunTraceDigestCapture(96, 1024);
	auto runB = RunTraceDigestCapture(96, 1024);

	const std::string& digestA = std::get<0>(runA);
	const std::string& digestB = std::get<0>(runB);
	const std::vector<GenesisInstructionTraceEntry>& snapshotA = std::get<1>(runA);
	const std::vector<GenesisInstructionTraceEntry>& snapshotB = std::get<1>(runB);

	ASSERT_FALSE(digestA.empty());
	ASSERT_FALSE(digestB.empty());
	ASSERT_EQ(snapshotA.size(), snapshotB.size());
	EXPECT_EQ(digestA, digestB);

	for (size_t i = 0; i < snapshotA.size(); i++) {
		EXPECT_EQ(snapshotA[i].ProgramCounterBefore, snapshotB[i].ProgramCounterBefore);
		EXPECT_EQ(snapshotA[i].ProgramCounterAfter, snapshotB[i].ProgramCounterAfter);
		EXPECT_EQ(snapshotA[i].Opcode, snapshotB[i].Opcode);
		EXPECT_EQ(snapshotA[i].InstructionCycleDelta, snapshotB[i].InstructionCycleDelta);
	}
}

TEST(GenesisExecutionTraceAndStallRecoveryTests, TraceRingBufferWrapRetainsMostRecentRows) {
	constexpr uint32_t InitialSp = 0x00fffe00;
	constexpr uint32_t InitialPc = 0x00000100;
	constexpr int InstructionCount = 200;
	constexpr uint32_t Capacity = 64;

	Emulator emu;
	GenesisConsole console(&emu);
	ASSERT_TRUE(LoadTraceConsole(emu, console, "genesis-trace-wrap.bin", InitialSp, InitialPc));
	ASSERT_NE(console.GetCpu(), nullptr);

	GenesisM68k* cpu = console.GetCpu();
	cpu->SetInstructionTraceEnabled(true);
	cpu->SetInstructionTraceCapacity(Capacity);
	cpu->ClearInstructionTrace();

	for (int i = 0; i < InstructionCount; i++) {
		cpu->Exec();
	}

	std::vector<GenesisInstructionTraceEntry> snapshot = cpu->GetInstructionTraceSnapshot();
	ASSERT_EQ(snapshot.size(), Capacity);

	uint32_t expectedFirstPc = InitialPc + (uint32_t)((InstructionCount - (int)Capacity) * 2);
	for (uint32_t i = 0; i < Capacity; i++) {
		EXPECT_EQ(snapshot[i].ProgramCounterBefore, expectedFirstPc + (i * 2));
		EXPECT_EQ(snapshot[i].Opcode, 0x4e71u);
	}
}

TEST(GenesisExecutionTraceAndStallRecoveryTests, TraceDigestChangesWithDifferentInstructionCounts) {
	auto runA = RunTraceDigestCapture(32, 512);
	auto runB = RunTraceDigestCapture(128, 512);

	const std::string& digestA = std::get<0>(runA);
	const std::string& digestB = std::get<0>(runB);

	ASSERT_FALSE(digestA.empty());
	ASSERT_FALSE(digestB.empty());
	EXPECT_NE(digestA, digestB);
}

TEST(GenesisExecutionTraceAndStallRecoveryTests, TraceSnapshotCanBeClearedAndReused) {
	constexpr uint32_t InitialSp = 0x00fffe00;
	constexpr uint32_t InitialPc = 0x00000100;

	Emulator emu;
	GenesisConsole console(&emu);
	ASSERT_TRUE(LoadTraceConsole(emu, console, "genesis-trace-clear.bin", InitialSp, InitialPc));
	ASSERT_NE(console.GetCpu(), nullptr);

	GenesisM68k* cpu = console.GetCpu();
	cpu->SetInstructionTraceEnabled(true);
	cpu->SetInstructionTraceCapacity(256);
	cpu->ClearInstructionTrace();

	for (int i = 0; i < 24; i++) {
		cpu->Exec();
	}
	ASSERT_EQ(cpu->GetInstructionTraceSnapshot().size(), 24u);

	cpu->ClearInstructionTrace();
	EXPECT_EQ(cpu->GetInstructionTraceSnapshot().size(), 0u);

	for (int i = 0; i < 10; i++) {
		cpu->Exec();
	}
	std::vector<GenesisInstructionTraceEntry> snapshot = cpu->GetInstructionTraceSnapshot();
	ASSERT_EQ(snapshot.size(), 10u);
	EXPECT_EQ(snapshot.front().ProgramCounterBefore, InitialPc + 48u);
}

TEST(GenesisExecutionTraceAndStallRecoveryTests, RunFrameStallWatchdogForcesAdvanceWhenCpuClockStops) {
	constexpr uint32_t InitialSp = 0x00fffe00;
	constexpr uint32_t InitialPc = 0x00000100;

	Emulator emu;
	GenesisConsole console(&emu);
	ASSERT_TRUE(LoadTraceConsole(emu, console, "genesis-runframe-stall-watchdog.bin", InitialSp, InitialPc));
	ASSERT_NE(console.GetCpu(), nullptr);
	ASSERT_NE(console.GetVdp(), nullptr);

	GenesisM68k* cpu = console.GetCpu();
	cpu->SetInstructionTraceEnabled(true);
	cpu->SetInstructionTraceCapacity(2048);
	cpu->ClearInstructionTrace();
	cpu->SetDebugForceNoCycleProgress(true);

	uint32_t frameBefore = console.GetVdp()->GetFrameCount();
	console.RunFrame();
	uint32_t frameAfter = console.GetVdp()->GetFrameCount();

	cpu->SetDebugForceNoCycleProgress(false);

	EXPECT_GT(frameAfter, frameBefore);
	EXPECT_GT(console.GetRunFrameStallEventCount(), 0u);
	EXPECT_GT(console.GetRunFrameForcedAdvanceCount(), 0u);
	EXPECT_FALSE(console.GetRunFrameLastStallSummary().empty());
	EXPECT_FALSE(cpu->BuildInstructionTraceDigest().empty());
}

TEST(GenesisExecutionTraceAndStallRecoveryTests, StallSummaryIncludesTraceDigestAndLastProgramCounter) {
	constexpr uint32_t InitialSp = 0x00fffe00;
	constexpr uint32_t InitialPc = 0x00000100;

	Emulator emu;
	GenesisConsole console(&emu);
	ASSERT_TRUE(LoadTraceConsole(emu, console, "genesis-stall-summary.bin", InitialSp, InitialPc));
	ASSERT_NE(console.GetCpu(), nullptr);

	GenesisM68k* cpu = console.GetCpu();
	cpu->SetInstructionTraceEnabled(true);
	cpu->SetInstructionTraceCapacity(1024);
	for (int i = 0; i < 40; i++) {
		cpu->Exec();
	}

	std::string summary = cpu->BuildExecutionStallSummary();
	EXPECT_NE(summary.find("digest="), std::string::npos);
	EXPECT_NE(summary.find("lastPc="), std::string::npos);
	EXPECT_NE(summary.find("lastOpcode="), std::string::npos);
}

TEST(GenesisExecutionTraceAndStallRecoveryTests, ForcedClockAdvanceCounterIncrementsWhenInvoked) {
	constexpr uint32_t InitialSp = 0x00fffe00;
	constexpr uint32_t InitialPc = 0x00000100;

	Emulator emu;
	GenesisConsole console(&emu);
	ASSERT_TRUE(LoadTraceConsole(emu, console, "genesis-force-clock-advance.bin", InitialSp, InitialPc));
	ASSERT_NE(console.GetCpu(), nullptr);

	GenesisM68k* cpu = console.GetCpu();
	uint64_t countBefore = cpu->GetForcedClockAdvanceCount();
	uint64_t cyclesBefore = cpu->GetState().CycleCount;

	cpu->ForceClockAdvance(512);

	EXPECT_EQ(cpu->GetForcedClockAdvanceCount(), countBefore + 1u);
	EXPECT_EQ(cpu->GetState().CycleCount, cyclesBefore + 512u);
}

TEST(GenesisExecutionTraceAndStallRecoveryTests, StartupDeterminismGateAndResilienceGateDigestsAreStableAcrossRuns) {
	GenesisM68kBoundaryScaffold scaffold;
	std::vector<GenesisCompatibilityRomCase> romSet;
	romSet.push_back({"Sonic The Hedgehog (USA)", BuildTraceBootRom(0x00fffe00, 0x00000100)});
	romSet.push_back({"Jurassic Park (USA)", BuildTraceBootRom(0x00fffe00, 0x00000100)});

	GenesisStartupDeterminismGateResult detA = GenesisSmokeHarness::RunStartupDeterminismGate(scaffold, romSet, 600);
	GenesisStartupDeterminismGateResult detB = GenesisSmokeHarness::RunStartupDeterminismGate(scaffold, romSet, 600);
	GenesisExecutionResilienceGateResult resA = GenesisSmokeHarness::RunExecutionResilienceGate(scaffold, romSet, 600);
	GenesisExecutionResilienceGateResult resB = GenesisSmokeHarness::RunExecutionResilienceGate(scaffold, romSet, 600);

	ASSERT_FALSE(detA.Digest.empty());
	ASSERT_FALSE(detB.Digest.empty());
	ASSERT_FALSE(resA.Digest.empty());
	ASSERT_FALSE(resB.Digest.empty());
	EXPECT_EQ(detA.Digest, detB.Digest);
	EXPECT_EQ(resA.Digest, resB.Digest);
}

TEST(GenesisExecutionTraceAndStallRecoveryTests, ExecutionResilienceGateProducesPerCaseOutputLinesAndSummary) {
	GenesisM68kBoundaryScaffold scaffold;
	std::vector<GenesisCompatibilityRomCase> romSet;
	romSet.push_back({"Sonic Spinball (USA)", BuildTraceBootRom(0x00fffe00, 0x00000100)});
	romSet.push_back({"Golden Axe (USA)", BuildTraceBootRom(0x00fffe00, 0x00000100)});

	GenesisExecutionResilienceGateResult result = GenesisSmokeHarness::RunExecutionResilienceGate(scaffold, romSet, 600);
	ASSERT_EQ((int)result.Entries.size(), (int)romSet.size());
	ASSERT_FALSE(result.OutputLines.empty());
	ASSERT_FALSE(result.Digest.empty());

	for (const GenesisExecutionResilienceGateEntry& entry : result.Entries) {
		EXPECT_FALSE(entry.Name.empty());
		EXPECT_FALSE(entry.TitleClass.empty());
		EXPECT_FALSE(entry.TraceDigest.empty());
		EXPECT_GT(entry.FrameCount, 0u);
	}

	bool hasSummary = false;
	for (const std::string& line : result.OutputLines) {
		if (line.find("GEN_RESILIENCE_GATE_SUMMARY") != std::string::npos) {
			hasSummary = true;
			break;
		}
	}
	EXPECT_TRUE(hasSummary);
}

TEST(GenesisExecutionTraceAndStallRecoveryTests, RunFrameFallbackMaintainsHeartbeatMonotonicityUnderClockStallInjection) {
	constexpr uint32_t InitialSp = 0x00fffe00;
	constexpr uint32_t InitialPc = 0x00000100;
	constexpr int FrameCount = 3;

	Emulator emu;
	GenesisConsole console(&emu);
	ASSERT_TRUE(LoadTraceConsole(emu, console, "genesis-runframe-fallback-heartbeat.bin", InitialSp, InitialPc));
	ASSERT_NE(console.GetCpu(), nullptr);
	ASSERT_NE(console.GetMemoryManager(), nullptr);

	GenesisM68k* cpu = console.GetCpu();
	cpu->SetInstructionTraceEnabled(true);
	cpu->SetInstructionTraceCapacity(4096);
	cpu->ClearInstructionTrace();
	cpu->SetDebugForceNoCycleProgress(true);

	std::vector<uint32_t> frameCounts;
	std::vector<uint64_t> instructionHeartbeats;
	std::vector<uint64_t> cycleHeartbeats;
	frameCounts.reserve(FrameCount);
	instructionHeartbeats.reserve(FrameCount);
	cycleHeartbeats.reserve(FrameCount);

	for (int i = 0; i < FrameCount; i++) {
		console.RunFrame();
		frameCounts.push_back(console.GetVdp()->GetFrameCount());
		GenesisIoState ioState = console.GetMemoryManager()->GetIoState();
		instructionHeartbeats.push_back(ioState.CpuInstructionHeartbeat);
		cycleHeartbeats.push_back(ioState.CpuCycleHeartbeat);
	}

	cpu->SetDebugForceNoCycleProgress(false);

	for (int i = 1; i < FrameCount; i++) {
		EXPECT_GT(frameCounts[i], frameCounts[i - 1]);
		EXPECT_GT(instructionHeartbeats[i], instructionHeartbeats[i - 1]);
		EXPECT_GT(cycleHeartbeats[i], cycleHeartbeats[i - 1]);
	}
	EXPECT_GT(console.GetRunFrameForcedAdvanceCount(), 0u);
}

TEST(GenesisExecutionTraceAndStallRecoveryTests, TraceDigestReflectsInterruptLatchAndStatusTransitions) {
	constexpr uint32_t InitialSp = 0x00fffe00;
	constexpr uint32_t InitialPc = 0x00000100;

	Emulator emu;
	GenesisConsole console(&emu);
	ASSERT_TRUE(LoadTraceConsole(emu, console, "genesis-trace-interrupt-latch.bin", InitialSp, InitialPc));
	ASSERT_NE(console.GetCpu(), nullptr);

	GenesisM68k* cpu = console.GetCpu();
	cpu->SetInstructionTraceEnabled(true);
	cpu->SetInstructionTraceCapacity(512);
	cpu->ClearInstructionTrace();

	for (int i = 0; i < 16; i++) {
		cpu->Exec();
	}
	std::string digestBeforeIrq = cpu->BuildInstructionTraceDigest();

	cpu->SetInterrupt(6);
	for (int i = 0; i < 16; i++) {
		cpu->Exec();
	}
	std::string digestAfterIrq = cpu->BuildInstructionTraceDigest();
	std::vector<GenesisInstructionTraceEntry> snapshot = cpu->GetInstructionTraceSnapshot();

	ASSERT_FALSE(digestBeforeIrq.empty());
	ASSERT_FALSE(digestAfterIrq.empty());
	EXPECT_NE(digestBeforeIrq, digestAfterIrq);

	bool sawLatchedInterruptTrace = false;
	for (const GenesisInstructionTraceEntry& entry : snapshot) {
		if (entry.InterruptLatched != 0) {
			sawLatchedInterruptTrace = true;
			break;
		}
	}
	EXPECT_TRUE(sawLatchedInterruptTrace);
}
