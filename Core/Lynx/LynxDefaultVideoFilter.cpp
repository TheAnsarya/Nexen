#include "pch.h"
#include "Lynx/LynxDefaultVideoFilter.h"
#include "Lynx/LynxConsole.h"
#include "Lynx/LynxTypes.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"
#include "Shared/ColorUtilities.h"

LynxDefaultVideoFilter::LynxDefaultVideoFilter(Emulator* emu, LynxConsole* console)
	: BaseVideoFilter(emu) {
	_emu = emu;
	_console = console;
	InitLookupTable();
}

void LynxDefaultVideoFilter::InitLookupTable() {
	VideoConfig config = _emu->GetSettings()->GetVideoConfig();

	bool needsAdjustment = (config.Hue != 0 || config.Saturation != 0 ||
		config.Brightness != 0 || config.Contrast != 0);

	if (needsAdjustment) {
		InitConversionMatrix(config.Hue, config.Saturation);

		for (int rgb444 = 0; rgb444 < 0x1000; rgb444++) {
			uint8_t r = ColorUtilities::Convert4BitTo8Bit((rgb444 >> 8) & 0xF);
			uint8_t g = ColorUtilities::Convert4BitTo8Bit((rgb444 >> 4) & 0xF);
			uint8_t b = ColorUtilities::Convert4BitTo8Bit(rgb444 & 0xF);

			ApplyColorOptions(r, g, b, config.Brightness, config.Contrast);
			_adjustedPalette[rgb444] = 0xFF000000 | (r << 16) | (g << 8) | b;
		}
		_useAdjustedPalette = true;
	} else {
		_useAdjustedPalette = false;
	}

	_videoConfig = config;
}

FrameInfo LynxDefaultVideoFilter::GetFrameInfo() {
	LynxRotation rotation = _console->GetRotation();
	FrameInfo info = {};
	if (rotation == LynxRotation::Left || rotation == LynxRotation::Right) {
		info.Width = LynxConstants::ScreenHeight; // 102
		info.Height = LynxConstants::ScreenWidth;  // 160
	} else {
		info.Width = LynxConstants::ScreenWidth;   // 160
		info.Height = LynxConstants::ScreenHeight;  // 102
	}
	return info;
}

void LynxDefaultVideoFilter::OnBeforeApplyFilter() {
	VideoConfig config = _emu->GetSettings()->GetVideoConfig();
	if (_videoConfig.Hue != config.Hue || _videoConfig.Saturation != config.Saturation ||
		_videoConfig.Brightness != config.Brightness || _videoConfig.Contrast != config.Contrast) {
		InitLookupTable();
	}
}

void LynxDefaultVideoFilter::ApplyFilter(uint16_t* ppuOutputBuffer) {
	// The Lynx frame buffer is already 32-bit ARGB — reinterpret the uint16_t* as uint32_t*
	uint32_t* srcBuffer = reinterpret_cast<uint32_t*>(ppuOutputBuffer);
	uint32_t* out = GetOutputBuffer();
	LynxRotation rotation = _console->GetRotation();

	uint32_t srcW = LynxConstants::ScreenWidth;   // 160
	uint32_t srcH = LynxConstants::ScreenHeight;   // 102
	uint32_t pixelCount = srcW * srcH;

	// When no palette adjustment is needed, skip the per-pixel lookup entirely.
	// This eliminates a branch + lambda call per pixel (~16,320 pixels/frame).
	if (!_useAdjustedPalette) {
		switch (rotation) {
			case LynxRotation::None:
				memcpy(out, srcBuffer, pixelCount * sizeof(uint32_t));
				break;

			case LynxRotation::Left:
				for (uint32_t y = 0; y < srcH; y++) {
					for (uint32_t x = 0; x < srcW; x++) {
						out[(srcW - 1 - x) * srcH + y] = srcBuffer[y * srcW + x];
					}
				}
				break;

			case LynxRotation::Right:
				for (uint32_t y = 0; y < srcH; y++) {
					for (uint32_t x = 0; x < srcW; x++) {
						out[x * srcH + (srcH - 1 - y)] = srcBuffer[y * srcW + x];
					}
				}
				break;
		}
		return;
	}

	// Color-adjusted path — apply palette lookup per pixel
	auto adjustPixel = [this](uint32_t pixel) -> uint32_t {
		uint8_t r4 = ((pixel >> 16) & 0xff) >> 4;
		uint8_t g4 = ((pixel >> 8) & 0xff) >> 4;
		uint8_t b4 = (pixel & 0xff) >> 4;
		uint16_t rgb444 = (r4 << 8) | (g4 << 4) | b4;
		return _adjustedPalette[rgb444];
	};

	switch (rotation) {
		case LynxRotation::None:
			for (uint32_t i = 0; i < pixelCount; i++) {
				out[i] = adjustPixel(srcBuffer[i]);
			}
			break;

		case LynxRotation::Left:
			for (uint32_t y = 0; y < srcH; y++) {
				for (uint32_t x = 0; x < srcW; x++) {
					out[(srcW - 1 - x) * srcH + y] = adjustPixel(srcBuffer[y * srcW + x]);
				}
			}
			break;

		case LynxRotation::Right:
			for (uint32_t y = 0; y < srcH; y++) {
				for (uint32_t x = 0; x < srcW; x++) {
					out[x * srcH + (srcH - 1 - y)] = adjustPixel(srcBuffer[y * srcW + x]);
				}
			}
			break;
	}
}
