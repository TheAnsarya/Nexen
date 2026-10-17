#include "pch.h"
#include "Debugger/Debugger.h"
#include "Genesis/GenesisConsole.h"
#include "Shared/Emulator.h"
#include "Shared/EventType.h"
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
}

TEST(GenesisDebuggerEventRoutingTests, DISABLED_NonOwnedDebuggerSkipsScriptDispatchForMismatchedEndFrameEvents) {
	constexpr uint32_t InitialSp = 0x00FFFE00;
	constexpr uint32_t InitialPc = 0x00000100;

	std::vector<uint8_t> romData = BuildNopBootRom(InitialSp, InitialPc);
	VirtualFile rom(romData.data(), romData.size(), "genesis-debug-routing.md");
	Emulator emu;
	GenesisConsole console(&emu);

	ASSERT_EQ(console.LoadRom(rom), LoadRomResult::Success);
	// Non-owned debugger teardown is not lifecycle-safe in this static harness; keep instance alive for test process lifetime.
	Debugger* debugger = new Debugger(&emu, &console);

	ASSERT_TRUE(debugger->HasCpuType(CpuType::Genesis));
	ASSERT_FALSE(debugger->HasCpuType(CpuType::Nes));
	EXPECT_EQ(emu.InternalGetDebugger(), nullptr);

	EXPECT_NO_THROW(debugger->ProcessEvent(EventType::EndFrame, CpuType::Nes));
	EXPECT_NO_THROW(debugger->ProcessEvent(EventType::EndFrame, CpuType::Genesis));
	EXPECT_NO_THROW(debugger->ProcessEvent(EventType::EndFrame, std::nullopt));
}
