using System;
using System.Buffers.Binary;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.IO.Hashing;
using System.Text;
using Nexen.Interop;
using Nexen.Utilities;

namespace Nexen.Debugger.Labels;

/// <summary>
/// Header data read from a .nexen-labels file.
/// </summary>
public sealed record NexenLabelHeader(
	ushort Version,
	byte Platform,
	byte Flags,
	uint RomCrc32,
	uint LabelCount,
	string RomName,
	string RomFilename
) {
	/// <summary>Whether the label data section is GZip-compressed.</summary>
	public bool IsCompressed => (Flags & NexenLabelFormat.FlagCompressed) != 0;
}

/// <summary>
/// Binary reader/writer for the Nexen native label format (.nexen-labels).
/// </summary>
/// <remarks>
/// <para>
/// File structure:
/// <list type="number">
///   <item><description>Header (32 bytes): magic, version, platform, flags, ROM CRC32, label count, data offset/size</description></item>
///   <item><description>ROM metadata: length-prefixed UTF-8 ROM name + filename</description></item>
///   <item><description>Label data: binary-serialized CodeLabel entries (optionally GZip-compressed)</description></item>
///   <item><description>Footer (8 bytes): CRC32 of all preceding data + end marker</description></item>
/// </list>
/// </para>
/// <para>
/// All multi-byte integers are little-endian (BinaryWriter/BinaryReader default).
/// Strings are length-prefixed: UInt16 byte count followed by UTF-8 bytes.
/// </para>
/// </remarks>
public static class NexenLabelFormat {
	/// <summary>Magic bytes identifying a .nexen-labels file.</summary>
	private static readonly byte[] Magic = "NXLABEL\0"u8.ToArray();

	/// <summary>Current format version: v1.0.</summary>
	public const ushort CurrentVersion = 0x0100;

	/// <summary>Flag indicating the label data section is GZip-compressed.</summary>
	public const byte FlagCompressed = 0x01;

	/// <summary>End-of-file marker.</summary>
	private static readonly byte[] EndMarker = "NXLE"u8.ToArray();

	/// <summary>Fixed header size in bytes.</summary>
	private const int HeaderSize = 32;

	/// <summary>Footer size in bytes (CRC32 + end marker).</summary>
	private const int FooterSize = 8;

	/// <summary>
	/// Write labels to a .nexen-labels binary file.
	/// </summary>
	/// <param name="path">Output file path.</param>
	/// <param name="labels">Labels to write.</param>
	/// <param name="romInfo">ROM information for metadata.</param>
	/// <param name="compress">If <c>true</c>, GZip-compress the label data section.</param>
	public static void Write(string path, List<CodeLabel> labels, RomInfo romInfo, bool compress = false) {
		// Sort labels for consistent output
		labels.Sort((a, b) => {
			int result = a.MemoryType.CompareTo(b.MemoryType);
			return result != 0 ? result : a.Address.CompareTo(b.Address);
		});

		string? dir = Path.GetDirectoryName(path);
		if (!string.IsNullOrEmpty(dir) && !Directory.Exists(dir)) {
			Directory.CreateDirectory(dir);
		}

		using var fileStream = new FileStream(path, FileMode.Create, FileAccess.Write, FileShare.None);
		using var crcStream = new Crc32Stream(fileStream);
		using var writer = new BinaryWriter(crcStream, Encoding.UTF8, leaveOpen: true);

		// Get ROM metadata
		uint romCrc32 = GetRomCrc32(romInfo);
		string romName = romInfo.GetRomName();
		string romFilename = Path.GetFileName(romInfo.RomPath);
		byte platform = (byte)romInfo.ConsoleType;
		byte flags = compress ? FlagCompressed : (byte)0;

		// Serialize label data to a memory buffer first (to get size)
		byte[] labelData = SerializeLabels(labels);
		byte[] dataToWrite = compress ? CompressData(labelData) : labelData;

		// Calculate data section offset = header(32) + romName + romFilename
		byte[] romNameBytes = Encoding.UTF8.GetBytes(romName);
		byte[] romFilenameBytes = Encoding.UTF8.GetBytes(romFilename);
		uint metadataSize = (uint)(2 + romNameBytes.Length + 2 + romFilenameBytes.Length);
		uint dataSectionOffset = HeaderSize + metadataSize;

		// === Write Header (32 bytes) ===
		writer.Write(Magic);                          // 0x00: 8 bytes magic
		writer.Write(CurrentVersion);                 // 0x08: 2 bytes version
		writer.Write(platform);                       // 0x0A: 1 byte platform
		writer.Write(flags);                          // 0x0B: 1 byte flags
		writer.Write(romCrc32);                       // 0x0C: 4 bytes ROM CRC32
		writer.Write((uint)labels.Count);             // 0x10: 4 bytes label count
		writer.Write(dataSectionOffset);              // 0x14: 4 bytes data offset
		writer.Write((uint)labelData.Length);          // 0x18: 4 bytes uncompressed size
		writer.Write((uint)0);                        // 0x1C: 4 bytes reserved

		// === Write ROM Metadata ===
		WriteLengthPrefixedString(writer, romNameBytes);
		WriteLengthPrefixedString(writer, romFilenameBytes);

		// === Write Label Data ===
		writer.Write(dataToWrite);

		// === Write Footer ===
		writer.Flush();
		uint contentCrc32 = crcStream.GetCrc32();
		// Write CRC32 directly to underlying stream (not through CRC calculation)
		fileStream.Write(BitConverter.GetBytes(contentCrc32));
		fileStream.Write(EndMarker);
	}

