#pragma once
#include "pch.h"

/// <summary>
/// Compile-time color format conversion utilities.
/// All functions are constexpr for zero-cost compile-time evaluation.
/// </summary>
/// <remarks>
/// Supports color depth conversions between:
/// - 2-bit (WonderSwan grayscale: 4 colors)
/// - 4-bit per channel (Genesis/SMS: 4096 colors RGB444)
/// - 5-bit per channel (SNES/GBA: 32768 colors RGB555)
/// - 8-bit per channel (Modern: 16.7M colors RGB888/ARGB8888)
///
/// Bit expansion preserves relative brightness:
/// - 2-bit to 8-bit: 0→0, 1→85, 2→170, 3→255 (evenly spaced)
/// - 4-bit to 8-bit: 0→0, 1→17, ... 15→255 (value * 17)
/// - 5-bit to 8-bit: 0→0, 1→8, ... 31→255 (value * 8 + value / 4)
///
/// All conversions marked [[nodiscard]] to prevent accidental discard.
/// </remarks>
class ColorUtilities {
public:
	/// <summary>
	/// Converts a 5-bit color component to 8-bit depth.
	/// </summary>
	/// <param name="color">5-bit color value (0-31)</param>
	/// <returns>8-bit color value (0-255) with full range</returns>
	/// <remarks>
	/// Uses the formula: (color << 3) + (color >> 2)
	/// This ensures the full 0-255 range is utilized by filling lower bits with upper bits.
	/// Commonly used for SNES/GBA RGB555 format conversion.
	/// </remarks>
	[[nodiscard]] static constexpr uint8_t Convert5BitTo8Bit(uint8_t color) {
		return (color << 3) + (color >> 2);
	}

	/// <summary>
	/// Converts a 4-bit color component to 8-bit depth.
	/// </summary>
	/// <param name="color">4-bit color value (0-15)</param>
	/// <returns>8-bit color value (0-255) with full range</returns>
	/// <remarks>
	/// Uses the formula: (color << 4) | color
	/// This duplicates the 4-bit value in both nibbles (e.g., 0xF becomes 0xFF).
	/// Commonly used for Genesis/SMS RGB444 format conversion.
	/// </remarks>
	[[nodiscard]] static constexpr uint8_t Convert4BitTo8Bit(uint8_t color) {
		return (color << 4) | color;
	}

	/// <summary>
	/// Converts RGB555 (15-bit) color to ARGB8888 (32-bit) with full alpha.
	/// </summary>
	/// <param name="rgb555">RGB555 color in format: xBBBBBGGGGGRRRRR (MSB unused)</param>
	/// <returns>ARGB8888 color in format: AAAAAAAARRRRRRRRGGGGGGGGBBBBBBBB</returns>
	/// <remarks>
	/// RGB555 layout:
	///   Bits 10-14: Blue (5 bits)
	///   Bits 5-9:   Green (5 bits)
	///   Bits 0-4:   Red (5 bits)
	///
	/// Used by SNES, GBA, and other 16-bit systems.
	/// Alpha channel is always 0xFF (fully opaque).
	/// This is a constexpr function that can be evaluated at compile time.
	/// </remarks>
	[[nodiscard]] static constexpr uint32_t Rgb555ToArgb(uint16_t rgb555) {
		uint8_t b = Convert5BitTo8Bit((rgb555 >> 10) & 0x1F);
		uint8_t g = Convert5BitTo8Bit((rgb555 >> 5) & 0x1F);
		uint8_t r = Convert5BitTo8Bit(rgb555 & 0x1F);

		return 0xFF000000 | (r << 16) | (g << 8) | b;
	}

	/// <summary>
	/// Extracts RGB components from RGB555 (15-bit) color to individual 8-bit values.
	/// </summary>
	/// <param name="rgb555">RGB555 color in format: xBBBBBGGGGGRRRRR (MSB unused)</param>
	/// <param name="r">Output red component (0-255)</param>
	/// <param name="g">Output green component (0-255)</param>
	/// <param name="b">Output blue component (0-255)</param>
	/// <remarks>
	/// Each 5-bit component is expanded to 8-bit using Convert5BitTo8Bit.
	/// Useful when individual color channels need to be processed separately.
	/// </remarks>
	static constexpr void Rgb555ToRgb(uint16_t rgb555, uint8_t& r, uint8_t& g, uint8_t& b) {
		b = Convert5BitTo8Bit((rgb555 >> 10) & 0x1F);
		g = Convert5BitTo8Bit((rgb555 >> 5) & 0x1F);
		r = Convert5BitTo8Bit(rgb555 & 0x1F);
	}

