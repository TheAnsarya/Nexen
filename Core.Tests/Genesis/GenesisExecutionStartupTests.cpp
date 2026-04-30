#include "pch.h"
#include "Genesis/GenesisConsole.h"
#include "Genesis/GenesisM68k.h"
#include "Genesis/GenesisMemoryManager.h"
#include "Shared/Emulator.h"
#include "Utilities/VirtualFile.h"

namespace {
	std::vector<uint8_t> BuildNopBootRom(uint32_t initialSp, uint32_t initialPc, size_t romSize = 0x2000) {
		std::vector<uint8_t> rom(romSize, 0);
		for (size_t i = 0; i + 1 < rom.size(); i += 2) {
			rom[i] = 0x4E;
			rom[i + 1] = 0x71;
		}

		rom[0] = (uint8_t)((initialSp >> 24) & 0xFF);
		rom[1] = (uint8_t)((initialSp >> 16) & 0xFF);
		rom[2] = (uint8_t)((initialSp >> 8) & 0xFF);
		rom[3] = (uint8_t)(initialSp & 0xFF);
		rom[4] = (uint8_t)((initialPc >> 24) & 0xFF);
		rom[5] = (uint8_t)((initialPc >> 16) & 0xFF);
		rom[6] = (uint8_t)((initialPc >> 8) & 0xFF);
		rom[7] = (uint8_t)(initialPc & 0xFF);
		return rom;
	}

	struct StepLoopSnapshot {
		std::vector<uint32_t> ProgramCounterHeartbeat;
		std::vector<uint64_t> CycleHeartbeat;
		std::vector<uint64_t> InstructionHeartbeat;
		uint32_t FinalPc = 0;
		uint64_t FinalCycles = 0;

		bool operator==(const StepLoopSnapshot&) const = default;
	};

	StepLoopSnapshot RunSteppedLoop(uint32_t initialSp, uint32_t initialPc, int stepCount) {
		std::vector<uint8_t> romData = BuildNopBootRom(initialSp, initialPc);
		VirtualFile rom(romData.data(), romData.size(), "boot-step-loop.md");
		Emulator emu;
		GenesisConsole console(&emu);

		StepLoopSnapshot snapshot = {};
		if (console.LoadRom(rom) != LoadRomResult::Success || !console.GetCpu() || !console.GetMemoryManager()) {
			return snapshot;
		}

		auto* cpu = console.GetCpu();
		auto* memoryManager = console.GetMemoryManager();

		for (int i = 0; i < stepCount; i++) {
			cpu->Exec();
			GenesisIoState ioState = memoryManager->GetIoState();
			snapshot.ProgramCounterHeartbeat.push_back(ioState.CpuProgramCounterHeartbeat);
			snapshot.CycleHeartbeat.push_back(ioState.CpuCycleHeartbeat);
			snapshot.InstructionHeartbeat.push_back(ioState.CpuInstructionHeartbeat);
		}

		GenesisM68kState finalState = cpu->GetState();
		snapshot.FinalPc = finalState.PC;
		snapshot.FinalCycles = finalState.CycleCount;
		return snapshot;
	}
}

TEST(GenesisExecutionStartupTests, LoadRomSeedsCpuFromResetVectors) {
	constexpr uint32_t InitialSp = 0x00FFFE00;
	constexpr uint32_t InitialPc = 0x00000100;
	std::vector<uint8_t> romData = BuildNopBootRom(InitialSp, InitialPc);

	VirtualFile rom(romData.data(), romData.size(), "boot-seed.md");
	Emulator emu;
	GenesisConsole console(&emu);

	ASSERT_EQ(console.LoadRom(rom), LoadRomResult::Success);
	ASSERT_NE(console.GetCpu(), nullptr);
	ASSERT_NE(console.GetMemoryManager(), nullptr);

	GenesisM68kState state = console.GetCpu()->GetState();
	GenesisIoState ioState = console.GetMemoryManager()->GetIoState();

	EXPECT_EQ(state.PC, InitialPc);
	EXPECT_EQ(state.SSP, InitialSp);
	EXPECT_EQ(state.A[7], InitialSp);
	EXPECT_GE(ioState.RomReadHeartbeat, 4u);
}

