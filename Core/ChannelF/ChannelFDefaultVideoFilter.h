#pragma once
#include "pch.h"
#include "Shared/Video/BaseVideoFilter.h"

class Emulator;

class ChannelFDefaultVideoFilter final : public BaseVideoFilter {
private:
	uint32_t _palette[128] = {};

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
			out[i] = _palette[ppuOutputBuffer[i] & 0x7f];
		}
	}

public:
	explicit ChannelFDefaultVideoFilter(Emulator* emu)
		: BaseVideoFilter(emu) {
		FrameInfo info = {};
		info.Width = 128;
		info.Height = 64;
		SetBaseFrameInfo(info);

		for (int i = 0; i < 128; i++) {
			uint32_t c = (uint32_t)(i * 2);
			_palette[i] = 0xff000000 | (c << 16) | (c << 8) | c;
		}
	}
};
