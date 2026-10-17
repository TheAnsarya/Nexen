#pragma once
#include "pch.h"
#include <memory>
#include "Shared/Video/BaseVideoFilter.h"
#include "Shared/Video/GenericNtscFilter.h"
#include "Shared/SettingTypes.h"

class Emulator;

/// <summary>
/// Game Boy default video filter - converts 15-bit RGB to 32-bit ARGB.
/// Supports GBC LCD color adjustment and optional NTSC filtering.
/// </summary>
/// <remarks>
/// **Pipeline:**
/// 1. PPU outputs 15-bit RGB (after palette lookup for DMG/GBC)
/// 2. Look up pre-computed 32-bit ARGB from palette table
/// 3. Apply optional GBC LCD color adjustment
/// 4. Apply brightness/contrast/hue/saturation adjustments
/// 5. Optional frame blending for LCD ghosting simulation
/// 6. Optional NTSC filter (via GenericNtscFilter)
/// 7. Output 32-bit ARGB
///
/// **Game Boy Models:**
/// - DMG (Original): 4 shades of green, converted to RGB via palette
/// - GBP (Pocket): 4 shades of gray
/// - GBC (Color): 15-bit RGB direct output
/// - GBA mode: Slightly different color characteristics
///
/// **GBC LCD Color Adjustment:**
/// - GBC LCD has unique color characteristics
/// - _gbcAdjustColors enables gamma/color curve correction
/// - Makes colors appear more like original hardware
///
/// **Frame Blending:**
/// - Simulates LCD persistence/ghosting
/// - Essential for games that exploit LCD response time
/// - Many GB games use rapid flicker for transparency effects
///
/// **NTSC Filter:**
/// - Optional composite video simulation
/// - Uses GenericNtscFilter for 15-bit RGB systems
/// - Useful for Super Game Boy simulation
///
/// **Resolution:**
/// - Native: 160×144
/// - No scaling variations (fixed resolution system)
///
/// **Color Space:**
/// - Input: 15-bit BGR555
/// - Palette table: 32,768 entries (0x8000)
/// - Output: 32-bit ARGB
/// </remarks>
class GbDefaultVideoFilter : public BaseVideoFilter {
private:
	uint32_t _calculatedPalette[0x8000] = {};  ///< Pre-computed ARGB palette (32K entries)
	VideoConfig _videoConfig = {};              ///< Video settings snapshot

	std::unique_ptr<uint16_t[]> _prevFrame;  ///< Previous frame for blending
	bool _blendFrames = false;                ///< Enable LCD frame blending
	bool _gbcAdjustColors = false;            ///< Apply GBC LCD color correction

	bool _applyNtscFilter = false;  ///< Enable NTSC composite simulation
	GenericNtscFilter _ntscFilter;  ///< NTSC filter for CRT effects

	/// <summary>Initialize 32K-entry RGB lookup table from settings.</summary>
	void InitLookupTable();

	/// <summary>Fast pixel blending using bit manipulation (50% blend).</summary>
	/// <param name="a">First pixel ARGB</param>
	/// <param name="b">Second pixel ARGB</param>
	/// <returns>Blended pixel ARGB</returns>
	__forceinline static uint32_t BlendPixels(uint32_t a, uint32_t b);

	/// <summary>Get pixel with optional frame blending.</summary>
	/// <param name="ppuFrame">PPU output buffer</param>
	/// <param name="offset">Pixel offset</param>
	/// <returns>32-bit ARGB pixel</returns>
	__forceinline uint32_t GetPixel(uint16_t* ppuFrame, uint32_t offset);

protected:
	/// <summary>Update settings before applying filter.</summary>
	void OnBeforeApplyFilter() override;

	/// <summary>Get output frame dimensions.</summary>
	/// <returns>Frame dimensions (160×144 native)</returns>
	FrameInfo GetFrameInfo() override;

public:
	/// <summary>Constructor initializes palette tables and NTSC filter.</summary>
	/// <param name="emu">Emulator instance for settings access</param>
	/// <param name="applyNtscFilter">Enable NTSC composite simulation</param>
	GbDefaultVideoFilter(Emulator* emu, bool applyNtscFilter);

	/// <summary>Destructor releases frame buffer.</summary>
	~GbDefaultVideoFilter();

	/// <summary>Apply filter to PPU output buffer.</summary>
	/// <param name="ppuOutputBuffer">15-bit RGB PPU output</param>
	void ApplyFilter(uint16_t* ppuOutputBuffer) override;
};