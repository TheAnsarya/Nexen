using System.IO.Compression;
using System.Text;
using System.Text.Json;

namespace Mesen.MovieConverter.Converters;

/// <summary>
/// Converter for BizHawk BK2 movie format (.bk2)
/// ZIP archive with JSON header and text-based input log
/// </summary>
public class Bk2MovieConverter : MovieConverterBase
{
    public override string[] Extensions => new[] { ".bk2" };
    public override string FormatName => "BizHawk Movie";
    public override MovieFormat Format => MovieFormat.Bk2;

    public override MovieData Read(Stream stream, string? fileName = null)
    {
        var movie = new MovieData { SourceFormat = MovieFormat.Bk2 };

        using var archive = new ZipArchive(stream, ZipArchiveMode.Read, leaveOpen: true);

        // Read Header.txt (key=value format)
        var headerEntry = archive.GetEntry("Header.txt");
        if (headerEntry != null)
        {
            using var reader = new StreamReader(headerEntry.Open());
            ParseHeader(movie, reader.ReadToEnd());
        }

        // Read Comments.txt
        var commentsEntry = archive.GetEntry("Comments.txt");
        if (commentsEntry != null)
        {
            using var reader = new StreamReader(commentsEntry.Open());
            movie.Description = reader.ReadToEnd().Trim();
        }

        // Read Subtitles.txt (optional)
        var subtitlesEntry = archive.GetEntry("Subtitles.txt");
        // Could parse subtitles if needed

        // Read SyncSettings.json (optional, contains detailed settings)
        var syncEntry = archive.GetEntry("SyncSettings.json");
        // Could parse if needed for controller config

        // Read Input Log.txt
        var inputEntry = archive.GetEntry("Input Log.txt");
        if (inputEntry != null)
        {
            using var reader = new StreamReader(inputEntry.Open());
            ParseInputLog(movie, reader.ReadToEnd());
        }

        // Read savestate if present
        var coreStateEntry = archive.GetEntry("Core.bin") ?? archive.GetEntry("CoreState.bin");
        if (coreStateEntry != null)
        {
            using var ms = new MemoryStream();
            coreStateEntry.Open().CopyTo(ms);
            movie.InitialState = ms.ToArray();
        }

        return movie;
    }

    private void ParseHeader(MovieData movie, string content)
    {
        var lines = content.Split('\n', StringSplitOptions.RemoveEmptyEntries);
        
        foreach (var line in lines)
        {
            int eqIndex = line.IndexOf(' ');
            if (eqIndex < 0) continue;

            string key = line.Substring(0, eqIndex).Trim();
            string value = line.Substring(eqIndex + 1).Trim();

            switch (key.ToLower())
            {
                case "movieversion":
                    movie.SourceFormatVersion = value;
                    break;
                case "author":
                    movie.Author = value;
                    break;
                case "emuversion":
                    movie.EmulatorVersion = value;
                    break;
                case "rerecords":
                    if (uint.TryParse(value, out var rerecords))
                        movie.RerecordCount = rerecords;
                    break;
                case "platform":
                case "core":
                    movie.SystemType = ParsePlatform(value);
                    break;
                case "gamename":
                    movie.GameName = value;
                    break;
                case "sha1":
                    movie.Sha1Hash = value;
                    break;
                case "md5":
                    movie.Md5Hash = value;
                    break;
                case "startsfromsavestate":
                    // Already handled by Core.bin presence
                    break;
                case "pal":
                    movie.Region = value.ToLower() == "true" ? RegionType.PAL : RegionType.NTSC;
                    break;
            }
        }
    }

    private SystemType ParsePlatform(string platform)
    {
        return platform.ToLower() switch
        {
            "snes" or "bsnes" => SystemType.Snes,
            "nes" => SystemType.Nes,
            "gb" or "gameboy" or "gambatte" or "sgb" => SystemType.Gb,
            "gbc" => SystemType.Gbc,
            "gba" or "mgba" => SystemType.Gba,
            "sms" or "mastersystem" => SystemType.Sms,
            "genesis" or "gen" or "md" => SystemType.Genesis,
            "pce" or "pcecd" => SystemType.Pce,
            _ => SystemType.Other
        };
    }

