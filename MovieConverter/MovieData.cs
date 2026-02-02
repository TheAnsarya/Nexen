using System.IO.Hashing;
using System.Runtime.CompilerServices;
using System.Text;
using System.Text.Json.Serialization;

namespace Nexen.MovieConverter;

/// <summary>
/// Common movie data structure that can be converted between TAS formats.
/// Contains all metadata and input frames for a TAS movie.
/// Designed to be a superset of all supported format features.
/// </summary>
public sealed class MovieData {
	// ========== Metadata ==========

	/// <summary>Author(s) of the TAS</summary>
	public string Author { get; set; } = "";

	/// <summary>Description or comments about the TAS</summary>
	public string Description { get; set; } = "";

	/// <summary>Name of the game</summary>
	public string GameName { get; set; } = "";

	/// <summary>ROM filename (without path)</summary>
	public string RomFileName { get; set; } = "";

	/// <summary>Internal ROM name (from header)</summary>
	public string? InternalRomName { get; set; }

	/// <summary>Game database ID (e.g., No-Intro DAT entry)</summary>
	public string? GameDatabaseId { get; set; }

	// ========== ROM Identification ==========

	/// <summary>SHA-1 hash of the ROM (40 hex chars)</summary>
	public string? Sha1Hash { get; set; }

	/// <summary>SHA-256 hash of the ROM (64 hex chars)</summary>
	public string? Sha256Hash { get; set; }

	/// <summary>MD5 hash of the ROM (32 hex chars)</summary>
	public string? Md5Hash { get; set; }

	/// <summary>CRC32 hash of the ROM</summary>
	public uint? Crc32 { get; set; }

	/// <summary>ROM file size in bytes</summary>
	public long? RomSize { get; set; }

	// ========== System/Timing ==========

	/// <summary>Target console/system</summary>
	public SystemType SystemType { get; set; } = SystemType.Snes;

	/// <summary>Video region (NTSC/PAL)</summary>
	public RegionType Region { get; set; } = RegionType.NTSC;

	/// <summary>Timing mode for frame counting</summary>
	public TimingMode TimingMode { get; set; } = TimingMode.AllFrames;

	/// <summary>Synchronization mode for playback</summary>
	public SyncMode SyncMode { get; set; } = SyncMode.Frame;

	/// <summary>Core/emulator settings string (for BizHawk compatibility)</summary>
	public string? CoreSettings { get; set; }

	/// <summary>Sync settings string (for BizHawk compatibility)</summary>
	public string? SyncSettings { get; set; }

	/// <summary>Frame rate based on region</summary>
	[JsonIgnore]
	public double FrameRate => Region switch {
		RegionType.PAL => SystemType switch {
			SystemType.Nes => 50.006978,
			SystemType.Snes => 50.006979,
			SystemType.Gb or SystemType.Gbc => 59.7275,
			_ => 50.0
		},
		RegionType.Dendy => 50.006978,
		_ => SystemType switch {
			SystemType.Nes => 60.098814,
			SystemType.Snes => 60.098814,
			SystemType.Gb or SystemType.Gbc => 59.7275,
			SystemType.Gba => 59.7275,
			_ => 60.0
		}
	};

	// ========== Movie Statistics ==========

	/// <summary>Number of times the movie was re-recorded (savestates loaded during recording)</summary>
	public ulong RerecordCount { get; set; }

	/// <summary>Number of lag frames (frames where input wasn't polled)</summary>
	public ulong LagFrameCount { get; set; }

	/// <summary>Total number of input frames</summary>
	[JsonIgnore]
	public int TotalFrames => InputFrames.Count;

	/// <summary>Movie duration based on frame count and rate</summary>
	[JsonIgnore]
	public TimeSpan Duration => TimeSpan.FromSeconds(TotalFrames / FrameRate);

	/// <summary>Number of blank frames at start (for lsnes compatibility)</summary>
	public int BlankFrames { get; set; }

	// ========== Controller Configuration ==========

	/// <summary>Number of active controller ports</summary>
	public int ControllerCount { get; set; } = 2;

	/// <summary>Controller type for each port</summary>
	public ControllerType[] PortTypes { get; set; } = new ControllerType[InputFrame.MaxPorts];

	/// <summary>Whether controllers are swapped (for SNES)</summary>
	public bool ControllersSwapped { get; set; }

	// ========== Movie Data ==========

	/// <summary>All input frames in the movie</summary>
	public List<InputFrame> InputFrames { get; set; } = [];

	/// <summary>Subframes for cycle-accurate input (optional)</summary>
	public List<SubframeInput>? Subframes { get; set; }

