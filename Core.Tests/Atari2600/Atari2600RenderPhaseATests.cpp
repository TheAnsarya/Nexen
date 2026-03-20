#include "pch.h"
#include "Shared/Emulator.h"
#include "Atari2600/Atari2600Console.h"
#include "Utilities/VirtualFile.h"

namespace {
	void LoadNopRom(Atari2600Console& console, const string& name) {
		vector<uint8_t> rom(4096, 0xEA);
		VirtualFile romFile(rom.data(), rom.size(), name);
		ASSERT_EQ(console.LoadRom(romFile), LoadRomResult::Success);
	}

	const uint16_t* GetFramePixels(Atari2600Console& console) {
		PpuFrameInfo frame = console.GetPpuFrame();
		return reinterpret_cast<const uint16_t*>(frame.FrameBuffer);
	}

	TEST(Atari2600RenderPhaseATests, PlayfieldRegistersChangeFrameOutput) {
		Emulator emu;
		Atari2600Console console(&emu);
		LoadNopRom(console, "render-playfield.a26");

		console.DebugWriteCartridge(0x0009, 0x01); // colubk
		console.DebugWriteCartridge(0x0008, 0x2F); // colupf
		console.DebugWriteCartridge(0x000D, 0x00); // pf0
		console.DebugWriteCartridge(0x000E, 0x00); // pf1
		console.DebugWriteCartridge(0x000F, 0x00); // pf2
		console.RunFrame();

		const uint16_t* baseline = GetFramePixels(console);
		uint16_t backgroundPixel = baseline[0];

		console.DebugWriteCartridge(0x000D, 0xF0);
		console.DebugWriteCartridge(0x000E, 0xFF);
		console.DebugWriteCartridge(0x000F, 0xFF);
		console.RunFrame();

		const uint16_t* playfield = GetFramePixels(console);
		size_t changedPixels = 0;
		for (uint32_t x = 0; x < Atari2600Console::ScreenWidth; x++) {
			if (playfield[x] != backgroundPixel) {
				changedPixels++;
			}
		}

		EXPECT_GT(changedPixels, 0u);
	}

	TEST(Atari2600RenderPhaseATests, PlayerLayerOverlaysPlayfieldColor) {
		Emulator emu;
		Atari2600Console console(&emu);
		LoadNopRom(console, "render-player-layer.a26");

		console.DebugWriteCartridge(0x0009, 0x01); // colubk
		console.DebugWriteCartridge(0x0008, 0x2F); // colupf
		console.DebugWriteCartridge(0x0006, 0xAE); // colup0
		console.DebugWriteCartridge(0x000D, 0xF0);
		console.DebugWriteCartridge(0x000E, 0xFF);
		console.DebugWriteCartridge(0x000F, 0xFF);
		console.DebugWriteCartridge(0x001B, 0xFF); // grp0
		console.RunFrame();

		const uint16_t* withPlayer = GetFramePixels(console);
		uint16_t playfieldPixel = withPlayer[0];
		uint16_t playerPixel = withPlayer[24];
		EXPECT_NE(playerPixel, playfieldPixel);

		console.DebugWriteCartridge(0x001B, 0x00);
		console.RunFrame();

		const uint16_t* withoutPlayer = GetFramePixels(console);
		EXPECT_EQ(withoutPlayer[24], withoutPlayer[0]);
	}

	TEST(Atari2600RenderPhaseATests, Refp0FlipsPlayerBitDirectionAcrossSpriteSpan) {
		Emulator emu;
		Atari2600Console console(&emu);
		LoadNopRom(console, "render-player-reflect.a26");

		console.DebugWriteCartridge(0x0009, 0x01); // colubk
		console.DebugWriteCartridge(0x0006, 0xAE); // colup0
		console.DebugWriteCartridge(0x001B, 0x80); // grp0 left-most bit only

		console.DebugWriteCartridge(0x000B, 0x00); // refp0 off
		console.RunFrame();
		const uint16_t* nonReflected = GetFramePixels(console);
		uint16_t backgroundPixel = nonReflected[32];
		EXPECT_NE(nonReflected[24], backgroundPixel);
		EXPECT_EQ(nonReflected[31], backgroundPixel);

		console.DebugWriteCartridge(0x000B, 0x08); // refp0 on
		console.RunFrame();
		const uint16_t* reflected = GetFramePixels(console);
		EXPECT_EQ(reflected[24], backgroundPixel);
		EXPECT_NE(reflected[31], backgroundPixel);
	}

	TEST(Atari2600RenderPhaseATests, BallEnableProducesDeterministicPlayfieldColoredPixels) {
		Emulator emu;
		Atari2600Console console(&emu);
		LoadNopRom(console, "render-ball-layer.a26");

		console.DebugWriteCartridge(0x0009, 0x01); // colubk
		console.DebugWriteCartridge(0x0008, 0x3E); // colupf
		console.DebugWriteCartridge(0x000D, 0x00);
		console.DebugWriteCartridge(0x000E, 0x00);
		console.DebugWriteCartridge(0x000F, 0x00);
		console.DebugWriteCartridge(0x001F, 0x02); // enabl
		console.RunFrame();

		const uint16_t* framePixels = GetFramePixels(console);
		uint16_t backgroundPixel = framePixels[0];

		size_t ballPixels = 0;
		for (uint32_t x = 80; x < 84; x++) {
			if (framePixels[x] != backgroundPixel) {
				ballPixels++;
			}
		}

		EXPECT_EQ(ballPixels, 4u);
	}

