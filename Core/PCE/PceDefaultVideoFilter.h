#pragma once
#include "pch.h"
#include "PCE/PceConstants.h"
#include "Shared/Video/BaseVideoFilter.h"
#include "Shared/EmuSettings.h"
#include "Shared/Emulator.h"

/// <summary>
/// PC Engine/TurboGrafx-16 default video filter - converts 9-bit RGB to 32-bit ARGB.
/// Handles variable resolution modes and dynamic clock dividers.
/// </summary>
/// <remarks>
/// **Pipeline:**
/// 1. VCE outputs 9-bit RGB (RGB333 format)
/// 2. Look up pre-computed 32-bit ARGB from palette table
/// 3. Apply brightness/contrast/hue/saturation adjustments
/// 4. Handle variable horizontal resolution (per-scanline clock dividers)
/// 5. Optional grayscale conversion for composite video simulation
/// 6. Output 32-bit ARGB
///
/// **Variable Resolution:**
/// - PCE supports multiple horizontal resolutions per frame
/// - Clock divider stored per-scanline in PPU buffer
/// - Resolutions: 256, 336, 512 pixels wide (varies by clock divider)
/// - _frameDivider tracks current frame's clock divider
/// - _forceFixedRes forces 4x internal resolution
///
/// **HES Player:**
/// - Special handling for HES (PC Engine Sound Format) files
/// - Fixed 256×240 output for audio visualization
/// - No VDC rendering required
///
/// **Color Space:**
/// - Input: 9-bit RGB333 (3 bits per channel)
/// - Palette table: 512 entries (+ 512 grayscale)
/// - Grayscale variant at offset 0x200 for B&W mode
/// - Output: 32-bit ARGB
///
/// **SuperGrafx:**
/// - Same filter works for SuperGrafx
/// - VPC handles layer compositing before filter
///
/// **Performance:**
/// - Pre-computed lookup table
/// - Per-row interpolation for variable resolution
/// - UpdateFrameSize() called each frame
/// </remarks>
class PceDefaultVideoFilter : public BaseVideoFilter {
private:
	uint32_t _calculatedPalette[0x400] = {};  ///< Pre-computed ARGB palette (512 color + 512 grayscale)
	VideoConfig _videoConfig = {};             ///< Video settings snapshot

protected:
	PcEngineConfig _pceConfig = {};    ///< PC Engine-specific settings
	uint8_t _frameDivider = 0;         ///< Current frame's clock divider (0 = mixed/4x scale)
	FrameInfo _pceFrameSize = {256, 242};  ///< Current output frame dimensions

	/// <summary>Get output frame dimensions based on clock divider.</summary>
	/// <returns>Frame dimensions (varies by horizontal resolution)</returns>
	FrameInfo GetFrameInfo() override {
		if (_emu->GetRomInfo().Format == RomFormat::PceHes) {
			// Give a fixed 256x240 of space to HES player to match the other players
			FrameInfo frame;
			frame.Width = 256;
			frame.Height = 240;
			return frame;
		} else {
			UpdateFrameSize();
			return _pceFrameSize;
		}
	}

	/// <summary>Calculate frame size from clock dividers (varies per-scanline).</summary>
	void UpdateFrameSize() {
		if (!_pceConfig.ForceFixedResolution) {
			// Try to use an output resolution that matches the core's output (instead of forcing 4x scale)
			constexpr uint32_t clockDividerOffset = PceConstants::MaxScreenWidth * PceConstants::ScreenHeight;

			OverscanDimensions overscan = BaseVideoFilter::GetOverscan();
			uint32_t rowCount = PceConstants::ScreenHeight - overscan.Top - overscan.Bottom;

			// Check if all visible scanlines use the same clock divider
			uint8_t frameDivider = 0;
			for (uint32_t i = 0; i < rowCount; i++) {
				uint8_t rowDivider = (uint8_t)_ppuOutputBuffer[clockDividerOffset + i + overscan.Top];
				if (frameDivider == 0) {
					frameDivider = rowDivider;
				} else if (frameDivider != rowDivider) {
					// Picture has multiple resolutions at once, use 4x scale
					frameDivider = 0;
					break;
				}
			}

			_frameDivider = frameDivider;

			// Refresh overscan left/right values after _frameDivider is updated
			overscan = BaseVideoFilter::GetOverscan();

			if (frameDivider) {
				// Uniform resolution - use native size
				FrameInfo size = {};
				size.Width = PceConstants::GetRowWidth(frameDivider) - (overscan.Left + overscan.Right) * 4 / frameDivider;
				size.Height = PceConstants::ScreenHeight - (overscan.Top + overscan.Bottom);
				_pceFrameSize = size;
			} else {
				// Mixed resolution - use 4x internal scale
				_pceFrameSize = BaseVideoFilter::GetFrameInfo();
			}
		} else {
			// Always output at 4x scale (allows recording movies properly, etc.)
			_frameDivider = 0;
			_pceFrameSize = BaseVideoFilter::GetFrameInfo();
		}
	}

