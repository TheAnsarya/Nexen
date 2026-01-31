#pragma once
#include "pch.h"
#include "PCE/PceTypes.h"
#include "Utilities/ISerializable.h"

class PceConsole;
class Emulator;

/// <summary>
/// PC Engine Video Color Encoder (VCE) - HuC6260.
/// Handles color palette, clock divider, and grayscale mode.
/// </summary>
/// <remarks>
/// The VCE provides:
/// - 512-entry palette RAM (9-bit colors, 512 unique colors)
/// - Clock divider selection (affects horizontal resolution)
/// - Scanline count selection (262 or 263 lines)
/// - Grayscale mode
///
/// **Palette Organization:**
/// - 32 palettes × 16 colors each = 512 entries
/// - First 16 palettes for backgrounds, next 16 for sprites
/// - Color format: GRB (3-3-3 bits)
///
/// **Clock Dividers:**
/// - /4: 256 pixels/line (low resolution)
/// - /3: 341 pixels/line (medium resolution)
/// - /2: 512 pixels/line (high resolution)
/// </remarks>
class PceVce final : public ISerializable {
private:
	/// <summary>VCE register state.</summary>
	PceVceState _state = {};

	/// <summary>Emulator instance.</summary>
	Emulator* _emu = nullptr;

	/// <summary>Console instance.</summary>
	PceConsole* _console = nullptr;

	/// <summary>512-entry palette RAM.</summary>
	uint16_t* _paletteRam = nullptr;

public:
	/// <summary>Constructs VCE with emulator reference.</summary>
	PceVce(Emulator* emu, PceConsole* console);
	~PceVce();

	/// <summary>Gets scanlines per frame (262 or 263).</summary>
	[[nodiscard]] uint16_t GetScanlineCount() { return _state.ScanlineCount; }

	/// <summary>Gets clock divider (2, 3, or 4).</summary>
	[[nodiscard]] uint8_t GetClockDivider() { return _state.ClockDivider; }

	/// <summary>Checks if grayscale mode is enabled.</summary>
	[[nodiscard]] bool IsGrayscale() { return _state.Grayscale; }

	/// <summary>Gets a palette entry by index.</summary>
	[[nodiscard]] uint16_t GetPalette(uint16_t addr) { return _paletteRam[addr]; }

	/// <summary>Gets reference to VCE state.</summary>
	PceVceState& GetState() { return _state; }

	/// <summary>Reads from VCE register.</summary>
	uint8_t Read(uint16_t addr);

	/// <summary>Writes to VCE register.</summary>
	void Write(uint16_t addr, uint8_t value);

	/// <summary>Serializes VCE state for save states.</summary>
	void Serialize(Serializer& s) override;
};
