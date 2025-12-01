using System.IO.Compression;
using System.Text;

namespace Mesen.MovieConverter.Converters;

/// <summary>
/// Converter for Snes9x SMV movie format (.smv)
/// Supports versions 1.43, 1.51, and 1.52
/// </summary>
public class SmvMovieConverter : MovieConverterBase
{
    public override string[] Extensions => new[] { ".smv" };
    public override string FormatName => "Snes9x Movie";
    public override MovieFormat Format => MovieFormat.Smv;

    // SMV header offsets
    private const int SignatureOffset = 0;
    private const int VersionOffset = 4;
    private const int UidOffset = 8;
    private const int RerecordOffset = 12;
    private const int FrameCountOffset = 16;
    private const int ControllerMaskOffset = 20;
    private const int MovieOptionsOffset = 21;
    private const int SyncOptions1Offset = 22;
    private const int SyncOptions2Offset = 23;
    private const int SavestateOffsetOffset = 24;
    private const int ControllerDataOffsetOffset = 28;

    // v1.51+ header offsets (additional 32 bytes)
    private const int InputSampleCountOffset = 32;
    private const int Port1TypeOffset = 36;
    private const int Port2TypeOffset = 37;

    public override MovieData Read(Stream stream, string? fileName = null)
    {
        var movie = new MovieData { SourceFormat = MovieFormat.Smv };
        var data = ReadAllBytes(stream);

        // Verify signature "SMV\x1A"
        if (data.Length < 32 || data[0] != 'S' || data[1] != 'M' || data[2] != 'V' || data[3] != 0x1A)
        {
            throw new InvalidDataException("Invalid SMV file signature");
        }

        // Read version
        int version = BitConverter.ToInt32(data, VersionOffset);
        movie.SourceFormatVersion = version switch
        {
            1 => "1.43",
            4 => "1.51",
            5 => "1.52",
            _ => $"Unknown ({version})"
        };

        // Read metadata
        movie.RerecordCount = BitConverter.ToUInt32(data, RerecordOffset);
        int frameCount = BitConverter.ToInt32(data, FrameCountOffset);

        // Controller mask (which ports have controllers)
        byte controllerMask = data[ControllerMaskOffset];
        movie.ControllerCount = CountBits(controllerMask);

        // Movie options
        byte movieOptions = data[MovieOptionsOffset];
        bool startsFromSavestate = (movieOptions & 0x01) == 0; // Bit 0: 0=savestate, 1=SRAM/reset
        movie.Region = (movieOptions & 0x02) != 0 ? RegionType.PAL : RegionType.NTSC;

        // Sync options
        byte syncOptions = data[SyncOptions2Offset];
        bool hasRomInfo = (syncOptions & 0x40) != 0;

        // Offsets
        int savestateOffset = BitConverter.ToInt32(data, SavestateOffsetOffset);
        int controllerDataOffset = BitConverter.ToInt32(data, ControllerDataOffsetOffset);

        // Header size depends on version
        int headerSize = version >= 4 ? 64 : 32;

        // Read metadata (UTF-16 encoded title between header and savestate/ROM info)
        int metadataEnd = savestateOffset;
        if (hasRomInfo)
        {
            metadataEnd -= 30; // ROM info is 30 bytes before savestate
        }

        if (metadataEnd > headerSize)
        {
            try
            {
                var metadataBytes = new byte[metadataEnd - headerSize];
                Array.Copy(data, headerSize, metadataBytes, 0, metadataBytes.Length);
                movie.Author = Encoding.Unicode.GetString(metadataBytes).TrimEnd('\0');
            }
            catch
            {
                // Metadata parsing failed, ignore
            }
        }

        // Read ROM info if present
        if (hasRomInfo && savestateOffset >= 30)
        {
            int romInfoOffset = savestateOffset - 30;
            movie.Crc32 = BitConverter.ToUInt32(data, romInfoOffset + 3);
            
            var gameNameBytes = new byte[23];
            Array.Copy(data, romInfoOffset + 7, gameNameBytes, 0, 23);
            movie.GameName = Encoding.ASCII.GetString(gameNameBytes).TrimEnd('\0');
        }

        // Read savestate/SRAM if present
        if (savestateOffset > 0 && savestateOffset < controllerDataOffset)
        {
            int stateLength = controllerDataOffset - savestateOffset;
            if (stateLength > 0)
            {
                var compressedState = new byte[stateLength];
                Array.Copy(data, savestateOffset, compressedState, 0, stateLength);
                
                if (startsFromSavestate)
                {
                    // Decompress gzip savestate
                    try
                    {
                        movie.InitialState = DecompressGzip(compressedState);
                    }
                    catch
                    {
                        movie.InitialState = compressedState; // Store as-is if decompression fails
                    }
                }
                else
                {
                    // SRAM data
                    try
                    {
                        movie.InitialSram = DecompressGzip(compressedState);
                    }
                    catch
                    {
                        movie.InitialSram = compressedState;
                    }
                }
            }
        }

        // Determine bytes per frame based on controller count
        int bytesPerFrame = movie.ControllerCount * 2;

        // Read controller data
        int inputDataLength = data.Length - controllerDataOffset;
        int actualFrameCount = inputDataLength / bytesPerFrame;

        // SMV has frameCount + 1 frames
        for (int i = 0; i <= Math.Min(frameCount, actualFrameCount - 1); i++)
        {
            var frame = new InputFrame { FrameNumber = i };
            int offset = controllerDataOffset + (i * bytesPerFrame);

            int portIndex = 0;
            for (int port = 0; port < 5; port++)
            {
                if ((controllerMask & (1 << port)) != 0)
                {
                    if (offset + 1 < data.Length)
                    {
                        ushort inputValue = BitConverter.ToUInt16(data, offset);
                        
                        // Check for reset marker (0xFFFF)
                        if (inputValue == 0xFFFF)
                        {
                            frame.Command = FrameCommand.SoftReset;
                            frame.Controllers[portIndex] = new ControllerInput();
                        }
                        else
                        {
                            frame.Controllers[portIndex] = ControllerInput.FromSmvFormat(inputValue);
                        }
                        
                        offset += 2;
                    }
                    portIndex++;
                }
            }

            movie.InputFrames.Add(frame);
        }

        movie.SystemType = SystemType.Snes;
        return movie;
    }

