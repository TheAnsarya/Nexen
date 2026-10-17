#pragma once
#include "pch.h"
#include "Shared/Video/BaseVideoFilter.h"
#include "NES/HdPacks/HdNesPack.h"

class Emulator;
class NesConsole;
struct HdPackData;

/// <summary>
/// NES HD Pack video filter - renders HD graphics replacements.
/// Replaces PPU output with high-resolution replacement graphics.
/// </summary>
/// <remarks>
/// **HD Packs:**
/// HD packs are community-created asset packs that replace NES graphics
/// with high-resolution equivalents. They can include:
/// - Tile replacements (CHR graphics → PNG images)
/// - Palette remapping
/// - Animation enhancements
/// - Background layer additions
///
/// **Pipeline:**
/// 1. PPU outputs tile indices and attributes
/// 2. HdNesPack matches tiles to HD replacements
/// 3. Renders HD graphics at configured scale
/// 4. Blends with non-replaced areas
/// 5. Output to frame buffer
///
/// **Scaling:**
/// - HD packs define their scale factor (1x-8x typical)
/// - Output resolution = base_res × scale
/// - Example: 256×240 → 2048×1920 at 8x scale
///
/// **Matching:**
/// - Tiles matched by CHR data (tile graphics)
/// - Palette matched for color-specific variants
/// - Position matching for context-aware replacements
///
/// **Features:**
/// - Tile replacement (sprites and backgrounds)
/// - Background layers (parallax, decorations)
/// - Palette-specific replacements
/// - Conditional rules (game state triggers)
///
/// **Performance:**
/// - Pre-loaded HD textures
/// - Fast tile matching via hash tables
/// - Scale-dependent memory usage
/// </remarks>
class HdVideoFilter : public BaseVideoFilter {
private:
	HdPackData* _hdData;  ///< HD pack data (tiles, rules, settings)
	unique_ptr<BaseHdNesPack> _hdNesPack = nullptr;  ///< HD pack renderer

public:
	/// <summary>Constructor initializes HD pack renderer.</summary>
	/// <param name="console">NES console instance</param>
	/// <param name="emu">Emulator instance for settings</param>
	/// <param name="hdData">HD pack data to use</param>
	HdVideoFilter(NesConsole* console, Emulator* emu, HdPackData* hdData);

	/// <summary>Default destructor.</summary>
	virtual ~HdVideoFilter() = default;

	/// <summary>Apply HD pack rendering to PPU output.</summary>
	/// <param name="ppuOutputBuffer">PPU tile/attribute output</param>
	void ApplyFilter(uint16_t* ppuOutputBuffer) override;

	/// <summary>Get scaled frame dimensions.</summary>
	/// <returns>Frame dimensions (scaled by HD pack factor)</returns>
	FrameInfo GetFrameInfo() override;

	/// <summary>Get scaled overscan dimensions.</summary>
	/// <returns>Overscan dimensions (scaled by HD pack factor)</returns>
	OverscanDimensions GetOverscan() override;
};
