using System.IO.Compression;
using System.Text;

namespace Mesen.MovieConverter.Converters;

/// <summary>
/// Converter for lsnes LSMV movie format (.lsmv)
/// Supports both text and binary formats (ZIP archive)
/// </summary>
public class LsmvMovieConverter : MovieConverterBase
{
    public override string[] Extensions => new[] { ".lsmv" };
    public override string FormatName => "lsnes Movie";
    public override MovieFormat Format => MovieFormat.Lsmv;

    public override MovieData Read(Stream stream, string? fileName = null)
    {
        var movie = new MovieData { SourceFormat = MovieFormat.Lsmv };

        using var archive = new ZipArchive(stream, ZipArchiveMode.Read, leaveOpen: true);

        // Read systemid (required)
        var systemIdEntry = archive.GetEntry("systemid");
        if (systemIdEntry != null)
        {
            using var reader = new StreamReader(systemIdEntry.Open());
            var systemId = reader.ReadToEnd().Trim();
            if (systemId != "lsnes-rr1")
            {
                throw new InvalidDataException($"Invalid LSMV system ID: {systemId}");
            }
            movie.SourceFormatVersion = systemId;
        }

        // Read gametype
        var gametypeEntry = archive.GetEntry("gametype");
        if (gametypeEntry != null)
        {
            using var reader = new StreamReader(gametypeEntry.Open());
            var gametype = reader.ReadToEnd().Trim();
            movie.SystemType = ParseGameType(gametype, out var region);
            movie.Region = region;
        }

        // Read gamename
        var gamenameEntry = archive.GetEntry("gamename");
        if (gamenameEntry != null)
        {
            using var reader = new StreamReader(gamenameEntry.Open());
            movie.GameName = reader.ReadToEnd().Trim();
        }

        // Read authors
        var authorsEntry = archive.GetEntry("authors");
        if (authorsEntry != null)
        {
            using var reader = new StreamReader(authorsEntry.Open());
            var lines = reader.ReadToEnd().Split('\n', StringSplitOptions.RemoveEmptyEntries);
            var authors = new List<string>();
            foreach (var line in lines)
            {
                // Format: "realname|nickname" or just text
                var parts = line.Split('|');
                if (parts.Length >= 2 && !string.IsNullOrWhiteSpace(parts[1]))
                    authors.Add(parts[1].Trim());
                else if (!string.IsNullOrWhiteSpace(parts[0]))
                    authors.Add(parts[0].Trim());
            }
            movie.Author = string.Join(", ", authors);
        }

        // Read ROM hashes
        foreach (var entry in archive.Entries)
        {
            if (entry.Name.EndsWith(".sha256"))
            {
                using var reader = new StreamReader(entry.Open());
                movie.Sha256Hash = reader.ReadToEnd().Trim();
                break;
            }
        }

        // Read rerecords
        var rerecordsEntry = archive.GetEntry("rerecords");
        if (rerecordsEntry != null)
        {
            using var reader = new StreamReader(rerecordsEntry.Open());
            if (uint.TryParse(reader.ReadToEnd().Trim(), out var rerecords))
            {
                movie.RerecordCount = rerecords;
            }
        }

        // Read savestate anchor if present
        var anchorEntry = archive.GetEntry("savestate.anchor");
        if (anchorEntry != null)
        {
            using var ms = new MemoryStream();
            anchorEntry.Open().CopyTo(ms);
            var data = ms.ToArray();
            // Last 32 bytes are SHA-256 checksum
            if (data.Length > 32)
            {
                movie.InitialState = new byte[data.Length - 32];
                Array.Copy(data, movie.InitialState, data.Length - 32);
            }
        }

        // Read SRAM files
        foreach (var entry in archive.Entries)
        {
            if (entry.Name.StartsWith("moviesram."))
            {
                using var ms = new MemoryStream();
                entry.Open().CopyTo(ms);
                movie.InitialSram = ms.ToArray();
                break; // Just use first SRAM for now
            }
        }

        // Read port configurations
        for (int i = 1; i <= 2; i++)
        {
            var portEntry = archive.GetEntry($"port{i}");
            if (portEntry != null)
            {
                using var reader = new StreamReader(portEntry.Open());
                var portType = reader.ReadToEnd().Trim().ToLower();
                movie.PortTypes[i - 1] = portType switch
                {
                    "gamepad" => ControllerType.Gamepad,
                    "mouse" => ControllerType.Mouse,
                    "superscope" => ControllerType.SuperScope,
                    "justifier" or "justifiers" => ControllerType.Justifier,
                    "multitap" => ControllerType.Multitap,
                    _ => ControllerType.None
                };
            }
        }

        // Count active controllers
        movie.ControllerCount = movie.PortTypes.Count(t => t != ControllerType.None);
        if (movie.ControllerCount == 0) movie.ControllerCount = 2; // Default

        // Read input
        var inputEntry = archive.GetEntry("input");
        if (inputEntry != null)
        {
            using var reader = new StreamReader(inputEntry.Open());
            ParseInput(movie, reader.ReadToEnd());
        }

        return movie;
    }

