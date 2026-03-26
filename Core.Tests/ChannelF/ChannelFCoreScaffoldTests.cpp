#include "pch.h"
#include "ChannelF/ChannelFCoreScaffold.h"

TEST(ChannelFCoreScaffold, ResetClearsState) {
	ChannelFCoreScaffold core;
	core.SetPanelButtons(0x0f);
	core.SetRightController(0xaa);
	core.SetLeftController(0x55);
	core.RunFrame();

	core.Reset();

	EXPECT_EQ(core.GetFrameCount(), 0u);
	EXPECT_EQ(core.GetMasterClock(), 0u);
	EXPECT_EQ(core.GetDeterministicState(), 0x6d2b79f5u);
}

TEST(ChannelFCoreScaffold, RunFrameAdvancesDeterministically) {
	ChannelFCoreScaffold a;
	ChannelFCoreScaffold b;

	a.SetPanelButtons(0x03);
	a.SetRightController(0x21);
	a.SetLeftController(0x84);

	b.SetPanelButtons(0x03);
	b.SetRightController(0x21);
	b.SetLeftController(0x84);

	for(int i = 0; i < 120; i++) {
		a.RunFrame();
		b.RunFrame();
	}

	EXPECT_EQ(a.GetFrameCount(), 120u);
	EXPECT_EQ(b.GetFrameCount(), 120u);
	EXPECT_EQ(a.GetMasterClock(), 120ull * ChannelFCoreScaffold::CyclesPerFrame);
	EXPECT_EQ(a.GetMasterClock(), b.GetMasterClock());
	EXPECT_EQ(a.GetDeterministicState(), b.GetDeterministicState());
}

TEST(ChannelFCoreScaffold, SerializeRoundtripPreservesState) {
	ChannelFCoreScaffold original;
	original.SetPanelButtons(0x0a);
	original.SetRightController(0x40);
	original.SetLeftController(0x80);
	for(int i = 0; i < 20; i++) {
		original.RunFrame();
	}

	Serializer saver(1, true, SerializeFormat::Binary);
	original.Serialize(saver);

	std::stringstream ss;
	saver.SaveTo(ss);
	ss.seekg(0);

	ChannelFCoreScaffold loaded;
	Serializer loader(1, false, SerializeFormat::Binary);
	loader.LoadFrom(ss);
	loaded.Serialize(loader);

	EXPECT_EQ(loaded.GetFrameCount(), original.GetFrameCount());
	EXPECT_EQ(loaded.GetMasterClock(), original.GetMasterClock());
	EXPECT_EQ(loaded.GetDeterministicState(), original.GetDeterministicState());
}

TEST(ChannelFCoreScaffold, LuxorHashesSelectSystemIIVariant) {
	ChannelFCoreScaffold core;
	core.DetectVariantFromHashes(
		"759e2ed31fbde4a2d8daf8b9f3e0dffebc90dae2",
		"95d339631d867c8f1d15a5f2ec26069d"
	);

	EXPECT_EQ(core.GetVariant(), ChannelFBiosVariant::SystemII);
}
