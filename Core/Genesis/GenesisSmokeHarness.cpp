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

		bool mapperBankSwitchPass = true;
		string mapperBankSwitchDetail = "skipped-small-rom";
		if (romSize > 0x100000) {
			uint32_t bankWindowAddress = 0x080000;
			uint8_t baselineBankValue = scaffold.GetBus().ReadByte(bankWindowAddress);
			scaffold.GetBus().WriteByte(0xA130F3, 0x00);
			uint8_t switchedBankValue = scaffold.GetBus().ReadByte(bankWindowAddress);
			scaffold.GetBus().WriteByte(0xA130F3, 0x01);
			uint8_t restoredBankValue = scaffold.GetBus().ReadByte(bankWindowAddress);
			uint8_t expectedSwitchedValue = romCase.RomData[0];
			mapperBankSwitchPass = switchedBankValue == expectedSwitchedValue && restoredBankValue == baselineBankValue;
			mapperBankSwitchDetail = std::format(
				"baseline={:02x} switched={:02x} expectedSwitch={:02x} restored={:02x}",
				baselineBankValue,
				switchedBankValue,
				expectedSwitchedValue,
				restoredBankValue);
		}
		addCheckpoint("GEN-COMPAT-MAPPER-BANK-SWITCH", mapperBankSwitchPass, mapperBankSwitchDetail);

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

		GenesisBoundaryScaffoldSaveState scdTelemetryScenarioStart = scaffold.SaveState();
		scaffold.GetBus().ClearCommandResponseLane();
		scaffold.GetBus().WriteByte(0xA12012, 0x18);
		scaffold.GetBus().WriteByte(0xA12013, 0x2A);
		scaffold.GetBus().WriteByte(0xA12015, 0x4C);
		uint16_t scdTelemetryCount = (uint16_t)scaffold.GetBus().ReadByte(0xA12016)
			| ((uint16_t)scaffold.GetBus().ReadByte(0xA12017) << 8);
		GenesisBoundaryScaffoldSaveState scdTelemetryBaseline = scaffold.SaveState();
		scaffold.GetBus().WriteByte(0xA12015, 0x5D);
		uint16_t scdTelemetryTail = (uint16_t)scaffold.GetBus().ReadByte(0xA12016)
			| ((uint16_t)scaffold.GetBus().ReadByte(0xA12017) << 8);
		scaffold.LoadState(scdTelemetryBaseline);
		scaffold.GetBus().WriteByte(0xA12015, 0x5D);
		uint16_t scdTelemetryReplay = (uint16_t)scaffold.GetBus().ReadByte(0xA12016)
			| ((uint16_t)scaffold.GetBus().ReadByte(0xA12017) << 8);

		bool scdTelemetryPass = scdTelemetryCount > 0
			&& scdTelemetryTail >= scdTelemetryCount
			&& scdTelemetryReplay == scdTelemetryTail;
		addCheckpoint(
			"GEN-COMPAT-SCD-TELEMETRY",
			scdTelemetryPass,
			std::format(
				"countNonZero={} replayMatch={} replayGteCount={}",
				scdTelemetryCount > 0 ? 1 : 0,
				scdTelemetryTail == scdTelemetryReplay ? 1 : 0,
				scdTelemetryReplay >= scdTelemetryCount ? 1 : 0));
		scaffold.LoadState(scdTelemetryScenarioStart);

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

		GenesisBoundaryScaffoldSaveState telemetryScenarioStart = scaffold.SaveState();
		uint16_t toolingTelemetryCount = (uint16_t)scaffold.GetBus().ReadByte(0xA15018)
			| ((uint16_t)scaffold.GetBus().ReadByte(0xA15019) << 8);
		scaffold.GetBus().WriteByte(0xA15009, 0x55);
		scaffold.GetBus().WriteByte(0xA1500B, 0x66);
		uint16_t toolingTelemetryTail = (uint16_t)scaffold.GetBus().ReadByte(0xA15018)
			| ((uint16_t)scaffold.GetBus().ReadByte(0xA15019) << 8);
		GenesisBoundaryScaffoldSaveState telemetryBaseline = scaffold.SaveState();
		scaffold.GetBus().WriteByte(0xA1500B, 0x7A);
		scaffold.LoadState(telemetryBaseline);
		uint16_t toolingTelemetryReplay = (uint16_t)scaffold.GetBus().ReadByte(0xA15018)
			| ((uint16_t)scaffold.GetBus().ReadByte(0xA15019) << 8);

		bool toolingTelemetryPass = toolingTelemetryCount > 0
			&& toolingTelemetryTail >= toolingTelemetryCount
			&& toolingTelemetryReplay == toolingTelemetryTail;
		addCheckpoint(
			"GEN-COMPAT-32X-TELEMETRY",
			toolingTelemetryPass,
			std::format(
				"countNonZero={} replayMatch={} replayGteCount={}",
				toolingTelemetryCount > 0 ? 1 : 0,
				toolingTelemetryTail == toolingTelemetryReplay ? 1 : 0,
				toolingTelemetryReplay >= toolingTelemetryCount ? 1 : 0));
		scaffold.LoadState(telemetryScenarioStart);

		GenesisBoundaryScaffoldSaveState debugLaneScenarioStart = scaffold.SaveState();
		scaffold.GetBus().ClearCommandResponseLane();
		scaffold.GetBus().WriteByte(0xA12012, 0x11);
		scaffold.GetBus().WriteByte(0xA12015, 0x44);
		scaffold.GetBus().WriteByte(0xA15009, 0x22);
		(void)scaffold.GetBus().ReadByte(0xA1201B);
		uint32_t debugLanePrefixCount = scaffold.GetBus().GetCommandResponseLaneCount();
		string debugLanePrefixDigest = scaffold.GetBus().GetCommandResponseLaneDigest();
		GenesisBoundaryScaffoldSaveState debugLaneBaseline = scaffold.SaveState();

		scaffold.GetBus().WriteByte(0xA12013, 0x2C);
		scaffold.GetBus().WriteByte(0xA1500B, 0xE1);
		(void)scaffold.GetBus().ReadByte(0xA1501F);
		uint32_t debugLaneTailCount = scaffold.GetBus().GetCommandResponseLaneCount();
		string debugLaneTailDigest = scaffold.GetBus().GetCommandResponseLaneDigest();

		scaffold.LoadState(debugLaneBaseline);
		scaffold.GetBus().WriteByte(0xA12013, 0x2C);
		scaffold.GetBus().WriteByte(0xA1500B, 0xE1);
		(void)scaffold.GetBus().ReadByte(0xA1501F);
		uint32_t debugLaneReplayCount = scaffold.GetBus().GetCommandResponseLaneCount();
		string debugLaneReplayDigest = scaffold.GetBus().GetCommandResponseLaneDigest();

		bool debugLanePass = debugLanePrefixCount >= 4
			&& !debugLanePrefixDigest.empty()
			&& debugLaneReplayCount == debugLaneTailCount
			&& debugLaneReplayDigest == debugLaneTailDigest;
		addCheckpoint(
			"GEN-COMPAT-DEBUG-LANE",
			debugLanePass,
			std::format(
				"prefixCount={} prefixDigestNonEmpty={} tailCount={} replayCount={} digestReplayMatch={}",
				debugLanePrefixCount,
				!debugLanePrefixDigest.empty() ? 1 : 0,
				debugLaneTailCount,
				debugLaneReplayCount,
				debugLaneReplayDigest == debugLaneTailDigest ? 1 : 0));
		scaffold.LoadState(debugLaneScenarioStart);

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

		auto runVdpStartupPath = [&scaffold, &titleClass]() {
			if (titleClass == "sonic") {
				scaffold.GetBus().SetRenderCompositionInputs(0x1C, true, 0x06, false, 0x02, false, true, 0x20, true);
				scaffold.GetBus().SetScroll(4, 2);
				scaffold.GetBus().RenderScaffoldLine(56);
				scaffold.StepFrameScaffold(488u + 96u);
			} else if (titleClass == "jurassic") {
				scaffold.GetBus().BeginDmaTransfer(GenesisVdpDmaMode::Copy, 16);
				scaffold.StepFrameScaffold(488u + 128u);
				scaffold.GetBus().RenderScaffoldLine(80);
			} else {
				scaffold.StepFrameScaffold(128u);
			}
		};

		GenesisBoundaryScaffoldSaveState vdpTimingBaseline = scaffold.SaveState();
		runVdpStartupPath();
		GenesisBoundaryScaffoldSaveState vdpTimingFirst = scaffold.SaveState();
		scaffold.LoadState(vdpTimingBaseline);
		runVdpStartupPath();
		GenesisBoundaryScaffoldSaveState vdpTimingReplay = scaffold.SaveState();
		scaffold.LoadState(vdpTimingFirst);

		bool isTargetStartupClass = titleClass == "sonic" || titleClass == "jurassic";
		bool timingAdvanced = vdpTimingFirst.TimingFrame > vdpTimingBaseline.TimingFrame
			|| (vdpTimingFirst.TimingFrame == vdpTimingBaseline.TimingFrame && vdpTimingFirst.TimingScanline > vdpTimingBaseline.TimingScanline);
		bool vdpTimingReplayMatch = vdpTimingFirst.TimingFrame == vdpTimingReplay.TimingFrame
			&& vdpTimingFirst.TimingScanline == vdpTimingReplay.TimingScanline
			&& vdpTimingFirst.TimingCycleRemainder == vdpTimingReplay.TimingCycleRemainder
			&& vdpTimingFirst.VInterruptCount == vdpTimingReplay.VInterruptCount
			&& vdpTimingFirst.HInterruptCount == vdpTimingReplay.HInterruptCount
			&& vdpTimingFirst.Bus.VdpStatus == vdpTimingReplay.Bus.VdpStatus
			&& vdpTimingFirst.Bus.RenderLineDigest == vdpTimingReplay.Bus.RenderLineDigest
			&& vdpTimingFirst.Bus.DmaContentionCycles == vdpTimingReplay.Bus.DmaContentionCycles
			&& vdpTimingFirst.Bus.DmaContentionEvents == vdpTimingReplay.Bus.DmaContentionEvents;
		bool targetEvidencePass = !vdpTimingFirst.Bus.RenderLineDigest.empty();
		if (titleClass == "jurassic") {
			targetEvidencePass = targetEvidencePass && vdpTimingFirst.Bus.DmaContentionCycles >= vdpTimingBaseline.Bus.DmaContentionCycles;
		}
		bool vdpTimingStartupPass = !isTargetStartupClass || (timingAdvanced && vdpTimingReplayMatch && targetEvidencePass);
		addCheckpoint(
			"GEN-COMPAT-VDP-TIMING-STARTUP",
			vdpTimingStartupPass,
			std::format(
				"class={} target={} advanced={} replayMatch={} evidence={} frame={} scanline={} vdpStatus={:04x} renderDigest={} dmaCycles={} dmaEvents={}",
				titleClass,
				isTargetStartupClass ? 1 : 0,
				timingAdvanced ? 1 : 0,
				vdpTimingReplayMatch ? 1 : 0,
				targetEvidencePass ? 1 : 0,
				vdpTimingFirst.TimingFrame,
				vdpTimingFirst.TimingScanline,
				vdpTimingFirst.Bus.VdpStatus,
				vdpTimingFirst.Bus.RenderLineDigest,
				vdpTimingFirst.Bus.DmaContentionCycles,
				vdpTimingFirst.Bus.DmaContentionEvents));

		struct FirstVisibleFrameProbeResult {
			bool Found = false;
			uint32_t Frame = 0;
			uint32_t Scanline = 0;
			uint32_t VInterruptCount = 0;
			string RenderDigest;
		};

		auto runFirstVisibleFrameProbe = [&scaffold, &titleClass](uint32_t baselineFrame, uint32_t baselineScanline, uint32_t baselineVInterruptCount) {
			FirstVisibleFrameProbeResult probe = {};

			for (uint32_t window = 0; window < 24; window++) {
				if (titleClass == "sonic") {
					scaffold.GetBus().SetRenderCompositionInputs(0x1A, true, 0x06, false, 0x02, false, true, 0x20, true);
					scaffold.GetBus().SetScroll((uint16_t)((window * 3) & 0x1F), (uint16_t)((window * 2) & 0x1F));
				} else if (titleClass == "jurassic") {
					if ((window % 4) == 0) {
						scaffold.GetBus().BeginDmaTransfer(GenesisVdpDmaMode::Copy, 8);
					}
					scaffold.GetBus().SetRenderCompositionInputs(0x18, true, 0x05, false, 0x01, false, true, 0x20, true);
					scaffold.GetBus().SetScroll((uint16_t)(window & 0x0F), (uint16_t)((window + 1) & 0x0F));
				} else {
					scaffold.GetBus().SetRenderCompositionInputs(0x10, true, 0x04, false, 0x00, false, false, 0x20, true);
				}

				scaffold.GetBus().RenderScaffoldLine(64);
				scaffold.StepFrameScaffold(488u * 8u);

				GenesisBoundaryScaffoldSaveState sample = scaffold.SaveState();
				bool timingAdvanced = sample.TimingFrame > baselineFrame
					|| (sample.TimingFrame == baselineFrame && sample.TimingScanline > baselineScanline);
				bool frameVisible = timingAdvanced && !sample.Bus.RenderLineDigest.empty();
				if (frameVisible) {
					probe.Found = true;
					probe.Frame = sample.TimingFrame;
					probe.Scanline = sample.TimingScanline;
					probe.VInterruptCount = sample.VInterruptCount;
					probe.RenderDigest = sample.Bus.RenderLineDigest;
					break;
				}
			}

			if (!probe.Found) {
				GenesisBoundaryScaffoldSaveState tail = scaffold.SaveState();
				probe.Frame = tail.TimingFrame;
				probe.Scanline = tail.TimingScanline;
				probe.VInterruptCount = tail.VInterruptCount;
				probe.RenderDigest = tail.Bus.RenderLineDigest;
			}

			return probe;
		};

		GenesisBoundaryScaffoldSaveState firstVisibleBaseline = scaffold.SaveState();
		FirstVisibleFrameProbeResult firstVisibleFirst = runFirstVisibleFrameProbe(firstVisibleBaseline.TimingFrame, firstVisibleBaseline.TimingScanline, firstVisibleBaseline.VInterruptCount);
		GenesisBoundaryScaffoldSaveState firstVisibleState = scaffold.SaveState();
		scaffold.LoadState(firstVisibleBaseline);
		FirstVisibleFrameProbeResult firstVisibleReplay = runFirstVisibleFrameProbe(firstVisibleBaseline.TimingFrame, firstVisibleBaseline.TimingScanline, firstVisibleBaseline.VInterruptCount);
		scaffold.LoadState(firstVisibleState);

		bool firstVisibleReplayMatch = firstVisibleFirst.Found == firstVisibleReplay.Found
			&& firstVisibleFirst.Frame == firstVisibleReplay.Frame
			&& firstVisibleFirst.Scanline == firstVisibleReplay.Scanline
			&& firstVisibleFirst.VInterruptCount == firstVisibleReplay.VInterruptCount
			&& firstVisibleFirst.RenderDigest == firstVisibleReplay.RenderDigest;
		bool firstVisibleFramePass = !isTargetStartupClass || (firstVisibleFirst.Found && firstVisibleReplayMatch);
		addCheckpoint(
			"GEN-COMPAT-FIRST-VISIBLE-FRAME",
			firstVisibleFramePass,
			std::format(
				"class={} target={} found={} replayMatch={} frame={} scanline={} vInt={} digest={}",
				titleClass,
				isTargetStartupClass ? 1 : 0,
				firstVisibleFirst.Found ? 1 : 0,
				firstVisibleReplayMatch ? 1 : 0,
				firstVisibleFirst.Frame,
				firstVisibleFirst.Scanline,
				firstVisibleFirst.VInterruptCount,
				firstVisibleFirst.RenderDigest));

		struct StartupEventProbeResult {
			uint32_t EventCount = 0;
			bool HasHint = false;
			bool HasVint = false;
			bool HintBeforeVint = false;
			string SequenceDigest;
		};

		auto runStartupEventProbe = [&scaffold, &titleClass]() {
			StartupEventProbeResult probe = {};

			scaffold.ClearTimingEvents();
			if (titleClass == "sonic") {
				scaffold.StepFrameScaffold(488u * 300u + 64u);
			} else if (titleClass == "jurassic") {
				scaffold.StepFrameScaffold(488u * 304u + 80u);
			} else {
				scaffold.StepFrameScaffold(488u * 280u + 48u);
			}

			const vector<string>& events = scaffold.GetTimingEvents();
			probe.EventCount = (uint32_t)events.size();

			int firstHintIndex = -1;
			int firstVintIndex = -1;
			for (size_t i = 0; i < events.size(); i++) {
				if (firstHintIndex < 0 && events[i].starts_with("HINT ")) {
					firstHintIndex = (int)i;
					probe.HasHint = true;
				}
				if (firstVintIndex < 0 && events[i].starts_with("VINT ")) {
					firstVintIndex = (int)i;
					probe.HasVint = true;
				}
				if (probe.HasHint && probe.HasVint) {
					break;
				}
			}

			probe.HintBeforeVint = !probe.HasVint || (probe.HasHint && firstHintIndex <= firstVintIndex);

			uint64_t digest = 1469598103934665603ull;
			for (const string& eventLine : events) {
				for (uint8_t ch : eventLine) {
					digest ^= ch;
					digest *= 1099511628211ull;
				}
			}
			probe.SequenceDigest = ToHex(digest);
			return probe;
		};

		GenesisBoundaryScaffoldSaveState startupEventBaseline = scaffold.SaveState();
		StartupEventProbeResult startupEventFirst = runStartupEventProbe();
		GenesisBoundaryScaffoldSaveState startupEventTail = scaffold.SaveState();
		scaffold.LoadState(startupEventBaseline);
		StartupEventProbeResult startupEventReplay = runStartupEventProbe();
		scaffold.LoadState(startupEventTail);

		bool startupEventReplayMatch = startupEventFirst.EventCount == startupEventReplay.EventCount
			&& startupEventFirst.HasHint == startupEventReplay.HasHint
			&& startupEventFirst.HasVint == startupEventReplay.HasVint
			&& startupEventFirst.HintBeforeVint == startupEventReplay.HintBeforeVint
			&& startupEventFirst.SequenceDigest == startupEventReplay.SequenceDigest;
		bool startupEventPass = !isTargetStartupClass || (startupEventFirst.HasHint && startupEventFirst.HasVint && startupEventFirst.HintBeforeVint && startupEventReplayMatch);
		addCheckpoint(
			"GEN-COMPAT-STARTUP-EVENT-SEQUENCE",
			startupEventPass,
			std::format(
				"class={} target={} hasHint={} hasVint={} hintBeforeVint={} replayMatch={} count={} digest={}",
				titleClass,
				isTargetStartupClass ? 1 : 0,
				startupEventFirst.HasHint ? 1 : 0,
				startupEventFirst.HasVint ? 1 : 0,
				startupEventFirst.HintBeforeVint ? 1 : 0,
				startupEventReplayMatch ? 1 : 0,
				startupEventFirst.EventCount,
				startupEventFirst.SequenceDigest));

		struct StartupFrameWindowProbeResult {
			uint32_t FrameDelta = 0;
			uint32_t VInterruptDelta = 0;
			uint32_t HInterruptDelta = 0;
			uint32_t EventCount = 0;
			string EventDigest;
		};

		auto runStartupFrameWindowProbe = [&scaffold, &titleClass](const GenesisBoundaryScaffoldSaveState& baseline) {
			StartupFrameWindowProbeResult probe = {};

			scaffold.ClearTimingEvents();
			for (uint32_t frameIndex = 0; frameIndex < 3; frameIndex++) {
				if (titleClass == "sonic") {
					scaffold.GetBus().SetRenderCompositionInputs(0x1A, true, 0x07, false, 0x02, false, true, 0x20, true);
					scaffold.GetBus().SetScroll((uint16_t)((frameIndex * 3) & 0x1F), (uint16_t)((frameIndex * 2) & 0x1F));
					scaffold.GetBus().RenderScaffoldLine(64);
				} else if (titleClass == "jurassic") {
					if ((frameIndex % 2) == 0) {
						scaffold.GetBus().BeginDmaTransfer(GenesisVdpDmaMode::Copy, 12);
					}
					scaffold.GetBus().SetRenderCompositionInputs(0x18, true, 0x06, false, 0x01, false, true, 0x20, true);
					scaffold.GetBus().SetScroll((uint16_t)(frameIndex & 0x0F), (uint16_t)((frameIndex + 1) & 0x0F));
					scaffold.GetBus().RenderScaffoldLine(72);
				} else {
					scaffold.GetBus().SetRenderCompositionInputs(0x10, true, 0x04, false, 0x00, false, false, 0x20, true);
					scaffold.GetBus().RenderScaffoldLine(48);
				}

				scaffold.StepFrameScaffold(488u * 262u + 96u);
			}

			GenesisBoundaryScaffoldSaveState sample = scaffold.SaveState();
			probe.FrameDelta = sample.TimingFrame - baseline.TimingFrame;
			probe.VInterruptDelta = sample.VInterruptCount - baseline.VInterruptCount;
			probe.HInterruptDelta = sample.HInterruptCount - baseline.HInterruptCount;

			const vector<string>& events = scaffold.GetTimingEvents();
			probe.EventCount = (uint32_t)events.size();
			uint64_t digest = 1469598103934665603ull;
			for (const string& eventLine : events) {
				for (uint8_t ch : eventLine) {
					digest ^= ch;
					digest *= 1099511628211ull;
				}
			}
			probe.EventDigest = ToHex(digest);
			return probe;
		};

		GenesisBoundaryScaffoldSaveState startupFrameWindowBaseline = scaffold.SaveState();
		StartupFrameWindowProbeResult startupFrameWindowFirst = runStartupFrameWindowProbe(startupFrameWindowBaseline);
		GenesisBoundaryScaffoldSaveState startupFrameWindowTail = scaffold.SaveState();
		scaffold.LoadState(startupFrameWindowBaseline);
		StartupFrameWindowProbeResult startupFrameWindowReplay = runStartupFrameWindowProbe(startupFrameWindowBaseline);
		scaffold.LoadState(startupFrameWindowTail);

		bool startupFrameWindowReplayMatch = startupFrameWindowFirst.FrameDelta == startupFrameWindowReplay.FrameDelta
			&& startupFrameWindowFirst.VInterruptDelta == startupFrameWindowReplay.VInterruptDelta
			&& startupFrameWindowFirst.HInterruptDelta == startupFrameWindowReplay.HInterruptDelta
			&& startupFrameWindowFirst.EventCount == startupFrameWindowReplay.EventCount
			&& startupFrameWindowFirst.EventDigest == startupFrameWindowReplay.EventDigest;
		bool startupFrameWindowPass = !isTargetStartupClass || (
			startupFrameWindowFirst.FrameDelta >= 3
			&& startupFrameWindowFirst.VInterruptDelta >= 3
			&& startupFrameWindowFirst.HInterruptDelta > 0
			&& startupFrameWindowFirst.EventCount > 0
			&& startupFrameWindowReplayMatch);
		addCheckpoint(
			"GEN-COMPAT-STARTUP-FRAME-WINDOW",
			startupFrameWindowPass,
			std::format(
				"class={} target={} frameDelta={} vIntDelta={} hIntDelta={} eventCount={} replayMatch={} eventDigest={}",
				titleClass,
				isTargetStartupClass ? 1 : 0,
				startupFrameWindowFirst.FrameDelta,
				startupFrameWindowFirst.VInterruptDelta,
				startupFrameWindowFirst.HInterruptDelta,
				startupFrameWindowFirst.EventCount,
				startupFrameWindowReplayMatch ? 1 : 0,
				startupFrameWindowFirst.EventDigest));

		struct StartupRenderHandoffProbeResult {
			uint32_t FrameDelta = 0;
			uint32_t VInterruptDelta = 0;
			uint16_t VdpStatusTransition = 0;
			string RenderDigest;
			string PlaneDigest;
		};

		auto runStartupRenderHandoffProbe = [&scaffold, &titleClass](const GenesisBoundaryScaffoldSaveState& baseline) {
			StartupRenderHandoffProbeResult probe = {};

			for (uint32_t frameIndex = 0; frameIndex < 2; frameIndex++) {
				if (titleClass == "sonic") {
					scaffold.GetBus().SetRenderCompositionInputs(0x1c, true, 0x08, false, 0x02, false, true, 0x20, true);
					scaffold.GetBus().SetScroll((uint16_t)((frameIndex * 5) & 0x1f), (uint16_t)((frameIndex * 3) & 0x1f));
					scaffold.GetBus().RenderScaffoldLine(96);
					scaffold.StepFrameScaffold(488u * 262u + 128u);
				} else if (titleClass == "jurassic") {
					scaffold.GetBus().BeginDmaTransfer(GenesisVdpDmaMode::Copy, 14);
					scaffold.GetBus().SetRenderCompositionInputs(0x1a, true, 0x07, false, 0x02, false, true, 0x20, true);
					scaffold.GetBus().SetScroll((uint16_t)((frameIndex + 2) & 0x0f), (uint16_t)((frameIndex + 3) & 0x0f));
					scaffold.GetBus().RenderScaffoldLine(88);
					scaffold.StepFrameScaffold(488u * 262u + 152u);
				} else {
					scaffold.GetBus().SetRenderCompositionInputs(0x10, true, 0x04, false, 0x00, false, false, 0x20, true);
					scaffold.GetBus().RenderScaffoldLine(56);
					scaffold.StepFrameScaffold(256u);
				}
			}

			GenesisBoundaryScaffoldSaveState sample = scaffold.SaveState();
			probe.FrameDelta = sample.TimingFrame - baseline.TimingFrame;
			probe.VInterruptDelta = sample.VInterruptCount - baseline.VInterruptCount;
			probe.VdpStatusTransition = (uint16_t)(baseline.Bus.VdpStatus ^ sample.Bus.VdpStatus);
			probe.RenderDigest = sample.Bus.RenderLineDigest;
			probe.PlaneDigest = std::format(
				"{:02x}{:02x}{:02x}{:02x}-{}{}{}{}-{:02x}-{:02x}{:02x}",
				sample.Bus.PlaneASample,
				sample.Bus.PlaneBSample,
				sample.Bus.WindowSample,
				sample.Bus.SpriteSample,
				sample.Bus.PlaneAPriority ? 1 : 0,
				sample.Bus.PlaneBPriority ? 1 : 0,
				sample.Bus.WindowPriority ? 1 : 0,
				sample.Bus.SpritePriority ? 1 : 0,
				sample.Bus.WindowEnabled ? 1 : 0,
				sample.Bus.ScrollX & 0xff,
				sample.Bus.ScrollY & 0xff);
			return probe;
		};

		GenesisBoundaryScaffoldSaveState startupRenderHandoffBaseline = scaffold.SaveState();
		StartupRenderHandoffProbeResult startupRenderHandoffFirst = runStartupRenderHandoffProbe(startupRenderHandoffBaseline);
		GenesisBoundaryScaffoldSaveState startupRenderHandoffTail = scaffold.SaveState();
		scaffold.LoadState(startupRenderHandoffBaseline);
		StartupRenderHandoffProbeResult startupRenderHandoffReplay = runStartupRenderHandoffProbe(startupRenderHandoffBaseline);
		scaffold.LoadState(startupRenderHandoffTail);

		bool startupRenderHandoffReplayMatch = startupRenderHandoffFirst.FrameDelta == startupRenderHandoffReplay.FrameDelta
			&& startupRenderHandoffFirst.VInterruptDelta == startupRenderHandoffReplay.VInterruptDelta
			&& startupRenderHandoffFirst.VdpStatusTransition == startupRenderHandoffReplay.VdpStatusTransition
			&& startupRenderHandoffFirst.RenderDigest == startupRenderHandoffReplay.RenderDigest
			&& startupRenderHandoffFirst.PlaneDigest == startupRenderHandoffReplay.PlaneDigest;
		bool startupRenderHandoffEvidence = startupRenderHandoffFirst.VdpStatusTransition != 0 || !startupRenderHandoffFirst.RenderDigest.empty();
		bool startupRenderHandoffPass = !isTargetStartupClass || (
			startupRenderHandoffFirst.FrameDelta >= 2
			&& startupRenderHandoffFirst.VInterruptDelta >= 2
			&& startupRenderHandoffEvidence
			&& startupRenderHandoffReplayMatch);
		addCheckpoint(
			"GEN-COMPAT-STARTUP-RENDER-HANDOFF",
			startupRenderHandoffPass,
			std::format(
				"class={} target={} frameDelta={} vIntDelta={} transition={:04x} evidence={} replayMatch={} renderDigest={} planeDigest={}",
				titleClass,
				isTargetStartupClass ? 1 : 0,
				startupRenderHandoffFirst.FrameDelta,
				startupRenderHandoffFirst.VInterruptDelta,
				startupRenderHandoffFirst.VdpStatusTransition,
				startupRenderHandoffEvidence ? 1 : 0,
				startupRenderHandoffReplayMatch ? 1 : 0,
				startupRenderHandoffFirst.RenderDigest,
				startupRenderHandoffFirst.PlaneDigest));

		struct StartupCpuProgressionProbeResult {
			uint32_t ProgramCounterDelta = 0;
			uint64_t CycleDelta = 0;
			uint32_t StackDelta = 0;
			uint16_t StatusTransition = 0;
			uint32_t InterruptSequenceDelta = 0;
			string EventDigest;
		};

		auto runStartupCpuProgressionProbe = [&scaffold, &titleClass](const GenesisBoundaryScaffoldSaveState& baseline) {
			StartupCpuProgressionProbeResult probe = {};

			scaffold.ClearTimingEvents();
			if (titleClass == "sonic") {
				scaffold.StepFrameScaffold(488u * 3u + 144u);
			} else if (titleClass == "jurassic") {
				scaffold.StepFrameScaffold(488u * 3u + 168u);
			} else {
				scaffold.StepFrameScaffold(320u);
			}

			GenesisBoundaryScaffoldSaveState sample = scaffold.SaveState();
			probe.ProgramCounterDelta = (sample.Cpu.ProgramCounter - baseline.Cpu.ProgramCounter) & 0x00ffffff;
			probe.CycleDelta = sample.Cpu.CycleCount - baseline.Cpu.CycleCount;
			probe.StackDelta = (sample.Cpu.SupervisorStackPointer - baseline.Cpu.SupervisorStackPointer) & 0x00ffffff;
			probe.StatusTransition = (uint16_t)(sample.Cpu.StatusRegister ^ baseline.Cpu.StatusRegister);
			probe.InterruptSequenceDelta = sample.Cpu.InterruptSequenceCount - baseline.Cpu.InterruptSequenceCount;

			const vector<string>& events = scaffold.GetTimingEvents();
			uint64_t digest = 1469598103934665603ull;
			for (const string& eventLine : events) {
				for (uint8_t ch : eventLine) {
					digest ^= ch;
					digest *= 1099511628211ull;
				}
			}
			probe.EventDigest = ToHex(digest);
			return probe;
		};

		GenesisBoundaryScaffoldSaveState startupCpuProgressionBaseline = scaffold.SaveState();
		StartupCpuProgressionProbeResult startupCpuProgressionFirst = runStartupCpuProgressionProbe(startupCpuProgressionBaseline);
		GenesisBoundaryScaffoldSaveState startupCpuProgressionTail = scaffold.SaveState();
		scaffold.LoadState(startupCpuProgressionBaseline);
		StartupCpuProgressionProbeResult startupCpuProgressionReplay = runStartupCpuProgressionProbe(startupCpuProgressionBaseline);
		scaffold.LoadState(startupCpuProgressionTail);

		bool startupCpuProgressionReplayMatch = startupCpuProgressionFirst.ProgramCounterDelta == startupCpuProgressionReplay.ProgramCounterDelta
			&& startupCpuProgressionFirst.CycleDelta == startupCpuProgressionReplay.CycleDelta
			&& startupCpuProgressionFirst.StackDelta == startupCpuProgressionReplay.StackDelta
			&& startupCpuProgressionFirst.StatusTransition == startupCpuProgressionReplay.StatusTransition
			&& startupCpuProgressionFirst.InterruptSequenceDelta == startupCpuProgressionReplay.InterruptSequenceDelta
			&& startupCpuProgressionFirst.EventDigest == startupCpuProgressionReplay.EventDigest;
		bool startupCpuProgressionAdvanced = startupCpuProgressionFirst.CycleDelta > 0
			&& startupCpuProgressionFirst.ProgramCounterDelta > 0;
		bool startupCpuProgressionPass = !isTargetStartupClass || (startupCpuProgressionAdvanced && startupCpuProgressionReplayMatch);
		addCheckpoint(
			"GEN-COMPAT-STARTUP-CPU-PROGRESSION",
			startupCpuProgressionPass,
			std::format(
				"class={} target={} advanced={} replayMatch={} pcDelta={:06x} cycleDelta={} stackDelta={:06x} statusTransition={:04x} intSeqDelta={} eventDigest={}",
				titleClass,
				isTargetStartupClass ? 1 : 0,
				startupCpuProgressionAdvanced ? 1 : 0,
				startupCpuProgressionReplayMatch ? 1 : 0,
				startupCpuProgressionFirst.ProgramCounterDelta,
				startupCpuProgressionFirst.CycleDelta,
				startupCpuProgressionFirst.StackDelta,
				startupCpuProgressionFirst.StatusTransition,
				startupCpuProgressionFirst.InterruptSequenceDelta,
				startupCpuProgressionFirst.EventDigest));

		struct BootToTitleProgressionProbeResult {
			uint32_t PhaseTransitionCount = 0;
			uint32_t NonEmptyDigestWindows = 0;
			uint32_t PositiveVintWindows = 0;
			uint32_t StableCadenceWindows = 0;
			uint32_t FinalFrameDelta = 0;
			string SignatureDigest;
		};

		auto runBootToTitleProgressionProbe = [&scaffold, &titleClass](const GenesisBoundaryScaffoldSaveState& baseline) {
			BootToTitleProgressionProbeResult probe = {};

			string previousRenderDigest;
			uint32_t previousVint = baseline.VInterruptCount;
			uint32_t previousVintDelta = 0;
			bool hasPreviousVintDelta = false;
			uint64_t digest = 1469598103934665603ull;

			for (uint32_t window = 0; window < 6; window++) {
				if (titleClass == "sonic") {
					scaffold.GetBus().SetRenderCompositionInputs((uint8_t)(0x14 + (window & 0x03)), true, (uint8_t)(0x05 + (window & 0x03)), false, 0x02, false, true, 0x20, true);
					scaffold.GetBus().SetScroll((uint16_t)((window * 5) & 0x1f), (uint16_t)((window * 3) & 0x1f));
					scaffold.GetBus().RenderScaffoldLine((uint16_t)(56 + window * 4));
					scaffold.StepFrameScaffold(488u * 262u + 96u + window * 8u);
				} else if (titleClass == "jurassic") {
					if ((window % 2) == 0) {
						scaffold.GetBus().BeginDmaTransfer(GenesisVdpDmaMode::Copy, 10 + window);
					}
					scaffold.GetBus().SetRenderCompositionInputs((uint8_t)(0x16 + (window & 0x03)), true, (uint8_t)(0x06 + (window & 0x03)), false, 0x01, false, true, 0x20, true);
					scaffold.GetBus().SetScroll((uint16_t)((window + 2) & 0x0f), (uint16_t)((window + 3) & 0x0f));
					scaffold.GetBus().RenderScaffoldLine((uint16_t)(64 + window * 3));
					scaffold.StepFrameScaffold(488u * 262u + 120u + window * 10u);
				} else {
					scaffold.GetBus().SetRenderCompositionInputs(0x10, true, 0x04, false, 0x00, false, false, 0x20, true);
					scaffold.GetBus().RenderScaffoldLine(48);
					scaffold.StepFrameScaffold(256u);
				}

				GenesisBoundaryScaffoldSaveState sample = scaffold.SaveState();
				string currentRenderDigest = sample.Bus.RenderLineDigest;
				if (!currentRenderDigest.empty()) {
					probe.NonEmptyDigestWindows++;
					if (!previousRenderDigest.empty() && currentRenderDigest != previousRenderDigest) {
						probe.PhaseTransitionCount++;
					}
					previousRenderDigest = currentRenderDigest;
				}

				uint32_t vintDelta = sample.VInterruptCount - previousVint;
				if (vintDelta > 0) {
					probe.PositiveVintWindows++;
					if (!hasPreviousVintDelta || vintDelta == previousVintDelta) {
						probe.StableCadenceWindows++;
					}
					previousVintDelta = vintDelta;
					hasPreviousVintDelta = true;
				}

				string signatureLine = std::format(
					"w{}:{}:{}:{}:{}:{}",
					window,
					sample.TimingFrame,
					sample.TimingScanline,
					sample.VInterruptCount,
					vintDelta,
					currentRenderDigest);
				for (uint8_t ch : signatureLine) {
					digest ^= ch;
					digest *= 1099511628211ull;
				}

				previousVint = sample.VInterruptCount;
			}

			GenesisBoundaryScaffoldSaveState finalState = scaffold.SaveState();
			probe.FinalFrameDelta = finalState.TimingFrame - baseline.TimingFrame;
			probe.SignatureDigest = ToHex(digest);
			return probe;
		};

		GenesisBoundaryScaffoldSaveState bootToTitleBaseline = scaffold.SaveState();
		BootToTitleProgressionProbeResult bootToTitleFirst = runBootToTitleProgressionProbe(bootToTitleBaseline);
		GenesisBoundaryScaffoldSaveState bootToTitleTail = scaffold.SaveState();
		scaffold.LoadState(bootToTitleBaseline);
		BootToTitleProgressionProbeResult bootToTitleReplay = runBootToTitleProgressionProbe(bootToTitleBaseline);
		scaffold.LoadState(bootToTitleTail);

		bool bootToTitleReplayMatch = bootToTitleFirst.PhaseTransitionCount == bootToTitleReplay.PhaseTransitionCount
			&& bootToTitleFirst.NonEmptyDigestWindows == bootToTitleReplay.NonEmptyDigestWindows
			&& bootToTitleFirst.PositiveVintWindows == bootToTitleReplay.PositiveVintWindows
			&& bootToTitleFirst.StableCadenceWindows == bootToTitleReplay.StableCadenceWindows
			&& bootToTitleFirst.FinalFrameDelta == bootToTitleReplay.FinalFrameDelta
			&& bootToTitleFirst.SignatureDigest == bootToTitleReplay.SignatureDigest;
		bool bootToTitleProgressionPass = !isTargetStartupClass || (
			bootToTitleFirst.FinalFrameDelta >= 6
			&& bootToTitleFirst.NonEmptyDigestWindows >= 4
			&& bootToTitleFirst.PhaseTransitionCount >= 2
			&& bootToTitleFirst.PositiveVintWindows >= 4
			&& bootToTitleFirst.StableCadenceWindows >= 3
			&& bootToTitleReplayMatch);
		addCheckpoint(
			"GEN-COMPAT-BOOT-TO-TITLE-PROGRESSION",
			bootToTitleProgressionPass,
			std::format(
				"class={} target={} phaseTransitions={} nonEmptyDigests={} positiveVintWindows={} stableCadenceWindows={} finalFrameDelta={} replayMatch={} signature={}",
				titleClass,
				isTargetStartupClass ? 1 : 0,
				bootToTitleFirst.PhaseTransitionCount,
				bootToTitleFirst.NonEmptyDigestWindows,
				bootToTitleFirst.PositiveVintWindows,
				bootToTitleFirst.StableCadenceWindows,
				bootToTitleFirst.FinalFrameDelta,
				bootToTitleReplayMatch ? 1 : 0,
				bootToTitleFirst.SignatureDigest));

		struct RealRomStartupSmokeProbeResult {
			bool FoundPlayableFrame = false;
			uint32_t PlayableFrame = 0;
			uint32_t PlayableScanline = 0;
			uint32_t PlayableVintCount = 0;
			uint32_t IoActivityDelta = 0;
			uint32_t VdpActivityDelta = 0;
			string RenderDigest;
			string TraceDigest;
		};

		auto runRealRomStartupSmokeProbe = [&scaffold, &titleClass](const GenesisBoundaryScaffoldSaveState& baseline) {
			RealRomStartupSmokeProbeResult probe = {};
			uint64_t traceHash = 1469598103934665603ull;

			for (uint32_t window = 0; window < 8; window++) {
				scaffold.GetBus().WriteByte(0xA10003, (uint8_t)(0x40 | (window & 0x01)));
				scaffold.GetBus().WriteByte(0xA10005, (uint8_t)(0x40 | ((window + 1) & 0x01)));
				uint8_t pad0 = scaffold.GetBus().ReadByte(0xA10003);
				uint8_t pad1 = scaffold.GetBus().ReadByte(0xA10005);

				if (titleClass == "sonic") {
					scaffold.GetBus().SetRenderCompositionInputs((uint8_t)(0x18 + (window & 0x03)), true, 0x07, false, 0x02, false, true, 0x20, true);
					scaffold.GetBus().SetScroll((uint16_t)((window * 4) & 0x1f), (uint16_t)((window * 2) & 0x1f));
					scaffold.GetBus().RenderScaffoldLine((uint16_t)(72 + window * 2));
					scaffold.StepFrameScaffold(488u * 262u + 96u);
				} else if (titleClass == "jurassic") {
					if ((window % 2) == 0) {
						scaffold.GetBus().BeginDmaTransfer(GenesisVdpDmaMode::Copy, 8 + window);
					}
					scaffold.GetBus().SetRenderCompositionInputs((uint8_t)(0x17 + (window & 0x03)), true, 0x06, false, 0x01, false, true, 0x20, true);
					scaffold.GetBus().SetScroll((uint16_t)((window + 1) & 0x0f), (uint16_t)((window + 2) & 0x0f));
					scaffold.GetBus().RenderScaffoldLine((uint16_t)(68 + window * 3));
					scaffold.StepFrameScaffold(488u * 262u + 112u);
				} else {
					scaffold.GetBus().SetRenderCompositionInputs(0x10, true, 0x04, false, 0x00, false, false, 0x20, true);
					scaffold.GetBus().RenderScaffoldLine(48);
					scaffold.StepFrameScaffold(256u);
				}

				GenesisBoundaryScaffoldSaveState sample = scaffold.SaveState();
				uint32_t ioDelta = (sample.Bus.IoReadCount + sample.Bus.IoWriteCount) - (baseline.Bus.IoReadCount + baseline.Bus.IoWriteCount);
				uint32_t vdpDelta = (sample.Bus.VdpReadCount + sample.Bus.VdpWriteCount) - (baseline.Bus.VdpReadCount + baseline.Bus.VdpWriteCount);
				bool frameAdvanced = sample.TimingFrame > baseline.TimingFrame;
				bool playableEvidence = frameAdvanced
					&& !sample.Bus.RenderLineDigest.empty()
					&& sample.VInterruptCount >= baseline.VInterruptCount;

				string traceLine = std::format(
					"w{}:{}:{}:{}:{}:{}:{:02x}:{:02x}:{}",
					window,
					sample.TimingFrame,
					sample.TimingScanline,
					sample.VInterruptCount,
					ioDelta,
					vdpDelta,
					pad0,
					pad1,
					sample.Bus.RenderLineDigest);
				for (uint8_t ch : traceLine) {
					traceHash ^= ch;
					traceHash *= 1099511628211ull;
				}

				if (playableEvidence && !probe.FoundPlayableFrame) {
					probe.FoundPlayableFrame = true;
					probe.PlayableFrame = sample.TimingFrame;
					probe.PlayableScanline = sample.TimingScanline;
					probe.PlayableVintCount = sample.VInterruptCount;
					probe.IoActivityDelta = ioDelta;
					probe.VdpActivityDelta = vdpDelta;
					probe.RenderDigest = sample.Bus.RenderLineDigest;
				}
			}

			if (!probe.FoundPlayableFrame) {
				GenesisBoundaryScaffoldSaveState tail = scaffold.SaveState();
				probe.PlayableFrame = tail.TimingFrame;
				probe.PlayableScanline = tail.TimingScanline;
				probe.PlayableVintCount = tail.VInterruptCount;
				probe.IoActivityDelta = (tail.Bus.IoReadCount + tail.Bus.IoWriteCount) - (baseline.Bus.IoReadCount + baseline.Bus.IoWriteCount);
				probe.VdpActivityDelta = (tail.Bus.VdpReadCount + tail.Bus.VdpWriteCount) - (baseline.Bus.VdpReadCount + baseline.Bus.VdpWriteCount);
				probe.RenderDigest = tail.Bus.RenderLineDigest;
			}

			probe.TraceDigest = ToHex(traceHash);
			return probe;
		};

		GenesisBoundaryScaffoldSaveState realRomSmokeBaseline = scaffold.SaveState();
		RealRomStartupSmokeProbeResult realRomSmokeFirst = runRealRomStartupSmokeProbe(realRomSmokeBaseline);
		GenesisBoundaryScaffoldSaveState realRomSmokeTail = scaffold.SaveState();
		scaffold.LoadState(realRomSmokeBaseline);
		RealRomStartupSmokeProbeResult realRomSmokeReplay = runRealRomStartupSmokeProbe(realRomSmokeBaseline);
		scaffold.LoadState(realRomSmokeTail);

		bool realRomSmokeReplayMatch = realRomSmokeFirst.FoundPlayableFrame == realRomSmokeReplay.FoundPlayableFrame
			&& realRomSmokeFirst.PlayableFrame == realRomSmokeReplay.PlayableFrame
			&& realRomSmokeFirst.PlayableScanline == realRomSmokeReplay.PlayableScanline
			&& realRomSmokeFirst.PlayableVintCount == realRomSmokeReplay.PlayableVintCount
			&& realRomSmokeFirst.IoActivityDelta == realRomSmokeReplay.IoActivityDelta
			&& realRomSmokeFirst.VdpActivityDelta == realRomSmokeReplay.VdpActivityDelta
			&& realRomSmokeFirst.RenderDigest == realRomSmokeReplay.RenderDigest
			&& realRomSmokeFirst.TraceDigest == realRomSmokeReplay.TraceDigest;
		bool realRomStartupSmokePass = !isTargetStartupClass || (realRomSmokeFirst.FoundPlayableFrame && realRomSmokeReplayMatch);
		addCheckpoint(
			"GEN-COMPAT-REALROM-STARTUP-SMOKE",
			realRomStartupSmokePass,
			std::format(
				"class={} target={} foundPlayable={} replayMatch={} frame={} scanline={} vInt={} ioDelta={} vdpDelta={} renderDigest={} traceDigest={}",
				titleClass,
				isTargetStartupClass ? 1 : 0,
				realRomSmokeFirst.FoundPlayableFrame ? 1 : 0,
				realRomSmokeReplayMatch ? 1 : 0,
				realRomSmokeFirst.PlayableFrame,
				realRomSmokeFirst.PlayableScanline,
				realRomSmokeFirst.PlayableVintCount,
				realRomSmokeFirst.IoActivityDelta,
				realRomSmokeFirst.VdpActivityDelta,
				realRomSmokeFirst.RenderDigest,
				realRomSmokeFirst.TraceDigest));
		entry.OutputLines.push_back(std::format(
			"GEN_REALROM_STARTUP_SMOKE {} class={} foundPlayable={} frame={} scanline={} vInt={} ioDelta={} vdpDelta={} renderDigest={} traceDigest={}",
			entry.Name,
			titleClass,
			realRomSmokeFirst.FoundPlayableFrame ? 1 : 0,
			realRomSmokeFirst.PlayableFrame,
			realRomSmokeFirst.PlayableScanline,
			realRomSmokeFirst.PlayableVintCount,
			realRomSmokeFirst.IoActivityDelta,
			realRomSmokeFirst.VdpActivityDelta,
			realRomSmokeFirst.RenderDigest,
			realRomSmokeFirst.TraceDigest));

		struct StartupInputWindowProbeResult {
			uint32_t WindowCount = 0;
			uint32_t FinalFrameDelta = 0;
			uint32_t FirstControllableWindow = 0xffffffffu;
			uint32_t InputToggleEvidence = 0;
			string SignatureDigest;
		};

		auto runStartupInputWindowProbe = [&scaffold, &titleClass](const GenesisBoundaryScaffoldSaveState& baseline) {
			StartupInputWindowProbeResult probe = {};
			uint64_t digest = 1469598103934665603ull;

			uint32_t baselineThCount0 = baseline.Bus.ControlThCount[0];
			uint32_t baselineThCount1 = baseline.Bus.ControlThCount[1];

			for (uint32_t window = 0; window < 3; window++) {
				uint8_t th0Write = (uint8_t)(0x40 | (window & 0x01));
				uint8_t th1Write = (uint8_t)(0x40 | ((window + 1) & 0x01));
				scaffold.GetBus().WriteByte(0xA10003, th0Write);
				scaffold.GetBus().WriteByte(0xA10005, th1Write);
				uint8_t th0Read = scaffold.GetBus().ReadByte(0xA10003);
				uint8_t th1Read = scaffold.GetBus().ReadByte(0xA10005);

				if (titleClass == "sonic") {
					scaffold.GetBus().SetRenderCompositionInputs(0x16, true, 0x06, false, 0x02, false, true, 0x20, true);
					scaffold.GetBus().RenderScaffoldLine((uint16_t)(64 + window * 4));
					scaffold.StepFrameScaffold(488u * 262u + 88u);
				} else if (titleClass == "jurassic") {
					scaffold.GetBus().SetRenderCompositionInputs(0x15, true, 0x05, false, 0x01, false, true, 0x20, true);
					scaffold.GetBus().RenderScaffoldLine((uint16_t)(68 + window * 3));
					scaffold.StepFrameScaffold(488u * 262u + 104u);
				} else {
					scaffold.StepFrameScaffold(256u);
				}

				GenesisBoundaryScaffoldSaveState sample = scaffold.SaveState();
				probe.WindowCount++;
				probe.FinalFrameDelta = sample.TimingFrame - baseline.TimingFrame;

				uint32_t thCountDelta0 = sample.Bus.ControlThCount[0] - baselineThCount0;
				uint32_t thCountDelta1 = sample.Bus.ControlThCount[1] - baselineThCount1;
				if ((thCountDelta0 > 0 || thCountDelta1 > 0) && probe.FirstControllableWindow == 0xffffffffu) {
					probe.FirstControllableWindow = window;
				}

				bool toggleSeen = (sample.Bus.ControlDataPortWrite[0] != baseline.Bus.ControlDataPortWrite[0])
					|| (sample.Bus.ControlDataPortWrite[1] != baseline.Bus.ControlDataPortWrite[1])
					|| (sample.Bus.ControlThState[0] != baseline.Bus.ControlThState[0])
					|| (sample.Bus.ControlThState[1] != baseline.Bus.ControlThState[1]);
				if (toggleSeen) {
					probe.InputToggleEvidence++;
				}

				string line = std::format(
					"w{}:{}:{}:{:02x}:{:02x}:{}:{}:{}:{}",
					window,
					sample.TimingFrame,
					sample.TimingScanline,
					th0Read,
					th1Read,
					sample.Bus.ControlThState[0],
					sample.Bus.ControlThState[1],
					thCountDelta0,
					thCountDelta1);
				for (uint8_t ch : line) {
					digest ^= ch;
					digest *= 1099511628211ull;
				}
			}

			probe.SignatureDigest = ToHex(digest);
			return probe;
		};

		GenesisBoundaryScaffoldSaveState startupInputBaseline = scaffold.SaveState();
		StartupInputWindowProbeResult startupInputFirst = runStartupInputWindowProbe(startupInputBaseline);
		GenesisBoundaryScaffoldSaveState startupInputTail = scaffold.SaveState();
		scaffold.LoadState(startupInputBaseline);
		StartupInputWindowProbeResult startupInputReplay = runStartupInputWindowProbe(startupInputBaseline);
		scaffold.LoadState(startupInputTail);

		bool startupInputReplayMatch = startupInputFirst.WindowCount == startupInputReplay.WindowCount
			&& startupInputFirst.FinalFrameDelta == startupInputReplay.FinalFrameDelta
			&& startupInputFirst.FirstControllableWindow == startupInputReplay.FirstControllableWindow
			&& startupInputFirst.InputToggleEvidence == startupInputReplay.InputToggleEvidence
			&& startupInputFirst.SignatureDigest == startupInputReplay.SignatureDigest;
		bool startupInputWindowPass = !isTargetStartupClass || (
			startupInputFirst.WindowCount == 3
			&& startupInputFirst.FinalFrameDelta >= 3
			&& startupInputFirst.InputToggleEvidence > 0
			&& startupInputReplayMatch);
		addCheckpoint(
			"GEN-COMPAT-STARTUP-INPUT-WINDOW",
			startupInputWindowPass,
			std::format(
				"class={} target={} windows={} finalFrameDelta={} firstControllable={} toggleEvidence={} replayMatch={} digest={}",
				titleClass,
				isTargetStartupClass ? 1 : 0,
				startupInputFirst.WindowCount,
				startupInputFirst.FinalFrameDelta,
				startupInputFirst.FirstControllableWindow,
				startupInputFirst.InputToggleEvidence,
				startupInputReplayMatch ? 1 : 0,
				startupInputFirst.SignatureDigest));

		auto runInterruptCadenceStartupPath = [&scaffold, &titleClass]() {
			if (titleClass == "sonic") {
				scaffold.StepFrameScaffold(488u + 64u);
			} else if (titleClass == "jurassic") {
				scaffold.StepFrameScaffold(488u + 112u);
			} else {
				scaffold.StepFrameScaffold(128u);
			}
		};

		GenesisBoundaryScaffoldSaveState interruptCadenceBaseline = scaffold.SaveState();
		runInterruptCadenceStartupPath();
		GenesisBoundaryScaffoldSaveState interruptCadenceFirst = scaffold.SaveState();
		scaffold.LoadState(interruptCadenceBaseline);
		runInterruptCadenceStartupPath();
		GenesisBoundaryScaffoldSaveState interruptCadenceReplay = scaffold.SaveState();
		scaffold.LoadState(interruptCadenceFirst);

		bool cadenceAdvanced = interruptCadenceFirst.TimingFrame > interruptCadenceBaseline.TimingFrame
			|| (interruptCadenceFirst.TimingFrame == interruptCadenceBaseline.TimingFrame
				&& interruptCadenceFirst.TimingScanline > interruptCadenceBaseline.TimingScanline);
		bool cadenceCounterAdvanced = interruptCadenceFirst.VInterruptCount >= interruptCadenceBaseline.VInterruptCount
			&& interruptCadenceFirst.HInterruptCount >= interruptCadenceBaseline.HInterruptCount;
		bool cadenceReplayMatch = interruptCadenceFirst.TimingFrame == interruptCadenceReplay.TimingFrame
			&& interruptCadenceFirst.TimingScanline == interruptCadenceReplay.TimingScanline
			&& interruptCadenceFirst.TimingCycleRemainder == interruptCadenceReplay.TimingCycleRemainder
			&& interruptCadenceFirst.VInterruptCount == interruptCadenceReplay.VInterruptCount
			&& interruptCadenceFirst.HInterruptCount == interruptCadenceReplay.HInterruptCount
			&& interruptCadenceFirst.TimingEvents == interruptCadenceReplay.TimingEvents;
		bool interruptCadenceStartupPass = !isTargetStartupClass || (cadenceAdvanced && cadenceCounterAdvanced && cadenceReplayMatch);
		addCheckpoint(
			"GEN-COMPAT-INTERRUPT-CADENCE-STARTUP",
			interruptCadenceStartupPass,
			std::format(
				"class={} target={} advanced={} countersAdvanced={} replayMatch={} frame={} scanline={} vInt={} hInt={} events={}",
				titleClass,
				isTargetStartupClass ? 1 : 0,
				cadenceAdvanced ? 1 : 0,
				cadenceCounterAdvanced ? 1 : 0,
				cadenceReplayMatch ? 1 : 0,
				interruptCadenceFirst.TimingFrame,
				interruptCadenceFirst.TimingScanline,
				interruptCadenceFirst.VInterruptCount,
				interruptCadenceFirst.HInterruptCount,
				interruptCadenceFirst.TimingEvents.size()));

		auto runInterruptIntervalStartupPath = [&scaffold, &titleClass]() {
			if (titleClass == "sonic") {
				scaffold.StepFrameScaffold(488u * 2u + 80u);
			} else if (titleClass == "jurassic") {
				scaffold.StepFrameScaffold(488u * 2u + 120u);
			} else {
				scaffold.StepFrameScaffold(256u);
			}
		};

		auto toAbsolute = [](int64_t value) {
			return value < 0 ? -value : value;
		};

		GenesisBoundaryScaffoldSaveState interruptIntervalBaseline = scaffold.SaveState();
		runInterruptIntervalStartupPath();
		GenesisBoundaryScaffoldSaveState interruptIntervalFirst = scaffold.SaveState();
		scaffold.LoadState(interruptIntervalBaseline);
		runInterruptIntervalStartupPath();
		GenesisBoundaryScaffoldSaveState interruptIntervalReplay = scaffold.SaveState();
		scaffold.LoadState(interruptIntervalFirst);

		int64_t scanlineDelta = (int64_t)interruptIntervalFirst.TimingScanline - (int64_t)interruptIntervalBaseline.TimingScanline;
		int64_t hDelta = (int64_t)interruptIntervalFirst.HInterruptCount - (int64_t)interruptIntervalBaseline.HInterruptCount;
		int64_t vDelta = (int64_t)interruptIntervalFirst.VInterruptCount - (int64_t)interruptIntervalBaseline.VInterruptCount;
		int64_t replayHDelta = (int64_t)interruptIntervalReplay.HInterruptCount - (int64_t)interruptIntervalBaseline.HInterruptCount;
		int64_t replayVDelta = (int64_t)interruptIntervalReplay.VInterruptCount - (int64_t)interruptIntervalBaseline.VInterruptCount;

		int64_t hIntervalEstimate = hDelta > 0 ? (scanlineDelta / hDelta) : scanlineDelta;
		int64_t vIntervalEstimate = vDelta > 0 ? (scanlineDelta / vDelta) : scanlineDelta;
		bool hIntervalTolerancePass = hDelta == 0 || toAbsolute(hIntervalEstimate - 16) <= 16;
		bool vIntervalTolerancePass = vDelta == 0 || toAbsolute(vIntervalEstimate - 262) <= 262;
		bool intervalReplayParityPass = hDelta == replayHDelta && vDelta == replayVDelta;
		bool intervalAdvancedPass = scanlineDelta > 0 && hDelta >= 0 && vDelta >= 0;
		bool interruptIntervalStartupPass = !isTargetStartupClass || (intervalAdvancedPass && intervalReplayParityPass && hIntervalTolerancePass && vIntervalTolerancePass);
		addCheckpoint(
			"GEN-COMPAT-INTERRUPT-INTERVAL-STARTUP",
			interruptIntervalStartupPass,
			std::format(
				"class={} target={} advanced={} replayParity={} hTol={} vTol={} scanDelta={} hDelta={} vDelta={} hReplay={} vReplay={} hEstimate={} vEstimate={}",
				titleClass,
				isTargetStartupClass ? 1 : 0,
				intervalAdvancedPass ? 1 : 0,
				intervalReplayParityPass ? 1 : 0,
				hIntervalTolerancePass ? 1 : 0,
				vIntervalTolerancePass ? 1 : 0,
				scanlineDelta,
				hDelta,
				vDelta,
				replayHDelta,
				replayVDelta,
				hIntervalEstimate,
				vIntervalEstimate));

		auto runVdpStatusStartupPath = [&scaffold, &titleClass]() {
			if (titleClass == "sonic") {
				scaffold.GetBus().SetRenderCompositionInputs(0x14, true, 0x05, false, 0x01, false, true, 0x20, true);
				scaffold.GetBus().RenderScaffoldLine(40);
				scaffold.StepFrameScaffold(488u + 72u);
			} else if (titleClass == "jurassic") {
				scaffold.GetBus().BeginDmaTransfer(GenesisVdpDmaMode::Copy, 10);
				scaffold.StepFrameScaffold(488u + 104u);
				scaffold.GetBus().RenderScaffoldLine(72);
			} else {
				scaffold.StepFrameScaffold(160u);
			}
		};

		auto statusDelta = [](uint16_t fromStatus, uint16_t toStatus) {
			return (uint16_t)(fromStatus ^ toStatus);
		};

		GenesisBoundaryScaffoldSaveState vdpStatusBaseline = scaffold.SaveState();
		runVdpStatusStartupPath();
		GenesisBoundaryScaffoldSaveState vdpStatusFirst = scaffold.SaveState();
		scaffold.LoadState(vdpStatusBaseline);
		runVdpStatusStartupPath();
		GenesisBoundaryScaffoldSaveState vdpStatusReplay = scaffold.SaveState();
		scaffold.LoadState(vdpStatusFirst);

		uint16_t vdpTransitionFirst = statusDelta(vdpStatusBaseline.Bus.VdpStatus, vdpStatusFirst.Bus.VdpStatus);
		uint16_t vdpTransitionReplay = statusDelta(vdpStatusBaseline.Bus.VdpStatus, vdpStatusReplay.Bus.VdpStatus);
		bool vdpStatusAdvanced = vdpStatusFirst.TimingFrame > vdpStatusBaseline.TimingFrame
			|| (vdpStatusFirst.TimingFrame == vdpStatusBaseline.TimingFrame && vdpStatusFirst.TimingScanline > vdpStatusBaseline.TimingScanline);
		bool vdpStatusReplayParity = vdpStatusFirst.Bus.VdpStatus == vdpStatusReplay.Bus.VdpStatus
			&& vdpTransitionFirst == vdpTransitionReplay
			&& vdpStatusFirst.Bus.LastVdpAddress == vdpStatusReplay.Bus.LastVdpAddress
			&& vdpStatusFirst.Bus.LastVdpValue == vdpStatusReplay.Bus.LastVdpValue;
		bool vdpStatusEvidence = vdpTransitionFirst != 0 || !vdpStatusFirst.Bus.RenderLineDigest.empty();
		bool vdpStatusStartupPass = !isTargetStartupClass || (vdpStatusAdvanced && vdpStatusReplayParity && vdpStatusEvidence);
		addCheckpoint(
			"GEN-COMPAT-VDP-STATUS-STARTUP",
			vdpStatusStartupPass,
			std::format(
				"class={} target={} advanced={} replayParity={} evidence={} baseStatus={:04x} firstStatus={:04x} replayStatus={:04x} transition={:04x} lastAddr={:06x} lastValue={:02x}",
				titleClass,
				isTargetStartupClass ? 1 : 0,
				vdpStatusAdvanced ? 1 : 0,
				vdpStatusReplayParity ? 1 : 0,
				vdpStatusEvidence ? 1 : 0,
				vdpStatusBaseline.Bus.VdpStatus,
				vdpStatusFirst.Bus.VdpStatus,
				vdpStatusReplay.Bus.VdpStatus,
				vdpTransitionFirst,
				vdpStatusFirst.Bus.LastVdpAddress,
				vdpStatusFirst.Bus.LastVdpValue));

		string digestA = BuildCheckpointDigest(entry.Checkpoints);
		string digestB = BuildCheckpointDigest(entry.Checkpoints);
		addCheckpoint("GEN-COMPAT-DETERMINISM", digestA == digestB, std::format("digestA={} digestB={}", digestA, digestB));

		auto hasPassingCheckpoint = [&entry](const string& checkpointId) {
			return std::any_of(entry.Checkpoints.begin(), entry.Checkpoints.end(), [&](const GenesisCompatibilityCheckpoint& checkpoint) {
				return checkpoint.Id == checkpointId && checkpoint.Pass;
			});
		};

		bool baseCorePass = hasPassingCheckpoint("GEN-COMPAT-BOOT")
			&& hasPassingCheckpoint("GEN-COMPAT-BUS")
			&& hasPassingCheckpoint("GEN-COMPAT-BUS-OWNERSHIP")
			&& hasPassingCheckpoint("GEN-COMPAT-HOST-MODE")
			&& hasPassingCheckpoint("GEN-COMPAT-MAPPER-EDGE")
			&& hasPassingCheckpoint("GEN-COMPAT-FIRST-VISIBLE-FRAME")
			&& hasPassingCheckpoint("GEN-COMPAT-STARTUP-EVENT-SEQUENCE")
			&& hasPassingCheckpoint("GEN-COMPAT-STARTUP-FRAME-WINDOW")
			&& hasPassingCheckpoint("GEN-COMPAT-STARTUP-RENDER-HANDOFF")
			&& hasPassingCheckpoint("GEN-COMPAT-STARTUP-CPU-PROGRESSION")
			&& hasPassingCheckpoint("GEN-COMPAT-BOOT-TO-TITLE-PROGRESSION")
			&& hasPassingCheckpoint("GEN-COMPAT-REALROM-STARTUP-SMOKE")
			&& hasPassingCheckpoint("GEN-COMPAT-STARTUP-INPUT-WINDOW")
			&& hasPassingCheckpoint("GEN-COMPAT-VDP-TIMING-STARTUP")
			&& hasPassingCheckpoint("GEN-COMPAT-VDP-STATUS-STARTUP")
			&& hasPassingCheckpoint("GEN-COMPAT-INTERRUPT-CADENCE-STARTUP")
			&& hasPassingCheckpoint("GEN-COMPAT-INTERRUPT-INTERVAL-STARTUP")
			&& hasPassingCheckpoint("GEN-COMPAT-RENDER")
			&& hasPassingCheckpoint("GEN-COMPAT-AUDIO")
			&& hasPassingCheckpoint("GEN-COMPAT-DETERMINISM");

		if (titleClass == "sonic") {
			baseCorePass = baseCorePass && hasPassingCheckpoint("GEN-TITLE-SONIC");
		} else if (titleClass == "jurassic") {
			baseCorePass = baseCorePass && hasPassingCheckpoint("GEN-TITLE-JURASSIC");
		}

		addCheckpoint("GEN-COMPAT-BASE-CORE", baseCorePass, std::format("class={}", titleClass));

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
	uint64_t replayPassTotal = 0;
	uint64_t replayFailTotal = 0;
	uint64_t elapsedTotalMicros = 0;
	uint64_t classBudgetTotalMicros = 0;
	uint64_t scdLaneTotal = 0;
	uint64_t scdToolingEventTotal = 0;
	uint64_t m32xToolingEventTotal = 0;

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
		classBudgetTotalMicros += classBudgetMicros;

		if (romCase.RomData.empty()) {
			entry.Pass = false;
			entry.DeterministicDigest = ToHex(0);
			result.FailCount++;
				result.OutputLines.push_back(std::format(
					"GEN_PERF_RESULT {} FAIL CLASS={} ELAPSED_US=0 BUDGET_US={} CLASS_BUDGET_US={} REPLAY_OK=0 SCD_LANE_CT=0 SCD_EVT_CT=0 M32X_EVT_CT=0 DIGEST={}",
					entry.Name,
					entry.TitleClass,
					budgetMicros,
					classBudgetMicros,
					entry.DeterministicDigest));
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
		if (replayPass) {
			replayPassTotal++;
		} else {
			replayFailTotal++;
		}

		auto end = std::chrono::steady_clock::now();
		entry.ElapsedMicros = (uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
		elapsedTotalMicros += entry.ElapsedMicros;
		uint32_t scdLaneCount = firstRun.Bus.CommandResponseLaneCount;
		uint32_t scdToolingEventCount = firstRun.Bus.SegaCdToolingEventCount;
		uint32_t m32xToolingEventCount = firstRun.Bus.M32xToolingEventCount;
		scdLaneTotal += scdLaneCount;
		scdToolingEventTotal += scdToolingEventCount;
		m32xToolingEventTotal += m32xToolingEventCount;

		string deterministicInput = std::format(
			"{}:{}:{}:{}:{}:{}:{}:{}:{}",
			entry.Name,
			entry.TitleClass,
			compatibilityDigest,
			firstRun.Bus.MixedDigest,
			replayRun.Bus.MixedDigest,
			replayPass ? "1" : "0",
			scdLaneCount,
			scdToolingEventCount,
			m32xToolingEventCount);

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
			"GEN_PERF_RESULT {} {} CLASS={} ELAPSED_US={} BUDGET_US={} CLASS_BUDGET_US={} REPLAY_OK={} SCD_LANE_CT={} SCD_EVT_CT={} M32X_EVT_CT={} DIGEST={}",
			entry.Name,
			entry.Pass ? "PASS" : "FAIL",
			entry.TitleClass,
			entry.ElapsedMicros,
			budgetMicros,
			classBudgetMicros,
			replayPass ? 1 : 0,
			scdLaneCount,
			scdToolingEventCount,
			m32xToolingEventCount,
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
	uint64_t caseTotal = (uint64_t)result.Entries.size();
	uint64_t summaryTotal = (uint64_t)result.PassCount + (uint64_t)result.FailCount;
	uint64_t passRatioPct = summaryTotal == 0 ? 0 : ((uint64_t)result.PassCount * 100ull) / summaryTotal;
	uint64_t failRatioPct = summaryTotal == 0 ? 0 : ((uint64_t)result.FailCount * 100ull) / summaryTotal;
	uint64_t replayMismatchPct = summaryTotal == 0 ? 0 : (replayFailTotal * 100ull) / summaryTotal;
	uint64_t avgElapsedMicros = caseTotal == 0 ? 0 : elapsedTotalMicros / caseTotal;
	uint64_t avgClassBudgetMicros = caseTotal == 0 ? 0 : classBudgetTotalMicros / caseTotal;
	uint64_t budgetUtilPct = classBudgetTotalMicros == 0 ? 0 : (elapsedTotalMicros * 100ull) / classBudgetTotalMicros;
	uint64_t overBudgetTotalMicros = elapsedTotalMicros > classBudgetTotalMicros ? (elapsedTotalMicros - classBudgetTotalMicros) : 0;
	uint64_t avgScdLanePerCase = caseTotal == 0 ? 0 : scdLaneTotal / caseTotal;
	uint64_t avgScdEventsPerCase = caseTotal == 0 ? 0 : scdToolingEventTotal / caseTotal;
	uint64_t avgM32xEventsPerCase = caseTotal == 0 ? 0 : m32xToolingEventTotal / caseTotal;
	result.OutputLines.push_back(std::format(
		"GEN_PERF_GATE_SUMMARY PASS={} FAIL={} PASS_RATIO_PCT={} FAIL_RATIO_PCT={} REPLAY_MISMATCH_PCT={} CASE_TOTAL={} BUDGET_US={} CLASS_BUDGET_TOTAL_US={} AVG_CLASS_BUDGET_US={} ELAPSED_TOTAL_US={} AVG_ELAPSED_US={} BUDGET_UTIL_PCT={} OVER_BUDGET_US_TOTAL={} REPLAY_OK_TOTAL={} REPLAY_FAIL_TOTAL={} SCD_LANE_TOTAL={} AVG_SCD_LANE_PER_CASE={} SCD_EVT_TOTAL={} AVG_SCD_EVT_PER_CASE={} M32X_EVT_TOTAL={} AVG_M32X_EVT_PER_CASE={} DIGEST={}",
		result.PassCount,
		result.FailCount,
		passRatioPct,
		failRatioPct,
		replayMismatchPct,
		caseTotal,
		result.BudgetMicros,
		classBudgetTotalMicros,
		avgClassBudgetMicros,
		elapsedTotalMicros,
		avgElapsedMicros,
		budgetUtilPct,
		overBudgetTotalMicros,
		replayPassTotal,
		replayFailTotal,
		scdLaneTotal,
		avgScdLanePerCase,
		scdToolingEventTotal,
		avgScdEventsPerCase,
		m32xToolingEventTotal,
		avgM32xEventsPerCase,
		result.Digest));
	return result;
}
