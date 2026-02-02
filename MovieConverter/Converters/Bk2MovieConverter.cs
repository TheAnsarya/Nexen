using System.Globalization;
using System.IO.Compression;
using System.Runtime.CompilerServices;
using System.Text;
using System.Text.Json;

namespace Nexen.MovieConverter.Converters;

/// <summary>
/// Converter for BizHawk BK2 movie format.
///
/// BK2 is a ZIP archive containing:
/// - Header.txt: Key-value pairs for metadata
/// - Input Log.txt: Input frames in BizHawk text format
/// - SyncSettings.json: JSON sync settings (optional)
/// - Comments.txt: Author comments (optional)
/// - Subtitles.txt: Subtitle annotations (optional)
/// - GreenZone: TAStudio branch data (optional)
/// - Markers.txt: Frame markers (optional)
/// - SaveRam: Initial SRAM data (optional)
///
/// Reference: https://tasvideos.org/Bizhawk/BK2Format
/// </summary>
public sealed class Bk2MovieConverter : MovieConverterBase {
	/// <inheritdoc/>
	public override string[] Extensions => [".bk2"];

	/// <inheritdoc/>
	public override string FormatName => "BizHawk Movie v2";

	/// <inheritdoc/>
	public override MovieFormat Format => MovieFormat.Bk2;

	// BK2 uses platform-specific input formats
	private static readonly Dictionary<string, SystemType> PlatformMap = new(StringComparer.OrdinalIgnoreCase) {
		["NES"] = SystemType.Nes,
		["SNES"] = SystemType.Snes,
		["GB"] = SystemType.Gb,
		["GBC"] = SystemType.Gbc,
		["GBA"] = SystemType.Gba,
		["SMS"] = SystemType.Sms,
		["GG"] = SystemType.GameGear,
		["GEN"] = SystemType.Genesis,
		["PCE"] = SystemType.PcEngine,
		["A26"] = SystemType.A2600,
		["A78"] = SystemType.A7800,
		["Coleco"] = SystemType.Coleco,
		["N64"] = SystemType.N64,
		["PSX"] = SystemType.Psx,
		["SAT"] = SystemType.Saturn,
		["WSWAN"] = SystemType.WonderSwan,
		["NGP"] = SystemType.NeoGeo,
		["Lynx"] = SystemType.Lynx,
		["VB"] = SystemType.VirtualBoy
	};

	private static readonly Dictionary<SystemType, string> ReversePlatformMap =
		PlatformMap.ToDictionary(kvp => kvp.Value, kvp => kvp.Key);

