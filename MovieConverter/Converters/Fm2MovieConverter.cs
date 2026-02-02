using System.Globalization;
using System.Runtime.CompilerServices;
using System.Text;

namespace Nexen.MovieConverter.Converters;

/// <summary>
/// Converter for FCEUX FM2 movie format.
///
/// FM2 is a text-based format containing:
/// - Header section with key-value pairs
/// - Input log with one line per frame
///
/// Reference: https://fceux.com/web/FM2.html
/// </summary>
public sealed class Fm2MovieConverter : MovieConverterBase {
	/// <inheritdoc/>
	public override string[] Extensions => [".fm2"];

	/// <inheritdoc/>
	public override string FormatName => "FCEUX Movie";

	/// <inheritdoc/>
	public override MovieFormat Format => MovieFormat.Fm2;

	/// <inheritdoc/>
	public override MovieData Read(Stream stream, string? fileName = null) {
		ArgumentNullException.ThrowIfNull(stream);

		using var reader = new StreamReader(stream, Encoding.UTF8, leaveOpen: true);

		var movie = new MovieData {
			SourceFormat = MovieFormat.Fm2,
			SystemType = SystemType.Nes,
			ControllerCount = 2
		};

		bool inHeader = true;
		int frameNumber = 0;

		while (reader.ReadLine() is { } line) {
			// Skip empty lines
			if (string.IsNullOrWhiteSpace(line)) {
				continue;
			}

			// Skip comments
			if (line.StartsWith("comment ", StringComparison.OrdinalIgnoreCase)) {
				if (!string.IsNullOrEmpty(movie.Description)) {
					movie.Description += "\n";
				}

				movie.Description += line[8..];
				continue;
			}

			// Header lines are key-value pairs
			if (inHeader && !line.StartsWith('|')) {
				ParseFm2HeaderLine(line, movie);
				continue;
			}

			// Input lines start with |
			if (line.StartsWith('|')) {
				inHeader = false;
				InputFrame frame = ParseFm2InputLine(line, frameNumber);
				movie.InputFrames.Add(frame);
				frameNumber++;
			}
		}

		return movie;
	}

	/// <summary>
	/// Parse FM2 header line
	/// </summary>
	private static void ParseFm2HeaderLine(string line, MovieData movie) {
		int spaceIdx = line.IndexOf(' ');
		if (spaceIdx <= 0) {
			return;
		}

		string key = line[..spaceIdx];
		string value = line[(spaceIdx + 1)..];

		switch (key.ToUpperInvariant()) {
			case "VERSION":
				movie.SourceFormatVersion = value;
				break;

			case "EMUVERSION":
				movie.EmulatorVersion = value;
				break;

			case "RERECORDCOUNT":
				if (ulong.TryParse(value, out ulong rr)) {
					movie.RerecordCount = rr;
				}

				break;

			case "PALMODE":
				if (value == "1" || value.Equals("true", StringComparison.OrdinalIgnoreCase)) {
					movie.Region = RegionType.PAL;
				}

				break;

			case "NEWPPU":
				movie.ExtraData ??= [];
				movie.ExtraData["NewPPU"] = value;
				break;

			case "FDS":
				movie.ExtraData ??= [];
				movie.ExtraData["FDS"] = value;
				break;

			case "FOURSCORE":
				if (value == "1" || value.Equals("true", StringComparison.OrdinalIgnoreCase)) {
					movie.ControllerCount = 4;
					movie.PortTypes[0] = ControllerType.FourScore;
				}

				break;

			case "PORT0":
			case "PORT1":
			case "PORT2":
				int portNum = key[4] - '0';
				if (portNum is >= 0 and < InputFrame.MaxPorts) {
					movie.PortTypes[portNum] = ParseFm2PortType(value);
				}

				break;

			case "ROMFILENAME":
				movie.RomFileName = value;
				break;

			case "ROMCHECKSUM":
				// FM2 uses base16 MD5
				movie.Md5Hash = value;
				break;

			case "GUID":
				movie.ExtraData ??= [];
				movie.ExtraData["GUID"] = value;
				break;

			case "STARTSFROMSAVESTATE":
				if (value == "1" || value.Equals("true", StringComparison.OrdinalIgnoreCase)) {
					movie.StartType = StartType.Savestate;
				}

				break;

			case "SAVEFROMFULLNES":
				// Savestate embedded in movie
				movie.ExtraData ??= [];
				movie.ExtraData["SaveFromFullNES"] = value;
				break;

			case "SUBTITLE":
				// Format: frame text
				string[] parts = value.Split(' ', 2);
				if (parts.Length >= 2 && int.TryParse(parts[0], out int frameNum)) {
					movie.Markers ??= [];
					movie.Markers.Add(new MovieMarker {
						FrameNumber = frameNum,
						Label = parts[1]
					});
				}

				break;
		}
	}

	/// <summary>
	/// Convert FM2 port type string to ControllerType
	/// </summary>
	private static ControllerType ParseFm2PortType(string value) {
		return int.TryParse(value, out int v) ? v switch {
			0 => ControllerType.None,
			1 => ControllerType.Gamepad,
			2 => ControllerType.Zapper,
			_ => ControllerType.Gamepad
		} : ControllerType.Gamepad;
	}

