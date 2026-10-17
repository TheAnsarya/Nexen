#pragma once
#include "pch.h"
#include "Shared/Video/BaseVideoFilter.h"

class Emulator;
class LynxConsole;

/// <summary>
/// Atari Lynx default video filter — copies 32-bit ARGB frame buffer
/// with optional rotation (0°, 90° left, 90° right).
///
/// The Lynx PPU (Mikey) outputs a pre-composited 32-bit ARGB frame buffer
/// from its 4096-color palette (12-bit RGB, 4 bits per channel).
/// This filter applies:
///   1. Screen rotation (based on LNX header or user override)
///   2. Brightness/contrast/hue/saturation adjustment
///   3. Output as 32-bit ARGB
///
/// Resolution:
///   - Native: 160×102 (landscape)
///   - Rotated: 102×160 (portrait, for games like Gauntlet: The Third Encounter)
/// </summary>
class LynxDefaultVideoFilter final : public BaseVideoFilter {
private:
	Emulator* _emu = nullptr;
	LynxConsole* _console = nullptr;
	VideoConfig _videoConfig = {};

	/// <summary>Pre-computed ARGB palette with adjustments applied</summary>
	/// <remarks>
	/// Lynx uses 12-bit color (4096 entries). Mikey already applies the palette,
	/// so this table is used only for brightness/contrast/hue/saturation adjustment.
	/// Entry format: each index encodes 4-bit R, G, B → full 32-bit ARGB.
	/// </remarks>
	uint32_t _adjustedPalette[0x1000] = {};
	bool _useAdjustedPalette = false;

	void InitLookupTable();

public:
	LynxDefaultVideoFilter(Emulator* emu, LynxConsole* console);
	~LynxDefaultVideoFilter() override = default;

	FrameInfo GetFrameInfo() override;
	void OnBeforeApplyFilter() override;
	void ApplyFilter(uint16_t* ppuOutputBuffer) override;
};