	// ========== Save State / SRAM ==========

	/// <summary>How the movie starts</summary>
	public StartType StartType { get; set; } = StartType.PowerOn;

	/// <summary>Initial savestate data (for movies starting from savestate)</summary>
	public byte[]? InitialState { get; set; }

	/// <summary>Initial SRAM data (for movies starting from SRAM)</summary>
	public byte[]? InitialSram { get; set; }

	/// <summary>Initial RTC (real-time clock) data</summary>
	public byte[]? InitialRtc { get; set; }

	/// <summary>Initial memory card data (for PlayStation, etc.)</summary>
	public byte[]? InitialMemoryCard { get; set; }

	/// <summary>BIOS file hash (when BIOS is required)</summary>
	public string? BiosHash { get; set; }

	/// <summary>Whether the movie starts from a savestate</summary>
	[JsonIgnore]
	public bool StartsFromSavestate => InitialState is { Length: > 0 };

	/// <summary>Whether the movie starts from SRAM</summary>
	[JsonIgnore]
	public bool StartsFromSram => InitialSram is { Length: > 0 };

	/// <summary>Whether the movie starts from a clean power-on state</summary>
	[JsonIgnore]
	public bool StartsFromPowerOn => !StartsFromSavestate && !StartsFromSram;

	// ========== Source Format Info ==========

	/// <summary>Original format this movie was loaded from</summary>
	public MovieFormat SourceFormat { get; set; }

	/// <summary>Version of the source format</summary>
	public string? SourceFormatVersion { get; set; }

	/// <summary>Emulator that created the movie</summary>
	public string? EmulatorVersion { get; set; }

	/// <summary>Date the movie was created</summary>
	public DateTime? CreatedDate { get; set; }

	/// <summary>Date the movie was last modified</summary>
	public DateTime? ModifiedDate { get; set; }

	// ========== Verification ==========

	/// <summary>Whether movie is verified to sync</summary>
	public bool? IsVerified { get; set; }

	/// <summary>Verification notes</summary>
	public string? VerificationNotes { get; set; }

	/// <summary>Final state hash (for verification)</summary>
	public string? FinalStateHash { get; set; }

	// ========== Platform-Specific ==========

	/// <summary>FDS: Disk sides in the movie</summary>
	public List<FdsDiskSide>? FdsDiskSides { get; set; }

	/// <summary>VS System: DIP switch settings</summary>
	public byte? VsDipSwitches { get; set; }

	/// <summary>VS System: PPU type</summary>
	public byte? VsPpuType { get; set; }

	/// <summary>Extra/custom data (for format-specific extensions)</summary>
	public Dictionary<string, string>? ExtraData { get; set; }

	// ========== Lua Scripting ==========

	/// <summary>Embedded Lua scripts</summary>
	public List<EmbeddedScript>? Scripts { get; set; }

	// ========== Annotations ==========

	/// <summary>Markers/bookmarks at specific frames</summary>
	public List<MovieMarker>? Markers { get; set; }

	/// <summary>Branch points (for branching TAS)</summary>
	public List<BranchPoint>? BranchPoints { get; set; }

	// ========== Constructor ==========

	public MovieData() {
		// Default: first two ports are gamepads, rest are disconnected
		for (int i = 0; i < PortTypes.Length; i++) {
			PortTypes[i] = i < 2 ? ControllerType.Gamepad : ControllerType.None;
		}
	}

	// ========== Methods ==========

	/// <summary>
	/// Add an input frame to the movie
	/// </summary>
	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public void AddFrame(InputFrame frame) {
		frame.FrameNumber = InputFrames.Count;
		InputFrames.Add(frame);
	}

	/// <summary>
	/// Add a marker at the specified frame
	/// </summary>
	public void AddMarker(int frameNumber, string label, string? description = null) {
		Markers ??= [];
		Markers.Add(new MovieMarker {
			FrameNumber = frameNumber,
			Label = label,
			Description = description
		});
	}

	/// <summary>
	/// Get frame at the specified index
	/// </summary>
	/// <param name="index">0-based frame index</param>
	/// <returns>The input frame, or null if out of range</returns>
	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public InputFrame? GetFrame(int index) {
		if ((uint)index >= (uint)InputFrames.Count) {
			return null;
		}

		return InputFrames[index];
	}

	/// <summary>
	/// Truncate movie to the specified frame (for rerecording)
	/// </summary>
	/// <param name="frameNumber">Last frame to keep (inclusive)</param>
	public void TruncateAt(int frameNumber) {
		if (frameNumber < 0) {
			InputFrames.Clear();
			return;
		}

		if (frameNumber < InputFrames.Count) {
			InputFrames.RemoveRange(frameNumber + 1, InputFrames.Count - frameNumber - 1);
		}
	}

