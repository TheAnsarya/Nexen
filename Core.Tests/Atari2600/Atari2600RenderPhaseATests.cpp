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
}
