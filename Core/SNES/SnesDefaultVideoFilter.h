#pragma once

#include "pch.h"
#include "Shared/Video/BaseVideoFilter.h"
#include "Shared/SettingTypes.h"

/// <summary>
/// SNES default video filter - converts 15-bit RGB to 32-bit ARGB output.
/// Handles high-resolution mode blending and fixed resolution output.
/// </summary>
/// <remarks>
/// **Pipeline:**
/// 1. PPU outputs 15-bit RGB (BGR555 format)
/// 2. Look up pre-computed 32-bit ARGB from palette table
/// 3. Apply brightness/contrast/hue/saturation adjustments
/// 4. Handle hi-res mode pixel blending (512px → 256px)
/// 5. Output 32-bit ARGB
///
/// **High-Resolution Modes:**
/// - SNES supports 256x224 and 512x224 modes
/// - Mode 5/6: 512-wide interlaced graphics
/// - Hi-res blending averages adjacent pixels for non-hi-res displays
/// - _blendHighRes enables/disables this averaging
///
/// **Fixed Resolution:**
/// - _forceFixedRes forces consistent output size
/// - Useful for video recording and screenshots
/// - Prevents frame size changes during gameplay
///
/// **Color Space:**
/// - Input: 15-bit BGR555 (5 bits each R/G/B)
/// - Palette table: 32,768 entries (0x8000)
/// - Output: 32-bit ARGB (0xAARRGGBB)
///
/// **Performance:**
/// - Pre-computed 32K-entry lookup table
/// - BlendPixels uses fast bit manipulation
/// - GetPixel handles both normal and hi-res modes
/// </remarks>
class SnesDefaultVideoFilter : public BaseVideoFilter {
private:
	uint32_t _calculatedPalette[0x8000] = {};  ///< Pre-computed ARGB palette (32K entries for 15-bit RGB)
	VideoConfig _videoConfig = {};              ///< Video settings snapshot

	bool _blendHighRes = false;   ///< Blend hi-res mode pixels (512 → 256 width)
	bool _forceFixedRes = false;  ///< Force consistent output resolution

	/// <summary>Initialize 32K-entry RGB lookup table from settings.</summary>
	void InitLookupTable();

	/// <summary>Fast pixel blending using bit manipulation.</summary>
	/// <param name="a">First pixel ARGB</param>
	/// <param name="b">Second pixel ARGB</param>
	/// <returns>Blended pixel ARGB</returns>
	__forceinline static constexpr uint32_t BlendPixels(uint32_t a, uint32_t b);

	/// <summary>Get pixel with optional hi-res blending.</summary>
	/// <param name="ppuFrame">PPU output buffer</param>
	/// <param name="offset">Pixel offset</param>
	/// <returns>32-bit ARGB pixel</returns>
	__forceinline uint32_t GetPixel(uint16_t* ppuFrame, uint32_t offset);

protected:
	/// <summary>Update lookup table if settings changed.</summary>
	void OnBeforeApplyFilter() override;

	/// <summary>Get output frame dimensions (may vary with hi-res mode).</summary>
	/// <returns>Frame dimensions</returns>
	FrameInfo GetFrameInfo() override;

public:
	/// <summary>Constructor initializes palette tables.</summary>
	/// <param name="emu">Emulator instance for settings access</param>
	SnesDefaultVideoFilter(Emulator* emu);

	/// <summary>Apply filter to PPU output buffer.</summary>
	/// <param name="ppuOutputBuffer">15-bit RGB PPU output</param>
	void ApplyFilter(uint16_t* ppuOutputBuffer) override;

	/// <summary>Get overscan dimensions adjusted for hi-res mode.</summary>
	/// <returns>Adjusted overscan dimensions</returns>
	OverscanDimensions GetOverscan() override;
};