TEST(GenesisExecutionStartupTests, ExecAdvancesPcCyclesAndExecutionHeartbeat) {
	constexpr uint32_t InitialSp = 0x00FFFE00;
	constexpr uint32_t InitialPc = 0x00000100;
	std::vector<uint8_t> romData = BuildNopBootRom(InitialSp, InitialPc);

	VirtualFile rom(romData.data(), romData.size(), "boot-exec.md");
	Emulator emu;
	GenesisConsole console(&emu);

	ASSERT_EQ(console.LoadRom(rom), LoadRomResult::Success);
	auto* cpu = console.GetCpu();
	auto* memoryManager = console.GetMemoryManager();
	ASSERT_NE(cpu, nullptr);
	ASSERT_NE(memoryManager, nullptr);

	GenesisM68kState before = cpu->GetState();
	GenesisIoState ioBefore = memoryManager->GetIoState();
	constexpr int StepCount = 6;

	for (int i = 0; i < StepCount; i++) {
		cpu->Exec();
	}

	GenesisM68kState after = cpu->GetState();
	GenesisIoState ioAfter = memoryManager->GetIoState();

	EXPECT_EQ(after.PC, before.PC + (StepCount * 2));
	EXPECT_GT(after.CycleCount, before.CycleCount);
	EXPECT_GT(ioAfter.RomReadHeartbeat, ioBefore.RomReadHeartbeat);
	EXPECT_GE(ioAfter.CpuInstructionHeartbeat, ioBefore.CpuInstructionHeartbeat + StepCount);
	EXPECT_EQ(ioAfter.CpuCycleHeartbeat, after.CycleCount);
	EXPECT_EQ(ioAfter.CpuProgramCounterHeartbeat, after.PC);
}

TEST(GenesisExecutionStartupTests, SteppedRunLoopHeartbeatProgressesDeterministicallyPerStep) {
	constexpr uint32_t InitialSp = 0x00FFFE00;
	constexpr uint32_t InitialPc = 0x00000100;
	constexpr int StepCount = 10;

	StepLoopSnapshot snapshot = RunSteppedLoop(InitialSp, InitialPc, StepCount);
	ASSERT_EQ((int)snapshot.ProgramCounterHeartbeat.size(), StepCount);
	ASSERT_EQ((int)snapshot.CycleHeartbeat.size(), StepCount);
	ASSERT_EQ((int)snapshot.InstructionHeartbeat.size(), StepCount);

	for (int i = 0; i < StepCount; i++) {
		EXPECT_EQ(snapshot.ProgramCounterHeartbeat[i], InitialPc + ((uint32_t)(i + 1) * 2));
	}

	for (int i = 1; i < StepCount; i++) {
		EXPECT_GT(snapshot.CycleHeartbeat[i], snapshot.CycleHeartbeat[i - 1]);
		EXPECT_EQ(snapshot.InstructionHeartbeat[i], snapshot.InstructionHeartbeat[i - 1] + 1);
	}

	EXPECT_EQ(snapshot.FinalPc, InitialPc + ((uint32_t)StepCount * 2));
	EXPECT_EQ(snapshot.ProgramCounterHeartbeat.back(), snapshot.FinalPc);
	EXPECT_EQ(snapshot.CycleHeartbeat.back(), snapshot.FinalCycles);
}

TEST(GenesisExecutionStartupTests, SteppedRunLoopHeartbeatSequenceIsDeterministicAcrossRuns) {
	constexpr uint32_t InitialSp = 0x00FFFE00;
	constexpr uint32_t InitialPc = 0x00000100;
	constexpr int StepCount = 24;

	StepLoopSnapshot runA = RunSteppedLoop(InitialSp, InitialPc, StepCount);
	StepLoopSnapshot runB = RunSteppedLoop(InitialSp, InitialPc, StepCount);

	EXPECT_EQ(runA, runB);
}
