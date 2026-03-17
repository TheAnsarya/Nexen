#pragma once
#include "pch.h"

class Atari2600Console;

struct Atari2600HarnessCheckpoint {
	string Id;
	bool Pass = false;
	string Context;
};

struct Atari2600HarnessResult {
	vector<Atari2600HarnessCheckpoint> Checkpoints;
	int PassCount = 0;
	int FailCount = 0;
	string Digest;
	vector<string> OutputLines;
};

struct Atari2600TimingSpikeResult {
	vector<uint32_t> ScanlineDeltas;
	bool Stable = false;
	string Digest;
	uint32_t InitialScanline = 0;
	uint32_t FinalScanline = 0;
	uint32_t FinalColorClock = 0;
	vector<string> OutputLines;
};

struct Atari2600BaselineRomCase {
	string Name;
	vector<uint8_t> RomData;
};

struct Atari2600BaselineRomEntry {
	string Name;
	bool Pass = false;
	Atari2600HarnessResult Result = {};
};

struct Atari2600BaselineRomSetResult {
	vector<Atari2600BaselineRomEntry> Entries;
	int PassCount = 0;
	int FailCount = 0;
	string Digest;
	vector<string> OutputLines;
};

struct Atari2600CompatibilityCheckpoint {
	string Id;
	bool Pass = false;
	string Context;
};

struct Atari2600CompatibilityEntry {
	string Name;
	string MapperMode;
	vector<Atari2600CompatibilityCheckpoint> Checkpoints;
	bool Pass = false;
	int PassCount = 0;
	int FailCount = 0;
	string Digest;
	vector<string> OutputLines;
};

struct Atari2600CompatibilityMatrixResult {
	vector<Atari2600CompatibilityEntry> Entries;
	int PassCount = 0;
	int FailCount = 0;
	string Digest;
	vector<string> OutputLines;
};

struct Atari2600PerformanceGateEntry {
	string Name;
	string MapperMode;
	bool Pass = false;
	uint64_t ElapsedMicros = 0;
	string DeterministicDigest;
};

struct Atari2600PerformanceGateResult {
	vector<Atari2600PerformanceGateEntry> Entries;
	int PassCount = 0;
	int FailCount = 0;
	uint64_t BudgetMicros = 0;
	string Digest;
	vector<string> OutputLines;
};

class Atari2600SmokeHarness {
public:
	static Atari2600HarnessResult RunBaseline(Atari2600Console& console);
	static Atari2600TimingSpikeResult RunTimingSpike(Atari2600Console& console, uint32_t iterations = 8);
	static Atari2600BaselineRomSetResult RunBaselineRomSet(Atari2600Console& console, const vector<Atari2600BaselineRomCase>& romSet);
	static Atari2600CompatibilityMatrixResult RunCompatibilityMatrix(Atari2600Console& console, const vector<Atari2600BaselineRomCase>& romSet);
	static Atari2600PerformanceGateResult RunPerformanceGate(Atari2600Console& console, const vector<Atari2600BaselineRomCase>& romSet, uint64_t budgetMicros = 25000);
};