	/// <inheritdoc/>
	public override MovieData Read(Stream stream, string? fileName = null) {
		ArgumentNullException.ThrowIfNull(stream);

		using var archive = new ZipArchive(stream, ZipArchiveMode.Read, leaveOpen: true);

		// Parse Header.txt
		ZipArchiveEntry headerEntry = archive.GetEntry("Header.txt")
			?? throw new MovieFormatException(Format, "Missing Header.txt in BK2 archive");

		var header = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
		using (Stream headerStream = headerEntry.Open())
		using (var reader = new StreamReader(headerStream, Encoding.UTF8)) {
			while (reader.ReadLine() is { } line) {
				if (string.IsNullOrWhiteSpace(line)) {
					continue;
				}

				int eqIdx = line.IndexOf(' ');
				if (eqIdx > 0) {
					string key = line[..eqIdx];
					string value = line[(eqIdx + 1)..];
					header[key] = value;
				}
			}
		}

		// Create movie
		var movie = new MovieData {
			SourceFormat = MovieFormat.Bk2,
			SourceFormatVersion = header.GetValueOrDefault("MovieVersion", "BizHawk v2.0"),
			EmulatorVersion = header.GetValueOrDefault("emuVersion"),
			Author = header.GetValueOrDefault("Author", ""),
			GameName = header.GetValueOrDefault("GameName", ""),
			RomFileName = header.GetValueOrDefault("Name", ""),
			Sha1Hash = header.GetValueOrDefault("SHA1"),
			Md5Hash = header.GetValueOrDefault("MD5"),
			CoreSettings = header.GetValueOrDefault("Core"),
		};

		// Parse system type
		if (header.TryGetValue("Platform", out string? platform) && PlatformMap.TryGetValue(platform, out SystemType sysType)) {
			movie.SystemType = sysType;
		}

		// Parse region
		if (header.TryGetValue("PAL", out string? palStr) && bool.TryParse(palStr, out bool isPal) && isPal) {
			movie.Region = RegionType.PAL;
		}

		// Parse rerecord count
		if (header.TryGetValue("rerecordCount", out string? rrStr) && ulong.TryParse(rrStr, out ulong rr)) {
			movie.RerecordCount = rr;
		}

		// Parse start type
		if (header.TryGetValue("StartsFromSavestate", out string? ssStr) && bool.TryParse(ssStr, out bool ss) && ss) {
			movie.StartType = StartType.Savestate;
		} else if (header.TryGetValue("StartsFromSaveram", out string? sramStr) && bool.TryParse(sramStr, out bool sram) && sram) {
			movie.StartType = StartType.Sram;
		}

		// Parse board info (for FDS, etc.)
		if (header.TryGetValue("BoardName", out string? board)) {
			movie.ExtraData ??= [];
			movie.ExtraData["BoardName"] = board;
		}

		// Parse firmware (BIOS)
		if (header.TryGetValue("FirmwareHash", out string? fwHash)) {
			movie.BiosHash = fwHash;
		}

		// Read Comments.txt
		ZipArchiveEntry? commentsEntry = archive.GetEntry("Comments.txt");
		if (commentsEntry != null) {
			using Stream commentsStream = commentsEntry.Open();
			using var reader = new StreamReader(commentsStream, Encoding.UTF8);
			movie.Description = reader.ReadToEnd();
		}

		// Read Subtitles.txt
		ZipArchiveEntry? subtitlesEntry = archive.GetEntry("Subtitles.txt");
		if (subtitlesEntry != null) {
			using Stream subStream = subtitlesEntry.Open();
			using var reader = new StreamReader(subStream, Encoding.UTF8);
			string subtitles = reader.ReadToEnd();
			if (!string.IsNullOrWhiteSpace(subtitles)) {
				movie.ExtraData ??= [];
				movie.ExtraData["Subtitles"] = subtitles;
			}
		}

		// Read SyncSettings.json
		ZipArchiveEntry? syncEntry = archive.GetEntry("SyncSettings.json");
		if (syncEntry != null) {
			using Stream syncStream = syncEntry.Open();
			using var reader = new StreamReader(syncStream, Encoding.UTF8);
			movie.SyncSettings = reader.ReadToEnd();
		}

		// Read Markers.txt
		ZipArchiveEntry? markersEntry = archive.GetEntry("Markers.txt");
		if (markersEntry != null) {
			movie.Markers = [];
			using Stream markersStream = markersEntry.Open();
			using var reader = new StreamReader(markersStream, Encoding.UTF8);
			while (reader.ReadLine() is { } line) {
				// Format: Frame\tLabel
				string[] parts = line.Split('\t');
				if (parts.Length >= 2 && int.TryParse(parts[0], out int frameNum)) {
					movie.Markers.Add(new MovieMarker {
						FrameNumber = frameNum,
						Label = parts[1],
						Description = parts.Length > 2 ? parts[2] : null
					});
				}
			}
		}

		// Read Input Log.txt
		ZipArchiveEntry inputEntry = archive.GetEntry("Input Log.txt")
			?? throw new MovieFormatException(Format, "Missing Input Log.txt in BK2 archive");

		using (Stream inputStream = inputEntry.Open())
		using (var reader = new StreamReader(inputStream, Encoding.UTF8)) {
			string? logHeader = null;
			int frameNumber = 0;
			int lagFrames = 0;

			while (reader.ReadLine() is { } line) {
				// First non-empty line is the log header
				if (logHeader == null) {
					if (line.StartsWith('[')) {
						logHeader = line;
						ParseBk2LogHeader(logHeader, movie);
					}

					continue;
				}

				// Skip empty lines
				if (string.IsNullOrWhiteSpace(line)) {
					continue;
				}

				// Parse input line
				InputFrame frame = ParseBk2InputLine(line, frameNumber, movie.SystemType, out bool isLag);
				movie.InputFrames.Add(frame);
				frameNumber++;

				if (isLag) {
					lagFrames++;
				}
			}

			movie.LagFrameCount = (ulong)lagFrames;
		}

		// Read SaveRam (initial SRAM)
		ZipArchiveEntry? saveramEntry = archive.GetEntry("SaveRam");
		if (saveramEntry != null) {
			using Stream sramStream = saveramEntry.Open();
			movie.InitialSram = ReadAllBytes(sramStream);
		}

		// Read Savestate
		ZipArchiveEntry? savestateEntry = archive.GetEntry("Core") // BizHawk uses various names
			?? archive.GetEntry("QuickNes")
			?? archive.GetEntry("BSNESv115");

		if (savestateEntry != null && movie.StartType == StartType.Savestate) {
			using Stream ssStream = savestateEntry.Open();
			movie.InitialState = ReadAllBytes(ssStream);
		}

		return movie;
	}

