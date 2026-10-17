#pragma once
#include "pch.h"
#include "Shared/Video/BaseVideoFilter.h"
#include "Shared/EmuSettings.h"
#include "Shared/Emulator.h"
#include "Shared/ColorUtilities.h"

class Emulator;

class Atari2600DefaultVideoFilter final : public BaseVideoFilter {
private:
	// Pre-computed ARGB palette — 128 NTSC TIA colors with adjustments applied
	uint32_t _calculatedPalette[128] = {};
	VideoConfig _videoConfig = {};

public:
	// Standard Stella NTSC TIA master palette — 128 colors as 0x00RRGGBB
	// 16 hues x 8 luminance levels, indexed by (tiaColor >> 1)
	static constexpr uint32_t NtscPalette[128] = {
		0x000000, 0x404040, 0x6c6c6c, 0x909090, 0xb0b0b0, 0xc8c8c8, 0xdcdcdc, 0xf4f4f4,
		0x444400, 0x646410, 0x848424, 0xa0a034, 0xb8b840, 0xd0d050, 0xe8e85c, 0xfcfc68,
		0x702800, 0x844414, 0x985c28, 0xac783c, 0xbc8c4c, 0xcca05c, 0xdcb468, 0xecc878,
		0x841800, 0x983418, 0xac5030, 0xc06848, 0xd0805c, 0xe09470, 0xeca880, 0xfcbc94,
		0x880000, 0x9c2020, 0xb03c3c, 0xc05858, 0xd07070, 0xe08888, 0xeca0a0, 0xfcb4b4,
		0x78005c, 0x8c2074, 0xa03c88, 0xb0589c, 0xc070b0, 0xd084c0, 0xdc9cd0, 0xecb0e0,
		0x480078, 0x602090, 0x783ca4, 0x8c58b8, 0xa070cc, 0xb484dc, 0xc49cec, 0xd4b0fc,
		0x140084, 0x302098, 0x4c3cac, 0x6858c0, 0x7c70d0, 0x9488e0, 0xa8a0ec, 0xbcb4fc,
		0x000088, 0x1c209c, 0x3840b0, 0x505cc0, 0x6874d0, 0x7c8ce0, 0x90a4ec, 0xa4b8fc,
		0x00187c, 0x1c3890, 0x3854a8, 0x5070bc, 0x689cc0, 0x7c9cdc, 0x90b4ec, 0xa4c8fc,
		0x002c5c, 0x1c4c78, 0x386890, 0x5084ac, 0x689cc0, 0x7cb4d4, 0x90cce8, 0xa4e0fc,
		0x003c2c, 0x1c5c48, 0x387c64, 0x509c80, 0x68b494, 0x7cd0ac, 0x90e4c0, 0xa4fcd4,
		0x003c00, 0x205c20, 0x407c40, 0x5c9c5c, 0x74b474, 0x8cd08c, 0xa4e4a4, 0xb8fcb8,
		0x143800, 0x345c1c, 0x507c38, 0x6c9850, 0x84b468, 0x9ccc7c, 0xb4e490, 0xc8fca4,
		0x2c3000, 0x4c501c, 0x687034, 0x848c4c, 0x9ca864, 0xb4c078, 0xccd488, 0xe0ec9c,
		0x442800, 0x644818, 0x846830, 0xa08444, 0xb89c58, 0xd0b46c, 0xe8cc7c, 0xfce08c,
	};

protected:
	void OnBeforeApplyFilter() override {
		VideoConfig& config = _emu->GetSettings()->GetVideoConfig();
		if (_videoConfig.Hue != config.Hue || _videoConfig.Saturation != config.Saturation ||
			_videoConfig.Contrast != config.Contrast || _videoConfig.Brightness != config.Brightness) {
			InitLookupTable();
		}
		_videoConfig = config;
	}

	void InitLookupTable() {
		VideoConfig config = _emu->GetSettings()->GetVideoConfig();
		InitConversionMatrix(config.Hue, config.Saturation);

		for (int i = 0; i < 128; i++) {
			uint32_t c = NtscPalette[i];
			uint8_t r = (uint8_t)((c >> 16) & 0xff);
			uint8_t g = (uint8_t)((c >> 8) & 0xff);
			uint8_t b = (uint8_t)(c & 0xff);

			if (config.Hue != 0 || config.Saturation != 0 || config.Brightness != 0 || config.Contrast != 0) {
				ApplyColorOptions(r, g, b, config.Brightness, config.Contrast);
			}
			_calculatedPalette[i] = 0xff000000 | (r << 16) | (g << 8) | b;
		}
		_videoConfig = config;
	}

public:
	explicit Atari2600DefaultVideoFilter(Emulator* emu) : BaseVideoFilter(emu) {
		FrameInfo info = {};
		info.Width = 160;
		info.Height = 192;
		SetBaseFrameInfo(info);
		InitLookupTable();
	}

	void ApplyFilter(uint16_t* ppuOutputBuffer) override {
		uint32_t* out = GetOutputBuffer();
		FrameInfo frame = _frameInfo;
		uint32_t pixelCount = frame.Width * frame.Height;

		if (!ppuOutputBuffer) {
			memset(out, 0, pixelCount * sizeof(uint32_t));
			return;
		}

		// ppuOutputBuffer contains TIA palette indices (0-127) from RenderDebugFrame
		for (uint32_t i = 0; i < pixelCount; i++) {
			out[i] = _calculatedPalette[ppuOutputBuffer[i] & 0x7f];
		}
	}
};
