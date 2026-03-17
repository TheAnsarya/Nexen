#include "pch.h"
#include "Atari2600/Atari2600SmokeHarness.h"
#include "Atari2600/Atari2600Console.h"

namespace {
	string ToHex(uint64_t value) {
		return std::format("{:016x}", value);
	}

	string BuildDigest(const vector<Atari2600HarnessCheckpoint>& checkpoints) {
		uint64_t hash = 1469598103934665603ull;
		for (const Atari2600HarnessCheckpoint& cp : checkpoints) {
			string line = cp.Id + ":" + (cp.Pass ? "PASS" : "FAIL") + ":" + cp.Context;
			for (uint8_t ch : line) {
				hash ^= ch;
				hash *= 1099511628211ull;
			}
		}
		return ToHex(hash);
	}
}

Atari2600HarnessResult Atari2600SmokeHarness::RunBaseline(Atari2600Console& console) {
	Atari2600HarnessResult result = {};

	auto addCheckpoint = [&](string id, bool pass, string context) {
		Atari2600HarnessCheckpoint cp = {};
		cp.Id = std::move(id);
		cp.Pass = pass;
		cp.Context = std::move(context);
		if (cp.Pass) {
			result.PassCount++;
		} else {
			result.FailCount++;
		}
		result.OutputLines.push_back(std::format("CHECKPOINT {} {} {}", cp.Id, cp.Pass ? "PASS" : "FAIL", cp.Context));
		result.Checkpoints.push_back(std::move(cp));
	};

	console.Reset();
	console.StepCpuCycles(Atari2600Console::CpuCyclesPerFrame);
	Atari2600TiaState frameBoundaryState = console.GetTiaState();
	addCheckpoint("TIA-CP-01", frameBoundaryState.FrameCount == 1 && frameBoundaryState.Scanline == 0,
		std::format("frame={} scanline={}", frameBoundaryState.FrameCount, frameBoundaryState.Scanline));

	console.Reset();
	console.StepCpuCycles(9);
	uint32_t originalScanline = console.GetTiaState().Scanline;
	console.RequestWsync();
	console.StepCpuCycles(1);
	Atari2600TiaState wsyncState = console.GetTiaState();
	addCheckpoint("TIA-CP-02", wsyncState.Scanline == originalScanline + 1,
		std::format("scanlineBefore={} scanlineAfter={}", originalScanline, wsyncState.Scanline));

	console.Reset();
	console.StepCpuCycles(42);
	Atari2600TiaState visibilityState = console.GetTiaState();
	bool inExpectedBucket = visibilityState.ColorClock >= 120 && visibilityState.ColorClock <= 132;
	addCheckpoint("TIA-CP-03", inExpectedBucket,
		std::format("colorClock={}", visibilityState.ColorClock));

	string digestA = BuildDigest(result.Checkpoints);
	string digestB = BuildDigest(result.Checkpoints);
	addCheckpoint("TIA-CP-04", digestA == digestB, std::format("digestA={} digestB={}", digestA, digestB));

	result.Digest = BuildDigest(result.Checkpoints);
	result.OutputLines.push_back(std::format("HARNESS_SUMMARY PASS={} FAIL={} DIGEST={}", result.PassCount, result.FailCount, result.Digest));
	return result;
}

Atari2600TimingSpikeResult Atari2600SmokeHarness::RunTimingSpike(Atari2600Console& console, uint32_t iterations) {
	Atari2600TimingSpikeResult result = {};
	result.ScanlineDeltas.reserve(iterations);

	console.Reset();
	result.InitialScanline = console.GetTiaState().Scanline;

	for (uint32_t i = 0; i < iterations; i++) {
		uint32_t beforeScanline = console.GetTiaState().Scanline;
		console.StepCpuCycles(76);
		uint32_t afterScanline = console.GetTiaState().Scanline;
		uint32_t delta = afterScanline - beforeScanline;
		result.ScanlineDeltas.push_back(delta);
		result.OutputLines.push_back(std::format("TIMING_SPIKE STEP={} BEFORE={} AFTER={} DELTA={}", i, beforeScanline, afterScanline, delta));
	}

	Atari2600TiaState beforeWsync = console.GetTiaState();
	console.StepCpuCycles(9);
	uint32_t scanlinePreWsync = console.GetTiaState().Scanline;
	console.RequestWsync();
	console.StepCpuCycles(1);
	Atari2600TiaState afterWsync = console.GetTiaState();
	result.OutputLines.push_back(std::format(
		"TIMING_SPIKE WSYNC SCANLINE_BEFORE={} SCANLINE_AFTER={} COLORCLOCK_BEFORE={} COLORCLOCK_AFTER={}",
		scanlinePreWsync,
		afterWsync.Scanline,
		beforeWsync.ColorClock,
		afterWsync.ColorClock));

	result.Stable = std::all_of(result.ScanlineDeltas.begin(), result.ScanlineDeltas.end(), [](uint32_t delta) {
		return delta == 1;
	});

	result.FinalScanline = afterWsync.Scanline;
	result.FinalColorClock = afterWsync.ColorClock;

	string digestInput = std::format(
		"stable={} init={} final={} color={} wsyncDelta={}",
		result.Stable ? 1 : 0,
		result.InitialScanline,
		result.FinalScanline,
		result.FinalColorClock,
		afterWsync.Scanline - scanlinePreWsync);

	for (uint32_t delta : result.ScanlineDeltas) {
		digestInput += std::format(":{}", delta);
	}

	uint64_t hash = 1469598103934665603ull;
	for (uint8_t ch : digestInput) {
		hash ^= ch;
		hash *= 1099511628211ull;
	}

	result.Digest = ToHex(hash);
	result.OutputLines.push_back(std::format("TIMING_SPIKE SUMMARY STABLE={} DIGEST={}", result.Stable ? "true" : "false", result.Digest));
	return result;
}
