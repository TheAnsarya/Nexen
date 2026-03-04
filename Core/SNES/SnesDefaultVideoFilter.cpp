#include "pch.h"
#include <algorithm>
#include "SNES/SnesDefaultVideoFilter.h"
#include "Shared/Video/DebugHud.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"
#include "Shared/SettingTypes.h"
#include "Shared/ColorUtilities.h"

SnesDefaultVideoFilter::SnesDefaultVideoFilter(Emulator* emu) : BaseVideoFilter(emu) {
	InitLookupTable();
}

FrameInfo SnesDefaultVideoFilter::GetFrameInfo() {
	if (_emu->GetRomInfo().Format == RomFormat::Spc) {
		// Give a fixed 256x240 of space to SPC player to match NES/GB players
		FrameInfo frame;
		frame.Width = 256;
		frame.Height = 240;
		return frame;
	} else {
		if (_forceFixedRes && _baseFrameInfo.Width == 256) {
			OverscanDimensions overscan = GetOverscan();
			return FrameInfo{512 - (overscan.Left + overscan.Right) * 2, 478 - (overscan.Top + overscan.Bottom) * 2};
		} else {
			return BaseVideoFilter::GetFrameInfo();
		}
	}
}

OverscanDimensions SnesDefaultVideoFilter::GetOverscan() {
	OverscanDimensions overscan = BaseVideoFilter::GetOverscan();
	int overscanMultiplier = _baseFrameInfo.Width == 512 ? 2 : 1;
	overscan.Top *= overscanMultiplier;
	overscan.Bottom *= overscanMultiplier;
	overscan.Left *= overscanMultiplier;
	overscan.Right *= overscanMultiplier;
	return overscan;
}

void SnesDefaultVideoFilter::InitLookupTable() {
	VideoConfig config = _emu->GetSettings()->GetVideoConfig();

	InitConversionMatrix(config.Hue, config.Saturation);

	for (int rgb555 = 0; rgb555 < 0x8000; rgb555++) {
		uint8_t r = ColorUtilities::Convert5BitTo8Bit(rgb555 & 0x1F);
		uint8_t g = ColorUtilities::Convert5BitTo8Bit((rgb555 >> 5) & 0x1F);
		uint8_t b = ColorUtilities::Convert5BitTo8Bit((rgb555 >> 10) & 0x1F);

		if (config.Hue != 0 || config.Saturation != 0 || config.Brightness != 0 || config.Contrast != 0) {
			ApplyColorOptions(r, g, b, config.Brightness, config.Contrast);
			_calculatedPalette[rgb555] = 0xFF000000 | (r << 16) | (g << 8) | b;
		} else {
			_calculatedPalette[rgb555] = 0xFF000000 | (r << 16) | (g << 8) | b;
		}
	}

	_videoConfig = config;
}

void SnesDefaultVideoFilter::OnBeforeApplyFilter() {
	VideoConfig& config = _emu->GetSettings()->GetVideoConfig();
	SnesConfig& snesConfig = _emu->GetSettings()->GetSnesConfig();

	if (_videoConfig.Hue != config.Hue || _videoConfig.Saturation != config.Saturation || _videoConfig.Contrast != config.Contrast || _videoConfig.Brightness != config.Brightness) {
		InitLookupTable();
	}
	_forceFixedRes = snesConfig.ForceFixedResolution;
	_blendHighRes = snesConfig.BlendHighResolutionModes;
	_videoConfig = config;
}

void SnesDefaultVideoFilter::ApplyFilter(uint16_t* ppuOutputBuffer) {
	if (_emu->GetRomInfo().Format == RomFormat::Spc) {
		return;
	}

	uint32_t* out = GetOutputBuffer();
	FrameInfo frameInfo = _frameInfo;
	OverscanDimensions overscan = GetOverscan();

	uint32_t width = _baseFrameInfo.Width;
	uint32_t xOffset = overscan.Left;
	uint32_t yOffset = overscan.Top * width;

	if (_baseFrameInfo.Width == 256 && _forceFixedRes) {
		uint32_t outIdx = 0;
		for (uint32_t i = 0; i < frameInfo.Height; i++) {
			uint32_t srcRow = (i / 2) * width + yOffset + xOffset;
			for (uint32_t j = 0; j < frameInfo.Width; j++) {
				out[outIdx++] = GetPixel(ppuOutputBuffer, srcRow + j / 2);
			}
		}
	} else {
		// Hoist row offset outside inner loop to eliminate per-pixel multiply
		uint32_t outIdx = 0;
		uint32_t srcOffset = yOffset + xOffset;
		for (uint32_t i = 0; i < frameInfo.Height; i++) {
			for (uint32_t j = 0; j < frameInfo.Width; j++) {
				out[outIdx++] = GetPixel(ppuOutputBuffer, srcOffset + j);
			}
			srcOffset += width;
		}
	}

	if (_baseFrameInfo.Width == 512 && _blendHighRes) {
		// Blend hi-res pixel pairs: (0,1), (2,3), etc.
		// Each pair is blended together and both set to the same result.
		// Fixes incorrect full-screen blur (Mesen2 PR #80).
		uint32_t* row = out;
		for (uint32_t i = 0; i < frameInfo.Height; i++) {
			for (uint32_t j = 0; j < frameInfo.Width; j += 2) {
				uint32_t blended = BlendPixels(row[j], row[j + 1]);
				row[j] = blended;
				row[j + 1] = blended;
			}
			row += frameInfo.Width;
		}
	}
}

uint32_t SnesDefaultVideoFilter::GetPixel(uint16_t* ppuFrame, uint32_t offset) {
	return _calculatedPalette[ppuFrame[offset]];
}

constexpr uint32_t SnesDefaultVideoFilter::BlendPixels(uint32_t a, uint32_t b) {
	return ((((a) ^ (b)) & 0xfffefefeL) >> 1) + ((a) & (b));
}
