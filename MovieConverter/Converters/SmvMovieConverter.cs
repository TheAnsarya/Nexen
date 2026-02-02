using System.Buffers.Binary;
using System.Runtime.CompilerServices;
using System.Text;

namespace Nexen.MovieConverter.Converters;

/// <summary>
/// Converter for Snes9x SMV movie format.
///
/// SMV is a binary format for SNES TAS movies.
/// Reference: https://tasvideos.org/EmulatorResources/Snes9x/SMV
/// </summary>
public sealed class SmvMovieConverter : MovieConverterBase {
	/// <inheritdoc/>
	public override string[] Extensions => [".smv"];

	/// <inheritdoc/>
	public override string FormatName => "Snes9x Movie";

	/// <inheritdoc/>
	public override MovieFormat Format => MovieFormat.Smv;

	// SMV header constants
	private const uint SmvSignature = 0x564D532D; // "SMV-" in little endian
	private const int SmvHeaderSize = 32;

	/// <inheritdoc/>
	public override MovieData Read(Stream stream, string? fileName = null) {
		ArgumentNullException.ThrowIfNull(stream);

		var movie = new MovieData {
			SourceFormat = MovieFormat.Smv,
			SystemType = SystemType.Snes,
			ControllerCount = 2
		};

		// Read header (32 bytes minimum)
		Span<byte> header = stackalloc byte[SmvHeaderSize];
		stream.ReadExactly(header);

		// Verify signature
		uint sig = BinaryPrimitives.ReadUInt32LittleEndian(header);
		if (sig != SmvSignature) {
			throw new MovieFormatException(Format, $"Invalid SMV signature: 0x{sig:x8}");
		}

		// Parse header
		uint version = BinaryPrimitives.ReadUInt32LittleEndian(header[4..]);
		movie.SourceFormatVersion = version.ToString();

		// Depending on version, header layout differs
		if (version >= 4) {
			// SMV 1.43+ format
			ParseSmv143Header(header, movie, stream);
		} else {
			// Older SMV format
			ParseSmvLegacyHeader(header, movie, stream);
		}

		return movie;
	}

	/// <summary>
	/// Parse SMV 1.43+ header
	/// </summary>
	private static void ParseSmv143Header(ReadOnlySpan<byte> header, MovieData movie, Stream stream) {
		// Offset 8: Movie UID
		uint uid = BinaryPrimitives.ReadUInt32LittleEndian(header[8..]);
		movie.ExtraData ??= [];
		movie.ExtraData["UID"] = uid.ToString();

		// Offset 12: Rerecord count
		movie.RerecordCount = BinaryPrimitives.ReadUInt32LittleEndian(header[12..]);

		// Offset 16: Frame count
		uint frameCount = BinaryPrimitives.ReadUInt32LittleEndian(header[16..]);

		// Offset 20: Controller flags
		byte controllerFlags = header[20];
		movie.ControllerCount = CountSetBits(controllerFlags & 0x1f); // 5 controller ports max
		if (movie.ControllerCount == 0) {
			movie.ControllerCount = 1;
		}

		// Offset 21: Movie options flags
		byte movieFlags = header[21];
		if ((movieFlags & 0x01) != 0) {
			movie.StartType = StartType.Savestate;
		}

		if ((movieFlags & 0x02) != 0) {
			movie.Region = RegionType.PAL;
		}

		// Offset 22: Sync flags (for peripheral options)
		byte syncFlags = header[22];

		// Configure controller types based on flags
		if ((syncFlags & 0x01) != 0) {
			movie.PortTypes[1] = ControllerType.SuperScope;
		}

		if ((syncFlags & 0x02) != 0) {
			movie.PortTypes[1] = ControllerType.Justifier;
		}

		if ((syncFlags & 0x04) != 0) {
			movie.PortTypes[0] = ControllerType.SnesMultitap;
		}

		if ((syncFlags & 0x08) != 0) {
			movie.PortTypes[1] = ControllerType.SnesMultitap;
		}

		// Offset 23: Emulator sync options 2
		_ = header[23];

		// Offset 24: Save state offset
		uint savestateOffset = BinaryPrimitives.ReadUInt32LittleEndian(header[24..]);

		// Offset 28: Input data offset
		uint inputOffset = BinaryPrimitives.ReadUInt32LittleEndian(header[28..]);

		// Read metadata (between header and savestate/input)
		if (inputOffset > SmvHeaderSize) {
			int metadataSize = (int)(inputOffset - SmvHeaderSize);
			byte[] metadataBytes = new byte[metadataSize];
			stream.ReadExactly(metadataBytes);
			ParseSmvMetadata(metadataBytes, movie);
		}

		// Skip to savestate if present
		if (movie.StartType == StartType.Savestate && savestateOffset > 0 && savestateOffset < inputOffset) {
			stream.Position = savestateOffset;
			int ssSize = (int)(inputOffset - savestateOffset);
			movie.InitialState = new byte[ssSize];
			stream.ReadExactly(movie.InitialState);
		}

		// Read input data
		stream.Position = inputOffset;
		ReadSmvInputFrames(stream, movie, frameCount);
	}