    private void ParseInputLog(MovieData movie, string content)
    {
        var lines = content.Split('\n');
        string? logKey = null;

        foreach (var rawLine in lines)
        {
            var line = rawLine.Trim();
            
            // Header line with format definition
            if (line.StartsWith("["))
            {
                // [ControllerType:#|...] format definition
                logKey = line;
                continue;
            }

            // Skip empty lines
            if (string.IsNullOrEmpty(line)) continue;

            // Input line starts with |
            if (line.StartsWith("|"))
            {
                ParseBk2InputLine(movie, line, logKey);
            }
        }
    }

    private void ParseBk2InputLine(MovieData movie, string line, string? logKey)
    {
        // BK2 format varies by system but generally:
        // |commands|controller1|controller2|...|
        
        var parts = line.Split('|', StringSplitOptions.None);
        if (parts.Length < 2) return;

        var frame = new InputFrame { FrameNumber = movie.InputFrames.Count };

        // Parse based on system type
        switch (movie.SystemType)
        {
            case SystemType.Snes:
                ParseSnesInput(frame, parts);
                break;
            case SystemType.Nes:
                ParseNesInput(frame, parts);
                break;
            case SystemType.Gb:
            case SystemType.Gbc:
                ParseGbInput(frame, parts);
                break;
            case SystemType.Gba:
                ParseGbaInput(frame, parts);
                break;
            default:
                // Try generic parsing
                ParseGenericInput(frame, parts);
                break;
        }

        movie.InputFrames.Add(frame);
    }

    private void ParseSnesInput(InputFrame frame, string[] parts)
    {
        // SNES BK2: |commands|UDLRsSYBAXlr|UDLRsSYBAXlr|
        // or: |power|reset|P1 Up|P1 Down|...|
        
        // Try to parse each part as controller input
        for (int i = 1; i < parts.Length && i <= 4; i++)
        {
            string input = parts[i];
            if (string.IsNullOrEmpty(input)) continue;

            // Check for various SNES formats
            if (input.Length >= 12)
            {
                // Compact format
                var ctrl = frame.Controllers[i - 1];
                ctrl.Type = ControllerType.Gamepad;
                
                // Try common orders: UDLRsSYBAXlr or BYsSudlrAXLR
                // Just do a character check for presence
                ctrl.Up = ContainsButton(input, 'U', 'u');
                ctrl.Down = ContainsButton(input, 'D', 'd');
                ctrl.Left = ContainsButton(input, 'L', 'l');
                ctrl.Right = ContainsButton(input, 'R', 'r');
                ctrl.Start = ContainsButton(input, 'S', 's') || ContainsButton(input, 'T', 't');
                ctrl.Select = ContainsButton(input, 's', 'e');
                ctrl.A = ContainsButton(input, 'A', 'a');
                ctrl.B = ContainsButton(input, 'B', 'b');
                ctrl.X = ContainsButton(input, 'X', 'x');
                ctrl.Y = ContainsButton(input, 'Y', 'y');
                ctrl.L = input.Contains('l') || input.Contains("L ");
                ctrl.R = input.Contains('r') || input.Contains("R ");
            }
        }
    }

    private void ParseNesInput(InputFrame frame, string[] parts)
    {
        // NES BK2: |power|reset|P1 Up, P1 Down, P1 Left, P1 Right, P1 Start, P1 Select, P1 B, P1 A|...|
        // or compact: |commands|UDLRTSBA|UDLRTSBA|
        
        for (int i = 1; i < parts.Length && i <= 4; i++)
        {
            string input = parts[i];
            if (string.IsNullOrEmpty(input)) continue;

            var ctrl = frame.Controllers[Math.Min(i - 1, 3)];
            ctrl.Type = ControllerType.Gamepad;

            // Check for verbose or compact format
            if (input.Contains(','))
            {
                // Verbose format with comma-separated button names
                ctrl.Up = input.Contains("Up");
                ctrl.Down = input.Contains("Down");
                ctrl.Left = input.Contains("Left");
                ctrl.Right = input.Contains("Right");
                ctrl.Start = input.Contains("Start");
                ctrl.Select = input.Contains("Select");
                ctrl.B = input.Contains(" B") || input.EndsWith("B");
                ctrl.A = input.Contains(" A") || input.EndsWith("A");
            }
            else if (input.Length >= 8)
            {
                // Compact format: UDLRTSBA
                ctrl.Up = input[0] != '.';
                ctrl.Down = input[1] != '.';
                ctrl.Left = input[2] != '.';
                ctrl.Right = input[3] != '.';
                ctrl.Start = input[4] != '.';
                ctrl.Select = input[5] != '.';
                ctrl.B = input[6] != '.';
                ctrl.A = input[7] != '.';
            }
        }
    }

