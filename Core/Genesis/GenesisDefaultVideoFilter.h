#pragma once
#include "pch.h"
#include "Shared/Video/BaseVideoFilter.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"
#include "Shared/ColorUtilities.h"

class GenesisConsole;

class GenesisDefaultVideoFilter final : public BaseVideoFilter {
private:
	uint32_t _calculatedPalette[0x8000] = {};
	VideoConfig _videoConfig = {};
	GenesisConsole* _console = nullptr;

	void InitLookupTable() {
		VideoConfig config = _emu->GetSettings()->GetVideoConfig();
		InitConversionMatrix(config.Hue, config.Saturation);

		for (int rgb555 = 0; rgb555 < 0x8000; rgb555++) {
			uint8_t r = ColorUtilities::Convert5BitTo8Bit(rgb555 & 0x1F);
			uint8_t g = ColorUtilities::Convert5BitTo8Bit((rgb555 >> 5) & 0x1F);
			uint8_t b = ColorUtilities::Convert5BitTo8Bit((rgb555 >> 10) & 0x1F);

			if (config.Hue != 0 || config.Saturation != 0 || config.Brightness != 0 || config.Contrast != 0) {
				ApplyColorOptions(r, g, b, config.Brightness, config.Contrast);
			}
			_calculatedPalette[rgb555] = 0xFF000000 | (r << 16) | (g << 8) | b;
		}

		_videoConfig = config;
	}

protected:
	void OnBeforeApplyFilter() override {
		VideoConfig& config = _emu->GetSettings()->GetVideoConfig();
		if (_videoConfig.Hue != config.Hue || _videoConfig.Saturation != config.Saturation ||
			_videoConfig.Contrast != config.Contrast || _videoConfig.Brightness != config.Brightness) {
			InitLookupTable();
		}
		_videoConfig = config;
	}

	FrameInfo GetFrameInfo() override {
		PpuFrameInfo frame = _emu->GetPpuFrame();
		FrameInfo info;
		info.Width = frame.Width;
		info.Height = frame.Height;
		return info;
	}

public:
	GenesisDefaultVideoFilter(Emulator* emu, GenesisConsole* console) : BaseVideoFilter(emu) {
		_console = console;
		InitLookupTable();
	}

	void ApplyFilter(uint16_t* ppuOutputBuffer) override {
		uint32_t* out = GetOutputBuffer();
		FrameInfo frameInfo = GetFrameInfo();
		uint32_t pixelCount = frameInfo.Width * frameInfo.Height;

		for (uint32_t i = 0; i < pixelCount; i++) {
			out[i] = _calculatedPalette[ppuOutputBuffer[i] & 0x7FFF];
		}
	}
};