	/// <summary>
	/// Parse older SMV header format
	/// </summary>
	private static void ParseSmvLegacyHeader(ReadOnlySpan<byte> header, MovieData movie, Stream stream) {
		// Legacy format has simpler header
		movie.RerecordCount = BinaryPrimitives.ReadUInt32LittleEndian(header[12..]);
		uint frameCount = BinaryPrimitives.ReadUInt32LittleEndian(header[16..]);

		byte flags = header[20];
		movie.ControllerCount = (flags & 0x01) != 0 ? 5 : 2;

		if ((flags & 0x02) != 0) {
			movie.StartType = StartType.Savestate;
		}

		if ((flags & 0x04) != 0) {
			movie.Region = RegionType.PAL;
		}

		// Input starts at offset 32
		ReadSmvInputFrames(stream, movie, frameCount);
	}

	/// <summary>
	/// Parse SMV metadata section
	/// </summary>
	private static void ParseSmvMetadata(byte[] data, MovieData movie) {
		// Metadata is null-terminated strings
		// First: ROM internal name
		int nullIdx = Array.IndexOf<byte>(data, 0);
		if (nullIdx > 0) {
			movie.GameName = Encoding.UTF8.GetString(data, 0, nullIdx);
		}

		// Additional metadata may follow
		// Author is sometimes in the metadata
		if (nullIdx >= 0 && nullIdx + 1 < data.Length) {
			int authorStart = nullIdx + 1;
			int authorEnd = Array.IndexOf<byte>(data, 0, authorStart);
			if (authorEnd > authorStart) {
				movie.Author = Encoding.UTF8.GetString(data, authorStart, authorEnd - authorStart);
			}
		}
	}

	/// <summary>
	/// Read SMV input frames
	/// </summary>
	private static void ReadSmvInputFrames(Stream stream, MovieData movie, uint frameCount) {
		// Each frame is 2 bytes per controller
		int bytesPerFrame = movie.ControllerCount * 2;
		byte[] frameBuffer = new byte[bytesPerFrame];

		for (uint i = 0; i < frameCount; i++) {
			if (stream.Read(frameBuffer, 0, bytesPerFrame) < bytesPerFrame) {
				break; // End of file
			}

			var frame = new InputFrame {
				FrameNumber = (int)i
			};

			for (int port = 0; port < movie.ControllerCount; port++) {
				ushort buttons = BinaryPrimitives.ReadUInt16LittleEndian(frameBuffer.AsSpan(port * 2));
				frame.Controllers[port] = ParseSmvButtons(buttons);
			}

			movie.InputFrames.Add(frame);
		}
	}

	/// <summary>
	/// Parse SMV button format to ControllerInput
	/// </summary>
	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	private static ControllerInput ParseSmvButtons(ushort buttons) {
		// SMV button layout (SNES):
		// Bit 0: R
		// Bit 1: L
		// Bit 2: X
		// Bit 3: A
		// Bit 4: Right
		// Bit 5: Left
		// Bit 6: Down
		// Bit 7: Up
		// Bit 8: Start
		// Bit 9: Select
		// Bit 10: Y
		// Bit 11: B
		return new ControllerInput {
			R = (buttons & 0x0001) != 0,
			L = (buttons & 0x0002) != 0,
			X = (buttons & 0x0004) != 0,
			A = (buttons & 0x0008) != 0,
			Right = (buttons & 0x0010) != 0,
			Left = (buttons & 0x0020) != 0,
			Down = (buttons & 0x0040) != 0,
			Up = (buttons & 0x0080) != 0,
			Start = (buttons & 0x0100) != 0,
			Select = (buttons & 0x0200) != 0,
			Y = (buttons & 0x0400) != 0,
			B = (buttons & 0x0800) != 0
		};
	}