    private void ParseGbInput(InputFrame frame, string[] parts)
    {
        // GB/GBC BK2: |commands|UDLRsSBA|
        if (parts.Length >= 2)
        {
            string input = parts[1];
            if (input.Length >= 8)
            {
                var ctrl = frame.Controllers[0];
                ctrl.Type = ControllerType.Gamepad;
                
                ctrl.Up = input[0] != '.';
                ctrl.Down = input[1] != '.';
                ctrl.Left = input[2] != '.';
                ctrl.Right = input[3] != '.';
                ctrl.Start = input[4] != '.';
                ctrl.Select = input[5] != '.';
                ctrl.B = input[6] != '.';
                ctrl.A = input[7] != '.';
            }
        }
    }

    private void ParseGbaInput(InputFrame frame, string[] parts)
    {
        // GBA BK2: |commands|UDLRsSBALR|
        if (parts.Length >= 2)
        {
            string input = parts[1];
            if (input.Length >= 10)
            {
                var ctrl = frame.Controllers[0];
                ctrl.Type = ControllerType.Gamepad;
                
                ctrl.Up = input[0] != '.';
                ctrl.Down = input[1] != '.';
                ctrl.Left = input[2] != '.';
                ctrl.Right = input[3] != '.';
                ctrl.Start = input[4] != '.';
                ctrl.Select = input[5] != '.';
                ctrl.B = input[6] != '.';
                ctrl.A = input[7] != '.';
                ctrl.L = input[8] != '.';
                ctrl.R = input[9] != '.';
            }
        }
    }

    private void ParseGenericInput(InputFrame frame, string[] parts)
    {
        // Try to parse as generic SNES-like input
        ParseSnesInput(frame, parts);
    }

    private bool ContainsButton(string input, char upper, char lower)
    {
        foreach (char c in input)
        {
            if (c == upper || c == lower) return true;
        }
        return false;
    }

    public override void Write(MovieData movie, Stream stream)
    {
        using var archive = new ZipArchive(stream, ZipArchiveMode.Create, leaveOpen: true);

        // Write Header.txt
        var headerEntry = archive.CreateEntry("Header.txt");
        using (var writer = new StreamWriter(headerEntry.Open()))
        {
            writer.WriteLine("MovieVersion BizHawk v2.0.0");
            writer.WriteLine($"Author {movie.Author}");
            writer.WriteLine($"emuVersion 2.9.1");
            writer.WriteLine($"rerecords {movie.RerecordCount}");
            writer.WriteLine($"Platform {GetPlatformString(movie.SystemType)}");
            writer.WriteLine($"GameName {movie.GameName}");
            
            if (!string.IsNullOrEmpty(movie.Sha1Hash))
                writer.WriteLine($"SHA1 {movie.Sha1Hash}");
            if (!string.IsNullOrEmpty(movie.Md5Hash))
                writer.WriteLine($"MD5 {movie.Md5Hash}");
            
            writer.WriteLine($"PAL {(movie.Region == RegionType.PAL ? "True" : "False")}");
            writer.WriteLine($"StartsFromSavestate {(movie.StartsFromSavestate ? "True" : "False")}");
        }

        // Write Comments.txt
        if (!string.IsNullOrEmpty(movie.Description))
        {
            var commentsEntry = archive.CreateEntry("Comments.txt");
            using var writer = new StreamWriter(commentsEntry.Open());
            writer.Write(movie.Description);
        }

        // Write Input Log.txt
        var inputEntry = archive.CreateEntry("Input Log.txt");
        using (var writer = new StreamWriter(inputEntry.Open()))
        {
            // Write log key header
            writer.WriteLine(GetLogKey(movie.SystemType));

            foreach (var frame in movie.InputFrames)
            {
                writer.WriteLine(FormatInputLine(frame, movie.SystemType));
            }

            // Write footer
            writer.WriteLine("[/Input]");
        }

        // Write savestate if present
        if (movie.InitialState != null && movie.InitialState.Length > 0)
        {
            var stateEntry = archive.CreateEntry("Core.bin");
            using var stateStream = stateEntry.Open();
            stateStream.Write(movie.InitialState, 0, movie.InitialState.Length);
        }
    }

