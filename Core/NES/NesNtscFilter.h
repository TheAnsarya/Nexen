#pragma once
#include "pch.h"
#include <memory>
#include "NES/NesTypes.h"
#include "Shared/Video/BaseVideoFilter.h"
#include "Utilities/NTSC/nes_ntsc.h"

class Emulator;

/// <summary>
/// NES NTSC composite video filter using Blargg's nes_ntsc library.
/// Simulates authentic CRT display with NTSC color generation.
/// </summary>
/// <remarks>
/// **Pipeline:**
/// 1. PPU outputs 6-bit palette indices (64 colors)
/// 2. nes_ntsc generates NTSC composite signal
/// 3. Simulates analog decoding artifacts
/// 4. Outputs 32-bit ARGB at ~2x horizontal resolution
///
/// **NES-Specific Features:**
/// - Emphasis bits affect NTSC encoding (not just RGB)
/// - PPU model differences (2C02, 2C03, 2C07, etc.)
/// - Authentic color generation from luma/chroma
/// - Row 0D handling (PPU "blacker than black")
///
/// **PPU Model Support:**
/// - 2C02 (NTSC NES, original)
/// - 2C03/2C04/2C05 (VS System variants)
/// - 2C07 (PAL NES)
/// - Dendy (Soviet clone)
///
/// **Configuration:**
/// - _palette: Current palette (512 entries with emphasis)
/// - _ppuModel: PPU variant for color generation
/// - _nesConfig: NES-specific settings
/// </remarks>
class NesNtscFilter : public BaseVideoFilter {
private:
	nes_ntsc_setup_t _ntscSetup = {};               ///< NTSC filter setup parameters
	nes_ntsc_t _ntscData = {};                      ///< Pre-computed NTSC conversion tables
	std::unique_ptr<uint32_t[]> _ntscBuffer;        ///< Intermediate filtered output buffer
	PpuModel _ppuModel = PpuModel::Ppu2C02;         ///< Current PPU model for color generation
	uint8_t _palette[512 * 3] = {};                 ///< RGB palette (64 colors × 8 emphasis)
	NesConfig _nesConfig = {};                      ///< NES configuration snapshot

protected:
	/// <summary>Update filter parameters and palette before applying.</summary>
	void OnBeforeApplyFilter() override;

	/// <summary>Get filtered frame dimensions.</summary>
	/// <returns>Frame info with ~2x horizontal resolution</returns>
	FrameInfo GetFrameInfo() override;

public:
	/// <summary>Constructor initializes NTSC filter tables.</summary>
	/// <param name="emu">Emulator instance for settings access</param>
	NesNtscFilter(Emulator* emu);

	/// <summary>Destructor releases filter resources.</summary>
	virtual ~NesNtscFilter();

	/// <summary>Get overscan dimensions for NTSC output.</summary>
	/// <returns>Overscan crop amounts (NES games often have garbage at edges)</returns>
	OverscanDimensions GetOverscan() override;

	/// <summary>Get HUD scale factors for NTSC resolution.</summary>
	/// <returns>Scale factors for OSD rendering</returns>
	HudScaleFactors GetScaleFactor() override;

	/// <summary>Apply NTSC filter to PPU output.</summary>
	/// <param name="ppuOutputBuffer">6-bit palette index PPU output</param>
	void ApplyFilter(uint16_t* ppuOutputBuffer) override;
};