	TEST(Atari2600RenderPhaseATests, HmoveBlankingDrawsBlackBarOnFirstEightPixels) {
		Emulator emu;
		Atari2600Console console(&emu);
		LoadNopRom(console, "render-hmove-blank.a26");

		console.DebugWriteCartridge(0x0009, 0x0E); // colubk (non-black)
		console.DebugWriteCartridge(0x002A, 0x00); // hmove
		console.RunFrame();

		const uint16_t* framePixels = GetFramePixels(console);
		for (uint32_t x = 0; x < 8; x++) {
			EXPECT_EQ(framePixels[x], 0u);
		}

		uint16_t nonBlankReference = framePixels[8];
		EXPECT_NE(nonBlankReference, 0u);
		EXPECT_EQ(framePixels[Atari2600Console::ScreenWidth], nonBlankReference);
	}

	TEST(Atari2600RenderPhaseATests, PlayfieldDuplicateAndReflectModesRenderExpectedHalves) {
		Emulator emu;
		Atari2600Console console(&emu);
		LoadNopRom(console, "render-playfield-reflect.a26");

		console.DebugWriteCartridge(0x0009, 0x01); // colubk
		console.DebugWriteCartridge(0x0008, 0x2F); // colupf
		console.DebugWriteCartridge(0x000D, 0x10); // pf0 bit 4 only (left-most nibble pixel)
		console.DebugWriteCartridge(0x000E, 0x00);
		console.DebugWriteCartridge(0x000F, 0x00);

		console.DebugWriteCartridge(0x000A, 0x00); // duplicate mode
		console.RunFrame();
		const uint16_t* duplicateFrame = GetFramePixels(console);
		uint16_t backgroundPixel = duplicateFrame[4];
		EXPECT_NE(duplicateFrame[0], backgroundPixel);
		EXPECT_NE(duplicateFrame[80], backgroundPixel);
		EXPECT_EQ(duplicateFrame[156], backgroundPixel);

		console.DebugWriteCartridge(0x000A, 0x01); // reflect mode
		console.RunFrame();
		const uint16_t* reflectFrame = GetFramePixels(console);
		EXPECT_NE(reflectFrame[0], backgroundPixel);
		EXPECT_EQ(reflectFrame[80], backgroundPixel);
		EXPECT_NE(reflectFrame[156], backgroundPixel);
	}

	TEST(Atari2600RenderPhaseATests, ScoreModeUsesPlayerColorsForPlayfieldHalves) {
		Emulator emu;
		Atari2600Console console(&emu);
		LoadNopRom(console, "render-playfield-score.a26");

		console.DebugWriteCartridge(0x0009, 0x01); // colubk
		console.DebugWriteCartridge(0x0008, 0x2F); // colupf
		console.DebugWriteCartridge(0x0006, 0xAE); // colup0
		console.DebugWriteCartridge(0x0007, 0x4C); // colup1
		console.DebugWriteCartridge(0x000D, 0xF0);
		console.DebugWriteCartridge(0x000E, 0xFF);
		console.DebugWriteCartridge(0x000F, 0xFF);

		console.DebugWriteCartridge(0x000A, 0x00); // score mode off
		console.RunFrame();
		const uint16_t* baseFrame = GetFramePixels(console);
		uint16_t baseLeft = baseFrame[0];
		uint16_t baseRight = baseFrame[120];
		EXPECT_EQ(baseLeft, baseRight);

		console.DebugWriteCartridge(0x000A, 0x02); // score mode on
		console.RunFrame();
		const uint16_t* scoreFrame = GetFramePixels(console);
		uint16_t scoreLeft = scoreFrame[0];
		uint16_t scoreRight = scoreFrame[120];
		EXPECT_NE(scoreLeft, scoreRight);
		EXPECT_NE(scoreLeft, baseLeft);
		EXPECT_NE(scoreRight, baseRight);
	}

