#include "pch.h"
#include "Genesis/GenesisVdp.h"
#include "Genesis/GenesisM68k.h"
#include "Genesis/Debugger/GenesisVdpTools.h"
#include "Shared/Emulator.h"

namespace {
	uint32_t CramToArgb(uint16_t cramColor) {
		uint8_t r3 = (uint8_t)((cramColor >> 1) & 0x07);
		uint8_t g3 = (uint8_t)((cramColor >> 5) & 0x07);
		uint8_t b3 = (uint8_t)((cramColor >> 9) & 0x07);

		uint8_t r = (uint8_t)((r3 << 5) | (r3 << 2) | (r3 >> 1));
		uint8_t g = (uint8_t)((g3 << 5) | (g3 << 2) | (g3 >> 1));
		uint8_t b = (uint8_t)((b3 << 5) | (b3 << 2) | (b3 >> 1));

		return 0xFF000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
	}

	void ConfigureSpriteRenderMode(GenesisVdp& vdp, bool h40, uint8_t reg5) {
		vdp.WriteControlPort((uint16_t)(0x8C00 | (h40 ? 0x81 : 0x80)));
		vdp.WriteControlPort(0x8144);
		vdp.WriteControlPort((uint16_t)(0x8500 | reg5));
	}

	void WriteSpriteSatEntry(uint8_t* vram, uint16_t satBase, uint8_t index, uint16_t rawY, uint8_t vertCells, uint8_t horizCells, uint8_t link, uint16_t attrWord, uint16_t rawX) {
		uint16_t addr = (uint16_t)(satBase + (uint16_t)index * 8u);
		uint16_t w1 = (uint16_t)((((horizCells - 1u) & 0x03u) << 10) | (((vertCells - 1u) & 0x03u) << 8) | (link & 0x7Fu));
		vram[(addr + 0) & 0xFFFF] = (uint8_t)(rawY >> 8);
		vram[(addr + 1) & 0xFFFF] = (uint8_t)rawY;
		vram[(addr + 2) & 0xFFFF] = (uint8_t)(w1 >> 8);
		vram[(addr + 3) & 0xFFFF] = (uint8_t)w1;
		vram[(addr + 4) & 0xFFFF] = (uint8_t)(attrWord >> 8);
		vram[(addr + 5) & 0xFFFF] = (uint8_t)attrWord;
		vram[(addr + 6) & 0xFFFF] = (uint8_t)(rawX >> 8);
		vram[(addr + 7) & 0xFFFF] = (uint8_t)rawX;
	}

	void WriteSolidSpriteTileRow0(uint8_t* vram, uint16_t tileIndex, uint8_t colorNibble) {
		uint32_t tileBase = (uint32_t)tileIndex * 32u;
		uint8_t packed = (uint8_t)((colorNibble << 4) | colorNibble);
		for (uint32_t i = 0; i < 4; i++) {
			vram[(tileBase + i) & 0xFFFF] = packed;
		}
	}

	void BuildArgbPaletteFromCram(GenesisVdp& vdp, std::array<uint32_t, 64>& palette) {
		for (uint32_t i = 0; i < palette.size(); i++) {
			palette[i] = CramToArgb(vdp.GetCramPointer()[i]);
		}
	}

	void SetupSingleVisibleSpriteAtBase(GenesisVdp& vdp, uint16_t satBase) {
		uint8_t* vram = vdp.GetVramPointer();
		memset(vram, 0, 0x10000);

		constexpr uint16_t kTileIndex = 8;
		uint32_t tileBase = kTileIndex * 32u;
		// Tile row 0: eight pixels with color index 1.
		vram[tileBase + 0] = 0x11;
		vram[tileBase + 1] = 0x11;
		vram[tileBase + 2] = 0x11;
		vram[tileBase + 3] = 0x11;

		uint16_t addr = satBase & 0xFFFF;
		// Sprite at (0,0) after hardware offset of 128.
		vram[(addr + 0) & 0xFFFF] = 0x00; // y high
		vram[(addr + 1) & 0xFFFF] = 0x80; // y low (128)
		vram[(addr + 2) & 0xFFFF] = 0x00; // size/link high (1x1)
		vram[(addr + 3) & 0xFFFF] = 0x00; // link = 0 end
		vram[(addr + 4) & 0xFFFF] = 0x80; // priority=1
		vram[(addr + 5) & 0xFFFF] = (uint8_t)kTileIndex;
		vram[(addr + 6) & 0xFFFF] = 0x00; // x high
		vram[(addr + 7) & 0xFFFF] = 0x80; // x low (128)
	}

	TEST(GenesisVdpObjectRunSafetyTests, RunOneScanlinePreservesNullDependencySafetyPreconditions) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		constexpr uint64_t kClocksPerScanline = 488;
		GenesisVdpState before = vdp.GetState();
		EXPECT_EQ(before.Registers[1] & 0x20, 0);
		EXPECT_EQ(before.Registers[0] & 0x10, 0);
		EXPECT_FALSE(before.DmaActive);

		vdp.Run(kClocksPerScanline);

