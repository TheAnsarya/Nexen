#pragma once
#include "pch.h"
#include <memory>
#include "Shared/Video/BaseVideoFilter.h"
#include "Shared/Video/GenericNtscFilter.h"

class WsConsole;
class Emulator;

/// <summary>
/// WonderSwan default video filter - converts 12-bit RGB to 32-bit ARGB.
/// Supports LCD color adjustment and optional NTSC filtering.
/// </summary>
/// <remarks>
/// **Pipeline:**
/// 1. PPU outputs 12-bit RGB (RGB444 format)
/// 2. Look up pre-computed 32-bit ARGB from palette table
/// 3. Apply optional WS LCD color adjustment
/// 4. Apply brightness/contrast/hue/saturation adjustments
/// 5. Optional frame blending for LCD ghosting simulation
/// 6. Optional NTSC filter (via GenericNtscFilter)
/// 7. Output 32-bit ARGB
///
/// **WonderSwan Models:**
/// - WS (Original): Monochrome LCD, 8 shades
/// - WSC (Color): 12-bit color LCD
/// - SwanCrystal: Improved color LCD
///
/// **LCD Color Adjustment:**
/// - WS/WSC LCDs have unique color characteristics
/// - _adjustColors enables gamma/color curve correction
/// - Makes colors appear more like original hardware
///
/// **Frame Blending:**
/// - Simulates LCD persistence/ghosting
/// - Important for games designed around LCD response
/// - Handles frame size changes gracefully
///
/// **NTSC Filter:**
/// - Optional composite video simulation
/// - Uses GenericNtscFilter for RGB systems
/// - For TV-out simulation
///
/// **Resolution:**
/// - WS: 224×144 (portrait) or 144×224 (landscape)
/// - Orientation can change mid-game
/// - _prevFrameSize tracks size changes for blending
///
/// **Color Space:**
/// - Input: 12-bit RGB (4 bits per channel)
/// - Palette table: 4,096 entries (0x1000)
/// - Output: 32-bit ARGB
/// </remarks>
class WsDefaultVideoFilter final : public BaseVideoFilter {
private:
	Emulator* _emu = nullptr;        ///< Emulator instance for settings
	WsConsole* _console = nullptr;   ///< Console instance for orientation detection
	uint32_t _calculatedPalette[0x1000] = {};  ///< Pre-computed ARGB palette (4K entries for 12-bit)
	VideoConfig _videoConfig = {};    ///< Video settings snapshot

	FrameInfo _prevFrameSize = {};               ///< Previous frame size for blend buffer management
	std::unique_ptr<uint16_t[]> _prevFrame;      ///< Previous frame for blending
	bool _blendFrames = false;                    ///< Enable LCD frame blending
	bool _adjustColors = false;                   ///< Apply WS LCD color correction

	bool _applyNtscFilter = false;  ///< Enable NTSC composite simulation
	GenericNtscFilter _ntscFilter;  ///< NTSC filter for CRT effects

	/// <summary>Fast pixel blending using bit manipulation (50% blend).</summary>
	/// <param name="a">First pixel ARGB</param>
	/// <param name="b">Second pixel ARGB</param>
	/// <returns>Blended pixel ARGB</returns>
	uint32_t BlendPixels(uint32_t a, uint32_t b);

	/// <summary>Initialize 4K-entry RGB lookup table from settings.</summary>
	void InitLookupTable();

protected:
	/// <summary>Get output frame dimensions (may change with orientation).</summary>
	/// <returns>Frame dimensions</returns>
	FrameInfo GetFrameInfo() override;

	/// <summary>Update settings and manage frame buffer before filter.</summary>
	void OnBeforeApplyFilter() override;

public:
	/// <summary>Constructor initializes palette tables and NTSC filter.</summary>
	/// <param name="emu">Emulator instance for settings access</param>
	/// <param name="console">WonderSwan console for orientation detection</param>
	/// <param name="applyNtscFilter">Enable NTSC composite simulation</param>
	WsDefaultVideoFilter(Emulator* emu, WsConsole* console, bool applyNtscFilter);

	/// <summary>Destructor releases frame buffer.</summary>
	~WsDefaultVideoFilter();

	/// <summary>Apply filter to PPU output buffer.</summary>
	/// <param name="ppuOutputBuffer">12-bit RGB PPU output</param>
	void ApplyFilter(uint16_t* ppuOutputBuffer) override;
};