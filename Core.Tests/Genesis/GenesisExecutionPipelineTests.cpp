#include "pch.h"
#include "Debugger/DebugTypes.h"
#include "Genesis/GenesisConsole.h"
#include "Genesis/GenesisM68k.h"
#include "Genesis/GenesisMemoryManager.h"
#include "Genesis/GenesisVdp.h"
#include "Shared/BaseControlManager.h"
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
	constexpr uint32_t InitialPc = 0x00000200;
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
	uint32_t expectedHeartbeatPc = InitialPc;

	for (int32_t i = 0; i < StepBudget; i++) {
		stepRequest.ProcessCpuExec();
		console.GetCpu()->Exec();
		GenesisIoState ioStep = console.GetMemoryManager()->GetIoState();
		expectedHeartbeatPc = InitialPc + (uint32_t)(i * 2);
		EXPECT_EQ(ioStep.CpuProgramCounterHeartbeat, expectedHeartbeatPc);
	}

	GenesisIoState ioAfter = console.GetMemoryManager()->GetIoState();
	EXPECT_EQ(stepRequest.StepCount, 0);
	EXPECT_EQ(stepRequest.BreakNeeded, BreakType::User);
	EXPECT_EQ(stepRequest.GetBreakSource(), BreakSource::CpuStep);
	EXPECT_EQ(ioAfter.CpuInstructionHeartbeat, ioBefore.CpuInstructionHeartbeat + (uint64_t)StepBudget);
	EXPECT_EQ(ioAfter.CpuProgramCounterHeartbeat, expectedHeartbeatPc);
}

TEST(GenesisExecutionPipelineTests, ContinuousRunFrameCadenceStaysMonotonicAndDeterministicAcrossRuns) {
	constexpr uint32_t InitialSp = 0x00fffe00;
	constexpr uint32_t InitialPc = 0x00000100;
	constexpr int FrameCount = 3;

	auto runCadenceCapture = [&]() {
		std::vector<uint8_t> romData = BuildGenesisNopBootRom(InitialSp, InitialPc);
		VirtualFile rom(romData.data(), romData.size(), "genesis-pipeline-cadence.bin");
		Emulator emu;
		GenesisConsole console(&emu);
		if (console.LoadRom(rom) != LoadRomResult::Success || !console.GetVdp() || !console.GetCpu() || !console.GetMemoryManager()) {
			return std::tuple<std::vector<uint32_t>, std::vector<uint64_t>, std::vector<uint64_t>, std::vector<uint64_t>>();
		}

		std::vector<uint32_t> frameCounts;
		std::vector<uint64_t> cpuCycles;
		std::vector<uint64_t> cycleHeartbeat;
		std::vector<uint64_t> instructionHeartbeat;
		frameCounts.reserve(FrameCount);
		cpuCycles.reserve(FrameCount);
		cycleHeartbeat.reserve(FrameCount);
		instructionHeartbeat.reserve(FrameCount);

		for (int i = 0; i < FrameCount; i++) {
			console.RunFrame();
			frameCounts.push_back(console.GetVdp()->GetFrameCount());
			cpuCycles.push_back(console.GetCpu()->GetState().CycleCount);
			GenesisIoState ioState = console.GetMemoryManager()->GetIoState();
			cycleHeartbeat.push_back(ioState.CpuCycleHeartbeat);
			instructionHeartbeat.push_back(ioState.CpuInstructionHeartbeat);
		}

		return std::make_tuple(frameCounts, cpuCycles, cycleHeartbeat, instructionHeartbeat);
	};

	auto runA = runCadenceCapture();
	auto runB = runCadenceCapture();

	auto& frameA = std::get<0>(runA);
	auto& cyclesA = std::get<1>(runA);
	auto& cycleHeartbeatA = std::get<2>(runA);
	auto& instructionHeartbeatA = std::get<3>(runA);

	ASSERT_EQ((int)frameA.size(), FrameCount);
	ASSERT_EQ((int)cyclesA.size(), FrameCount);
	ASSERT_EQ((int)cycleHeartbeatA.size(), FrameCount);
	ASSERT_EQ((int)instructionHeartbeatA.size(), FrameCount);

	for (int i = 1; i < FrameCount; i++) {
		EXPECT_GT(frameA[i], frameA[i - 1]);
		EXPECT_GT(cyclesA[i], cyclesA[i - 1]);
		EXPECT_GT(cycleHeartbeatA[i], cycleHeartbeatA[i - 1]);
		EXPECT_GT(instructionHeartbeatA[i], instructionHeartbeatA[i - 1]);
	}

	EXPECT_EQ(runA, runB);
}