    private SystemType ParseGameType(string gametype, out RegionType region)
    {
        region = RegionType.NTSC;
        
        return gametype.ToLower() switch
        {
            "snes_ntsc" => SystemType.Snes,
            "snes_pal" => SetRegion(SystemType.Snes, RegionType.PAL, out region),
            "sgb_ntsc" => SystemType.Gb,
            "sgb_pal" => SetRegion(SystemType.Gb, RegionType.PAL, out region),
            "gdmg" => SystemType.Gb,
            "ggbc" or "ggbca" => SystemType.Gbc,
            "bsx" or "bsxslotted" => SystemType.Snes,
            "sufamiturbo" => SystemType.Snes,
            _ => SystemType.Snes
        };
    }

    private static SystemType SetRegion(SystemType type, RegionType newRegion, out RegionType region)
    {
        region = newRegion;
        return type;
    }

    private void ParseInput(MovieData movie, string content)
    {
        var lines = content.Split('\n');
        int frameNum = 0;
        InputFrame? currentFrame = null;

        foreach (var rawLine in lines)
        {
            var line = rawLine.Trim();
            if (string.IsNullOrEmpty(line)) continue;

            // Check if this starts a new frame
            bool isNewFrame = !line.StartsWith("\t") && !line.StartsWith(" ") && 
                             !line.StartsWith(".") && !line.StartsWith("|");
            
            if (isNewFrame && currentFrame != null)
            {
                movie.InputFrames.Add(currentFrame);
            }

            if (isNewFrame || currentFrame == null)
            {
                currentFrame = new InputFrame { FrameNumber = frameNum++ };
            }

            // Parse input line: F.|BYsSudlrAXLR or FR X Y|BYsSudlrAXLR (reset)
            // First character indicates frame (F) or subframe (.)
            // Second character: '.' = normal, 'R' = reset
            
            int pipeIndex = line.IndexOf('|');
            if (pipeIndex < 0) continue;

            string prefix = line.Substring(0, pipeIndex);
            string inputPart = line.Substring(pipeIndex + 1);

            // Check for reset
            if (prefix.Length >= 2 && prefix[1] != '.' && prefix[1] != ' ')
            {
                // Reset frame
                if (prefix.Contains(' '))
                {
                    // Delayed reset: FR X Y|...
                    currentFrame.Command = FrameCommand.SoftReset;
                }
                else
                {
                    currentFrame.Command = FrameCommand.SoftReset;
                }
            }

            // Parse controller input (up to 2 controllers for now)
            // Format: BYsSudlrAXLR
            if (inputPart.Length >= 12)
            {
                currentFrame.Controllers[0] = ControllerInput.FromLsmvFormat(inputPart.Substring(0, 12));
                
                // Check for second controller (separated by pipe or directly after)
                if (inputPart.Length >= 24)
                {
                    currentFrame.Controllers[1] = ControllerInput.FromLsmvFormat(inputPart.Substring(12, 12));
                }
                else if (inputPart.Contains('|'))
                {
                    var parts = inputPart.Split('|');
                    if (parts.Length >= 2 && parts[1].Length >= 12)
                    {
                        currentFrame.Controllers[1] = ControllerInput.FromLsmvFormat(parts[1].Substring(0, 12));
                    }
                }
            }
        }

        // Add last frame
        if (currentFrame != null)
        {
            movie.InputFrames.Add(currentFrame);
        }
    }

