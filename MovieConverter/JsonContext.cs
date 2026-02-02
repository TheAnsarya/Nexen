using System.Text.Json.Serialization;

namespace Nexen.MovieConverter;

/// <summary>
/// JSON serialization context for AOT compilation support.
/// Uses source generation for trimming and native AOT compatibility.
/// </summary>
[JsonSourceGenerationOptions(
	WriteIndented = true,
	PropertyNamingPolicy = JsonKnownNamingPolicy.CamelCase,
	DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull,
	UseStringEnumConverter = true)]
[JsonSerializable(typeof(MovieMetadata))]
[JsonSerializable(typeof(List<MovieMarker>))]
[JsonSerializable(typeof(List<EmbeddedScript>))]
[JsonSerializable(typeof(Dictionary<string, string>))]
[JsonSerializable(typeof(NexenMovieHeader))]
internal sealed partial class MovieJsonContext : JsonSerializerContext {
}

/// <summary>
/// Header metadata stored in movie.json inside the archive
/// </summary>
public sealed record NexenMovieHeader {
	/// <summary>Format version</summary>
	public required string FormatVersion { get; init; }

	/// <summary>Emulator version that created the movie</summary>
	public string? EmulatorVersion { get; init; }

	/// <summary>Author of the TAS</summary>
	public string? Author { get; init; }

	/// <summary>Description/comments</summary>
	public string? Description { get; init; }

	/// <summary>Game name</summary>
	public string? GameName { get; init; }

	/// <summary>ROM filename</summary>
	public string? RomFileName { get; init; }

	/// <summary>Internal ROM name from header</summary>
	public string? InternalRomName { get; init; }

	/// <summary>Game database ID</summary>
	public string? GameDatabaseId { get; init; }

	/// <summary>Target system</summary>
	public SystemType SystemType { get; init; }

	/// <summary>Region (NTSC/PAL)</summary>
	public RegionType Region { get; init; }

	/// <summary>Timing mode</summary>
	public TimingMode TimingMode { get; init; }

	/// <summary>Sync mode</summary>
	public SyncMode SyncMode { get; init; }

	/// <summary>How the movie starts</summary>
	public StartType StartType { get; init; }

	/// <summary>Total number of frames</summary>
	public int TotalFrames { get; init; }

	/// <summary>Re-record count</summary>
	public ulong RerecordCount { get; init; }

	/// <summary>Lag frame count</summary>
	public ulong LagFrameCount { get; init; }

	/// <summary>Number of blank frames</summary>
	public int BlankFrames { get; init; }

	/// <summary>Number of controller ports</summary>
	public int ControllerCount { get; init; }

	/// <summary>Controller types per port</summary>
	public ControllerType[]? PortTypes { get; init; }

	/// <summary>Whether controllers are swapped</summary>
	public bool ControllersSwapped { get; init; }

	/// <summary>SHA-1 of ROM</summary>
	public string? Sha1Hash { get; init; }

	/// <summary>SHA-256 of ROM</summary>
	public string? Sha256Hash { get; init; }

	/// <summary>MD5 of ROM</summary>
	public string? Md5Hash { get; init; }

	/// <summary>CRC32 of ROM</summary>
	public uint? Crc32 { get; init; }

	/// <summary>ROM size in bytes</summary>
	public long? RomSize { get; init; }

	/// <summary>BIOS hash if required</summary>
	public string? BiosHash { get; init; }

	/// <summary>BizHawk core settings</summary>
	public string? CoreSettings { get; init; }

	/// <summary>BizHawk sync settings</summary>
	public string? SyncSettings { get; init; }

	/// <summary>Original movie format</summary>
	public MovieFormat SourceFormat { get; init; }

	/// <summary>Original format version</summary>
	public string? SourceFormatVersion { get; init; }

	/// <summary>Creation date</summary>
	public DateTime? CreatedDate { get; init; }

	/// <summary>Last modified date</summary>
	public DateTime? ModifiedDate { get; init; }

	/// <summary>Whether movie is verified</summary>
	public bool? IsVerified { get; init; }

	/// <summary>Verification notes</summary>
	public string? VerificationNotes { get; init; }

	/// <summary>Final state hash for verification</summary>
	public string? FinalStateHash { get; init; }

	/// <summary>VS System DIP switches</summary>
	public byte? VsDipSwitches { get; init; }

	/// <summary>VS System PPU type</summary>
	public byte? VsPpuType { get; init; }

	/// <summary>Extra data dictionary</summary>
	public Dictionary<string, string>? ExtraData { get; init; }
}

/// <summary>
/// Metadata for movie archives (without binary data)
/// </summary>
public sealed record MovieMetadata {
	/// <summary>Author</summary>
	public string? Author { get; init; }

	/// <summary>Description</summary>
	public string? Description { get; init; }

	/// <summary>Game name</summary>
	public string? GameName { get; init; }

	/// <summary>ROM filename</summary>
	public string? RomFileName { get; init; }

	/// <summary>System type</summary>
	public SystemType SystemType { get; init; }

	/// <summary>Region</summary>
	public RegionType Region { get; init; }

	/// <summary>ROM SHA-1</summary>
	public string? Sha1Hash { get; init; }

	/// <summary>ROM CRC32</summary>
	public uint? Crc32 { get; init; }
}
