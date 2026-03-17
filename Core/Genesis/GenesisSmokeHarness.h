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

class GenesisSmokeHarness {
public:
	static GenesisCompatibilityMatrixResult RunCompatibilityMatrix(GenesisM68kBoundaryScaffold& scaffold, const vector<GenesisCompatibilityRomCase>& romSet);
};
