using System.Buffers.Binary;
using System.Text;

namespace Nexen.MovieConverter.Converters;

/// <summary>
/// Converter for VisualBoyAdvance VBM movie format.
///
/// VBM is a binary format for GBA/GB/GBC TAS movies.
/// Reference: https://tasvideos.org/EmulatorResources/VBA/VBM
///
/// Header structure (64 bytes):
/// - 0x00: Signature "VBM\x1a" (4 bytes)
/// - 0x04: Version (4 bytes, little endian)
/// - 0x08: UID (4 bytes)
/// - 0x0C: Frame count (4 bytes)
/// - 0x10: Rerecord count (4 bytes)
/// - 0x14: Start flags (1 byte)
/// - 0x15: Controller flags (1 byte)
/// - 0x16: System flags (1 byte)
/// - 0x17: Reserved (1 byte)
/// - 0x18: Save state offset (4 bytes)
/// - 0x1C: Controller data offset (4 bytes)
/// - 0x20: Author info (64 bytes, null-terminated)
/// - 0x60: ROM title (12 bytes, internal name)
/// - 0x6C: Minor version (1 byte)
/// - 0x6D: ROM CRC (1 byte)
/// - 0x6E: ROM checksum (2 bytes)
/// - 0x70: BIOS flags (4 bytes)
/// - 0x74: ROM serial (12 bytes)
/// - 0x80: Controller data starts here (or at offset from header)
/// </summary>
public sealed class VbmMovieConverter : MovieConverterBase {
	/// <inheritdoc/>
	public override string[] Extensions => [".vbm"];

	/// <inheritdoc/>
	public override string FormatName => "VisualBoyAdvance Movie";

	/// <inheritdoc/>
	public override MovieFormat Format => MovieFormat.Vbm;

	/// <inheritdoc/>
	public override bool CanWrite => false; // Read-only for now

	// VBM header constants
	private const uint VbmSignature = 0x1a4d4256; // "VBM\x1a" in little endian
	private const int VbmHeaderSize = 64;
	private const int VbmExtendedHeaderSize = 128;

	// Start flags
	private const byte StartFromSavestate = 0x01;
	private const byte StartFromSram = 0x02;
	private const byte StartFromBios = 0x04;

	// System flags
	private const byte SystemGba = 0x01;
	private const byte SystemGbc = 0x02;
	private const byte SystemSgb = 0x04;

	/// <inheritdoc/>
	public override MovieData Read(Stream stream, string? fileName = null) {
		ArgumentNullException.ThrowIfNull(stream);

		var movie = new MovieData {
			SourceFormat = MovieFormat.Vbm,
			ControllerCount = 1 // GBA/GB typically 1 controller
		};

		// Read header
		Span<byte> header = stackalloc byte[VbmExtendedHeaderSize];
		int bytesRead = stream.Read(header);
		if (bytesRead < VbmHeaderSize) {
			throw new MovieFormatException(Format, "File too small for VBM header");
		}

		// Verify signature
		uint sig = BinaryPrimitives.ReadUInt32LittleEndian(header);
		if (sig != VbmSignature) {
			throw new MovieFormatException(Format, $"Invalid VBM signature: 0x{sig:x8}");
		}

		// Parse header
		ParseVbmHeader(header[..bytesRead], movie, stream);

		return movie;
	}

