#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"

namespace {
	TEST(GenesisYm2612TimingTests, AddressDataWriteSequenceUpdatesYmRegisters) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		scaffold.GetBus().WriteByte(0xA04000, 0x22);
		scaffold.GetBus().WriteByte(0xA04001, 0x5A);
		scaffold.GetBus().WriteByte(0xA04002, 0x30);
		scaffold.GetBus().WriteByte(0xA04003, 0x7F);

		EXPECT_EQ(scaffold.GetBus().GetYmRegister(0x22), 0x5Au);
		EXPECT_EQ(scaffold.GetBus().GetYmRegister(0x130), 0x7Fu);
		EXPECT_EQ(scaffold.GetBus().GetYmWriteCount(), 2u);
	}

	TEST(GenesisYm2612TimingTests, YmSampleGenerationIsDeterministicAcrossRuns) {
		GenesisM68kBoundaryScaffold scaffoldA;
		scaffoldA.Startup();
		scaffoldA.GetBus().WriteByte(0xA04000, 0x22);
		scaffoldA.GetBus().WriteByte(0xA04001, 0x11);
		scaffoldA.GetBus().WriteByte(0xA04000, 0x27);
		scaffoldA.GetBus().WriteByte(0xA04001, 0x34);
		scaffoldA.StepFrameScaffold(144u * 12u);

		GenesisM68kBoundaryScaffold scaffoldB;
		scaffoldB.Startup();
		scaffoldB.GetBus().WriteByte(0xA04000, 0x22);
		scaffoldB.GetBus().WriteByte(0xA04001, 0x11);
		scaffoldB.GetBus().WriteByte(0xA04000, 0x27);
		scaffoldB.GetBus().WriteByte(0xA04001, 0x34);
		scaffoldB.StepFrameScaffold(144u * 12u);

		EXPECT_EQ(scaffoldA.GetBus().GetYmSampleCount(), scaffoldB.GetBus().GetYmSampleCount());
		EXPECT_EQ(scaffoldA.GetBus().GetYmLastSample(), scaffoldB.GetBus().GetYmLastSample());
		EXPECT_EQ(scaffoldA.GetBus().GetYmDigest(), scaffoldB.GetBus().GetYmDigest());
	}

	TEST(GenesisYm2612TimingTests, YmSampleCountTracksStepTimingProgression) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		scaffold.StepFrameScaffold(143);
		EXPECT_EQ(scaffold.GetBus().GetYmSampleCount(), 0u);
		scaffold.StepFrameScaffold(1);
		EXPECT_EQ(scaffold.GetBus().GetYmSampleCount(), 1u);
	}
}
