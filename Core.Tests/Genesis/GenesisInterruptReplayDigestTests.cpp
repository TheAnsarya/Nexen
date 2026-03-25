#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"

namespace {
	static constexpr uint32_t CyclesPerScanline = 488;
	static constexpr uint32_t ScanlinesPerFrame = 262;

	void LoadNopRom(GenesisM68kBoundaryScaffold& scaffold) {
		vector<uint8_t> rom(0x4000, 0);
		for (size_t i = 0; i + 1 < rom.size(); i += 2) {
			rom[i] = 0x4E;
			rom[i + 1] = 0x71;
		}
		scaffold.LoadRom(rom);
	}

	struct InterruptReplayDigest {
		uint32_t HInterruptCount;
		uint32_t VInterruptCount;
		uint32_t TimingFrame;
		uint32_t TimingScanline;
		vector<string> TimingEvents;
		string MixedDigest;
		string YmDigest;
		string PsgDigest;

		bool operator==(const InterruptReplayDigest&) const = default;
	};

	InterruptReplayDigest CaptureReplayDigest(const GenesisM68kBoundaryScaffold& scaffold) {
		const auto& bus = const_cast<GenesisM68kBoundaryScaffold&>(scaffold).GetBus();
		return InterruptReplayDigest{
			scaffold.GetHorizontalInterruptCount(),
			scaffold.GetVerticalInterruptCount(),
			scaffold.GetTimingFrame(),
			scaffold.GetTimingScanline(),
			scaffold.GetTimingEvents(),
			bus.GetMixedDigest(),
			bus.GetYmDigest(),
			bus.GetPsgDigest()
		};
	}

	void RunMixedScheduleTail(GenesisM68kBoundaryScaffold& scaffold, uint32_t frames) {
		auto& bus = scaffold.GetBus();
		for (uint32_t f = 0; f < frames; f++) {
			scaffold.StepFrameScaffold(CyclesPerScanline * ScanlinesPerFrame);
			bus.StepYm2612(144 * 10);
			bus.StepSn76489(128 * 10);
			bus.UpdateMixedSample();
		}
	}

	// ----- Tests -----

	TEST(GenesisInterruptReplayDigestTests, HIntOnlyScheduleDigestIsDeterministic) {
		auto run = []() -> InterruptReplayDigest {
			GenesisM68kBoundaryScaffold scaffold;
			LoadNopRom(scaffold);
			scaffold.Startup();
			scaffold.ConfigureInterruptSchedule(true, 16, false);
			scaffold.ClearTimingEvents();

			auto& bus = scaffold.GetBus();
			bus.WriteByte(0xA11200, 0x01);
			bus.YmWriteAddress(0, 0x22);
			bus.YmWriteData(0, 0x14);
			bus.PsgWrite(0x90);

			RunMixedScheduleTail(scaffold, 4);
			return CaptureReplayDigest(scaffold);
		};

		InterruptReplayDigest first = run();
		InterruptReplayDigest second = run();
		EXPECT_EQ(first, second);
		EXPECT_GT(first.HInterruptCount, 0u);
		EXPECT_EQ(first.VInterruptCount, 0u);
	}

	TEST(GenesisInterruptReplayDigestTests, VIntOnlyScheduleDigestIsDeterministic) {
		auto run = []() -> InterruptReplayDigest {
			GenesisM68kBoundaryScaffold scaffold;
			LoadNopRom(scaffold);
			scaffold.Startup();
			scaffold.ConfigureInterruptSchedule(false, 16, true);
			scaffold.ClearTimingEvents();

			auto& bus = scaffold.GetBus();
			bus.WriteByte(0xA11200, 0x01);
			bus.PsgWrite(0xA0);
			bus.PsgWrite(0x15);

			RunMixedScheduleTail(scaffold, 4);
			return CaptureReplayDigest(scaffold);
		};

		InterruptReplayDigest first = run();
		InterruptReplayDigest second = run();
		EXPECT_EQ(first, second);
		EXPECT_EQ(first.HInterruptCount, 0u);
		EXPECT_GT(first.VInterruptCount, 0u);
	}

	TEST(GenesisInterruptReplayDigestTests, MixedHIntVIntScheduleDigestIsDeterministic) {
		auto run = []() -> InterruptReplayDigest {
			GenesisM68kBoundaryScaffold scaffold;
			LoadNopRom(scaffold);
			scaffold.Startup();
			scaffold.ConfigureInterruptSchedule(true, 8, true);
			scaffold.ClearTimingEvents();

			auto& bus = scaffold.GetBus();
			bus.WriteByte(0xA11200, 0x01);
			bus.YmWriteAddress(0, 0x30);
			bus.YmWriteData(0, 0x1A);
			bus.PsgWrite(0x90);
			bus.PsgWrite(0x08);

			RunMixedScheduleTail(scaffold, 4);
			return CaptureReplayDigest(scaffold);
		};

		InterruptReplayDigest first = run();
		InterruptReplayDigest second = run();
		EXPECT_EQ(first, second);
		EXPECT_GT(first.HInterruptCount, 0u);
		EXPECT_GT(first.VInterruptCount, 0u);
	}