	/// <summary>
	/// Parse the BK2 input log header to determine controller configuration
	/// </summary>
	private static void ParseBk2LogHeader(string header, MovieData movie) {
		// BK2 header format: [Input Log v2] or just controller layout like |P1|P2|...
		// Determine controller count from header segments
		string[] parts = header.Split('|');
		int controllers = 0;

		foreach (string part in parts) {
			if (part.StartsWith('P') ||
				part.StartsWith("Power", StringComparison.Ordinal) ||
				part.StartsWith("Reset", StringComparison.Ordinal)) {
				continue;
			}

			// Count non-command segments as controller ports
			if (!string.IsNullOrWhiteSpace(part) && !part.Contains('[')) {
				controllers++;
			}
		}

		movie.ControllerCount = Math.Max(controllers, 1);
	}

	/// <summary>
	/// Parse a single BK2 input line
	/// </summary>
	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	private static InputFrame ParseBk2InputLine(string line, int frameNumber, SystemType system, out bool isLag) {
		var frame = new InputFrame {
			FrameNumber = frameNumber
		};
		isLag = false;

		// BK2 format: |command|P1 buttons|P2 buttons|...|
		// or for lag frames: |lag|
		if (line.Equals("|lag|", StringComparison.OrdinalIgnoreCase) ||
			line.Contains("|L|", StringComparison.OrdinalIgnoreCase)) {
			isLag = true;
			frame.Command = FrameCommand.LagFrame;
			return frame;
		}

		string[] parts = line.Split('|', StringSplitOptions.None);
		int portIndex = 0;

		for (int i = 1; i < parts.Length - 1; i++) // Skip first and last empty segments
		{
			string segment = parts[i];

			// Check for command segments
			if (segment.Equals("r", StringComparison.OrdinalIgnoreCase) ||
				segment.Contains("Reset", StringComparison.OrdinalIgnoreCase)) {
				frame.Command |= FrameCommand.Reset;
				continue;
			}

			if (segment.Equals("P", StringComparison.OrdinalIgnoreCase) ||
				segment.Contains("Power", StringComparison.OrdinalIgnoreCase)) {
				frame.Command |= FrameCommand.Power;
				continue;
			}

			// Parse controller input
			if (portIndex < InputFrame.MaxPorts) {
				frame.Controllers[portIndex] = ParseBk2ControllerInput(segment, system);
				portIndex++;
			}
		}

		return frame;
	}

	/// <summary>
	/// Parse BK2 controller button string
	/// </summary>
	private static ControllerInput ParseBk2ControllerInput(string segment, SystemType system) {
		var input = new ControllerInput();

		// BK2 uses different button orders for different systems
		// NES: "RLDUTSBA" (Right, Left, Down, Up, sTart, Select, B, A)
		// SNES: "BYsSUDLRAXlr" (B, Y, select, Start, Up, Down, Left, Right, A, X, L, R)
		// Genesis: "UDLRSABC" or "UDLRSABCxyz" (3/6 button)

		switch (system) {
			case SystemType.Nes:
				ParseNesInput(segment, input);
				break;

			case SystemType.Snes:
				ParseSnesInput(segment, input);
				break;

			case SystemType.Genesis:
				ParseGenesisInput(segment, input);
				break;

			case SystemType.Gb:
			case SystemType.Gbc:
				ParseGbInput(segment, input);
				break;

			case SystemType.Gba:
				ParseGbaInput(segment, input);
				break;

			default:
				// Generic button parsing
				ParseGenericInput(segment, input);
				break;
		}

		return input;
	}

	private static void ParseNesInput(string s, ControllerInput input) {
		// NES: RLDUTSBA (8 chars)
		if (s.Length < 8) {
			return;
		}

		input.Right = s[0] != '.';
		input.Left = s[1] != '.';
		input.Down = s[2] != '.';
		input.Up = s[3] != '.';
		input.Start = s[4] != '.';
		input.Select = s[5] != '.';
		input.B = s[6] != '.';
		input.A = s[7] != '.';
	}

