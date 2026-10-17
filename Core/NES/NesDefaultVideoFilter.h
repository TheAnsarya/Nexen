#pragma once

#include "pch.h"
#include "Shared/Video/BaseVideoFilter.h"
#include "NES/NesTypes.h"

class NesConsole;

/// <summary>
/// NES default video filter - converts palette indices to RGB output.
/// No NTSC simulation, just direct palette lookup with adjustments.
/// </summary>
/// <remarks>
/// **Pipeline:**
/// 1. PPU outputs 6-bit palette indices (64 colors)
/// 2. Apply emphasis bits (8 combinations × 64 = 512 total colors)
/// 3. Look up RGB values from calculated palette
/// 4. Apply brightness/contrast/hue adjustments
/// 5. Output 32-bit ARGB
///
/// **Palette Generation:**
/// - Base 64-color palette from PPU model
/// - 8 emphasis bit combinations (R/G/B emphasis)
/// - Generates 512-entry lookup table
/// - Supports custom palette files
///
/// **PPU Model Support:**
/// - 2C02 (NTSC NES) - Standard palette
/// - 2C03/2C04/2C05 (VS System) - Variant palettes
/// - 2C07 (PAL NES) - Different color timing
/// - Dendy - Soviet clone palette
///
/// **Performance:**
/// - Pre-computed _calculatedPalette lookup table
/// - Single pass through PPU buffer
/// - No NTSC artifacts (faster than NesNtscFilter)
/// </remarks>
class NesDefaultVideoFilter : public BaseVideoFilter {
private:
	uint32_t _calculatedPalette[512] = {};  ///< Pre-computed RGB palette (64 colors × 8 emphasis)
	VideoConfig _videoConfig = {};           ///< Video settings snapshot
	NesConfig _nesConfig = {};               ///< NES-specific settings snapshot
	PpuModel _ppuModel = PpuModel::Ppu2C02;  ///< Current PPU model

	/// <summary>Initialize palette lookup table from settings.</summary>
	void InitLookupTable();

protected:
	/// <summary>Convert PPU palette indices to RGB.</summary>
	/// <param name="ppuOutputBuffer">6-bit palette index buffer</param>
	/// <param name="outputBuffer">32-bit ARGB output</param>
	void DecodePpuBuffer(uint16_t* ppuOutputBuffer, uint32_t* outputBuffer);

	/// <summary>Update palette table before applying filter.</summary>
	void OnBeforeApplyFilter() override;

public:
	/// <summary>Constructor initializes palette tables.</summary>
	/// <param name="emu">Emulator instance for settings access</param>
	NesDefaultVideoFilter(Emulator* emu);

	/// <summary>Apply PAL border to frame (PAL NES has visible border).</summary>
	/// <param name="ppuOutputBuffer">PPU output to modify</param>
	static void ApplyPalBorder(uint16_t* ppuOutputBuffer);

	/// <summary>Generate full 512-entry palette (64 colors × 8 emphasis).</summary>
	/// <param name="paletteBuffer">Output palette buffer</param>
	/// <param name="model">PPU model (affects emphasis behavior)</param>
	static void GenerateFullColorPalette(uint32_t paletteBuffer[512], PpuModel model = PpuModel::Ppu2C02);

	/// <summary>Get full palette for specific configuration.</summary>
	/// <param name="palette">Output palette buffer</param>
	/// <param name="nesCfg">NES configuration</param>
	/// <param name="model">PPU model</param>
	static void GetFullPalette(uint32_t palette[512], NesConfig& nesCfg, PpuModel model);

	/// <summary>Get default brightness for palette entry.</summary>
	/// <param name="colorIndex">Palette color index (0-511)</param>
	/// <param name="model">PPU model</param>
	/// <returns>Brightness value (0-255)</returns>
	static uint32_t GetDefaultPixelBrightness(uint16_t colorIndex, PpuModel model);

	/// <summary>Apply filter to PPU output.</summary>
	/// <param name="ppuOutputBuffer">6-bit palette index PPU output</param>
	void ApplyFilter(uint16_t* ppuOutputBuffer) override;
};
