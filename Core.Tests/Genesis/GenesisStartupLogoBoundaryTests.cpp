#include "pch.h"
#include "Genesis/GenesisMemoryManager.h"
#include "Shared/Emulator.h"
#include <memory>

namespace {
	struct ScopedEnvVar {
		std::string Name;
		bool HadValue = false;
		std::string OldValue;

		ScopedEnvVar(const char* name, const char* value)
			: Name(name) {
			const char* existing = std::getenv(Name.c_str());
			if (existing) {
				HadValue = true;
				OldValue = existing;
			}

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

	struct StartupBoundaryFixture {
		std::vector<ScopedEnvVar> Vars;

		StartupBoundaryFixture()
			: Vars() {
			Vars.reserve(24);
			ResetAll();
		}

		void ResetAll() {
			Vars.emplace_back("NEXEN_GENESIS_STARTUP_PROFILE", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_STARTUP_WINDOW_FRAMES", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_STARTUP_CHECKPOINT_INTERVAL_FRAMES", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_STARTUP_CHECKPOINT_END_FRAME", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_TMSS_STRICT", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_Z80_BUSREQ_ACK_DELAY_MCLK", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_Z80_BUSRESUME_DELAY_MCLK", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_Z80_LATCH_HIGH_BYTE_ONLY", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_PREFER_MESEN_BUS_HANDOFF", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_POWERON_Z80_RESET_ASSERTED", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_STARTUP_TRACE", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_STARTUP_TRACE_FRAME_END", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_STARTUP_TRACE_MAX_LINES", nullptr);
		}
	};

	GenesisMemoryManager CreateMemoryManagerWithDefaults() {
		static std::vector<std::unique_ptr<Emulator>> sOwnedEmulators;
		auto emu = std::make_unique<Emulator>();
		emu->Initialize(false);

		std::vector<uint8_t> romData(0x400000);
		GenesisMemoryManager memoryManager;
		memoryManager.Init(emu.get(), nullptr, romData, nullptr, nullptr, nullptr);
		memoryManager.ResetRuntimeState(true);
		sOwnedEmulators.push_back(std::move(emu));
		return memoryManager;
	}

	GenesisMemoryManager CreateMemoryManagerWithEnv(const std::vector<std::pair<const char*, const char*>>& envVars) {
		StartupBoundaryFixture fixture;
		for (const auto& [name, value] : envVars) {
			fixture.Vars.emplace_back(name, value);
		}

		return CreateMemoryManagerWithDefaults();
	}

	TEST(GenesisStartupLogoBoundaryTests, UnknownProfileFallsBackToLogoCompatDefaults) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "future-profile" }
		});

		EXPECT_EQ(memoryManager.GetStartupWindowFrames(), 16u);
		EXPECT_EQ(memoryManager.GetStartupCheckpointIntervalFrames(), 1u);
		EXPECT_EQ(memoryManager.GetStartupCheckpointEndFrame(), 600u);
		EXPECT_TRUE(memoryManager.GetStartupProfilePreferMesenBusHandoff());
	}

	TEST(GenesisStartupLogoBoundaryTests, InvalidWindowValueKeepsProfileDefault) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "strict" },
			{ "NEXEN_GENESIS_STARTUP_WINDOW_FRAMES", "invalid" }
		});

		EXPECT_EQ(memoryManager.GetStartupWindowFrames(), 0u);
	}

	TEST(GenesisStartupLogoBoundaryTests, OutOfRangeWindowValueKeepsProfileDefault) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_WINDOW_FRAMES", "121" }
		});

		EXPECT_EQ(memoryManager.GetStartupWindowFrames(), 16u);
	}

	TEST(GenesisStartupLogoBoundaryTests, BoundaryWindowValueAtMaximumIsAccepted) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_WINDOW_FRAMES", "120" }
		});

		EXPECT_EQ(memoryManager.GetStartupWindowFrames(), 120u);
	}

	TEST(GenesisStartupLogoBoundaryTests, InvalidCheckpointIntervalKeepsProfileDefault) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "strict" },
			{ "NEXEN_GENESIS_STARTUP_CHECKPOINT_INTERVAL_FRAMES", "0" }
		});

		EXPECT_EQ(memoryManager.GetStartupCheckpointIntervalFrames(), 2u);
	}

	TEST(GenesisStartupLogoBoundaryTests, MaximumCheckpointIntervalIsAccepted) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_CHECKPOINT_INTERVAL_FRAMES", "120" }
		});

		EXPECT_EQ(memoryManager.GetStartupCheckpointIntervalFrames(), 120u);
	}

	TEST(GenesisStartupLogoBoundaryTests, OutOfRangeCheckpointEndKeepsProfileDefault) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_CHECKPOINT_END_FRAME", "2000" }
		});

		EXPECT_EQ(memoryManager.GetStartupCheckpointEndFrame(), 600u);
	}

	TEST(GenesisStartupLogoBoundaryTests, MaximumCheckpointEndIsAccepted) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_CHECKPOINT_END_FRAME", "1800" }
		});

		EXPECT_EQ(memoryManager.GetStartupCheckpointEndFrame(), 1800u);
	}

	TEST(GenesisStartupLogoBoundaryTests, InvalidBusDelayValuesKeepDefaults) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_Z80_BUSREQ_ACK_DELAY_MCLK", "bad" },
			{ "NEXEN_GENESIS_Z80_BUSRESUME_DELAY_MCLK", "bad" }
		});

		EXPECT_EQ(memoryManager.GetZ80BusReqAckDelaySettingMclk(), 7u);
		EXPECT_EQ(memoryManager.GetZ80BusResumeDelaySettingMclk(), 7u);
	}

	TEST(GenesisStartupLogoBoundaryTests, MaximumBusDelayValuesAreAccepted) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_Z80_BUSREQ_ACK_DELAY_MCLK", "255" },
			{ "NEXEN_GENESIS_Z80_BUSRESUME_DELAY_MCLK", "255" }
		});

		EXPECT_EQ(memoryManager.GetZ80BusReqAckDelaySettingMclk(), 255u);
		EXPECT_EQ(memoryManager.GetZ80BusResumeDelaySettingMclk(), 255u);
	}

	TEST(GenesisStartupLogoBoundaryTests, OutOfRangeBusDelayValuesKeepDefaults) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_Z80_BUSREQ_ACK_DELAY_MCLK", "256" },
			{ "NEXEN_GENESIS_Z80_BUSRESUME_DELAY_MCLK", "4096" }
		});

		EXPECT_EQ(memoryManager.GetZ80BusReqAckDelaySettingMclk(), 7u);
		EXPECT_EQ(memoryManager.GetZ80BusResumeDelaySettingMclk(), 7u);
	}

	TEST(GenesisStartupLogoBoundaryTests, BoolParsingAcceptsNumericValueForHandoffPreference) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_PREFER_MESEN_BUS_HANDOFF", "0" }
		});

		EXPECT_FALSE(memoryManager.GetStartupProfilePreferMesenBusHandoff());
	}

	TEST(GenesisStartupLogoBoundaryTests, BoolParsingAcceptsNumericValueForPowerOnResetState) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_POWERON_Z80_RESET_ASSERTED", "0" }
		});

		EXPECT_FALSE(memoryManager.GetZ80ResetAsserted());
		EXPECT_TRUE(memoryManager.GetZ80RuntimeRunning());
	}

	TEST(GenesisStartupLogoBoundaryTests, BoolParsingIgnoresUnknownValuesForLatchMode) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_Z80_LATCH_HIGH_BYTE_ONLY", "unknown" }
		});

		memoryManager.Write8(0xA11101, 0x01);
		EXPECT_FALSE(memoryManager.GetZ80BusRequestLatched());
	}

	TEST(GenesisStartupLogoBoundaryTests, StrictProfileCanBeOverriddenWithExplicitCompatibilityHandoff) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "strict" },
			{ "NEXEN_GENESIS_PREFER_MESEN_BUS_HANDOFF", "1" }
		});

		EXPECT_TRUE(memoryManager.GetStartupProfilePreferMesenBusHandoff());
		EXPECT_EQ(memoryManager.GetStartupCheckpointIntervalFrames(), 2u);
	}

	TEST(GenesisStartupLogoBoundaryTests, MesenAliasProfileUsesMesenStartupDefaults) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "mesen-startup" }
		});

		EXPECT_EQ(memoryManager.GetStartupWindowFrames(), 10u);
		EXPECT_EQ(memoryManager.GetStartupCheckpointIntervalFrames(), 1u);
		EXPECT_EQ(memoryManager.GetStartupCheckpointEndFrame(), 600u);
		EXPECT_TRUE(memoryManager.GetStartupProfilePreferMesenBusHandoff());
	}
}