	private static void ParseSnesInput(string s, ControllerInput input) {
		// SNES: BYsSUDLRAXlr (12 chars)
		if (s.Length < 12) {
			return;
		}

		input.B = s[0] != '.';
		input.Y = s[1] != '.';
		input.Select = s[2] != '.';
		input.Start = s[3] != '.';
		input.Up = s[4] != '.';
		input.Down = s[5] != '.';
		input.Left = s[6] != '.';
		input.Right = s[7] != '.';
		input.A = s[8] != '.';
		input.X = s[9] != '.';
		input.L = s[10] != '.';
		input.R = s[11] != '.';
	}

	private static void ParseGenesisInput(string s, ControllerInput input) {
		// Genesis 3-button: UDLRSABC (8 chars)
		// Genesis 6-button: UDLRSABCxyzm (12 chars)
		if (s.Length < 8) {
			return;
		}

		input.Up = s[0] != '.';
		input.Down = s[1] != '.';
		input.Left = s[2] != '.';
		input.Right = s[3] != '.';
		input.Start = s[4] != '.';
		input.A = s[5] != '.';
		input.B = s[6] != '.';
		input.C = s[7] != '.';

		if (s.Length >= 12) {
			input.X = s[8] != '.';
			input.Y = s[9] != '.';
			input.Z = s[10] != '.';
			input.Mode = s[11] != '.';
		}
	}

	private static void ParseGbInput(string s, ControllerInput input) =>
		// GB: RLDUTSBA (8 chars, same as NES)
		ParseNesInput(s, input);

	private static void ParseGbaInput(string s, ControllerInput input) {
		// GBA: RLDUSTBALR (10 chars)
		if (s.Length < 10) {
			return;
		}

		input.Right = s[0] != '.';
		input.Left = s[1] != '.';
		input.Down = s[2] != '.';
		input.Up = s[3] != '.';
		input.Start = s[4] != '.';
		input.Select = s[5] != '.';
		input.B = s[6] != '.';
		input.A = s[7] != '.';
		input.L = s[8] != '.';
		input.R = s[9] != '.';
	}

	private static void ParseGenericInput(string s, ControllerInput input) {
		// Try to detect any pressed buttons by non-dot characters
		foreach (char c in s) {
			switch (char.ToUpperInvariant(c)) {
				case 'U': input.Up = true; break;
				case 'D': input.Down = true; break;
				case 'L': input.Left = true; break;
				case 'R': input.Right = true; break;
				case 'A': input.A = true; break;
				case 'B': input.B = true; break;
				case 'X': input.X = true; break;
				case 'Y': input.Y = true; break;
				case 'S': input.Start = true; break;
				case 'T': input.Select = true; break; // Some formats use T for select
			}
		}
	}

