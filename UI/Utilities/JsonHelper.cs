using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Threading.Tasks;
using Nexen.Config;
using Nexen.Debugger;
using Nexen.Debugger.Labels;
using Nexen.Debugger.Utilities;
using Nexen.Utilities;
using Nexen.ViewModels;

namespace Nexen.Utilities {
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
}

/// <summary>
/// Source-generated JSON serializer context for Nexen's core types.
/// </summary>
/// <remarks>
/// <para>
/// This context enables AOT (Ahead-of-Time) compilation compatibility and
/// improved serialization performance by generating serialization code at compile time.
/// </para>
/// <para>
/// Configured with:
/// <list type="bullet">
///   <item><description>WriteIndented = true for human-readable output</description></item>
///   <item><description>IgnoreReadOnlyProperties = true to skip computed properties</description></item>
///   <item><description>UseStringEnumConverter = true for readable enum values</description></item>
/// </list>
/// </para>
/// </remarks>
[JsonSerializable(typeof(Configuration))]
[JsonSerializable(typeof(Breakpoint))]
[JsonSerializable(typeof(CodeLabel))]
[JsonSerializable(typeof(GameDipSwitches))]
[JsonSerializable(typeof(CheatCodes))]
[JsonSerializable(typeof(GameConfig))]
[JsonSerializable(typeof(DebugWorkspace))]
[JsonSerializable(typeof(UpdateInfo))]
[JsonSourceGenerationOptions(
	WriteIndented = true,
	IgnoreReadOnlyProperties = true,
	UseStringEnumConverter = true
)]
public partial class NexenSerializerContext : JsonSerializerContext { }

/// <summary>
/// Source-generated JSON serializer context using camelCase naming policy.
/// </summary>
/// <remarks>
/// Used for types that need camelCase property names in JSON output,
/// typically for compatibility with external JSON formats or APIs.
/// </remarks>
[JsonSerializable(typeof(DocEntryViewModel[]))]
[JsonSerializable(typeof(DocFileFormat))]
[JsonSerializable(typeof(CheatDatabase))]
[JsonSourceGenerationOptions(
	WriteIndented = true,
	IgnoreReadOnlyProperties = true,
	UseStringEnumConverter = true,
	PropertyNamingPolicy = JsonKnownNamingPolicy.CamelCase
)]
public partial class NexenCamelCaseSerializerContext : JsonSerializerContext { }
