using System;
using System.Buffers.Binary;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Text;

namespace Nexen.Utilities.Mcp;

/// <summary>
/// Minimal Pansy metadata file reader (TheAnsarya/pansy v1.0 binary format).
///
/// Parses the 32-byte header, the section table, and the SYMBOLS / COMMENTS /
/// MEMORY_REGIONS sections — the parts useful for "what's at this address"
/// lookups. We deliberately skip CDL maps, cross-refs, bookmarks, etc; the MCP
/// surface for those would be its own tool family if a project needs them.
///
/// Format reference: https://github.com/TheAnsarya/pansy/blob/main/docs/FILE-FORMAT.md
/// </summary>
internal sealed class PansyReader
{
	public const string Magic = "PANSY";

	private const uint SectionCodeDataMap = 0x0001;
	private const uint SectionSymbols = 0x0002;
	private const uint SectionComments = 0x0003;
	private const uint SectionMemoryRegions = 0x0004;

	private const ushort FlagCompressed = 0x0001;

	public ushort Version { get; private init; }
	public ushort Flags { get; private init; }
	public byte Platform { get; private init; }
	public uint RomSize { get; private init; }
	public uint RomCrc32 { get; private init; }
	public IReadOnlyList<SectionInfo> Sections { get; private init; } = Array.Empty<SectionInfo>();
	public IReadOnlyList<SymbolEntry> Symbols { get; private init; } = Array.Empty<SymbolEntry>();
	public IReadOnlyList<CommentEntry> Comments { get; private init; } = Array.Empty<CommentEntry>();
	public IReadOnlyList<MemoryRegionEntry> MemoryRegions { get; private init; } = Array.Empty<MemoryRegionEntry>();

	public bool IsCompressed => (Flags & FlagCompressed) != 0;

	public readonly record struct SectionInfo(uint Type, uint Offset, uint CompressedSize, uint UncompressedSize);
	public readonly record struct SymbolEntry(uint Address, byte Type, byte Flags, string Name);
	public readonly record struct CommentEntry(uint Address, byte Type, string Text);
	public readonly record struct MemoryRegionEntry(uint StartAddress, uint EndAddress, byte Type, byte Bank, string Name);

	public static PansyReader Load(byte[] data)
	{
		if(data.Length < 32) {
			throw new InvalidDataException("not a Pansy file (truncated header)");
		}
		if(data[0] != 'P' || data[1] != 'A' || data[2] != 'N' || data[3] != 'S' || data[4] != 'Y') {
			throw new InvalidDataException("not a Pansy file (bad magic)");
		}

		var span = data.AsSpan();
		ushort version = BinaryPrimitives.ReadUInt16LittleEndian(span[8..]);
		ushort flags = BinaryPrimitives.ReadUInt16LittleEndian(span[10..]);
		byte platform = data[12];
		uint romSize = BinaryPrimitives.ReadUInt32LittleEndian(span[16..]);
		uint romCrc32 = BinaryPrimitives.ReadUInt32LittleEndian(span[20..]);
		uint sectionCount = BinaryPrimitives.ReadUInt32LittleEndian(span[24..]);

		// Section table starts immediately after the 32-byte header. Each entry
		// is 16 bytes (Type/Offset/CompSize/UncompSize, all uint32 LE).
		var sections = new List<SectionInfo>((int)sectionCount);
		int tableEndOffset = 32 + (int)(sectionCount * 16);
		if(tableEndOffset > data.Length) {
			throw new InvalidDataException($"section table exceeds file size ({tableEndOffset} > {data.Length})");
		}
		int p = 32;
		for(int i = 0; i < sectionCount; i++) {
			uint type = BinaryPrimitives.ReadUInt32LittleEndian(span[p..]);
			uint offset = BinaryPrimitives.ReadUInt32LittleEndian(span[(p + 4)..]);
			uint compSize = BinaryPrimitives.ReadUInt32LittleEndian(span[(p + 8)..]);
			uint uncompSize = BinaryPrimitives.ReadUInt32LittleEndian(span[(p + 12)..]);
			sections.Add(new SectionInfo(type, offset, compSize, uncompSize));
			p += 16;
		}

		List<SymbolEntry> symbols = new();
		List<CommentEntry> comments = new();
		List<MemoryRegionEntry> regions = new();

		bool compressedFlag = (flags & FlagCompressed) != 0;

		foreach(var section in sections) {
			byte[] body = ReadSectionBody(data, section, compressedFlag);
			switch(section.Type) {
				case SectionSymbols: ParseSymbols(body, symbols); break;
				case SectionComments: ParseComments(body, comments); break;
				case SectionMemoryRegions: ParseMemoryRegions(body, regions); break;
				default: /* skip unsupported section types */ break;
			}
		}

		return new PansyReader {
			Version = version,
			Flags = flags,
			Platform = platform,
			RomSize = romSize,
			RomCrc32 = romCrc32,
			Sections = sections,
			Symbols = symbols,
			Comments = comments,
			MemoryRegions = regions,
		};
	}