	/// <inheritdoc/>
	public override void Write(MovieData movie, Stream stream) {
		ArgumentNullException.ThrowIfNull(movie);
		ArgumentNullException.ThrowIfNull(stream);

		using var archive = new ZipArchive(stream, ZipArchiveMode.Create, leaveOpen: true);

		// Write Header.txt
		ZipArchiveEntry headerEntry = archive.CreateEntry("Header.txt", CompressionLevel.Optimal);
		using (Stream headerStream = headerEntry.Open())
		using (var writer = new StreamWriter(headerStream, Encoding.UTF8)) {
			writer.WriteLine($"MovieVersion BizHawk v2.9.1");

			if (!string.IsNullOrEmpty(movie.Author)) {
				writer.WriteLine($"Author {movie.Author}");
			}

			if (!string.IsNullOrEmpty(movie.EmulatorVersion)) {
				writer.WriteLine($"emuVersion {movie.EmulatorVersion}");
			}

			// Platform
			if (ReversePlatformMap.TryGetValue(movie.SystemType, out string? platform)) {
				writer.WriteLine($"Platform {platform}");
			}

			// Game info
			if (!string.IsNullOrEmpty(movie.GameName)) {
				writer.WriteLine($"GameName {movie.GameName}");
			}

			if (!string.IsNullOrEmpty(movie.RomFileName)) {
				writer.WriteLine($"Name {movie.RomFileName}");
			}

			// Hashes
			if (!string.IsNullOrEmpty(movie.Sha1Hash)) {
				writer.WriteLine($"SHA1 {movie.Sha1Hash.ToUpperInvariant()}");
			}

			if (!string.IsNullOrEmpty(movie.Md5Hash)) {
				writer.WriteLine($"MD5 {movie.Md5Hash.ToUpperInvariant()}");
			}

			// Core
			if (!string.IsNullOrEmpty(movie.CoreSettings)) {
				writer.WriteLine($"Core {movie.CoreSettings}");
			}

			// Region
			writer.WriteLine($"PAL {(movie.Region == RegionType.PAL ? "True" : "False")}");

			// Rerecords
			writer.WriteLine($"rerecordCount {movie.RerecordCount}");

			// Start type
			if (movie.StartType == StartType.Savestate) {
				writer.WriteLine("StartsFromSavestate True");
			} else if (movie.StartType == StartType.Sram) {
				writer.WriteLine("StartsFromSaveram True");
			}

			// BIOS
			if (!string.IsNullOrEmpty(movie.BiosHash)) {
				writer.WriteLine($"FirmwareHash {movie.BiosHash}");
			}

			// Board name
			if (movie.ExtraData?.TryGetValue("BoardName", out string? boardName) == true) {
				writer.WriteLine($"BoardName {boardName}");
			}
		}

		// Write Comments.txt
		if (!string.IsNullOrEmpty(movie.Description)) {
			ZipArchiveEntry commentsEntry = archive.CreateEntry("Comments.txt", CompressionLevel.Optimal);
			using Stream commentsStream = commentsEntry.Open();
			using var writer = new StreamWriter(commentsStream, Encoding.UTF8);
			writer.Write(movie.Description);
		}

		// Write SyncSettings.json
		if (!string.IsNullOrEmpty(movie.SyncSettings)) {
			ZipArchiveEntry syncEntry = archive.CreateEntry("SyncSettings.json", CompressionLevel.Optimal);
			using Stream syncStream = syncEntry.Open();
			using var writer = new StreamWriter(syncStream, Encoding.UTF8);
			writer.Write(movie.SyncSettings);
		}

		// Write Markers.txt
		if (movie.Markers is { Count: > 0 }) {
			ZipArchiveEntry markersEntry = archive.CreateEntry("Markers.txt", CompressionLevel.Optimal);
			using Stream markersStream = markersEntry.Open();
			using var writer = new StreamWriter(markersStream, Encoding.UTF8);

			foreach (MovieMarker marker in movie.Markers) {
				writer.Write(marker.FrameNumber.ToString(CultureInfo.InvariantCulture));
				writer.Write('\t');
				writer.Write(marker.Label);
				if (!string.IsNullOrEmpty(marker.Description)) {
					writer.Write('\t');
					writer.Write(marker.Description);
				}

				writer.WriteLine();
			}
		}

		// Write Subtitles.txt
		if (movie.ExtraData?.TryGetValue("Subtitles", out string? subtitles) == true) {
			ZipArchiveEntry subEntry = archive.CreateEntry("Subtitles.txt", CompressionLevel.Optimal);
			using Stream subStream = subEntry.Open();
			using var writer = new StreamWriter(subStream, Encoding.UTF8);
			writer.Write(subtitles);
		}

		// Write Input Log.txt
		ZipArchiveEntry inputEntry = archive.CreateEntry("Input Log.txt", CompressionLevel.Optimal);
		using (Stream inputStream = inputEntry.Open())
		using (var writer = new StreamWriter(inputStream, Encoding.UTF8)) {
			// Write header line with button layout
			writer.WriteLine(GetBk2LogHeader(movie.SystemType, movie.ControllerCount));

			// Write frames
			foreach (InputFrame frame in movie.InputFrames) {
				writer.WriteLine(FormatBk2InputLine(frame, movie.SystemType, movie.ControllerCount));
			}
		}

		// Write SaveRam
		if (movie.InitialSram is { Length: > 0 }) {
			ZipArchiveEntry sramEntry = archive.CreateEntry("SaveRam", CompressionLevel.Optimal);
			using Stream sramStream = sramEntry.Open();
			sramStream.Write(movie.InitialSram);
		}

		// Write Savestate (using generic name)
		if (movie.InitialState is { Length: > 0 } && movie.StartType == StartType.Savestate) {
			ZipArchiveEntry ssEntry = archive.CreateEntry("Core", CompressionLevel.Optimal);
			using Stream ssStream = ssEntry.Open();
			ssStream.Write(movie.InitialState);
		}
	}

	private static string GetBk2LogHeader(SystemType system, int controllerCount) {
		var sb = new StringBuilder();
		sb.Append('|');

		// Add command column
		sb.Append(".|");

		// Add controller columns
		for (int i = 0; i < controllerCount; i++) {
			sb.Append(GetButtonLayoutHeader(system));
			sb.Append('|');
		}

		return sb.ToString();
	}

