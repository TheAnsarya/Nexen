#pragma once

/// <summary>
/// Compile-time loop unrolling using template metaprogramming and C++17 if constexpr.
/// Generates optimal code by evaluating loop at compile-time with zero runtime overhead.
/// </summary>
/// <typeparam name="first">Starting index (inclusive)</typeparam>
/// <typeparam name="last">Ending index (exclusive)</typeparam>
/// <remarks>
/// Usage pattern:
/// <code>
/// StaticFor<0, 8>::Apply([&](auto i) {
///     // i is std::integral_constant<int, N> - can be used in constexpr context
///     array[i] = process(i);
/// });
/// </code>
///
/// Benefits over runtime loop:
/// - Complete loop unrolling (no branch overhead)
/// - Index value known at compile-time (enables further optimizations)
/// - Can be used with compile-time arrays and constexpr functions
///
/// Uses if constexpr (C++17) for zero-cost compile-time branching.
/// Recursive template instantiation terminates with specialized StaticFor<n, n> base case.
/// </remarks>
template <int first, int last>
struct StaticFor {
	/// <summary>
	/// Apply callback function for each integer in range [first, last).
	/// </summary>
	/// <typeparam name="callback">Functor or lambda type accepting std::integral_constant<int, N></typeparam>
	/// <param name="f">Callback function to invoke for each index</param>
	/// <remarks>
	/// Callback receives std::integral_constant<int, i> for each iteration.
	/// This allows using the index value in constexpr contexts within the callback.
	/// Entire loop is unrolled at compile-time - no runtime iteration overhead.
	/// </remarks>
	template <typename callback>
	static inline constexpr void Apply(callback const& f) {
		if constexpr (first < last) {
			f(std::integral_constant<int, first>{});
			StaticFor<first + 1, last>::Apply(f);
		}
	}
};

/// <summary>
/// Template specialization for empty range (base case for recursion termination).
/// </summary>
/// <typeparam name="n">Index where first == last</typeparam>
template <int n>
struct StaticFor<n, n> {
	/// <summary>
	/// No-op for empty range (recursion base case).
	/// </summary>
	/// <typeparam name="callback">Callback type (unused)</typeparam>
	/// <param name="f">Callback function (not invoked)</param>
	template <typename callback>
	static inline constexpr void Apply(callback const& f) {}
};