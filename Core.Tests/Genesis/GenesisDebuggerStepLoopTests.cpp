#include "pch.h"
#include "Debugger/DebugTypes.h"
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
		std::vector<int32_t> StepCount;
		std::vector<uint32_t> ProgramCounter;
		std::vector<uint64_t> CycleCount;
		std::vector<uint64_t> InstructionHeartbeat;
		BreakType FinalBreakType = BreakType::None;
		BreakSource FinalBreakSource = BreakSource::Unspecified;

		bool operator==(const StepLoopSnapshot&) const = default;
	};

	StepLoopSnapshot RunDebuggerStepLoop(uint32_t initialSp, uint32_t initialPc, int32_t stepBudget) {
		std::vector<uint8_t> romData = BuildNopBootRom(initialSp, initialPc);
		VirtualFile rom(romData.data(), romData.size(), "genesis-debug-step.md");
		Emulator emu;
		GenesisConsole console(&emu);
		StepLoopSnapshot snapshot = {};

		if (console.LoadRom(rom) != LoadRomResult::Success || !console.GetCpu() || !console.GetMemoryManager()) {
			return snapshot;
		}

		StepRequest stepRequest(StepType::Step);
		stepRequest.StepCount = stepBudget;
		auto* cpu = console.GetCpu();
		auto* memoryManager = console.GetMemoryManager();

		for (int32_t i = 0; i < stepBudget; i++) {
			stepRequest.ProcessCpuExec();
			cpu->Exec();

			GenesisM68kState cpuState = cpu->GetState();
			GenesisIoState ioState = memoryManager->GetIoState();
			snapshot.StepCount.push_back(stepRequest.StepCount);
			snapshot.ProgramCounter.push_back(cpuState.PC);
			snapshot.CycleCount.push_back(cpuState.CycleCount);
			snapshot.InstructionHeartbeat.push_back(ioState.CpuInstructionHeartbeat);
		}

		snapshot.FinalBreakType = stepRequest.BreakNeeded;
		snapshot.FinalBreakSource = stepRequest.GetBreakSource();
		return snapshot;
	}
}

TEST(GenesisDebuggerStepLoopTests, StepRequestProgressesDeterministicallyAcrossSteppedLoop) {
	constexpr uint32_t InitialSp = 0x00FFFE00;
	constexpr uint32_t InitialPc = 0x00000100;
	constexpr int32_t StepBudget = 12;

	StepLoopSnapshot snapshot = RunDebuggerStepLoop(InitialSp, InitialPc, StepBudget);
	ASSERT_EQ((int32_t)snapshot.StepCount.size(), StepBudget);
	ASSERT_EQ((int32_t)snapshot.ProgramCounter.size(), StepBudget);
	ASSERT_EQ((int32_t)snapshot.CycleCount.size(), StepBudget);
	ASSERT_EQ((int32_t)snapshot.InstructionHeartbeat.size(), StepBudget);

	for (int32_t i = 0; i < StepBudget; i++) {
		EXPECT_EQ(snapshot.StepCount[i], StepBudget - (i + 1));
		EXPECT_EQ(snapshot.ProgramCounter[i], InitialPc + (uint32_t)((i + 1) * 2));
	}

	for (int32_t i = 1; i < StepBudget; i++) {
		EXPECT_GT(snapshot.CycleCount[i], snapshot.CycleCount[i - 1]);
		EXPECT_EQ(snapshot.InstructionHeartbeat[i], snapshot.InstructionHeartbeat[i - 1] + 1);
	}

	EXPECT_EQ(snapshot.FinalBreakType, BreakType::User);
	EXPECT_EQ(snapshot.FinalBreakSource, BreakSource::CpuStep);
}

TEST(GenesisDebuggerStepLoopTests, StepLoopSequenceIsDeterministicAcrossIndependentRuns) {
	constexpr uint32_t InitialSp = 0x00FFFE00;
	constexpr uint32_t InitialPc = 0x00000100;
	constexpr int32_t StepBudget = 24;

	StepLoopSnapshot runA = RunDebuggerStepLoop(InitialSp, InitialPc, StepBudget);
	StepLoopSnapshot runB = RunDebuggerStepLoop(InitialSp, InitialPc, StepBudget);
	EXPECT_EQ(runA, runB);
}

TEST(GenesisDebuggerStepLoopTests, StepRequestRunStateClearsStepBudgetsAndBreakFlags) {
	StepRequest stepRequest(StepType::Step);
	stepRequest.StepCount = 5;
	stepRequest.CpuCycleStepCount = 11;
	stepRequest.SetBreakSource(BreakSource::CpuStep, true);

	ASSERT_EQ(stepRequest.BreakNeeded, BreakType::User);
	ASSERT_EQ(stepRequest.GetBreakSource(), BreakSource::CpuStep);

	stepRequest.StepCount = -1;
	stepRequest.CpuCycleStepCount = -1;
	stepRequest.BreakNeeded = BreakType::None;
	stepRequest.Source = BreakSource::Unspecified;
	stepRequest.ExSource = BreakSource::Unspecified;

	EXPECT_EQ(stepRequest.StepCount, -1);
	EXPECT_EQ(stepRequest.CpuCycleStepCount, -1);
	EXPECT_EQ(stepRequest.BreakNeeded, BreakType::None);
	EXPECT_EQ(stepRequest.GetBreakSource(), BreakSource::Unspecified);
}