	/// <summary>Get pixel from calculated palette.</summary>
	/// <param name="ppuFrame">VCE output buffer</param>
	/// <param name="offset">Pixel offset</param>
	/// <returns>32-bit ARGB pixel</returns>
	uint32_t GetPixel(uint16_t* ppuFrame, uint32_t offset) {
		return _calculatedPalette[ppuFrame[offset] & 0x3FF];  // Mask to 10 bits (9-bit color + grayscale flag)
	}

	/// <summary>Get HUD scale factors for variable resolution.</summary>
	/// <returns>Scale factors (may differ X vs Y for non-square pixels)</returns>
	HudScaleFactors GetScaleFactor() override {
		if (_emu->GetRomInfo().Format == RomFormat::PceHes) {
			return {1, 1};
		} else if (_frameDivider == 0) {
			// 4x internal resolution
			return {PceConstants::InternalResMultipler, PceConstants::InternalResMultipler};
		} else {
			// Variable horizontal scale based on clock divider
			return {(double)4 / _frameDivider, 1};
		}
	}

	/// <summary>Get overscan dimensions adjusted for resolution mode.</summary>
	/// <returns>Adjusted overscan dimensions</returns>
	OverscanDimensions GetOverscan() override {
		OverscanDimensions overscan = BaseVideoFilter::GetOverscan();
		uint8_t frameDivider = _frameDivider;
		if (frameDivider == 0) {
			// 4x internal scale - multiply all overscan values
			overscan.Top *= PceConstants::InternalResMultipler;
			overscan.Bottom *= PceConstants::InternalResMultipler;
			overscan.Left *= PceConstants::InternalResMultipler;
			overscan.Right *= PceConstants::InternalResMultipler;
		} else {
			// Native resolution - scale horizontal only
			overscan.Left *= 4.0 / frameDivider;
			overscan.Right *= 4.0 / frameDivider;
		}
		return overscan;
	}

	/// <summary>Initialize palette lookup table (color + grayscale).</summary>
	void InitLookupTable() {
		VideoConfig& config = _emu->GetSettings()->GetVideoConfig();
		PcEngineConfig& pceCfg = _emu->GetSettings()->GetPcEngineConfig();

		InitConversionMatrix(config.Hue, config.Saturation);

		// Build lookup for all 512 RGB333 colors
		for (int rgb333 = 0; rgb333 < 0x0200; rgb333++) {
			uint32_t color = pceCfg.Palette[rgb333];
			uint8_t r = (color >> 16) & 0xFF;
			uint8_t g = (color >> 8) & 0xFF;
			uint8_t b = color & 0xFF;

			if (config.Hue != 0 || config.Saturation != 0 || config.Brightness != 0 || config.Contrast != 0) {
				ApplyColorOptions(r, g, b, config.Brightness, config.Contrast);
			}

			// Convert RGB to grayscale color (for B&W composite mode)
			uint8_t grayscaleY = (uint8_t)std::clamp(0.299 * r + 0.587 * g + 0.114 * b, 0.0, 255.0);
			_calculatedPalette[rgb333 | 0x200] = 0xFF000000 | (grayscaleY << 16) | (grayscaleY << 8) | grayscaleY;

			// Regular RGB color
			_calculatedPalette[rgb333] = 0xFF000000 | (r << 16) | (g << 8) | b;
		}

		_videoConfig = config;
	}