	/// <summary>
	/// Convert ControllerInput to SMV button format
	/// </summary>
	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	private static ushort ToSmvButtons(ControllerInput input) {
		ushort buttons = 0;
		if (input.R) {
			buttons |= 0x0001;
		}

		if (input.L) {
			buttons |= 0x0002;
		}

		if (input.X) {
			buttons |= 0x0004;
		}

		if (input.A) {
			buttons |= 0x0008;
		}

		if (input.Right) {
			buttons |= 0x0010;
		}

		if (input.Left) {
			buttons |= 0x0020;
		}

		if (input.Down) {
			buttons |= 0x0040;
		}

		if (input.Up) {
			buttons |= 0x0080;
		}

		if (input.Start) {
			buttons |= 0x0100;
		}

		if (input.Select) {
			buttons |= 0x0200;
		}

		if (input.Y) {
			buttons |= 0x0400;
		}

		if (input.B) {
			buttons |= 0x0800;
		}

		return buttons;
	}

	/// <inheritdoc/>
	public override void Write(MovieData movie, Stream stream) {
		ArgumentNullException.ThrowIfNull(movie);
		ArgumentNullException.ThrowIfNull(stream);

		using var writer = new BinaryWriter(stream, Encoding.UTF8, leaveOpen: true);

		// Calculate offsets
		byte[] gameNameBytes = Encoding.UTF8.GetBytes(movie.GameName ?? "");
		byte[] authorBytes = Encoding.UTF8.GetBytes(movie.Author ?? "");
		int metadataSize = gameNameBytes.Length + 1 + authorBytes.Length + 1;

		uint savestateOffset = movie.InitialState is { Length: > 0 }
			? (uint)(SmvHeaderSize + metadataSize)
			: 0;

		uint inputOffset = savestateOffset > 0
			? (uint)(savestateOffset + movie.InitialState!.Length)
			: (uint)(SmvHeaderSize + metadataSize);

		// Write header
		Span<byte> header = stackalloc byte[SmvHeaderSize];

		BinaryPrimitives.WriteUInt32LittleEndian(header, SmvSignature); // SMV signature
		BinaryPrimitives.WriteUInt32LittleEndian(header[4..], 5); // Version (SMV 1.51)
		BinaryPrimitives.WriteUInt32LittleEndian(header[8..], (uint)Environment.TickCount); // UID
		BinaryPrimitives.WriteUInt32LittleEndian(header[12..], (uint)movie.RerecordCount);
		BinaryPrimitives.WriteUInt32LittleEndian(header[16..], (uint)movie.TotalFrames);

		// Controller flags (5 ports)
		byte controllerFlags = 0;
		for (int i = 0; i < Math.Min(movie.ControllerCount, 5); i++) {
			controllerFlags |= (byte)(1 << i);
		}

		header[20] = controllerFlags;

		// Movie flags
		byte movieFlags = 0;
		if (movie.StartType == StartType.Savestate) {
			movieFlags |= 0x01;
		}

		if (movie.Region == RegionType.PAL) {
			movieFlags |= 0x02;
		}

		header[21] = movieFlags;

		// Sync flags
		byte syncFlags = 0;
		if (movie.PortTypes[1] == ControllerType.SuperScope) {
			syncFlags |= 0x01;
		}

		if (movie.PortTypes[1] == ControllerType.Justifier) {
			syncFlags |= 0x02;
		}

		if (movie.PortTypes[0] == ControllerType.SnesMultitap) {
			syncFlags |= 0x04;
		}

		if (movie.PortTypes[1] == ControllerType.SnesMultitap) {
			syncFlags |= 0x08;
		}

		header[22] = syncFlags;

		header[23] = 0; // Sync flags 2

		BinaryPrimitives.WriteUInt32LittleEndian(header[24..], savestateOffset);
		BinaryPrimitives.WriteUInt32LittleEndian(header[28..], inputOffset);

		stream.Write(header);

		// Write metadata
		stream.Write(gameNameBytes);
		stream.WriteByte(0); // null terminator
		stream.Write(authorBytes);
		stream.WriteByte(0); // null terminator

		// Write savestate if present
		if (movie.InitialState is { Length: > 0 }) {
			stream.Write(movie.InitialState);
		}

		// Write input frames
		Span<byte> frameBuffer = stackalloc byte[10]; // Max 5 controllers * 2 bytes

		foreach (InputFrame frame in movie.InputFrames) {
			for (int port = 0; port < movie.ControllerCount; port++) {
				ushort buttons = ToSmvButtons(frame.Controllers[port]);
				BinaryPrimitives.WriteUInt16LittleEndian(frameBuffer[(port * 2)..], buttons);
			}

			stream.Write(frameBuffer[..(movie.ControllerCount * 2)]);
		}
	}

	/// <summary>
	/// Count set bits in a byte
	/// </summary>
	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	private static int CountSetBits(int value) {
		int count = 0;
		while (value != 0) {
			count += value & 1;
			value >>= 1;
		}

		return count;
	}
}
