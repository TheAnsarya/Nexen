#pragma once
#include "pch.h"
#include "Shared/Video/BaseVideoFilter.h"

class Emulator;

class ChannelFDefaultVideoFilter final : public BaseVideoFilter {
public:
	// Channel F 8-color hardware palette (ARGB)
	// Per MAME reference: channelf_pens[]
	static constexpr uint32_t ChannelFColors[8] = {
		0xff101010, // 0: BLACK
		0xfffdfdfd, // 1: WHITE
		0xffff3153, // 2: RED
		0xff02cc5d, // 3: GREEN
		0xff4b3ff3, // 4: BLUE
		0xffe0e0e0, // 5: LTGRAY
		0xff91ffa6, // 6: LTGREEN
		0xffced0ff  // 7: LTBLUE
	};

	// 4 palettes × 4 colors = 16-entry colormap
	// Per-row palette selected by VRAM columns 125 and 126
	// Frame buffer stores palette_offset + pixel_color (0-15)
	static constexpr uint32_t ChannelFColormap[16] = {
		0xff101010, 0xfffdfdfd, 0xfffdfdfd, 0xfffdfdfd, // Palette 0: BLACK, WHITE, WHITE, WHITE
		0xffced0ff, 0xff4b3ff3, 0xffff3153, 0xff02cc5d, // Palette 1: LTBLUE, BLUE, RED, GREEN
		0xffe0e0e0, 0xff4b3ff3, 0xffff3153, 0xff02cc5d, // Palette 2: LTGRAY, BLUE, RED, GREEN
		0xff91ffa6, 0xff4b3ff3, 0xffff3153, 0xff02cc5d  // Palette 3: LTGREEN, BLUE, RED, GREEN
	};

protected:
	void ApplyFilter(uint16_t* ppuOutputBuffer) override {
		uint32_t* out = GetOutputBuffer();
		FrameInfo frame = _frameInfo;
		uint32_t pixelCount = frame.Width * frame.Height;

		if (!ppuOutputBuffer) {
			memset(out, 0, pixelCount * sizeof(uint32_t));
			return;
		}

		for (uint32_t i = 0; i < pixelCount; i++) {
			out[i] = ChannelFColormap[ppuOutputBuffer[i] & 0x0f];
		}
	}

public:
	explicit ChannelFDefaultVideoFilter(Emulator* emu)
		: BaseVideoFilter(emu) {
		FrameInfo info = {};
		info.Width = 128;
		info.Height = 64;
		SetBaseFrameInfo(info);
	}
};
