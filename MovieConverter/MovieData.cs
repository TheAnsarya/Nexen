namespace Mesen.MovieConverter;

/// <summary>
/// Common movie data structure that can be converted between formats
/// </summary>
public class MovieData
{
    // Metadata
    public string Author { get; set; } = "";
    public string Description { get; set; } = "";
    public string GameName { get; set; } = "";
    public string RomFileName { get; set; } = "";
    
    // ROM identification
    public string? Sha1Hash { get; set; }
    public string? Sha256Hash { get; set; }
    public string? Md5Hash { get; set; }
    public uint? Crc32 { get; set; }

    // Timing
    public SystemType SystemType { get; set; } = SystemType.Snes;
    public RegionType Region { get; set; } = RegionType.NTSC;
    public double FrameRate => Region == RegionType.PAL ? 50.0069808566 : 60.098813897;

    // Movie statistics
    public uint RerecordCount { get; set; }
    public uint LagFrameCount { get; set; }
    public int TotalFrames => InputFrames.Count;
    public TimeSpan Duration => TimeSpan.FromSeconds(TotalFrames / FrameRate);

    // Controller configuration
    public int ControllerCount { get; set; } = 2;
    public ControllerType[] PortTypes { get; set; } = new ControllerType[8];

    // Movie data
    public List<InputFrame> InputFrames { get; set; } = new();

    // Save state/SRAM data
    public byte[]? InitialState { get; set; }
    public byte[]? InitialSram { get; set; }
    public bool StartsFromSavestate => InitialState != null && InitialState.Length > 0;
    public bool StartsFromSram => InitialSram != null && InitialSram.Length > 0;

    // Source format info
    public MovieFormat SourceFormat { get; set; }
    public string? SourceFormatVersion { get; set; }
    public string? EmulatorVersion { get; set; }

    public MovieData()
    {
        for (int i = 0; i < PortTypes.Length; i++)
        {
            PortTypes[i] = i < 2 ? ControllerType.Gamepad : ControllerType.None;
        }
    }

    /// <summary>
    /// Get a summary of the movie
    /// </summary>
    public string GetSummary()
    {
        return $"""
            Movie Summary:
            ==============
            Source Format: {SourceFormat} {SourceFormatVersion}
            System: {SystemType} ({Region})
            Game: {GameName}
            ROM: {RomFileName}
            Author: {Author}
            
            Statistics:
            - Total Frames: {TotalFrames:N0}
            - Duration: {Duration:hh\:mm\:ss\.fff}
            - Rerecords: {RerecordCount:N0}
            - Lag Frames: {LagFrameCount:N0}
            
            Start State:
            - From Savestate: {StartsFromSavestate}
            - From SRAM: {StartsFromSram}
            - Clean Start: {!StartsFromSavestate && !StartsFromSram}
            
            Controllers: {ControllerCount}
            """;
    }
}

public enum MovieFormat
{
    Unknown,
    Mesen,      // .mmm - Mesen2 native format
    Smv,        // .smv - Snes9x
    Lsmv,       // .lsmv - lsnes
    Fm2,        // .fm2 - FCEUX (NES)
    Bk2,        // .bk2 - BizHawk
    Vbm,        // .vbm - VisualBoyAdvance
    Gmv,        // .gmv - Gens (Genesis)
    Mcm         // .mcm - Mednafen
}

public enum SystemType
{
    Nes,
    Snes,
    Gb,
    Gbc,
    Gba,
    Sms,
    Genesis,
    Pce,
    Other
}

public enum RegionType
{
    NTSC,
    PAL
}