	TEST(GenesisInterruptReplayDigestTests, DifferentHIntIntervalsProduceDifferentDigests) {
		auto run = [](uint32_t interval) -> InterruptReplayDigest {
			GenesisM68kBoundaryScaffold scaffold;
			LoadNopRom(scaffold);
			scaffold.Startup();
			scaffold.ConfigureInterruptSchedule(true, interval, true);
			scaffold.ClearTimingEvents();

			auto& bus = scaffold.GetBus();
			bus.WriteByte(0xA11200, 0x01);
			bus.YmWriteAddress(0, 0x22);
			bus.YmWriteData(0, 0x14);

			RunMixedScheduleTail(scaffold, 3);
			return CaptureReplayDigest(scaffold);
		};

		InterruptReplayDigest interval4 = run(4);
		InterruptReplayDigest interval32 = run(32);

		EXPECT_NE(interval4.HInterruptCount, interval32.HInterruptCount);
		EXPECT_NE(interval4.TimingEvents, interval32.TimingEvents);
	}

	TEST(GenesisInterruptReplayDigestTests, SaveStateReplayMatchesBaselineForMixedSchedule) {
		GenesisM68kBoundaryScaffold scaffold;
		LoadNopRom(scaffold);
		scaffold.Startup();
		scaffold.ConfigureInterruptSchedule(true, 12, true);
		scaffold.ClearTimingEvents();

		auto& bus = scaffold.GetBus();
		bus.WriteByte(0xA11200, 0x01);
		bus.YmWriteAddress(0, 0x30);
		bus.YmWriteData(0, 0x14);
		bus.PsgWrite(0x90);

		// Run to a checkpoint
		RunMixedScheduleTail(scaffold, 2);
		GenesisBoundaryScaffoldSaveState checkpoint = scaffold.SaveState();

		// Baseline continuation
		RunMixedScheduleTail(scaffold, 3);
		InterruptReplayDigest baseline = CaptureReplayDigest(scaffold);

		// Replay from checkpoint
		scaffold.LoadState(checkpoint);
		RunMixedScheduleTail(scaffold, 3);
		InterruptReplayDigest replay = CaptureReplayDigest(scaffold);

		EXPECT_EQ(replay, baseline);
	}

	TEST(GenesisInterruptReplayDigestTests, MultiFrameInterruptCountsAccumulateCorrectly) {
		GenesisM68kBoundaryScaffold scaffold;
		LoadNopRom(scaffold);
		scaffold.Startup();
		scaffold.ConfigureInterruptSchedule(true, 8, true);
		scaffold.ClearTimingEvents();

		RunMixedScheduleTail(scaffold, 1);
		uint32_t hAfter1 = scaffold.GetHorizontalInterruptCount();
		uint32_t vAfter1 = scaffold.GetVerticalInterruptCount();

		RunMixedScheduleTail(scaffold, 3);
		uint32_t hAfter4 = scaffold.GetHorizontalInterruptCount();
		uint32_t vAfter4 = scaffold.GetVerticalInterruptCount();

		// H-INT counts should grow proportionally (interval 8 means ~32 per frame)
		EXPECT_GT(hAfter4, hAfter1);
		// V-INT fires once per frame
		EXPECT_EQ(vAfter4, vAfter1 + 3u);
	}

	TEST(GenesisInterruptReplayDigestTests, HighFrequencyHIntWithDmaProducesStableDigest) {
		auto run = []() -> InterruptReplayDigest {
			GenesisM68kBoundaryScaffold scaffold;
			LoadNopRom(scaffold);
			scaffold.Startup();
			scaffold.ConfigureInterruptSchedule(true, 1, true);
			scaffold.ClearTimingEvents();

			auto& bus = scaffold.GetBus();
			bus.WriteByte(0xA11200, 0x01);
			bus.BeginDmaTransfer(GenesisVdpDmaMode::Copy, 30);

			for (uint32_t i = 0; i < 3; i++) {
				scaffold.StepFrameScaffold(CyclesPerScanline * ScanlinesPerFrame);
				bus.StepYm2612(144 * 5);
				bus.StepSn76489(128 * 5);
				bus.UpdateMixedSample();
				if (i == 1) {
					bus.BeginDmaTransfer(GenesisVdpDmaMode::Fill, 15);
				}
			}
			return CaptureReplayDigest(scaffold);
		};

		InterruptReplayDigest first = run();
		InterruptReplayDigest second = run();
		EXPECT_EQ(first, second);
		EXPECT_GT(first.HInterruptCount, 100u); // 1 per scanline × 262 × 3 frames
	}