TEST(GenesisExecutionPipelineTests, RunFrameAdvancesExactlyOneFramePerInvocationDeterministically) {
	constexpr uint32_t InitialSp = 0x00fffe00;
	constexpr uint32_t InitialPc = 0x00000100;
	constexpr int FrameCount = 5;

	auto captureRun = [&]() {
		std::vector<uint8_t> romData = BuildGenesisNopBootRom(InitialSp, InitialPc);
		VirtualFile rom(romData.data(), romData.size(), "genesis-pipeline-single-step-frame.bin");
		Emulator emu;
		GenesisConsole console(&emu);
		if (console.LoadRom(rom) != LoadRomResult::Success || !console.GetVdp() || !console.GetMemoryManager()) {
			return std::tuple<std::vector<uint32_t>, std::vector<uint64_t>>();
		}

		std::vector<uint32_t> frameDeltas;
		std::vector<uint64_t> instructionDeltas;
		frameDeltas.reserve(FrameCount);
		instructionDeltas.reserve(FrameCount);

		for (int i = 0; i < FrameCount; i++) {
			uint32_t frameBefore = console.GetVdp()->GetFrameCount();
			GenesisIoState ioBefore = console.GetMemoryManager()->GetIoState();

			console.RunFrame();

			uint32_t frameAfter = console.GetVdp()->GetFrameCount();
			GenesisIoState ioAfter = console.GetMemoryManager()->GetIoState();
			frameDeltas.push_back(frameAfter - frameBefore);
			instructionDeltas.push_back(ioAfter.CpuInstructionHeartbeat - ioBefore.CpuInstructionHeartbeat);
		}

		return std::make_tuple(frameDeltas, instructionDeltas);
	};

	auto runA = captureRun();
	auto runB = captureRun();

	auto& frameDeltasA = std::get<0>(runA);
	auto& instructionDeltasA = std::get<1>(runA);

	ASSERT_EQ((int)frameDeltasA.size(), FrameCount);
	ASSERT_EQ((int)instructionDeltasA.size(), FrameCount);

	for (int i = 0; i < FrameCount; i++) {
		EXPECT_EQ(frameDeltasA[i], 1u);
		EXPECT_GT(instructionDeltasA[i], 0u);
	}

	EXPECT_EQ(runA, runB);
}

TEST(GenesisExecutionPipelineTests, RunFrameAdvancesPollAndLagCountersDeterministicallyInHeadlessCadence) {
	constexpr uint32_t InitialSp = 0x00fffe00;
	constexpr uint32_t InitialPc = 0x00000100;
	constexpr int FrameCount = 4;

	auto captureRun = [&]() {
		std::vector<uint8_t> romData = BuildGenesisNopBootRom(InitialSp, InitialPc);
		VirtualFile rom(romData.data(), romData.size(), "genesis-pipeline-poll-lag-cadence.bin");
		Emulator emu;
		GenesisConsole console(&emu);
		if (console.LoadRom(rom) != LoadRomResult::Success || !console.GetControlManager()) {
			return std::tuple<std::vector<uint32_t>, std::vector<uint32_t>>();
		}

		BaseControlManager* controlManager = console.GetControlManager();
		std::vector<uint32_t> pollDeltas;
		std::vector<uint32_t> lagDeltas;
		pollDeltas.reserve(FrameCount);
		lagDeltas.reserve(FrameCount);

		for (int i = 0; i < FrameCount; i++) {
			uint32_t pollBefore = controlManager->GetPollCounter();
			uint32_t lagBefore = controlManager->GetLagCounter();

			console.RunFrame();

			uint32_t pollAfter = controlManager->GetPollCounter();
			uint32_t lagAfter = controlManager->GetLagCounter();
			pollDeltas.push_back(pollAfter - pollBefore);
			lagDeltas.push_back(lagAfter - lagBefore);
		}

		return std::make_tuple(pollDeltas, lagDeltas);
	};

	auto runA = captureRun();
	auto runB = captureRun();

	auto& pollDeltasA = std::get<0>(runA);
	auto& lagDeltasA = std::get<1>(runA);

	ASSERT_EQ((int)pollDeltasA.size(), FrameCount);
	ASSERT_EQ((int)lagDeltasA.size(), FrameCount);

	for (int i = 0; i < FrameCount; i++) {
		EXPECT_EQ(pollDeltasA[i], 1u);
		EXPECT_EQ(lagDeltasA[i], 1u);
	}

	EXPECT_EQ(runA, runB);
}
