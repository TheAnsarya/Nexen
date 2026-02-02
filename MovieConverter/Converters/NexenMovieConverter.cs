using System.IO.Compression;
using System.Text;
using System.Text.Json;

namespace Nexen.MovieConverter.Converters;

/// <summary>
/// Converter for Nexen native movie format (.nexen-movie).
///
/// The .nexen-movie format is a ZIP archive containing:
/// - movie.json: Metadata (author, ROM info, settings)
/// - input.txt: Human-readable input log (one frame per line)
/// - savestate.bin: Optional initial savestate
/// - sram.bin: Optional initial SRAM
/// - rtc.bin: Optional initial RTC data
/// - markers.json: Optional bookmark/marker data
/// - scripts/: Optional embedded Lua scripts
/// </summary>
public sealed class NexenMovieConverter : MovieConverterBase {
	/// <inheritdoc/>
	public override string[] Extensions => [".nexen-movie"];

	/// <inheritdoc/>
	public override string FormatName => "Nexen Movie";

	/// <inheritdoc/>
	public override MovieFormat Format => MovieFormat.Nexen;

	/// <inheritdoc/>
	public override MovieData Read(Stream stream, string? fileName = null) {
		ArgumentNullException.ThrowIfNull(stream);

		using var archive = new ZipArchive(stream, ZipArchiveMode.Read, leaveOpen: true);

		// Read metadata
		ZipArchiveEntry metadataEntry = archive.GetEntry("movie.json")
			?? throw new MovieFormatException(Format, "Missing movie.json in archive");

		NexenMovieHeader metadata;
		using (Stream metadataStream = metadataEntry.Open()) {
			metadata = JsonSerializer.Deserialize(metadataStream, MovieJsonContext.Default.NexenMovieHeader)
				?? throw new MovieFormatException(Format, "Invalid movie.json");
		}

		// Create movie from metadata
		var movie = new MovieData {
			Author = metadata.Author ?? "",
			Description = metadata.Description ?? "",
			GameName = metadata.GameName ?? "",
			RomFileName = metadata.RomFileName ?? "",
			InternalRomName = metadata.InternalRomName,
			GameDatabaseId = metadata.GameDatabaseId,
			Sha1Hash = metadata.Sha1Hash,
			Sha256Hash = metadata.Sha256Hash,
			Md5Hash = metadata.Md5Hash,
			Crc32 = metadata.Crc32,
			RomSize = metadata.RomSize,
			SystemType = metadata.SystemType,
			Region = metadata.Region,
			TimingMode = metadata.TimingMode,
			SyncMode = metadata.SyncMode,
			CoreSettings = metadata.CoreSettings,
			SyncSettings = metadata.SyncSettings,
			RerecordCount = metadata.RerecordCount,
			LagFrameCount = metadata.LagFrameCount,
			BlankFrames = metadata.BlankFrames,
			ControllerCount = metadata.ControllerCount,
			ControllersSwapped = metadata.ControllersSwapped,
			StartType = metadata.StartType,
			BiosHash = metadata.BiosHash,
			SourceFormat = MovieFormat.Nexen,
			SourceFormatVersion = metadata.FormatVersion,
			EmulatorVersion = metadata.EmulatorVersion,
			CreatedDate = metadata.CreatedDate,
			ModifiedDate = metadata.ModifiedDate,
			IsVerified = metadata.IsVerified,
			VerificationNotes = metadata.VerificationNotes,
			FinalStateHash = metadata.FinalStateHash,
			VsDipSwitches = metadata.VsDipSwitches,
			VsPpuType = metadata.VsPpuType,
			ExtraData = metadata.ExtraData
		};

		// Copy port types
		if (metadata.PortTypes != null) {
			for (int i = 0; i < metadata.PortTypes.Length && i < movie.PortTypes.Length; i++) {
				movie.PortTypes[i] = metadata.PortTypes[i];
			}
		}

		// Read input log
		ZipArchiveEntry? inputEntry = archive.GetEntry("input.txt");
		if (inputEntry != null) {
			using Stream inputStream = inputEntry.Open();
			using var reader = new StreamReader(inputStream);

			int frameNumber = 0;
			while (reader.ReadLine() is { } line) {
				// Skip empty lines and comments
				if (string.IsNullOrWhiteSpace(line) || line.TrimStart().StartsWith("//")) {
					continue;
				}

				var frame = InputFrame.FromNexenLogLine(line, frameNumber);
				movie.InputFrames.Add(frame);
				frameNumber++;
			}
		}

		// Read optional savestate
		ZipArchiveEntry? savestateEntry = archive.GetEntry("savestate.bin");
		if (savestateEntry != null) {
			using Stream ssStream = savestateEntry.Open();
			movie.InitialState = ReadAllBytes(ssStream);
			movie.StartType = StartType.Savestate;
		}

		// Read optional SRAM
		ZipArchiveEntry? sramEntry = archive.GetEntry("sram.bin");
		if (sramEntry != null) {
			using Stream sramStream = sramEntry.Open();
			movie.InitialSram = ReadAllBytes(sramStream);
			if (movie.StartType == StartType.PowerOn) {
				movie.StartType = StartType.Sram;
			}
		}

		// Read optional RTC
		ZipArchiveEntry? rtcEntry = archive.GetEntry("rtc.bin");
		if (rtcEntry != null) {
			using Stream rtcStream = rtcEntry.Open();
			movie.InitialRtc = ReadAllBytes(rtcStream);
		}

		// Read optional markers
		ZipArchiveEntry? markersEntry = archive.GetEntry("markers.json");
		if (markersEntry != null) {
			using Stream markersStream = markersEntry.Open();
			movie.Markers = JsonSerializer.Deserialize(markersStream, MovieJsonContext.Default.ListMovieMarker);
		}

		// Read optional embedded scripts
		foreach (ZipArchiveEntry entry in archive.Entries) {
			if (entry.FullName.StartsWith("scripts/", StringComparison.OrdinalIgnoreCase) &&
				entry.FullName.Length > 8) {
				movie.Scripts ??= [];
				using Stream scriptStream = entry.Open();
				using var reader = new StreamReader(scriptStream);
				movie.Scripts.Add(new EmbeddedScript {
					Name = entry.Name,
					Content = reader.ReadToEnd(),
					Language = Path.GetExtension(entry.Name).ToUpperInvariant() switch {
						".LUA" => "lua",
						".PY" => "python",
						".JS" => "javascript",
						_ => "lua"
					}
				});
			}
		}

		return movie;
	}