	private static string GetButtonLayoutHeader(SystemType system) {
		return system switch {
			SystemType.Nes => "RLDUTSBA",
			SystemType.Snes => "BYsSUDLRAXlr",
			SystemType.Genesis => "UDLRSABCxyzm",
			SystemType.Gb or SystemType.Gbc => "RLDUTSBA",
			SystemType.Gba => "RLDUTSBAlr",
			_ => "RLDUTSBA"
		};
	}

	private static string FormatBk2InputLine(InputFrame frame, SystemType system, int controllerCount) {
		var sb = new StringBuilder();
		sb.Append('|');

		// Command column
		if (frame.Command.HasFlag(FrameCommand.Reset)) {
			sb.Append('r');
		} else if (frame.Command.HasFlag(FrameCommand.Power)) {
			sb.Append('P');
		} else if (frame.Command.HasFlag(FrameCommand.LagFrame)) {
			sb.Append('L');
		} else {
			sb.Append('.');
		}

		sb.Append('|');

		// Controller columns
		for (int i = 0; i < controllerCount; i++) {
			ControllerInput input = frame.Controllers[i];
			sb.Append(FormatBk2Controller(input, system));
			sb.Append('|');
		}

		return sb.ToString();
	}

	private static string FormatBk2Controller(ControllerInput input, SystemType system) {
		return system switch {
			SystemType.Nes => FormatNes(input),
			SystemType.Snes => FormatSnes(input),
			SystemType.Genesis => FormatGenesis(input),
			SystemType.Gb or SystemType.Gbc => FormatNes(input),
			SystemType.Gba => FormatGba(input),
			_ => FormatNes(input)
		};
	}

	private static string FormatNes(ControllerInput i) {
		return string.Create(8, i, static (chars, input) => {
			chars[0] = input.Right ? 'R' : '.';
			chars[1] = input.Left ? 'L' : '.';
			chars[2] = input.Down ? 'D' : '.';
			chars[3] = input.Up ? 'U' : '.';
			chars[4] = input.Start ? 'T' : '.';
			chars[5] = input.Select ? 'S' : '.';
			chars[6] = input.B ? 'B' : '.';
			chars[7] = input.A ? 'A' : '.';
		});
	}

	private static string FormatSnes(ControllerInput i) {
		return string.Create(12, i, static (chars, input) => {
			chars[0] = input.B ? 'B' : '.';
			chars[1] = input.Y ? 'Y' : '.';
			chars[2] = input.Select ? 's' : '.';
			chars[3] = input.Start ? 'S' : '.';
			chars[4] = input.Up ? 'U' : '.';
			chars[5] = input.Down ? 'D' : '.';
			chars[6] = input.Left ? 'L' : '.';
			chars[7] = input.Right ? 'R' : '.';
			chars[8] = input.A ? 'A' : '.';
			chars[9] = input.X ? 'X' : '.';
			chars[10] = input.L ? 'l' : '.';
			chars[11] = input.R ? 'r' : '.';
		});
	}

	private static string FormatGenesis(ControllerInput i) {
		return string.Create(12, i, static (chars, input) => {
			chars[0] = input.Up ? 'U' : '.';
			chars[1] = input.Down ? 'D' : '.';
			chars[2] = input.Left ? 'L' : '.';
			chars[3] = input.Right ? 'R' : '.';
			chars[4] = input.Start ? 'S' : '.';
			chars[5] = input.A ? 'A' : '.';
			chars[6] = input.B ? 'B' : '.';
			chars[7] = input.C ? 'C' : '.';
			chars[8] = input.X ? 'x' : '.';
			chars[9] = input.Y ? 'y' : '.';
			chars[10] = input.Z ? 'z' : '.';
			chars[11] = input.Mode ? 'm' : '.';
		});
	}

	private static string FormatGba(ControllerInput i) {
		return string.Create(10, i, static (chars, input) => {
			chars[0] = input.Right ? 'R' : '.';
			chars[1] = input.Left ? 'L' : '.';
			chars[2] = input.Down ? 'D' : '.';
			chars[3] = input.Up ? 'U' : '.';
			chars[4] = input.Start ? 'T' : '.';
			chars[5] = input.Select ? 'S' : '.';
			chars[6] = input.B ? 'B' : '.';
			chars[7] = input.A ? 'A' : '.';
			chars[8] = input.L ? 'l' : '.';
			chars[9] = input.R ? 'r' : '.';
		});
	}
}
