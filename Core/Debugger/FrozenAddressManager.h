#pragma once
#include "pch.h"

/// <summary>
/// Manager for frozen memory addresses (prevent writes).
/// </summary>
/// <remarks>
/// Architecture:
/// - Maintains set of frozen addresses
/// - Prevents emulation from modifying frozen values
/// - Range-based freeze/unfreeze operations
///
/// Freezing mechanism:
/// - Frozen addresses checked before write
/// - Write blocked if address frozen
/// - Original value maintained
///
/// Performance:
/// - Hash set for O(1) lookup
/// - Size check before lookup (fast path if empty)
///
/// Use cases:
/// - Infinite health/lives (freeze HP/lives addresses)
/// - Time freeze (freeze timer)
/// - Debug testing (hold specific values)
/// </remarks>
class FrozenAddressManager {
protected:
	unordered_set<uint32_t> _frozenAddresses; ///< Set of frozen addresses

public:
	/// <summary>
	/// Update frozen state for address range.
	/// </summary>
	/// <param name="start">Start address (inclusive)</param>
	/// <param name="end">End address (inclusive)</param>
	/// <param name="freeze">True to freeze, false to unfreeze</param>
	void UpdateFrozenAddresses(uint32_t start, uint32_t end, bool freeze) {
		if (freeze) {
			for (uint32_t i = start; i <= end; i++) {
				_frozenAddresses.emplace(i);
			}
		} else {
			for (uint32_t i = start; i <= end; i++) {
				_frozenAddresses.erase(i);
			}
		}
	}

	/// <summary>
	/// Check if address is frozen (hot path).
	/// </summary>
	/// <param name="addr">Address to check</param>
	/// <returns>True if frozen</returns>
	/// <remarks>
	/// Size check first for fast path when no addresses frozen.
	/// </remarks>
	bool IsFrozenAddress(uint32_t addr) {
		return _frozenAddresses.size() > 0 && _frozenAddresses.find(addr) != _frozenAddresses.end();
	}

	/// <summary>
	/// Get frozen state for address range.
	/// </summary>
	/// <param name="start">Start address</param>
	/// <param name="end">End address</param>
	/// <param name="outState">Output boolean array (frozen state per address)</param>
	void GetFrozenState(uint32_t start, uint32_t end, bool* outState) {
		for (uint32_t i = start; i <= end; i++) {
			outState[i - start] = _frozenAddresses.find(i) != _frozenAddresses.end();
		}
	}
};