	TEST(Atari2600RenderPhaseATests, PriorityBitPlacesPlayfieldAbovePlayers) {
		Emulator emu;
		Atari2600Console console(&emu);
		LoadNopRom(console, "render-playfield-priority.a26");

		console.DebugWriteCartridge(0x0009, 0x01); // colubk
		console.DebugWriteCartridge(0x0008, 0x3E); // colupf
		console.DebugWriteCartridge(0x0006, 0xAE); // colup0
		console.DebugWriteCartridge(0x000D, 0xF0);
		console.DebugWriteCartridge(0x000E, 0xFF);
		console.DebugWriteCartridge(0x000F, 0xFF);
		console.DebugWriteCartridge(0x0010, 0x00); // resp0 at x=0
		console.DebugWriteCartridge(0x001B, 0xFF); // grp0 enabled

		console.DebugWriteCartridge(0x000A, 0x00); // priority off
		console.RunFrame();
		const uint16_t* playerPriorityFrame = GetFramePixels(console);
		uint16_t playerDominantPixel = playerPriorityFrame[0];

		console.DebugWriteCartridge(0x000A, 0x04); // priority on
		console.RunFrame();
		const uint16_t* playfieldPriorityFrame = GetFramePixels(console);
		uint16_t playfieldDominantPixel = playfieldPriorityFrame[0];

		EXPECT_NE(playfieldDominantPixel, playerDominantPixel);
		EXPECT_EQ(playfieldDominantPixel, playfieldPriorityFrame[4]);
	}

	TEST(Atari2600RenderPhaseATests, PlayerSpriteWrapsAtRightEdge) {
		Emulator emu;
		Atari2600Console console(&emu);
		LoadNopRom(console, "render-player-wrap.a26");

		console.DebugWriteCartridge(0x0009, 0x01); // colubk
		console.DebugWriteCartridge(0x0006, 0xAE); // colup0
		console.DebugWriteCartridge(0x001B, 0xFF); // grp0
		console.StepCpuCycles(53);                 // color clock 159
		console.DebugWriteCartridge(0x0010, 0x00); // resp0 => x=159
		console.RunFrame();

		const uint16_t* frame = GetFramePixels(console);
		uint16_t background = frame[8];
		EXPECT_NE(frame[159], background);
		EXPECT_NE(frame[0], background);
		EXPECT_NE(frame[6], background);
		EXPECT_EQ(frame[7], background);
	}

	TEST(Atari2600RenderPhaseATests, BallWrapSetsCxblpfCollisionLatch) {
		Emulator emu;
		Atari2600Console console(&emu);
		LoadNopRom(console, "render-ball-wrap-collision.a26");

		console.DebugWriteCartridge(0x000D, 0x10); // pf0 only left-most playfield pixel
		console.DebugWriteCartridge(0x000E, 0x00);
		console.DebugWriteCartridge(0x000F, 0x00);
		console.DebugWriteCartridge(0x001F, 0x02); // enabl
		console.StepCpuCycles(53);                 // color clock 159
		console.DebugWriteCartridge(0x0014, 0x00); // resbl => x=159
		console.RunFrame();

		EXPECT_TRUE((console.DebugReadCartridge(0x0006) & 0x80) != 0); // cxblpf
	}

	TEST(Atari2600RenderPhaseATests, NusizPlayerCopyModeRendersExpectedSpacing) {
		Emulator emu;
		Atari2600Console console(&emu);
		LoadNopRom(console, "render-nusiz-player-copy.a26");

		console.DebugWriteCartridge(0x0009, 0x01); // colubk
		console.DebugWriteCartridge(0x0006, 0xAE); // colup0
		console.DebugWriteCartridge(0x001B, 0xFF); // grp0

		console.DebugWriteCartridge(0x0004, 0x00); // nusiz0 single copy
		console.RunFrame();
		const uint16_t* singleFrame = GetFramePixels(console);
		uint16_t backgroundPixel = singleFrame[8];
		EXPECT_NE(singleFrame[24], backgroundPixel);
		EXPECT_EQ(singleFrame[40], backgroundPixel);

		console.DebugWriteCartridge(0x0004, 0x01); // nusiz0 two close copies (0,+16)
		console.RunFrame();
		const uint16_t* copyFrame = GetFramePixels(console);
		EXPECT_NE(copyFrame[24], backgroundPixel);
		EXPECT_NE(copyFrame[40], backgroundPixel);
	}

	TEST(Atari2600RenderPhaseATests, NusizMissileWidthBitsScaleMissileSpan) {
		Emulator emu;
		Atari2600Console console(&emu);
		LoadNopRom(console, "render-nusiz-missile-width.a26");

		console.DebugWriteCartridge(0x0009, 0x01); // colubk
		console.DebugWriteCartridge(0x0006, 0xAE); // colup0
		console.DebugWriteCartridge(0x001D, 0x02); // enam0

		console.DebugWriteCartridge(0x0004, 0x00); // nusiz0 missile width 1
		console.RunFrame();
		const uint16_t* width1Frame = GetFramePixels(console);
		uint16_t backgroundPixel = width1Frame[0];
		size_t width1Count = 0;
		for (uint32_t x = 32; x < 40; x++) {
			if (width1Frame[x] != backgroundPixel) {
				width1Count++;
			}
		}
		EXPECT_EQ(width1Count, 1u);

		console.DebugWriteCartridge(0x0004, 0x30); // nusiz0 missile width 8
		console.RunFrame();
		const uint16_t* width8Frame = GetFramePixels(console);
		size_t width8Count = 0;
		for (uint32_t x = 32; x < 40; x++) {
			if (width8Frame[x] != backgroundPixel) {
				width8Count++;
			}
		}
		EXPECT_EQ(width8Count, 8u);
	}
}
