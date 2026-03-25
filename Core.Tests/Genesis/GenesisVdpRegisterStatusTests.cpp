#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"

namespace {
	TEST(GenesisVdpRegisterStatusTests, ControlWordUpdatesRegisterFileAndStatusSemantics) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		scaffold.GetBus().WriteByte(0xC00004, 0x81);
		scaffold.GetBus().WriteByte(0xC00005, 0x40);
		EXPECT_EQ(scaffold.GetBus().GetVdpControlWordLatch(), 0x8140u);
		EXPECT_EQ(scaffold.GetBus().GetVdpRegister(1), 0x40u);
		EXPECT_TRUE((scaffold.GetBus().GetVdpStatus() & 0x0008u) != 0);

		scaffold.GetBus().WriteByte(0xC00004, 0x81);
		scaffold.GetBus().WriteByte(0xC00005, 0x00);
		EXPECT_EQ(scaffold.GetBus().GetVdpRegister(1), 0x00u);
		EXPECT_TRUE((scaffold.GetBus().GetVdpStatus() & 0x0008u) == 0);
	}

	TEST(GenesisVdpRegisterStatusTests, StatusReadClearsStickyPendingBitOnHighByteRead) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		scaffold.GetBus().WriteByte(0xC00004, 0x40);
		scaffold.GetBus().WriteByte(0xC00005, 0x00);

		uint8_t statusFirst = scaffold.GetBus().ReadByte(0xC00004);
		uint8_t statusSecond = scaffold.GetBus().ReadByte(0xC00004);

		EXPECT_TRUE((statusFirst & 0x80u) != 0);
		EXPECT_TRUE((statusSecond & 0x80u) == 0);
	}

	TEST(GenesisVdpRegisterStatusTests, ControlWordLatchPermutationsPreserveDeterministicStatusSideEffects) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		vector<uint16_t> controlWords = {
			0x8140, // reg1: set display/status bit semantics
			0x4000, // command class side effects
			0x8100, // reg1: clear display/status bit semantics
			0x8f02, // reg15 write
			0x4abc, // command class side effects
			0x97c0, // reg23 DMA mode bits
			0x8120, // reg1: DMA enable bit path
			0x40ff, // command class side effects
		};

		for (size_t i = 0; i < controlWords.size(); i++) {
			uint16_t word = controlWords[i];
			uint8_t hi = (uint8_t)((word >> 8) & 0xff);
			uint8_t lo = (uint8_t)(word & 0xff);

			scaffold.GetBus().WriteByte(0xC00004, hi);
			scaffold.GetBus().WriteByte(0xC00005, lo);

			SCOPED_TRACE(std::format("step={} word={:04x}", i, word));
			EXPECT_EQ(scaffold.GetBus().GetVdpControlWordLatch(), word);

			if ((word & 0xc000u) == 0x8000u && ((word >> 8) & 0x1fu) == 1u) {
				bool expectedSet = (word & 0x40u) != 0;
				bool actualSet = (scaffold.GetBus().GetVdpStatus() & 0x0008u) != 0;
				EXPECT_EQ(actualSet, expectedSet);
			}

			if ((word & 0xc000u) == 0x4000u) {
				uint16_t status = scaffold.GetBus().GetVdpStatus();
				EXPECT_TRUE((status & 0x8000u) != 0);
				EXPECT_TRUE((status & 0x0001u) == 0);
				EXPECT_TRUE((status & 0x0002u) != 0);
			}
		}
	}

	TEST(GenesisVdpRegisterStatusTests, ControlWordPermutationFuzzDigestIsDeterministicAcrossRuns) {
		auto runScenario = []() {
			GenesisM68kBoundaryScaffold scaffold;
			scaffold.Startup();

			uint32_t seed = 0x13579bdfu;
			uint64_t hash = 1469598103934665603ull;
			for (uint32_t i = 0; i < 96; i++) {
				seed = seed * 1664525u + 1013904223u;
				uint16_t word;
				if ((i % 5u) == 0u) {
					word = (uint16_t)(0x4000u | (seed & 0x3fffu));
				} else {
					uint8_t reg = (uint8_t)((seed >> 27) & 0x1fu);
					uint8_t value = (uint8_t)((seed >> 8) & 0xffu);
					word = (uint16_t)(0x8000u | ((uint16_t)reg << 8) | value);
				}

				scaffold.GetBus().WriteByte(0xC00004, (uint8_t)(word >> 8));
				scaffold.GetBus().WriteByte(0xC00005, (uint8_t)word);

				uint16_t latch = scaffold.GetBus().GetVdpControlWordLatch();
				uint16_t status = scaffold.GetBus().GetVdpStatus();
				uint8_t reg1 = scaffold.GetBus().GetVdpRegister(1);

				EXPECT_EQ(latch, word) << "step=" << i << " seed=" << seed;

				string line = std::format("{}:{:04x}:{:04x}:{:04x}:{:02x}", i, word, latch, status, reg1);
				for (uint8_t ch : line) {
					hash ^= ch;
					hash *= 1099511628211ull;
				}
			}

			return std::make_tuple(hash, scaffold.GetBus().GetVdpStatus(), scaffold.GetBus().GetVdpControlWordLatch());
		};

		auto runA = runScenario();
		auto runB = runScenario();

		EXPECT_EQ(std::get<0>(runA), std::get<0>(runB));
		EXPECT_EQ(std::get<1>(runA), std::get<1>(runB));
		EXPECT_EQ(std::get<2>(runA), std::get<2>(runB));
	}
}
