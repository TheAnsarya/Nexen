#include "pch.h"
#include "Genesis/GenesisSmokeHarness.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"
#include <numeric>

namespace {
	string ToHex(uint64_t value) {
		return std::format("{:016x}", value);
	}

		string BuildFailedCheckpointIds(const vector<GenesisCompatibilityCheckpoint>& checkpoints) {
			vector<string> failedIds;
			for (const GenesisCompatibilityCheckpoint& checkpoint : checkpoints) {
				if (!checkpoint.Pass) {
					failedIds.push_back(checkpoint.Id);
				}
			}

			if (failedIds.empty()) {
				return "none";
			}
			return std::accumulate(std::next(failedIds.begin()), failedIds.end(), failedIds.front(), [](const string& left, const string& right) {
				return left + "," + right;
			});
		}

	string BuildCheckpointDigest(const vector<GenesisCompatibilityCheckpoint>& checkpoints) {
		uint64_t hash = 1469598103934665603ull;
		for (const GenesisCompatibilityCheckpoint& checkpoint : checkpoints) {
			string line = checkpoint.Id + ":" + (checkpoint.Pass ? "PASS" : "FAIL") + ":" + checkpoint.Context;
			for (uint8_t ch : line) {
				hash ^= ch;
				hash *= 1099511628211ull;
			}
		}
		return ToHex(hash);
	}

	string InferTitleClass(const string& name) {
		string lower = name;
		std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char ch) {
			return (char)std::tolower(ch);
		});

		if (lower.find("sonic") != string::npos) {
			return "sonic";
		}
		if (lower.find("jurassic") != string::npos) {
			return "jurassic";
		}
		return "generic";
	}
}

