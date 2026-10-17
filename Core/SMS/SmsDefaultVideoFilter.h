#pragma once
#include "pch.h"
#include "SMS/SmsConsole.h"
#include "SMS/SmsTypes.h"
#include "Shared/Video/BaseVideoFilter.h"
#include "Shared/EmuSettings.h"
#include "Shared/Emulator.h"
#include "Shared/RewindManager.h"
#include "Shared/ColorUtilities.h"

/// <summary>
/// SMS/Game Gear default video filter - converts 15-bit RGB to 32-bit ARGB.
/// Supports Game Gear frame blending for LCD ghosting simulation.
/// </summary>
/// <remarks>
/// **Pipeline:**
/// 1. VDP outputs 15-bit RGB (BGR555 format)
/// 2. Look up pre-computed 32-bit ARGB from palette table
/// 3. Apply brightness/contrast/hue/saturation adjustments
/// 4. Optional frame blending for Game Gear LCD simulation
/// 5. Output 32-bit ARGB
///
/// **Game Gear Frame Blending:**
/// - Simulates LCD ghosting/persistence
/// - Blends current frame with previous frame
/// - Disabled during rewind and pause
/// - Uses fast bit-manipulation blending
///
/// **Supported Models:**
/// - Sega Master System (SMS)
/// - Game Gear (with LCD blending option)
/// - SG-1000 (early Sega console)
///
/// **Resolution:**
/// - SMS: 256×192 (NTSC) or 256×240 (PAL)
/// - Game Gear: 160×144 visible area
/// - Internal buffer: 256×240 for all modes
///
/// **Color Space:**
/// - Input: 15-bit BGR555
/// - Palette table: 32,768 entries (0x8000)
/// - Output: 32-bit ARGB
/// </remarks>
class SmsDefaultVideoFilter : public BaseVideoFilter {
private:
	uint32_t _calculatedPalette[0x8000] = {};  ///< Pre-computed ARGB palette (32K entries)
	VideoConfig _videoConfig = {};              ///< Video settings snapshot
	uint16_t _prevFrame[256 * 240] = {};        ///< Previous frame for GG blending
	bool _blendFrames = false;                  ///< Enable Game Gear frame blending
	SmsConsole* _console = nullptr;             ///< Console instance for model detection

protected:
	/// <summary>Update settings and frame blending state before filter.</summary>
	void OnBeforeApplyFilter() override {
		VideoConfig& config = _emu->GetSettings()->GetVideoConfig();
		SmsConfig smsConfig = _emu->GetSettings()->GetSmsConfig();

		// Check if color settings changed requiring palette rebuild
		if (_videoConfig.Hue != config.Hue || _videoConfig.Saturation != config.Saturation || _videoConfig.Contrast != config.Contrast || _videoConfig.Brightness != config.Brightness) {
			InitLookupTable();
		}

		// Frame blending only for Game Gear, not during rewind/pause
		bool blendFrames = _console->GetModel() == SmsModel::GameGear && smsConfig.GgBlendFrames && !_emu->GetRewindManager()->IsRewinding() && !_emu->IsPaused();
		if (_blendFrames != blendFrames) {
			_blendFrames = blendFrames;
			memset(_prevFrame, 0, 256 * 240 * sizeof(uint16_t));  // Clear blend buffer on mode change
		}
		_videoConfig = config;
	}

	/// <summary>Initialize 32K-entry RGB lookup table from settings.</summary>
	void InitLookupTable() {
		VideoConfig config = _emu->GetSettings()->GetVideoConfig();

		InitConversionMatrix(config.Hue, config.Saturation);

		// Build lookup table for all 32768 possible RGB555 values
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

	/// <summary>Get pixel with optional frame blending for Game Gear.</summary>
	/// <param name="vdpFrame">VDP output buffer</param>
	/// <param name="offset">Pixel offset</param>
	/// <returns>32-bit ARGB pixel (blended if enabled)</returns>
	uint32_t GetPixel(uint16_t* vdpFrame, uint32_t offset) {
		if (_blendFrames) {
			return BlendPixels(_calculatedPalette[_prevFrame[offset]], _calculatedPalette[vdpFrame[offset]]);
		} else {
			return _calculatedPalette[vdpFrame[offset]];
		}
	}

	/// <summary>Fast pixel blending using bit manipulation (50% blend).</summary>
	/// <param name="a">First pixel ARGB</param>
	/// <param name="b">Second pixel ARGB</param>
	/// <returns>Blended pixel ARGB</returns>
	/// <remarks>Uses ((a^b) & 0xFEFEFEFE) >> 1 + (a & b) formula for speed.</remarks>
	uint32_t BlendPixels(uint32_t a, uint32_t b) {
		return ((((a) ^ (b)) & 0xfffefefeL) >> 1) + ((a) & (b));
	}

public:
	/// <summary>Constructor initializes palette tables.</summary>
	/// <param name="emu">Emulator instance for settings access</param>
	/// <param name="console">SMS console instance for model detection</param>
	SmsDefaultVideoFilter(Emulator* emu, SmsConsole* console) : BaseVideoFilter(emu) {
		_console = console;
		InitLookupTable();
	}

	/// <summary>Apply filter to VDP output buffer.</summary>
	/// <param name="ppuOutputBuffer">15-bit RGB VDP output</param>
	void ApplyFilter(uint16_t* ppuOutputBuffer) override {
		uint16_t* in = ppuOutputBuffer;
		uint32_t* out = GetOutputBuffer();

		OverscanDimensions overscan = GetOverscan();
		FrameInfo frame = _frameInfo;

		// Calculate viewport offset for different scanline counts
		uint32_t linesToSkip = _console->GetVdp()->GetViewportYOffset();
		uint32_t scanlineCount = _console->GetVdp()->GetState().VisibleScanlineCount;

		// Process each scanline with overscan cropping
		for (uint32_t y = 0; y < frame.Height; y++) {
			if (y + overscan.Top < linesToSkip || y > linesToSkip + scanlineCount - overscan.Top) {
				// Outside visible area - fill with black
				memset(out + y * frame.Width, 0, frame.Width * sizeof(uint32_t));
			} else {
				// Convert visible pixels through palette lookup
				for (uint32_t x = 0; x < frame.Width; x++) {
					out[(y * frame.Width) + x] = GetPixel(in, (y + overscan.Top - linesToSkip) * _baseFrameInfo.Width + x + overscan.Left);
				}
			}
		}

		// Save frame for next frame's blending (if enabled)
		if (_blendFrames) {
			std::copy(in, in + 256 * 240, _prevFrame);
		}
	}
};