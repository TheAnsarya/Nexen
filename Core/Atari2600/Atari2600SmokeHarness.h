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

class Atari2600SmokeHarness {
public:
	static Atari2600HarnessResult RunBaseline(Atari2600Console& console);
	static Atari2600TimingSpikeResult RunTimingSpike(Atari2600Console& console, uint32_t iterations = 8);
};
