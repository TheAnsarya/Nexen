#pragma once
#include "pch.h"

class GenesisM68kBoundaryScaffold;

struct GenesisCompatibilityRomCase {
	string Name;
	vector<uint8_t> RomData;
};

struct GenesisCompatibilityCheckpoint {
	string Id;
	bool Pass = false;
	string Context;
};

struct GenesisCompatibilityEntry {
	string Name;
	vector<GenesisCompatibilityCheckpoint> Checkpoints;
	bool Pass = false;
	int PassCount = 0;
	int FailCount = 0;
	string Digest;
	vector<string> OutputLines;
};

struct GenesisCompatibilityMatrixResult {
	vector<GenesisCompatibilityEntry> Entries;
	int PassCount = 0;
	int FailCount = 0;
	string Digest;
	vector<string> OutputLines;
};

struct GenesisPerformanceGateEntry {
	string Name;
	string TitleClass;
	bool Pass = false;
	uint64_t ElapsedMicros = 0;
	string DeterministicDigest;
};

struct GenesisPerformanceGateResult {
	vector<GenesisPerformanceGateEntry> Entries;
	int PassCount = 0;
	int FailCount = 0;
	uint64_t BudgetMicros = 0;
	string Digest;
	vector<string> OutputLines;
};

struct GenesisStartupDeterminismGateEntry {
	string Name;
	string TitleClass;
	bool Pass = false;
	int RequiredCheckpointCount = 0;
	int PassingRequiredCheckpointCount = 0;
	string SignatureA;
	string SignatureB;
};

struct GenesisStartupDeterminismGateResult {
	vector<GenesisStartupDeterminismGateEntry> Entries;
	int PassCount = 0;
	int FailCount = 0;
	uint32_t FrameWindow = 0;
	string Digest;
	vector<string> OutputLines;
};

struct GenesisExecutionResilienceGateEntry {
	string Name;
	string TitleClass;
	bool Pass = false;
	uint32_t FrameCount = 0;
	uint64_t StallEvents = 0;
	uint64_t ForcedAdvances = 0;
	string TraceDigest;
	string StallSummary;
};

struct GenesisExecutionResilienceGateResult {
	vector<GenesisExecutionResilienceGateEntry> Entries;
	int PassCount = 0;
	int FailCount = 0;
	uint32_t FrameWindow = 0;
	string Digest;
	vector<string> OutputLines;
};

class GenesisSmokeHarness {
public:
	static GenesisCompatibilityMatrixResult RunCompatibilityMatrix(GenesisM68kBoundaryScaffold& scaffold, const vector<GenesisCompatibilityRomCase>& romSet);
	static GenesisPerformanceGateResult RunPerformanceGate(GenesisM68kBoundaryScaffold& scaffold, const vector<GenesisCompatibilityRomCase>& romSet, uint64_t budgetMicros = 25000);
	static GenesisStartupDeterminismGateResult RunStartupDeterminismGate(GenesisM68kBoundaryScaffold& scaffold, const vector<GenesisCompatibilityRomCase>& romSet, uint32_t frameWindow = 600);
	static GenesisExecutionResilienceGateResult RunExecutionResilienceGate(GenesisM68kBoundaryScaffold& scaffold, const vector<GenesisCompatibilityRomCase>& romSet, uint32_t frameWindow = 600);
};
