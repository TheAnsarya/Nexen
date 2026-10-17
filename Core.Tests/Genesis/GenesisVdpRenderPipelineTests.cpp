#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"

namespace {
	TEST(GenesisVdpRenderPipelineTests, SpritePriorityOverridesBackgroundComposition) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		scaffold.GetBus().SetRenderCompositionInputs(0x12, true, 0x06, false, 0x00, false, false, 0x2A, true);
		scaffold.GetBus().RenderScaffoldLine(4);

		EXPECT_EQ(scaffold.GetBus().GetRenderLinePixel(0), 0x2Au);
	}

	TEST(GenesisVdpRenderPipelineTests, WindowPriorityCanOverridePlaneAWhenEnabled) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		scaffold.GetBus().SetRenderCompositionInputs(0x21, true, 0x10, false, 0x3C, true, true, 0x00, false);
		scaffold.GetBus().RenderScaffoldLine(4);

		EXPECT_EQ(scaffold.GetBus().GetRenderLinePixel(0), 0x3Cu);
	}

	TEST(GenesisVdpRenderPipelineTests, RenderDigestIsDeterministicForStableInputs) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		scaffold.GetBus().SetRenderCompositionInputs(0x10, true, 0x04, false, 0x00, false, false, 0x00, false);
		scaffold.GetBus().SetScroll(3, 1);
		scaffold.GetBus().RenderScaffoldLine(32);
		string digestA = scaffold.GetBus().GetRenderLineDigest();
		scaffold.GetBus().RenderScaffoldLine(32);
		string digestB = scaffold.GetBus().GetRenderLineDigest();

		EXPECT_FALSE(digestA.empty());
		EXPECT_EQ(digestA, digestB);
	}

	TEST(GenesisVdpRenderPipelineTests, ScrollOffsetChangesRenderDigest) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		scaffold.GetBus().SetRenderCompositionInputs(0x18, true, 0x03, false, 0x00, false, false, 0x00, false);
		scaffold.GetBus().SetScroll(0, 0);
		scaffold.GetBus().RenderScaffoldLine(32);
		string digestA = scaffold.GetBus().GetRenderLineDigest();
		scaffold.GetBus().SetScroll(5, 0);
		scaffold.GetBus().RenderScaffoldLine(32);
		string digestB = scaffold.GetBus().GetRenderLineDigest();

		EXPECT_NE(digestA, digestB);
	}
}