    public override void Write(MovieData movie, Stream stream)
    {
        using var writer = new BinaryWriter(stream, Encoding.UTF8, leaveOpen: true);

        // Calculate offsets
        int headerSize = 32; // Using v1.43 format for compatibility
        byte[] metadataBytes = Encoding.Unicode.GetBytes(movie.Author + "\0");
        int metadataSize = metadataBytes.Length;
        
        int savestateOffset = headerSize + metadataSize;
        byte[]? stateData = null;

        if (movie.InitialState != null && movie.InitialState.Length > 0)
        {
            stateData = CompressGzip(movie.InitialState);
            savestateOffset = headerSize + metadataSize;
        }
        else if (movie.InitialSram != null && movie.InitialSram.Length > 0)
        {
            stateData = CompressGzip(movie.InitialSram);
        }

        int controllerDataOffset = savestateOffset + (stateData?.Length ?? 0);

        // Write header
        writer.Write((byte)'S');
        writer.Write((byte)'M');
        writer.Write((byte)'V');
        writer.Write((byte)0x1A);

        writer.Write(1); // Version 1 (1.43)
        writer.Write((int)DateTimeOffset.UtcNow.ToUnixTimeSeconds()); // UID
        writer.Write(movie.RerecordCount);
        writer.Write(movie.TotalFrames);

        // Controller mask
        byte controllerMask = 0;
        for (int i = 0; i < Math.Min(movie.ControllerCount, 5); i++)
        {
            controllerMask |= (byte)(1 << i);
        }
        writer.Write(controllerMask);

        // Movie options
        byte movieOptions = 0;
        if (!movie.StartsFromSavestate) movieOptions |= 0x01;
        if (movie.Region == RegionType.PAL) movieOptions |= 0x02;
        writer.Write(movieOptions);

        // Sync options
        writer.Write((byte)0); // sync options 1
        writer.Write((byte)0x01); // sync options 2 (MOVIE_SYNC_DATA_EXISTS)

        // Offsets
        writer.Write(savestateOffset);
        writer.Write(controllerDataOffset);

        // Write metadata
        writer.Write(metadataBytes);

        // Write savestate/SRAM
        if (stateData != null)
        {
            writer.Write(stateData);
        }

        // Write controller data
        foreach (var frame in movie.InputFrames)
        {
            for (int port = 0; port < 5; port++)
            {
                if ((controllerMask & (1 << port)) != 0)
                {
                    if (frame.Command == FrameCommand.SoftReset)
                    {
                        writer.Write((ushort)0xFFFF);
                    }
                    else
                    {
                        writer.Write(frame.Controllers[port].ToSmvFormat());
                    }
                }
            }
        }
    }

    private static int CountBits(byte value)
    {
        int count = 0;
        while (value != 0)
        {
            count += value & 1;
            value >>= 1;
        }
        return count;
    }

    private static byte[] DecompressGzip(byte[] data)
    {
        using var input = new MemoryStream(data);
        using var gzip = new GZipStream(input, CompressionMode.Decompress);
        using var output = new MemoryStream();
        gzip.CopyTo(output);
        return output.ToArray();
    }

    private static byte[] CompressGzip(byte[] data)
    {
        using var output = new MemoryStream();
        using (var gzip = new GZipStream(output, CompressionLevel.Optimal))
        {
            gzip.Write(data, 0, data.Length);
        }
        return output.ToArray();
    }
}