	/// <summary>
	/// Read labels from a .nexen-labels binary file.
	/// </summary>
	/// <param name="path">Path to the label file.</param>
	/// <returns>Tuple of (labels, header metadata).</returns>
	/// <exception cref="InvalidDataException">Thrown if the file is corrupt or has wrong format.</exception>
	public static (List<CodeLabel> Labels, NexenLabelHeader Header) Read(string path) {
		byte[] fileData = File.ReadAllBytes(path);

		if (fileData.Length < HeaderSize + FooterSize) {
			throw new InvalidDataException("File too small to be a valid .nexen-labels file.");
		}

		// === Validate Footer ===
		int footerOffset = fileData.Length - FooterSize;
		uint storedCrc32 = BitConverter.ToUInt32(fileData, footerOffset);
		if (fileData[footerOffset + 4] != EndMarker[0] ||
			fileData[footerOffset + 5] != EndMarker[1] ||
			fileData[footerOffset + 6] != EndMarker[2] ||
			fileData[footerOffset + 7] != EndMarker[3]) {
			throw new InvalidDataException("Invalid end marker in .nexen-labels file.");
		}

		// Verify CRC32 of content (everything before footer)
		uint computedCrc32 = ComputeCrc32(fileData.AsSpan(0, footerOffset));
		if (storedCrc32 != computedCrc32) {
			throw new InvalidDataException($"CRC32 mismatch: expected 0x{storedCrc32:x8}, computed 0x{computedCrc32:x8}.");
		}

		// === Read Header ===
		using var ms = new MemoryStream(fileData, 0, footerOffset);
		using var reader = new BinaryReader(ms, Encoding.UTF8);

		byte[] magic = reader.ReadBytes(8);
		if (!magic.AsSpan().SequenceEqual(Magic)) {
			throw new InvalidDataException("Invalid magic bytes. Not a .nexen-labels file.");
		}

		ushort version = reader.ReadUInt16();
		if (version > CurrentVersion) {
			throw new InvalidDataException($"Unsupported format version 0x{version:x4}. Maximum supported: 0x{CurrentVersion:x4}.");
		}

		byte platform = reader.ReadByte();
		byte flags = reader.ReadByte();
		uint romCrc32 = reader.ReadUInt32();
		uint labelCount = reader.ReadUInt32();
		uint dataSectionOffset = reader.ReadUInt32();
		uint uncompressedSize = reader.ReadUInt32();
		_ = reader.ReadUInt32(); // reserved

		// === Read ROM Metadata ===
		string romName = ReadLengthPrefixedString(reader);
		string romFilename = ReadLengthPrefixedString(reader);

		// === Read Label Data ===
		bool isCompressed = (flags & FlagCompressed) != 0;
		int dataLength = footerOffset - (int)dataSectionOffset;
		byte[] rawData = reader.ReadBytes(dataLength);

		byte[] labelData = isCompressed ? DecompressData(rawData, (int)uncompressedSize) : rawData;

		List<CodeLabel> labels = DeserializeLabels(labelData, (int)labelCount);

		var header = new NexenLabelHeader(version, platform, flags, romCrc32, labelCount, romName, romFilename);
		return (labels, header);
	}