	TEST(GenesisInterruptReplayDigestTests, InterruptScheduleChangeMidRunProducesDeterministicDigest) {
		auto run = []() -> InterruptReplayDigest {
			GenesisM68kBoundaryScaffold scaffold;
			LoadNopRom(scaffold);
			scaffold.Startup();
			scaffold.ConfigureInterruptSchedule(true, 32, true);
			scaffold.ClearTimingEvents();

			auto& bus = scaffold.GetBus();
			bus.WriteByte(0xA11200, 0x01);
			bus.YmWriteAddress(0, 0x22);
			bus.YmWriteData(0, 0x14);

			// Run 2 frames with interval=32
			RunMixedScheduleTail(scaffold, 2);

			// Change interval mid-run
			scaffold.ConfigureInterruptSchedule(true, 4, true);

			// Run 2 more frames with interval=4
			RunMixedScheduleTail(scaffold, 2);
			return CaptureReplayDigest(scaffold);
		};

		InterruptReplayDigest first = run();
		InterruptReplayDigest second = run();
		EXPECT_EQ(first, second);
	}

	TEST(GenesisInterruptReplayDigestTests, DisablingInterruptsMidRunStopsAccumulation) {
		GenesisM68kBoundaryScaffold scaffold;
		LoadNopRom(scaffold);
		scaffold.Startup();
		scaffold.ConfigureInterruptSchedule(true, 8, true);
		scaffold.ClearTimingEvents();

		RunMixedScheduleTail(scaffold, 2);
		uint32_t hBefore = scaffold.GetHorizontalInterruptCount();
		uint32_t vBefore = scaffold.GetVerticalInterruptCount();

		// Disable both interrupts
		scaffold.ConfigureInterruptSchedule(false, 8, false);

		RunMixedScheduleTail(scaffold, 2);
		uint32_t hAfter = scaffold.GetHorizontalInterruptCount();
		uint32_t vAfter = scaffold.GetVerticalInterruptCount();

		// No new interrupts should have fired
		EXPECT_EQ(hAfter, hBefore);
		EXPECT_EQ(vAfter, vBefore);
	}

	TEST(GenesisInterruptReplayDigestTests, FuzzedScheduleSequenceProducesDeterministicDigest) {
		auto run = [](uint32_t seed) -> InterruptReplayDigest {
			GenesisM68kBoundaryScaffold scaffold;
			LoadNopRom(scaffold);
			scaffold.Startup();

			auto& bus = scaffold.GetBus();
			bus.WriteByte(0xA11200, 0x01);

			uint64_t hash = 14695981039346656037ull;
			uint64_t prime = 1099511628211ull;
			hash = (hash ^ seed) * prime;

			for (uint32_t i = 0; i < 48; i++) {
				hash = (hash ^ i) * prime;
				uint32_t interval = 1 + (uint32_t)(hash >> 4 & 0x3F);
				bool hEnabled = (hash >> 10 & 1) != 0;
				bool vEnabled = (hash >> 11 & 1) != 0;
				scaffold.ConfigureInterruptSchedule(hEnabled, interval, vEnabled);
				scaffold.ClearTimingEvents();

				// Step a varied number of scanlines
				uint32_t scanlines = 50 + (uint32_t)(hash >> 12 & 0xFF);
				scaffold.StepFrameScaffold(CyclesPerScanline * scanlines);

				if ((i % 5) == 0) {
					bus.YmWriteAddress(0, (uint8_t)(0x30 + (hash >> 20 & 0x0F)));
					bus.YmWriteData(0, (uint8_t)(hash >> 28 & 0xFF));
					bus.StepYm2612(144);
				}
				if ((i % 7) == 0) {
					bus.PsgWrite((uint8_t)(0x80 | (hash >> 16 & 0x7F)));
					bus.StepSn76489(128);
				}
				bus.UpdateMixedSample();
			}
			return CaptureReplayDigest(scaffold);
		};

		InterruptReplayDigest first = run(42);
		InterruptReplayDigest second = run(42);
		EXPECT_EQ(first, second);

		InterruptReplayDigest other = run(99);
		EXPECT_NE(first.HInterruptCount, other.HInterruptCount);
	}

	TEST(GenesisInterruptReplayDigestTests, SaveStateReplayAcrossMidScheduleChangeMatchesBaseline) {
		GenesisM68kBoundaryScaffold scaffold;
		LoadNopRom(scaffold);
		scaffold.Startup();
		scaffold.ConfigureInterruptSchedule(true, 16, true);
		scaffold.ClearTimingEvents();

		auto& bus = scaffold.GetBus();
		bus.WriteByte(0xA11200, 0x01);
		bus.YmWriteAddress(0, 0x30);
		bus.YmWriteData(0, 0x14);

		RunMixedScheduleTail(scaffold, 2);

		// Save at the boundary between schedule configs
		GenesisBoundaryScaffoldSaveState checkpoint = scaffold.SaveState();

		// Baseline: change schedule, continue
		scaffold.ConfigureInterruptSchedule(true, 4, true);
		RunMixedScheduleTail(scaffold, 3);
		InterruptReplayDigest baseline = CaptureReplayDigest(scaffold);

		// Replay from checkpoint
		scaffold.LoadState(checkpoint);
		scaffold.ConfigureInterruptSchedule(true, 4, true);
		RunMixedScheduleTail(scaffold, 3);
		InterruptReplayDigest replay = CaptureReplayDigest(scaffold);

		EXPECT_EQ(replay, baseline);
	}
}
