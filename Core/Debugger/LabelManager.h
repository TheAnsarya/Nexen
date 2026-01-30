#pragma once
#include "pch.h"
#include <unordered_map>
#include <functional>
#include "Debugger/DebugTypes.h"

class Debugger;

/// <summary>
/// Fast hasher for address keys in label lookups.
/// </summary>
class AddressHasher {
public:
	/// <summary>
	/// Hash function for 64-bit address keys.
	/// </summary>
	/// <param name="addr">Packed address key (memory type + address)</param>
	/// <returns>Hash value</returns>
	/// <remarks>
	/// Identity hash (address is already well-distributed).
	/// </remarks>
	size_t operator()(const uint64_t& addr) const {
		// Quick hash for addresses
		return addr;
	}
};

/// <summary>
/// Label and comment information for memory address.
/// </summary>
struct LabelInfo {
	string Label;   ///< Symbol label (e.g., "PlayerX", "UpdateSprite")
	string Comment; ///< Code comment (e.g., "// Move player right")
};

/// <summary>
/// Manages symbol labels and code comments for debugger.
/// </summary>
/// <remarks>
/// Architecture:
/// - Single LabelManager shared by all CPUs
/// - Labels stored per (MemoryType, Address) pair
/// - Bidirectional lookup: address → label and label → address
///
/// Label sources:
/// - Imported from debug symbol files (.dbg, .mlb, Mesen labels)
/// - User-added labels in debugger UI
/// - Auto-generated labels (subroutine_XXXX)
/// - Register labels (special CPU registers)
///
/// Label format:
/// - Valid C identifier (alphanumeric + underscore)
/// - Case-sensitive
/// - No duplicate labels
/// - Max length ~256 characters
///
/// Memory type scoping:
/// - Labels scoped to memory type (ROM, RAM, SRAM, etc.)
/// - Same label can exist in different memory types
/// - Example: "PlayerData" in ROM (table) and RAM (instance)
///
/// Use cases:
/// - Disassembly display (show labels instead of addresses)
/// - Conditional breakpoints ("break at PlayerUpdate")
/// - Expression evaluation ("[PlayerX] > 100")
/// - Code comments in disassembly view
/// </remarks>
class LabelManager {
private:
	unordered_map<uint64_t, LabelInfo, AddressHasher> _codeLabels; ///< Address → Label/Comment map
	unordered_map<string, uint64_t> _codeLabelReverseLookup;       ///< Label → Address map

	Debugger* _debugger; ///< Parent debugger instance

	/// <summary>
	/// Pack memory type and address into 64-bit key.
	/// </summary>
	/// <param name="absoluteAddr">Absolute address</param>
	/// <param name="memType">Memory type</param>
	/// <returns>Packed key for hash map</returns>
	/// <remarks>
	/// Key format:
	/// - Upper 16 bits: Memory type enum
	/// - Lower 48 bits: Address
	/// </remarks>
	int64_t GetLabelKey(uint32_t absoluteAddr, MemoryType memType);

	/// <summary>
	/// Extract memory type from packed key.
	/// </summary>
	/// <param name="key">Packed label key</param>
	/// <returns>Memory type</returns>
	MemoryType GetKeyMemoryType(uint64_t key);

	/// <summary>
	/// Internal label lookup (no register labels).
	/// </summary>
	/// <param name="address">Address info</param>
	/// <param name="label">Output label string</param>
	/// <returns>True if label found</returns>
	bool InternalGetLabel(AddressInfo address, string& label);

public:
	/// <summary>
	/// Constructor for label manager.
	/// </summary>
	/// <param name="debugger">Main debugger instance</param>
	LabelManager(Debugger* debugger);

	/// <summary>
	/// Set label and comment for address.
	/// </summary>
	/// <param name="address">Memory address</param>
	/// <param name="memType">Memory type</param>
	/// <param name="label">Symbol label (empty to remove)</param>
	/// <param name="comment">Code comment (empty to remove)</param>
	/// <remarks>
	/// Label validation:
	/// - Must be valid C identifier
	/// - No duplicates allowed
	/// - Empty label removes entry
	///
	/// Updates:
	/// - _codeLabels hash map (address → label)
	/// - _codeLabelReverseLookup (label → address)
	/// </remarks>
	void SetLabel(uint32_t address, MemoryType memType, string label, string comment);

	/// <summary>
	/// Clear all labels and comments.
	/// </summary>
	void ClearLabels();

	/// <summary>
	/// Get absolute address for label.
	/// </summary>
	/// <param name="label">Label to lookup</param>
	/// <returns>Address info or invalid address if not found</returns>
	AddressInfo GetLabelAbsoluteAddress(string& label);

	/// <summary>
	/// Get CPU-relative address for label.
	/// </summary>
	/// <param name="label">Label to lookup</param>
	/// <param name="cpuType">Target CPU type</param>
	/// <returns>Relative address or -1 if not found</returns>
	int32_t GetLabelRelativeAddress(string& label, CpuType cpuType);

	/// <summary>
	/// Get label for address.
	/// </summary>
	/// <param name="address">Address info</param>
	/// <param name="checkRegisterLabels">True to check CPU register labels</param>
	/// <returns>Label string or empty if no label</returns>
	/// <remarks>
	/// Register labels:
	/// - Special labels for CPU registers ($2000 = "PPUCTRL", etc.)
	/// - Platform-specific (NES registers, SNES registers, etc.)
	/// </remarks>
	string GetLabel(AddressInfo address, bool checkRegisterLabels = true);

	/// <summary>
	/// Get comment for address.
	/// </summary>
	/// <param name="absAddress">Absolute address info</param>
	/// <returns>Comment string or empty if no comment</returns>
	string GetComment(AddressInfo absAddress);

	/// <summary>
	/// Get both label and comment for address.
	/// </summary>
	/// <param name="address">Address info</param>
	/// <param name="label">Output label info</param>
	/// <returns>True if label or comment found</returns>
	bool GetLabelAndComment(AddressInfo address, LabelInfo& label);

	/// <summary>
	/// Check if label exists.
	/// </summary>
	/// <param name="label">Label to check</param>
	/// <returns>True if label defined</returns>
	bool ContainsLabel(string& label);

	/// <summary>
	/// Check if address has label or comment.
	/// </summary>
	/// <param name="address">Address info</param>
	/// <returns>True if label or comment exists</returns>
	bool HasLabelOrComment(AddressInfo address);
};