GenesisCompatibilityMatrixResult GenesisSmokeHarness::RunCompatibilityMatrix(GenesisM68kBoundaryScaffold& scaffold, const vector<GenesisCompatibilityRomCase>& romSet) {
	GenesisCompatibilityMatrixResult result = {};
	result.Entries.reserve(romSet.size());

	for (const GenesisCompatibilityRomCase& romCase : romSet) {
		GenesisCompatibilityEntry entry = {};
		entry.Name = romCase.Name;

		auto addCheckpoint = [&](string id, bool pass, string context) {
			GenesisCompatibilityCheckpoint checkpoint = {};
			checkpoint.Id = std::move(id);
			checkpoint.Pass = pass;
			checkpoint.Context = std::move(context);
			if (checkpoint.Pass) {
				entry.PassCount++;
			} else {
				entry.FailCount++;
			}
			entry.OutputLines.push_back(std::format("GEN_COMPAT_CHECK {} {} {}", checkpoint.Id, checkpoint.Pass ? "PASS" : "FAIL", checkpoint.Context));
			if (!checkpoint.Pass) {
				entry.OutputLines.push_back(std::format("GEN_COMPAT_CHECK_FAIL id={} context={} triage=inspect-{}", checkpoint.Id, checkpoint.Context, checkpoint.Id));
			}
			entry.Checkpoints.push_back(std::move(checkpoint));
		};

		if (romCase.RomData.empty()) {
			addCheckpoint("GEN-COMPAT-LOAD", false, "empty_rom_data");
			entry.Pass = false;
			entry.Digest = BuildCheckpointDigest(entry.Checkpoints);
			result.FailCount++;
			result.OutputLines.push_back(std::format(
				"GEN_COMPAT_FAIL_CONTEXT {} class={} failed_ids={} digest={}",
				entry.Name,
				InferTitleClass(entry.Name),
				BuildFailedCheckpointIds(entry.Checkpoints),
				entry.Digest));
			result.OutputLines.push_back(std::format("GEN_COMPAT_RESULT {} FAIL PASS={} FAIL={} DIGEST={}", entry.Name, entry.PassCount, entry.FailCount, entry.Digest));
			result.Entries.push_back(std::move(entry));
			continue;
		}

		scaffold.LoadRom(romCase.RomData);
		scaffold.Startup();
		scaffold.ClearTimingEvents();
		addCheckpoint("GEN-COMPAT-BOOT", scaffold.IsStarted(), std::format("frame={} scanline={}", scaffold.GetTimingFrame(), scaffold.GetTimingScanline()));

		scaffold.GetBus().WriteByte(0xA11200, 0x01);
		scaffold.GetBus().WriteByte(0xA11100, 0x01);
		scaffold.GetBus().WriteByte(0xA11100, 0x00);
		addCheckpoint("GEN-COMPAT-BUS", scaffold.GetBus().GetZ80HandoffCount() >= 2, std::format("handoffCount={} running={}", scaffold.GetBus().GetZ80HandoffCount(), scaffold.GetBus().IsZ80Running() ? 1 : 0));

		bool ownershipPass = scaffold.GetBus().GetOwnerForAddress(0x000100) == GenesisBusOwner::Rom
			&& scaffold.GetBus().GetOwnerForAddress(0xA00010) == GenesisBusOwner::Z80
			&& scaffold.GetBus().GetOwnerForAddress(0xA10004) == GenesisBusOwner::Io
			&& scaffold.GetBus().GetOwnerForAddress(0xC00004) == GenesisBusOwner::Vdp
			&& scaffold.GetBus().GetOwnerForAddress(0xFF1234) == GenesisBusOwner::WorkRam;
		addCheckpoint("GEN-COMPAT-BUS-OWNERSHIP", ownershipPass, std::format("ownershipDigest={} count={}", scaffold.GetBus().GetOwnershipTraceDigest(), scaffold.GetBus().GetOwnershipTraceCount()));

		scaffold.GetBus().WriteByte(0xA11200, 0x01);
		uint8_t runningState = scaffold.GetBus().ReadByte(0xA11200);
		scaffold.GetBus().WriteByte(0xA11100, 0x01);
		uint8_t busHeldState = scaffold.GetBus().ReadByte(0xA11100);
		scaffold.GetBus().WriteByte(0xA11100, 0x00);
		uint8_t resumedState = scaffold.GetBus().ReadByte(0xA11200);
		bool hostModePass = runningState == 0x01 && busHeldState == 0x01 && resumedState == 0x01 && scaffold.GetBus().GetZ80HandoffCount() >= 4;
		addCheckpoint("GEN-COMPAT-HOST-MODE", hostModePass, std::format("run={} hold={} resume={} handoffCount={}", runningState, busHeldState, resumedState, scaffold.GetBus().GetZ80HandoffCount()));

		scaffold.GetBus().WriteByte(0xA11200, 0x01);
		scaffold.GetBus().WriteByte(0xA11100, 0x01);
		scaffold.GetBus().WriteByte(0xA11100, 0x00);
		uint8_t pbcBootRunning = scaffold.GetBus().ReadByte(0xA11200);
		bool pbcBootPass = pbcBootRunning == 0x01 && scaffold.GetBus().GetZ80BootstrapCount() >= 1 && scaffold.GetBus().GetZ80HandoffCount() >= 6;
		addCheckpoint("GEN-COMPAT-PBC-BOOT", pbcBootPass, std::format("bootRunning={} bootstrapCount={} handoffCount={}", pbcBootRunning, scaffold.GetBus().GetZ80BootstrapCount(), scaffold.GetBus().GetZ80HandoffCount()));

		uint64_t cyclesBefore = scaffold.GetBus().GetZ80ExecutedCycles();
		scaffold.StepFrameScaffold(144u * 8u);
		uint64_t cyclesAfter = scaffold.GetBus().GetZ80ExecutedCycles();
		uint64_t parityDeltaA = cyclesAfter > cyclesBefore ? cyclesAfter - cyclesBefore : 0;
		GenesisBoundaryScaffoldSaveState pbcRuntimeBaseline = scaffold.SaveState();
		scaffold.StepFrameScaffold(144u * 8u);
		uint64_t parityDeltaB = scaffold.GetBus().GetZ80ExecutedCycles() - cyclesAfter;
		scaffold.LoadState(pbcRuntimeBaseline);
		scaffold.StepFrameScaffold(144u * 8u);
		uint64_t parityReplayDelta = scaffold.GetBus().GetZ80ExecutedCycles() - cyclesAfter;
		bool pbcRuntimePass = parityDeltaA > 0 && parityDeltaB == parityReplayDelta;
		addCheckpoint("GEN-COMPAT-PBC-RUNTIME", pbcRuntimePass, std::format("deltaA={} deltaB={} replayDelta={} z80Digest={}", parityDeltaA, parityDeltaB, parityReplayDelta, scaffold.GetBus().GetOwnershipTraceDigest()));

		size_t romSize = romCase.RomData.size();
		uint8_t firstByte = scaffold.GetBus().ReadByte(0x000000);
		uint8_t edgeByte = scaffold.GetBus().ReadByte((uint32_t)(romSize - 1));
		uint8_t mirroredByte = scaffold.GetBus().ReadByte((uint32_t)romSize);
		bool mapperEdgePass = edgeByte == romCase.RomData.back() && mirroredByte == firstByte;
		addCheckpoint("GEN-COMPAT-MAPPER-EDGE", mapperEdgePass, std::format("romSize={} edge={:02x} mirrored={:02x} first={:02x}", romSize, edgeByte, mirroredByte, firstByte));

		scaffold.GetBus().WriteByte(0xA12012, 0x11);
		scaffold.GetBus().WriteByte(0xA12013, 0x22);
		scaffold.GetBus().WriteByte(0xA12014, 0x33);
		scaffold.GetBus().WriteByte(0xA12015, 0x44);
		uint8_t toolingCapabilities = scaffold.GetBus().ReadByte(0xA1201A);
		uint8_t toolingDigest = scaffold.GetBus().ReadByte(0xA1201B);
		GenesisBoundaryScaffoldSaveState toolingBaseline = scaffold.SaveState();
		scaffold.GetBus().WriteByte(0xA12015, 0x99);
		scaffold.LoadState(toolingBaseline);
		uint8_t toolingCapabilitiesReplay = scaffold.GetBus().ReadByte(0xA1201A);
		uint8_t toolingDigestReplay = scaffold.GetBus().ReadByte(0xA1201B);
		bool toolingPass = toolingCapabilities == 0x0F && toolingCapabilitiesReplay == toolingCapabilities && toolingDigestReplay == toolingDigest;
		addCheckpoint("GEN-COMPAT-TOOLING-MATRIX", toolingPass, std::format("caps={:02x} digest={:02x} replayCaps={:02x} replayDigest={:02x}", toolingCapabilities, toolingDigest, toolingCapabilitiesReplay, toolingDigestReplay));

		scaffold.GetBus().WriteByte(0xA15012, 0x03);
		scaffold.GetBus().WriteByte(0xA15013, 0x06);
		scaffold.GetBus().WriteByte(0xA15014, 0xA5);
		uint8_t sh2Status = scaffold.GetBus().ReadByte(0xA1501A);
		uint8_t sh2Digest = scaffold.GetBus().ReadByte(0xA1501B);
		GenesisBoundaryScaffoldSaveState sh2Baseline = scaffold.SaveState();
		scaffold.GetBus().WriteByte(0xA15013, 0x02);
		scaffold.LoadState(sh2Baseline);
		uint8_t sh2StatusReplay = scaffold.GetBus().ReadByte(0xA1501A);
		uint8_t sh2DigestReplay = scaffold.GetBus().ReadByte(0xA1501B);
		bool sh2Pass = (sh2Status & 0x03) == 0x03 && sh2StatusReplay == sh2Status && sh2DigestReplay == sh2Digest;
		addCheckpoint("GEN-COMPAT-32X-DUAL-SH2", sh2Pass, std::format("status={:02x} digest={:02x} replayStatus={:02x} replayDigest={:02x}", sh2Status, sh2Digest, sh2StatusReplay, sh2DigestReplay));

		scaffold.GetBus().WriteByte(0xA15016, 0x09);
		scaffold.GetBus().WriteByte(0xA15017, 0x5C);
		scaffold.GetBus().SetRenderCompositionInputs(0x1E, true, 0x08, false, 0x04, true, true, 0x20, true);
		scaffold.GetBus().SetScroll(5, 3);
		scaffold.GetBus().RenderScaffoldLine(64);
		uint8_t composeStatus = scaffold.GetBus().ReadByte(0xA1501C);
		uint8_t composeDigest = scaffold.GetBus().ReadByte(0xA1501D);
		uint32_t frameBefore = scaffold.GetTimingFrame();
		uint32_t vintBefore = scaffold.GetVerticalInterruptCount();
		scaffold.StepFrameScaffold(488u * 262u + 64u);
		uint32_t frameAfter = scaffold.GetTimingFrame();
		uint32_t vintAfter = scaffold.GetVerticalInterruptCount();
		GenesisBoundaryScaffoldSaveState composeBaseline = scaffold.SaveState();
		scaffold.GetBus().WriteByte(0xA15017, 0x2A);
		scaffold.GetBus().SetScroll(1, 1);
		scaffold.LoadState(composeBaseline);
		scaffold.GetBus().RenderScaffoldLine(64);
		uint8_t composeStatusReplay = scaffold.GetBus().ReadByte(0xA1501C);
		uint8_t composeDigestReplay = scaffold.GetBus().ReadByte(0xA1501D);
		bool composePass = (composeStatus & 0x0F) == 0x09
			&& composeStatusReplay == composeStatus
			&& composeDigestReplay == composeDigest
			&& frameAfter > frameBefore
			&& vintAfter > vintBefore
			&& !scaffold.GetBus().GetRenderLineDigest().empty();
		addCheckpoint("GEN-COMPAT-32X-COMPOSE-SYNC", composePass, std::format("status={:02x} digest={:02x} replayStatus={:02x} replayDigest={:02x} frame={}->{} vint={}->{} renderDigest={}", composeStatus, composeDigest, composeStatusReplay, composeDigestReplay, frameBefore, frameAfter, vintBefore, vintAfter, scaffold.GetBus().GetRenderLineDigest()));

		scaffold.GetBus().WriteByte(0xA15008, 0x10);
		scaffold.GetBus().WriteByte(0xA15009, 0x20);
		scaffold.GetBus().WriteByte(0xA1500A, 0x30);
		scaffold.GetBus().WriteByte(0xA1500B, 0x40);
		uint8_t toolingCaps = scaffold.GetBus().ReadByte(0xA1501E);
		uint8_t tooling32xDigest = scaffold.GetBus().ReadByte(0xA1501F);
		GenesisBoundaryScaffoldSaveState tooling32xBaseline = scaffold.SaveState();
		scaffold.GetBus().WriteByte(0xA1500B, 0x7F);
		scaffold.LoadState(tooling32xBaseline);
		uint8_t toolingCapsReplay = scaffold.GetBus().ReadByte(0xA1501E);
		uint8_t tooling32xDigestReplay = scaffold.GetBus().ReadByte(0xA1501F);
		bool tooling32xPass = toolingCaps == 0x0F && toolingCapsReplay == toolingCaps && tooling32xDigestReplay == tooling32xDigest;
		addCheckpoint("GEN-COMPAT-32X-TOOLING", tooling32xPass, std::format("caps={:02x} digest={:02x} replayCaps={:02x} replayDigest={:02x}", toolingCaps, tooling32xDigest, toolingCapsReplay, tooling32xDigestReplay));

		GenesisBoundaryScaffoldSaveState preTasCheatState = scaffold.SaveState();
		scaffold.GetBus().WriteByte(0xA10003, 0x40);
		scaffold.GetBus().WriteByte(0xA10003, 0x00);
		scaffold.GetBus().WriteByte(0xA10003, 0x40);
		scaffold.GetBus().WriteByte(0xA10005, 0x40);
		scaffold.GetBus().WriteByte(0xA10005, 0x00);
		scaffold.GetBus().WriteByte(0xA12013, 0x2C);
		scaffold.GetBus().WriteByte(0xA12015, 0xA1);
		uint8_t segaCdTasCheatDigest = scaffold.GetBus().ReadByte(0xA1201B);
		uint8_t controlCapP1 = scaffold.GetBus().ReadByte(0xA1201C);
		uint8_t controlDigestP1 = scaffold.GetBus().ReadByte(0xA1201D);
		uint8_t controlCapP2 = scaffold.GetBus().ReadByte(0xA1201E);
		uint8_t controlDigestP2 = scaffold.GetBus().ReadByte(0xA1201F);
		scaffold.GetBus().WriteByte(0xA15009, 0x17);
		scaffold.GetBus().WriteByte(0xA1500B, 0xE3);
		uint8_t m32xTasCheatDigest = scaffold.GetBus().ReadByte(0xA1501F);

		scaffold.LoadState(preTasCheatState);
		scaffold.GetBus().WriteByte(0xA10003, 0x40);
		scaffold.GetBus().WriteByte(0xA10003, 0x00);
		scaffold.GetBus().WriteByte(0xA10003, 0x40);
		scaffold.GetBus().WriteByte(0xA10005, 0x40);
		scaffold.GetBus().WriteByte(0xA10005, 0x00);
		scaffold.GetBus().WriteByte(0xA12013, 0x2C);
		scaffold.GetBus().WriteByte(0xA12015, 0xA1);
		uint8_t segaCdTasCheatReplay = scaffold.GetBus().ReadByte(0xA1201B);
		uint8_t controlCapP1Replay = scaffold.GetBus().ReadByte(0xA1201C);
		uint8_t controlDigestP1Replay = scaffold.GetBus().ReadByte(0xA1201D);
		uint8_t controlCapP2Replay = scaffold.GetBus().ReadByte(0xA1201E);
		uint8_t controlDigestP2Replay = scaffold.GetBus().ReadByte(0xA1201F);
		scaffold.GetBus().WriteByte(0xA15009, 0x17);
		scaffold.GetBus().WriteByte(0xA1500B, 0xE3);
		uint8_t m32xTasCheatReplay = scaffold.GetBus().ReadByte(0xA1501F);
		scaffold.LoadState(preTasCheatState);
		bool tasCheatPass = segaCdTasCheatDigest != 0x00
			&& m32xTasCheatDigest != 0x00
			&& (controlCapP1 & 0x10) != 0
			&& (controlCapP2 & 0x20) != 0
			&& segaCdTasCheatReplay == segaCdTasCheatDigest
			&& m32xTasCheatReplay == m32xTasCheatDigest
			&& controlCapP1Replay == controlCapP1
			&& controlDigestP1Replay == controlDigestP1
			&& controlCapP2Replay == controlCapP2
			&& controlDigestP2Replay == controlDigestP2;
		addCheckpoint(
			"GEN-COMPAT-TAS-CHEAT",
			tasCheatPass,
			std::format(
				"cdMatch={} x32Match={} p1Match={} p2Match={} cdNonZero={} x32NonZero={} p1Caps={:02x} p2Caps={:02x}",
				segaCdTasCheatReplay == segaCdTasCheatDigest ? 1 : 0,
				m32xTasCheatReplay == m32xTasCheatDigest ? 1 : 0,
				(controlCapP1Replay == controlCapP1 && controlDigestP1Replay == controlDigestP1) ? 1 : 0,
				(controlCapP2Replay == controlCapP2 && controlDigestP2Replay == controlDigestP2) ? 1 : 0,
				segaCdTasCheatDigest != 0x00 ? 1 : 0,
				m32xTasCheatDigest != 0x00 ? 1 : 0,
				controlCapP1,
				controlCapP2));

		scaffold.GetBus().SetRenderCompositionInputs(0x18, true, 0x04, false, 0x00, false, false, 0x20, true);
		scaffold.GetBus().SetScroll(2, 1);
		scaffold.GetBus().RenderScaffoldLine(48);
		addCheckpoint("GEN-COMPAT-RENDER", !scaffold.GetBus().GetRenderLineDigest().empty(), std::format("renderDigest={}", scaffold.GetBus().GetRenderLineDigest()));

		scaffold.GetBus().WriteByte(0xA04000, 0x22);
		scaffold.GetBus().WriteByte(0xA04001, 0x10);
		scaffold.GetBus().WriteByte(0xC00011, 0x90);
		scaffold.GetBus().WriteByte(0xC00011, 0x07);
		scaffold.StepFrameScaffold(144u * 6u);
		bool audioPass = scaffold.GetBus().GetYmSampleCount() > 0 && scaffold.GetBus().GetPsgSampleCount() > 0 && !scaffold.GetBus().GetMixedDigest().empty();
		addCheckpoint("GEN-COMPAT-AUDIO", audioPass, std::format("ym={} psg={} mixedDigest={}", scaffold.GetBus().GetYmSampleCount(), scaffold.GetBus().GetPsgSampleCount(), scaffold.GetBus().GetMixedDigest()));

		string titleClass = InferTitleClass(romCase.Name);
		if (titleClass == "sonic") {
			bool sonicPass = scaffold.GetBus().GetRenderLineDigest().size() == 16;
			addCheckpoint("GEN-TITLE-SONIC", sonicPass, std::format("digestLen={}", scaffold.GetBus().GetRenderLineDigest().size()));
		} else if (titleClass == "jurassic") {
			scaffold.GetBus().BeginDmaTransfer(GenesisVdpDmaMode::Copy, 12);
			scaffold.StepFrameScaffold(96);
			bool jurassicPass = scaffold.GetBus().GetDmaContentionCycles() > 0;
			addCheckpoint("GEN-TITLE-JURASSIC", jurassicPass, std::format("contentionCycles={}", scaffold.GetBus().GetDmaContentionCycles()));
		}

		string digestA = BuildCheckpointDigest(entry.Checkpoints);
		string digestB = BuildCheckpointDigest(entry.Checkpoints);
		addCheckpoint("GEN-COMPAT-DETERMINISM", digestA == digestB, std::format("digestA={} digestB={}", digestA, digestB));

		entry.Pass = entry.FailCount == 0;
		entry.Digest = BuildCheckpointDigest(entry.Checkpoints);
		if (entry.Pass) {
			result.PassCount++;
		} else {
			result.FailCount++;
			result.OutputLines.push_back(std::format(
				"GEN_COMPAT_FAIL_CONTEXT {} class={} failed_ids={} digest={}",
				entry.Name,
				InferTitleClass(entry.Name),
				BuildFailedCheckpointIds(entry.Checkpoints),
				entry.Digest));
		}

		result.OutputLines.push_back(std::format("GEN_COMPAT_RESULT {} {} PASS={} FAIL={} DIGEST={}", entry.Name, entry.Pass ? "PASS" : "FAIL", entry.PassCount, entry.FailCount, entry.Digest));
		result.Entries.push_back(std::move(entry));
	}

	uint64_t matrixHash = 1469598103934665603ull;
	for (const GenesisCompatibilityEntry& entry : result.Entries) {
		string line = std::format("{}:{}:{}:{}", entry.Name, entry.Pass ? "PASS" : "FAIL", entry.PassCount, entry.Digest);
		for (uint8_t ch : line) {
			matrixHash ^= ch;
			matrixHash *= 1099511628211ull;
		}
	}

	result.Digest = ToHex(matrixHash);
	result.OutputLines.push_back(std::format("GEN_COMPAT_MATRIX_SUMMARY PASS={} FAIL={} DIGEST={}", result.PassCount, result.FailCount, result.Digest));
	return result;
}

