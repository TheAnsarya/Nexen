#include "pch.h"
#include "Debugger/DebugTypes.h"
#include "Genesis/GenesisConsole.h"
#include "Genesis/GenesisM68k.h"
#include "Genesis/GenesisMemoryManager.h"
#include "Genesis/GenesisVdp.h"
#include "Shared/Emulator.h"
#include "Utilities/VirtualFile.h"

namespace {
	std::vector<uint8_t> BuildGenesisNopBootRom(uint32_t initialSp, uint32_t initialPc, size_t romSize = 0x40000) {
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
}

TEST(GenesisExecutionPipelineTests, LoadRomSeedsCpuFromResetVectors) {
	constexpr uint32_t InitialSp = 0x00fffe00;
	constexpr uint32_t InitialPc = 0x00000100;
	std::vector<uint8_t> romData = BuildGenesisNopBootRom(InitialSp, InitialPc);
	VirtualFile rom(romData.data(), romData.size(), "genesis-pipeline-load.bin");
	Emulator emu;
	GenesisConsole console(&emu);

	ASSERT_EQ(console.LoadRom(rom), LoadRomResult::Success);
	ASSERT_NE(console.GetCpu(), nullptr);
	ASSERT_NE(console.GetMemoryManager(), nullptr);

	GenesisM68kState cpuState = console.GetCpu()->GetState();
	EXPECT_EQ(cpuState.PC & 0x00ffffff, InitialPc);
	EXPECT_EQ(cpuState.A[7] & 0x00ffffff, InitialSp);

	GenesisIoState ioState = console.GetMemoryManager()->GetIoState();
	EXPECT_EQ(ioState.CpuInstructionHeartbeat, 0u);
	EXPECT_EQ(ioState.CpuProgramCounterHeartbeat, 0u);
}

TEST(GenesisExecutionPipelineTests, RunFrameAdvancesCodeExecutionAndHeartbeat) {
	constexpr uint32_t InitialSp = 0x00fffe00;
	constexpr uint32_t InitialPc = 0x00000100;
	std::vector<uint8_t> romData = BuildGenesisNopBootRom(InitialSp, InitialPc);
	VirtualFile rom(romData.data(), romData.size(), "genesis-pipeline-run.bin");
	Emulator emu;
	GenesisConsole console(&emu);

	ASSERT_EQ(console.LoadRom(rom), LoadRomResult::Success);
	ASSERT_NE(console.GetCpu(), nullptr);
	ASSERT_NE(console.GetVdp(), nullptr);
	ASSERT_NE(console.GetMemoryManager(), nullptr);

	uint32_t frameBefore = console.GetVdp()->GetFrameCount();
	GenesisM68kState cpuBefore = console.GetCpu()->GetState();
	GenesisIoState ioBefore = console.GetMemoryManager()->GetIoState();

	console.RunFrame();

	uint32_t frameAfter = console.GetVdp()->GetFrameCount();
	GenesisM68kState cpuAfter = console.GetCpu()->GetState();
	GenesisIoState ioAfter = console.GetMemoryManager()->GetIoState();

	EXPECT_GT(frameAfter, frameBefore);
	EXPECT_GT(cpuAfter.CycleCount, cpuBefore.CycleCount);
	EXPECT_GT(ioAfter.CpuInstructionHeartbeat, ioBefore.CpuInstructionHeartbeat);
	EXPECT_NE(ioAfter.CpuProgramCounterHeartbeat, ioBefore.CpuProgramCounterHeartbeat);
}

TEST(GenesisExecutionPipelineTests, StepRequestProgressTracksSteppedCpuExecution) {
	constexpr uint32_t InitialSp = 0x00fffe00;
	constexpr uint32_t InitialPc = 0x00000100;
	constexpr int32_t StepBudget = 8;
	std::vector<uint8_t> romData = BuildGenesisNopBootRom(InitialSp, InitialPc);
	VirtualFile rom(romData.data(), romData.size(), "genesis-pipeline-step.bin");
	Emulator emu;
	GenesisConsole console(&emu);

	ASSERT_EQ(console.LoadRom(rom), LoadRomResult::Success);
	ASSERT_NE(console.GetCpu(), nullptr);
	ASSERT_NE(console.GetMemoryManager(), nullptr);

	StepRequest stepRequest(StepType::Step);
	stepRequest.StepCount = StepBudget;
	GenesisIoState ioBefore = console.GetMemoryManager()->GetIoState();

	for (int32_t i = 0; i < StepBudget; i++) {
		stepRequest.ProcessCpuExec();
		console.GetCpu()->Exec();
	}

	GenesisIoState ioAfter = console.GetMemoryManager()->GetIoState();
	EXPECT_EQ(stepRequest.StepCount, 0);
	EXPECT_EQ(stepRequest.BreakNeeded, BreakType::User);
	EXPECT_EQ(stepRequest.GetBreakSource(), BreakSource::CpuStep);
	EXPECT_EQ(ioAfter.CpuInstructionHeartbeat, ioBefore.CpuInstructionHeartbeat + (uint64_t)StepBudget);
}