	/// <summary>
	/// Calculate the CRC32 of the input data (for verification)
	/// </summary>
	public uint CalculateInputCrc32() {
		var crc = new Crc32();
		Span<byte> buffer = stackalloc byte[2];

		foreach (InputFrame frame in InputFrames) {
			for (int i = 0; i < ControllerCount; i++) {
				ushort smv = frame.Controllers[i].ToSmvFormat();
				buffer[0] = (byte)(smv & 0xff);
				buffer[1] = (byte)(smv >> 8);
				crc.Append(buffer);
			}
		}

		return crc.GetCurrentHashAsUInt32();
	}

	/// <summary>
	/// Get a human-readable summary of the movie
	/// </summary>
	public string GetSummary() {
		var sb = new StringBuilder(1024);

		sb.AppendLine("Movie Summary");
		sb.AppendLine("═════════════════════════════════════════");
		sb.AppendLine();

		// Source Info
		sb.AppendLine($"Source Format:   {SourceFormat} {SourceFormatVersion}");
		if (!string.IsNullOrEmpty(EmulatorVersion)) {
			sb.AppendLine($"Emulator:        {EmulatorVersion}");
		}

		sb.AppendLine();

		// Game Info
		sb.AppendLine($"System:          {SystemType} ({Region})");
		if (!string.IsNullOrEmpty(GameName)) {
			sb.AppendLine($"Game:            {GameName}");
		}

		if (!string.IsNullOrEmpty(RomFileName)) {
			sb.AppendLine($"ROM:             {RomFileName}");
		}

		if (!string.IsNullOrEmpty(Author)) {
			sb.AppendLine($"Author:          {Author}");
		}

		sb.AppendLine();

		// Statistics
		sb.AppendLine("Statistics");
		sb.AppendLine("───────────────────────────────────────────");
		sb.AppendLine($"Total Frames:    {TotalFrames:N0}");
		sb.AppendLine($"Duration:        {Duration:hh\\:mm\\:ss\\.fff}");
		sb.AppendLine($"Rerecords:       {RerecordCount:N0}");
		sb.AppendLine($"Lag Frames:      {LagFrameCount:N0}");
		sb.AppendLine();

		// Start State
		sb.AppendLine("Start State");
		sb.AppendLine("───────────────────────────────────────────");
		sb.AppendLine($"Start Type:      {StartType}");
		sb.AppendLine($"From Savestate:  {StartsFromSavestate}");
		sb.AppendLine($"From SRAM:       {StartsFromSram}");
		sb.AppendLine($"Clean Start:     {StartsFromPowerOn}");
		sb.AppendLine();

		// Controllers
		sb.AppendLine($"Controllers:     {ControllerCount}");
		for (int i = 0; i < ControllerCount; i++) {
			sb.AppendLine($"  Port {i + 1}:        {PortTypes[i]}");
		}

		// ROM Hashes
		if (Crc32.HasValue || !string.IsNullOrEmpty(Sha1Hash) || !string.IsNullOrEmpty(Md5Hash)) {
			sb.AppendLine();
			sb.AppendLine("ROM Identification");
			sb.AppendLine("───────────────────────────────────────────");
			if (Crc32.HasValue) {
				sb.AppendLine($"CRC32:           {Crc32:x8}");
			}

			if (!string.IsNullOrEmpty(Md5Hash)) {
				sb.AppendLine($"MD5:             {Md5Hash.ToLowerInvariant()}");
			}

			if (!string.IsNullOrEmpty(Sha1Hash)) {
				sb.AppendLine($"SHA-1:           {Sha1Hash.ToLowerInvariant()}");
			}
		}

		// Markers
		if (Markers is { Count: > 0 }) {
			sb.AppendLine();
			sb.AppendLine($"Markers:         {Markers.Count}");
		}

		return sb.ToString();
	}

