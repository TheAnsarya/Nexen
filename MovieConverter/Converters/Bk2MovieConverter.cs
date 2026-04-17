using System.Globalization;
using System.IO.Compression;
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
			TryParseA2600PortTypesFromSyncSettings(movie);
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
				InputFrame frame = ParseBk2InputLine(line, frameNumber, movie.SystemType, movie.PortTypes, out bool isLag);
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
	private static InputFrame ParseBk2InputLine(string line, int frameNumber, SystemType system, ControllerType[]? portTypes, out bool isLag) {
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

		ReadOnlySpan<char> span = line.AsSpan();
		Span<Range> ranges = stackalloc Range[16];
		int count = span.Split(ranges, '|');
		int portIndex = 0;

		// Segment 1 is always the command column
		if (count > 2) {
			ReadOnlySpan<char> cmdSegment = span[ranges[1]];
			if (cmdSegment.Equals("r", StringComparison.OrdinalIgnoreCase) ||
				cmdSegment.Contains("Reset", StringComparison.OrdinalIgnoreCase)) {
				frame.Command |= FrameCommand.Reset;
			} else if (cmdSegment.Equals("P", StringComparison.OrdinalIgnoreCase) ||
				cmdSegment.Contains("Power", StringComparison.OrdinalIgnoreCase)) {
				frame.Command |= FrameCommand.Power;
			}
		}

		// For A2600, the last segment before trailing empty is console switches (Sr)
		int controllerEndIdx = count - 1;
		if (system == SystemType.A2600 && count > 3) {
			ReadOnlySpan<char> lastSegment = span[ranges[count - 2]];
			if (lastSegment.Length <= 2) {
				// Parse as console switches segment
				ParseA2600ConsoleSwitches(lastSegment, frame);
				controllerEndIdx = count - 2;
			}
		}

		// Segments 2..controllerEndIdx-1 are controller ports
		for (int i = 2; i < controllerEndIdx; i++) {
			ReadOnlySpan<char> segment = span[ranges[i]];

			// Parse controller input
			if (portIndex < InputFrame.MaxPorts) {
				ControllerType portType = system == SystemType.A2600 && portTypes != null && portIndex < portTypes.Length
					? portTypes[portIndex]
					: ControllerType.Gamepad;

				frame.Controllers[portIndex] = ParseBk2ControllerInput(segment, system, portType);
				if (system == SystemType.A2600) {
					frame.Controllers[portIndex].Type = portType;
				}
				portIndex++;
			}
		}

		return frame;
	}

	/// <summary>
	/// Parse BK2 controller button string
	/// </summary>
	private static ControllerInput ParseBk2ControllerInput(ReadOnlySpan<char> segment, SystemType system, ControllerType portType = ControllerType.Gamepad) {
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

			case SystemType.Lynx:
				ParseLynxInput(segment, input);
				break;

			case SystemType.A2600:
				ParseA2600Input(segment, input, portType);
				break;

			default:
				// Generic button parsing
				ParseGenericInput(segment, input);
				break;
		}

		return input;
	}

	private static void ParseNesInput(ReadOnlySpan<char> s, ControllerInput input) {
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

	private static void ParseSnesInput(ReadOnlySpan<char> s, ControllerInput input) {
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

	private static void ParseGenesisInput(ReadOnlySpan<char> s, ControllerInput input) {
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

	private static void ParseGbInput(ReadOnlySpan<char> s, ControllerInput input) =>
		// GB: RLDUTSBA (8 chars, same as NES)
		ParseNesInput(s, input);

	private static void ParseGbaInput(ReadOnlySpan<char> s, ControllerInput input) {
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

	private static void ParseLynxInput(ReadOnlySpan<char> s, ControllerInput input) {
		// Lynx: UDLRABOoP (9 chars)
		// U=Up, D=Down, L=Left, R=Right, A=A, B=B, O=Option1, o=Option2, P=Pause
		if (s.Length < 9) {
			return;
		}

		input.Up = s[0] != '.';
		input.Down = s[1] != '.';
		input.Left = s[2] != '.';
		input.Right = s[3] != '.';
		input.A = s[4] != '.';
		input.B = s[5] != '.';
		input.L = s[6] != '.';     // Option1 -> L
		input.R = s[7] != '.';     // Option2 -> R
		input.Start = s[8] != '.'; // Pause -> Start
	}

	private static void ParseA2600Input(ReadOnlySpan<char> s, ControllerInput input, ControllerType portType) {
		switch (portType) {
			case ControllerType.Atari2600Paddle:
				ParseA2600PaddleInput(s, input);
				return;

			case ControllerType.Atari2600Keypad:
				ParseA2600KeypadInput(s, input);
				return;

			case ControllerType.Atari2600DrivingController:
				ParseA2600DrivingInput(s, input);
				return;

			case ControllerType.Atari2600BoosterGrip:
				ParseA2600BoosterGripInput(s, input);
				return;

			default:
				ParseA2600JoystickInput(s, input);
				return;
		}
	}

	private static void ParseA2600JoystickInput(ReadOnlySpan<char> s, ControllerInput input) {
		// A2600 format: UDLRB (5 chars digital) or UDLRB,PPP (with paddle position)
		// U=Up, D=Down, L=Left, R=Right, B=Button (Fire), PPP=paddle 0-255
		int commaIdx = s.IndexOf(',');
		ReadOnlySpan<char> buttons = commaIdx >= 0 ? s[..commaIdx] : s;

		if (buttons.Length >= 1) input.Up = buttons[0] != '.';
		if (buttons.Length >= 2) input.Down = buttons[1] != '.';
		if (buttons.Length >= 3) input.Left = buttons[2] != '.';
		if (buttons.Length >= 4) input.Right = buttons[3] != '.';
		if (buttons.Length >= 5) input.A = buttons[4] != '.'; // Fire button mapped to A

		// Parse paddle position if present after comma
		if (commaIdx >= 0 && commaIdx + 1 < s.Length) {
			ReadOnlySpan<char> paddleStr = s[(commaIdx + 1)..].Trim();
			if (byte.TryParse(paddleStr, out byte paddleValue)) {
				input.PaddlePosition = paddleValue;
			}
		}
	}

	private static void ParseA2600PaddleInput(ReadOnlySpan<char> s, ControllerInput input) =>
		ParseA2600JoystickInput(s, input);

	private static void ParseA2600KeypadInput(ReadOnlySpan<char> s, ControllerInput input) {
		// Keypad order: 123456789*0#
		if (s.Length < 12) {
			return;
		}

		input.Y = s[0] != '.';
		input.X = s[1] != '.';
		input.L = s[2] != '.';
		input.B = s[3] != '.';
		input.A = s[4] != '.';
		input.R = s[5] != '.';
		input.Select = s[6] != '.';
		input.Start = s[7] != '.';
		input.Up = s[8] != '.';
		input.Left = s[9] != '.';
		input.Down = s[10] != '.';
		input.Right = s[11] != '.';
	}

	private static void ParseA2600DrivingInput(ReadOnlySpan<char> s, ControllerInput input) {
		// Driving format: LRB[,pos]
		int commaIdx = s.IndexOf(',');
		ReadOnlySpan<char> buttons = commaIdx >= 0 ? s[..commaIdx] : s;

		if (buttons.Length >= 1) input.Left = buttons[0] != '.';
		if (buttons.Length >= 2) input.Right = buttons[1] != '.';
		if (buttons.Length >= 3) input.A = buttons[2] != '.';

		if (commaIdx >= 0 && commaIdx + 1 < s.Length) {
			ReadOnlySpan<char> value = s[(commaIdx + 1)..].Trim();
			if (byte.TryParse(value, out byte rotary)) {
				input.PaddlePosition = rotary;
			}
		}
	}

	private static void ParseA2600BoosterGripInput(ReadOnlySpan<char> s, ControllerInput input) {
		// Booster Grip format: UDLRABC (fire/trigger/booster)
		if (s.Length < 7) {
			return;
		}

		input.Up = s[0] != '.';
		input.Down = s[1] != '.';
		input.Left = s[2] != '.';
		input.Right = s[3] != '.';
		input.A = s[4] != '.'; // Fire
		input.B = s[5] != '.'; // Trigger
		input.C = s[6] != '.'; // Booster
	}

	private static void ParseA2600ConsoleSwitches(ReadOnlySpan<char> s, InputFrame frame) {
		// Console switches: Sr (Select, Reset) — 2 chars
		for (int i = 0; i < s.Length; i++) {
			switch (s[i]) {
				case 'S':
					frame.Command |= FrameCommand.Atari2600Select;
					break;
				case 'r':
					frame.Command |= FrameCommand.Atari2600Reset;
					break;
			}
		}
	}

	private static void ParseGenericInput(ReadOnlySpan<char> s, ControllerInput input) {
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
		string? syncSettingsToWrite = movie.SyncSettings;
		if (movie.SystemType == SystemType.A2600) {
			syncSettingsToWrite = MergeA2600PortTypesIntoSyncSettings(syncSettingsToWrite, movie.PortTypes, movie.ControllerCount);
		}

		if (!string.IsNullOrEmpty(syncSettingsToWrite)) {
			ZipArchiveEntry syncEntry = archive.CreateEntry("SyncSettings.json", CompressionLevel.Optimal);
			using Stream syncStream = syncEntry.Open();
			using var writer = new StreamWriter(syncStream, Encoding.UTF8);
			writer.Write(syncSettingsToWrite);
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
			// Write log header marker (required for reading back)
			writer.WriteLine("[Input]");

			// Write frames
			var sb = new StringBuilder(64);
			foreach (InputFrame frame in movie.InputFrames) {
				sb.Clear();
				FormatBk2InputLine(sb, frame, movie.SystemType, movie.ControllerCount, movie.PortTypes);
				writer.Write(sb);
				writer.WriteLine();
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

		// A2600: add console switches column
		if (system == SystemType.A2600) {
			sb.Append("Sr|");
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
			SystemType.Lynx => "UDLRABOoP",
			SystemType.A2600 => "UDLRB",
			_ => "RLDUTSBA"
		};
	}

	private static void FormatBk2InputLine(StringBuilder sb, InputFrame frame, SystemType system, int controllerCount, ControllerType[]? portTypes) {
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
			ControllerType portType = system == SystemType.A2600 && portTypes != null && i < portTypes.Length
				? portTypes[i]
				: ControllerType.Gamepad;
			sb.Append(FormatBk2Controller(input, system, portType));
			sb.Append('|');
		}

		// A2600: append console switches segment
		if (system == SystemType.A2600) {
			sb.Append(frame.Command.HasFlag(FrameCommand.Atari2600Select) ? 'S' : '.');
			sb.Append(frame.Command.HasFlag(FrameCommand.Atari2600Reset) ? 'r' : '.');
			sb.Append('|');
		}
	}

	private static string FormatBk2Controller(ControllerInput input, SystemType system, ControllerType portType = ControllerType.Gamepad) {
		return system switch {
			SystemType.Nes => FormatNes(input),
			SystemType.Snes => FormatSnes(input),
			SystemType.Genesis => FormatGenesis(input),
			SystemType.Gb or SystemType.Gbc => FormatNes(input),
			SystemType.Gba => FormatGba(input),
			SystemType.Lynx => FormatLynx(input),
			SystemType.A2600 => FormatA2600(input, portType),
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

	private static string FormatLynx(ControllerInput i) {
		return string.Create(9, i, static (chars, input) => {
			chars[0] = input.Up ? 'U' : '.';
			chars[1] = input.Down ? 'D' : '.';
			chars[2] = input.Left ? 'L' : '.';
			chars[3] = input.Right ? 'R' : '.';
			chars[4] = input.A ? 'A' : '.';
			chars[5] = input.B ? 'B' : '.';
			chars[6] = input.L ? 'O' : '.';     // Option1
			chars[7] = input.R ? 'o' : '.';     // Option2
			chars[8] = input.Start ? 'P' : '.'; // Pause
		});
	}

	private static string FormatA2600(ControllerInput i, ControllerType portType) {
		return portType switch {
			ControllerType.Atari2600Paddle => FormatA2600Paddle(i),
			ControllerType.Atari2600Keypad => FormatA2600Keypad(i),
			ControllerType.Atari2600DrivingController => FormatA2600Driving(i),
			ControllerType.Atari2600BoosterGrip => FormatA2600BoosterGrip(i),
			_ => FormatA2600Joystick(i)
		};
	}

	private static string FormatA2600Joystick(ControllerInput i) {
		var buttons = string.Create(5, i, static (chars, input) => {
			chars[0] = input.Up ? 'U' : '.';
			chars[1] = input.Down ? 'D' : '.';
			chars[2] = input.Left ? 'L' : '.';
			chars[3] = input.Right ? 'R' : '.';
			chars[4] = input.A ? 'B' : '.'; // Fire button
		});

		// Append paddle position if present: UDLRB,  128
		if (i.PaddlePosition is byte paddle) {
			return $"{buttons},{paddle,5}";
		}

		return buttons;
	}

	private static string FormatA2600Paddle(ControllerInput i) => FormatA2600Joystick(i);

	private static string FormatA2600Keypad(ControllerInput i) {
		return string.Create(12, i, static (chars, input) => {
			chars[0] = input.Y ? '1' : '.';
			chars[1] = input.X ? '2' : '.';
			chars[2] = input.L ? '3' : '.';
			chars[3] = input.B ? '4' : '.';
			chars[4] = input.A ? '5' : '.';
			chars[5] = input.R ? '6' : '.';
			chars[6] = input.Select ? '7' : '.';
			chars[7] = input.Start ? '8' : '.';
			chars[8] = input.Up ? '9' : '.';
			chars[9] = input.Left ? '*' : '.';
			chars[10] = input.Down ? '0' : '.';
			chars[11] = input.Right ? '#' : '.';
		});
	}

	private static string FormatA2600Driving(ControllerInput i) {
		var buttons = string.Create(3, i, static (chars, input) => {
			chars[0] = input.Left ? 'L' : '.';
			chars[1] = input.Right ? 'R' : '.';
			chars[2] = input.A ? 'B' : '.';
		});

		if (i.PaddlePosition is byte rotary) {
			return $"{buttons},{rotary,5}";
		}

		return buttons;
	}

	private static string FormatA2600BoosterGrip(ControllerInput i) {
		return string.Create(7, i, static (chars, input) => {
			chars[0] = input.Up ? 'U' : '.';
			chars[1] = input.Down ? 'D' : '.';
			chars[2] = input.Left ? 'L' : '.';
			chars[3] = input.Right ? 'R' : '.';
			chars[4] = input.A ? 'A' : '.'; // Fire
			chars[5] = input.B ? 'B' : '.'; // Trigger
			chars[6] = input.C ? 'C' : '.'; // Booster
		});
	}

	private static void TryParseA2600PortTypesFromSyncSettings(MovieData movie) {
		if (movie.SystemType != SystemType.A2600 || string.IsNullOrWhiteSpace(movie.SyncSettings)) {
			return;
		}

		try {
			using JsonDocument doc = JsonDocument.Parse(movie.SyncSettings);
			if (!doc.RootElement.TryGetProperty("Nexen", out JsonElement nexenNode)) {
				return;
			}

			if (!nexenNode.TryGetProperty("A2600PortTypes", out JsonElement portTypesNode) ||
				portTypesNode.ValueKind != JsonValueKind.Array) {
				return;
			}

			int idx = 0;
			foreach (JsonElement element in portTypesNode.EnumerateArray()) {
				if (idx >= movie.PortTypes.Length) {
					break;
				}

				if (element.ValueKind == JsonValueKind.String) {
					movie.PortTypes[idx] = ParseA2600PortTypeString(element.GetString());
				}

				idx++;
			}
		} catch (JsonException) {
			// Ignore malformed SyncSettings and keep default controller port types.
		}
	}

	private static string MergeA2600PortTypesIntoSyncSettings(string? existingSyncSettings, ControllerType[]? portTypes, int controllerCount) {
		if (portTypes == null || controllerCount <= 0) {
			return existingSyncSettings ?? string.Empty;
		}

		int count = Math.Clamp(controllerCount, 1, InputFrame.MaxPorts);
		var typeNames = new string[count];
		for (int i = 0; i < count; i++) {
			typeNames[i] = ToA2600PortTypeString(i < portTypes.Length ? portTypes[i] : ControllerType.Atari2600Joystick);
		}

		try {
			using var output = new MemoryStream();
			using var writer = new Utf8JsonWriter(output);

			if (!string.IsNullOrWhiteSpace(existingSyncSettings)) {
				using JsonDocument doc = JsonDocument.Parse(existingSyncSettings);
				if (doc.RootElement.ValueKind != JsonValueKind.Object) {
					return existingSyncSettings;
				}

				writer.WriteStartObject();
				bool hasNexen = false;
				foreach (JsonProperty prop in doc.RootElement.EnumerateObject()) {
					if (prop.NameEquals("Nexen")) {
						hasNexen = true;
						WriteMergedNexenObject(writer, prop.Value, typeNames);
					} else {
						prop.WriteTo(writer);
					}
				}

				if (!hasNexen) {
					WriteMergedNexenObject(writer, null, typeNames);
				}

				writer.WriteEndObject();
			} else {
				writer.WriteStartObject();
				WriteMergedNexenObject(writer, null, typeNames);
				writer.WriteEndObject();
			}

			writer.Flush();
			return Encoding.UTF8.GetString(output.ToArray());
		} catch (JsonException) {
			// Preserve existing settings if invalid and still emit A2600 metadata if no settings exist.
			if (!string.IsNullOrWhiteSpace(existingSyncSettings)) {
				return existingSyncSettings;
			}

			using var output = new MemoryStream();
			using var writer = new Utf8JsonWriter(output);
			writer.WriteStartObject();
			WriteMergedNexenObject(writer, null, typeNames);
			writer.WriteEndObject();
			writer.Flush();
			return Encoding.UTF8.GetString(output.ToArray());
		}
	}

	private static void WriteMergedNexenObject(Utf8JsonWriter writer, JsonElement? existingNexen, string[] typeNames) {
		writer.WritePropertyName("Nexen");
		writer.WriteStartObject();

		if (existingNexen is JsonElement existing && existing.ValueKind == JsonValueKind.Object) {
			foreach (JsonProperty prop in existing.EnumerateObject()) {
				if (!prop.NameEquals("A2600PortTypes")) {
					prop.WriteTo(writer);
				}
			}
		}

		writer.WritePropertyName("A2600PortTypes");
		writer.WriteStartArray();
		foreach (string typeName in typeNames) {
			writer.WriteStringValue(typeName);
		}
		writer.WriteEndArray();

		writer.WriteEndObject();
	}

	private static string ToA2600PortTypeString(ControllerType portType) {
		return portType switch {
			ControllerType.Atari2600Paddle => "Atari2600Paddle",
			ControllerType.Atari2600Keypad => "Atari2600Keypad",
			ControllerType.Atari2600DrivingController => "Atari2600DrivingController",
			ControllerType.Atari2600BoosterGrip => "Atari2600BoosterGrip",
			_ => "Atari2600Joystick"
		};
	}

	private static ControllerType ParseA2600PortTypeString(string? value) {
		return value switch {
			"Atari2600Paddle" => ControllerType.Atari2600Paddle,
			"Atari2600Keypad" => ControllerType.Atari2600Keypad,
			"Atari2600DrivingController" => ControllerType.Atari2600DrivingController,
			"Atari2600BoosterGrip" => ControllerType.Atari2600BoosterGrip,
			_ => ControllerType.Atari2600Joystick
		};
	}
}
