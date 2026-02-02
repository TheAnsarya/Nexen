using System.IO.Compression;
using System.Text;

namespace Nexen.MovieConverter.Converters;

/// <summary>
/// Converter for lsnes LSMV movie format.
/// LSMV is a ZIP-based format used by lsnes for SNES/GB TASing.
/// </summary>
public sealed class LsmvMovieConverter : MovieConverterBase {
	/// <inheritdoc />
	public override MovieFormat Format => MovieFormat.Lsmv;

	/// <inheritdoc />
	public override string[] Extensions => [".lsmv"];

	/// <inheritdoc />
	public override string FormatName => "lsnes Movie Format";

	/// <inheritdoc />
	public override MovieData Read(Stream stream, string? fileName = null) {
		ArgumentNullException.ThrowIfNull(stream);

		var movie = new MovieData {
			SourceFormat = MovieFormat.Lsmv
		};

		using var archive = new ZipArchive(stream, ZipArchiveMode.Read, leaveOpen: true);

		// Read manifest (systemid, coreversion, etc.)
		var manifestEntry = archive.GetEntry("manifest");
		if (manifestEntry is not null) {
			using var reader = new StreamReader(manifestEntry.Open(), Encoding.UTF8);
			ParseManifest(reader, movie);
		}

		// Read gamename
		var gamenameEntry = archive.GetEntry("gamename");
		if (gamenameEntry is not null) {
			using var reader = new StreamReader(gamenameEntry.Open(), Encoding.UTF8);
			movie.GameName = reader.ReadToEnd().Trim();
		}

		// Read authors
		var authorsEntry = archive.GetEntry("authors");
		if (authorsEntry is not null) {
			using var reader = new StreamReader(authorsEntry.Open(), Encoding.UTF8);
			movie.Author = reader.ReadToEnd().Trim();
		}

		// Read rerecord count from rerecords file
		var rerecordsEntry = archive.GetEntry("rerecords");
		if (rerecordsEntry is not null) {
			using var reader = new StreamReader(rerecordsEntry.Open(), Encoding.UTF8);
			if (ulong.TryParse(reader.ReadToEnd().Trim(), out var rerecords)) {
				movie.RerecordCount = rerecords;
			}
		}

		// Read input log
		var inputEntry = archive.GetEntry("input");
		if (inputEntry is not null) {
			using var reader = new StreamReader(inputEntry.Open(), Encoding.UTF8);
			ParseInputLog(reader, movie);
		}

		// Read savestate if present (for savestate-start movies)
		var savestateEntry = archive.GetEntry("savestate");
		if (savestateEntry is not null) {
			movie.StartType = StartType.Savestate;
			using var ms = new MemoryStream();
			savestateEntry.Open().CopyTo(ms);
			movie.InitialState = ms.ToArray();
		}

		// Read SRAM if present
		var sramEntry = archive.GetEntry("moviesram");
		if (sramEntry is not null) {
			using var ms = new MemoryStream();
			sramEntry.Open().CopyTo(ms);
			movie.InitialSram = ms.ToArray();
		}

		// Read ROM hashes
		foreach (var entry in archive.Entries) {
			if (entry.Name.StartsWith("slot", StringComparison.Ordinal) && entry.Name.EndsWith(".sha256", StringComparison.Ordinal)) {
				using var reader = new StreamReader(entry.Open(), Encoding.UTF8);
				var hash = reader.ReadToEnd().Trim();
				// Store as ROM SHA256 in verification
				if (string.IsNullOrEmpty(movie.Sha256Hash)) {
					movie.Sha256Hash = hash;
				}
			}
		}

		// Read subtitles
		var subtitlesEntry = archive.GetEntry("subtitles");
		if (subtitlesEntry is not null) {
			using var reader = new StreamReader(subtitlesEntry.Open(), Encoding.UTF8);
			ParseSubtitles(reader, movie);
		}

		return movie;
	}

