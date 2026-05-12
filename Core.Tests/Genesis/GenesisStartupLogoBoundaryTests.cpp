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
			Vars.reserve(64);
			ResetAll();
		}

		void ResetAll() {
			Vars.emplace_back("NEXEN_GENESIS_STARTUP_PROFILE", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_STARTUP_TITLE_HINT", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_STARTUP_ROM_AUTOTUNE", nullptr);
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
			Vars.emplace_back("NEXEN_GENESIS_STARTUP_TRACE", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_STARTUP_TRACE_FRAME_END", nullptr);
			Vars.emplace_back("NEXEN_GENESIS_STARTUP_TRACE_MAX_LINES", nullptr);
		}
	};

	void WriteGenesisHeaderField(std::vector<uint8_t>& romData, size_t offset, size_t length, const std::string& value) {
		for (size_t i = 0; i < length; i++) {
			romData[offset + i] = i < value.size() ? (uint8_t)value[i] : (uint8_t)' ';
		}
	}

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

	GenesisMemoryManager CreateMemoryManagerWithTitle(const std::string& title, const std::string& productCode = "") {
		static std::vector<std::unique_ptr<Emulator>> sOwnedEmulators;
		auto emu = std::make_unique<Emulator>();
		emu->Initialize(false);

		std::vector<uint8_t> romData(0x400000);
		WriteGenesisHeaderField(romData, 0x120, 48, title);
		WriteGenesisHeaderField(romData, 0x150, 48, title);
		if (!productCode.empty()) {
			WriteGenesisHeaderField(romData, 0x180, 14, productCode);
		}

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

	GenesisMemoryManager CreateMemoryManagerWithEnvAndTitle(const std::vector<std::pair<const char*, const char*>>& envVars, const std::string& title, const std::string& productCode = "") {
		StartupBoundaryFixture fixture;
		for (const auto& [name, value] : envVars) {
			fixture.Vars.emplace_back(name, value);
		}

		return CreateMemoryManagerWithTitle(title, productCode);
	}

	TEST(GenesisStartupLogoBoundaryTests, UnknownProfileFallsBackToLogoCompatDefaults) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "future-profile" }
		});

		EXPECT_EQ(memoryManager.GetStartupWindowFrames(), 16u);
		EXPECT_EQ(memoryManager.GetStartupCheckpointIntervalFrames(), 1u);
		EXPECT_EQ(memoryManager.GetStartupCheckpointEndFrame(), 600u);
		EXPECT_TRUE(memoryManager.GetStartupProfilePreferNexenBusHandoff());
	}

	TEST(GenesisStartupLogoBoundaryTests, MesenAliasProfileSelectsMesenCompatibleDefaults) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "mesen" }
		});

		EXPECT_EQ(memoryManager.GetStartupWindowFrames(), 0u);
		EXPECT_FALSE(memoryManager.GetStartupProfilePreferNexenBusHandoff());
		EXPECT_EQ(memoryManager.GetZ80BusReqAckDelaySettingMclk(), 45u);
		EXPECT_EQ(memoryManager.GetZ80BusResumeDelaySettingMclk(), 15u);
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

	TEST(GenesisStartupLogoBoundaryTests, OutOfRangeEarlyLateDelayValuesKeepProfileDefaults) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "hybrid" },
			{ "NEXEN_GENESIS_Z80_EARLY_BUSREQ_ACK_DELAY_MCLK", "999" },
			{ "NEXEN_GENESIS_Z80_EARLY_BUSRESUME_DELAY_MCLK", "999" },
			{ "NEXEN_GENESIS_Z80_LATE_BUSREQ_ACK_DELAY_MCLK", "999" },
			{ "NEXEN_GENESIS_Z80_LATE_BUSRESUME_DELAY_MCLK", "999" }
		});

		EXPECT_EQ(memoryManager.GetZ80BusReqAckDelaySettingMclk(), 45u);
		EXPECT_EQ(memoryManager.GetZ80BusResumeDelaySettingMclk(), 15u);
	}

	TEST(GenesisStartupLogoBoundaryTests, BoolParsingAcceptsNumericValueForHandoffPreference) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_PREFER_NEXENREF_BUS_HANDOFF", "0" }
		});

		EXPECT_FALSE(memoryManager.GetStartupProfilePreferNexenBusHandoff());
	}

	TEST(GenesisStartupLogoBoundaryTests, BoolParsingAcceptsNumericValueForPowerOnResetState) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_POWERON_Z80_RESET_ASSERTED", "0" }
		});

		EXPECT_FALSE(memoryManager.GetZ80ResetAsserted());
		EXPECT_TRUE(memoryManager.GetZ80RuntimeRunning());
	}

	TEST(GenesisStartupLogoBoundaryTests, InvalidBoolValuesDoNotOverrideDynamicBusTimingDefaults) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "hybrid" },
			{ "NEXEN_GENESIS_USE_DYNAMIC_BUS_TIMING", "banana" }
		});

		EXPECT_EQ(memoryManager.GetZ80BusReqAckDelaySettingMclk(), 45u);
		EXPECT_EQ(memoryManager.GetZ80BusResumeDelaySettingMclk(), 15u);
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
			{ "NEXEN_GENESIS_PREFER_NEXENREF_BUS_HANDOFF", "1" }
		});

		EXPECT_TRUE(memoryManager.GetStartupProfilePreferNexenBusHandoff());
		EXPECT_EQ(memoryManager.GetStartupCheckpointIntervalFrames(), 2u);
	}

	TEST(GenesisStartupLogoBoundaryTests, ExplicitMesenHandoffPreferenceCanBeEnabledIndependently) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "logo-compat" },
			{ "NEXEN_GENESIS_PREFER_MESEN_BUS_HANDOFF", "1" }
		});

		EXPECT_EQ(memoryManager.GetStartupWindowFrames(), 16u);
		EXPECT_EQ(memoryManager.GetStartupCheckpointEndFrame(), 600u);
	}

	TEST(GenesisStartupLogoBoundaryTests, NexenAliasProfileUsesNexenStartupDefaults) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "nexen-ref-startup" }
		});

		EXPECT_EQ(memoryManager.GetStartupWindowFrames(), 10u);
		EXPECT_EQ(memoryManager.GetStartupCheckpointIntervalFrames(), 1u);
		EXPECT_EQ(memoryManager.GetStartupCheckpointEndFrame(), 600u);
		EXPECT_TRUE(memoryManager.GetStartupProfilePreferNexenBusHandoff());
	}

	TEST(GenesisStartupLogoBoundaryTests, HybridAliasProfileUsesDynamicFriendlyDefaults) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "hybrid-startup" }
		});

		EXPECT_EQ(memoryManager.GetStartupWindowFrames(), 6u);
		EXPECT_TRUE(memoryManager.GetStartupProfilePreferNexenBusHandoff());
		EXPECT_EQ(memoryManager.GetZ80BusReqAckDelaySettingMclk(), 45u);
		EXPECT_EQ(memoryManager.GetZ80BusResumeDelaySettingMclk(), 15u);
	}

	TEST(GenesisStartupLogoBoundaryTests, StrictPhaseStartLowerThanLogoEndIsClampedToLogoEnd) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_LOGO_PHASE_END_FRAME", "220" },
			{ "NEXEN_GENESIS_STARTUP_STRICT_PHASE_START_FRAME", "100" }
		});

		EXPECT_EQ(memoryManager.GetStartupLogoPhaseEndFrame(), 220u);
		EXPECT_EQ(memoryManager.GetStartupStrictPhaseStartFrame(), 220u);
	}

	TEST(GenesisStartupLogoBoundaryTests, OutOfRangePhaseBoundaryValuesKeepProfileDefaults) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_LOGO_PHASE_END_FRAME", "1900" },
			{ "NEXEN_GENESIS_STARTUP_STRICT_PHASE_START_FRAME", "1900" }
		});

		EXPECT_EQ(memoryManager.GetStartupLogoPhaseEndFrame(), 120u);
		EXPECT_EQ(memoryManager.GetStartupStrictPhaseStartFrame(), 300u);
	}

	TEST(GenesisStartupLogoBoundaryTests, UnifiedDelayOverridesCanBeRefinedByLatePhaseSpecificOverrides) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "hybrid" },
			{ "NEXEN_GENESIS_USE_DYNAMIC_BUS_TIMING", "1" },
			{ "NEXEN_GENESIS_Z80_BUSREQ_ACK_DELAY_MCLK", "23" },
			{ "NEXEN_GENESIS_Z80_BUSRESUME_DELAY_MCLK", "25" },
			{ "NEXEN_GENESIS_Z80_LATE_BUSREQ_ACK_DELAY_MCLK", "11" },
			{ "NEXEN_GENESIS_Z80_LATE_BUSRESUME_DELAY_MCLK", "13" },
			{ "NEXEN_GENESIS_STARTUP_LOGO_PHASE_END_FRAME", "100" },
			{ "NEXEN_GENESIS_STARTUP_STRICT_PHASE_START_FRAME", "200" }
		});

		EXPECT_EQ(memoryManager.GetEffectiveZ80BusReqAckDelayForFrame(0u), 23u);
		EXPECT_EQ(memoryManager.GetEffectiveZ80BusResumeDelayForFrame(0u), 25u);
		EXPECT_EQ(memoryManager.GetEffectiveZ80BusReqAckDelayForFrame(200u), 11u);
		EXPECT_EQ(memoryManager.GetEffectiveZ80BusResumeDelayForFrame(200u), 13u);
	}

	TEST(GenesisStartupLogoBoundaryTests, DynamicTimingDisabledUsesResolvedStaticDelaySettingsAcrossAllPhaseFrames) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "hybrid" },
			{ "NEXEN_GENESIS_USE_DYNAMIC_BUS_TIMING", "0" },
			{ "NEXEN_GENESIS_Z80_BUSREQ_ACK_DELAY_MCLK", "18" },
			{ "NEXEN_GENESIS_Z80_BUSRESUME_DELAY_MCLK", "20" },
			{ "NEXEN_GENESIS_Z80_EARLY_BUSREQ_ACK_DELAY_MCLK", "45" },
			{ "NEXEN_GENESIS_Z80_LATE_BUSREQ_ACK_DELAY_MCLK", "7" }
		});

		EXPECT_FALSE(memoryManager.GetStartupUseDynamicBusTiming());
		EXPECT_EQ(memoryManager.GetEffectiveZ80BusReqAckDelayForFrame(0u), 45u);
		EXPECT_EQ(memoryManager.GetEffectiveZ80BusReqAckDelayForFrame(600u), 45u);
		EXPECT_EQ(memoryManager.GetEffectiveZ80BusResumeDelayForFrame(0u), 20u);
		EXPECT_EQ(memoryManager.GetEffectiveZ80BusResumeDelayForFrame(600u), 20u);
	}

	TEST(GenesisStartupLogoBoundaryTests, SonicStartupProfileAliasProvidesAggressiveStartupDefaults) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnv({
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "sonic-startup" }
		});

		EXPECT_EQ(memoryManager.GetStartupWindowFrames(), 24u);
		EXPECT_EQ(memoryManager.GetStartupBootRelaxFrames(), 6u);
		EXPECT_EQ(memoryManager.GetStartupLogoPhaseEndFrame(), 180u);
		EXPECT_EQ(memoryManager.GetStartupStrictPhaseStartFrame(), 420u);
		EXPECT_EQ(memoryManager.GetStartupCheckpointEndFrame(), 900u);
		EXPECT_TRUE(memoryManager.GetStartupUseDynamicBusTiming());
		EXPECT_TRUE(memoryManager.GetStartupStrictTmssDuringLogo());
		EXPECT_TRUE(memoryManager.GetStartupForceTmssUntilUnlock());
		EXPECT_TRUE(memoryManager.GetTmssStrictMode());
	}

	TEST(GenesisStartupLogoBoundaryTests, SonicTitleAutotuneActivatesWhenProfileIsImplicit) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnvAndTitle({}, "SONIC THE HEDGEHOG");

		EXPECT_TRUE(memoryManager.GetStartupTitleAutotuneApplied());
		EXPECT_FALSE(memoryManager.GetStartupTitleHintUsed());
		EXPECT_EQ(memoryManager.GetStartupTitleClassValue(), 2u);
		EXPECT_GE(memoryManager.GetStartupWindowFrames(), 24u);
		EXPECT_GE(memoryManager.GetStartupLogoPhaseEndFrame(), 180u);
		EXPECT_GE(memoryManager.GetStartupStrictPhaseStartFrame(), 420u);
		EXPECT_GE(memoryManager.GetStartupCheckpointEndFrame(), 900u);
		EXPECT_GE(memoryManager.GetStartupEarlyBusReqAckDelayMclk(), 45u);
		EXPECT_GE(memoryManager.GetStartupEarlyBusResumeDelayMclk(), 15u);
		EXPECT_TRUE(memoryManager.GetStartupUseDynamicBusTiming());
		EXPECT_TRUE(memoryManager.GetStartupProfilePreferMesenBusHandoff());
	}

	TEST(GenesisStartupLogoBoundaryTests, SonicTitleAutotuneCanBeDisabledByEnv) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnvAndTitle({
			{ "NEXEN_GENESIS_STARTUP_ROM_AUTOTUNE", "0" }
		}, "SONIC THE HEDGEHOG");

		EXPECT_FALSE(memoryManager.GetStartupTitleAutotuneApplied());
		EXPECT_EQ(memoryManager.GetStartupWindowFrames(), 16u);
		EXPECT_EQ(memoryManager.GetStartupLogoPhaseEndFrame(), 120u);
		EXPECT_FALSE(memoryManager.GetStartupProfilePreferMesenBusHandoff());
	}

	TEST(GenesisStartupLogoBoundaryTests, SonicTitleAutotuneDoesNotOverrideExplicitProfileSelection) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnvAndTitle({
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "strict" }
		}, "SONIC THE HEDGEHOG");

		EXPECT_FALSE(memoryManager.GetStartupTitleAutotuneApplied());
		EXPECT_EQ(memoryManager.GetStartupWindowFrames(), 0u);
		EXPECT_EQ(memoryManager.GetStartupLogoPhaseEndFrame(), 0u);
		EXPECT_EQ(memoryManager.GetStartupStrictPhaseStartFrame(), 0u);
		EXPECT_TRUE(memoryManager.GetTmssStrictMode());
	}

	TEST(GenesisStartupLogoBoundaryTests, StartupTitleHintCanTriggerAutotuneWithoutSonicRomHeader) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithEnvAndTitle({
			{ "NEXEN_GENESIS_STARTUP_TITLE_HINT", "sonic 2" }
		}, "TEST PROGRAM");

		EXPECT_TRUE(memoryManager.GetStartupTitleAutotuneApplied());
		EXPECT_TRUE(memoryManager.GetStartupTitleHintUsed());
		EXPECT_EQ(memoryManager.GetStartupTitleClassValue(), 3u);
		EXPECT_GE(memoryManager.GetStartupLogoPhaseEndFrame(), 180u);
		EXPECT_GE(memoryManager.GetStartupStrictPhaseStartFrame(), 420u);
	}

	TEST(GenesisStartupLogoBoundaryTests, UnknownTitleDoesNotTriggerAutotune) {
		GenesisMemoryManager memoryManager = CreateMemoryManagerWithTitle("HOMEBREW DIAGNOSTIC");

		EXPECT_FALSE(memoryManager.GetStartupTitleAutotuneApplied());
		EXPECT_EQ(memoryManager.GetStartupTitleClassValue(), 0u);
		EXPECT_EQ(memoryManager.GetStartupWindowFrames(), 16u);
		EXPECT_EQ(memoryManager.GetStartupBootRelaxFrames(), 2u);
	}
}




