#pragma once
#include "pch.h"
#include "Shared/Video/BaseVideoFilter.h"

class Emulator;

class Atari2600DefaultVideoFilter final : public BaseVideoFilter {
public:
	explicit Atari2600DefaultVideoFilter(Emulator* emu) : BaseVideoFilter(emu) {
		FrameInfo info = {};
		info.Width = 160;
		info.Height = 192;
		SetBaseFrameInfo(info);
	}

	void ApplyFilter(uint16_t* ppuOutputBuffer) override {
		uint32_t* out = GetOutputBuffer();
		FrameInfo frame = _frameInfo;
		uint32_t pixelCount = frame.Width * frame.Height;

		if (!ppuOutputBuffer) {
			memset(out, 0, pixelCount * sizeof(uint32_t));
			return;
		}

		for (uint32_t i = 0; i < pixelCount; i++) {
			uint16_t p = ppuOutputBuffer[i];
			uint8_t r = (uint8_t)(((p >> 11) & 0x1F) << 3);
			uint8_t g = (uint8_t)(((p >> 5) & 0x3F) << 2);
			uint8_t b = (uint8_t)((p & 0x1F) << 3);
			out[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
		}
	}
};
