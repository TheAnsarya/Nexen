using System.Buffers.Binary;
using System.Text;

namespace Nexen.MovieConverter.Converters;

/// <summary>
/// Converter for Gens GMV movie format.
///
/// GMV is a binary format for Sega Genesis/Mega Drive TAS movies.
/// Reference: https://tasvideos.org/EmulatorResources/Gens/GMV
///
/// Header structure (64 bytes):
/// - 0x00: Signature "Gens Movie " (16 bytes, space-padded)
/// - 0x10: Save/3-button flags (1 byte)
/// - 0x11: Controller config (1 byte)
/// - 0x12: Reserved (2 bytes)
/// - 0x14: Rerecord count (4 bytes, little endian)
/// - 0x18: Author (40 bytes, null-terminated)
/// - 0x40: Frame data starts here
///
/// Each frame is 3 bytes for 3-button, 6 bytes for 6-button controllers.
/// </summary>
public sealed class GmvMovieConverter : MovieConverterBase {
	/// <inheritdoc/>
	public override string[] Extensions => [".gmv"];

	/// <inheritdoc/>
	public override string FormatName => "Gens Movie";

	/// <inheritdoc/>
	public override MovieFormat Format => MovieFormat.Gmv;

	/// <inheritdoc/>
	public override bool CanWrite => false; // Read-only for now

	// GMV header constants
	private const int GmvHeaderSize = 64;
	private const int GmvSignatureLength = 16;
	private static readonly byte[] GmvSignature = "Gens Movie "u8.ToArray();

	// Flags
	private const byte FlagSavestate = 0x80;
	private const byte Flag3Button = 0x40;
	private const byte FlagPal = 0x20;

	/// <inheritdoc/>
	public override MovieData Read(Stream stream, string? fileName = null) {
		ArgumentNullException.ThrowIfNull(stream);

		var movie = new MovieData {
			SourceFormat = MovieFormat.Gmv,
			SystemType = SystemType.Genesis,
			ControllerCount = 2
		};

		// Read header
		Span<byte> header = stackalloc byte[GmvHeaderSize];
		stream.ReadExactly(header);

		// Verify signature
		if (!header[..GmvSignatureLength].SequenceEqual(GmvSignature)) {
			throw new MovieFormatException(Format, "Invalid GMV signature");
		}

		// Parse header
		ParseGmvHeader(header, movie, stream);

		return movie;
	}

