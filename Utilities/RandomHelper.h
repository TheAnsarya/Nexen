#pragma once
#include "pch.h"
#include <random>

/// <summary>
/// Random number generation utilities using Mersenne Twister algorithm (std::mt19937).
/// Provides simple interface for generating random integers and booleans.
/// </summary>
/// <remarks>
/// Uses std::random_device for seeding to ensure non-deterministic output.
/// WARNING: Each call to GetValue() creates new random_device and mt19937 engine (inefficient).
/// For performance-critical code, consider caching engine instance.
/// Mersenne Twister period: 2^19937-1 (extremely long before repeat).
/// </remarks>
class RandomHelper {
public:
	/// <summary>
	/// Generate random integer in closed interval [min, max].
	/// </summary>
	/// <typeparam name="T">Integer type (int, uint8_t, uint32_t, etc.)</typeparam>
	/// <param name="min">Minimum value (inclusive)</param>
	/// <param name="max">Maximum value (inclusive)</param>
	/// <returns>Random integer where min <= result <= max</returns>
	/// <remarks>
	/// Uses std::uniform_int_distribution for unbiased sampling.
	/// Creates new random engine on each call (consider caching for hot paths).
	/// Example: GetValue(1, 6) simulates dice roll.
	/// </remarks>
	template <typename T>
	static T GetValue(T min, T max) {
		std::random_device rd;
		std::mt19937 mt(rd());
		std::uniform_int_distribution<> dist(min, max);
		return dist(mt);
	}

	/// <summary>
	/// Generate random boolean (50% true, 50% false).
	/// </summary>
	/// <returns>Random boolean value</returns>
	/// <remarks>
	/// Implemented as GetValue(0, 1) == 1, so inherits same overhead.
	/// For frequent boolean generation, consider caching engine and using std::bernoulli_distribution.
	/// </remarks>
	static bool GetBool() {
		return GetValue(0, 1) == 1;
	}
};
