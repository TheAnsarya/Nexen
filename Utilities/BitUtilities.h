#pragma once
#include "pch.h"

/// <summary>
/// Bit manipulation utilities using compile-time bit positions and forced inline expansion.
/// All methods are templated on bit position for optimal code generation.
/// Uses __forceinline to ensure zero-cost abstraction (no function call overhead).
/// </summary>
/// <remarks>
/// Template parameter bitNumber is constexpr evaluated at compile-time.
/// Generated code is identical to manual bit shifting/masking operations.
/// Common uses: Register manipulation, flag packing, CPU emulation.
/// </remarks>
class BitUtilities {
public:
	/// <summary>
	/// Set 8-bit value at specific bit position within larger integer.
	/// </summary>
	/// <typeparam name="bitNumber">Starting bit position (0-based, compile-time constant)</typeparam>
	/// <typeparam name="T">Destination integer type (uint8_t, uint16_t, uint32_t, etc.)</typeparam>
	/// <param name="dst">Reference to destination value (modified in-place)</param>
	/// <param name="src">8-bit value to insert at bit position</param>
	/// <remarks>
	/// Preserves all bits outside the 8-bit window at [bitNumber, bitNumber+7].
	/// Example: SetBits<8>(dst, 0x42) sets bits 8-15 to 0x42, preserves bits 0-7 and 16+.
	/// Forced inline ensures this compiles to optimal AND/OR sequence.
	/// </remarks>
	template <uint8_t bitNumber, typename T>
	__forceinline static void SetBits(T& dst, uint8_t src) {
		dst = (dst & ~(0xFF << bitNumber)) | (src << bitNumber);
	}

	/// <summary>
	/// Extract 8-bit value from specific bit position within larger integer.
	/// </summary>
	/// <typeparam name="bitNumber">Starting bit position (0-based, compile-time constant)</typeparam>
	/// <typeparam name="T">Source integer type (uint8_t, uint16_t, uint32_t, etc.)</typeparam>
	/// <param name="value">Source value to extract from</param>
	/// <returns>8-bit value extracted from bits [bitNumber, bitNumber+7]</returns>
	/// <remarks>
	/// Example: GetBits<8>(0x1234) returns 0x12 (bits 8-15).
	/// [[nodiscard]] prevents accidentally discarding extraction result.
	/// Forced inline ensures this compiles to single shift instruction.
	/// </remarks>
	template <uint8_t bitNumber, typename T>
	[[nodiscard]] __forceinline static uint8_t GetBits(T value) {
		return (uint8_t)(value >> bitNumber);
	}
};
