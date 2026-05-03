#include "pch.h"
#include "Genesis/GenesisConsole.h"
#include "Genesis/GenesisM68k.h"
#include "Genesis/GenesisMemoryManager.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"
#include "Utilities/VirtualFile.h"

namespace {
	std::vector<uint8_t> BuildNopBootRom(uint32_t initialSp, uint32_t initialPc, size_t romSize = 0x2000, bool includeSegaHeader = false) {
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
		if (includeSegaHeader && rom.size() >= 0x104) {
			rom[0x100] = 'S';
			rom[0x101] = 'E';
			rom[0x102] = 'G';
			rom[0x103] = 'A';
		}

		return rom;
	}

	std::vector<uint8_t> EncodeSmdFromLinear(const std::vector<uint8_t>& linearRom) {
		std::vector<uint8_t> smd(0x200 + linearRom.size(), 0);
		const size_t payloadSize = linearRom.size();
		const size_t fullBlockBytes = payloadSize & ~((size_t)0x3fff);

		for (size_t block = 0; block < fullBlockBytes; block += 0x4000) {
			size_t srcBase = block;
			size_t dstBase = 0x200 + block;
			for (size_t i = 0; i < 0x2000; i++) {
				smd[dstBase + i] = linearRom[srcBase + (i << 1) + 1];
				smd[dstBase + 0x2000 + i] = linearRom[srcBase + (i << 1)];
			}
		}

		if (fullBlockBytes < payloadSize) {
			memcpy(smd.data() + 0x200 + fullBlockBytes, linearRom.data() + fullBlockBytes, payloadSize - fullBlockBytes);
		}

		return smd;
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
	EXPECT_EQ(ioAfter.CpuProgramCounterHeartbeat, after.PC - 2);
}

TEST(GenesisExecutionStartupTests, TasOnDataRegisterSetsBitSevenAndLogicalFlags) {
	constexpr uint32_t InitialSp = 0x00FFFE00;
	constexpr uint32_t InitialPc = 0x00000200;
	std::vector<uint8_t> romData = BuildNopBootRom(InitialSp, InitialPc);

	romData[InitialPc + 0] = 0x70; // moveq #0,d0
	romData[InitialPc + 1] = 0x00;
	romData[InitialPc + 2] = 0x4A; // tas d0
	romData[InitialPc + 3] = 0xC0;

	VirtualFile rom(romData.data(), romData.size(), "boot-tas-d0.md");
	Emulator emu;
	GenesisConsole console(&emu);

	ASSERT_EQ(console.LoadRom(rom), LoadRomResult::Success);
	auto* cpu = console.GetCpu();
	ASSERT_NE(cpu, nullptr);

	cpu->Exec();
	cpu->Exec();

	GenesisM68kState state = cpu->GetState();
	EXPECT_EQ(state.D[0], 0x00000080u);
	EXPECT_EQ(state.PC, InitialPc + 4);
	EXPECT_EQ(state.SR & M68kFlags::Negative, 0u);
	EXPECT_EQ(state.SR & M68kFlags::Zero, M68kFlags::Zero);
	EXPECT_EQ(state.SR & M68kFlags::Overflow, 0u);
	EXPECT_EQ(state.SR & M68kFlags::Carry, 0u);
}

TEST(GenesisExecutionStartupTests, TasOnAbsoluteLongMemoryUpdatesTargetByte) {
	constexpr uint32_t InitialSp = 0x00FFFE00;
	constexpr uint32_t InitialPc = 0x00000200;
	constexpr uint32_t TargetAddress = 0x00FF0000;
	std::vector<uint8_t> romData = BuildNopBootRom(InitialSp, InitialPc);

	romData[InitialPc + 0] = 0x4A; // tas $00ff0000.l
	romData[InitialPc + 1] = 0xF9;
	romData[InitialPc + 2] = 0x00;
	romData[InitialPc + 3] = 0xFF;
	romData[InitialPc + 4] = 0x00;
	romData[InitialPc + 5] = 0x00;

	VirtualFile rom(romData.data(), romData.size(), "boot-tas-mem.md");
	Emulator emu;
	GenesisConsole console(&emu);

	ASSERT_EQ(console.LoadRom(rom), LoadRomResult::Success);
	auto* cpu = console.GetCpu();
	auto* memoryManager = console.GetMemoryManager();
	ASSERT_NE(cpu, nullptr);
	ASSERT_NE(memoryManager, nullptr);

	memoryManager->Write8(TargetAddress, 0x00);
	cpu->Exec();

	GenesisM68kState state = cpu->GetState();
	EXPECT_EQ(memoryManager->Read8(TargetAddress), 0x80u);
	EXPECT_EQ(state.PC, InitialPc + 6);
	EXPECT_EQ(state.SR & M68kFlags::Zero, M68kFlags::Zero);
	EXPECT_EQ(state.SR & M68kFlags::Negative, 0u);
	EXPECT_EQ(state.SR & M68kFlags::Overflow, 0u);
	EXPECT_EQ(state.SR & M68kFlags::Carry, 0u);
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
		EXPECT_EQ(snapshot.ProgramCounterHeartbeat[i], InitialPc + ((uint32_t)i * 2));
	}