	/// <summary>
	/// Parse GMV header and read input data
	/// </summary>
	private static void ParseGmvHeader(ReadOnlySpan<byte> header, MovieData movie, Stream stream) {
		// Flags at offset 0x10
		byte flags = header[0x10];

		// Start from savestate?
		if ((flags & FlagSavestate) != 0) {
			movie.StartType = StartType.Savestate;
		}

		// 3-button vs 6-button
		bool is3Button = (flags & Flag3Button) != 0;
		movie.ExtraData ??= [];
		movie.ExtraData["ControllerMode"] = is3Button ? "3-button" : "6-button";

		// PAL/NTSC
		if ((flags & FlagPal) != 0) {
			movie.Region = RegionType.PAL;
		} else {
			movie.Region = RegionType.NTSC;
		}

		// Controller configuration at offset 0x11
		byte controllerConfig = header[0x11];
		// Bit 0: Player 1 enabled
		// Bit 1: Player 2 enabled
		int controllerCount = 0;
		if ((controllerConfig & 0x01) != 0) {
			controllerCount++;
		}

		if ((controllerConfig & 0x02) != 0) {
			controllerCount++;
		}

		movie.ControllerCount = Math.Max(controllerCount, 1);

		// Set port types
		movie.PortTypes[0] = ControllerType.Gamepad;
		movie.PortTypes[1] = movie.ControllerCount > 1 ? ControllerType.Gamepad : ControllerType.None;
		for (int i = 2; i < movie.PortTypes.Length; i++) {
			movie.PortTypes[i] = ControllerType.None;
		}

		// Rerecord count at offset 0x14
		movie.RerecordCount = BinaryPrimitives.ReadUInt32LittleEndian(header[0x14..]);

		// Author at offset 0x18 (40 bytes)
		ReadOnlySpan<byte> authorBytes = header[0x18..0x40];
		movie.Author = ReadNullTerminatedString(authorBytes);

		// Source format version (GMV doesn't have explicit version, use "1")
		movie.SourceFormatVersion = "1";

		// Frame data starts at offset 0x40
		// Each frame: 3 bytes per controller for 3-button, 6 bytes for 6-button
		int bytesPerController = is3Button ? 3 : 3; // Both are 3 bytes in GMV
		int bytesPerFrame = bytesPerController * movie.ControllerCount;

		// Calculate frame count from remaining data
		long dataStart = GmvHeaderSize;
		long dataSize = stream.Length - dataStart;
		int frameCount = (int)(dataSize / bytesPerFrame);

		// Read frames
		Span<byte> frameBuffer = stackalloc byte[6]; // Max 2 controllers * 3 bytes

		for (int i = 0; i < frameCount; i++) {
			int read = stream.Read(frameBuffer[..bytesPerFrame]);
			if (read < bytesPerFrame) {
				break;
			}

			var frame = new InputFrame(i);

			// Parse each controller
			for (int c = 0; c < movie.ControllerCount && c < 2; c++) {
				int offset = c * bytesPerController;
				frame.Controllers[c] = ParseGmvController(frameBuffer.Slice(offset, bytesPerController), is3Button);
			}

			movie.InputFrames.Add(frame);
		}
	}

	/// <summary>
	/// Parse GMV controller data into ControllerInput
	/// </summary>
	/// <param name="data">Controller data (3 bytes)</param>
	/// <param name="is3Button">True for 3-button mode</param>
	/// <returns>Parsed controller input</returns>
	private static ControllerInput ParseGmvController(ReadOnlySpan<byte> data, bool is3Button) {
		var input = new ControllerInput();

		// GMV uses inverted bits (0 = pressed, 1 = released)
		// Byte 0: Up, Down, Left, Right, B, C, A, Start
		// Byte 1: Mode, X, Y, Z (6-button only, or always 0xFF for 3-button)
		// Byte 2: Reserved

		byte byte0 = data[0];
		byte byte1 = data.Length > 1 ? data[1] : (byte)0xff;

		// Standard buttons (byte 0, inverted)
		input.Up = (byte0 & 0x01) == 0;
		input.Down = (byte0 & 0x02) == 0;
		input.Left = (byte0 & 0x04) == 0;
		input.Right = (byte0 & 0x08) == 0;
		input.B = (byte0 & 0x10) == 0;
		input.C = (byte0 & 0x20) == 0;
		input.A = (byte0 & 0x40) == 0;
		input.Start = (byte0 & 0x80) == 0;

		if (!is3Button) {
			// 6-button mode: X, Y, Z, Mode (byte 1, inverted)
			input.Mode = (byte1 & 0x01) == 0;
			input.X = (byte1 & 0x02) == 0;
			input.Y = (byte1 & 0x04) == 0;
			input.Z = (byte1 & 0x08) == 0;
		}

		return input;
	}

	/// <summary>
	/// Read a null-terminated string from a byte span
	/// </summary>
	private static string ReadNullTerminatedString(ReadOnlySpan<byte> bytes) {
		int nullIndex = bytes.IndexOf((byte)0);
		if (nullIndex >= 0) {
			bytes = bytes[..nullIndex];
		}

		return Encoding.ASCII.GetString(bytes).Trim();
	}

	/// <inheritdoc/>
	public override void Write(MovieData movie, Stream stream) {
		throw new NotSupportedException("GMV writing is not yet implemented");
	}
}