	/// <summary>
	/// Create a deep copy of this movie
	/// </summary>
	public MovieData Clone() {
		var clone = new MovieData {
			Author = Author,
			Description = Description,
			GameName = GameName,
			RomFileName = RomFileName,
			InternalRomName = InternalRomName,
			GameDatabaseId = GameDatabaseId,
			Sha1Hash = Sha1Hash,
			Sha256Hash = Sha256Hash,
			Md5Hash = Md5Hash,
			Crc32 = Crc32,
			RomSize = RomSize,
			SystemType = SystemType,
			Region = Region,
			TimingMode = TimingMode,
			SyncMode = SyncMode,
			CoreSettings = CoreSettings,
			SyncSettings = SyncSettings,
			RerecordCount = RerecordCount,
			LagFrameCount = LagFrameCount,
			BlankFrames = BlankFrames,
			ControllerCount = ControllerCount,
			ControllersSwapped = ControllersSwapped,
			StartType = StartType,
			BiosHash = BiosHash,
			SourceFormat = SourceFormat,
			SourceFormatVersion = SourceFormatVersion,
			EmulatorVersion = EmulatorVersion,
			CreatedDate = CreatedDate,
			ModifiedDate = ModifiedDate,
			IsVerified = IsVerified,
			VerificationNotes = VerificationNotes,
			FinalStateHash = FinalStateHash,
			VsDipSwitches = VsDipSwitches,
			VsPpuType = VsPpuType
		};

		// Clone port types
		Array.Copy(PortTypes, clone.PortTypes, PortTypes.Length);

		// Clone frames
		clone.InputFrames = new List<InputFrame>(InputFrames.Count);
		foreach (InputFrame frame in InputFrames) {
			clone.InputFrames.Add(frame.Clone());
		}

		// Clone state data
		if (InitialState != null) {
			clone.InitialState = [.. InitialState];
		}

		if (InitialSram != null) {
			clone.InitialSram = [.. InitialSram];
		}

		if (InitialRtc != null) {
			clone.InitialRtc = [.. InitialRtc];
		}

		if (InitialMemoryCard != null) {
			clone.InitialMemoryCard = [.. InitialMemoryCard];
		}

		// Clone collections
		if (Markers != null) {
			clone.Markers = [.. Markers];
		}

		if (BranchPoints != null) {
			clone.BranchPoints = [.. BranchPoints];
		}

		if (FdsDiskSides != null) {
			clone.FdsDiskSides = [.. FdsDiskSides];
		}

		if (Scripts != null) {
			clone.Scripts = [.. Scripts];
		}

		if (ExtraData != null) {
			clone.ExtraData = new Dictionary<string, string>(ExtraData);
		}

		return clone;
	}

	public override string ToString() =>
		$"{GameName} - {TotalFrames} frames ({Duration:mm\\:ss\\.ff}) by {Author}";
}

/// <summary>
/// FDS disk side information
/// </summary>
public sealed record FdsDiskSide {
	/// <summary>Disk number (0-based)</summary>
	public int DiskNumber { get; init; }

	/// <summary>Side (A=0, B=1)</summary>
	public int Side { get; init; }

	/// <summary>Frame when this disk side is inserted</summary>
	public int InsertFrame { get; init; }
}

/// <summary>
/// Movie marker/bookmark
/// </summary>
public sealed record MovieMarker {
	/// <summary>Frame number of the marker</summary>
	public int FrameNumber { get; init; }

	/// <summary>Short label for the marker</summary>
	public required string Label { get; init; }

	/// <summary>Optional longer description</summary>
	public string? Description { get; init; }

	/// <summary>Color for UI display (hex format)</summary>
	public string? Color { get; init; }

	/// <summary>Type of marker</summary>
	public MarkerType Type { get; init; } = MarkerType.Bookmark;
}

/// <summary>
/// Branch point for branching TAS workflows
/// </summary>
public sealed record BranchPoint {
	/// <summary>Unique ID for this branch</summary>
	public required string Id { get; init; }

	/// <summary>Frame number where branch starts</summary>
	public int FrameNumber { get; init; }

	/// <summary>Name of the branch</summary>
	public string? Name { get; init; }

	/// <summary>Savestate at this branch point</summary>
	public byte[]? State { get; init; }

	/// <summary>Parent branch ID (null for root)</summary>
	public string? ParentBranchId { get; init; }
}

/// <summary>
/// Embedded script in the movie
/// </summary>
public sealed record EmbeddedScript {
	/// <summary>Script filename</summary>
	public required string Name { get; init; }

	/// <summary>Script language (Lua, Python, etc.)</summary>
	public string Language { get; init; } = "lua";

	/// <summary>Script content</summary>
	public required string Content { get; init; }

	/// <summary>Whether to run on movie load</summary>
	public bool AutoRun { get; init; }
}

/// <summary>
/// Subframe input for cycle-accurate timing
/// </summary>
public sealed record SubframeInput {
	/// <summary>Parent frame number</summary>
	public int FrameNumber { get; init; }

	/// <summary>Cycle offset within frame</summary>
	public long CycleOffset { get; init; }

	/// <summary>Controller port</summary>
	public int Port { get; init; }

	/// <summary>Input state</summary>
	public required ControllerInput Input { get; init; }
}
