using System.IO.Compression;
using System.Text;

namespace Mesen.MovieConverter.Converters;

/// <summary>
/// Converter for Mesen2 native movie format (.mmo)
/// </summary>
public class MesenMovieConverter : MovieConverterBase
{
    public override string[] Extensions => new[] { ".mmo" };
    public override string FormatName => "Mesen Movie";
    public override MovieFormat Format => MovieFormat.Mesen;

    public override MovieData Read(Stream stream, string? fileName = null)
    {
        var movie = new MovieData { SourceFormat = MovieFormat.Mesen };
        
        using var archive = new ZipArchive(stream, ZipArchiveMode.Read, leaveOpen: true);

        // Read GameSettings.txt
        var settingsEntry = archive.GetEntry("GameSettings.txt");
        if (settingsEntry != null)
        {
            using var reader = new StreamReader(settingsEntry.Open());
            ParseSettings(movie, reader.ReadToEnd());
        }

        // Read MovieInfo.txt
        var infoEntry = archive.GetEntry("MovieInfo.txt");
        if (infoEntry != null)
        {
            using var reader = new StreamReader(infoEntry.Open());
            ParseMovieInfo(movie, reader.ReadToEnd());
        }

        // Read Input.txt
        var inputEntry = archive.GetEntry("Input.txt");
        if (inputEntry != null)
        {
            using var reader = new StreamReader(inputEntry.Open());
            ParseInput(movie, reader.ReadToEnd());
        }

        // Read SaveState.mss if present
        var stateEntry = archive.GetEntry("SaveState.mss");
        if (stateEntry != null)
        {
            using var ms = new MemoryStream();
            stateEntry.Open().CopyTo(ms);
            movie.InitialState = ms.ToArray();
        }

        return movie;
    }

    private void ParseSettings(MovieData movie, string content)
    {
        var lines = content.Split('\n', StringSplitOptions.RemoveEmptyEntries);
        foreach (var line in lines)
        {
            var parts = line.Split(' ', 2);
            if (parts.Length < 2) continue;

            var key = parts[0].Trim();
            var value = parts[1].Trim();

            switch (key)
            {
                case "MesenVersion":
                    movie.EmulatorVersion = value;
                    break;
                case "GameFile":
                    movie.RomFileName = value;
                    break;
                case "SHA1":
                    movie.Sha1Hash = value;
                    break;
                case "emu.consoleType":
                    movie.SystemType = ParseConsoleType(value);
                    break;
            }
        }
    }

    private SystemType ParseConsoleType(string value)
    {
        return value.ToLower() switch
        {
            "snes" => SystemType.Snes,
            "nes" => SystemType.Nes,
            "gameboy" or "gb" => SystemType.Gb,
            "gameboycolor" or "gbc" => SystemType.Gbc,
            "gba" => SystemType.Gba,
            "sms" or "mastersystem" => SystemType.Sms,
            "genesis" => SystemType.Genesis,
            "pce" or "pcengine" => SystemType.Pce,
            _ => SystemType.Other
        };
    }

    private void ParseMovieInfo(MovieData movie, string content)
    {
        var lines = content.Split('\n');
        bool inDescription = false;
        var descriptionBuilder = new StringBuilder();

        foreach (var line in lines)
        {
            if (inDescription)
            {
                descriptionBuilder.AppendLine(line);
            }
            else if (line.StartsWith("Author "))
            {
                movie.Author = line.Substring(7).Trim();
            }
            else if (line.StartsWith("Description"))
            {
                inDescription = true;
            }
        }

        movie.Description = descriptionBuilder.ToString().Trim();
    }

    private void ParseInput(MovieData movie, string content)
    {
        var lines = content.Split('\n', StringSplitOptions.RemoveEmptyEntries);
        int frameNum = 0;

        foreach (var line in lines)
        {
            if (!line.StartsWith("|")) continue;

            var parts = line.Split('|', StringSplitOptions.RemoveEmptyEntries);
            var frame = new InputFrame { FrameNumber = frameNum++ };

            for (int i = 0; i < Math.Min(parts.Length, frame.Controllers.Length); i++)
            {
                frame.Controllers[i] = ControllerInput.FromMesenFormat(parts[i]);
            }

            movie.InputFrames.Add(frame);
        }
    }

    public override void Write(MovieData movie, Stream stream)
    {
        using var archive = new ZipArchive(stream, ZipArchiveMode.Create, leaveOpen: true);

        // Write GameSettings.txt
        var settingsEntry = archive.CreateEntry("GameSettings.txt");
        using (var writer = new StreamWriter(settingsEntry.Open()))
        {
            writer.WriteLine($"MesenVersion {movie.EmulatorVersion ?? "2.1.0"}");
            writer.WriteLine($"MovieFormatVersion 2");
            writer.WriteLine($"GameFile {movie.RomFileName}");
            if (!string.IsNullOrEmpty(movie.Sha1Hash))
                writer.WriteLine($"SHA1 {movie.Sha1Hash}");
            
            // Console type
            var consoleType = movie.SystemType switch
            {
                SystemType.Snes => "Snes",
                SystemType.Nes => "Nes",
                SystemType.Gb => "Gameboy",
                SystemType.Gbc => "GameboyColor",
                SystemType.Gba => "Gba",
                _ => "Snes"
            };
            writer.WriteLine($"emu.consoleType {consoleType}");
        }

        // Write MovieInfo.txt
        if (!string.IsNullOrEmpty(movie.Author) || !string.IsNullOrEmpty(movie.Description))
        {
            var infoEntry = archive.CreateEntry("MovieInfo.txt");
            using var writer = new StreamWriter(infoEntry.Open());
            writer.WriteLine($"Author {movie.Author}");
            writer.WriteLine("Description");
            writer.WriteLine(movie.Description);
        }

        // Write Input.txt
        var inputEntry = archive.CreateEntry("Input.txt");
        using (var writer = new StreamWriter(inputEntry.Open()))
        {
            foreach (var frame in movie.InputFrames)
            {
                var parts = new List<string>();
                for (int i = 0; i < movie.ControllerCount; i++)
                {
                    parts.Add(frame.Controllers[i].ToMesenFormat());
                }
                writer.WriteLine("|" + string.Join("|", parts));
            }
        }

        // Write SaveState.mss if present
        if (movie.InitialState != null && movie.InitialState.Length > 0)
        {
            var stateEntry = archive.CreateEntry("SaveState.mss");
            using var stateStream = stateEntry.Open();
            stateStream.Write(movie.InitialState, 0, movie.InitialState.Length);
        }
    }
}
