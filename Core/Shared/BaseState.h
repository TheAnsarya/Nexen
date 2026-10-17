#pragma once

/// <summary>
/// Base marker struct for serializable emulation state structures.
/// All state structs (CPU registers, PPU state, etc.) inherit from this.
/// </summary>
/// <remarks>
/// Empty base class serves as type marker for template metaprogramming.
/// Allows static_assert checks: static_assert(std::is_base_of_v<BaseState, T>)
/// Used by Serializer to distinguish state structs from other types.
/// Enables consistent handling of all emulation state across platforms.
/// </remarks>
struct BaseState {
};