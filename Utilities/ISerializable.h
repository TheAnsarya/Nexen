#pragma once

class Serializer;

/// <summary>
/// Result codes for deserialization operations.
/// Used to distinguish between different failure modes when loading save states.
/// </summary>
enum class DeserializeResult {
	/// <summary>Deserialization succeeded - state loaded successfully</summary>
	Success,
	
	/// <summary>Invalid file format - not a valid save state file</summary>
	/// <remarks>Wrong magic number, corrupted header, or incompatible version</remarks>
	InvalidFile,
	
	/// <summary>Specific error occurred during deserialization</summary>
	/// <remarks>
	/// File format is valid but specific component failed to deserialize.
	/// Error details should be logged separately.
	/// </remarks>
	SpecificError,
};

/// <summary>
/// Interface for objects that support serialization to save states.
/// All emulated components (CPU, PPU, RAM, etc.) implement this interface.
/// </summary>
/// <remarks>
/// Save states use a binary serialization format with versioning support.
/// Implementations must handle both serialization (save) and deserialization (load)
/// in the same Serialize() method using Serializer direction flags.
/// 
/// Design pattern: Single method for save/load reduces code duplication and ensures
/// symmetric serialization (all saved data is loaded, all loaded data is saved).
/// </remarks>
class ISerializable {
public:
	/// <summary>
	/// Serialize or deserialize object state based on Serializer mode.
	/// </summary>
	/// <param name="s">Serializer object (determines save vs load direction)</param>
	/// <remarks>
	/// Implementation pattern:
	/// <code>
	/// void MyClass::Serialize(Serializer& s) {
	///     s.Serialize(_register1);
	///     s.Serialize(_register2);
	///     s.Serialize(_memoryArray, MemorySize);
	/// }
	/// </code>
	/// 
	/// Same code handles both save and load - Serializer tracks direction internally.
	/// Order of Serialize() calls must remain consistent across versions.
	/// Use Serializer versioning support for backward compatibility.
	/// </remarks>
	virtual void Serialize(Serializer& s) = 0;
};
