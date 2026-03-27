#pragma once
#include "pch.h"
#include "Shared/Video/BaseVideoFilter.h"

class Emulator;

class ChannelFDefaultVideoFilter final : public BaseVideoFilter {
public:
	// Channel F 4-color palette (ARGB)
	static constexpr uint32_t ChannelFPalette[4] = {
		0xff101010, // Color 0: Background (black/dark gray)
		0xff1cdf1c, // Color 1: Green
		0xff3131fd, // Color 2: Blue
		0xfffdfdfd  // Color 3: White/Gray
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
			out[i] = ChannelFPalette[ppuOutputBuffer[i] & 0x03];
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
