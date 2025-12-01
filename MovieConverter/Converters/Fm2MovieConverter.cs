using System.Text;

namespace Mesen.MovieConverter.Converters;

/// <summary>
/// Converter for FCEUX FM2 movie format (.fm2)
/// Text-based format for NES movies
/// </summary>
public class Fm2MovieConverter : MovieConverterBase
{
    public override string[] Extensions => new[] { ".fm2" };
    public override string FormatName => "FCEUX Movie";
    public override MovieFormat Format => MovieFormat.Fm2;

    public override MovieData Read(Stream stream, string? fileName = null)
    {
        var movie = new MovieData 
        { 
            SourceFormat = MovieFormat.Fm2,
            SystemType = SystemType.Nes
        };

        using var reader = new StreamReader(stream);
        string? line;
        bool inBinaryMode = false;

        while ((line = reader.ReadLine()) != null)
        {
            // Skip empty lines
            if (string.IsNullOrWhiteSpace(line)) continue;

            // Input lines start with |
            if (line.StartsWith("|"))
            {
                if (!inBinaryMode)
                {
                    ParseInputLine(movie, line);
                }
            }
            else
            {
                // Header key-value pair
                ParseHeaderLine(movie, line);
                
                if (line.StartsWith("binary ") && line.Contains("true"))
                {
                    inBinaryMode = true;
                    // Binary mode not fully implemented - would need binary parsing
                }
            }
        }

        return movie;
    }

    private void ParseHeaderLine(MovieData movie, string line)
    {
        int spaceIndex = line.IndexOf(' ');
        if (spaceIndex < 0) return;

        string key = line.Substring(0, spaceIndex).Trim();
        string value = line.Substring(spaceIndex + 1).Trim();

        switch (key.ToLower())
        {
            case "version":
                movie.SourceFormatVersion = value;
                break;
            case "emuversion":
                movie.EmulatorVersion = value;
                break;
            case "rerecordcount":
                if (uint.TryParse(value, out var rerecords))
                    movie.RerecordCount = rerecords;
                break;
            case "palflag":
                movie.Region = value == "1" ? RegionType.PAL : RegionType.NTSC;
                break;
            case "romfilename":
                movie.RomFileName = value;
                break;
            case "romchecksum":
                // FM2 uses base64 of hexified MD5 (weird format)
                movie.Md5Hash = value;
                break;
            case "comment":
                // Comments can have subjects: "comment author Name"
                if (value.StartsWith("author "))
                {
                    movie.Author = value.Substring(7).Trim();
                }
                else
                {
                    if (!string.IsNullOrEmpty(movie.Description))
                        movie.Description += "\n";
                    movie.Description += value;
                }
                break;
            case "subtitle":
                // Subtitles: "subtitle FRAME Text"
                // Could store these but for now just add to description
                break;
            case "guid":
                // Unique identifier
                break;
            case "fourscore":
                if (value == "1")
                    movie.ControllerCount = 4;
                break;
            case "port0":
            case "port1":
                int portNum = key[4] - '0';
                movie.PortTypes[portNum] = ParsePortType(value);
                break;
        }
    }

    private ControllerType ParsePortType(string value)
    {
        return value switch
        {
            "0" => ControllerType.None,         // SI_NONE
            "1" => ControllerType.Gamepad,      // SI_GAMEPAD
            "2" => ControllerType.SuperScope,   // SI_ZAPPER (close enough)
            _ => ControllerType.None
        };
    }