	/// <inheritdoc />
	public override void Write(MovieData movie, Stream stream) {
		ArgumentNullException.ThrowIfNull(stream);
		ArgumentNullException.ThrowIfNull(movie);

		using var archive = new ZipArchive(stream, ZipArchiveMode.Create, leaveOpen: true);

		// Write manifest
		WriteManifest(archive, movie);

		// Write gamename
		if (!string.IsNullOrEmpty(movie.GameName)) {
			var entry = archive.CreateEntry("gamename");
			using var writer = new StreamWriter(entry.Open(), Encoding.UTF8);
			writer.Write(movie.GameName);
		}

		// Write authors
		if (!string.IsNullOrEmpty(movie.Author)) {
			var entry = archive.CreateEntry("authors");
			using var writer = new StreamWriter(entry.Open(), Encoding.UTF8);
			writer.Write(movie.Author);
		}

		// Write rerecords
		var rerecordsEntry = archive.CreateEntry("rerecords");
		using (var writer = new StreamWriter(rerecordsEntry.Open(), Encoding.UTF8)) {
			writer.Write(movie.RerecordCount.ToString());
		}

		// Write input log
		WriteInputLog(archive, movie);

		// Write savestate if present
		if (movie.InitialState is { Length: > 0 }) {
			var entry = archive.CreateEntry("savestate");
			using var entryStream = entry.Open();
			entryStream.Write(movie.InitialState);
		}

		// Write SRAM if present
		if (movie.InitialSram is { Length: > 0 }) {
			var entry = archive.CreateEntry("moviesram");
			using var entryStream = entry.Open();
			entryStream.Write(movie.InitialSram);
		}

		// Write ROM hash
		if (!string.IsNullOrEmpty(movie.Sha256Hash)) {
			var entry = archive.CreateEntry("slot1.sha256");
			using var writer = new StreamWriter(entry.Open(), Encoding.UTF8);
			writer.Write(movie.Sha256Hash);
		}

		// Write subtitles from comments
		WriteSubtitles(archive, movie);
	}

	private static void ParseManifest(StreamReader reader, MovieData movie) {
		string? line;
		while ((line = reader.ReadLine()) is not null) {
			if (string.IsNullOrWhiteSpace(line)) continue;

			var parts = line.Split(' ', 2, StringSplitOptions.TrimEntries);
			if (parts.Length < 1) continue;

			var key = parts[0].ToLowerInvariant();
			var value = parts.Length > 1 ? parts[1] : string.Empty;

			switch (key) {
				case "systemid":
					movie.SystemType = ParseSystemId(value);
					break;

				case "coreversion":
					movie.EmulatorVersion = value;
					break;

				case "gametype":
					// Parse game type for region info
					if (value.Contains("pal", StringComparison.OrdinalIgnoreCase)) {
						movie.Region = RegionType.PAL;
					} else {
						movie.Region = RegionType.NTSC;
					}
					break;

				case "port1":
				case "port2":
					// Controller configuration
					var portIndex = key[4] - '1'; // "port1" -> 0, "port2" -> 1
					if (portIndex >= 0 && portIndex < movie.PortTypes.Length) {
						movie.PortTypes[portIndex] = ParseControllerType(value);
					}
					break;

				case "projectid":
					// Store project ID in extra data
					movie.ExtraData ??= [];
					movie.ExtraData["lsmv_projectid"] = value;
					break;
			}
		}
	}

	private static SystemType ParseSystemId(string systemId) {
		return systemId.ToLowerInvariant() switch {
			"snes" or "bsnes" => SystemType.Snes,
			"gb" or "gameboy" => SystemType.Gb,
			"gbc" => SystemType.Gbc,
			"sgb" => SystemType.Gb, // Super Game Boy counts as GB
			_ => SystemType.Snes // Default to SNES for lsnes
		};
	}

	private static ControllerType ParseControllerType(string type) {
		return type.ToLowerInvariant() switch {
			"gamepad" or "gamepad16" => ControllerType.Gamepad,
			"mouse" => ControllerType.Mouse,
			"superscope" => ControllerType.SuperScope,
			"justifier" or "justifiers" => ControllerType.Justifier,
			"multitap" => ControllerType.SnesMultitap,
			"none" => ControllerType.None,
			_ => ControllerType.Gamepad
		};
	}