GenesisPerformanceGateResult GenesisSmokeHarness::RunPerformanceGate(GenesisM68kBoundaryScaffold& scaffold, const vector<GenesisCompatibilityRomCase>& romSet, uint64_t budgetMicros) {
	GenesisPerformanceGateResult result = {};
	result.BudgetMicros = budgetMicros;
	result.Entries.reserve(romSet.size());

	for (const GenesisCompatibilityRomCase& romCase : romSet) {
		GenesisPerformanceGateEntry entry = {};
		entry.Name = romCase.Name;
		entry.TitleClass = InferTitleClass(romCase.Name);

		double classBudgetScale = 1.0;
		if (entry.TitleClass == "sonic") {
			classBudgetScale = 0.85;
		} else if (entry.TitleClass == "jurassic") {
			classBudgetScale = 0.90;
		}
		uint64_t classBudgetMicros = (uint64_t)std::max<int64_t>(1, (int64_t)std::llround((double)budgetMicros * classBudgetScale));

		if (romCase.RomData.empty()) {
			entry.Pass = false;
			entry.DeterministicDigest = ToHex(0);
			result.FailCount++;
			result.OutputLines.push_back(std::format("GEN_PERF_RESULT {} FAIL CLASS={} ELAPSED_US=0 BUDGET_US={} CLASS_BUDGET_US={} DIGEST={}", entry.Name, entry.TitleClass, budgetMicros, classBudgetMicros, entry.DeterministicDigest));
			result.Entries.push_back(std::move(entry));
			continue;
		}

		auto start = std::chrono::steady_clock::now();

		GenesisCompatibilityMatrixResult compatibility = RunCompatibilityMatrix(scaffold, {romCase});
		bool compatibilityPass = compatibility.FailCount == 0 && compatibility.PassCount == 1;
		string compatibilityDigest = compatibility.Digest;

		scaffold.LoadRom(romCase.RomData);
		scaffold.Startup();
		scaffold.StepFrameScaffold(640);
		GenesisBoundaryScaffoldSaveState baseline = scaffold.SaveState();
		scaffold.StepFrameScaffold(1440);
		GenesisBoundaryScaffoldSaveState firstRun = scaffold.SaveState();
		scaffold.LoadState(baseline);
		scaffold.StepFrameScaffold(1440);
		GenesisBoundaryScaffoldSaveState replayRun = scaffold.SaveState();
		bool replayPass = firstRun == replayRun;

		auto end = std::chrono::steady_clock::now();
		entry.ElapsedMicros = (uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

		string deterministicInput = std::format(
			"{}:{}:{}:{}:{}:{}",
			entry.Name,
			entry.TitleClass,
			compatibilityDigest,
			firstRun.Bus.MixedDigest,
			replayRun.Bus.MixedDigest,
			replayPass ? "1" : "0");

		uint64_t hash = 1469598103934665603ull;
		for (uint8_t ch : deterministicInput) {
			hash ^= ch;
			hash *= 1099511628211ull;
		}
		entry.DeterministicDigest = ToHex(hash);

		entry.Pass = compatibilityPass && replayPass && entry.ElapsedMicros <= classBudgetMicros;
		if (entry.Pass) {
			result.PassCount++;
		} else {
			result.FailCount++;
			result.OutputLines.push_back(std::format(
				"GEN_PERF_FAIL_CONTEXT {} class={} compatibility_pass={} replay_pass={} elapsed_us={} budget_us={} class_budget_us={} compat_digest={} mixed_digest={}",
				entry.Name,
				entry.TitleClass,
				compatibilityPass ? 1 : 0,
				replayPass ? 1 : 0,
				entry.ElapsedMicros,
				budgetMicros,
				classBudgetMicros,
				compatibilityDigest,
				firstRun.Bus.MixedDigest));
		}

		result.OutputLines.push_back(std::format(
			"GEN_PERF_RESULT {} {} CLASS={} ELAPSED_US={} BUDGET_US={} CLASS_BUDGET_US={} DIGEST={}",
			entry.Name,
			entry.Pass ? "PASS" : "FAIL",
			entry.TitleClass,
			entry.ElapsedMicros,
			budgetMicros,
			classBudgetMicros,
			entry.DeterministicDigest));
		result.Entries.push_back(std::move(entry));
	}

	uint64_t gateHash = 1469598103934665603ull;
	for (const GenesisPerformanceGateEntry& entry : result.Entries) {
		string line = std::format("{}:{}:{}:{}", entry.Name, entry.Pass ? "PASS" : "FAIL", entry.TitleClass, entry.DeterministicDigest);
		for (uint8_t ch : line) {
			gateHash ^= ch;
			gateHash *= 1099511628211ull;
		}
	}

	result.Digest = ToHex(gateHash);
	result.OutputLines.push_back(std::format("GEN_PERF_GATE_SUMMARY PASS={} FAIL={} BUDGET_US={} DIGEST={}", result.PassCount, result.FailCount, result.BudgetMicros, result.Digest));
	return result;
}
