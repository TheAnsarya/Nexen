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

	struct Harness {
		std::vector<ScopedEnvVar> Env;
		std::vector<std::unique_ptr<Emulator>> OwnedEmulators;
		GenesisMemoryManager Memory;

		Harness(const std::vector<std::pair<const char*, const char*>>& vars)
			: Env(), OwnedEmulators(), Memory() {
			Env.reserve(64);
			OwnedEmulators.reserve(2);
			ResetTrackedEnv();
			for (const auto& [name, value] : vars) {
				Env.emplace_back(name, value);
			}

			auto emu = std::make_unique<Emulator>();
			emu->Initialize(false);
			std::vector<uint8_t> romData(0x400000);
			Memory.Init(emu.get(), nullptr, romData, nullptr, nullptr, nullptr);
			Memory.ResetRuntimeState(true);
			OwnedEmulators.push_back(std::move(emu));
		}

		void ResetTrackedEnv() {
			Env.emplace_back("NEXEN_GENESIS_STARTUP_PROFILE", nullptr);
			Env.emplace_back("NEXEN_GENESIS_STARTUP_WINDOW_FRAMES", nullptr);
			Env.emplace_back("NEXEN_GENESIS_STARTUP_BOOT_RELAX_FRAMES", nullptr);
			Env.emplace_back("NEXEN_GENESIS_STARTUP_LOGO_PHASE_END_FRAME", nullptr);
			Env.emplace_back("NEXEN_GENESIS_STARTUP_STRICT_PHASE_START_FRAME", nullptr);
			Env.emplace_back("NEXEN_GENESIS_STARTUP_CHECKPOINT_INTERVAL_FRAMES", nullptr);
			Env.emplace_back("NEXEN_GENESIS_STARTUP_CHECKPOINT_END_FRAME", nullptr);
			Env.emplace_back("NEXEN_GENESIS_TMSS_STRICT", nullptr);
			Env.emplace_back("NEXEN_GENESIS_TMSS_UNLOCK_DELAY_MCLK", nullptr);
			Env.emplace_back("NEXEN_GENESIS_TMSS_STRICT_DURING_LOGO", nullptr);
			Env.emplace_back("NEXEN_GENESIS_TMSS_FORCE_UNTIL_UNLOCK", nullptr);
			Env.emplace_back("NEXEN_GENESIS_Z80_BUSREQ_ACK_DELAY_MCLK", nullptr);
			Env.emplace_back("NEXEN_GENESIS_Z80_BUSRESUME_DELAY_MCLK", nullptr);
			Env.emplace_back("NEXEN_GENESIS_Z80_EARLY_BUSREQ_ACK_DELAY_MCLK", nullptr);
			Env.emplace_back("NEXEN_GENESIS_Z80_EARLY_BUSRESUME_DELAY_MCLK", nullptr);
			Env.emplace_back("NEXEN_GENESIS_Z80_LATE_BUSREQ_ACK_DELAY_MCLK", nullptr);
			Env.emplace_back("NEXEN_GENESIS_Z80_LATE_BUSRESUME_DELAY_MCLK", nullptr);
			Env.emplace_back("NEXEN_GENESIS_USE_DYNAMIC_BUS_TIMING", nullptr);
			Env.emplace_back("NEXEN_GENESIS_Z80_LATCH_HIGH_BYTE_ONLY", nullptr);
			Env.emplace_back("NEXEN_GENESIS_PREFER_NEXENREF_BUS_HANDOFF", nullptr);
			Env.emplace_back("NEXEN_GENESIS_PREFER_MESEN_BUS_HANDOFF", nullptr);
			Env.emplace_back("NEXEN_GENESIS_POWERON_Z80_RESET_ASSERTED", nullptr);
		}
	};

	void RunBusHandoffWarmup(GenesisMemoryManager& memory, uint32_t iterationCount) {
		for (uint32_t i = 0; i < iterationCount; i++) {
			memory.Write8(0xA11200, 0x01);
			memory.Write8(0xA11100, 0x01);
			memory.Write8(0xA11100, 0x00);
			memory.Write8(0xA11200, 0x00);
			memory.Write8(0xA11200, 0x01);
		}
	}

	TEST(GenesisStartupLogoStressTests, StartupProfileMatrixMaintainsDeterministicCoreSettings) {
		struct ProfileCase {
			const char* Name;
			const char* Profile;
			uint32_t ExpectedWindow;
			bool ExpectedPreferNexen;
		};

		const std::array<ProfileCase, 5> cases = {
			ProfileCase{ "logo-compat", "logo-compat", 16u, true },
			ProfileCase{ "nexen-ref", "nexen-ref", 10u, true },
			ProfileCase{ "strict", "strict", 0u, false },
			ProfileCase{ "mesen", "mesen", 0u, false },
			ProfileCase{ "hybrid", "hybrid", 6u, true }
		};

		for (const auto& profileCase : cases) {
			std::vector<std::pair<const char*, const char*>> envVars = {
				{ "NEXEN_GENESIS_STARTUP_PROFILE", profileCase.Profile }
			};
			Harness harness(envVars);

			EXPECT_EQ(harness.Memory.GetStartupWindowFrames(), profileCase.ExpectedWindow) << profileCase.Name;
			EXPECT_EQ(harness.Memory.GetStartupProfilePreferNexenBusHandoff(), profileCase.ExpectedPreferNexen) << profileCase.Name;
			if (std::string(profileCase.Profile) == "strict") {
				EXPECT_EQ(harness.Memory.GetStartupCheckpointEndFrame(), 120u) << profileCase.Name;
			} else {
				EXPECT_EQ(harness.Memory.GetStartupCheckpointEndFrame(), 600u) << profileCase.Name;
			}
		}
	}

	TEST(GenesisStartupLogoStressTests, RepeatedBusHandoffLoopsKeepRuntimeCountersMonotonicAndStable) {
		std::vector<std::pair<const char*, const char*>> envVars = {
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "nexen-ref" },
			{ "NEXEN_GENESIS_Z80_BUSREQ_ACK_DELAY_MCLK", "9" },
			{ "NEXEN_GENESIS_Z80_BUSRESUME_DELAY_MCLK", "9" }
		};
		Harness harness(envVars);

		uint64_t initialTransitions = harness.Memory.GetZ80RuntimeTransitionCount();
		RunBusHandoffWarmup(harness.Memory, 64);

		EXPECT_GE(harness.Memory.GetZ80RuntimeTransitionCount(), initialTransitions + 64u);
		EXPECT_GT(harness.Memory.GetZ80RuntimeStateEpoch(), 0u);
		EXPECT_GE(harness.Memory.GetZ80RuntimeLastTransitionClock(), 0u);
	}

	TEST(GenesisStartupLogoStressTests, DelayProfilesDriveExpectedRequestAndResumeLatchesAcrossWarmup) {
		const std::array<uint16_t, 4> delayValues = { 1u, 3u, 7u, 15u };
		for (uint16_t delay : delayValues) {
			std::string delayText = std::to_string(delay);
			std::vector<std::pair<const char*, const char*>> envVars = {
				{ "NEXEN_GENESIS_Z80_BUSREQ_ACK_DELAY_MCLK", delayText.c_str() },
				{ "NEXEN_GENESIS_Z80_BUSRESUME_DELAY_MCLK", delayText.c_str() }
			};
			Harness harness(envVars);

			harness.Memory.Write8(0xA11200, 0x01);
			harness.Memory.Write8(0xA11100, 0x01);
			EXPECT_EQ(harness.Memory.GetZ80BusReqDelayMclk(), delay);

			harness.Memory.Write8(0xA11100, 0x00);
			EXPECT_EQ(harness.Memory.GetZ80ResumeDelayMclk(), delay);
		}
	}

	TEST(GenesisStartupLogoStressTests, MesenCompatibleDelayProfileMaintainsExpectedHandshakeDelays) {
		std::vector<std::pair<const char*, const char*>> envVars = {
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "mesen-compat" }
		};
		Harness harness(envVars);

		harness.Memory.Write8(0xA11200, 0x01);
		harness.Memory.Write8(0xA11100, 0x01);
		EXPECT_EQ(harness.Memory.GetZ80BusReqDelayMclk(), 45u);

		harness.Memory.Write8(0xA11100, 0x00);
		EXPECT_EQ(harness.Memory.GetZ80ResumeDelayMclk(), 15u);
	}

	TEST(GenesisStartupLogoStressTests, HybridProfileAcceptsPhaseDelayOverridesWithoutLosingDeterminism) {
		auto runScenario = []() {
			std::vector<std::pair<const char*, const char*>> envVars = {
				{ "NEXEN_GENESIS_STARTUP_PROFILE", "hybrid" },
				{ "NEXEN_GENESIS_USE_DYNAMIC_BUS_TIMING", "1" },
				{ "NEXEN_GENESIS_Z80_EARLY_BUSREQ_ACK_DELAY_MCLK", "33" },
				{ "NEXEN_GENESIS_Z80_EARLY_BUSRESUME_DELAY_MCLK", "17" },
				{ "NEXEN_GENESIS_Z80_LATE_BUSREQ_ACK_DELAY_MCLK", "9" },
				{ "NEXEN_GENESIS_Z80_LATE_BUSRESUME_DELAY_MCLK", "7" }
			};
			Harness harness(envVars);

			RunBusHandoffWarmup(harness.Memory, 128);
			return std::tuple<uint64_t, uint64_t, uint16_t, uint16_t>(
				harness.Memory.GetZ80RuntimeTransitionCount(),
				harness.Memory.GetZ80RuntimeStateEpoch(),
				harness.Memory.GetZ80BusReqAckDelaySettingMclk(),
				harness.Memory.GetZ80BusResumeDelaySettingMclk());
		};

		auto runA = runScenario();
		auto runB = runScenario();
		EXPECT_EQ(runA, runB);
		EXPECT_EQ(std::get<2>(runA), 33u);
		EXPECT_EQ(std::get<3>(runA), 17u);
	}

	TEST(GenesisStartupLogoStressTests, StartupPhaseOverrideStressKeepsCoreRuntimeStateStable) {
		std::vector<std::pair<const char*, const char*>> envVars = {
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "hybrid" },
			{ "NEXEN_GENESIS_STARTUP_BOOT_RELAX_FRAMES", "12" },
			{ "NEXEN_GENESIS_STARTUP_LOGO_PHASE_END_FRAME", "240" },
			{ "NEXEN_GENESIS_STARTUP_STRICT_PHASE_START_FRAME", "480" },
			{ "NEXEN_GENESIS_TMSS_UNLOCK_DELAY_MCLK", "30" },
			{ "NEXEN_GENESIS_TMSS_FORCE_UNTIL_UNLOCK", "1" }
		};
		Harness harness(envVars);

		RunBusHandoffWarmup(harness.Memory, 96);
		EXPECT_EQ(harness.Memory.GetStartupCheckpointEndFrame(), 600u);
		EXPECT_GE(harness.Memory.GetZ80RuntimeTransitionCount(), 96u);
		EXPECT_GT(harness.Memory.GetZ80RuntimeStateEpoch(), 0u);
	}

	TEST(GenesisStartupLogoStressTests, HighByteOnlyLatchPolicyPreventsOddAddressNoiseDuringStartup) {
		std::vector<std::pair<const char*, const char*>> envVars = {
			{ "NEXEN_GENESIS_Z80_LATCH_HIGH_BYTE_ONLY", "1" }
		};
		Harness harness(envVars);

		for (uint32_t i = 0; i < 128; i++) {
			harness.Memory.Write8(0xA11101, 0x01);
			harness.Memory.Write8(0xA11201, 0x01);
		}

		EXPECT_FALSE(harness.Memory.GetZ80BusRequestLatched());
		EXPECT_TRUE(harness.Memory.GetZ80ResetAsserted());

		harness.Memory.Write8(0xA11100, 0x01);
		harness.Memory.Write8(0xA11200, 0x01);
		EXPECT_TRUE(harness.Memory.GetZ80BusRequestLatched());
		EXPECT_FALSE(harness.Memory.GetZ80ResetAsserted());
	}

	TEST(GenesisStartupLogoStressTests, ByteAgnosticLatchPolicyAllowsOddAddressTransitionsForDebugTuning) {
		std::vector<std::pair<const char*, const char*>> envVars = {
			{ "NEXEN_GENESIS_Z80_LATCH_HIGH_BYTE_ONLY", "0" }
		};
		Harness harness(envVars);

		harness.Memory.Write8(0xA11101, 0x01);
		harness.Memory.Write8(0xA11201, 0x01);
		EXPECT_TRUE(harness.Memory.GetZ80BusRequestLatched());
		EXPECT_FALSE(harness.Memory.GetZ80ResetAsserted());
	}

	TEST(GenesisStartupLogoStressTests, DebugBusSequencesMirrorRuntimeLatchingPolicyAndDoNotRegress) {
		std::vector<std::pair<const char*, const char*>> envVars = {
			{ "NEXEN_GENESIS_Z80_LATCH_HIGH_BYTE_ONLY", "1" },
			{ "NEXEN_GENESIS_Z80_BUSREQ_ACK_DELAY_MCLK", "5" },
			{ "NEXEN_GENESIS_Z80_BUSRESUME_DELAY_MCLK", "6" }
		};
		Harness harness(envVars);

		for (uint32_t i = 0; i < 32; i++) {
			harness.Memory.DebugWrite8(0xA11200, 0x01);
			harness.Memory.DebugWrite8(0xA11100, 0x01);
			EXPECT_EQ(harness.Memory.GetZ80BusReqDelayMclk(), 5u);

			harness.Memory.DebugWrite8(0xA11100, 0x00);
			EXPECT_EQ(harness.Memory.GetZ80ResumeDelayMclk(), 6u);
			harness.Memory.DebugWrite8(0xA11200, 0x00);
			harness.Memory.DebugWrite8(0xA11200, 0x01);
		}

		EXPECT_GT(harness.Memory.GetZ80RuntimeTransitionCount(), 0u);
	}

	TEST(GenesisStartupLogoStressTests, StartupWindowOverridesSupportAggressiveFirstTenSecondSweeps) {
		std::vector<std::pair<const char*, const char*>> envVars = {
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "nexen-ref" },
			{ "NEXEN_GENESIS_STARTUP_WINDOW_FRAMES", "60" },
			{ "NEXEN_GENESIS_STARTUP_CHECKPOINT_INTERVAL_FRAMES", "2" },
			{ "NEXEN_GENESIS_STARTUP_CHECKPOINT_END_FRAME", "600" }
		};
		Harness harness(envVars);

		EXPECT_EQ(harness.Memory.GetStartupWindowFrames(), 60u);
		EXPECT_EQ(harness.Memory.GetStartupCheckpointIntervalFrames(), 2u);
		EXPECT_EQ(harness.Memory.GetStartupCheckpointEndFrame(), 600u);
		EXPECT_TRUE(harness.Memory.GetStartupProfilePreferNexenBusHandoff());
	}

	TEST(GenesisStartupLogoStressTests, StartupConfigurationStressMatrixRemainsDeterministicAcrossTwoHarnesses) {
		auto runScenario = []() {
			std::vector<std::pair<const char*, const char*>> envVars = {
				{ "NEXEN_GENESIS_STARTUP_PROFILE", "logo-compat" },
				{ "NEXEN_GENESIS_STARTUP_WINDOW_FRAMES", "32" },
				{ "NEXEN_GENESIS_Z80_BUSREQ_ACK_DELAY_MCLK", "8" },
				{ "NEXEN_GENESIS_Z80_BUSRESUME_DELAY_MCLK", "8" },
				{ "NEXEN_GENESIS_Z80_LATCH_HIGH_BYTE_ONLY", "1" }
			};
			Harness harness(envVars);

			RunBusHandoffWarmup(harness.Memory, 96);
			return std::tuple<uint64_t, uint64_t, uint16_t, uint16_t, bool>(
				harness.Memory.GetZ80RuntimeTransitionCount(),
				harness.Memory.GetZ80RuntimeStateEpoch(),
				harness.Memory.GetZ80BusReqAckDelaySettingMclk(),
				harness.Memory.GetZ80BusResumeDelaySettingMclk(),
				harness.Memory.GetStartupProfilePreferNexenBusHandoff());
		};

		auto runA = runScenario();
		auto runB = runScenario();
		EXPECT_EQ(runA, runB);
	}

	TEST(GenesisStartupLogoStressTests, HybridDynamicDelaysAreMonotonicAcrossStartupSweep) {
		std::vector<std::pair<const char*, const char*>> envVars = {
			{ "NEXEN_GENESIS_STARTUP_PROFILE", "hybrid" },
			{ "NEXEN_GENESIS_USE_DYNAMIC_BUS_TIMING", "1" },
			{ "NEXEN_GENESIS_STARTUP_LOGO_PHASE_END_FRAME", "120" },
			{ "NEXEN_GENESIS_STARTUP_STRICT_PHASE_START_FRAME", "360" },
			{ "NEXEN_GENESIS_Z80_EARLY_BUSREQ_ACK_DELAY_MCLK", "45" },
			{ "NEXEN_GENESIS_Z80_EARLY_BUSRESUME_DELAY_MCLK", "15" },
			{ "NEXEN_GENESIS_Z80_LATE_BUSREQ_ACK_DELAY_MCLK", "7" },
			{ "NEXEN_GENESIS_Z80_LATE_BUSRESUME_DELAY_MCLK", "7" }
		};
		Harness harness(envVars);

		uint16_t prevAck = harness.Memory.GetEffectiveZ80BusReqAckDelayForFrame(0u);
		uint16_t prevResume = harness.Memory.GetEffectiveZ80BusResumeDelayForFrame(0u);
		for (uint32_t frame = 20u; frame <= 420u; frame += 20u) {
			uint16_t ack = harness.Memory.GetEffectiveZ80BusReqAckDelayForFrame(frame);
			uint16_t resume = harness.Memory.GetEffectiveZ80BusResumeDelayForFrame(frame);
			EXPECT_LE(ack, prevAck);
			EXPECT_LE(resume, prevResume);
			prevAck = ack;
			prevResume = resume;
		}

		EXPECT_EQ(harness.Memory.GetEffectiveZ80BusReqAckDelayForFrame(480u), 7u);
		EXPECT_EQ(harness.Memory.GetEffectiveZ80BusResumeDelayForFrame(480u), 7u);
	}

	TEST(GenesisStartupLogoStressTests, StartupProfileParityMaintainsExpectedEarlyBootHandshakeTargets) {
		struct PhaseParityCase {
			const char* Profile;
			uint16_t EarlyAck;
			uint16_t EarlyResume;
			uint16_t StrictAck;
			uint16_t StrictResume;
		};

		const std::array<PhaseParityCase, 4> cases = {
			PhaseParityCase{ "logo-compat", 7u, 7u, 7u, 7u },
			PhaseParityCase{ "nexen-ref", 7u, 7u, 7u, 7u },
			PhaseParityCase{ "mesen-compat", 45u, 15u, 45u, 15u },
			PhaseParityCase{ "strict", 45u, 15u, 45u, 15u }
		};

		for (const auto& profileCase : cases) {
			std::vector<std::pair<const char*, const char*>> envVars = {
				{ "NEXEN_GENESIS_STARTUP_PROFILE", profileCase.Profile }
			};
			Harness harness(envVars);
			EXPECT_EQ(harness.Memory.GetEffectiveZ80BusReqAckDelayForFrame(0u), profileCase.EarlyAck) << profileCase.Profile;
			EXPECT_EQ(harness.Memory.GetEffectiveZ80BusResumeDelayForFrame(0u), profileCase.EarlyResume) << profileCase.Profile;
			EXPECT_EQ(harness.Memory.GetEffectiveZ80BusReqAckDelayForFrame(600u), profileCase.StrictAck) << profileCase.Profile;
			EXPECT_EQ(harness.Memory.GetEffectiveZ80BusResumeDelayForFrame(600u), profileCase.StrictResume) << profileCase.Profile;
		}
	}

	TEST(GenesisStartupLogoStressTests, DynamicRetuneSweepIsDeterministicAcrossIdenticalHarnessRuns) {
		auto runRetuneScenario = []() {
			Harness harness({
				{ "NEXEN_GENESIS_STARTUP_PROFILE", "hybrid" },
				{ "NEXEN_GENESIS_USE_DYNAMIC_BUS_TIMING", "1" },
				{ "NEXEN_GENESIS_STARTUP_LOGO_PHASE_END_FRAME", "90" },
				{ "NEXEN_GENESIS_STARTUP_STRICT_PHASE_START_FRAME", "270" },
				{ "NEXEN_GENESIS_Z80_EARLY_BUSREQ_ACK_DELAY_MCLK", "39" },
				{ "NEXEN_GENESIS_Z80_EARLY_BUSRESUME_DELAY_MCLK", "17" },
				{ "NEXEN_GENESIS_Z80_LATE_BUSREQ_ACK_DELAY_MCLK", "9" },
				{ "NEXEN_GENESIS_Z80_LATE_BUSRESUME_DELAY_MCLK", "7" }
			});

			for (uint32_t frame = 0u; frame <= 360u; frame += 30u) {
				harness.Memory.RefreshStartupBusTimingForFrame(frame);
			}

			return std::tuple<uint32_t, uint32_t, uint16_t, uint16_t>(
				harness.Memory.GetStartupBusTimingRetuneCount(),
				harness.Memory.GetStartupLastBusTimingFrame(),
				harness.Memory.GetZ80BusReqAckDelaySettingMclk(),
				harness.Memory.GetZ80BusResumeDelaySettingMclk());
		};

		auto runA = runRetuneScenario();
		auto runB = runRetuneScenario();
		EXPECT_EQ(runA, runB);
		EXPECT_GE(std::get<0>(runA), 2u);
		EXPECT_EQ(std::get<1>(runA), 270u);
		EXPECT_EQ(std::get<2>(runA), 9u);
		EXPECT_EQ(std::get<3>(runA), 7u);
	}
}