		GenesisVdpState after = vdp.GetState();
		EXPECT_FALSE(after.DmaActive);
		EXPECT_EQ(after.Registers[1] & 0x20, 0);
		EXPECT_EQ(after.Registers[0] & 0x10, 0);
	}

	TEST(GenesisVdpObjectRunSafetyTests, HIntCounterUsesRuntimeCounterWhileKeepingR10Stable) {
		GenesisM68k cpu;
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, &cpu, nullptr);

		// Enable HBlank interrupt path (R0 bit 4) and set R10 counter to 2.
		vdp.WriteControlPort(0x8012);
		vdp.WriteControlPort(0x8A02);

		constexpr uint64_t kClocksPerScanline = 488;
		vdp.Run(kClocksPerScanline * 2);

		GenesisVdpState state = vdp.GetState();
		EXPECT_EQ(state.Registers[10], 0x02);
		EXPECT_EQ(state.HIntCounter, 0x00);
	}

	TEST(GenesisVdpObjectRunSafetyTests, SetRegionTogglesPalStatusBit) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		GenesisVdpState initial = vdp.GetState();
		EXPECT_EQ(initial.StatusRegister & VdpStatus::PalMode, 0u);

		vdp.SetRegion(true);
		EXPECT_EQ(vdp.GetScreenHeight(), 240u);
		GenesisVdpState palState = vdp.GetState();
		EXPECT_NE(palState.StatusRegister & VdpStatus::PalMode, 0u);

		vdp.SetRegion(false);
		EXPECT_EQ(vdp.GetScreenHeight(), 224u);
		GenesisVdpState ntscState = vdp.GetState();
		EXPECT_EQ(ntscState.StatusRegister & VdpStatus::PalMode, 0u);
	}

	TEST(GenesisVdpObjectRunSafetyTests, ResetAppliesStartupDefaultsForModeAndAutoIncrement) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		GenesisVdpState state = vdp.GetState();
		EXPECT_EQ(state.Registers[1], 0x04);
		EXPECT_EQ(state.Registers[12], 0x81);
		EXPECT_EQ(state.Registers[15], 0x02);
		EXPECT_NE(state.StatusRegister & VdpStatus::FifoEmpty, 0u);
		EXPECT_EQ(vdp.GetScreenWidth(), 320u);
	}

	TEST(GenesisVdpObjectRunSafetyTests, H40SpriteTableBaseMasksReg5Bit0ForVisibleSpriteFetch) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		vdp.GetCramPointer()[0] = 0x0000;
		vdp.GetCramPointer()[1] = 0x000e;

		// H40 mode (default) and display enabled.
		vdp.WriteControlPort(0x8C81);
		vdp.WriteControlPort(0x8144);
		// R5 bit0 set; H40 should ignore this bit and use base 0x0000.
		vdp.WriteControlPort(0x8501);

		SetupSingleVisibleSpriteAtBase(vdp, 0x0000);
		vdp.Run(488);

		uint16_t* frame = vdp.GetScreenBuffer(false);
		EXPECT_EQ(frame[0], 0x7c00);
	}

	TEST(GenesisVdpObjectRunSafetyTests, H32SpriteTableBaseUsesReg5Bit0ForVisibleSpriteFetch) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		vdp.GetCramPointer()[0] = 0x0000;
		vdp.GetCramPointer()[1] = 0x000e;

		// Force H32 mode and enable display.
		vdp.WriteControlPort(0x8C80);
		vdp.WriteControlPort(0x8144);
		// In H32 mode, R5 bit0 selects SAT base 0x0200.
		vdp.WriteControlPort(0x8501);

		SetupSingleVisibleSpriteAtBase(vdp, 0x0200);
		vdp.Run(488);

		uint16_t* frame = vdp.GetScreenBuffer(false);
		EXPECT_EQ(frame[0], 0x7c00);
	}

	TEST(GenesisVdpObjectRunSafetyTests, H32DebuggerPreviewDropsSpritesBeyondSixteenPerLine) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		vdp.GetCramPointer()[0] = 0x0000;
		vdp.GetCramPointer()[1] = 0x000e;
		ConfigureSpriteRenderMode(vdp, false, 0x00);

		uint8_t* vram = vdp.GetVramPointer();
		memset(vram, 0, 0x10000);
		constexpr uint16_t kTileIndex = 16;
		WriteSolidSpriteTileRow0(vram, kTileIndex, 1);

		constexpr uint16_t kSatBase = 0x0000;
		for (uint8_t i = 0; i < 17; i++) {
			uint8_t link = (i == 16) ? 0 : (uint8_t)(i + 1);
			uint16_t rawX = (uint16_t)(128 + (uint16_t)i * 8u);
			WriteSpriteSatEntry(vram, kSatBase, i, 128, 1, 1, link, (uint16_t)(0x8000 | kTileIndex), rawX);
		}

		vdp.Run(488);

		GenesisVdpState state = vdp.GetState();
		GenesisVdpState ppuState = state;
		GenesisVdpTools tools(nullptr, nullptr, nullptr);
		GetSpritePreviewOptions options = {};
		options.Background = SpriteBackground::Gray;
		DebugSpritePreviewInfo previewInfo = tools.GetSpritePreviewInfo(options, (BaseState&)state, (BaseState&)ppuState);

		std::array<uint32_t, 64> palette = {};
		for (uint32_t i = 0; i < palette.size(); i++) {
			palette[i] = CramToArgb(vdp.GetCramPointer()[i]);
		}

		std::vector<DebugSpriteInfo> sprites(previewInfo.SpriteCount);
		std::vector<uint32_t> spritePreviews(previewInfo.SpriteCount * 128u * 128u);
		std::vector<uint32_t> screenPreview(previewInfo.Width * previewInfo.Height);

		tools.GetSpriteList(options, (BaseState&)state, (BaseState&)ppuState, vdp.GetVramPointer(), nullptr, palette.data(), sprites.data(), spritePreviews.data(), screenPreview.data());

		uint32_t previewRowStart = previewInfo.VisibleY * previewInfo.Width + previewInfo.VisibleX;
		constexpr uint32_t kVisibleBg = 0xFF666666;
		EXPECT_NE(screenPreview[previewRowStart], kVisibleBg);
		EXPECT_EQ(screenPreview[previewRowStart + 128], kVisibleBg);
	}

	TEST(GenesisVdpObjectRunSafetyTests, MaskSpriteHidesFollowingSpritesInDebuggerPreview) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		vdp.GetCramPointer()[0] = 0x0000;
		vdp.GetCramPointer()[1] = 0x000e;
		ConfigureSpriteRenderMode(vdp, true, 0x00);

		uint8_t* vram = vdp.GetVramPointer();
		memset(vram, 0, 0x10000);
		constexpr uint16_t kTileIndex = 4;
		WriteSolidSpriteTileRow0(vram, kTileIndex, 1);

		constexpr uint16_t kSatBase = 0x0000;
		WriteSpriteSatEntry(vram, kSatBase, 0, 128, 1, 1, 1, (uint16_t)(0x8000 | kTileIndex), 128);
		WriteSpriteSatEntry(vram, kSatBase, 1, 128, 1, 1, 2, (uint16_t)(0x8000 | kTileIndex), 0);
		WriteSpriteSatEntry(vram, kSatBase, 2, 128, 1, 1, 0, (uint16_t)(0x8000 | kTileIndex), 144);

		vdp.Run(488);

		GenesisVdpState state = vdp.GetState();
		GenesisVdpState ppuState = state;
		GenesisVdpTools tools(nullptr, nullptr, nullptr);
		GetSpritePreviewOptions options = {};
		options.Background = SpriteBackground::Gray;
		DebugSpritePreviewInfo previewInfo = tools.GetSpritePreviewInfo(options, (BaseState&)state, (BaseState&)ppuState);

		std::array<uint32_t, 64> palette = {};
		for (uint32_t i = 0; i < palette.size(); i++) {
			palette[i] = CramToArgb(vdp.GetCramPointer()[i]);
		}

		std::vector<DebugSpriteInfo> sprites(previewInfo.SpriteCount);
		std::vector<uint32_t> spritePreviews(previewInfo.SpriteCount * 128u * 128u);
		std::vector<uint32_t> screenPreview(previewInfo.Width * previewInfo.Height);

		tools.GetSpriteList(options, (BaseState&)state, (BaseState&)ppuState, vdp.GetVramPointer(), nullptr, palette.data(), sprites.data(), spritePreviews.data(), screenPreview.data());

		uint32_t previewRowStart = previewInfo.VisibleY * previewInfo.Width + previewInfo.VisibleX;
		constexpr uint32_t kVisibleBg = 0xFF666666;
		EXPECT_NE(screenPreview[previewRowStart], kVisibleBg);
		EXPECT_EQ(screenPreview[previewRowStart + 16], kVisibleBg);
	}

	TEST(GenesisVdpObjectRunSafetyTests, DebugSpriteScreenPreviewParitiesRenderedSliceForSingleSprite) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		vdp.GetCramPointer()[0] = 0x0000;
		vdp.GetCramPointer()[1] = 0x000e;
		ConfigureSpriteRenderMode(vdp, true, 0x00);
		SetupSingleVisibleSpriteAtBase(vdp, 0x0000);
		vdp.Run(488);

		GenesisVdpState state = vdp.GetState();
		GenesisVdpState ppuState = state;
		GenesisVdpTools tools(nullptr, nullptr, nullptr);
		GetSpritePreviewOptions options = {};
		options.Background = SpriteBackground::Gray;
		DebugSpritePreviewInfo previewInfo = tools.GetSpritePreviewInfo(options, (BaseState&)state, (BaseState&)ppuState);

		std::array<uint32_t, 64> palette = {};
		for (uint32_t i = 0; i < palette.size(); i++) {
			palette[i] = CramToArgb(vdp.GetCramPointer()[i]);
		}

		std::vector<DebugSpriteInfo> sprites(previewInfo.SpriteCount);
		std::vector<uint32_t> spritePreviews(previewInfo.SpriteCount * 128u * 128u);
		std::vector<uint32_t> screenPreview(previewInfo.Width * previewInfo.Height);

		tools.GetSpriteList(options, (BaseState&)state, (BaseState&)ppuState, vdp.GetVramPointer(), nullptr, palette.data(), sprites.data(), spritePreviews.data(), screenPreview.data());

		uint16_t* frame = vdp.GetScreenBuffer(false);
		uint32_t previewRowStart = previewInfo.VisibleY * previewInfo.Width + previewInfo.VisibleX;
		constexpr uint32_t kVisibleBg = 0xFF666666;
		for (uint32_t x = 0; x < 8; x++) {
			bool frameDrawn = frame[x] != 0;
			bool previewDrawn = screenPreview[previewRowStart + x] != kVisibleBg;
			EXPECT_EQ(frameDrawn, previewDrawn);
		}

		EXPECT_EQ(sprites[0].Visibility, SpriteVisibility::Visible);
		for (uint32_t x = 0; x < 8; x++) {
			EXPECT_NE(spritePreviews[x], kVisibleBg);
		}
	}

	TEST(GenesisVdpObjectRunSafetyTests, DebugTilemapParitiesRenderedFrameBackdropForTransparentPlaneCellAtOrigin) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		vdp.GetCramPointer()[0] = 0x000e;
		vdp.GetCramPointer()[1] = 0x0000;
		vdp.WriteControlPort(0x8144);
		vdp.WriteControlPort(0x8200);
		vdp.WriteControlPort(0x9000);

		uint8_t* vram = vdp.GetVramPointer();
		memset(vram, 0, 0x10000);
		constexpr uint16_t kTileIndex = 12;

		vram[0] = (uint8_t)(kTileIndex >> 8);
		vram[1] = (uint8_t)kTileIndex;

		vdp.Run(488);

		GenesisVdpState state = vdp.GetState();
		GenesisVdpState ppuState = state;
		GenesisVdpTools tools(nullptr, nullptr, nullptr);
		GetTilemapOptions options = {};
		options.Layer = 0;
		options.DisplayMode = TilemapDisplayMode::Default;

		std::array<uint32_t, 64> palette = {};
		BuildArgbPaletteFromCram(vdp, palette);

		FrameInfo tilemapSize = tools.GetTilemapSize(options, (BaseState&)state);
		std::vector<uint32_t> tilemapBuffer(tilemapSize.Width * tilemapSize.Height);
		tools.GetTilemap(options, (BaseState&)state, (BaseState&)ppuState, vram, palette.data(), tilemapBuffer.data());

		uint16_t* frame = vdp.GetScreenBuffer(false);
		EXPECT_EQ(frame[0], 0x7c00);
		EXPECT_EQ(tilemapBuffer[0], palette[0]);
	}

	TEST(GenesisVdpObjectRunSafetyTests, DebugTilemapReportsLineScrollForPlaneAAndB) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		vdp.WriteControlPort(0x8B03);
		vdp.WriteControlPort(0x8D00);

		uint8_t* vram = vdp.GetVramPointer();
		memset(vram, 0, 0x10000);
		vram[0] = 0x00;
		vram[1] = 0x15;
		vram[2] = 0x00;
		vram[3] = 0x22;

		GenesisVdpState state = vdp.GetState();
		GenesisVdpState ppuState = state;
		GenesisVdpTools tools(nullptr, nullptr, nullptr);

		std::array<uint32_t, 64> palette = {};
		BuildArgbPaletteFromCram(vdp, palette);

		GetTilemapOptions planeAOptions = {};
		planeAOptions.Layer = 0;
		planeAOptions.DisplayMode = TilemapDisplayMode::Default;
		FrameInfo sizeA = tools.GetTilemapSize(planeAOptions, (BaseState&)state);
		std::vector<uint32_t> bufferA(sizeA.Width * sizeA.Height);
		DebugTilemapInfo infoA = tools.GetTilemap(planeAOptions, (BaseState&)state, (BaseState&)ppuState, vram, palette.data(), bufferA.data());

		GetTilemapOptions planeBOptions = {};
		planeBOptions.Layer = 1;
		planeBOptions.DisplayMode = TilemapDisplayMode::Default;
		FrameInfo sizeB = tools.GetTilemapSize(planeBOptions, (BaseState&)state);
		std::vector<uint32_t> bufferB(sizeB.Width * sizeB.Height);
		DebugTilemapInfo infoB = tools.GetTilemap(planeBOptions, (BaseState&)state, (BaseState&)ppuState, vram, palette.data(), bufferB.data());

		EXPECT_EQ(infoA.ScrollX, 0x15u);
		EXPECT_EQ(infoB.ScrollX, 0x22u);
	}

	TEST(GenesisVdpObjectRunSafetyTests, DebugTilemapUsesBaseHScrollEntryInMode0) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		vdp.WriteControlPort(0x8B00);
		vdp.WriteControlPort(0x8D00);

		uint8_t* vram = vdp.GetVramPointer();
		memset(vram, 0, 0x10000);
		vram[0] = 0x00;
		vram[1] = 0x2a;
		vram[2] = 0x00;
		vram[3] = 0x3b;

		GenesisVdpState state = vdp.GetState();
		state.VCounter = 211;
		GenesisVdpState ppuState = state;
		GenesisVdpTools tools(nullptr, nullptr, nullptr);

		std::array<uint32_t, 64> palette = {};
		BuildArgbPaletteFromCram(vdp, palette);

		GetTilemapOptions planeAOptions = {};
		planeAOptions.Layer = 0;
		planeAOptions.DisplayMode = TilemapDisplayMode::Default;
		FrameInfo sizeA = tools.GetTilemapSize(planeAOptions, (BaseState&)state);
		std::vector<uint32_t> bufferA(sizeA.Width * sizeA.Height);
		DebugTilemapInfo infoA = tools.GetTilemap(planeAOptions, (BaseState&)state, (BaseState&)ppuState, vram, palette.data(), bufferA.data());

		GetTilemapOptions planeBOptions = {};
		planeBOptions.Layer = 1;
		planeBOptions.DisplayMode = TilemapDisplayMode::Default;
		FrameInfo sizeB = tools.GetTilemapSize(planeBOptions, (BaseState&)state);
		std::vector<uint32_t> bufferB(sizeB.Width * sizeB.Height);
		DebugTilemapInfo infoB = tools.GetTilemap(planeBOptions, (BaseState&)state, (BaseState&)ppuState, vram, palette.data(), bufferB.data());

		EXPECT_EQ(infoA.ScrollX, 0x2au);
		EXPECT_EQ(infoB.ScrollX, 0x3bu);
	}

	TEST(GenesisVdpObjectRunSafetyTests, DebugTilemapClampsActiveLineTo240VisibleHeightInLineScrollMode) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		vdp.WriteControlPort(0x810c);
		vdp.WriteControlPort(0x8B03);
		vdp.WriteControlPort(0x8D00);

		uint8_t* vram = vdp.GetVramPointer();
		memset(vram, 0, 0x10000);
		constexpr uint16_t kClampedLine = 239;
		uint16_t addr = (uint16_t)(kClampedLine * 4u);
		vram[addr + 0] = 0x01;
		vram[addr + 1] = 0x11;
		vram[addr + 2] = 0x02;
		vram[addr + 3] = 0x22;

		GenesisVdpState state = vdp.GetState();
		state.VCounter = 260;
		GenesisVdpState ppuState = state;
		GenesisVdpTools tools(nullptr, nullptr, nullptr);

		std::array<uint32_t, 64> palette = {};
		BuildArgbPaletteFromCram(vdp, palette);

		GetTilemapOptions planeAOptions = {};
		planeAOptions.Layer = 0;
		planeAOptions.DisplayMode = TilemapDisplayMode::Default;
		FrameInfo sizeA = tools.GetTilemapSize(planeAOptions, (BaseState&)state);
		std::vector<uint32_t> bufferA(sizeA.Width * sizeA.Height);
		DebugTilemapInfo infoA = tools.GetTilemap(planeAOptions, (BaseState&)state, (BaseState&)ppuState, vram, palette.data(), bufferA.data());

		GetTilemapOptions planeBOptions = {};
		planeBOptions.Layer = 1;
		planeBOptions.DisplayMode = TilemapDisplayMode::Default;
		FrameInfo sizeB = tools.GetTilemapSize(planeBOptions, (BaseState&)state);
		std::vector<uint32_t> bufferB(sizeB.Width * sizeB.Height);
		DebugTilemapInfo infoB = tools.GetTilemap(planeBOptions, (BaseState&)state, (BaseState&)ppuState, vram, palette.data(), bufferB.data());

		EXPECT_EQ(infoA.ScrollX, 0x111u);
		EXPECT_EQ(infoB.ScrollX, 0x222u);
	}

	TEST(GenesisVdpObjectRunSafetyTests, DebugTilemapTileInfoUsesActiveHorizontalScrollPosition) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		vdp.WriteControlPort(0x8200);
		vdp.WriteControlPort(0x8B03);
		vdp.WriteControlPort(0x8D01);

		uint8_t* vram = vdp.GetVramPointer();
		memset(vram, 0, 0x10000);

		vram[0x0400] = 0x00;
		vram[0x0401] = 0x08;
		vram[0x0402] = 0x00;
		vram[0x0403] = 0x00;

		constexpr uint16_t kTileCol0 = 0x0011;
		constexpr uint16_t kTileCol1 = 0x0123;
		vram[0x0000] = (uint8_t)((kTileCol0 >> 8) & 0xFFu);
		vram[0x0001] = (uint8_t)(kTileCol0 & 0xFFu);
		vram[0x0002] = (uint8_t)((kTileCol1 >> 8) & 0xFFu);
		vram[0x0003] = (uint8_t)(kTileCol1 & 0xFFu);

		GenesisVdpState state = vdp.GetState();
		GenesisVdpState ppuState = state;
		GenesisVdpTools tools(nullptr, nullptr, nullptr);

		GetTilemapOptions options = {};
		options.Layer = 0;
		options.DisplayMode = TilemapDisplayMode::Default;
		DebugTilemapTileInfo info = tools.GetTilemapTileInfo(0, 0, vram, options, (BaseState&)state, (BaseState&)ppuState);

		EXPECT_EQ(info.Column, 1);
		EXPECT_EQ(info.Row, 0);
		EXPECT_EQ(info.TileMapAddress, 0x0002);
		EXPECT_EQ(info.TileIndex, kTileCol1 & 0x07FF);
	}

	TEST(GenesisVdpObjectRunSafetyTests, DebugTilemapTileInfoUsesPerColumnVerticalScrollAtSampledX) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		vdp.WriteControlPort(0x8200);
		vdp.WriteControlPort(0x8B04);

		uint8_t* vram = vdp.GetVramPointer();
		memset(vram, 0, 0x10000);

		constexpr uint16_t kTileRow0Col2 = 0x0007;
		constexpr uint16_t kTileRow2Col2 = 0x0155;
		constexpr uint16_t kRow0Col2Addr = 0x0004;
		constexpr uint16_t kRow2Col2Addr = 0x0084;
		vram[kRow0Col2Addr + 0] = (uint8_t)((kTileRow0Col2 >> 8) & 0xFFu);
		vram[kRow0Col2Addr + 1] = (uint8_t)(kTileRow0Col2 & 0xFFu);
		vram[kRow2Col2Addr + 0] = (uint8_t)((kTileRow2Col2 >> 8) & 0xFFu);
		vram[kRow2Col2Addr + 1] = (uint8_t)(kTileRow2Col2 & 0xFFu);

		GenesisVdpState state = vdp.GetState();
		state.Vsram[2] = 16;
		GenesisVdpState ppuState = state;
		GenesisVdpTools tools(nullptr, nullptr, nullptr);

		GetTilemapOptions options = {};
		options.Layer = 0;
		options.DisplayMode = TilemapDisplayMode::Default;
		DebugTilemapTileInfo info = tools.GetTilemapTileInfo(16, 0, vram, options, (BaseState&)state, (BaseState&)ppuState);

		EXPECT_EQ(info.Column, 2);
		EXPECT_EQ(info.Row, 2);
		EXPECT_EQ(info.TileMapAddress, kRow2Col2Addr);
		EXPECT_EQ(info.TileIndex, kTileRow2Col2 & 0x07FF);
	}

	TEST(GenesisVdpObjectRunSafetyTests, DebugTilemapTileInfoUsesEffectiveScrolledXForPerColumnVerticalScrollPlaneA) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		vdp.WriteControlPort(0x8200);
		vdp.WriteControlPort(0x8B07);
		vdp.WriteControlPort(0x8D01);

		uint8_t* vram = vdp.GetVramPointer();
		memset(vram, 0, 0x10000);

		// Line 0 H-scroll: plane A = 16, plane B = 0.
		vram[0x0400] = 0x00;
		vram[0x0401] = 0x10;
		vram[0x0402] = 0x00;
		vram[0x0403] = 0x00;

		constexpr uint16_t kTileRow0Col2 = 0x0006;
		constexpr uint16_t kTileRow2Col2 = 0x0144;
		constexpr uint16_t kRow0Col2Addr = 0x0004;
		constexpr uint16_t kRow2Col2Addr = 0x0084;
		vram[kRow0Col2Addr + 0] = (uint8_t)((kTileRow0Col2 >> 8) & 0xFFu);
		vram[kRow0Col2Addr + 1] = (uint8_t)(kTileRow0Col2 & 0xFFu);
		vram[kRow2Col2Addr + 0] = (uint8_t)((kTileRow2Col2 >> 8) & 0xFFu);
		vram[kRow2Col2Addr + 1] = (uint8_t)(kTileRow2Col2 & 0xFFu);

		GenesisVdpState state = vdp.GetState();
		state.Vsram[0] = 0;
		state.Vsram[2] = 16;
		GenesisVdpState ppuState = state;
		GenesisVdpTools tools(nullptr, nullptr, nullptr);

		GetTilemapOptions options = {};
		options.Layer = 0;
		options.DisplayMode = TilemapDisplayMode::Default;
		DebugTilemapTileInfo info = tools.GetTilemapTileInfo(0, 0, vram, options, (BaseState&)state, (BaseState&)ppuState);

		EXPECT_EQ(info.Column, 2);
		EXPECT_EQ(info.Row, 2);
		EXPECT_EQ(info.TileMapAddress, kRow2Col2Addr);
		EXPECT_EQ(info.TileIndex, kTileRow2Col2 & 0x07FF);
	}

	TEST(GenesisVdpObjectRunSafetyTests, DebugTilemapTileInfoUsesEffectiveScrolledXForPerColumnVerticalScrollPlaneB) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		vdp.WriteControlPort(0x8400);
		vdp.WriteControlPort(0x8B07);
		vdp.WriteControlPort(0x8D01);

		uint8_t* vram = vdp.GetVramPointer();
		memset(vram, 0, 0x10000);

		// Line 0 H-scroll: plane A = 0, plane B = 16.
		vram[0x0400] = 0x00;
		vram[0x0401] = 0x00;
		vram[0x0402] = 0x00;
		vram[0x0403] = 0x10;

		constexpr uint16_t kTileRow0Col2 = 0x0012;
		constexpr uint16_t kTileRow2Col2 = 0x0177;
		constexpr uint16_t kRow0Col2Addr = 0x0004;
		constexpr uint16_t kRow2Col2Addr = 0x0084;
		vram[kRow0Col2Addr + 0] = (uint8_t)((kTileRow0Col2 >> 8) & 0xFFu);
		vram[kRow0Col2Addr + 1] = (uint8_t)(kTileRow0Col2 & 0xFFu);
		vram[kRow2Col2Addr + 0] = (uint8_t)((kTileRow2Col2 >> 8) & 0xFFu);
		vram[kRow2Col2Addr + 1] = (uint8_t)(kTileRow2Col2 & 0xFFu);

		GenesisVdpState state = vdp.GetState();
		state.Vsram[1] = 0;
		state.Vsram[3] = 16;
		GenesisVdpState ppuState = state;
		GenesisVdpTools tools(nullptr, nullptr, nullptr);

		GetTilemapOptions options = {};
		options.Layer = 1;
		options.DisplayMode = TilemapDisplayMode::Default;
		DebugTilemapTileInfo info = tools.GetTilemapTileInfo(0, 0, vram, options, (BaseState&)state, (BaseState&)ppuState);

		EXPECT_EQ(info.Column, 2);
		EXPECT_EQ(info.Row, 2);
		EXPECT_EQ(info.TileMapAddress, kRow2Col2Addr);
		EXPECT_EQ(info.TileIndex, kTileRow2Col2 & 0x07FF);
	}

	TEST(GenesisVdpObjectRunSafetyTests, DebugTilemapTileInfoPreservesFlipPriorityAndPixelDataUnderScrollPlaneA) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		vdp.WriteControlPort(0x8200);
		vdp.WriteControlPort(0x8B07);
		vdp.WriteControlPort(0x8D01);

		uint8_t* vram = vdp.GetVramPointer();
		memset(vram, 0, 0x10000);

		// Line 0 H-scroll: plane A = 16.
		vram[0x0400] = 0x00;
		vram[0x0401] = 0x10;
		// Line 2 H-scroll: plane A = 16 (sample uses y=2).
		vram[0x0408] = 0x00;
		vram[0x0409] = 0x10;

		constexpr uint16_t kTileIndex = 0x0055;
		constexpr uint16_t kEntry = 0xF855;
		constexpr uint16_t kEntryAddr = 0x0084;
		vram[kEntryAddr + 0] = (uint8_t)((kEntry >> 8) & 0xFFu);
		vram[kEntryAddr + 1] = (uint8_t)(kEntry & 0xFFu);

		// Row 5, pixel 6 (high nibble of byte 3) should be selected after h/v flip at sample (x=1,y=2).
		uint32_t tileBase = (uint32_t)kTileIndex * 32u;
		vram[(tileBase + 5u * 4u + 3u) & 0xFFFFu] = 0xB0;

		GenesisVdpState state = vdp.GetState();
		state.Vsram[2] = 16;
		GenesisVdpState ppuState = state;
		GenesisVdpTools tools(nullptr, nullptr, nullptr);

		GetTilemapOptions options = {};
		options.Layer = 0;
		options.DisplayMode = TilemapDisplayMode::Default;
		DebugTilemapTileInfo info = tools.GetTilemapTileInfo(1, 2, vram, options, (BaseState&)state, (BaseState&)ppuState);

		EXPECT_EQ(info.Column, 2);
		EXPECT_EQ(info.Row, 2);
		EXPECT_EQ(info.TileMapAddress, kEntryAddr);
		EXPECT_EQ(info.TileIndex, kTileIndex);
		EXPECT_EQ(info.PaletteIndex, 3);
		EXPECT_EQ(info.HighPriority, NullableBoolean::True);
		EXPECT_EQ(info.HorizontalMirroring, NullableBoolean::True);
		EXPECT_EQ(info.VerticalMirroring, NullableBoolean::True);
		EXPECT_EQ(info.PixelData, 0x0b);
	}

	TEST(GenesisVdpObjectRunSafetyTests, DebugTilemapTileInfoPreservesFlipPriorityAndPixelDataUnderScrollPlaneB) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		vdp.WriteControlPort(0x8400);
		vdp.WriteControlPort(0x8B07);
		vdp.WriteControlPort(0x8D01);

		uint8_t* vram = vdp.GetVramPointer();
		memset(vram, 0, 0x10000);

		// Line 0 H-scroll: plane B = 16.
		vram[0x0402] = 0x00;
		vram[0x0403] = 0x10;
		// Line 2 H-scroll: plane B = 16 (sample uses y=2).
		vram[0x040A] = 0x00;
		vram[0x040B] = 0x10;

		constexpr uint16_t kTileIndex = 0x0034;
		constexpr uint16_t kEntry = 0xF834;
		constexpr uint16_t kEntryAddr = 0x0084;
		vram[kEntryAddr + 0] = (uint8_t)((kEntry >> 8) & 0xFFu);
		vram[kEntryAddr + 1] = (uint8_t)(kEntry & 0xFFu);

		uint32_t tileBase = (uint32_t)kTileIndex * 32u;
		vram[(tileBase + 5u * 4u + 3u) & 0xFFFFu] = 0xC0;

		GenesisVdpState state = vdp.GetState();
		state.Vsram[3] = 16;
		GenesisVdpState ppuState = state;
		GenesisVdpTools tools(nullptr, nullptr, nullptr);

		GetTilemapOptions options = {};
		options.Layer = 1;
		options.DisplayMode = TilemapDisplayMode::Default;
		DebugTilemapTileInfo info = tools.GetTilemapTileInfo(1, 2, vram, options, (BaseState&)state, (BaseState&)ppuState);

		EXPECT_EQ(info.Column, 2);
		EXPECT_EQ(info.Row, 2);
		EXPECT_EQ(info.TileMapAddress, kEntryAddr);
		EXPECT_EQ(info.TileIndex, kTileIndex);
		EXPECT_EQ(info.PaletteIndex, 3);
		EXPECT_EQ(info.HighPriority, NullableBoolean::True);
		EXPECT_EQ(info.HorizontalMirroring, NullableBoolean::True);
		EXPECT_EQ(info.VerticalMirroring, NullableBoolean::True);
		EXPECT_EQ(info.PixelData, 0x0c);
	}

	TEST(GenesisVdpObjectRunSafetyTests, DebugTilemapTileInfoUsesSampledYForLineScrollSelection) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		vdp.WriteControlPort(0x8200);
		vdp.WriteControlPort(0x8B03);
		vdp.WriteControlPort(0x8D01);

		uint8_t* vram = vdp.GetVramPointer();
		memset(vram, 0, 0x10000);

		// H-scroll table at $0400: line 0 uses scroll 0, line 1 uses scroll 8.
		vram[0x0400] = 0x00;
		vram[0x0401] = 0x00;
		vram[0x0404] = 0x00;
		vram[0x0405] = 0x08;

		constexpr uint16_t kTileCol0 = 0x0009;
		constexpr uint16_t kTileCol1 = 0x012a;
		vram[0x0000] = (uint8_t)((kTileCol0 >> 8) & 0xFFu);
		vram[0x0001] = (uint8_t)(kTileCol0 & 0xFFu);
		vram[0x0002] = (uint8_t)((kTileCol1 >> 8) & 0xFFu);
		vram[0x0003] = (uint8_t)(kTileCol1 & 0xFFu);

		GenesisVdpState state = vdp.GetState();
		state.VCounter = 0;
		GenesisVdpState ppuState = state;
		GenesisVdpTools tools(nullptr, nullptr, nullptr);

		GetTilemapOptions options = {};
		options.Layer = 0;
		options.DisplayMode = TilemapDisplayMode::Default;
		DebugTilemapTileInfo info = tools.GetTilemapTileInfo(0, 1, vram, options, (BaseState&)state, (BaseState&)ppuState);

		EXPECT_EQ(info.Column, 1);
		EXPECT_EQ(info.Row, 0);
		EXPECT_EQ(info.TileMapAddress, 0x0002);
		EXPECT_EQ(info.TileIndex, kTileCol1 & 0x07FF);
	}

	TEST(GenesisVdpObjectRunSafetyTests, DebugTilemapTileInfoUsesSampledYGroupForCellScrollSelection) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		vdp.WriteControlPort(0x8200);
		vdp.WriteControlPort(0x8B02);
		vdp.WriteControlPort(0x8D01);

		uint8_t* vram = vdp.GetVramPointer();
		memset(vram, 0, 0x10000);

		// Cell mode: group 0 at $0400 uses scroll 0, group 8+ at $0420 uses scroll 8.
		vram[0x0400] = 0x00;
		vram[0x0401] = 0x00;
		vram[0x0420] = 0x00;
		vram[0x0421] = 0x08;

		constexpr uint16_t kTileCol0 = 0x0010;
		constexpr uint16_t kTileCol1 = 0x0133;
		vram[0x0000] = (uint8_t)((kTileCol0 >> 8) & 0xFFu);
		vram[0x0001] = (uint8_t)(kTileCol0 & 0xFFu);
		vram[0x0002] = (uint8_t)((kTileCol1 >> 8) & 0xFFu);
		vram[0x0003] = (uint8_t)(kTileCol1 & 0xFFu);
		// Row 1, column 1 is selected after sampled y/group scroll in this test.
		vram[0x0042] = (uint8_t)((kTileCol1 >> 8) & 0xFFu);
		vram[0x0043] = (uint8_t)(kTileCol1 & 0xFFu);

		GenesisVdpState state = vdp.GetState();
		state.VCounter = 0;
		GenesisVdpState ppuState = state;
		GenesisVdpTools tools(nullptr, nullptr, nullptr);

		GetTilemapOptions options = {};
		options.Layer = 0;
		options.DisplayMode = TilemapDisplayMode::Default;
		DebugTilemapTileInfo info = tools.GetTilemapTileInfo(0, 9, vram, options, (BaseState&)state, (BaseState&)ppuState);

		EXPECT_EQ(info.Column, 1);
		EXPECT_EQ(info.Row, 1);
		EXPECT_EQ(info.TileMapAddress, 0x0042);
		EXPECT_EQ(info.TileIndex, kTileCol1 & 0x07FF);
	}

	TEST(GenesisVdpObjectRunSafetyTests, DebugTilemapTileInfoMatchesTilemapCoordinatesUnderLineScroll) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		vdp.WriteControlPort(0x8200);
		vdp.WriteControlPort(0x8B03);
		vdp.WriteControlPort(0x8D01);

		uint8_t* vram = vdp.GetVramPointer();
		memset(vram, 0, 0x10000);

		// Line 1 H-scroll: plane A = 8.
		vram[0x0404] = 0x00;
		vram[0x0405] = 0x08;

		constexpr uint16_t kEntry = 0x0123;
		vram[0x0002] = (uint8_t)((kEntry >> 8) & 0xFFu);
		vram[0x0003] = (uint8_t)(kEntry & 0xFFu);

		GenesisVdpState state = vdp.GetState();
		state.VCounter = 1;
		GenesisVdpState ppuState = state;
		GenesisVdpTools tools(nullptr, nullptr, nullptr);

		std::array<uint32_t, 64> palette = {};
		BuildArgbPaletteFromCram(vdp, palette);

		GetTilemapOptions mapOptions = {};
		mapOptions.Layer = 0;
		mapOptions.DisplayMode = TilemapDisplayMode::Default;
		FrameInfo mapSize = tools.GetTilemapSize(mapOptions, (BaseState&)state);
		std::vector<uint32_t> mapBuffer(mapSize.Width * mapSize.Height);
		DebugTilemapInfo mapInfo = tools.GetTilemap(mapOptions, (BaseState&)state, (BaseState&)ppuState, vram, palette.data(), mapBuffer.data());

		constexpr uint32_t kSampleX = 0;
		constexpr uint32_t kSampleY = 1;
		DebugTilemapTileInfo tileInfo = tools.GetTilemapTileInfo(kSampleX, kSampleY, vram, mapOptions, (BaseState&)state, (BaseState&)ppuState);

		uint32_t expectedCol = ((kSampleX + mapInfo.ScrollX) % mapSize.Width) / 8u;
		uint32_t expectedRow = ((kSampleY + mapInfo.ScrollY) % mapSize.Height) / 8u;
		uint32_t expectedAddr = ((expectedRow * (mapSize.Width / 8u) + expectedCol) * 2u) & 0xFFFFu;
		uint16_t expectedEntry = (uint16_t)(((uint16_t)vram[expectedAddr] << 8) | vram[(expectedAddr + 1u) & 0xFFFFu]);

		EXPECT_EQ((uint32_t)tileInfo.Column, expectedCol);
		EXPECT_EQ((uint32_t)tileInfo.Row, expectedRow);
		EXPECT_EQ((uint32_t)tileInfo.TileMapAddress, expectedAddr);
		EXPECT_EQ((uint32_t)tileInfo.TileIndex, (expectedEntry & 0x07FFu));
	}

	TEST(GenesisVdpObjectRunSafetyTests, DebugTilemapTileInfoMatchesTilemapCoordinatesUnderCellScroll) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		vdp.WriteControlPort(0x8200);
		vdp.WriteControlPort(0x8B02);
		vdp.WriteControlPort(0x8D01);

		uint8_t* vram = vdp.GetVramPointer();
		memset(vram, 0, 0x10000);

		// Cell group for lines 8-15 uses scroll 8.
		vram[0x0420] = 0x00;
		vram[0x0421] = 0x08;

		constexpr uint16_t kEntry = 0x0166;
		vram[0x0042] = (uint8_t)((kEntry >> 8) & 0xFFu);
		vram[0x0043] = (uint8_t)(kEntry & 0xFFu);

		GenesisVdpState state = vdp.GetState();
		state.VCounter = 9;
		GenesisVdpState ppuState = state;
		GenesisVdpTools tools(nullptr, nullptr, nullptr);

		std::array<uint32_t, 64> palette = {};
		BuildArgbPaletteFromCram(vdp, palette);

		GetTilemapOptions mapOptions = {};
		mapOptions.Layer = 0;
		mapOptions.DisplayMode = TilemapDisplayMode::Default;
		FrameInfo mapSize = tools.GetTilemapSize(mapOptions, (BaseState&)state);
		std::vector<uint32_t> mapBuffer(mapSize.Width * mapSize.Height);
		DebugTilemapInfo mapInfo = tools.GetTilemap(mapOptions, (BaseState&)state, (BaseState&)ppuState, vram, palette.data(), mapBuffer.data());

		constexpr uint32_t kSampleX = 0;
		constexpr uint32_t kSampleY = 9;
		DebugTilemapTileInfo tileInfo = tools.GetTilemapTileInfo(kSampleX, kSampleY, vram, mapOptions, (BaseState&)state, (BaseState&)ppuState);

		uint32_t expectedCol = ((kSampleX + mapInfo.ScrollX) % mapSize.Width) / 8u;
		uint32_t expectedRow = ((kSampleY + mapInfo.ScrollY) % mapSize.Height) / 8u;
		uint32_t expectedAddr = ((expectedRow * (mapSize.Width / 8u) + expectedCol) * 2u) & 0xFFFFu;
		uint16_t expectedEntry = (uint16_t)(((uint16_t)vram[expectedAddr] << 8) | vram[(expectedAddr + 1u) & 0xFFFFu]);

		EXPECT_EQ((uint32_t)tileInfo.Column, expectedCol);
		EXPECT_EQ((uint32_t)tileInfo.Row, expectedRow);
		EXPECT_EQ((uint32_t)tileInfo.TileMapAddress, expectedAddr);
		EXPECT_EQ((uint32_t)tileInfo.TileIndex, (expectedEntry & 0x07FFu));
	}

	TEST(GenesisVdpObjectRunSafetyTests, DebugTilemapTileInfoMaintainsParityAcrossH32ToH40LineScrollTransition) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		vdp.WriteControlPort(0x8200);
		vdp.WriteControlPort(0x8B03);
		vdp.WriteControlPort(0x8D01);
		vdp.WriteControlPort(0x8C80);

		uint8_t* vram = vdp.GetVramPointer();
		memset(vram, 0, 0x10000);

		// Active line 5 uses scroll 8 (entry at 0x0400 + line*4 + planeA*2).
		vram[0x0414] = 0x00;
		vram[0x0415] = 0x08;

		GenesisVdpTools tools(nullptr, nullptr, nullptr);
		GetTilemapOptions mapOptions = {};
		mapOptions.Layer = 0;
		mapOptions.DisplayMode = TilemapDisplayMode::Default;

		std::array<uint32_t, 64> palette = {};
		BuildArgbPaletteFromCram(vdp, palette);

		constexpr uint32_t kSampleX = 300;
		constexpr uint32_t kSampleY = 5;
		constexpr uint16_t kEntryH32 = 0x0125;
		constexpr uint16_t kEntryH40 = 0x016a;

		GenesisVdpState stateH32 = vdp.GetState();
		stateH32.VCounter = (uint16_t)kSampleY;
		GenesisVdpState ppuStateH32 = stateH32;

		FrameInfo mapSizeH32 = tools.GetTilemapSize(mapOptions, (BaseState&)stateH32);
		std::vector<uint32_t> mapBufferH32(mapSizeH32.Width * mapSizeH32.Height);
		DebugTilemapInfo mapInfoH32 = tools.GetTilemap(mapOptions, (BaseState&)stateH32, (BaseState&)ppuStateH32, vram, palette.data(), mapBufferH32.data());

		uint32_t expectedColH32 = ((kSampleX + mapInfoH32.ScrollX) % mapSizeH32.Width) / 8u;
		uint32_t expectedRowH32 = ((kSampleY + mapInfoH32.ScrollY) % mapSizeH32.Height) / 8u;
		uint32_t expectedAddrH32 = ((expectedRowH32 * (mapSizeH32.Width / 8u) + expectedColH32) * 2u) & 0xFFFFu;
		vram[expectedAddrH32] = (uint8_t)((kEntryH32 >> 8) & 0xFFu);
		vram[(expectedAddrH32 + 1u) & 0xFFFFu] = (uint8_t)(kEntryH32 & 0xFFu);

		DebugTilemapTileInfo tileInfoH32 = tools.GetTilemapTileInfo(kSampleX, kSampleY, vram, mapOptions, (BaseState&)stateH32, (BaseState&)ppuStateH32);
		EXPECT_EQ((uint32_t)tileInfoH32.Column, expectedColH32);
		EXPECT_EQ((uint32_t)tileInfoH32.Row, expectedRowH32);
		EXPECT_EQ((uint32_t)tileInfoH32.TileMapAddress, expectedAddrH32);
		EXPECT_EQ((uint32_t)tileInfoH32.TileIndex, (kEntryH32 & 0x07FFu));

		vdp.WriteControlPort(0x8C81);

		GenesisVdpState stateH40 = vdp.GetState();
		stateH40.VCounter = (uint16_t)kSampleY;
		GenesisVdpState ppuStateH40 = stateH40;

		FrameInfo mapSizeH40 = tools.GetTilemapSize(mapOptions, (BaseState&)stateH40);
		std::vector<uint32_t> mapBufferH40(mapSizeH40.Width * mapSizeH40.Height);
		DebugTilemapInfo mapInfoH40 = tools.GetTilemap(mapOptions, (BaseState&)stateH40, (BaseState&)ppuStateH40, vram, palette.data(), mapBufferH40.data());

		uint32_t expectedColH40 = ((kSampleX + mapInfoH40.ScrollX) % mapSizeH40.Width) / 8u;
		uint32_t expectedRowH40 = ((kSampleY + mapInfoH40.ScrollY) % mapSizeH40.Height) / 8u;
		uint32_t expectedAddrH40 = ((expectedRowH40 * (mapSizeH40.Width / 8u) + expectedColH40) * 2u) & 0xFFFFu;
		vram[expectedAddrH40] = (uint8_t)((kEntryH40 >> 8) & 0xFFu);
		vram[(expectedAddrH40 + 1u) & 0xFFFFu] = (uint8_t)(kEntryH40 & 0xFFu);

		DebugTilemapTileInfo tileInfoH40 = tools.GetTilemapTileInfo(kSampleX, kSampleY, vram, mapOptions, (BaseState&)stateH40, (BaseState&)ppuStateH40);
		EXPECT_EQ((uint32_t)tileInfoH40.Column, expectedColH40);
		EXPECT_EQ((uint32_t)tileInfoH40.Row, expectedRowH40);
		EXPECT_EQ((uint32_t)tileInfoH40.TileMapAddress, expectedAddrH40);
		EXPECT_EQ((uint32_t)tileInfoH40.TileIndex, (kEntryH40 & 0x07FFu));
	}

	TEST(GenesisVdpObjectRunSafetyTests, DebugTilemapTileInfoMaintainsParityAcrossH40ToH32CellScrollTransition) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		vdp.WriteControlPort(0x8200);
		vdp.WriteControlPort(0x8B02);
		vdp.WriteControlPort(0x8D01);
		vdp.WriteControlPort(0x8C81);

		uint8_t* vram = vdp.GetVramPointer();
		memset(vram, 0, 0x10000);

		// Active line group 24 uses scroll 8 (entry at 0x0400 + (line & ~7)*4 + planeA*2).
		vram[0x0460] = 0x00;
		vram[0x0461] = 0x08;

		GenesisVdpTools tools(nullptr, nullptr, nullptr);
		GetTilemapOptions mapOptions = {};
		mapOptions.Layer = 0;
		mapOptions.DisplayMode = TilemapDisplayMode::Default;

		std::array<uint32_t, 64> palette = {};
		BuildArgbPaletteFromCram(vdp, palette);

		constexpr uint32_t kSampleX = 300;
		constexpr uint32_t kSampleY = 24;
		constexpr uint16_t kEntryH40 = 0x0171;
		constexpr uint16_t kEntryH32 = 0x0139;

		GenesisVdpState stateH40 = vdp.GetState();
		stateH40.VCounter = (uint16_t)kSampleY;
		GenesisVdpState ppuStateH40 = stateH40;

		FrameInfo mapSizeH40 = tools.GetTilemapSize(mapOptions, (BaseState&)stateH40);
		std::vector<uint32_t> mapBufferH40(mapSizeH40.Width * mapSizeH40.Height);
		DebugTilemapInfo mapInfoH40 = tools.GetTilemap(mapOptions, (BaseState&)stateH40, (BaseState&)ppuStateH40, vram, palette.data(), mapBufferH40.data());

		uint32_t expectedColH40 = ((kSampleX + mapInfoH40.ScrollX) % mapSizeH40.Width) / 8u;
		uint32_t expectedRowH40 = ((kSampleY + mapInfoH40.ScrollY) % mapSizeH40.Height) / 8u;
		uint32_t expectedAddrH40 = ((expectedRowH40 * (mapSizeH40.Width / 8u) + expectedColH40) * 2u) & 0xFFFFu;
		vram[expectedAddrH40] = (uint8_t)((kEntryH40 >> 8) & 0xFFu);
		vram[(expectedAddrH40 + 1u) & 0xFFFFu] = (uint8_t)(kEntryH40 & 0xFFu);

		DebugTilemapTileInfo tileInfoH40 = tools.GetTilemapTileInfo(kSampleX, kSampleY, vram, mapOptions, (BaseState&)stateH40, (BaseState&)ppuStateH40);
		EXPECT_EQ((uint32_t)tileInfoH40.Column, expectedColH40);
		EXPECT_EQ((uint32_t)tileInfoH40.Row, expectedRowH40);
		EXPECT_EQ((uint32_t)tileInfoH40.TileMapAddress, expectedAddrH40);
		EXPECT_EQ((uint32_t)tileInfoH40.TileIndex, (kEntryH40 & 0x07FFu));

		vdp.WriteControlPort(0x8C80);

		GenesisVdpState stateH32 = vdp.GetState();
		stateH32.VCounter = (uint16_t)kSampleY;
		GenesisVdpState ppuStateH32 = stateH32;

		FrameInfo mapSizeH32 = tools.GetTilemapSize(mapOptions, (BaseState&)stateH32);
		std::vector<uint32_t> mapBufferH32(mapSizeH32.Width * mapSizeH32.Height);
		DebugTilemapInfo mapInfoH32 = tools.GetTilemap(mapOptions, (BaseState&)stateH32, (BaseState&)ppuStateH32, vram, palette.data(), mapBufferH32.data());

		uint32_t expectedColH32 = ((kSampleX + mapInfoH32.ScrollX) % mapSizeH32.Width) / 8u;
		uint32_t expectedRowH32 = ((kSampleY + mapInfoH32.ScrollY) % mapSizeH32.Height) / 8u;
		uint32_t expectedAddrH32 = ((expectedRowH32 * (mapSizeH32.Width / 8u) + expectedColH32) * 2u) & 0xFFFFu;
		vram[expectedAddrH32] = (uint8_t)((kEntryH32 >> 8) & 0xFFu);
		vram[(expectedAddrH32 + 1u) & 0xFFFFu] = (uint8_t)(kEntryH32 & 0xFFu);

		DebugTilemapTileInfo tileInfoH32 = tools.GetTilemapTileInfo(kSampleX, kSampleY, vram, mapOptions, (BaseState&)stateH32, (BaseState&)ppuStateH32);
		EXPECT_EQ((uint32_t)tileInfoH32.Column, expectedColH32);
		EXPECT_EQ((uint32_t)tileInfoH32.Row, expectedRowH32);
		EXPECT_EQ((uint32_t)tileInfoH32.TileMapAddress, expectedAddrH32);
		EXPECT_EQ((uint32_t)tileInfoH32.TileIndex, (kEntryH32 & 0x07FFu));
	}

	TEST(GenesisVdpObjectRunSafetyTests, DebugTilemapTileInfoMaintainsParityAcrossH32ToH40PerColumnVScrollTransitionPlaneA) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		vdp.WriteControlPort(0x8200);
		vdp.WriteControlPort(0x8B04);
		vdp.WriteControlPort(0x8C80);

		uint8_t* vram = vdp.GetVramPointer();
		memset(vram, 0, 0x10000);

		GenesisVdpTools tools(nullptr, nullptr, nullptr);
		GetTilemapOptions mapOptions = {};
		mapOptions.Layer = 0;
		mapOptions.DisplayMode = TilemapDisplayMode::Default;

		std::array<uint32_t, 64> palette = {};
		BuildArgbPaletteFromCram(vdp, palette);

		constexpr uint32_t kSampleX = 0;
		constexpr uint32_t kSampleY = 0;
		constexpr uint16_t kEntryH32 = 0x0135;
		constexpr uint16_t kEntryH40 = 0x016e;

		GenesisVdpState stateH32 = vdp.GetState();
		stateH32.VCounter = (uint16_t)kSampleY;
		stateH32.Vsram[0] = 0x0030;
		stateH32.Vsram[1] = 0x0000;
		stateH32.Vsram[2] = 0x0010;
		stateH32.Vsram[3] = 0x0020;
		stateH32.Vsram[4] = 0x0030;
		stateH32.Vsram[5] = 0x0040;
		GenesisVdpState ppuStateH32 = stateH32;

		FrameInfo mapSizeH32 = tools.GetTilemapSize(mapOptions, (BaseState&)stateH32);
		std::vector<uint32_t> mapBufferH32(mapSizeH32.Width * mapSizeH32.Height);
		DebugTilemapInfo mapInfoH32 = tools.GetTilemap(mapOptions, (BaseState&)stateH32, (BaseState&)ppuStateH32, vram, palette.data(), mapBufferH32.data());

		uint32_t expectedColH32 = ((kSampleX + mapInfoH32.ScrollX) % mapSizeH32.Width) / 8u;
		uint32_t expectedRowH32 = ((kSampleY + mapInfoH32.ScrollY) % mapSizeH32.Height) / 8u;
		uint32_t expectedAddrH32 = ((expectedRowH32 * (mapSizeH32.Width / 8u) + expectedColH32) * 2u) & 0xFFFFu;
		vram[expectedAddrH32] = (uint8_t)((kEntryH32 >> 8) & 0xFFu);
		vram[(expectedAddrH32 + 1u) & 0xFFFFu] = (uint8_t)(kEntryH32 & 0xFFu);

		DebugTilemapTileInfo tileInfoH32 = tools.GetTilemapTileInfo(kSampleX, kSampleY, vram, mapOptions, (BaseState&)stateH32, (BaseState&)ppuStateH32);
		EXPECT_EQ((uint32_t)tileInfoH32.Column, expectedColH32);
		EXPECT_EQ((uint32_t)tileInfoH32.Row, expectedRowH32);
		EXPECT_EQ((uint32_t)tileInfoH32.TileMapAddress, expectedAddrH32);
		EXPECT_EQ((uint32_t)tileInfoH32.TileIndex, (kEntryH32 & 0x07FFu));

		vdp.WriteControlPort(0x8C81);

		GenesisVdpState stateH40 = vdp.GetState();
		stateH40.VCounter = (uint16_t)kSampleY;
		stateH40.Vsram[0] = stateH32.Vsram[0];
		stateH40.Vsram[1] = stateH32.Vsram[1];
		stateH40.Vsram[2] = stateH32.Vsram[2];
		stateH40.Vsram[3] = stateH32.Vsram[3];
		stateH40.Vsram[4] = stateH32.Vsram[4];
		stateH40.Vsram[5] = stateH32.Vsram[5];
		GenesisVdpState ppuStateH40 = stateH40;

		FrameInfo mapSizeH40 = tools.GetTilemapSize(mapOptions, (BaseState&)stateH40);
		std::vector<uint32_t> mapBufferH40(mapSizeH40.Width * mapSizeH40.Height);
		DebugTilemapInfo mapInfoH40 = tools.GetTilemap(mapOptions, (BaseState&)stateH40, (BaseState&)ppuStateH40, vram, palette.data(), mapBufferH40.data());

		uint32_t expectedColH40 = ((kSampleX + mapInfoH40.ScrollX) % mapSizeH40.Width) / 8u;
		uint32_t expectedRowH40 = ((kSampleY + mapInfoH40.ScrollY) % mapSizeH40.Height) / 8u;
		uint32_t expectedAddrH40 = ((expectedRowH40 * (mapSizeH40.Width / 8u) + expectedColH40) * 2u) & 0xFFFFu;
		vram[expectedAddrH40] = (uint8_t)((kEntryH40 >> 8) & 0xFFu);
		vram[(expectedAddrH40 + 1u) & 0xFFFFu] = (uint8_t)(kEntryH40 & 0xFFu);

		DebugTilemapTileInfo tileInfoH40 = tools.GetTilemapTileInfo(kSampleX, kSampleY, vram, mapOptions, (BaseState&)stateH40, (BaseState&)ppuStateH40);
		EXPECT_EQ((uint32_t)tileInfoH40.Column, expectedColH40);
		EXPECT_EQ((uint32_t)tileInfoH40.Row, expectedRowH40);
		EXPECT_EQ((uint32_t)tileInfoH40.TileMapAddress, expectedAddrH40);
		EXPECT_EQ((uint32_t)tileInfoH40.TileIndex, (kEntryH40 & 0x07FFu));
	}

	TEST(GenesisVdpObjectRunSafetyTests, DebugTilemapTileInfoMaintainsParityAcrossH40ToH32PerColumnVScrollTransitionPlaneB) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		vdp.WriteControlPort(0x8400);
		vdp.WriteControlPort(0x8B04);
		vdp.WriteControlPort(0x8C81);

		uint8_t* vram = vdp.GetVramPointer();
		memset(vram, 0, 0x10000);

		GenesisVdpTools tools(nullptr, nullptr, nullptr);
		GetTilemapOptions mapOptions = {};
		mapOptions.Layer = 1;
		mapOptions.DisplayMode = TilemapDisplayMode::Default;

		std::array<uint32_t, 64> palette = {};
		BuildArgbPaletteFromCram(vdp, palette);

		constexpr uint32_t kSampleX = 0;
		constexpr uint32_t kSampleY = 0;
		constexpr uint16_t kEntryH40 = 0x0157;
		constexpr uint16_t kEntryH32 = 0x011d;

		GenesisVdpState stateH40 = vdp.GetState();
		stateH40.VCounter = (uint16_t)kSampleY;
		stateH40.Vsram[0] = 0x0000;
		stateH40.Vsram[1] = 0x0040;
		stateH40.Vsram[2] = 0x0010;
		stateH40.Vsram[3] = 0x0020;
		stateH40.Vsram[4] = 0x0030;
		stateH40.Vsram[5] = 0x0040;
		GenesisVdpState ppuStateH40 = stateH40;

		FrameInfo mapSizeH40 = tools.GetTilemapSize(mapOptions, (BaseState&)stateH40);
		std::vector<uint32_t> mapBufferH40(mapSizeH40.Width * mapSizeH40.Height);
		DebugTilemapInfo mapInfoH40 = tools.GetTilemap(mapOptions, (BaseState&)stateH40, (BaseState&)ppuStateH40, vram, palette.data(), mapBufferH40.data());

		uint32_t expectedColH40 = ((kSampleX + mapInfoH40.ScrollX) % mapSizeH40.Width) / 8u;
		uint32_t expectedRowH40 = ((kSampleY + mapInfoH40.ScrollY) % mapSizeH40.Height) / 8u;
		uint32_t expectedAddrH40 = ((expectedRowH40 * (mapSizeH40.Width / 8u) + expectedColH40) * 2u) & 0xFFFFu;
		vram[expectedAddrH40] = (uint8_t)((kEntryH40 >> 8) & 0xFFu);
		vram[(expectedAddrH40 + 1u) & 0xFFFFu] = (uint8_t)(kEntryH40 & 0xFFu);

		DebugTilemapTileInfo tileInfoH40 = tools.GetTilemapTileInfo(kSampleX, kSampleY, vram, mapOptions, (BaseState&)stateH40, (BaseState&)ppuStateH40);
		EXPECT_EQ((uint32_t)tileInfoH40.Column, expectedColH40);
		EXPECT_EQ((uint32_t)tileInfoH40.Row, expectedRowH40);
		EXPECT_EQ((uint32_t)tileInfoH40.TileMapAddress, expectedAddrH40);
		EXPECT_EQ((uint32_t)tileInfoH40.TileIndex, (kEntryH40 & 0x07FFu));

		vdp.WriteControlPort(0x8C80);

		GenesisVdpState stateH32 = vdp.GetState();
		stateH32.VCounter = (uint16_t)kSampleY;
		stateH32.Vsram[0] = stateH40.Vsram[0];
		stateH32.Vsram[1] = stateH40.Vsram[1];
		stateH32.Vsram[2] = stateH40.Vsram[2];
		stateH32.Vsram[3] = stateH40.Vsram[3];
		stateH32.Vsram[4] = stateH40.Vsram[4];
		stateH32.Vsram[5] = stateH40.Vsram[5];
		GenesisVdpState ppuStateH32 = stateH32;

		FrameInfo mapSizeH32 = tools.GetTilemapSize(mapOptions, (BaseState&)stateH32);
		std::vector<uint32_t> mapBufferH32(mapSizeH32.Width * mapSizeH32.Height);
		DebugTilemapInfo mapInfoH32 = tools.GetTilemap(mapOptions, (BaseState&)stateH32, (BaseState&)ppuStateH32, vram, palette.data(), mapBufferH32.data());

		uint32_t expectedColH32 = ((kSampleX + mapInfoH32.ScrollX) % mapSizeH32.Width) / 8u;
		uint32_t expectedRowH32 = ((kSampleY + mapInfoH32.ScrollY) % mapSizeH32.Height) / 8u;
		uint32_t expectedAddrH32 = ((expectedRowH32 * (mapSizeH32.Width / 8u) + expectedColH32) * 2u) & 0xFFFFu;
		vram[expectedAddrH32] = (uint8_t)((kEntryH32 >> 8) & 0xFFu);
		vram[(expectedAddrH32 + 1u) & 0xFFFFu] = (uint8_t)(kEntryH32 & 0xFFu);

		DebugTilemapTileInfo tileInfoH32 = tools.GetTilemapTileInfo(kSampleX, kSampleY, vram, mapOptions, (BaseState&)stateH32, (BaseState&)ppuStateH32);
		EXPECT_EQ((uint32_t)tileInfoH32.Column, expectedColH32);
		EXPECT_EQ((uint32_t)tileInfoH32.Row, expectedRowH32);
		EXPECT_EQ((uint32_t)tileInfoH32.TileMapAddress, expectedAddrH32);
		EXPECT_EQ((uint32_t)tileInfoH32.TileIndex, (kEntryH32 & 0x07FFu));
	}

	TEST(GenesisVdpObjectRunSafetyTests, DebugTilemapReportsCellScrollUsingActiveLineGroup) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		vdp.WriteControlPort(0x8B02);
		vdp.WriteControlPort(0x8D00);

		uint8_t* vram = vdp.GetVramPointer();
		memset(vram, 0, 0x10000);
		// Cell mode uses (line & ~7) * 4; for line group starting at 8 this is 0x20.
		vram[0x20] = 0x00;
		vram[0x21] = 0x33;
		vram[0x22] = 0x00;
		vram[0x23] = 0x44;

		GenesisVdpState state = vdp.GetState();
		state.VCounter = 8;
		GenesisVdpState ppuState = state;
		GenesisVdpTools tools(nullptr, nullptr, nullptr);
		std::array<uint32_t, 64> palette = {};
		BuildArgbPaletteFromCram(vdp, palette);

		GetTilemapOptions planeAOptions = {};
		planeAOptions.Layer = 0;
		planeAOptions.DisplayMode = TilemapDisplayMode::Default;
		FrameInfo sizeA = tools.GetTilemapSize(planeAOptions, (BaseState&)state);
		std::vector<uint32_t> bufferA(sizeA.Width * sizeA.Height);
		DebugTilemapInfo infoA = tools.GetTilemap(planeAOptions, (BaseState&)state, (BaseState&)ppuState, vram, palette.data(), bufferA.data());

		GetTilemapOptions planeBOptions = {};
		planeBOptions.Layer = 1;
		planeBOptions.DisplayMode = TilemapDisplayMode::Default;
		FrameInfo sizeB = tools.GetTilemapSize(planeBOptions, (BaseState&)state);
		std::vector<uint32_t> bufferB(sizeB.Width * sizeB.Height);
		DebugTilemapInfo infoB = tools.GetTilemap(planeBOptions, (BaseState&)state, (BaseState&)ppuState, vram, palette.data(), bufferB.data());

		EXPECT_EQ(infoA.ScrollX, 0x33u);
		EXPECT_EQ(infoB.ScrollX, 0x44u);
	}

	TEST(GenesisVdpObjectRunSafetyTests, DebugTilemapReportsPerColumnVScrollForPlaneAAndB) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		vdp.WriteControlPort(0x8B04);

		GenesisVdpState state = vdp.GetState();
		state.Vsram[0] = 0x0055;
		state.Vsram[1] = 0x0066;
		GenesisVdpState ppuState = state;

		GenesisVdpTools tools(nullptr, nullptr, nullptr);
		uint8_t* vram = vdp.GetVramPointer();
		std::array<uint32_t, 64> palette = {};
		BuildArgbPaletteFromCram(vdp, palette);

		GetTilemapOptions planeAOptions = {};
		planeAOptions.Layer = 0;
		planeAOptions.DisplayMode = TilemapDisplayMode::Default;
		FrameInfo sizeA = tools.GetTilemapSize(planeAOptions, (BaseState&)state);
		std::vector<uint32_t> bufferA(sizeA.Width * sizeA.Height);
		DebugTilemapInfo infoA = tools.GetTilemap(planeAOptions, (BaseState&)state, (BaseState&)ppuState, vram, palette.data(), bufferA.data());

		GetTilemapOptions planeBOptions = {};
		planeBOptions.Layer = 1;
		planeBOptions.DisplayMode = TilemapDisplayMode::Default;
		FrameInfo sizeB = tools.GetTilemapSize(planeBOptions, (BaseState&)state);
		std::vector<uint32_t> bufferB(sizeB.Width * sizeB.Height);
		DebugTilemapInfo infoB = tools.GetTilemap(planeBOptions, (BaseState&)state, (BaseState&)ppuState, vram, palette.data(), bufferB.data());

		EXPECT_EQ(infoA.ScrollY, 0x55u);
		EXPECT_EQ(infoB.ScrollY, 0x66u);
	}

	TEST(GenesisVdpObjectRunSafetyTests, DebugTilemapUsesActiveScrollColumnForPerColumnVScroll) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		vdp.WriteControlPort(0x8B04);
		vdp.WriteControlPort(0x8D00);

		uint8_t* vram = vdp.GetVramPointer();
		memset(vram, 0, 0x10000);
		vram[0] = 0x00;
		vram[1] = 0x10;
		vram[2] = 0x00;
		vram[3] = 0x30;

		GenesisVdpState state = vdp.GetState();
		state.Vsram[2] = 0x0123;
		state.Vsram[7] = 0x0234;
		GenesisVdpState ppuState = state;

		GenesisVdpTools tools(nullptr, nullptr, nullptr);
		std::array<uint32_t, 64> palette = {};
		BuildArgbPaletteFromCram(vdp, palette);

		GetTilemapOptions planeAOptions = {};
		planeAOptions.Layer = 0;
		planeAOptions.DisplayMode = TilemapDisplayMode::Default;
		FrameInfo sizeA = tools.GetTilemapSize(planeAOptions, (BaseState&)state);
		std::vector<uint32_t> bufferA(sizeA.Width * sizeA.Height);
		DebugTilemapInfo infoA = tools.GetTilemap(planeAOptions, (BaseState&)state, (BaseState&)ppuState, vram, palette.data(), bufferA.data());

		GetTilemapOptions planeBOptions = {};
		planeBOptions.Layer = 1;
		planeBOptions.DisplayMode = TilemapDisplayMode::Default;
		FrameInfo sizeB = tools.GetTilemapSize(planeBOptions, (BaseState&)state);
		std::vector<uint32_t> bufferB(sizeB.Width * sizeB.Height);
		DebugTilemapInfo infoB = tools.GetTilemap(planeBOptions, (BaseState&)state, (BaseState&)ppuState, vram, palette.data(), bufferB.data());

		EXPECT_EQ(infoA.ScrollX, 0x10u);
		EXPECT_EQ(infoB.ScrollX, 0x30u);
		EXPECT_EQ(infoA.ScrollY, 0x123u);
		EXPECT_EQ(infoB.ScrollY, 0x234u);
	}

	TEST(GenesisVdpObjectRunSafetyTests, DebugTilemapWrapsPerColumnVScrollSelectionForHighScrollValues) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		vdp.WriteControlPort(0x8B04);
		vdp.WriteControlPort(0x8D00);

		uint8_t* vram = vdp.GetVramPointer();
		memset(vram, 0, 0x10000);
		vram[0] = 0x02;
		vram[1] = 0x00;
		vram[2] = 0x02;
		vram[3] = 0x10;

		GenesisVdpState state = vdp.GetState();
		state.Vsram[0] = 0x0099;
		state.Vsram[3] = 0x00aa;
		GenesisVdpState ppuState = state;

		GenesisVdpTools tools(nullptr, nullptr, nullptr);
		std::array<uint32_t, 64> palette = {};
		BuildArgbPaletteFromCram(vdp, palette);

		GetTilemapOptions planeAOptions = {};
		planeAOptions.Layer = 0;
		planeAOptions.DisplayMode = TilemapDisplayMode::Default;
		FrameInfo sizeA = tools.GetTilemapSize(planeAOptions, (BaseState&)state);
		std::vector<uint32_t> bufferA(sizeA.Width * sizeA.Height);
		DebugTilemapInfo infoA = tools.GetTilemap(planeAOptions, (BaseState&)state, (BaseState&)ppuState, vram, palette.data(), bufferA.data());

		GetTilemapOptions planeBOptions = {};
		planeBOptions.Layer = 1;
		planeBOptions.DisplayMode = TilemapDisplayMode::Default;
		FrameInfo sizeB = tools.GetTilemapSize(planeBOptions, (BaseState&)state);
		std::vector<uint32_t> bufferB(sizeB.Width * sizeB.Height);
		DebugTilemapInfo infoB = tools.GetTilemap(planeBOptions, (BaseState&)state, (BaseState&)ppuState, vram, palette.data(), bufferB.data());

		EXPECT_EQ(infoA.ScrollX, 0x200u);
		EXPECT_EQ(infoB.ScrollX, 0x210u);
		EXPECT_EQ(infoA.ScrollY, 0x99u);
		EXPECT_EQ(infoB.ScrollY, 0xaau);
	}

	TEST(GenesisVdpObjectRunSafetyTests, EarlierSpriteInChainWinsOverlapEvenWhenLaterHasHigherPriorityFlag) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		vdp.GetCramPointer()[0] = 0x0000;
		vdp.GetCramPointer()[17] = 0x000e;
		vdp.GetCramPointer()[33] = 0x00e0;
		ConfigureSpriteRenderMode(vdp, true, 0x00);

		uint8_t* vram = vdp.GetVramPointer();
		memset(vram, 0, 0x10000);
		constexpr uint16_t kTileIndex = 20;
		WriteSolidSpriteTileRow0(vram, kTileIndex, 1);

		constexpr uint16_t kSatBase = 0x0000;
		WriteSpriteSatEntry(vram, kSatBase, 0, 128, 1, 1, 1, (uint16_t)(0x2000 | kTileIndex), 128);
		WriteSpriteSatEntry(vram, kSatBase, 1, 128, 1, 1, 0, (uint16_t)(0xC000 | kTileIndex), 128);

		vdp.Run(488);

		GenesisVdpState state = vdp.GetState();
		GenesisVdpState ppuState = state;
		GenesisVdpTools tools(nullptr, nullptr, nullptr);
		GetSpritePreviewOptions options = {};
		options.Background = SpriteBackground::Gray;
		DebugSpritePreviewInfo previewInfo = tools.GetSpritePreviewInfo(options, (BaseState&)state, (BaseState&)ppuState);

		std::array<uint32_t, 64> palette = {};
		BuildArgbPaletteFromCram(vdp, palette);

		std::vector<DebugSpriteInfo> sprites(previewInfo.SpriteCount);
		std::vector<uint32_t> spritePreviews(previewInfo.SpriteCount * 128u * 128u);
		std::vector<uint32_t> screenPreview(previewInfo.Width * previewInfo.Height);

		tools.GetSpriteList(options, (BaseState&)state, (BaseState&)ppuState, vdp.GetVramPointer(), nullptr, palette.data(), sprites.data(), spritePreviews.data(), screenPreview.data());

		uint32_t previewRowStart = previewInfo.VisibleY * previewInfo.Width + previewInfo.VisibleX;
		EXPECT_EQ(screenPreview[previewRowStart], palette[17]);
		EXPECT_NE(screenPreview[previewRowStart], palette[33]);
	}

	TEST(GenesisVdpObjectRunSafetyTests, EarlierSpriteRemainsVisibleOnPartialOverlapEdge) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		vdp.GetCramPointer()[0] = 0x0000;
		vdp.GetCramPointer()[17] = 0x000e;
		vdp.GetCramPointer()[33] = 0x00e0;
		ConfigureSpriteRenderMode(vdp, true, 0x00);

		uint8_t* vram = vdp.GetVramPointer();
		memset(vram, 0, 0x10000);
		constexpr uint16_t kTileIndex = 20;
		WriteSolidSpriteTileRow0(vram, kTileIndex, 1);

		constexpr uint16_t kSatBase = 0x0000;
		WriteSpriteSatEntry(vram, kSatBase, 0, 128, 1, 1, 1, (uint16_t)(0x2000 | kTileIndex), 128);
		WriteSpriteSatEntry(vram, kSatBase, 1, 128, 1, 1, 0, (uint16_t)(0xC000 | kTileIndex), 132);

		vdp.Run(488);

		GenesisVdpState state = vdp.GetState();
		GenesisVdpState ppuState = state;
		GenesisVdpTools tools(nullptr, nullptr, nullptr);
		GetSpritePreviewOptions options = {};
		options.Background = SpriteBackground::Gray;
		DebugSpritePreviewInfo previewInfo = tools.GetSpritePreviewInfo(options, (BaseState&)state, (BaseState&)ppuState);

		std::array<uint32_t, 64> palette = {};
		BuildArgbPaletteFromCram(vdp, palette);
		std::vector<DebugSpriteInfo> sprites(previewInfo.SpriteCount);
		std::vector<uint32_t> spritePreviews(previewInfo.SpriteCount * 128u * 128u);
		std::vector<uint32_t> screenPreview(previewInfo.Width * previewInfo.Height);
		tools.GetSpriteList(options, (BaseState&)state, (BaseState&)ppuState, vdp.GetVramPointer(), nullptr, palette.data(), sprites.data(), spritePreviews.data(), screenPreview.data());

		uint32_t previewRowStart = previewInfo.VisibleY * previewInfo.Width + previewInfo.VisibleX;
		EXPECT_EQ(screenPreview[previewRowStart + 4], palette[17]);
	}

	TEST(GenesisVdpObjectRunSafetyTests, NonOverlappingSpritesKeepIndependentColors) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		vdp.GetCramPointer()[0] = 0x0000;
		vdp.GetCramPointer()[17] = 0x000e;
		vdp.GetCramPointer()[33] = 0x00e0;
		ConfigureSpriteRenderMode(vdp, true, 0x00);

		uint8_t* vram = vdp.GetVramPointer();
		memset(vram, 0, 0x10000);
		constexpr uint16_t kTileIndex = 20;
		WriteSolidSpriteTileRow0(vram, kTileIndex, 1);

		constexpr uint16_t kSatBase = 0x0000;
		WriteSpriteSatEntry(vram, kSatBase, 0, 128, 1, 1, 1, (uint16_t)(0x2000 | kTileIndex), 128);
		WriteSpriteSatEntry(vram, kSatBase, 1, 128, 1, 1, 0, (uint16_t)(0x4000 | kTileIndex), 160);

		vdp.Run(488);

		GenesisVdpState state = vdp.GetState();
		GenesisVdpState ppuState = state;
		GenesisVdpTools tools(nullptr, nullptr, nullptr);
		GetSpritePreviewOptions options = {};
		options.Background = SpriteBackground::Gray;
		DebugSpritePreviewInfo previewInfo = tools.GetSpritePreviewInfo(options, (BaseState&)state, (BaseState&)ppuState);

		std::array<uint32_t, 64> palette = {};
		BuildArgbPaletteFromCram(vdp, palette);
		std::vector<DebugSpriteInfo> sprites(previewInfo.SpriteCount);
		std::vector<uint32_t> spritePreviews(previewInfo.SpriteCount * 128u * 128u);
		std::vector<uint32_t> screenPreview(previewInfo.Width * previewInfo.Height);
		tools.GetSpriteList(options, (BaseState&)state, (BaseState&)ppuState, vdp.GetVramPointer(), nullptr, palette.data(), sprites.data(), spritePreviews.data(), screenPreview.data());

		uint32_t previewRowStart = previewInfo.VisibleY * previewInfo.Width + previewInfo.VisibleX;
		EXPECT_EQ(screenPreview[previewRowStart], palette[17]);
		EXPECT_EQ(screenPreview[previewRowStart + 32], palette[33]);
	}

	TEST(GenesisVdpObjectRunSafetyTests, MalformedSatOutOfRangeLinkKeepsDebuggerPreviewStable) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		vdp.GetCramPointer()[0] = 0x0000;
		vdp.GetCramPointer()[1] = 0x000e;
		ConfigureSpriteRenderMode(vdp, true, 0x00);

		uint8_t* vram = vdp.GetVramPointer();
		memset(vram, 0, 0x10000);
		constexpr uint16_t kTileIndex = 4;
		WriteSolidSpriteTileRow0(vram, kTileIndex, 1);

		constexpr uint16_t kSatBase = 0x0000;
		WriteSpriteSatEntry(vram, kSatBase, 0, 128, 1, 1, 127, (uint16_t)(0x8000 | kTileIndex), 128);

		vdp.Run(488);

		GenesisVdpState state = vdp.GetState();
		GenesisVdpState ppuState = state;
		GenesisVdpTools tools(nullptr, nullptr, nullptr);
		GetSpritePreviewOptions options = {};
		options.Background = SpriteBackground::Gray;
		DebugSpritePreviewInfo previewInfo = tools.GetSpritePreviewInfo(options, (BaseState&)state, (BaseState&)ppuState);

		std::array<uint32_t, 64> palette = {};
		BuildArgbPaletteFromCram(vdp, palette);

		std::vector<DebugSpriteInfo> sprites(previewInfo.SpriteCount);
		std::vector<uint32_t> spritePreviews(previewInfo.SpriteCount * 128u * 128u);
		std::vector<uint32_t> screenPreview(previewInfo.Width * previewInfo.Height);
		tools.GetSpriteList(options, (BaseState&)state, (BaseState&)ppuState, vram, nullptr, palette.data(), sprites.data(), spritePreviews.data(), screenPreview.data());

		uint32_t previewRowStart = previewInfo.VisibleY * previewInfo.Width + previewInfo.VisibleX;
		constexpr uint32_t kVisibleBg = 0xFF666666;
		EXPECT_NE(screenPreview[previewRowStart], kVisibleBg);
		EXPECT_EQ(screenPreview[previewRowStart + 16], kVisibleBg);
	}

	TEST(GenesisVdpObjectRunSafetyTests, SelfReferentialSatLinkDoesNotBreakDebuggerPreviewComposition) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		vdp.GetCramPointer()[0] = 0x0000;
		vdp.GetCramPointer()[1] = 0x000e;
		ConfigureSpriteRenderMode(vdp, true, 0x00);

		uint8_t* vram = vdp.GetVramPointer();
		memset(vram, 0, 0x10000);
		constexpr uint16_t kTileIndex = 4;
		WriteSolidSpriteTileRow0(vram, kTileIndex, 1);

		constexpr uint16_t kSatBase = 0x0000;
		WriteSpriteSatEntry(vram, kSatBase, 0, 128, 1, 1, 1, (uint16_t)(0x8000 | kTileIndex), 128);
		WriteSpriteSatEntry(vram, kSatBase, 1, 128, 1, 1, 1, (uint16_t)(0x8000 | kTileIndex), 128);

		vdp.Run(488);

		GenesisVdpState state = vdp.GetState();
		GenesisVdpState ppuState = state;
		GenesisVdpTools tools(nullptr, nullptr, nullptr);
		GetSpritePreviewOptions options = {};
		options.Background = SpriteBackground::Gray;
		DebugSpritePreviewInfo previewInfo = tools.GetSpritePreviewInfo(options, (BaseState&)state, (BaseState&)ppuState);

		std::array<uint32_t, 64> palette = {};
		BuildArgbPaletteFromCram(vdp, palette);

		std::vector<DebugSpriteInfo> sprites(previewInfo.SpriteCount);
		std::vector<uint32_t> spritePreviews(previewInfo.SpriteCount * 128u * 128u);
		std::vector<uint32_t> screenPreview(previewInfo.Width * previewInfo.Height);
		tools.GetSpriteList(options, (BaseState&)state, (BaseState&)ppuState, vram, nullptr, palette.data(), sprites.data(), spritePreviews.data(), screenPreview.data());

		uint32_t previewRowStart = previewInfo.VisibleY * previewInfo.Width + previewInfo.VisibleX;
		constexpr uint32_t kVisibleBg = 0xFF666666;
		EXPECT_NE(screenPreview[previewRowStart], kVisibleBg);
		EXPECT_EQ(screenPreview[previewRowStart + 16], kVisibleBg);
	}

	TEST(GenesisVdpObjectRunSafetyTests, StartupDisplayEnableCanRenderFirstVisibleScanline) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		// CRAM color format is 0000BBB0GGG0RRR0; this sets full red.
		vdp.GetCramPointer()[0] = 0x000e;
		// Enable display (R1 bit 6) while preserving existing startup bits.
		vdp.WriteControlPort(0x8144);
		vdp.Run(488);

		uint16_t* frame = vdp.GetScreenBuffer(false);
		EXPECT_EQ(frame[0], 0x7c00);
	}

	TEST(GenesisVdpObjectRunSafetyTests, VBlankAndVIntStartAtFirstVBlankLineBoundary) {
		Emulator emu;
		emu.Initialize(false);
		GenesisM68k cpu;
		GenesisVdp vdp;
		vdp.Init(&emu, nullptr, &cpu, nullptr);

		// Enable VBlank interrupt while keeping display disabled startup mode.
		vdp.WriteControlPort(0x8124);

		GenesisVdpState prev = vdp.GetState();
		bool reachedVBlankBoundary = false;
		constexpr uint32_t MaxCycles = 200000;

		for (uint32_t cycle = 1; cycle <= MaxCycles; cycle++) {
			vdp.Run(cycle);
			GenesisVdpState curr = vdp.GetState();

			if (curr.VCounter == 224 && curr.HCounter == 0) {
				reachedVBlankBoundary = true;
				EXPECT_EQ(prev.VCounter, 223u);
				EXPECT_EQ(prev.StatusRegister & VdpStatus::VBlankFlag, 0u);
				EXPECT_NE(curr.StatusRegister & VdpStatus::VBlankFlag, 0u);
				EXPECT_NE(curr.StatusRegister & VdpStatus::VIntPending, 0u);
				break;
			}

			prev = curr;
		}

		EXPECT_TRUE(reachedVBlankBoundary);
	}

	TEST(GenesisVdpObjectRunSafetyTests, FrameStartBoundaryClearsVBlankFlag) {
		Emulator emu;
		emu.Initialize(false);
		GenesisVdp vdp;
		vdp.Init(&emu, nullptr, nullptr, nullptr);

		GenesisVdpState prev = vdp.GetState();
		bool reachedFrameStartBoundary = false;
		constexpr uint32_t MaxCycles = 300000;

		for (uint32_t cycle = 1; cycle <= MaxCycles; cycle++) {
			vdp.Run(cycle);
			GenesisVdpState curr = vdp.GetState();

			if (curr.FrameCount > 0 && curr.VCounter == 0 && curr.HCounter == 0) {
				reachedFrameStartBoundary = true;
				EXPECT_EQ(prev.VCounter, 261u);
				EXPECT_NE(prev.StatusRegister & VdpStatus::VBlankFlag, 0u);
				EXPECT_EQ(curr.StatusRegister & VdpStatus::VBlankFlag, 0u);
				break;
			}

			prev = curr;
		}

		EXPECT_TRUE(reachedFrameStartBoundary);
	}
}