	/// <summary>
	/// Parse VBM header and read input data
	/// </summary>
	private static void ParseVbmHeader(ReadOnlySpan<byte> header, MovieData movie, Stream stream) {
		// Version
		uint version = BinaryPrimitives.ReadUInt32LittleEndian(header[4..]);
		movie.SourceFormatVersion = version.ToString();

		// UID
		uint uid = BinaryPrimitives.ReadUInt32LittleEndian(header[8..]);
		movie.ExtraData ??= [];
		movie.ExtraData["UID"] = uid.ToString();

		// Frame count
		uint frameCount = BinaryPrimitives.ReadUInt32LittleEndian(header[12..]);

		// Rerecord count
		movie.RerecordCount = BinaryPrimitives.ReadUInt32LittleEndian(header[16..]);

		// Start flags
		byte startFlags = header[20];
		if ((startFlags & StartFromSavestate) != 0) {
			movie.StartType = StartType.Savestate;
		} else if ((startFlags & StartFromSram) != 0) {
			movie.StartType = StartType.Sram;
		} else if ((startFlags & StartFromBios) != 0) {
			movie.StartType = StartType.Bios;
		} else {
			movie.StartType = StartType.PowerOn;
		}

		// Controller flags
		byte controllerFlags = header[21];
		movie.ControllerCount = CountSetBits(controllerFlags & 0x0f);
		if (movie.ControllerCount == 0) {
			movie.ControllerCount = 1;
		}

		// System flags
		byte systemFlags = header[22];
		if ((systemFlags & SystemGba) != 0) {
			movie.SystemType = SystemType.Gba;
		} else if ((systemFlags & SystemGbc) != 0) {
			movie.SystemType = SystemType.Gbc;
		} else if ((systemFlags & SystemSgb) != 0) {
			movie.SystemType = SystemType.Gb; // SGB uses GB
		} else {
			movie.SystemType = SystemType.Gb;
		}

		// Set port types based on system
		movie.PortTypes[0] = ControllerType.Gamepad;
		for (int i = 1; i < movie.PortTypes.Length; i++) {
			movie.PortTypes[i] = i < movie.ControllerCount ? ControllerType.Gamepad : ControllerType.None;
		}

		// Savestate offset (if starting from savestate)
		uint savestateOffset = BinaryPrimitives.ReadUInt32LittleEndian(header[24..]);

		// Controller data offset
		uint controllerDataOffset = BinaryPrimitives.ReadUInt32LittleEndian(header[28..]);

		// Author info (64 bytes at offset 0x20)
		if (header.Length >= 0x60) {
			ReadOnlySpan<byte> authorBytes = header[0x20..0x60];
			movie.Author = ReadNullTerminatedString(authorBytes);
		}

		// ROM title (12 bytes at offset 0x60)
		if (header.Length >= 0x6C) {
			ReadOnlySpan<byte> romTitleBytes = header[0x60..0x6C];
			movie.InternalRomName = ReadNullTerminatedString(romTitleBytes);
			if (!string.IsNullOrEmpty(movie.InternalRomName)) {
				movie.GameName = movie.InternalRomName;
			}
		}

		// ROM CRC (1 byte at 0x6D) and checksum (2 bytes at 0x6E)
		if (header.Length >= 0x70) {
			byte romCrc = header[0x6D];
			ushort romChecksum = BinaryPrimitives.ReadUInt16LittleEndian(header[0x6E..]);
			movie.ExtraData["RomCRC"] = romCrc.ToString("x2");
			movie.ExtraData["RomChecksum"] = romChecksum.ToString("x4");
		}

		// ROM serial (12 bytes at 0x74)
		if (header.Length >= 0x80) {
			ReadOnlySpan<byte> serialBytes = header[0x74..0x80];
			string serial = ReadNullTerminatedString(serialBytes);
			if (!string.IsNullOrEmpty(serial)) {
				movie.ExtraData["RomSerial"] = serial;
			}
		}

		// Read savestate if present
		if (movie.StartType == StartType.Savestate && savestateOffset > 0 && savestateOffset < controllerDataOffset) {
			stream.Position = savestateOffset;
			int savestateSize = (int)(controllerDataOffset - savestateOffset);
			movie.InitialState = new byte[savestateSize];
			stream.ReadExactly(movie.InitialState);
		}

		// Seek to controller data
		// For older VBM versions, controllerDataOffset may be invalid (larger than file)
		// In that case, input data starts at offset 0x40 (after 64-byte header)
		if (controllerDataOffset > 0 && controllerDataOffset < (uint)stream.Length) {
			stream.Position = controllerDataOffset;
		} else {
			// Fall back to reading immediately after header
			stream.Position = VbmHeaderSize;
		}

		// Read input frames
		// VBM uses 2 bytes per controller per frame for GB/GBC, or more for GBA
		int bytesPerFrame = movie.SystemType == SystemType.Gba ? 2 * movie.ControllerCount : 2 * movie.ControllerCount;

		Span<byte> frameBuffer = stackalloc byte[8]; // Max 4 controllers * 2 bytes

		for (uint i = 0; i < frameCount; i++) {
			int read = stream.Read(frameBuffer[..bytesPerFrame]);
			if (read < bytesPerFrame) {
				break; // End of file
			}

			var frame = new InputFrame((int)i);

			for (int c = 0; c < movie.ControllerCount && c < InputFrame.MaxPorts; c++) {
				ushort buttons = BinaryPrimitives.ReadUInt16LittleEndian(frameBuffer[(c * 2)..]);
				frame.Controllers[c] = ParseVbmButtons(buttons, movie.SystemType);
			}

			movie.InputFrames.Add(frame);
		}
	}

	/// <summary>
	/// Parse VBM button format into ControllerInput
	/// </summary>
	/// <param name="buttons">Button bits from VBM</param>
	/// <param name="system">System type for button mapping</param>
	/// <returns>Parsed controller input</returns>
	private static ControllerInput ParseVbmButtons(ushort buttons, SystemType system) {
		var input = new ControllerInput();

		if (system == SystemType.Gba) {
			// GBA button layout (10 buttons):
			// Bit 0: A
			// Bit 1: B
			// Bit 2: Select
			// Bit 3: Start
			// Bit 4: Right
			// Bit 5: Left
			// Bit 6: Up
			// Bit 7: Down
			// Bit 8: R
			// Bit 9: L
			input.A = (buttons & 0x0001) != 0;
			input.B = (buttons & 0x0002) != 0;
			input.Select = (buttons & 0x0004) != 0;
			input.Start = (buttons & 0x0008) != 0;
			input.Right = (buttons & 0x0010) != 0;
			input.Left = (buttons & 0x0020) != 0;
			input.Up = (buttons & 0x0040) != 0;
			input.Down = (buttons & 0x0080) != 0;
			input.R = (buttons & 0x0100) != 0;
			input.L = (buttons & 0x0200) != 0;
		} else {
			// GB/GBC button layout (8 buttons):
			// Bit 0: A
			// Bit 1: B
			// Bit 2: Select
			// Bit 3: Start
			// Bit 4: Right
			// Bit 5: Left
			// Bit 6: Up
			// Bit 7: Down
			input.A = (buttons & 0x0001) != 0;
			input.B = (buttons & 0x0002) != 0;
			input.Select = (buttons & 0x0004) != 0;
			input.Start = (buttons & 0x0008) != 0;
			input.Right = (buttons & 0x0010) != 0;
			input.Left = (buttons & 0x0020) != 0;
			input.Up = (buttons & 0x0040) != 0;
			input.Down = (buttons & 0x0080) != 0;
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

	/// <summary>
	/// Count set bits in a byte
	/// </summary>
	private static int CountSetBits(int value) {
		int count = 0;
		while (value != 0) {
			count += value & 1;
			value >>= 1;
		}

		return count;
	}

	/// <inheritdoc/>
	public override void Write(MovieData movie, Stream stream) {
		throw new NotSupportedException("VBM writing is not yet implemented");
	}
}