    public override void Write(MovieData movie, Stream stream)
    {
        using var archive = new ZipArchive(stream, ZipArchiveMode.Create, leaveOpen: true);

        // Write systemid
        WriteTextEntry(archive, "systemid", "lsnes-rr1");

        // Write controlsversion
        WriteTextEntry(archive, "controlsversion", "0");

        // Write gametype
        string gametype = movie.SystemType switch
        {
            SystemType.Snes => movie.Region == RegionType.PAL ? "snes_pal" : "snes_ntsc",
            SystemType.Gb => movie.Region == RegionType.PAL ? "sgb_pal" : "sgb_ntsc",
            SystemType.Gbc => "ggbc",
            _ => "snes_ntsc"
        };
        WriteTextEntry(archive, "gametype", gametype);

        // Write gamename
        if (!string.IsNullOrEmpty(movie.GameName))
        {
            WriteTextEntry(archive, "gamename", movie.GameName);
        }

        // Write authors
        if (!string.IsNullOrEmpty(movie.Author))
        {
            WriteTextEntry(archive, "authors", $"|{movie.Author}");
        }

        // Write projectid (generate a random one)
        WriteTextEntry(archive, "projectid", Guid.NewGuid().ToString("N"));

        // Write ROM hash
        if (!string.IsNullOrEmpty(movie.Sha256Hash))
        {
            WriteTextEntry(archive, "rom.sha256", movie.Sha256Hash);
        }
        else if (!string.IsNullOrEmpty(movie.Sha1Hash))
        {
            // Convert SHA1 to note (LSMV prefers SHA256)
            WriteTextEntry(archive, "rom.sha1", movie.Sha1Hash);
        }

        // Write rerecords
        WriteTextEntry(archive, "rerecords", movie.RerecordCount.ToString());

        // Write port configurations
        WriteTextEntry(archive, "port1", "gamepad");
        if (movie.ControllerCount >= 2)
        {
            WriteTextEntry(archive, "port2", "gamepad");
        }

        // Write savestate anchor if present
        if (movie.InitialState != null && movie.InitialState.Length > 0)
        {
            var anchorEntry = archive.CreateEntry("savestate.anchor");
            using var anchorStream = anchorEntry.Open();
            anchorStream.Write(movie.InitialState, 0, movie.InitialState.Length);
            
            // Add SHA-256 checksum (32 zero bytes as placeholder)
            var checksum = new byte[32];
            anchorStream.Write(checksum, 0, 32);
        }

        // Write SRAM if present
        if (movie.InitialSram != null && movie.InitialSram.Length > 0)
        {
            var sramEntry = archive.CreateEntry("moviesram.srm");
            using var sramStream = sramEntry.Open();
            sramStream.Write(movie.InitialSram, 0, movie.InitialSram.Length);
        }

        // Write input
        var inputEntry = archive.CreateEntry("input");
        using (var writer = new StreamWriter(inputEntry.Open()))
        {
            foreach (var frame in movie.InputFrames)
            {
                // Frame marker
                char frameChar = 'F';
                char resetChar = frame.Command.HasFlag(FrameCommand.SoftReset) ? 'R' : '.';
                
                // Build input string
                var sb = new StringBuilder();
                sb.Append(frameChar);
                sb.Append(resetChar);
                sb.Append('|');
                
                // Controller 1
                sb.Append(frame.Controllers[0].ToLsmvFormat());
                
                // Controller 2 if present
                if (movie.ControllerCount >= 2)
                {
                    sb.Append(frame.Controllers[1].ToLsmvFormat());
                }
                
                writer.WriteLine(sb.ToString());
            }
        }
    }

    private void WriteTextEntry(ZipArchive archive, string name, string content)
    {
        var entry = archive.CreateEntry(name);
        using var writer = new StreamWriter(entry.Open());
        writer.Write(content);
    }
}
