#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"

namespace {
	// Z80 bus control ports: 0xA11100 = bus request, 0xA11200 = reset.
	// Write 0x01 to 0xA11200 → bootstrap (release reset).
	// Write 0x00 to 0xA11200 → assert reset (hold Z80 in reset).
	// Write 0x01 to 0xA11100 → request bus (halt Z80, M68K gains Z80 bus).
	// Write 0x00 to 0xA11100 → release bus (Z80 resumes).

	TEST(GenesisZ80BusOverlapStressTests, RapidRequestReleaseToggle) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		auto& bus = scaffold.GetBus();

		// Bootstrap Z80.
		bus.WriteByte(0xA11200, 0x01);
		ASSERT_TRUE(bus.IsZ80Running());

		// Rapid request/release 32 times.
		for (uint32_t i = 0; i < 32; i++) {
			bus.WriteByte(0xA11100, 0x01);
			EXPECT_FALSE(bus.IsZ80Running());
			bus.WriteByte(0xA11100, 0x00);
			EXPECT_TRUE(bus.IsZ80Running());
		}

		EXPECT_GE(bus.GetZ80HandoffCount(), 32u);
	}

	TEST(GenesisZ80BusOverlapStressTests, ResetDuringBusRequestKeepsConsistentState) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		auto& bus = scaffold.GetBus();

		bus.WriteByte(0xA11200, 0x01);
		scaffold.StepFrameScaffold(40);
		uint64_t initialCycles = bus.GetZ80ExecutedCycles();
		EXPECT_GT(initialCycles, 0u);

		// Request bus (halt Z80).
		bus.WriteByte(0xA11100, 0x01);
		EXPECT_FALSE(bus.IsZ80Running());

		// Assert reset while bus is requested.
		bus.WriteByte(0xA11200, 0x00);
		EXPECT_FALSE(bus.IsZ80Running());

		// Step — Z80 should not run.
		scaffold.StepFrameScaffold(40);
		EXPECT_EQ(bus.GetZ80ExecutedCycles(), initialCycles);

		// Release reset → re-bootstrap.
		bus.WriteByte(0xA11200, 0x01);

		// Release bus → Z80 should run again.
		bus.WriteByte(0xA11100, 0x00);
		scaffold.StepFrameScaffold(40);
		EXPECT_GT(bus.GetZ80ExecutedCycles(), initialCycles);
		EXPECT_TRUE(bus.IsZ80Running());
	}

	TEST(GenesisZ80BusOverlapStressTests, DoubleResetWithBusRequestInterleavedKeepsCounts) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		auto& bus = scaffold.GetBus();

		// Bootstrap.
		bus.WriteByte(0xA11200, 0x01);
		EXPECT_EQ(bus.GetZ80BootstrapCount(), 1u);

		// Sequence: reset → bus-req → release-reset → release-bus → reset → release-reset.
		bus.WriteByte(0xA11200, 0x00); // reset
		EXPECT_FALSE(bus.IsZ80Running());
		bus.WriteByte(0xA11100, 0x01); // bus request
		bus.WriteByte(0xA11200, 0x01); // release reset
		bus.WriteByte(0xA11100, 0x00); // release bus

		// Z80 should be bootstrapped and running after full sequence.
		EXPECT_TRUE(bus.IsZ80Bootstrapped());

		bus.WriteByte(0xA11200, 0x00); // reset again
		EXPECT_FALSE(bus.IsZ80Running());
		bus.WriteByte(0xA11200, 0x01); // release reset

		EXPECT_TRUE(bus.IsZ80Bootstrapped());
	}

	TEST(GenesisZ80BusOverlapStressTests, BusRequestWithoutBootstrapDoesNotCrash) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		auto& bus = scaffold.GetBus();

		// Don't bootstrap — request bus anyway.
		bus.WriteByte(0xA11100, 0x01);
		scaffold.StepFrameScaffold(40);

		// Z80 never ran, so cycles should be 0.
		EXPECT_EQ(bus.GetZ80ExecutedCycles(), 0u);

		bus.WriteByte(0xA11100, 0x00);
		scaffold.StepFrameScaffold(40);
		EXPECT_EQ(bus.GetZ80ExecutedCycles(), 0u);
	}

	TEST(GenesisZ80BusOverlapStressTests, AlternatingResetAndBusRequestPattern) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		auto& bus = scaffold.GetBus();

		bus.WriteByte(0xA11200, 0x01);

		// Alternate: reset → bus-req → release-reset → release-bus, 8 iterations.
		for (uint32_t i = 0; i < 8; i++) {
			bus.WriteByte(0xA11200, 0x00);  // reset
			bus.WriteByte(0xA11100, 0x01);  // bus request
			scaffold.StepFrameScaffold(20);
			bus.WriteByte(0xA11200, 0x01);  // release reset
			bus.WriteByte(0xA11100, 0x00);  // release bus
			scaffold.StepFrameScaffold(20);
		}

		// Z80 should be running and bootstrapped after the sequence.
		EXPECT_TRUE(bus.IsZ80Running());
		EXPECT_TRUE(bus.IsZ80Bootstrapped());
	}

	TEST(GenesisZ80BusOverlapStressTests, CycleAccountingDuringOverlapStress) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		auto& bus = scaffold.GetBus();

		bus.WriteByte(0xA11200, 0x01);
		scaffold.StepFrameScaffold(100);
		uint64_t cyclesBeforeStress = bus.GetZ80ExecutedCycles();
		EXPECT_GT(cyclesBeforeStress, 0u);

		// Stress: request bus, step (Z80 halted), release, step (Z80 runs).
		for (uint32_t i = 0; i < 16; i++) {
			bus.WriteByte(0xA11100, 0x01);
			scaffold.StepFrameScaffold(20);
			uint64_t haltedCycles = bus.GetZ80ExecutedCycles();

			bus.WriteByte(0xA11100, 0x00);
			scaffold.StepFrameScaffold(20);
			uint64_t resumedCycles = bus.GetZ80ExecutedCycles();

			// Cycles should increase only when Z80 is running.
			EXPECT_GT(resumedCycles, haltedCycles) << "iteration=" << i;
		}

		EXPECT_GT(bus.GetZ80ExecutedCycles(), cyclesBeforeStress);
	}

	TEST(GenesisZ80BusOverlapStressTests, DmaOverlapWithBusRequestDoesNotCorruptCounters) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		auto& bus = scaffold.GetBus();

		bus.WriteByte(0xA11200, 0x01);
		bus.BeginDmaTransfer(GenesisVdpDmaMode::Copy, 32);

		// Overlap: bus request during active DMA.
		scaffold.StepFrameScaffold(40);
		bus.WriteByte(0xA11100, 0x01);
		scaffold.StepFrameScaffold(40);

		uint32_t contentionCycles = bus.GetDmaContentionCycles();
		uint32_t contentionEvents = bus.GetDmaContentionEvents();

		// Release bus and continue DMA.
		bus.WriteByte(0xA11100, 0x00);
		scaffold.StepFrameScaffold(80);

		EXPECT_GE(bus.GetDmaContentionCycles(), contentionCycles);
		EXPECT_GE(bus.GetDmaContentionEvents(), contentionEvents);
		EXPECT_TRUE(bus.IsZ80Running());
	}

	TEST(GenesisZ80BusOverlapStressTests, OverlapStressFuzzDigestIsDeterministic) {
		auto runScenario = []() {
			GenesisM68kBoundaryScaffold scaffold;
			scaffold.Startup();
			auto& bus = scaffold.GetBus();

			bus.WriteByte(0xA11200, 0x01);

			uint32_t seed = 0xdeadbeef;
			uint64_t hash = 1469598103934665603ull;

			for (uint32_t i = 0; i < 64; i++) {
				seed = seed * 1664525u + 1013904223u;

				// Pseudo-random operation selection.
				uint8_t op = (uint8_t)((seed >> 24) & 0x03);
				switch (op) {
					case 0: bus.WriteByte(0xA11100, 0x01); break; // bus request
					case 1: bus.WriteByte(0xA11100, 0x00); break; // bus release
					case 2: bus.WriteByte(0xA11200, 0x00); break; // assert reset
					case 3: bus.WriteByte(0xA11200, 0x01); break; // release reset
				}

				scaffold.StepFrameScaffold(20 + (seed & 0x1f));

				// Hash the state after each step.
				string line = std::format("{}:{}:{}:{}:{}:{}",
					i, op,
					bus.GetZ80ExecutedCycles(),
					bus.GetZ80HandoffCount(),
					bus.IsZ80Running() ? 1 : 0,
					bus.GetZ80BootstrapCount());

				for (uint8_t ch : line) {
					hash ^= ch;
					hash *= 1099511628211ull;
				}
			}

			return std::make_tuple(
				hash,
				bus.GetZ80ExecutedCycles(),
				bus.GetZ80HandoffCount(),
				bus.GetZ80BootstrapCount(),
				bus.IsZ80Running()
			);
		};

		auto runA = runScenario();
		auto runB = runScenario();

		EXPECT_EQ(std::get<0>(runA), std::get<0>(runB));
		EXPECT_EQ(std::get<1>(runA), std::get<1>(runB));
		EXPECT_EQ(std::get<2>(runA), std::get<2>(runB));
		EXPECT_EQ(std::get<3>(runA), std::get<3>(runB));
		EXPECT_EQ(std::get<4>(runA), std::get<4>(runB));
	}

	TEST(GenesisZ80BusOverlapStressTests, SaveStateReplayFromMidOverlapSequence) {
		GenesisM68kBoundaryScaffold runA;
		runA.Startup();
		auto& busA = runA.GetBus();

		busA.WriteByte(0xA11200, 0x01);
		runA.StepFrameScaffold(60);
		busA.WriteByte(0xA11100, 0x01); // request bus
		runA.StepFrameScaffold(20);

		// Save state mid-overlap (bus requested, Z80 halted).
		GenesisBoundaryScaffoldSaveState checkpoint = runA.SaveState();

		busA.WriteByte(0xA11200, 0x00); // assert reset
		busA.WriteByte(0xA11200, 0x01); // release reset
		busA.WriteByte(0xA11100, 0x00); // release bus
		runA.StepFrameScaffold(100);

		// Replay from checkpoint.
		GenesisM68kBoundaryScaffold runB;
		runB.LoadState(checkpoint);
		auto& busB = runB.GetBus();

		busB.WriteByte(0xA11200, 0x00);
		busB.WriteByte(0xA11200, 0x01);
		busB.WriteByte(0xA11100, 0x00);
		runB.StepFrameScaffold(100);

		EXPECT_EQ(busA.GetZ80ExecutedCycles(), busB.GetZ80ExecutedCycles());
		EXPECT_EQ(busA.GetZ80HandoffCount(), busB.GetZ80HandoffCount());
		EXPECT_EQ(busA.GetZ80BootstrapCount(), busB.GetZ80BootstrapCount());
		EXPECT_EQ(busA.IsZ80Running(), busB.IsZ80Running());
	}

	TEST(GenesisZ80BusOverlapStressTests, RedundantBusRequestsDoNotDoubleCount) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		auto& bus = scaffold.GetBus();

		bus.WriteByte(0xA11200, 0x01);
		scaffold.StepFrameScaffold(40);

		// Request bus multiple times without releasing.
		bus.WriteByte(0xA11100, 0x01);
		uint32_t handoffsAfterFirst = bus.GetZ80HandoffCount();
		bus.WriteByte(0xA11100, 0x01);
		bus.WriteByte(0xA11100, 0x01);

		// Handoff count should not increase for redundant requests.
		EXPECT_EQ(bus.GetZ80HandoffCount(), handoffsAfterFirst);
		EXPECT_FALSE(bus.IsZ80Running());
	}

	TEST(GenesisZ80BusOverlapStressTests, RedundantBusReleasesDoNotDoubleCount) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		auto& bus = scaffold.GetBus();

		bus.WriteByte(0xA11200, 0x01);
		scaffold.StepFrameScaffold(40);

		bus.WriteByte(0xA11100, 0x01);
		bus.WriteByte(0xA11100, 0x00);
		uint32_t handoffsAfterRelease = bus.GetZ80HandoffCount();

		// Redundant releases while already running.
		bus.WriteByte(0xA11100, 0x00);
		bus.WriteByte(0xA11100, 0x00);

		EXPECT_EQ(bus.GetZ80HandoffCount(), handoffsAfterRelease);
		EXPECT_TRUE(bus.IsZ80Running());
	}
}