	private static void ParseInputLog(StreamReader reader, MovieData movie) {
		int frameNumber = 0;
		string? line;

		while ((line = reader.ReadLine()) is not null) {
			if (string.IsNullOrWhiteSpace(line)) continue;

			// LSMV format: F<reset>|<port1>|<port2>|...
			// Port format for SNES gamepad: BYsSudlrAXLR (12 buttons)
			// Dots indicate unpressed, letters indicate pressed

			var frame = new InputFrame(frameNumber++);

			// Skip non-input lines (comments, etc.)
			if (!line.StartsWith('F') && !line.StartsWith('|')) continue;

			// Parse reset flag
			if (line.StartsWith('F')) {
				var afterF = line.Length > 1 ? line[1] : '.';
				if (afterF == 'R') {
					frame.Command = FrameCommand.SoftReset;
				} else if (afterF == 'P') {
					frame.Command = FrameCommand.HardReset;
				}
			}

			// Find input sections (after the F flag and reset indicator)
			var pipeIndex = line.IndexOf('|', StringComparison.Ordinal);
			if (pipeIndex < 0) {
				movie.AddFrame(frame);
				continue;
			}

			var parts = line[(pipeIndex + 1)..].Split('|');

			for (int port = 0; port < Math.Min(parts.Length, InputFrame.MaxPorts); port++) {
				var portData = parts[port].Trim();
				if (string.IsNullOrEmpty(portData)) continue;

				ParseLsmvPortInput(portData, frame.Controllers[port], movie.SystemType);
			}

			movie.AddFrame(frame);
		}
	}

	private static void ParseLsmvPortInput(string portData, ControllerInput input, SystemType system) {
		// SNES gamepad format: BYsSudlrAXLR (12 chars)
		// GB format: ABsSudlr (8 chars)
		// Dots mean not pressed, letters mean pressed

		if (system is SystemType.Gb or SystemType.Gbc) {
			// GB: A B Select Start Up Down Left Right
			if (portData.Length >= 8) {
				input.A = portData[0] != '.';
				input.B = portData[1] != '.';
				input.Select = portData[2] != '.';
				input.Start = portData[3] != '.';
				input.Up = portData[4] != '.';
				input.Down = portData[5] != '.';
				input.Left = portData[6] != '.';
				input.Right = portData[7] != '.';
			}
		} else {
			// SNES: B Y Select Start Up Down Left Right A X L R
			if (portData.Length >= 12) {
				input.B = portData[0] != '.';
				input.Y = portData[1] != '.';
				input.Select = portData[2] != '.';
				input.Start = portData[3] != '.';
				input.Up = portData[4] != '.';
				input.Down = portData[5] != '.';
				input.Left = portData[6] != '.';
				input.Right = portData[7] != '.';
				input.A = portData[8] != '.';
				input.X = portData[9] != '.';
				input.L = portData[10] != '.';
				input.R = portData[11] != '.';
			}
		}
	}

	private static void ParseSubtitles(StreamReader reader, MovieData movie) {
		string? line;
		while ((line = reader.ReadLine()) is not null) {
			if (string.IsNullOrWhiteSpace(line)) continue;

			// LSMV subtitle format: frame length text
			var parts = line.Split(' ', 3);
			if (parts.Length >= 3 && int.TryParse(parts[0], out var frame)) {
				movie.Markers ??= [];
				var marker = new MovieMarker {
					FrameNumber = frame,
					Label = parts[2],
					Type = MarkerType.Subtitle
				};
				movie.Markers.Add(marker);
			}
		}
	}

