using System.Text.Json.Serialization;
using Nexen.Config;
using Nexen.Debugger;
using Nexen.Debugger.Labels;
using Nexen.Debugger.Utilities;
using Nexen.Utilities;
using Nexen.ViewModels;

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
[JsonSerializable(typeof(GitHubRelease))]
[JsonSerializable(typeof(GitHubAsset))]
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