	/// <summary>
	/// Parse FM2 input line
	/// </summary>
	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	private static InputFrame ParseFm2InputLine(string line, int frameNumber) {
		var frame = new InputFrame {
			FrameNumber = frameNumber
		};

		// FM2 format: |command|P1|P2|...|
		// NES buttons: RLDUTSBA
		string[] parts = line.Split('|');

		if (parts.Length >= 2) {
			// Parse command byte
			string cmd = parts[1];
			if (!string.IsNullOrEmpty(cmd)) {
				foreach (char c in cmd) {
					switch (c) {
						case '1': // Reset
							frame.Command |= FrameCommand.Reset;
							break;
						case '2': // Power
							frame.Command |= FrameCommand.Power;
							break;
						case '4': // FDS disk insert
							frame.Command |= FrameCommand.FdsInsert;
							break;
						case '8': // FDS disk side
							frame.Command |= FrameCommand.FdsSide;
							break;
						case '0': // VS insert coin
							frame.Command |= FrameCommand.VsInsertCoin;
							break;
					}
				}
			}
		}

		// Parse controller inputs
		for (int port = 0; port < Math.Min(parts.Length - 2, InputFrame.MaxPorts); port++) {
			string buttonStr = parts[port + 2];
			if (string.IsNullOrEmpty(buttonStr) || buttonStr.Length < 8) {
				continue;
			}

			ControllerInput input = frame.Controllers[port];

			// FM2 NES button order: RLDUTSBA
			input.Right = buttonStr[0] != '.';
			input.Left = buttonStr[1] != '.';
			input.Down = buttonStr[2] != '.';
			input.Up = buttonStr[3] != '.';
			input.Start = buttonStr[4] != '.';
			input.Select = buttonStr[5] != '.';
			input.B = buttonStr[6] != '.';
			input.A = buttonStr[7] != '.';
		}

		return frame;
	}

	/// <inheritdoc/>
	public override void Write(MovieData movie, Stream stream) {
		ArgumentNullException.ThrowIfNull(movie);
		ArgumentNullException.ThrowIfNull(stream);

		using var writer = new StreamWriter(stream, Encoding.UTF8, leaveOpen: true);

		// Write header
		writer.WriteLine("version 3");
		writer.WriteLine($"emuVersion {movie.EmulatorVersion ?? "FCEUX 2.6.6"}");
		writer.WriteLine($"rerecordCount {movie.RerecordCount}");
		writer.WriteLine($"palFlag {(movie.Region == RegionType.PAL ? "1" : "0")}");

		if (!string.IsNullOrEmpty(movie.RomFileName)) {
			writer.WriteLine($"romFilename {movie.RomFileName}");
		}

		if (!string.IsNullOrEmpty(movie.Md5Hash)) {
			writer.WriteLine($"romChecksum base64:{movie.Md5Hash}");
		}

		// Generate GUID if not present
		string guid = movie.ExtraData?.GetValueOrDefault("GUID")
			?? Guid.NewGuid().ToString("B").ToUpperInvariant();
		writer.WriteLine($"guid {guid}");

		// Port configuration
		for (int i = 0; i < Math.Min(movie.ControllerCount, 2); i++) {
			int portType = movie.PortTypes[i] switch {
				ControllerType.None => 0,
				ControllerType.Gamepad => 1,
				ControllerType.Zapper => 2,
				_ => 1
			};
			writer.WriteLine($"port{i} {portType}");
		}

		if (movie.ControllerCount >= 4) {
			writer.WriteLine("fourscore 1");
		}

		// Start type
		if (movie.StartType == StartType.Savestate) {
			writer.WriteLine("startsFromSavestate 1");
		}

		// Comments
		if (!string.IsNullOrEmpty(movie.Description)) {
			foreach (string line in movie.Description.Split('\n')) {
				writer.WriteLine($"comment {line}");
			}
		}

		// Subtitles from markers
		if (movie.Markers is { Count: > 0 }) {
			foreach (MovieMarker marker in movie.Markers) {
				writer.WriteLine($"subtitle {marker.FrameNumber} {marker.Label}");
			}
		}

		// Write input frames
		foreach (InputFrame frame in movie.InputFrames) {
			writer.WriteLine(FormatFm2InputLine(frame, movie.ControllerCount));
		}
	}

	/// <summary>
	/// Format frame as FM2 input line
	/// </summary>
	private static string FormatFm2InputLine(InputFrame frame, int controllerCount) {
		var sb = new StringBuilder(32);
		sb.Append('|');

		// Command byte
		char cmd = '.';
		if (frame.Command.HasFlag(FrameCommand.Reset)) {
			cmd = '1';
		} else if (frame.Command.HasFlag(FrameCommand.Power)) {
			cmd = '2';
		} else if (frame.Command.HasFlag(FrameCommand.FdsInsert)) {
			cmd = '4';
		} else if (frame.Command.HasFlag(FrameCommand.FdsSide)) {
			cmd = '8';
		} else if (frame.Command.HasFlag(FrameCommand.VsInsertCoin)) {
			cmd = '0';
		}

		sb.Append(cmd);
		sb.Append('|');

		// Controller inputs
		for (int i = 0; i < controllerCount; i++) {
			ControllerInput input = frame.Controllers[i];

			// FM2 NES button order: RLDUTSBA
			sb.Append(input.Right ? 'R' : '.');
			sb.Append(input.Left ? 'L' : '.');
			sb.Append(input.Down ? 'D' : '.');
			sb.Append(input.Up ? 'U' : '.');
			sb.Append(input.Start ? 'T' : '.');
			sb.Append(input.Select ? 'S' : '.');
			sb.Append(input.B ? 'B' : '.');
			sb.Append(input.A ? 'A' : '.');
			sb.Append('|');
		}

		return sb.ToString();
	}
}