    private void ParseInputLine(MovieData movie, string line)
    {
        // Format: |commands|port0|port1|port2|
        // NES Gamepad: RLDUTSBA (Right, Left, Down, Up, sTart, Select, B, A)
        
        var parts = line.Split('|', StringSplitOptions.None);
        if (parts.Length < 2) return;

        var frame = new InputFrame { FrameNumber = movie.InputFrames.Count };

        // Parse commands (first field after initial |)
        if (parts.Length > 1 && !string.IsNullOrEmpty(parts[1]))
        {
            if (int.TryParse(parts[1], out var commands))
            {
                if ((commands & 1) != 0) frame.Command |= FrameCommand.SoftReset;
                if ((commands & 2) != 0) frame.Command |= FrameCommand.HardReset;
                if ((commands & 4) != 0) frame.Command |= FrameCommand.FdsInsert;
                if ((commands & 8) != 0) frame.Command |= FrameCommand.FdsSelect;
                if ((commands & 16) != 0) frame.Command |= FrameCommand.VsInsertCoin;
            }
        }

        // Parse controller inputs (NES format: RLDUTSBA)
        for (int i = 2; i < parts.Length && i - 2 < 4; i++)
        {
            string input = parts[i];
            if (string.IsNullOrEmpty(input) || input.Length < 8) continue;

            var ctrl = new ControllerInput { Type = ControllerType.Gamepad };
            
            // NES FM2 format: RLDUTSBA
            ctrl.Right = input[0] != '.' && input[0] != ' ';
            ctrl.Left = input[1] != '.' && input[1] != ' ';
            ctrl.Down = input[2] != '.' && input[2] != ' ';
            ctrl.Up = input[3] != '.' && input[3] != ' ';
            ctrl.Start = input[4] != '.' && input[4] != ' ';
            ctrl.Select = input[5] != '.' && input[5] != ' ';
            ctrl.B = input[6] != '.' && input[6] != ' ';
            ctrl.A = input[7] != '.' && input[7] != ' ';

            frame.Controllers[i - 2] = ctrl;
        }

        movie.InputFrames.Add(frame);
    }

    public override void Write(MovieData movie, Stream stream)
    {
        using var writer = new StreamWriter(stream, Encoding.UTF8, leaveOpen: true);

        // Write header
        writer.WriteLine("version 3");
        writer.WriteLine($"emuVersion {movie.EmulatorVersion ?? "29000"}");
        writer.WriteLine($"rerecordCount {movie.RerecordCount}");
        writer.WriteLine($"palFlag {(movie.Region == RegionType.PAL ? "1" : "0")}");
        writer.WriteLine($"romFilename {movie.RomFileName}");
        
        if (!string.IsNullOrEmpty(movie.Md5Hash))
            writer.WriteLine($"romChecksum {movie.Md5Hash}");

        writer.WriteLine($"guid {Guid.NewGuid():D}");

        // Port configuration
        bool fourscore = movie.ControllerCount > 2;
        if (fourscore)
        {
            writer.WriteLine("fourscore 1");
        }
        else
        {
            writer.WriteLine("port0 1"); // SI_GAMEPAD
            writer.WriteLine("port1 1");
        }
        writer.WriteLine("port2 0"); // SI_NONE for expansion

        // Author and description as comments
        if (!string.IsNullOrEmpty(movie.Author))
            writer.WriteLine($"comment author {movie.Author}");
        
        if (!string.IsNullOrEmpty(movie.Description))
        {
            foreach (var descLine in movie.Description.Split('\n'))
            {
                writer.WriteLine($"comment {descLine}");
            }
        }

        // Write input
        foreach (var frame in movie.InputFrames)
        {
            var sb = new StringBuilder("|");

            // Commands
            int commands = 0;
            if (frame.Command.HasFlag(FrameCommand.SoftReset)) commands |= 1;
            if (frame.Command.HasFlag(FrameCommand.HardReset)) commands |= 2;
            if (frame.Command.HasFlag(FrameCommand.FdsInsert)) commands |= 4;
            if (frame.Command.HasFlag(FrameCommand.FdsSelect)) commands |= 8;
            if (frame.Command.HasFlag(FrameCommand.VsInsertCoin)) commands |= 16;
            sb.Append(commands);

            // Controller inputs (NES format: RLDUTSBA)
            int controllerCount = fourscore ? 4 : 2;
            for (int i = 0; i < controllerCount; i++)
            {
                sb.Append('|');
                var ctrl = frame.Controllers[i];
                
                sb.Append(ctrl.Right ? 'R' : '.');
                sb.Append(ctrl.Left ? 'L' : '.');
                sb.Append(ctrl.Down ? 'D' : '.');
                sb.Append(ctrl.Up ? 'U' : '.');
                sb.Append(ctrl.Start ? 'T' : '.');
                sb.Append(ctrl.Select ? 'S' : '.');
                sb.Append(ctrl.B ? 'B' : '.');
                sb.Append(ctrl.A ? 'A' : '.');
            }

            // Port 2 (expansion, usually empty for NES)
            sb.Append('|');

            writer.WriteLine(sb.ToString());
        }
    }
}