	/// <summary>
	/// Read only the header from a .nexen-labels file (quick peek without parsing all labels).
	/// </summary>
	/// <param name="path">Path to the label file.</param>
	/// <returns>Header metadata.</returns>
	public static NexenLabelHeader ReadHeader(string path) {
		using var fs = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read);
		using var reader = new BinaryReader(fs, Encoding.UTF8);

		if (fs.Length < HeaderSize + FooterSize) {
			throw new InvalidDataException("File too small to be a valid .nexen-labels file.");
		}

		byte[] magic = reader.ReadBytes(8);
		if (!magic.AsSpan().SequenceEqual(Magic)) {
			throw new InvalidDataException("Invalid magic bytes. Not a .nexen-labels file.");
		}

		ushort version = reader.ReadUInt16();
		byte platform = reader.ReadByte();
		byte flags = reader.ReadByte();
		uint romCrc32 = reader.ReadUInt32();
		uint labelCount = reader.ReadUInt32();
		_ = reader.ReadUInt32(); // data offset
		_ = reader.ReadUInt32(); // uncompressed size
		_ = reader.ReadUInt32(); // reserved

		string romName = ReadLengthPrefixedString(reader);
		string romFilename = ReadLengthPrefixedString(reader);

		return new NexenLabelHeader(version, platform, flags, romCrc32, labelCount, romName, romFilename);
	}

	/// <summary>
	/// Check if a .nexen-labels file's ROM CRC32 matches the currently loaded ROM.
	/// </summary>
	/// <param name="path">Path to the label file.</param>
	/// <param name="romInfo">Current ROM information.</param>
	/// <returns><c>true</c> if the CRC32 matches or file has CRC32 = 0 (unknown).</returns>
	public static bool ValidateRomMatch(string path, RomInfo romInfo) {
		try {
			var header = ReadHeader(path);
			if (header.RomCrc32 == 0) {
				return true; // Unknown CRC, allow loading
			}
			return header.RomCrc32 == GetRomCrc32(romInfo);
		} catch (Exception ex) {
			Debug.WriteLine($"NexenLabelFormat.ValidateRomMatch failed: {ex.Message}");
			return false;
		}
	}

	/// <summary>
	/// Check if a file appears to be a valid .nexen-labels file by checking magic bytes.
	/// </summary>
	/// <param name="path">Path to check.</param>
	/// <returns><c>true</c> if the file starts with the correct magic bytes.</returns>
	public static bool IsNexenLabelFile(string path) {
		try {
			using var fs = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read);
			if (fs.Length < HeaderSize + FooterSize) return false;
			byte[] magic = new byte[8];
			fs.ReadExactly(magic);
			return magic.AsSpan().SequenceEqual(Magic);
		} catch (Exception ex) {
			Debug.WriteLine($"NexenLabelFormat.IsNexenLabelFile failed for '{path}': {ex.Message}");
			return false;
		}
	}

	// === Private Helpers ===

	/// <summary>Serialize a list of labels to a byte array.</summary>
	private static byte[] SerializeLabels(List<CodeLabel> labels) {
		using var ms = new MemoryStream(labels.Count * 32); // estimate
		using var writer = new BinaryWriter(ms, Encoding.UTF8);

		foreach (var label in labels) {
			label.WriteBinary(writer);
		}

		writer.Flush();
		return ms.ToArray();
	}

	/// <summary>Deserialize labels from a byte array.</summary>
	private static List<CodeLabel> DeserializeLabels(byte[] data, int expectedCount) {
		var labels = new List<CodeLabel>(expectedCount);
		using var ms = new MemoryStream(data);
		using var reader = new BinaryReader(ms, Encoding.UTF8);

		while (ms.Position < ms.Length) {
			labels.Add(CodeLabel.ReadBinary(reader));
		}

		return labels;
	}

	/// <summary>GZip-compress data.</summary>
	private static byte[] CompressData(byte[] data) {
		using var output = new MemoryStream();
		using (var gzip = new GZipStream(output, CompressionLevel.Optimal, leaveOpen: true)) {
			gzip.Write(data);
		}
		return output.ToArray();
	}

	/// <summary>GZip-decompress data.</summary>
	private static byte[] DecompressData(byte[] compressedData, int uncompressedSize) {
		byte[] result = new byte[uncompressedSize];
		using var input = new MemoryStream(compressedData);
		using var gzip = new GZipStream(input, CompressionMode.Decompress);
		int bytesRead = 0;
		int totalRead = 0;
		while (totalRead < uncompressedSize && (bytesRead = gzip.Read(result, totalRead, uncompressedSize - totalRead)) > 0) {
			totalRead += bytesRead;
		}
		return result;
	}

	/// <summary>Compute CRC32 of a span of bytes.</summary>
	private static uint ComputeCrc32(ReadOnlySpan<byte> data) {
		var crc = new Crc32();
		crc.Append(data);
		byte[] hash = crc.GetCurrentHash();
		// Crc32 returns bytes in little-endian order
		return BitConverter.ToUInt32(hash);
	}

	/// <summary>Get the ROM CRC32 from RomInfo.</summary>
	private static uint GetRomCrc32(RomInfo romInfo) {
		try {
			return RomHashService.ComputeRomHashes(romInfo).Crc32Value;
		} catch (Exception ex) {
			Debug.WriteLine($"NexenLabelFormat.GetRomCrc32 failed: {ex.Message}");
			return 0;
		}
	}

	/// <summary>Write a length-prefixed UTF-8 string (UInt16 byte count + bytes).</summary>
	private static void WriteLengthPrefixedString(BinaryWriter writer, byte[] utf8Bytes) {
		writer.Write((ushort)utf8Bytes.Length);
		writer.Write(utf8Bytes);
	}

	/// <summary>Read a length-prefixed UTF-8 string (UInt16 byte count + bytes).</summary>
	private static string ReadLengthPrefixedString(BinaryReader reader) {
		ushort length = reader.ReadUInt16();
		byte[] bytes = reader.ReadBytes(length);
		return Encoding.UTF8.GetString(bytes);
	}
}

/// <summary>
/// A stream wrapper that computes CRC32 of all data written through it.
/// </summary>
internal sealed class Crc32Stream : Stream {
	private readonly Stream _inner;
	private readonly Crc32 _crc = new();

	public Crc32Stream(Stream inner) {
		_inner = inner;
	}

	public uint GetCrc32() {
		byte[] hash = _crc.GetCurrentHash();
		return BitConverter.ToUInt32(hash);
	}

	public override void Write(byte[] buffer, int offset, int count) {
		_crc.Append(buffer.AsSpan(offset, count));
		_inner.Write(buffer, offset, count);
	}

	public override void Write(ReadOnlySpan<byte> buffer) {
		_crc.Append(buffer);
		_inner.Write(buffer);
	}

	public override void Flush() => _inner.Flush();
	public override bool CanRead => false;
	public override bool CanSeek => false;
	public override bool CanWrite => true;
	public override long Length => _inner.Length;
	public override long Position { get => _inner.Position; set => _inner.Position = value; }
	public override int Read(byte[] buffer, int offset, int count) => throw new NotSupportedException();
	public override long Seek(long offset, SeekOrigin origin) => throw new NotSupportedException();
	public override void SetLength(long value) => throw new NotSupportedException();
}
