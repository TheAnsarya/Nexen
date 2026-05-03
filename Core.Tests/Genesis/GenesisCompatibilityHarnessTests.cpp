#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"
#include "Genesis/GenesisSmokeHarness.h"

namespace {
	vector<GenesisCompatibilityRomCase> BuildGenesisCompatibilityCorpus() {
		vector<GenesisCompatibilityRomCase> corpus;
		corpus.push_back({"sonic-1.bin", vector<uint8_t>(0x8000, 0x4E)});
		corpus.push_back({"jurassic-park.bin", vector<uint8_t>(0x8000, 0x4E)});
		return corpus;
	}

	TEST(GenesisCompatibilityHarnessTests, CompatibilityDigestIsDeterministicAcrossRuns) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus = BuildGenesisCompatibilityCorpus();

		GenesisCompatibilityMatrixResult runA = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);
		GenesisCompatibilityMatrixResult runB = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);

		EXPECT_EQ(runA.PassCount, (int)corpus.size());
		EXPECT_EQ(runA.FailCount, 0);
		EXPECT_FALSE(runA.Digest.empty());
		EXPECT_EQ(runA.Digest, runB.Digest);
	}

	TEST(GenesisCompatibilityHarnessTests, EmptyRomEntryTriggersFailureDiagnostics) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus;
		corpus.push_back({"empty.bin", {}});

		GenesisCompatibilityMatrixResult result = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);
		EXPECT_EQ(result.PassCount, 0);
		EXPECT_EQ(result.FailCount, 1);
		EXPECT_FALSE(result.Digest.empty());

		bool hasFailContext = std::any_of(result.OutputLines.begin(), result.OutputLines.end(), [](const string& line) {
			return line.starts_with("GEN_COMPAT_FAIL_CONTEXT ");
		});
		EXPECT_TRUE(hasFailContext);
	}

	TEST(GenesisCompatibilityHarnessTests, BenchmarkCorpusAssumptionsHoldForAllPassScenario) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus = BuildGenesisCompatibilityCorpus();

		GenesisCompatibilityMatrixResult result = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);

		ASSERT_FALSE(corpus.empty());
		EXPECT_EQ((int)result.Entries.size(), (int)corpus.size());
		EXPECT_EQ(result.PassCount + result.FailCount, (int)corpus.size());
		EXPECT_EQ(result.FailCount, 0);
		EXPECT_FALSE(result.Digest.empty());
	}

	TEST(GenesisCompatibilityHarnessTests, MapperEdgeAndOwnershipCheckpointsArePresentAndPassing) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus = BuildGenesisCompatibilityCorpus();

		GenesisCompatibilityMatrixResult result = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);

		ASSERT_EQ((int)result.Entries.size(), (int)corpus.size());
		for (const GenesisCompatibilityEntry& entry : result.Entries) {
			auto hasPassingCheckpoint = [&](const string& checkpointId) {
				return std::any_of(entry.Checkpoints.begin(), entry.Checkpoints.end(), [&](const GenesisCompatibilityCheckpoint& checkpoint) {
					return checkpoint.Id == checkpointId && checkpoint.Pass;
				});
			};

			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-BUS-OWNERSHIP"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-MAPPER-EDGE"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-HOST-MODE"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-TOOLING-MATRIX"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-SCD-TELEMETRY"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-32X-DUAL-SH2"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-32X-COMPOSE-SYNC"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-32X-TOOLING"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-32X-TELEMETRY"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-DEBUG-LANE"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-FIRST-VISIBLE-FRAME"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-STARTUP-EVENT-SEQUENCE"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-STARTUP-FRAME-WINDOW"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-STARTUP-RENDER-HANDOFF"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-STARTUP-CPU-PROGRESSION"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-BOOT-TO-TITLE-PROGRESSION"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-TAS-CHEAT"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-PBC-BOOT"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-PBC-RUNTIME"));
		}
	}

	TEST(GenesisCompatibilityHarnessTests, BaseConsoleReadinessCheckpointsArePresentForTargetCorpus) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus = BuildGenesisCompatibilityCorpus();

		GenesisCompatibilityMatrixResult result = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);

		ASSERT_EQ((int)result.Entries.size(), (int)corpus.size());
		for (const GenesisCompatibilityEntry& entry : result.Entries) {
			auto hasPassingCheckpoint = [&](const string& checkpointId) {
				return std::any_of(entry.Checkpoints.begin(), entry.Checkpoints.end(), [&](const GenesisCompatibilityCheckpoint& checkpoint) {
					return checkpoint.Id == checkpointId && checkpoint.Pass;
				});
			};

			// Base Genesis/Mega Drive readiness gates (no Sega CD/32X/PBC dependencies).
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-BUS-OWNERSHIP"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-MAPPER-EDGE"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-HOST-MODE"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-FIRST-VISIBLE-FRAME"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-STARTUP-EVENT-SEQUENCE"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-STARTUP-FRAME-WINDOW"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-STARTUP-RENDER-HANDOFF"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-STARTUP-CPU-PROGRESSION"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-BOOT-TO-TITLE-PROGRESSION"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-DETERMINISM"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-DEBUG-LANE"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-TAS-CHEAT"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-VDP-TIMING-STARTUP"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-VDP-STATUS-STARTUP"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-INTERRUPT-CADENCE-STARTUP"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-INTERRUPT-INTERVAL-STARTUP"));
			EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-BASE-CORE"));
		}
	}

	TEST(GenesisCompatibilityHarnessTests, VdpTimingStartupCheckpointContextIsDeterministicAcrossRuns) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus = BuildGenesisCompatibilityCorpus();

		auto readVdpTimingContext = [&](const GenesisCompatibilityMatrixResult& result, const string& romName) {
			auto entryIt = std::find_if(result.Entries.begin(), result.Entries.end(), [&](const GenesisCompatibilityEntry& entry) {
				return entry.Name == romName;
			});
			if (entryIt == result.Entries.end()) {
				ADD_FAILURE() << "Missing compatibility entry for " << romName;
				return string();
			}

			auto checkpointIt = std::find_if(entryIt->Checkpoints.begin(), entryIt->Checkpoints.end(), [](const GenesisCompatibilityCheckpoint& checkpoint) {
				return checkpoint.Id == "GEN-COMPAT-VDP-TIMING-STARTUP";
			});
			if (checkpointIt == entryIt->Checkpoints.end()) {
				ADD_FAILURE() << "Missing GEN-COMPAT-VDP-TIMING-STARTUP checkpoint for " << romName;
				return string();
			}

			EXPECT_TRUE(checkpointIt->Pass);
			return checkpointIt->Context;
		};

		GenesisCompatibilityMatrixResult runA = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);
		GenesisCompatibilityMatrixResult runB = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);

		ASSERT_EQ((int)runA.Entries.size(), (int)corpus.size());
		ASSERT_EQ((int)runB.Entries.size(), (int)corpus.size());

		for (const GenesisCompatibilityRomCase& romCase : corpus) {
			string contextA = readVdpTimingContext(runA, romCase.Name);
			string contextB = readVdpTimingContext(runB, romCase.Name);
			EXPECT_EQ(contextA, contextB);
		}
	}

	TEST(GenesisCompatibilityHarnessTests, FirstVisibleFrameCheckpointContextIsDeterministicAcrossRuns) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus = BuildGenesisCompatibilityCorpus();

		auto readFirstVisibleFrameContext = [&](const GenesisCompatibilityMatrixResult& result, const string& romName) {
			auto entryIt = std::find_if(result.Entries.begin(), result.Entries.end(), [&](const GenesisCompatibilityEntry& entry) {
				return entry.Name == romName;
			});
			if (entryIt == result.Entries.end()) {
				ADD_FAILURE() << "Missing compatibility entry for " << romName;
				return string();
			}

			auto checkpointIt = std::find_if(entryIt->Checkpoints.begin(), entryIt->Checkpoints.end(), [](const GenesisCompatibilityCheckpoint& checkpoint) {
				return checkpoint.Id == "GEN-COMPAT-FIRST-VISIBLE-FRAME";
			});
			if (checkpointIt == entryIt->Checkpoints.end()) {
				ADD_FAILURE() << "Missing GEN-COMPAT-FIRST-VISIBLE-FRAME checkpoint for " << romName;
				return string();
			}

			EXPECT_TRUE(checkpointIt->Pass);
			return checkpointIt->Context;
		};

		GenesisCompatibilityMatrixResult runA = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);
		GenesisCompatibilityMatrixResult runB = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);

		ASSERT_EQ((int)runA.Entries.size(), (int)corpus.size());
		ASSERT_EQ((int)runB.Entries.size(), (int)corpus.size());

		for (const GenesisCompatibilityRomCase& romCase : corpus) {
			string contextA = readFirstVisibleFrameContext(runA, romCase.Name);
			string contextB = readFirstVisibleFrameContext(runB, romCase.Name);
			EXPECT_EQ(contextA, contextB);
		}
	}

	TEST(GenesisCompatibilityHarnessTests, StartupEventSequenceCheckpointContextIsDeterministicAcrossRuns) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus = BuildGenesisCompatibilityCorpus();

		auto readStartupEventSequenceContext = [&](const GenesisCompatibilityMatrixResult& result, const string& romName) {
			auto entryIt = std::find_if(result.Entries.begin(), result.Entries.end(), [&](const GenesisCompatibilityEntry& entry) {
				return entry.Name == romName;
			});
			if (entryIt == result.Entries.end()) {
				ADD_FAILURE() << "Missing compatibility entry for " << romName;
				return string();
			}

			auto checkpointIt = std::find_if(entryIt->Checkpoints.begin(), entryIt->Checkpoints.end(), [](const GenesisCompatibilityCheckpoint& checkpoint) {
				return checkpoint.Id == "GEN-COMPAT-STARTUP-EVENT-SEQUENCE";
			});
			if (checkpointIt == entryIt->Checkpoints.end()) {
				ADD_FAILURE() << "Missing GEN-COMPAT-STARTUP-EVENT-SEQUENCE checkpoint for " << romName;
				return string();
			}

			EXPECT_TRUE(checkpointIt->Pass);
			return checkpointIt->Context;
		};

		GenesisCompatibilityMatrixResult runA = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);
		GenesisCompatibilityMatrixResult runB = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);

		ASSERT_EQ((int)runA.Entries.size(), (int)corpus.size());
		ASSERT_EQ((int)runB.Entries.size(), (int)corpus.size());

		for (const GenesisCompatibilityRomCase& romCase : corpus) {
			string contextA = readStartupEventSequenceContext(runA, romCase.Name);
			string contextB = readStartupEventSequenceContext(runB, romCase.Name);
			EXPECT_EQ(contextA, contextB);
		}
	}

	TEST(GenesisCompatibilityHarnessTests, StartupFrameWindowCheckpointContextIsDeterministicAcrossRuns) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus = BuildGenesisCompatibilityCorpus();

		auto readStartupFrameWindowContext = [&](const GenesisCompatibilityMatrixResult& result, const string& romName) {
			auto entryIt = std::find_if(result.Entries.begin(), result.Entries.end(), [&](const GenesisCompatibilityEntry& entry) {
				return entry.Name == romName;
			});
			if (entryIt == result.Entries.end()) {
				ADD_FAILURE() << "Missing compatibility entry for " << romName;
				return string();
			}

			auto checkpointIt = std::find_if(entryIt->Checkpoints.begin(), entryIt->Checkpoints.end(), [](const GenesisCompatibilityCheckpoint& checkpoint) {
				return checkpoint.Id == "GEN-COMPAT-STARTUP-FRAME-WINDOW";
			});
			if (checkpointIt == entryIt->Checkpoints.end()) {
				ADD_FAILURE() << "Missing GEN-COMPAT-STARTUP-FRAME-WINDOW checkpoint for " << romName;
				return string();
			}

			EXPECT_TRUE(checkpointIt->Pass);
			return checkpointIt->Context;
		};

		GenesisCompatibilityMatrixResult runA = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);
		GenesisCompatibilityMatrixResult runB = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);

		ASSERT_EQ((int)runA.Entries.size(), (int)corpus.size());
		ASSERT_EQ((int)runB.Entries.size(), (int)corpus.size());

		for (const GenesisCompatibilityRomCase& romCase : corpus) {
			string contextA = readStartupFrameWindowContext(runA, romCase.Name);
			string contextB = readStartupFrameWindowContext(runB, romCase.Name);
			EXPECT_EQ(contextA, contextB);
		}
	}

	TEST(GenesisCompatibilityHarnessTests, StartupRenderHandoffCheckpointContextIsDeterministicAcrossRuns) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus = BuildGenesisCompatibilityCorpus();

		auto readStartupRenderHandoffContext = [&](const GenesisCompatibilityMatrixResult& result, const string& romName) {
			auto entryIt = std::find_if(result.Entries.begin(), result.Entries.end(), [&](const GenesisCompatibilityEntry& entry) {
				return entry.Name == romName;
			});
			if (entryIt == result.Entries.end()) {
				ADD_FAILURE() << "Missing compatibility entry for " << romName;
				return string();
			}

			auto checkpointIt = std::find_if(entryIt->Checkpoints.begin(), entryIt->Checkpoints.end(), [](const GenesisCompatibilityCheckpoint& checkpoint) {
				return checkpoint.Id == "GEN-COMPAT-STARTUP-RENDER-HANDOFF";
			});
			if (checkpointIt == entryIt->Checkpoints.end()) {
				ADD_FAILURE() << "Missing GEN-COMPAT-STARTUP-RENDER-HANDOFF checkpoint for " << romName;
				return string();
			}

			EXPECT_TRUE(checkpointIt->Pass);
			return checkpointIt->Context;
		};

		GenesisCompatibilityMatrixResult runA = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);
		GenesisCompatibilityMatrixResult runB = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);

		ASSERT_EQ((int)runA.Entries.size(), (int)corpus.size());
		ASSERT_EQ((int)runB.Entries.size(), (int)corpus.size());

		for (const GenesisCompatibilityRomCase& romCase : corpus) {
			string contextA = readStartupRenderHandoffContext(runA, romCase.Name);
			string contextB = readStartupRenderHandoffContext(runB, romCase.Name);
			EXPECT_EQ(contextA, contextB);
		}
	}

	TEST(GenesisCompatibilityHarnessTests, StartupCpuProgressionCheckpointContextIsDeterministicAcrossRuns) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus = BuildGenesisCompatibilityCorpus();

		auto readStartupCpuProgressionContext = [&](const GenesisCompatibilityMatrixResult& result, const string& romName) {
			auto entryIt = std::find_if(result.Entries.begin(), result.Entries.end(), [&](const GenesisCompatibilityEntry& entry) {
				return entry.Name == romName;
			});
			if (entryIt == result.Entries.end()) {
				ADD_FAILURE() << "Missing compatibility entry for " << romName;
				return string();
			}

			auto checkpointIt = std::find_if(entryIt->Checkpoints.begin(), entryIt->Checkpoints.end(), [](const GenesisCompatibilityCheckpoint& checkpoint) {
				return checkpoint.Id == "GEN-COMPAT-STARTUP-CPU-PROGRESSION";
			});
			if (checkpointIt == entryIt->Checkpoints.end()) {
				ADD_FAILURE() << "Missing GEN-COMPAT-STARTUP-CPU-PROGRESSION checkpoint for " << romName;
				return string();
			}

			EXPECT_TRUE(checkpointIt->Pass);
			return checkpointIt->Context;
		};

		GenesisCompatibilityMatrixResult runA = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);
		GenesisCompatibilityMatrixResult runB = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);

		ASSERT_EQ((int)runA.Entries.size(), (int)corpus.size());
		ASSERT_EQ((int)runB.Entries.size(), (int)corpus.size());

		for (const GenesisCompatibilityRomCase& romCase : corpus) {
			string contextA = readStartupCpuProgressionContext(runA, romCase.Name);
			string contextB = readStartupCpuProgressionContext(runB, romCase.Name);
			EXPECT_EQ(contextA, contextB);
		}
	}

	TEST(GenesisCompatibilityHarnessTests, BootToTitleProgressionCheckpointContextIsDeterministicAcrossRuns) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus = BuildGenesisCompatibilityCorpus();

		auto readBootToTitleProgressionContext = [&](const GenesisCompatibilityMatrixResult& result, const string& romName) {
			auto entryIt = std::find_if(result.Entries.begin(), result.Entries.end(), [&](const GenesisCompatibilityEntry& entry) {
				return entry.Name == romName;
			});
			if (entryIt == result.Entries.end()) {
				ADD_FAILURE() << "Missing compatibility entry for " << romName;
				return string();
			}

			auto checkpointIt = std::find_if(entryIt->Checkpoints.begin(), entryIt->Checkpoints.end(), [](const GenesisCompatibilityCheckpoint& checkpoint) {
				return checkpoint.Id == "GEN-COMPAT-BOOT-TO-TITLE-PROGRESSION";
			});
			if (checkpointIt == entryIt->Checkpoints.end()) {
				ADD_FAILURE() << "Missing GEN-COMPAT-BOOT-TO-TITLE-PROGRESSION checkpoint for " << romName;
				return string();
			}

			EXPECT_TRUE(checkpointIt->Pass);
			return checkpointIt->Context;
		};

		GenesisCompatibilityMatrixResult runA = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);
		GenesisCompatibilityMatrixResult runB = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);

		ASSERT_EQ((int)runA.Entries.size(), (int)corpus.size());
		ASSERT_EQ((int)runB.Entries.size(), (int)corpus.size());

		for (const GenesisCompatibilityRomCase& romCase : corpus) {
			string contextA = readBootToTitleProgressionContext(runA, romCase.Name);
			string contextB = readBootToTitleProgressionContext(runB, romCase.Name);
			EXPECT_EQ(contextA, contextB);
		}
	}

	TEST(GenesisCompatibilityHarnessTests, InterruptCadenceStartupCheckpointContextIsDeterministicAcrossRuns) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus = BuildGenesisCompatibilityCorpus();

		auto readInterruptCadenceContext = [&](const GenesisCompatibilityMatrixResult& result, const string& romName) {
			auto entryIt = std::find_if(result.Entries.begin(), result.Entries.end(), [&](const GenesisCompatibilityEntry& entry) {
				return entry.Name == romName;
			});
			if (entryIt == result.Entries.end()) {
				ADD_FAILURE() << "Missing compatibility entry for " << romName;
				return string();
			}

			auto checkpointIt = std::find_if(entryIt->Checkpoints.begin(), entryIt->Checkpoints.end(), [](const GenesisCompatibilityCheckpoint& checkpoint) {
				return checkpoint.Id == "GEN-COMPAT-INTERRUPT-CADENCE-STARTUP";
			});
			if (checkpointIt == entryIt->Checkpoints.end()) {
				ADD_FAILURE() << "Missing GEN-COMPAT-INTERRUPT-CADENCE-STARTUP checkpoint for " << romName;
				return string();
			}

			EXPECT_TRUE(checkpointIt->Pass);
			return checkpointIt->Context;
		};

		GenesisCompatibilityMatrixResult runA = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);
		GenesisCompatibilityMatrixResult runB = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);

		ASSERT_EQ((int)runA.Entries.size(), (int)corpus.size());
		ASSERT_EQ((int)runB.Entries.size(), (int)corpus.size());

		for (const GenesisCompatibilityRomCase& romCase : corpus) {
			string contextA = readInterruptCadenceContext(runA, romCase.Name);
			string contextB = readInterruptCadenceContext(runB, romCase.Name);
			EXPECT_EQ(contextA, contextB);
		}
	}

	TEST(GenesisCompatibilityHarnessTests, InterruptIntervalStartupCheckpointContextIsDeterministicAcrossRuns) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus = BuildGenesisCompatibilityCorpus();

		auto readInterruptIntervalContext = [&](const GenesisCompatibilityMatrixResult& result, const string& romName) {
			auto entryIt = std::find_if(result.Entries.begin(), result.Entries.end(), [&](const GenesisCompatibilityEntry& entry) {
				return entry.Name == romName;
			});
			if (entryIt == result.Entries.end()) {
				ADD_FAILURE() << "Missing compatibility entry for " << romName;
				return string();
			}

			auto checkpointIt = std::find_if(entryIt->Checkpoints.begin(), entryIt->Checkpoints.end(), [](const GenesisCompatibilityCheckpoint& checkpoint) {
				return checkpoint.Id == "GEN-COMPAT-INTERRUPT-INTERVAL-STARTUP";
			});
			if (checkpointIt == entryIt->Checkpoints.end()) {
				ADD_FAILURE() << "Missing GEN-COMPAT-INTERRUPT-INTERVAL-STARTUP checkpoint for " << romName;
				return string();
			}

			EXPECT_TRUE(checkpointIt->Pass);
			return checkpointIt->Context;
		};

		GenesisCompatibilityMatrixResult runA = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);
		GenesisCompatibilityMatrixResult runB = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);

		ASSERT_EQ((int)runA.Entries.size(), (int)corpus.size());
		ASSERT_EQ((int)runB.Entries.size(), (int)corpus.size());

		for (const GenesisCompatibilityRomCase& romCase : corpus) {
			string contextA = readInterruptIntervalContext(runA, romCase.Name);
			string contextB = readInterruptIntervalContext(runB, romCase.Name);
			EXPECT_EQ(contextA, contextB);
		}
	}

	TEST(GenesisCompatibilityHarnessTests, VdpStatusStartupCheckpointContextIsDeterministicAcrossRuns) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus = BuildGenesisCompatibilityCorpus();

		auto readVdpStatusContext = [&](const GenesisCompatibilityMatrixResult& result, const string& romName) {
			auto entryIt = std::find_if(result.Entries.begin(), result.Entries.end(), [&](const GenesisCompatibilityEntry& entry) {
				return entry.Name == romName;
			});
			if (entryIt == result.Entries.end()) {
				ADD_FAILURE() << "Missing compatibility entry for " << romName;
				return string();
			}

			auto checkpointIt = std::find_if(entryIt->Checkpoints.begin(), entryIt->Checkpoints.end(), [](const GenesisCompatibilityCheckpoint& checkpoint) {
				return checkpoint.Id == "GEN-COMPAT-VDP-STATUS-STARTUP";
			});
			if (checkpointIt == entryIt->Checkpoints.end()) {
				ADD_FAILURE() << "Missing GEN-COMPAT-VDP-STATUS-STARTUP checkpoint for " << romName;
				return string();
			}

			EXPECT_TRUE(checkpointIt->Pass);
			return checkpointIt->Context;
		};

		GenesisCompatibilityMatrixResult runA = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);
		GenesisCompatibilityMatrixResult runB = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);

		ASSERT_EQ((int)runA.Entries.size(), (int)corpus.size());
		ASSERT_EQ((int)runB.Entries.size(), (int)corpus.size());

		for (const GenesisCompatibilityRomCase& romCase : corpus) {
			string contextA = readVdpStatusContext(runA, romCase.Name);
			string contextB = readVdpStatusContext(runB, romCase.Name);
			EXPECT_EQ(contextA, contextB);
		}
	}

	TEST(GenesisCompatibilityHarnessTests, PbcHostModeScenarioDigestIsDeterministicAcrossRuns) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus;
		corpus.push_back({"pbc-host-sms-boot.bin", vector<uint8_t>(0x8000, 0x4E)});
		corpus.push_back({"pbc-host-sms-runtime.bin", vector<uint8_t>(0x10000, 0x4E)});

		GenesisCompatibilityMatrixResult runA = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);
		GenesisCompatibilityMatrixResult runB = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);

		EXPECT_EQ(runA.PassCount, (int)corpus.size());
		EXPECT_EQ(runA.FailCount, 0);
		EXPECT_EQ(runA.Digest, runB.Digest);
		EXPECT_EQ(runA.OutputLines, runB.OutputLines);
	}

	TEST(GenesisCompatibilityHarnessTests, PbcHostModeScenarioIncludesDeterministicCheckpointHooks) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus;
		corpus.push_back({"pbc-host-sms-scenario.bin", vector<uint8_t>(0xC000, 0x4E)});

		GenesisCompatibilityMatrixResult result = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);

		ASSERT_EQ(result.PassCount, 1);
		ASSERT_EQ(result.FailCount, 0);
		ASSERT_EQ(result.Entries.size(), 1u);

		const GenesisCompatibilityEntry& entry = result.Entries[0];
		auto hasPassingCheckpoint = [&](const string& checkpointId) {
			return std::any_of(entry.Checkpoints.begin(), entry.Checkpoints.end(), [&](const GenesisCompatibilityCheckpoint& checkpoint) {
				return checkpoint.Id == checkpointId && checkpoint.Pass;
			});
		};

		EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-HOST-MODE"));
		EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-MAPPER-EDGE"));
		EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-DETERMINISM"));
		EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-SCD-TELEMETRY"));
		EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-32X-TELEMETRY"));
		EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-DEBUG-LANE"));
		EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-TAS-CHEAT"));
		EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-PBC-BOOT"));
		EXPECT_TRUE(hasPassingCheckpoint("GEN-COMPAT-PBC-RUNTIME"));
	}

	TEST(GenesisCompatibilityHarnessTests, SegaCdTelemetryCheckpointContextIsDeterministicAcrossRuns) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus;
		corpus.push_back({"scd-telemetry-parity.bin", vector<uint8_t>(0x10000, 0x4E)});

		auto readTelemetryContext = [&](const GenesisCompatibilityMatrixResult& result) {
			EXPECT_EQ(result.PassCount, 1);
			EXPECT_EQ(result.FailCount, 0);
			const GenesisCompatibilityEntry& entry = result.Entries[0];
			auto it = std::find_if(entry.Checkpoints.begin(), entry.Checkpoints.end(), [](const GenesisCompatibilityCheckpoint& checkpoint) {
				return checkpoint.Id == "GEN-COMPAT-SCD-TELEMETRY";
			});
			if (it == entry.Checkpoints.end()) {
				ADD_FAILURE() << "Missing GEN-COMPAT-SCD-TELEMETRY checkpoint";
				return string();
			}
			EXPECT_TRUE(it->Pass);
			return it->Context;
		};

		GenesisCompatibilityMatrixResult runA = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);
		GenesisCompatibilityMatrixResult runB = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);

		string contextA = readTelemetryContext(runA);
		string contextB = readTelemetryContext(runB);
		EXPECT_EQ(contextA, contextB);
	}

	TEST(GenesisCompatibilityHarnessTests, M32xTelemetryCheckpointContextIsDeterministicAcrossRuns) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus;
		corpus.push_back({"m32x-telemetry-parity.bin", vector<uint8_t>(0x10000, 0x4E)});

		auto readTelemetryContext = [&](const GenesisCompatibilityMatrixResult& result) {
			EXPECT_EQ(result.PassCount, 1);
			EXPECT_EQ(result.FailCount, 0);
			const GenesisCompatibilityEntry& entry = result.Entries[0];
			auto it = std::find_if(entry.Checkpoints.begin(), entry.Checkpoints.end(), [](const GenesisCompatibilityCheckpoint& checkpoint) {
				return checkpoint.Id == "GEN-COMPAT-32X-TELEMETRY";
			});
			if (it == entry.Checkpoints.end()) {
				ADD_FAILURE() << "Missing GEN-COMPAT-32X-TELEMETRY checkpoint";
				return string();
			}
			EXPECT_TRUE(it->Pass);
			return it->Context;
		};

		GenesisCompatibilityMatrixResult runA = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);
		GenesisCompatibilityMatrixResult runB = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);

		string contextA = readTelemetryContext(runA);
		string contextB = readTelemetryContext(runB);
		EXPECT_EQ(contextA, contextB);
	}

	TEST(GenesisCompatibilityHarnessTests, DebugLaneCheckpointContextIsDeterministicAcrossRuns) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus;
		corpus.push_back({"debug-lane-parity.bin", vector<uint8_t>(0x10000, 0x4E)});

		auto readDebugLaneContext = [&](const GenesisCompatibilityMatrixResult& result) {
			EXPECT_EQ(result.PassCount, 1);
			EXPECT_EQ(result.FailCount, 0);
			const GenesisCompatibilityEntry& entry = result.Entries[0];
			auto it = std::find_if(entry.Checkpoints.begin(), entry.Checkpoints.end(), [](const GenesisCompatibilityCheckpoint& checkpoint) {
				return checkpoint.Id == "GEN-COMPAT-DEBUG-LANE";
			});
			if (it == entry.Checkpoints.end()) {
				ADD_FAILURE() << "Missing GEN-COMPAT-DEBUG-LANE checkpoint";
				return string();
			}
			EXPECT_TRUE(it->Pass);
			return it->Context;
		};

		GenesisCompatibilityMatrixResult runA = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);
		GenesisCompatibilityMatrixResult runB = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);

		string contextA = readDebugLaneContext(runA);
		string contextB = readDebugLaneContext(runB);
		EXPECT_EQ(contextA, contextB);
	}

	TEST(GenesisCompatibilityHarnessTests, TasCheatCheckpointContextIsDeterministicAcrossRuns) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus;
		corpus.push_back({"tas-cheat-parity.bin", vector<uint8_t>(0x10000, 0x4E)});

		auto readTasCheatContext = [&](const GenesisCompatibilityMatrixResult& result) {
			EXPECT_EQ(result.PassCount, 1);
			EXPECT_EQ(result.FailCount, 0);
			const GenesisCompatibilityEntry& entry = result.Entries[0];
			auto it = std::find_if(entry.Checkpoints.begin(), entry.Checkpoints.end(), [](const GenesisCompatibilityCheckpoint& checkpoint) {
				return checkpoint.Id == "GEN-COMPAT-TAS-CHEAT";
			});
			if (it == entry.Checkpoints.end()) {
				ADD_FAILURE() << "Missing GEN-COMPAT-TAS-CHEAT checkpoint";
				return string();
			}
			EXPECT_TRUE(it->Pass);
			return it->Context;
		};

		GenesisCompatibilityMatrixResult runA = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);
		GenesisCompatibilityMatrixResult runB = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);

		string contextA = readTasCheatContext(runA);
		string contextB = readTasCheatContext(runB);
		EXPECT_EQ(contextA, contextB);
	}

	TEST(GenesisCompatibilityHarnessTests, PbcRuntimeParityCheckpointContextIsDeterministicAcrossRuns) {
		GenesisM68kBoundaryScaffold scaffold;
		vector<GenesisCompatibilityRomCase> corpus;
		corpus.push_back({"pbc-runtime-parity.bin", vector<uint8_t>(0x10000, 0x4E)});

		auto readRuntimeContext = [&](const GenesisCompatibilityMatrixResult& result) {
			EXPECT_EQ(result.PassCount, 1);
			EXPECT_EQ(result.FailCount, 0);
			const GenesisCompatibilityEntry& entry = result.Entries[0];
			auto it = std::find_if(entry.Checkpoints.begin(), entry.Checkpoints.end(), [](const GenesisCompatibilityCheckpoint& checkpoint) {
				return checkpoint.Id == "GEN-COMPAT-PBC-RUNTIME";
			});
			if (it == entry.Checkpoints.end()) {
				ADD_FAILURE() << "Missing GEN-COMPAT-PBC-RUNTIME checkpoint";
				return string();
			}
			EXPECT_TRUE(it->Pass);
			return it->Context;
		};

		GenesisCompatibilityMatrixResult runA = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);
		GenesisCompatibilityMatrixResult runB = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);

		string contextA = readRuntimeContext(runA);
		string contextB = readRuntimeContext(runB);
		EXPECT_EQ(contextA, contextB);
	}
}