	private static void WriteManifest(ZipArchive archive, MovieData movie) {
		var entry = archive.CreateEntry("manifest");
		using var writer = new StreamWriter(entry.Open(), Encoding.UTF8);

		// System ID
		var systemId = movie.SystemType switch {
			SystemType.Snes => "snes",
			SystemType.Gb => "gb",
			SystemType.Gbc => "gbc",
			_ => "snes"
		};
		writer.WriteLine($"systemid {systemId}");

		// Emulator version
		if (!string.IsNullOrEmpty(movie.EmulatorVersion)) {
			writer.WriteLine($"coreversion {movie.EmulatorVersion}");
		}

		// Controllers
		for (int i = 0; i < movie.PortTypes.Length && i < 2; i++) {
			var controllerName = movie.PortTypes[i] switch {
				ControllerType.None => "none",
				ControllerType.Mouse => "mouse",
				ControllerType.SuperScope => "superscope",
				ControllerType.Justifier => "justifier",
				ControllerType.SnesMultitap => "multitap",
				_ => "gamepad16"
			};
			writer.WriteLine($"port{i + 1} {controllerName}");
		}

		// Project ID from extra data
		if (movie.ExtraData?.TryGetValue("lsmv_projectid", out var projectId) == true) {
			writer.WriteLine($"projectid {projectId}");
		}
	}

	private static void WriteInputLog(ZipArchive archive, MovieData movie) {
		var entry = archive.CreateEntry("input");
		using var writer = new StreamWriter(entry.Open(), Encoding.UTF8);

		foreach (var frame in movie.InputFrames) {
			var sb = new StringBuilder();

			// Frame marker with reset
			sb.Append('F');
			if (frame.Command.HasFlag(FrameCommand.SoftReset)) {
				sb.Append('R');
			} else if (frame.Command.HasFlag(FrameCommand.HardReset)) {
				sb.Append('P');
			} else {
				sb.Append('.');
			}

			// Ports (always 2 for LSMV)
			for (int port = 0; port < 2 && port < frame.Controllers.Length; port++) {
				sb.Append('|');
				sb.Append(FormatLsmvPortInput(frame.Controllers[port], movie.SystemType));
			}

			sb.Append('|');
			writer.WriteLine(sb.ToString());
		}
	}

	private static string FormatLsmvPortInput(ControllerInput input, SystemType system) {
		if (system is SystemType.Gb or SystemType.Gbc) {
			// GB: A B Select Start Up Down Left Right
			return string.Create(8, input, static (span, i) => {
				span[0] = i.A ? 'A' : '.';
				span[1] = i.B ? 'B' : '.';
				span[2] = i.Select ? 's' : '.';
				span[3] = i.Start ? 'S' : '.';
				span[4] = i.Up ? 'u' : '.';
				span[5] = i.Down ? 'd' : '.';
				span[6] = i.Left ? 'l' : '.';
				span[7] = i.Right ? 'r' : '.';
			});
		}

		// SNES: B Y Select Start Up Down Left Right A X L R
		return string.Create(12, input, static (span, i) => {
			span[0] = i.B ? 'B' : '.';
			span[1] = i.Y ? 'Y' : '.';
			span[2] = i.Select ? 's' : '.';
			span[3] = i.Start ? 'S' : '.';
			span[4] = i.Up ? 'u' : '.';
			span[5] = i.Down ? 'd' : '.';
			span[6] = i.Left ? 'l' : '.';
			span[7] = i.Right ? 'r' : '.';
			span[8] = i.A ? 'A' : '.';
			span[9] = i.X ? 'X' : '.';
			span[10] = i.L ? 'L' : '.';
			span[11] = i.R ? 'R' : '.';
		});
	}

	private static void WriteSubtitles(ZipArchive archive, MovieData movie) {
		if (movie.Markers is null) return;

		var subtitles = movie.Markers.Where(m => m.Type == MarkerType.Subtitle).ToList();
		if (subtitles.Count == 0) return;

		var entry = archive.CreateEntry("subtitles");
		using var writer = new StreamWriter(entry.Open(), Encoding.UTF8);

		foreach (var marker in subtitles) {
			// Frame duration text
			writer.WriteLine($"{marker.FrameNumber} 120 {marker.Label}");
		}
	}
}
