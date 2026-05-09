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

	struct StartupProfileFixture {
		std::vector<ScopedEnvVar> Vars;

		StartupProfileFixture()
			: Vars() {
			Vars.reserve(16);
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
		StartupProfileFixture fixture;
		for (const auto& [name, value] : envVars) {
			fixture.Vars.emplace_back(name, value);
		}

		return CreateMemoryManagerWithDefaults();
	}

	TEST(GenesisStartupLogoProfileTests, DefaultProfileUsesLogoCompatibilityWindowAndTenSecondCheckpointRange) {
		StartupProfileFixture fixture;
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithDefaults();

		EXPECT_EQ(memoryManager.GetStartupWindowFrames(), 16u);
		EXPECT_EQ(memoryManager.GetStartupCheckpointIntervalFrames(), 1u);
		EXPECT_EQ(memoryManager.GetStartupCheckpointEndFrame(), 600u);
		EXPECT_TRUE(memoryManager.GetStartupProfilePreferNexenBusHandoff());
		EXPECT_EQ(memoryManager.GetZ80BusReqAckDelaySettingMclk(), 7u);
		EXPECT_EQ(memoryManager.GetZ80BusResumeDelaySettingMclk(), 7u);
	}

	TEST(GenesisStartupLogoProfileTests, StrictProfileDisablesStartupWindowAndNexenCompatibilityBias) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "strict-startup" }
		});

		EXPECT_EQ(memoryManager.GetStartupWindowFrames(), 0u);
		EXPECT_EQ(memoryManager.GetStartupCheckpointIntervalFrames(), 2u);
		EXPECT_EQ(memoryManager.GetStartupCheckpointEndFrame(), 120u);
		EXPECT_FALSE(memoryManager.GetStartupProfilePreferNexenBusHandoff());
	}

	TEST(GenesisStartupLogoProfileTests, NexenProfileUsesFirstTenSecondCheckpointAndCompatibilityBias) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "mesen" }
		});

		EXPECT_EQ(memoryManager.GetStartupWindowFrames(), 10u);
		EXPECT_EQ(memoryManager.GetStartupCheckpointIntervalFrames(), 1u);
		EXPECT_EQ(memoryManager.GetStartupCheckpointEndFrame(), 600u);
		EXPECT_TRUE(memoryManager.GetStartupProfilePreferNexenBusHandoff());
	}

	TEST(GenesisStartupLogoProfileTests, ExplicitWindowAndCheckpointOverridesApplyAcrossProfiles) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "strict" },
			{ "NEXEN_GENESIS_STARTUP_WINDOW_FRAMES", "24" },
			{ "NEXEN_GENESIS_STARTUP_CHECKPOINT_INTERVAL_FRAMES", "3" },
			{ "NEXEN_GENESIS_STARTUP_CHECKPOINT_END_FRAME", "900" }
		});

		EXPECT_EQ(memoryManager.GetStartupWindowFrames(), 24u);
		EXPECT_EQ(memoryManager.GetStartupCheckpointIntervalFrames(), 3u);
		EXPECT_EQ(memoryManager.GetStartupCheckpointEndFrame(), 900u);
	}

	TEST(GenesisStartupLogoProfileTests, ConfigurableBusReqAndResumeDelaysAreApplied) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_Z80_BUSREQ_ACK_DELAY_MCLK", "11" },
			{ "NEXEN_GENESIS_Z80_BUSRESUME_DELAY_MCLK", "13" }
		});

		EXPECT_EQ(memoryManager.GetZ80BusReqAckDelaySettingMclk(), 11u);
		EXPECT_EQ(memoryManager.GetZ80BusResumeDelaySettingMclk(), 13u);
	}

	TEST(GenesisStartupLogoProfileTests, PowerOnResetAssertionCanBeDisabledForStartupTuning) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_POWERON_Z80_RESET_ASSERTED", "0" }
		});

		EXPECT_FALSE(memoryManager.GetZ80ResetAsserted());
		EXPECT_TRUE(memoryManager.GetZ80RuntimeRunning());
	}

	TEST(GenesisStartupLogoProfileTests, BusReqOddByteWriteIgnoredWhenHighByteLatchOnlyIsEnabled) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_Z80_LATCH_HIGH_BYTE_ONLY", "1" }
		});

		EXPECT_FALSE(memoryManager.GetZ80BusRequestLatched());
		memoryManager.Write8(0xA11101, 0x01);
		EXPECT_FALSE(memoryManager.GetZ80BusRequestLatched());

		memoryManager.Write8(0xA11100, 0x01);
		EXPECT_TRUE(memoryManager.GetZ80BusRequestLatched());
	}

	TEST(GenesisStartupLogoProfileTests, ResetOddByteWriteIgnoredWhenHighByteLatchOnlyIsEnabled) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_Z80_LATCH_HIGH_BYTE_ONLY", "1" }
		});

		EXPECT_TRUE(memoryManager.GetZ80ResetAsserted());
		memoryManager.Write8(0xA11201, 0x01);
		EXPECT_TRUE(memoryManager.GetZ80ResetAsserted());

		memoryManager.Write8(0xA11200, 0x01);
		EXPECT_FALSE(memoryManager.GetZ80ResetAsserted());
	}

	TEST(GenesisStartupLogoProfileTests, BusReqOddByteWriteCanLatchWhenConfiguredForByteAgnosticMode) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_Z80_LATCH_HIGH_BYTE_ONLY", "0" }
		});

		EXPECT_FALSE(memoryManager.GetZ80BusRequestLatched());
		memoryManager.Write8(0xA11101, 0x01);
		EXPECT_TRUE(memoryManager.GetZ80BusRequestLatched());
	}

	TEST(GenesisStartupLogoProfileTests, ResetOddByteWriteCanLatchWhenConfiguredForByteAgnosticMode) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_Z80_LATCH_HIGH_BYTE_ONLY", "0" }
		});

		EXPECT_TRUE(memoryManager.GetZ80ResetAsserted());
		memoryManager.Write8(0xA11201, 0x01);
		EXPECT_FALSE(memoryManager.GetZ80ResetAsserted());
	}

	TEST(GenesisStartupLogoProfileTests, BusReqDelayUsesConfiguredValueAfterResetRelease) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_Z80_BUSREQ_ACK_DELAY_MCLK", "21" }
		});

		memoryManager.Write8(0xA11200, 0x01);
		EXPECT_FALSE(memoryManager.GetZ80ResetAsserted());
		memoryManager.Write8(0xA11100, 0x01);
		EXPECT_EQ(memoryManager.GetZ80BusReqDelayMclk(), 21u);
		EXPECT_FALSE(memoryManager.GetZ80BusAckLatched());
	}

	TEST(GenesisStartupLogoProfileTests, ResumeDelayUsesConfiguredValueAfterBusRequestRelease) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_Z80_BUSRESUME_DELAY_MCLK", "19" }
		});

		memoryManager.Write8(0xA11200, 0x01);
		memoryManager.Write8(0xA11100, 0x01);
		memoryManager.Write8(0xA11100, 0x00);
		EXPECT_EQ(memoryManager.GetZ80ResumeDelayMclk(), 19u);
	}

	TEST(GenesisStartupLogoProfileTests, DebugPathHonorsHighByteOnlyBusReqLatching) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_Z80_LATCH_HIGH_BYTE_ONLY", "1" }
		});

		EXPECT_FALSE(memoryManager.GetZ80BusRequestLatched());
		memoryManager.DebugWrite8(0xA11101, 0x01);
		EXPECT_FALSE(memoryManager.GetZ80BusRequestLatched());
		memoryManager.DebugWrite8(0xA11100, 0x01);
		EXPECT_TRUE(memoryManager.GetZ80BusRequestLatched());
	}

	TEST(GenesisStartupLogoProfileTests, DebugPathHonorsHighByteOnlyResetLatching) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_Z80_LATCH_HIGH_BYTE_ONLY", "1" }
		});

		EXPECT_TRUE(memoryManager.GetZ80ResetAsserted());
		memoryManager.DebugWrite8(0xA11201, 0x01);
		EXPECT_TRUE(memoryManager.GetZ80ResetAsserted());
		memoryManager.DebugWrite8(0xA11200, 0x01);
		EXPECT_FALSE(memoryManager.GetZ80ResetAsserted());
	}

	TEST(GenesisStartupLogoProfileTests, DebugPathAllowsByteAgnosticBusReqLatchingWhenConfigured) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_Z80_LATCH_HIGH_BYTE_ONLY", "0" }
		});

		memoryManager.DebugWrite8(0xA11101, 0x01);
		EXPECT_TRUE(memoryManager.GetZ80BusRequestLatched());
	}

	TEST(GenesisStartupLogoProfileTests, DebugPathAllowsByteAgnosticResetLatchingWhenConfigured) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_Z80_LATCH_HIGH_BYTE_ONLY", "0" }
		});

		memoryManager.DebugWrite8(0xA11201, 0x01);
		EXPECT_FALSE(memoryManager.GetZ80ResetAsserted());
	}

	TEST(GenesisStartupLogoProfileTests, StartupProfileParsingSupportsRepeatedRuntimeReconfiguration) {
		{
			GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
				{ "NEXEN_GENESIS_STARTUP_PROFILE", "strict" }
			});
			EXPECT_EQ(memoryManager.GetStartupWindowFrames(), 0u);
			EXPECT_FALSE(memoryManager.GetStartupProfilePreferNexenBusHandoff());
		}

		{
			GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
				{ "NEXEN_GENESIS_STARTUP_PROFILE", "mesen" }
			});
			EXPECT_EQ(memoryManager.GetStartupWindowFrames(), 10u);
			EXPECT_TRUE(memoryManager.GetStartupProfilePreferNexenBusHandoff());
		}
	}
}