	private static byte[] ReadSectionBody(byte[] data, SectionInfo section, bool compressedFlag)
	{
		// Per Pansy spec: a section is compressed iff the file has the
		// COMPRESSED flag AND CompressedSize != UncompressedSize. Some
		// sections (e.g. small ones that DEFLATE wouldn't shrink) are stored
		// raw even in a compressed file.
		if(section.Offset + section.CompressedSize > data.Length) {
			throw new InvalidDataException($"section type 0x{section.Type:X4} extends past end of file");
		}
		bool isDeflated = compressedFlag && section.CompressedSize != section.UncompressedSize;
		if(!isDeflated) {
			byte[] raw = new byte[section.CompressedSize];
			Array.Copy(data, (int)section.Offset, raw, 0, raw.Length);
			return raw;
		}
		using var ms = new MemoryStream(data, (int)section.Offset, (int)section.CompressedSize, writable: false);
		using var ds = new DeflateStream(ms, CompressionMode.Decompress);
		byte[] result = new byte[section.UncompressedSize];
		ds.ReadExactly(result);
		return result;
	}

	private static void ParseSymbols(byte[] data, List<SymbolEntry> sink)
	{
		var span = data.AsSpan();
		int pos = 0, len = data.Length;
		// Minimum record: 4 (addr) + 1 (type) + 1 (flags) + 2 (nameLen) + 0 +
		// 2 (valueLen) + 0 = 10 bytes.
		while(pos + 10 <= len) {
			uint addr = BinaryPrimitives.ReadUInt32LittleEndian(span[pos..]);
			byte type = data[pos + 4];
			byte flagByte = data[pos + 5];
			ushort nameLen = BinaryPrimitives.ReadUInt16LittleEndian(span[(pos + 6)..]);
			pos += 8;
			if(pos + nameLen > len) break;
			string name = Encoding.UTF8.GetString(data, pos, nameLen);
			pos += nameLen;
			if(pos + 2 > len) break;
			ushort valueLen = BinaryPrimitives.ReadUInt16LittleEndian(span[pos..]);
			pos += 2 + valueLen;  // skip the value bytes; we don't need them for lookup
			sink.Add(new SymbolEntry(addr, type, flagByte, name));
		}
	}

	private static void ParseComments(byte[] data, List<CommentEntry> sink)
	{
		var span = data.AsSpan();
		int pos = 0, len = data.Length;
		// Minimum record: 4 (addr) + 1 (type) + 2 (textLen) + 0 = 7 bytes.
		while(pos + 7 <= len) {
			uint addr = BinaryPrimitives.ReadUInt32LittleEndian(span[pos..]);
			byte type = data[pos + 4];
			ushort textLen = BinaryPrimitives.ReadUInt16LittleEndian(span[(pos + 5)..]);
			pos += 7;
			if(pos + textLen > len) break;
			string text = Encoding.UTF8.GetString(data, pos, textLen);
			pos += textLen;
			sink.Add(new CommentEntry(addr, type, text));
		}
	}

	private static void ParseMemoryRegions(byte[] data, List<MemoryRegionEntry> sink)
	{
		var span = data.AsSpan();
		int pos = 0, len = data.Length;
		// Minimum: 4 (start) + 4 (end) + 1 (type) + 1 (bank) + 2 (flags) + 2 (nameLen) + 0 = 14 bytes.
		while(pos + 14 <= len) {
			uint start = BinaryPrimitives.ReadUInt32LittleEndian(span[pos..]);
			uint end = BinaryPrimitives.ReadUInt32LittleEndian(span[(pos + 4)..]);
			byte type = data[pos + 8];
			byte bank = data[pos + 9];
			// flags at pos+10 (uint16) — currently unused
			ushort nameLen = BinaryPrimitives.ReadUInt16LittleEndian(span[(pos + 12)..]);
			pos += 14;
			if(pos + nameLen > len) break;
			string name = Encoding.UTF8.GetString(data, pos, nameLen);
			pos += nameLen;
			sink.Add(new MemoryRegionEntry(start, end, type, bank, name));
		}
	}
}
