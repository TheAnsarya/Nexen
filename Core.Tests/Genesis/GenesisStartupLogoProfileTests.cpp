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

	struct StartupProfileFixture {
		std::vector<ScopedEnvVar> Vars;

		StartupProfileFixture()
			: Vars() {
			Vars.reserve(64);
			ResetAll();
		}

		void ResetAll() {
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
		EXPECT_EQ(memoryManager.GetZ80BusReqAckDelaySettingMclk(), 45u);
		EXPECT_EQ(memoryManager.GetZ80BusResumeDelaySettingMclk(), 15u);
	}

	TEST(GenesisStartupLogoProfileTests, NexenProfileUsesFirstTenSecondCheckpointAndCompatibilityBias) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "nexen-ref" }
		});

		EXPECT_EQ(memoryManager.GetStartupWindowFrames(), 10u);
		EXPECT_EQ(memoryManager.GetStartupCheckpointIntervalFrames(), 1u);
		EXPECT_EQ(memoryManager.GetStartupCheckpointEndFrame(), 600u);
		EXPECT_TRUE(memoryManager.GetStartupProfilePreferNexenBusHandoff());
		EXPECT_EQ(memoryManager.GetZ80BusReqAckDelaySettingMclk(), 7u);
		EXPECT_EQ(memoryManager.GetZ80BusResumeDelaySettingMclk(), 7u);
	}

	TEST(GenesisStartupLogoProfileTests, MesenCompatProfileUsesMesenLikeStartupBusTiming) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "mesen-compat" }
		});

		EXPECT_EQ(memoryManager.GetStartupWindowFrames(), 0u);
		EXPECT_EQ(memoryManager.GetStartupCheckpointIntervalFrames(), 1u);
		EXPECT_EQ(memoryManager.GetStartupCheckpointEndFrame(), 600u);
		EXPECT_FALSE(memoryManager.GetStartupProfilePreferNexenBusHandoff());
		EXPECT_EQ(memoryManager.GetZ80BusReqAckDelaySettingMclk(), 45u);
		EXPECT_EQ(memoryManager.GetZ80BusResumeDelaySettingMclk(), 15u);
	}

	TEST(GenesisStartupLogoProfileTests, HybridProfileStartsWithConservativeStartupDelays) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "hybrid" }
		});

		EXPECT_EQ(memoryManager.GetStartupWindowFrames(), 6u);
		EXPECT_TRUE(memoryManager.GetStartupProfilePreferNexenBusHandoff());
		EXPECT_EQ(memoryManager.GetZ80BusReqAckDelaySettingMclk(), 45u);
		EXPECT_EQ(memoryManager.GetZ80BusResumeDelaySettingMclk(), 15u);
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

	TEST(GenesisStartupLogoProfileTests, DynamicBusTimingStartsFromConfiguredEarlyDelays) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "hybrid" },
			{ "NEXEN_GENESIS_USE_DYNAMIC_BUS_TIMING", "1" },
			{ "NEXEN_GENESIS_Z80_EARLY_BUSREQ_ACK_DELAY_MCLK", "17" },
			{ "NEXEN_GENESIS_Z80_EARLY_BUSRESUME_DELAY_MCLK", "19" },
			{ "NEXEN_GENESIS_Z80_LATE_BUSREQ_ACK_DELAY_MCLK", "9" },
			{ "NEXEN_GENESIS_Z80_LATE_BUSRESUME_DELAY_MCLK", "11" }
		});

		EXPECT_EQ(memoryManager.GetZ80BusReqAckDelaySettingMclk(), 17u);
		EXPECT_EQ(memoryManager.GetZ80BusResumeDelaySettingMclk(), 19u);
	}

	TEST(GenesisStartupLogoProfileTests, DynamicBusTimingCanStillBeForcedWithUnifiedDelayOverrides) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "hybrid" },
			{ "NEXEN_GENESIS_USE_DYNAMIC_BUS_TIMING", "1" },
			{ "NEXEN_GENESIS_Z80_BUSREQ_ACK_DELAY_MCLK", "23" },
			{ "NEXEN_GENESIS_Z80_BUSRESUME_DELAY_MCLK", "25" }
		});

		EXPECT_EQ(memoryManager.GetZ80BusReqAckDelaySettingMclk(), 23u);
		EXPECT_EQ(memoryManager.GetZ80BusResumeDelaySettingMclk(), 25u);
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

	TEST(GenesisStartupLogoProfileTests, HybridProfileCanUseMesenCompatibleBusRequestDelayAtResetRelease) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "hybrid" },
			{ "NEXEN_GENESIS_USE_DYNAMIC_BUS_TIMING", "1" },
			{ "NEXEN_GENESIS_Z80_EARLY_BUSREQ_ACK_DELAY_MCLK", "45" }
		});

		memoryManager.Write8(0xA11200, 0x01);
		EXPECT_FALSE(memoryManager.GetZ80ResetAsserted());
		memoryManager.Write8(0xA11100, 0x01);
		EXPECT_EQ(memoryManager.GetZ80BusReqDelayMclk(), 45u);
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
				{ "NEXEN_GENESIS_STARTUP_PROFILE", "nexen-ref" }
			});
			EXPECT_EQ(memoryManager.GetStartupWindowFrames(), 10u);
			EXPECT_TRUE(memoryManager.GetStartupProfilePreferNexenBusHandoff());
		}

		{
			GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
				{ "NEXEN_GENESIS_STARTUP_PROFILE", "mesen-compat" }
			});
			EXPECT_EQ(memoryManager.GetStartupWindowFrames(), 0u);
			EXPECT_FALSE(memoryManager.GetStartupProfilePreferNexenBusHandoff());
			EXPECT_EQ(memoryManager.GetZ80BusReqAckDelaySettingMclk(), 45u);
		}
	}

	TEST(GenesisStartupLogoProfileTests, StartupBootRelaxAndPhaseOverridesDoNotBreakDefaultBusDelaySelection) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_BOOT_RELAX_FRAMES", "8" },
			{ "NEXEN_GENESIS_STARTUP_LOGO_PHASE_END_FRAME", "180" },
			{ "NEXEN_GENESIS_STARTUP_STRICT_PHASE_START_FRAME", "360" }
		});

		EXPECT_EQ(memoryManager.GetStartupWindowFrames(), 16u);
		EXPECT_EQ(memoryManager.GetZ80BusReqAckDelaySettingMclk(), 7u);
		EXPECT_EQ(memoryManager.GetZ80BusResumeDelaySettingMclk(), 7u);
	}

	TEST(GenesisStartupLogoProfileTests, MesenHandoffPreferenceOverrideCanBeEnabledExplicitly) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "logo-compat" },
			{ "NEXEN_GENESIS_PREFER_MESEN_BUS_HANDOFF", "1" }
		});

		// Public API exposes nexen-pref, so ensure explicit mesen-pref does not regress
		// compatibility defaults for startup checkpoint policies.
		EXPECT_EQ(memoryManager.GetStartupCheckpointEndFrame(), 600u);
		EXPECT_EQ(memoryManager.GetZ80BusReqAckDelaySettingMclk(), 7u);
	}

	TEST(GenesisStartupLogoProfileTests, StartupPhaseOverridesAreStoredAndStrictStartIsClampedToLogoPhaseEnd) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_BOOT_RELAX_FRAMES", "9" },
			{ "NEXEN_GENESIS_STARTUP_LOGO_PHASE_END_FRAME", "160" },
			{ "NEXEN_GENESIS_STARTUP_STRICT_PHASE_START_FRAME", "100" }
		});

		EXPECT_EQ(memoryManager.GetStartupBootRelaxFrames(), 9u);
		EXPECT_EQ(memoryManager.GetStartupLogoPhaseEndFrame(), 160u);
		EXPECT_EQ(memoryManager.GetStartupStrictPhaseStartFrame(), 160u);
	}

	TEST(GenesisStartupLogoProfileTests, HybridDynamicTimingInterpolatesAcrossPhaseBoundaries) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "hybrid" },
			{ "NEXEN_GENESIS_USE_DYNAMIC_BUS_TIMING", "1" },
			{ "NEXEN_GENESIS_STARTUP_LOGO_PHASE_END_FRAME", "100" },
			{ "NEXEN_GENESIS_STARTUP_STRICT_PHASE_START_FRAME", "300" },
			{ "NEXEN_GENESIS_Z80_EARLY_BUSREQ_ACK_DELAY_MCLK", "45" },
			{ "NEXEN_GENESIS_Z80_EARLY_BUSRESUME_DELAY_MCLK", "15" },
			{ "NEXEN_GENESIS_Z80_LATE_BUSREQ_ACK_DELAY_MCLK", "7" },
			{ "NEXEN_GENESIS_Z80_LATE_BUSRESUME_DELAY_MCLK", "7" }
		});

		EXPECT_TRUE(memoryManager.GetStartupUseDynamicBusTiming());
		EXPECT_TRUE(memoryManager.IsStartupLogoPhaseForFrame(99u));
		EXPECT_FALSE(memoryManager.IsStartupLogoPhaseForFrame(100u));
		EXPECT_FALSE(memoryManager.IsStartupStrictPhaseForFrame(299u));
		EXPECT_TRUE(memoryManager.IsStartupStrictPhaseForFrame(300u));

		EXPECT_EQ(memoryManager.GetEffectiveZ80BusReqAckDelayForFrame(0u), 45u);
		EXPECT_EQ(memoryManager.GetEffectiveZ80BusReqAckDelayForFrame(100u), 45u);
		EXPECT_EQ(memoryManager.GetEffectiveZ80BusReqAckDelayForFrame(200u), 26u);
		EXPECT_EQ(memoryManager.GetEffectiveZ80BusReqAckDelayForFrame(300u), 7u);

		EXPECT_EQ(memoryManager.GetEffectiveZ80BusResumeDelayForFrame(0u), 15u);
		EXPECT_EQ(memoryManager.GetEffectiveZ80BusResumeDelayForFrame(100u), 15u);
		EXPECT_EQ(memoryManager.GetEffectiveZ80BusResumeDelayForFrame(200u), 11u);
		EXPECT_EQ(memoryManager.GetEffectiveZ80BusResumeDelayForFrame(300u), 7u);
	}

	TEST(GenesisStartupLogoProfileTests, NonDynamicTimingRemainsStableAcrossFrameSweeps) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "logo-compat" },
			{ "NEXEN_GENESIS_USE_DYNAMIC_BUS_TIMING", "0" },
			{ "NEXEN_GENESIS_Z80_BUSREQ_ACK_DELAY_MCLK", "12" },
			{ "NEXEN_GENESIS_Z80_BUSRESUME_DELAY_MCLK", "14" }
		});

		EXPECT_FALSE(memoryManager.GetStartupUseDynamicBusTiming());
		for (uint32_t frame = 0; frame <= 720; frame += 120) {
			EXPECT_EQ(memoryManager.GetEffectiveZ80BusReqAckDelayForFrame(frame), 12u);
			EXPECT_EQ(memoryManager.GetEffectiveZ80BusResumeDelayForFrame(frame), 14u);
		}
	}

	TEST(GenesisStartupLogoProfileTests, RefreshStartupBusTimingForFrameTracksPhaseRetunes) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "hybrid" },
			{ "NEXEN_GENESIS_USE_DYNAMIC_BUS_TIMING", "1" },
			{ "NEXEN_GENESIS_STARTUP_LOGO_PHASE_END_FRAME", "60" },
			{ "NEXEN_GENESIS_STARTUP_STRICT_PHASE_START_FRAME", "120" },
			{ "NEXEN_GENESIS_Z80_EARLY_BUSREQ_ACK_DELAY_MCLK", "30" },
			{ "NEXEN_GENESIS_Z80_EARLY_BUSRESUME_DELAY_MCLK", "20" },
			{ "NEXEN_GENESIS_Z80_LATE_BUSREQ_ACK_DELAY_MCLK", "10" },
			{ "NEXEN_GENESIS_Z80_LATE_BUSRESUME_DELAY_MCLK", "8" }
		});

		uint32_t initialRetunes = memoryManager.GetStartupBusTimingRetuneCount();
		memoryManager.RefreshStartupBusTimingForFrame(60u);
		memoryManager.RefreshStartupBusTimingForFrame(90u);
		memoryManager.RefreshStartupBusTimingForFrame(120u);

		EXPECT_GE(memoryManager.GetStartupBusTimingRetuneCount(), initialRetunes + 2u);
		EXPECT_EQ(memoryManager.GetStartupLastBusTimingFrame(), 120u);
		EXPECT_EQ(memoryManager.GetZ80BusReqAckDelaySettingMclk(), 10u);
		EXPECT_EQ(memoryManager.GetZ80BusResumeDelaySettingMclk(), 8u);
	}
}