	for (int i = 1; i < StepCount; i++) {
		EXPECT_GT(snapshot.CycleHeartbeat[i], snapshot.CycleHeartbeat[i - 1]);
		EXPECT_EQ(snapshot.InstructionHeartbeat[i], snapshot.InstructionHeartbeat[i - 1] + 1);
	}

	EXPECT_EQ(snapshot.FinalPc, InitialPc + ((uint32_t)StepCount * 2));
	EXPECT_EQ(snapshot.ProgramCounterHeartbeat.back(), snapshot.FinalPc - 2);
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

TEST(GenesisExecutionStartupTests, ResetClearsExecutionHeartbeatsBeforeNextInstruction) {
	constexpr uint32_t InitialSp = 0x00FFFE00;
	constexpr uint32_t InitialPc = 0x00000100;
	std::vector<uint8_t> romData = BuildNopBootRom(InitialSp, InitialPc);

	VirtualFile rom(romData.data(), romData.size(), "boot-reset-heartbeat.md");
	Emulator emu;
	GenesisConsole console(&emu);

	ASSERT_EQ(console.LoadRom(rom), LoadRomResult::Success);
	auto* cpu = console.GetCpu();
	auto* memoryManager = console.GetMemoryManager();
	ASSERT_NE(cpu, nullptr);
	ASSERT_NE(memoryManager, nullptr);

	for (int i = 0; i < 6; i++) {
		cpu->Exec();
	}

	GenesisIoState ioBeforeReset = memoryManager->GetIoState();
	EXPECT_GT(ioBeforeReset.CpuInstructionHeartbeat, 0u);
	EXPECT_GT(ioBeforeReset.CpuCycleHeartbeat, 0u);

	console.Reset();

	GenesisIoState ioAfterReset = memoryManager->GetIoState();
	EXPECT_EQ(ioAfterReset.CpuProgramCounterHeartbeat, 0u);
	EXPECT_EQ(ioAfterReset.CpuCycleHeartbeat, 0u);
	EXPECT_EQ(ioAfterReset.CpuInstructionHeartbeat, 0u);
	EXPECT_EQ(ioAfterReset.RomReadHeartbeat, 0u);
}

TEST(GenesisExecutionStartupTests, ResetHeartbeatClearingIsDeterministicAcrossRuns) {
	constexpr uint32_t InitialSp = 0x00FFFE00;
	constexpr uint32_t InitialPc = 0x00000100;

	auto captureAfterReset = [&]() {
		std::vector<uint8_t> romData = BuildNopBootRom(InitialSp, InitialPc);
		VirtualFile rom(romData.data(), romData.size(), "boot-reset-heartbeat-determinism.md");
		Emulator emu;
		GenesisConsole console(&emu);
		if (console.LoadRom(rom) != LoadRomResult::Success || !console.GetCpu() || !console.GetMemoryManager()) {
			return std::tuple<uint32_t, uint64_t, uint64_t, uint64_t>(0, 0, 0, 0);
		}

		auto* cpu = console.GetCpu();
		auto* memoryManager = console.GetMemoryManager();
		for (int i = 0; i < 6; i++) {
			cpu->Exec();
		}

		console.Reset();
		GenesisIoState io = memoryManager->GetIoState();
		return std::make_tuple(io.CpuProgramCounterHeartbeat, io.CpuCycleHeartbeat, io.CpuInstructionHeartbeat, io.RomReadHeartbeat);
	};

	auto runA = captureAfterReset();
	auto runB = captureAfterReset();

	EXPECT_EQ(std::get<0>(runA), 0u);
	EXPECT_EQ(std::get<1>(runA), 0u);
	EXPECT_EQ(std::get<2>(runA), 0u);
	EXPECT_EQ(std::get<3>(runA), 0u);
	EXPECT_EQ(runA, runB);
}

TEST(GenesisExecutionStartupTests, SoftResetClearsDebugTranscriptLaneStateDeterministically) {
	constexpr uint32_t InitialSp = 0x00FFFE00;
	constexpr uint32_t InitialPc = 0x00000100;

	auto captureAfterSoftReset = [&]() {
		std::vector<uint8_t> romData = BuildNopBootRom(InitialSp, InitialPc);
		VirtualFile rom(romData.data(), romData.size(), "boot-reset-debug-lane.md");
		Emulator emu;
		GenesisConsole console(&emu);
		if (console.LoadRom(rom) != LoadRomResult::Success || !console.GetMemoryManager()) {
			return std::tuple<uint32_t, uint64_t, uint32_t, uint8_t, uint8_t, uint8_t>(0, 0, 0, 0, 0, 0);
		}

		auto* memoryManager = console.GetMemoryManager();
		memoryManager->DebugWrite8(0xA10009, 0x12);
		memoryManager->DebugWrite8(0xA00000, 0x34);
		memoryManager->DebugWrite8(0xA11100, 0x01);

		GenesisIoState ioBeforeReset = memoryManager->GetIoState();
		EXPECT_GT(ioBeforeReset.DebugTranscriptLaneCount, 0u);
		EXPECT_NE(ioBeforeReset.DebugTranscriptLaneDigest, 0ull);

		console.Reset();

		GenesisIoState ioAfterReset = memoryManager->GetIoState();
		return std::make_tuple(
			ioAfterReset.DebugTranscriptLaneCount,
			ioAfterReset.DebugTranscriptLaneDigest,
			ioAfterReset.DebugTranscriptEntryAddress[0],
			ioAfterReset.DebugTranscriptEntryValue[0],
			ioAfterReset.DebugTranscriptEntryFlags[0],
			ioAfterReset.TmssEnabled);
	};

	auto runA = captureAfterSoftReset();
	auto runB = captureAfterSoftReset();

	EXPECT_EQ(std::get<0>(runA), 0u);
	EXPECT_EQ(std::get<1>(runA), 0ull);
	EXPECT_EQ(std::get<2>(runA), 0u);
	EXPECT_EQ(std::get<3>(runA), 0u);
	EXPECT_EQ(std::get<4>(runA), 0u);
	EXPECT_EQ(runA, runB);
}

TEST(GenesisExecutionStartupTests, SoftResetClearsToolingTelemetryCountersDeterministically) {
	constexpr uint32_t InitialSp = 0x00FFFE00;
	constexpr uint32_t InitialPc = 0x00000100;

	auto captureAfterSoftReset = [&]() {
		std::vector<uint8_t> romData = BuildNopBootRom(InitialSp, InitialPc);
		VirtualFile rom(romData.data(), romData.size(), "boot-reset-tooling-telemetry.md");
		Emulator emu;
		GenesisConsole console(&emu);
		if (console.LoadRom(rom) != LoadRomResult::Success || !console.GetMemoryManager()) {
			return std::tuple<uint16_t, uint16_t, uint8_t, uint8_t>(0, 0, 0, 0);
		}

		auto* memoryManager = console.GetMemoryManager();
		memoryManager->Write8(0xA12012, 0x11);
		memoryManager->Write8(0xA12013, 0x22);
		memoryManager->Write8(0xA12014, 0x33);
		memoryManager->Write8(0xA12015, 0x44);
		memoryManager->Write8(0xA15008, 0x55);
		memoryManager->Write8(0xA15009, 0x66);
		memoryManager->Write8(0xA1500A, 0x77);
		memoryManager->Write8(0xA1500B, 0x88);

		uint16_t segaCdEventCountBefore = (uint16_t)memoryManager->Read8(0xA12016)
			| ((uint16_t)memoryManager->Read8(0xA12017) << 8);
		uint16_t m32xEventCountBefore = (uint16_t)memoryManager->Read8(0xA15018)
			| ((uint16_t)memoryManager->Read8(0xA15019) << 8);
		EXPECT_GT(segaCdEventCountBefore, 0u);
		EXPECT_GT(m32xEventCountBefore, 0u);

		console.Reset();

		uint16_t segaCdEventCountAfter = (uint16_t)memoryManager->Read8(0xA12016)
			| ((uint16_t)memoryManager->Read8(0xA12017) << 8);
		uint16_t m32xEventCountAfter = (uint16_t)memoryManager->Read8(0xA15018)
			| ((uint16_t)memoryManager->Read8(0xA15019) << 8);
		uint8_t segaCdDigestAfter = memoryManager->Read8(0xA1201B);
		uint8_t m32xDigestAfter = memoryManager->Read8(0xA1501F);
		return std::make_tuple(segaCdEventCountAfter, m32xEventCountAfter, segaCdDigestAfter, m32xDigestAfter);
	};

	auto runA = captureAfterSoftReset();
	auto runB = captureAfterSoftReset();

	EXPECT_EQ(std::get<0>(runA), 0u);
	EXPECT_EQ(std::get<1>(runA), 0u);
	EXPECT_EQ(std::get<2>(runA), 0u);
	EXPECT_EQ(std::get<3>(runA), 0u);
	EXPECT_EQ(runA, runB);
}

TEST(GenesisExecutionStartupTests, LoadRomDecodesSmdExtensionToLinearResetVectors) {
	constexpr uint32_t InitialSp = 0x00FFFE00;
	constexpr uint32_t InitialPc = 0x00000100;
	std::vector<uint8_t> linearRom = BuildNopBootRom(InitialSp, InitialPc, 0x4000, true);
	std::vector<uint8_t> smdRom = EncodeSmdFromLinear(linearRom);

	VirtualFile rom(smdRom.data(), smdRom.size(), "boot-smd.smd");
	Emulator emu;
	GenesisConsole console(&emu);

	ASSERT_EQ(console.LoadRom(rom), LoadRomResult::Success);
	ASSERT_NE(console.GetCpu(), nullptr);
	GenesisM68kState state = console.GetCpu()->GetState();
	EXPECT_EQ(state.PC & 0x00ffffff, InitialPc);
	EXPECT_EQ(state.A[7] & 0x00ffffff, InitialSp);
}

TEST(GenesisExecutionStartupTests, LoadRomHeuristicallyDecodesSmdLayoutWithoutSmdExtension) {
	constexpr uint32_t InitialSp = 0x00FFFE00;
	constexpr uint32_t InitialPc = 0x00000100;
	std::vector<uint8_t> linearRom = BuildNopBootRom(InitialSp, InitialPc, 0x4000, true);
	std::vector<uint8_t> smdRom = EncodeSmdFromLinear(linearRom);

	VirtualFile rom(smdRom.data(), smdRom.size(), "boot-heuristic.bin");
	Emulator emu;
	GenesisConsole console(&emu);

	ASSERT_EQ(console.LoadRom(rom), LoadRomResult::Success);
	ASSERT_NE(console.GetCpu(), nullptr);
	GenesisM68kState state = console.GetCpu()->GetState();
	EXPECT_EQ(state.PC & 0x00ffffff, InitialPc);
	EXPECT_EQ(state.A[7] & 0x00ffffff, InitialSp);
}

TEST(GenesisExecutionStartupTests, ResetReSyncsTmssEnabledFromRuntimeSettingsDeterministically) {
	constexpr uint32_t InitialSp = 0x00FFFE00;
	constexpr uint32_t InitialPc = 0x00000100;
	std::vector<uint8_t> romData = BuildNopBootRom(InitialSp, InitialPc, 0x4000, true);

	VirtualFile rom(romData.data(), romData.size(), "boot-tmss-reset-sync.md");
	Emulator emu;
	emu.GetSettings()->GetGenesisConfig().EnableTmss = false;
	GenesisConsole console(&emu);

	ASSERT_EQ(console.LoadRom(rom), LoadRomResult::Success);
	ASSERT_NE(console.GetMemoryManager(), nullptr);

	GenesisIoState initialIo = console.GetMemoryManager()->GetIoState();
	EXPECT_EQ(initialIo.TmssEnabled, 0u);
	EXPECT_EQ(initialIo.TmssUnlocked, 0u);

	emu.GetSettings()->GetGenesisConfig().EnableTmss = true;
	console.Reset();
	GenesisIoState tmssEnabledIo = console.GetMemoryManager()->GetIoState();
	EXPECT_EQ(tmssEnabledIo.TmssEnabled, 1u);
	EXPECT_EQ(tmssEnabledIo.TmssUnlocked, 0u);

	emu.GetSettings()->GetGenesisConfig().EnableTmss = false;
	console.Reset();
	GenesisIoState tmssDisabledIo = console.GetMemoryManager()->GetIoState();
	EXPECT_EQ(tmssDisabledIo.TmssEnabled, 0u);
	EXPECT_EQ(tmssDisabledIo.TmssUnlocked, 0u);
}
