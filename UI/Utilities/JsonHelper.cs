using System;
using System.IO;
using System.Text.Json;

namespace Nexen.Utilities;

/// <summary>
/// Provides JSON serialization utilities using System.Text.Json source generators.
/// </summary>
/// <remarks>
/// Uses <see cref="NexenSerializerContext"/> for AOT-compatible, high-performance
/// JSON serialization throughout the application.
/// </remarks>
public static class JsonHelper {
	/// <summary>
	/// Creates a deep copy of an object via JSON serialization.
	/// </summary>
	/// <typeparam name="T">The type of object to clone.</typeparam>
	/// <param name="obj">The object to clone.</param>
	/// <returns>A new instance with the same property values.</returns>
	/// <exception cref="Exception">Thrown if deserialization fails.</exception>
	/// <remarks>
	/// This method serializes the object to JSON bytes and then deserializes back,
	/// creating a completely independent copy. Uses UTF-8 bytes for efficiency.
	/// </remarks>
	public static T Clone<T>(T obj) where T : notnull {
		using MemoryStream stream = new MemoryStream();
		byte[] jsonData = JsonSerializer.SerializeToUtf8Bytes(obj, obj.GetType(), NexenSerializerContext.Default);
		T? clone = (T?)JsonSerializer.Deserialize(jsonData.AsSpan<byte>(), obj.GetType(), NexenSerializerContext.Default);
		if (clone is null) {
			throw new Exception("Invalid object");
		}

		return clone;
	}
}
