#include "pch.h"
#include "Genesis/GenesisConsole.h"
#include "Genesis/GenesisM68k.h"
#include "Genesis/GenesisMemoryManager.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"
#include "Utilities/VirtualFile.h"

namespace {
	struct ScopedEnvVar {
		std::string Name;
		bool HadValue = false;
		std::string OldValue;

		ScopedEnvVar(const char* name, const char* value)
			: Name(name) {
			char* existing = nullptr;
			size_t existingLen = 0;
			if (_dupenv_s(&existing, &existingLen, Name.c_str()) == 0 && existing != nullptr) {
				HadValue = true;
				OldValue = existing;
			}
			std::free(existing);

			if (value) {
				_putenv_s(Name.c_str(), value);
			} else {
				_putenv_s(Name.c_str(), "");
			}
		}

		~ScopedEnvVar() {
			if (HadValue) {
				_putenv_s(Name.c_str(), OldValue.c_str());
			} else {
				_putenv_s(Name.c_str(), "");
			}
		}
	};

	struct ScopedStartupEnv {
		std::vector<ScopedEnvVar> Vars;

		ScopedStartupEnv(const std::vector<std::pair<const char*, const char*>>& vars)
			: Vars() {
			Vars.reserve(64);
			ResetTrackedVars();
			for (const auto& [name, value] : vars) {
				Vars.emplace_back(name, value);
			}
		}