	/// <inheritdoc/>
	public override void Write(MovieData movie, Stream stream) {
		ArgumentNullException.ThrowIfNull(movie);
		ArgumentNullException.ThrowIfNull(stream);

		using var archive = new ZipArchive(stream, ZipArchiveMode.Create, leaveOpen: true);

		// Write metadata
		var metadata = new NexenMovieHeader {
			FormatVersion = "2.0",
			EmulatorVersion = movie.EmulatorVersion ?? "Nexen",
			CreatedDate = movie.CreatedDate ?? DateTime.UtcNow,
			ModifiedDate = DateTime.UtcNow,

			Author = movie.Author,
			Description = movie.Description,
			GameName = movie.GameName,
			RomFileName = movie.RomFileName,
			InternalRomName = movie.InternalRomName,
			GameDatabaseId = movie.GameDatabaseId,

			Sha1Hash = movie.Sha1Hash,
			Sha256Hash = movie.Sha256Hash,
			Md5Hash = movie.Md5Hash,
			Crc32 = movie.Crc32,
			RomSize = movie.RomSize,

			SystemType = movie.SystemType,
			Region = movie.Region,
			TimingMode = movie.TimingMode,
			SyncMode = movie.SyncMode,
			CoreSettings = movie.CoreSettings,
			SyncSettings = movie.SyncSettings,
			ControllerCount = movie.ControllerCount,
			PortTypes = movie.PortTypes,
			ControllersSwapped = movie.ControllersSwapped,

			RerecordCount = movie.RerecordCount,
			LagFrameCount = movie.LagFrameCount,
			BlankFrames = movie.BlankFrames,
			TotalFrames = movie.TotalFrames,

			StartType = movie.StartType,
			BiosHash = movie.BiosHash,

			IsVerified = movie.IsVerified,
			VerificationNotes = movie.VerificationNotes,
			FinalStateHash = movie.FinalStateHash,

			VsDipSwitches = movie.VsDipSwitches,
			VsPpuType = movie.VsPpuType,

			ExtraData = movie.ExtraData
		};

		ZipArchiveEntry metadataEntry = archive.CreateEntry("movie.json", CompressionLevel.Optimal);
		using (Stream metadataStream = metadataEntry.Open()) {
			JsonSerializer.Serialize(metadataStream, metadata, MovieJsonContext.Default.NexenMovieHeader);
		}

		// Write input log
		ZipArchiveEntry inputEntry = archive.CreateEntry("input.txt", CompressionLevel.Optimal);
		using (Stream inputStream = inputEntry.Open())
		using (var writer = new StreamWriter(inputStream, Encoding.UTF8)) {
			// Header comment
			writer.WriteLine("// Nexen Movie Input Log v2.0");
			writer.WriteLine($"// Game: {movie.GameName}");
			writer.WriteLine($"// Author: {movie.Author}");
			writer.WriteLine($"// System: {movie.SystemType} ({movie.Region})");
			writer.WriteLine($"// Frames: {movie.TotalFrames}");
			writer.WriteLine($"// Duration: {movie.Duration:hh\\:mm\\:ss\\.fff}");
			writer.WriteLine($"// Rerecords: {movie.RerecordCount}");
			writer.WriteLine("// Format: [CMD:...]|P1|P2|...|[LAG]|[# comment]");
			writer.WriteLine("// Buttons: BYsSUDLRAXLR (SNES) or RLDUSTBA (NES)");
			writer.WriteLine();

			foreach (InputFrame frame in movie.InputFrames) {
				writer.WriteLine(frame.ToNexenLogLine(movie.ControllerCount));
			}
		}

		// Write optional savestate
		if (movie.InitialState is { Length: > 0 }) {
			ZipArchiveEntry ssEntry = archive.CreateEntry("savestate.bin", CompressionLevel.Optimal);
			using Stream ssStream = ssEntry.Open();
			ssStream.Write(movie.InitialState);
		}

		// Write optional SRAM
		if (movie.InitialSram is { Length: > 0 }) {
			ZipArchiveEntry sramEntry = archive.CreateEntry("sram.bin", CompressionLevel.Optimal);
			using Stream sramStream = sramEntry.Open();
			sramStream.Write(movie.InitialSram);
		}

		// Write optional RTC
		if (movie.InitialRtc is { Length: > 0 }) {
			ZipArchiveEntry rtcEntry = archive.CreateEntry("rtc.bin", CompressionLevel.Optimal);
			using Stream rtcStream = rtcEntry.Open();
			rtcStream.Write(movie.InitialRtc);
		}

		// Write optional markers
		if (movie.Markers is { Count: > 0 }) {
			ZipArchiveEntry markersEntry = archive.CreateEntry("markers.json", CompressionLevel.Optimal);
			using Stream markersStream = markersEntry.Open();
			JsonSerializer.Serialize(markersStream, movie.Markers, MovieJsonContext.Default.ListMovieMarker);
		}

		// Write optional embedded scripts
		if (movie.Scripts is { Count: > 0 }) {
			foreach (EmbeddedScript script in movie.Scripts) {
				string ext = script.Language.ToUpperInvariant() switch {
					"PYTHON" => ".py",
					"JAVASCRIPT" => ".js",
					_ => ".lua"
				};
				string scriptName = script.Name.EndsWith(ext, StringComparison.OrdinalIgnoreCase)
					? script.Name
					: script.Name + ext;

				ZipArchiveEntry scriptEntry = archive.CreateEntry($"scripts/{scriptName}", CompressionLevel.Optimal);
				using Stream scriptStream = scriptEntry.Open();
				using var writer = new StreamWriter(scriptStream, Encoding.UTF8);
				writer.Write(script.Content);
			}
		}
	}
}