	/// <summary>Update lookup table if settings changed.</summary>
	void OnBeforeApplyFilter() override {
		VideoConfig& config = _emu->GetSettings()->GetVideoConfig();
		PcEngineConfig& pceConfig = _emu->GetSettings()->GetPcEngineConfig();

		// Check if color settings or palette changed
		bool optionsChanged = (_videoConfig.Hue != config.Hue ||
		                       _videoConfig.Saturation != config.Saturation ||
		                       _videoConfig.Contrast != config.Contrast ||
		                       _videoConfig.Brightness != config.Brightness ||
		                       memcmp(_pceConfig.Palette, pceConfig.Palette, sizeof(pceConfig.Palette)) != 0);

		if (optionsChanged) {
			InitLookupTable();
		}

		_videoConfig = config;
		_pceConfig = pceConfig;
	}

public:
	/// <summary>Constructor initializes palette tables.</summary>
	/// <param name="emu">Emulator instance for settings access</param>
	PceDefaultVideoFilter(Emulator* emu) : BaseVideoFilter(emu) {
		InitLookupTable();
	}

	/// <summary>Apply filter to VCE output buffer with variable resolution handling.</summary>
	/// <param name="ppuOutputBuffer">9-bit RGB VCE output with per-row clock dividers</param>
	void ApplyFilter(uint16_t* ppuOutputBuffer) override {
		if (_emu->GetRomInfo().Format == RomFormat::PceHes) {
			return;  // HES player doesn't need video rendering
		}

		uint32_t* out = GetOutputBuffer();
		FrameInfo frameInfo = _frameInfo;
		FrameInfo baseFrameInfo = _baseFrameInfo;
		OverscanDimensions overscan = BaseVideoFilter::GetOverscan();

		uint32_t yOffset = overscan.Top * PceConstants::MaxScreenWidth;
		constexpr uint32_t clockDividerOffset = PceConstants::MaxScreenWidth * PceConstants::ScreenHeight;

		uint32_t rowCount = PceConstants::ScreenHeight - overscan.Top - overscan.Bottom;

		uint32_t verticalScale = baseFrameInfo.Height / PceConstants::ScreenHeight;

		if (verticalScale != PceConstants::InternalResMultipler) {
			// Invalid data - wrong vertical scale
			return;
		}

		if (_frameDivider != 0) {
			// Use dynamic resolution (changes based on the screen content)
			// Makes video filters work properly
			for (uint32_t i = 0; i < rowCount; i++) {
				uint32_t xOffset = PceConstants::GetLeftOverscan(_frameDivider) + (overscan.Left * 4 / _frameDivider);
				uint32_t baseDstOffset = i * frameInfo.Width;
				uint32_t baseSrcOffset = i * PceConstants::MaxScreenWidth + yOffset + xOffset;
				// Direct 1:1 pixel copy for uniform resolution
				for (uint32_t j = 0; j < frameInfo.Width; j++) {
					out[baseDstOffset + j] = GetPixel(ppuOutputBuffer, baseSrcOffset + j);
				}
			}
		} else {
			// Always output at 4x scale (mixed resolution or forced)
			for (uint32_t i = 0; i < rowCount; i++) {
				uint8_t clockDivider = ppuOutputBuffer[clockDividerOffset + i + overscan.Top];
				uint32_t xOffset = PceConstants::GetLeftOverscan(clockDivider) + (overscan.Left * 4 / (clockDivider ? clockDivider : 4));
				uint32_t rowWidth = PceConstants::GetRowWidth(clockDivider);

				// Interpolate row data across the whole screen
				double ratio = (double)rowWidth / baseFrameInfo.Width;

				uint32_t baseDstOffset = i * verticalScale * frameInfo.Width;
				uint32_t baseSrcOffset = i * PceConstants::MaxScreenWidth + yOffset + xOffset;
				
				// Horizontal interpolation for variable width scanlines
				for (uint32_t j = 0; j < frameInfo.Width; j++) {
					out[baseDstOffset + j] = GetPixel(ppuOutputBuffer, baseSrcOffset + (int)(j * ratio));
				}

				// Duplicate row for vertical scale
				for (uint32_t j = 1; j < verticalScale; j++) {
					memcpy(out + baseDstOffset + (j * frameInfo.Width), out + baseDstOffset, frameInfo.Width * sizeof(uint32_t));
				}
			}
		}
	}
};