		void ResetTrackedVars() {
			Vars.emplace_back("NEXEN_GENESIS_STARTUP_PROFILE", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_STARTUP_WINDOW_FRAMES", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_STARTUP_BOOT_RELAX_FRAMES", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_STARTUP_LOGO_PHASE_END_FRAME", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_STARTUP_STRICT_PHASE_START_FRAME", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_STARTUP_CHECKPOINT_INTERVAL_FRAMES", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_STARTUP_CHECKPOINT_END_FRAME", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_TMSS_STRICT", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_TMSS_UNLOCK_DELAY_MCLK", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_TMSS_STRICT_DURING_LOGO", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_TMSS_FORCE_UNTIL_UNLOCK", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_Z80_BUSREQ_ACK_DELAY_MCLK", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_Z80_BUSRESUME_DELAY_MCLK", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_Z80_EARLY_BUSREQ_ACK_DELAY_MCLK", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_Z80_EARLY_BUSRESUME_DELAY_MCLK", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_Z80_LATE_BUSREQ_ACK_DELAY_MCLK", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_Z80_LATE_BUSRESUME_DELAY_MCLK", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_USE_DYNAMIC_BUS_TIMING", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_Z80_LATCH_HIGH_BYTE_ONLY", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_PREFER_NEXENREF_BUS_HANDOFF", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_PREFER_MESEN_BUS_HANDOFF", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_POWERON_Z80_RESET_ASSERTED", nullptr);
		}
	};

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

TEST(GenesisExecutionStartupTests, VdpStatusReadClearsVBlankInterruptPendingBit) {
	constexpr uint32_t InitialSp = 0x00FFFE00;
	constexpr uint32_t InitialPc = 0x00000200;
	std::vector<uint8_t> romData = BuildNopBootRom(InitialSp, InitialPc);

	VirtualFile rom(romData.data(), romData.size(), "boot-vint-status-clear.md");
	Emulator emu;
	GenesisConsole console(&emu);

	ASSERT_EQ(console.LoadRom(rom), LoadRomResult::Success);
	auto* cpu = console.GetCpu();
	auto* memoryManager = console.GetMemoryManager();
	auto* vdp = console.GetVdp();
	ASSERT_NE(cpu, nullptr);
	ASSERT_NE(memoryManager, nullptr);
	ASSERT_NE(vdp, nullptr);

	// Enable VINT generation while keeping the CPU interrupt mask at level 7 so
	// the pending flag can be observed before any handler-side acknowledge path.
	memoryManager->Write16(0xC00004, 0x8120);
	GenesisM68kState state = cpu->GetState();
	state.SR = (uint16_t)((state.SR & ~M68kFlags::IntMask) | 0x0700);
	cpu->SetState(state);

	bool sawPendingVint = false;
	for (int i = 0; i < 250000; i++) {
		cpu->Exec();
		if ((vdp->GetState().StatusRegister & VdpStatus::VIntPending) != 0) {
			sawPendingVint = true;
			break;
		}
	}
	ASSERT_TRUE(sawPendingVint);

	uint16_t statusBeforeClear = memoryManager->Read16(0xC00004);
	uint16_t statusAfterClear = memoryManager->Read16(0xC00004);

	EXPECT_NE(statusBeforeClear & VdpStatus::VIntPending, 0u);
	EXPECT_EQ(statusAfterClear & VdpStatus::VIntPending, 0u);
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

TEST(GenesisExecutionStartupTests, StartupTmssStrictForcePolicyLocksDataPortsUntilUnlockDelayElapses) {
	ScopedStartupEnv env({
		{ "NEXEN_GENESIS_STARTUP_PROFILE", "strict" },
		{ "NEXEN_GENESIS_TMSS_STRICT", "1" },
		{ "NEXEN_GENESIS_TMSS_FORCE_UNTIL_UNLOCK", "1" },
		{ "NEXEN_GENESIS_TMSS_UNLOCK_DELAY_MCLK", "24" }
	});

	constexpr uint32_t InitialSp = 0x00FFFE00;
	constexpr uint32_t InitialPc = 0x00000100;
	std::vector<uint8_t> romData = BuildNopBootRom(InitialSp, InitialPc, 0x4000, true);
	VirtualFile rom(romData.data(), romData.size(), "boot-tmss-force-lock.md");
	Emulator emu;
	emu.GetSettings()->GetGenesisConfig().EnableTmss = true;
	GenesisConsole console(&emu);

	ASSERT_EQ(console.LoadRom(rom), LoadRomResult::Success);
	ASSERT_NE(console.GetMemoryManager(), nullptr);
	auto* mm = console.GetMemoryManager();

	EXPECT_TRUE(mm->GetTmssEnabled());
	EXPECT_TRUE(mm->GetTmssStrictMode());
	EXPECT_TRUE(mm->GetStartupForceTmssUntilUnlock());
	EXPECT_FALSE(mm->GetTmssUnlocked());
	EXPECT_FALSE(mm->IsTmssLockedReadAllowedForAddr(0xC00000));
	EXPECT_FALSE(mm->IsTmssLockedWriteAllowedForAddr(0xC00000));

	mm->Write8(0xA14000, 'S');
	mm->Write8(0xA14001, 'E');
	mm->Write8(0xA14002, 'G');
	mm->Write8(0xA14003, 'A');

	EXPECT_TRUE(mm->GetStartupHadTmssSignature());
	EXPECT_TRUE(mm->GetTmssUnlockPending());
	EXPECT_FALSE(mm->GetTmssUnlocked());
	EXPECT_FALSE(mm->IsTmssLockedWriteAllowedForAddr(0xC00000));

	mm->Exec(16);
	EXPECT_TRUE(mm->GetTmssUnlockPending());
	EXPECT_GT(mm->GetTmssUnlockDelayMclk(), 0u);

	mm->Exec(16);
	EXPECT_FALSE(mm->GetTmssUnlockPending());
	EXPECT_TRUE(mm->GetTmssUnlocked());
	EXPECT_TRUE(mm->IsTmssLockedReadAllowedForAddr(0xC00000));
	EXPECT_TRUE(mm->IsTmssLockedWriteAllowedForAddr(0xC00000));
}

TEST(GenesisExecutionStartupTests, StartupTmssStrictDuringLogoDisablesBypassWithinStartupWindow) {
	ScopedStartupEnv env({
		{ "NEXEN_GENESIS_STARTUP_PROFILE", "logo-compat" },
		{ "NEXEN_GENESIS_TMSS_STRICT", "1" },
		{ "NEXEN_GENESIS_TMSS_STRICT_DURING_LOGO", "1" },
		{ "NEXEN_GENESIS_STARTUP_WINDOW_FRAMES", "64" },
		{ "NEXEN_GENESIS_STARTUP_LOGO_PHASE_END_FRAME", "120" }
	});

	constexpr uint32_t InitialSp = 0x00FFFE00;
	constexpr uint32_t InitialPc = 0x00000100;
	std::vector<uint8_t> romData = BuildNopBootRom(InitialSp, InitialPc, 0x4000, true);
	VirtualFile rom(romData.data(), romData.size(), "boot-tmss-strict-logo.md");
	Emulator emu;
	emu.GetSettings()->GetGenesisConfig().EnableTmss = true;
	GenesisConsole console(&emu);

	ASSERT_EQ(console.LoadRom(rom), LoadRomResult::Success);
	ASSERT_NE(console.GetMemoryManager(), nullptr);
	auto* mm = console.GetMemoryManager();

	EXPECT_TRUE(mm->GetTmssEnabled());
	EXPECT_TRUE(mm->GetTmssStrictMode());
	EXPECT_TRUE(mm->GetStartupStrictTmssDuringLogo());
	EXPECT_FALSE(mm->GetTmssUnlocked());
	EXPECT_FALSE(mm->IsTmssLockedReadAllowedForAddr(0xC00000));
	EXPECT_FALSE(mm->IsTmssLockedWriteAllowedForAddr(0xC00000));

	// Status/HV ports remain available even in strict TMSS lock mode.
	EXPECT_TRUE(mm->IsTmssLockedReadAllowedForAddr(0xC00008));
	EXPECT_TRUE(mm->IsTmssLockedWriteAllowedForAddr(0xC00004));
}

TEST(GenesisExecutionStartupTests, StartupTmssStrictDelayedUnlockCheckpointsRemainDeterministicAcrossRuns) {
	auto captureCheckpointTimeline = []() {
		ScopedStartupEnv env({
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "hybrid" },
			{ "NEXEN_GENESIS_TMSS_STRICT", "1" },
			{ "NEXEN_GENESIS_TMSS_STRICT_DURING_LOGO", "1" },
			{ "NEXEN_GENESIS_TMSS_FORCE_UNTIL_UNLOCK", "1" },
			{ "NEXEN_GENESIS_TMSS_UNLOCK_DELAY_MCLK", "32" },
			{ "NEXEN_GENESIS_STARTUP_WINDOW_FRAMES", "96" },
			{ "NEXEN_GENESIS_STARTUP_LOGO_PHASE_END_FRAME", "144" }
		});

		constexpr uint32_t InitialSp = 0x00FFFE00;
		constexpr uint32_t InitialPc = 0x00000100;
		std::vector<uint8_t> romData = BuildNopBootRom(InitialSp, InitialPc, 0x4000, true);
		VirtualFile rom(romData.data(), romData.size(), "boot-tmss-strict-delayed-unlock.md");
		Emulator emu;
		emu.GetSettings()->GetGenesisConfig().EnableTmss = true;
		GenesisConsole console(&emu);

		if (console.LoadRom(rom) != LoadRomResult::Success || !console.GetMemoryManager()) {
			return std::tuple<std::vector<uint64_t>, uint32_t>(std::vector<uint64_t>(), 0xffffffffu);
		}

		auto* mm = console.GetMemoryManager();
		mm->Write8(0xA14000, 'S');
		mm->Write8(0xA14001, 'E');
		mm->Write8(0xA14002, 'G');
		mm->Write8(0xA14003, 'A');

		std::vector<uint64_t> checkpoints;
		checkpoints.reserve(10);
		uint32_t firstUnlockedStep = 0xffffffffu;
		for (uint32_t step = 0; step < 10; step++) {
			mm->Exec(4);
			uint64_t packed = 0;
			packed |= (uint64_t)(mm->GetTmssUnlockPending() ? 1u : 0u) << 0;
			packed |= (uint64_t)(mm->GetTmssUnlocked() ? 1u : 0u) << 1;
			packed |= (uint64_t)(mm->IsTmssLockedReadAllowedForAddr(0xC00000) ? 1u : 0u) << 2;
			packed |= (uint64_t)(mm->IsTmssLockedWriteAllowedForAddr(0xC00000) ? 1u : 0u) << 3;
			packed |= (uint64_t)(mm->GetTmssUnlockDelayMclk() & 0xFFFFu) << 8;
			checkpoints.push_back(packed);
			if (mm->GetTmssUnlocked() && firstUnlockedStep == 0xffffffffu) {
				firstUnlockedStep = step;
			}
		}

		return std::tuple<std::vector<uint64_t>, uint32_t>(checkpoints, firstUnlockedStep);
	};

	auto runA = captureCheckpointTimeline();
	auto runB = captureCheckpointTimeline();

	ASSERT_FALSE(std::get<0>(runA).empty());
	EXPECT_EQ(runA, runB);
	EXPECT_NE(std::get<1>(runA), 0xffffffffu);
	EXPECT_LT(std::get<1>(runA), 10u);
}

TEST(GenesisExecutionStartupTests, FirstSecondArbitrationTelemetryRemainsDeterministicAcrossRuns) {
	auto capture = []() {
		ScopedStartupEnv env({
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "hybrid" },
			{ "NEXEN_GENESIS_USE_DYNAMIC_BUS_TIMING", "1" },
			{ "NEXEN_GENESIS_STARTUP_LOGO_PHASE_END_FRAME", "90" },
			{ "NEXEN_GENESIS_STARTUP_STRICT_PHASE_START_FRAME", "240" },
			{ "NEXEN_GENESIS_Z80_EARLY_BUSREQ_ACK_DELAY_MCLK", "45" },
			{ "NEXEN_GENESIS_Z80_EARLY_BUSRESUME_DELAY_MCLK", "15" },
			{ "NEXEN_GENESIS_Z80_LATE_BUSREQ_ACK_DELAY_MCLK", "9" },
			{ "NEXEN_GENESIS_Z80_LATE_BUSRESUME_DELAY_MCLK", "7" }
		});

		constexpr uint32_t InitialSp = 0x00FFFE00;
		constexpr uint32_t InitialPc = 0x00000100;
		std::vector<uint8_t> romData = BuildNopBootRom(InitialSp, InitialPc, 0x4000, true);
		VirtualFile rom(romData.data(), romData.size(), "boot-first-second-arbitration.md");
		Emulator emu;
		GenesisConsole console(&emu);
		if (console.LoadRom(rom) != LoadRomResult::Success || !console.GetMemoryManager() || !console.GetCpu()) {
			return std::tuple<uint8_t, uint8_t, uint16_t, uint64_t, uint32_t>(0, 0, 0, 0, 0);
		}

		auto* mm = console.GetMemoryManager();
		for (int i = 0; i < 64; i++) {
			mm->Write8(0xA11200, 0x01);
			mm->Write8(0xA11100, 0x01);
			mm->Exec(8);
			mm->Write8(0xA11100, 0x00);
			mm->Exec(8);
			mm->Write8(0xA11200, 0x00);
			mm->Exec(4);
		}

		return std::tuple<uint8_t, uint8_t, uint16_t, uint64_t, uint32_t>(
			mm->GetStartupArbitrationDigest(),
			mm->GetStartupArbitrationEpoch(),
			mm->GetStartupLastArbitrationMclk(),
			mm->GetZ80RuntimeTransitionCount(),
			mm->GetStartupBusTimingRetuneCount());
	};

	auto runA = capture();
	auto runB = capture();

	EXPECT_EQ(runA, runB);
	EXPECT_GT(std::get<2>(runA), 0u);
	EXPECT_GT(std::get<3>(runA), 0u);
}
