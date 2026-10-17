#pragma once
#include "pch.h"
#include <memory>
#include "Shared/Video/BaseVideoFilter.h"
#include "Utilities/NTSC/snes_ntsc.h"

class Emulator;

/// <summary>
/// SNES NTSC composite video filter using Blargg's snes_ntsc library.
/// Simulates authentic CRT display with NTSC artifacts.
/// </summary>
/// <remarks>
/// **Pipeline:**
/// 1. PPU outputs 15-bit RGB (5 bits per channel)
/// 2. snes_ntsc converts to NTSC composite signal
/// 3. Simulates analog video artifacts (dot crawl, color bleeding)
/// 4. Outputs 32-bit ARGB at ~2x horizontal resolution
///
/// **Visual Effects:**
/// - Color fringing from composite encoding
/// - Dot crawl (moving artifacts on sharp edges)
/// - Color bleeding (adjacent colors blend)
/// - Scanline simulation
/// - Authentic CRT phosphor response
///
/// **Performance:**
/// - Pre-computed lookup tables for real-time filtering
/// - ~2x horizontal scaling (artifacts need subpixel resolution)
/// - Efficient SIMD implementation in snes_ntsc library
///
/// **Configuration:**
/// - _ntscSetup: Filter parameters (sharpness, resolution, artifacts)
/// - _ntscData: Pre-computed lookup tables
/// - _ntscBuffer: Intermediate filtered output
/// </remarks>
class SnesNtscFilter : public BaseVideoFilter {
private:
	snes_ntsc_setup_t _ntscSetup = {};               ///< NTSC filter setup parameters
	snes_ntsc_t _ntscData = {};                      ///< Pre-computed NTSC conversion tables
	std::unique_ptr<uint32_t[]> _ntscBuffer;         ///< Intermediate filtered output buffer

protected:
	/// <summary>Update filter parameters before applying.</summary>
	void OnBeforeApplyFilter() override;

public:
	/// <summary>Constructor initializes NTSC filter tables.</summary>
	/// <param name="emu">Emulator instance for settings access</param>
	SnesNtscFilter(Emulator* emu);

	/// <summary>Destructor releases filter resources.</summary>
	virtual ~SnesNtscFilter();

	/// <summary>Apply NTSC filter to PPU output.</summary>
	/// <param name="ppuOutputBuffer">15-bit RGB PPU output</param>
	void ApplyFilter(uint16_t* ppuOutputBuffer) override;

	/// <summary>Get filtered frame dimensions.</summary>
	/// <returns>Frame info with ~2x horizontal resolution</returns>
	FrameInfo GetFrameInfo() override;

	/// <summary>Get overscan dimensions for NTSC output.</summary>
	/// <returns>Overscan crop amounts (edges hidden by CRT bezel)</returns>
	OverscanDimensions GetOverscan() override;

	/// <summary>Get HUD scale factors for NTSC resolution.</summary>
	/// <returns>Scale factors for OSD rendering</returns>
	HudScaleFactors GetScaleFactor() override;
};