    private string GetPlatformString(SystemType system)
    {
        return system switch
        {
            SystemType.Snes => "SNES",
            SystemType.Nes => "NES",
            SystemType.Gb => "GB",
            SystemType.Gbc => "GBC",
            SystemType.Gba => "GBA",
            SystemType.Sms => "SMS",
            SystemType.Genesis => "GEN",
            SystemType.Pce => "PCE",
            _ => "SNES"
        };
    }

    private string GetLogKey(SystemType system)
    {
        return system switch
        {
            SystemType.Snes => "[Input]\nLogKey:#Power|Reset|P1 Up|P1 Down|P1 Left|P1 Right|P1 Select|P1 Start|P1 Y|P1 B|P1 A|P1 X|P1 L|P1 R|P2 Up|P2 Down|P2 Left|P2 Right|P2 Select|P2 Start|P2 Y|P2 B|P2 A|P2 X|P2 L|P2 R|",
            SystemType.Nes => "[Input]\nLogKey:#Power|Reset|P1 Up|P1 Down|P1 Left|P1 Right|P1 Start|P1 Select|P1 B|P1 A|P2 Up|P2 Down|P2 Left|P2 Right|P2 Start|P2 Select|P2 B|P2 A|",
            SystemType.Gb or SystemType.Gbc => "[Input]\nLogKey:#Power|Up|Down|Left|Right|Start|Select|B|A|",
            SystemType.Gba => "[Input]\nLogKey:#Power|Up|Down|Left|Right|Start|Select|B|A|L|R|",
            _ => "[Input]\nLogKey:#Reset|P1|P2|"
        };
    }

    private string FormatInputLine(InputFrame frame, SystemType system)
    {
        var sb = new StringBuilder("|");

        // Power/Reset commands
        sb.Append(frame.Command.HasFlag(FrameCommand.HardReset) ? "1" : ".");
        sb.Append("|");
        sb.Append(frame.Command.HasFlag(FrameCommand.SoftReset) ? "1" : ".");
        sb.Append("|");

        switch (system)
        {
            case SystemType.Snes:
                // P1
                sb.Append(FormatSnesController(frame.Controllers[0]));
                sb.Append("|");
                // P2
                sb.Append(FormatSnesController(frame.Controllers[1]));
                sb.Append("|");
                break;
            case SystemType.Nes:
                // P1
                sb.Append(FormatNesController(frame.Controllers[0]));
                sb.Append("|");
                // P2
                sb.Append(FormatNesController(frame.Controllers[1]));
                sb.Append("|");
                break;
            default:
                sb.Append(FormatSnesController(frame.Controllers[0]));
                sb.Append("|");
                break;
        }

        return sb.ToString();
    }

    private string FormatSnesController(ControllerInput ctrl)
    {
        var sb = new StringBuilder();
        sb.Append(ctrl.Up ? 'U' : '.');
        sb.Append(ctrl.Down ? 'D' : '.');
        sb.Append(ctrl.Left ? 'L' : '.');
        sb.Append(ctrl.Right ? 'R' : '.');
        sb.Append(ctrl.Select ? 's' : '.');
        sb.Append(ctrl.Start ? 'S' : '.');
        sb.Append(ctrl.Y ? 'Y' : '.');
        sb.Append(ctrl.B ? 'B' : '.');
        sb.Append(ctrl.A ? 'A' : '.');
        sb.Append(ctrl.X ? 'X' : '.');
        sb.Append(ctrl.L ? 'l' : '.');
        sb.Append(ctrl.R ? 'r' : '.');
        return sb.ToString();
    }

    private string FormatNesController(ControllerInput ctrl)
    {
        var sb = new StringBuilder();
        sb.Append(ctrl.Up ? 'U' : '.');
        sb.Append(ctrl.Down ? 'D' : '.');
        sb.Append(ctrl.Left ? 'L' : '.');
        sb.Append(ctrl.Right ? 'R' : '.');
        sb.Append(ctrl.Start ? 'T' : '.');
        sb.Append(ctrl.Select ? 'S' : '.');
        sb.Append(ctrl.B ? 'B' : '.');
        sb.Append(ctrl.A ? 'A' : '.');
        return sb.ToString();
    }
}