	/// <summary>
	/// Converts 2-bit-per-channel RGB (6-bit total) to RGB555 format.
	/// </summary>
	/// <param name="value">RGB222 value with 2 bits per channel (bits 5-0: RRGGBB)</param>
	/// <returns>RGB555 value with 5 bits per channel</returns>
	/// <remarks>
	/// Expands each 2-bit color component to 5 bits by replicating and shifting.
	/// Used by WonderSwan LCD grayscale modes.
	/// </remarks>
	[[nodiscard]] static constexpr uint16_t Rgb222To555(uint8_t value) {
		return (
		    ((value & 0x30) << 9) | ((value & 0x30) << 7) | ((value & 0x20) << 5) |
		    ((value & 0x0C) << 6) | ((value & 0x0C) << 4) | ((value & 0x08) << 2) |
		    ((value & 0x03) << 3) | ((value & 0x03) << 1) | ((value & 0x02) >> 1));
	}

	/// <summary>
	/// Converts RGB444 (12-bit) color to RGB555 (15-bit) format.
	/// </summary>
	/// <param name="value">RGB444 color with 4 bits per channel (bits 11-0: RRRRGGGGBBBB)</param>
	/// <returns>RGB555 value with 5 bits per channel</returns>
	/// <remarks>
	/// Expands each 4-bit color component to 5 bits by replicating the MSB.
	/// Used by Sega Genesis/Master System VDP.
	/// </remarks>
	[[nodiscard]] static constexpr uint16_t Rgb444To555(uint16_t value) {
		return (
		    ((value & 0xF00) << 3) | ((value & 0x800) >> 1) |
		    ((value & 0x0F0) << 2) | ((value & 0x080) >> 2) |
		    ((value & 0x00F) << 1) | ((value & 0x008) >> 3));
	}

	/// <summary>
	/// Converts RGB222 (6-bit) color directly to ARGB8888 (32-bit).
	/// </summary>
	/// <param name="rgb222">RGB222 color with 2 bits per channel</param>
	/// <returns>ARGB8888 color with full alpha</returns>
	/// <remarks>
	/// Convenience function that chains Rgb222To555 and Rgb555ToArgb.
	/// </remarks>
	[[nodiscard]] static constexpr uint32_t Rgb222ToArgb(uint8_t rgb222) {
		return Rgb555ToArgb(Rgb222To555(rgb222));
	}

	/// <summary>
	/// Converts RGB444 (12-bit) color directly to ARGB8888 (32-bit).
	/// </summary>
	/// <param name="rgb444">RGB444 color with 4 bits per channel</param>
	/// <returns>ARGB8888 color with full alpha</returns>
	/// <remarks>
	/// Convenience function that chains Rgb444To555 and Rgb555ToArgb.
	/// Used for Genesis/SMS palette conversion.
	/// </remarks>
	[[nodiscard]] static constexpr uint32_t Rgb444ToArgb(uint16_t rgb444) {
		return Rgb555ToArgb(Rgb444To555(rgb444));
	}

	/// <summary>
	/// Converts BGR444 (12-bit, reversed channel order) to ARGB8888 (32-bit).
	/// </summary>
	/// <param name="bgr444">BGR444 color in format: BBBBGGGGRRRR (blue in MSB)</param>
	/// <returns>ARGB8888 color with full alpha</returns>
	/// <remarks>
	/// Note the reversed channel order: BGR instead of RGB.
	/// Each 4-bit component is duplicated to fill 8 bits.
	/// Used by some PPU color modes.
	/// </remarks>
	[[nodiscard]] static constexpr uint32_t Bgr444ToArgb(uint16_t bgr444) {
		uint8_t b = (bgr444 & 0x00F);
		uint8_t g = (bgr444 & 0x0F0) >> 4;
		uint8_t r = (bgr444 & 0xF00) >> 8;
		return 0xFF000000 | (r << 20) | (r << 16) | (g << 12) | (g << 8) | (b << 4) | (b << 0);
	}

	/// <summary>
	/// Converts ARGB8888 (32-bit) color to RGB555 (15-bit) format.
	/// </summary>
	/// <param name="rgb888">ARGB8888 color (alpha channel is ignored)</param>
	/// <returns>RGB555 color with 5 bits per channel</returns>
	/// <remarks>
	/// Extracts the 5 most significant bits from each 8-bit color component.
	/// This is the inverse operation of Rgb555ToArgb.
	/// Information loss occurs due to bit depth reduction (24-bit → 15-bit).
	/// </remarks>
	[[nodiscard]] static constexpr uint16_t Rgb888To555(uint32_t rgb888) {
		uint8_t r = (rgb888 >> 19) & 0x1F;
		uint8_t g = (rgb888 >> 11) & 0x1F;
		uint8_t b = (rgb888 >> 3) & 0x1F;

		return (b << 10) | (g << 5) | r;
	}
};
