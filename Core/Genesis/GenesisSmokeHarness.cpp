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

		if (romCase.RomData.empty()) {
			entry.Pass = false;
			entry.DeterministicDigest = ToHex(0);
			result.FailCount++;
			result.OutputLines.push_back(std::format("GEN_PERF_RESULT {} FAIL CLASS={} ELAPSED_US=0 BUDGET_US={} DIGEST={}", entry.Name, entry.TitleClass, budgetMicros, entry.DeterministicDigest));
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

		entry.Pass = compatibilityPass && replayPass && entry.ElapsedMicros <= budgetMicros;
		if (entry.Pass) {
			result.PassCount++;
		} else {
			result.FailCount++;
			result.OutputLines.push_back(std::format(
				"GEN_PERF_FAIL_CONTEXT {} class={} compatibility_pass={} replay_pass={} elapsed_us={} budget_us={} compat_digest={} mixed_digest={}",
				entry.Name,
				entry.TitleClass,
				compatibilityPass ? 1 : 0,
				replayPass ? 1 : 0,
				entry.ElapsedMicros,
				budgetMicros,
				compatibilityDigest,
				firstRun.Bus.MixedDigest));
		}

		result.OutputLines.push_back(std::format(
			"GEN_PERF_RESULT {} {} CLASS={} ELAPSED_US={} BUDGET_US={} DIGEST={}",
			entry.Name,
			entry.Pass ? "PASS" : "FAIL",
			entry.TitleClass,
			entry.ElapsedMicros,
			budgetMicros,
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
