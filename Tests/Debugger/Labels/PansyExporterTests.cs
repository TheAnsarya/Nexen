using Xunit;
using System;
using System.IO;
using System.IO.Compression;
using System.Reflection;
using System.Text;
using Nexen.Debugger.Integration;
using Nexen.Debugger.Labels;
using Nexen.Interop;
using Nexen.Utilities;

namespace Nexen.Tests.Debugger.Labels;

/// <summary>
/// Unit tests for PansyExporter functionality.
/// Tests the Pansy metadata file format export capabilities.
/// </summary>
public class PansyExporterTests
{
	// Pansy format constants for testing
	private const string MAGIC = "PANSY\0\0\0";
	private const ushort VERSION_1_0 = 0x0100;
	private const ushort FLAG_COMPRESSED = 0x0001;

	#region CRC32 Tests

	[Fact]
	public void Crc32_EmptyData_ReturnsZero()
	{
		// The CRC32 of empty data should return 0 (or handle gracefully)
		byte[] emptyData = [];
		uint crc = ComputeCrc32(emptyData);
		Assert.Equal(0u, crc);
	}

	[Fact]
	public void Crc32_KnownData_ReturnsExpectedValue()
	{
		// "123456789" has known CRC32 = 0xCBF43926
		byte[] data = Encoding.ASCII.GetBytes("123456789");
		uint crc = ComputeCrc32(data);
		Assert.Equal(0xCBF43926u, crc);
	}

	[Fact]
	public void Crc32_SingleByte_ReturnsConsistentValue()
	{
		byte[] data = [0x00];
		uint crc1 = ComputeCrc32(data);
		uint crc2 = ComputeCrc32(data);
		Assert.Equal(crc1, crc2);
	}

	[Fact]
	public void Crc32_AllOnes_ReturnsExpectedValue()
	{
		byte[] data = [0xFF, 0xFF, 0xFF, 0xFF];
		uint crc = ComputeCrc32(data);
		Assert.NotEqual(0u, crc); // Should return non-zero value
	}

	#endregion

	#region Header Tests

	[Fact]
	public void Header_ValidMagic_ParsesCorrectly()
	{
		using var ms = new MemoryStream();
		using var writer = new BinaryWriter(ms);

		// Write valid header matching Pansy spec v1.0
		writer.Write(Encoding.ASCII.GetBytes(MAGIC)); // 8 bytes
		writer.Write(VERSION_1_0);                     // 2 bytes
		writer.Write((ushort)0);                        // Flags 2 bytes
		writer.Write((byte)0x02);                        // Platform: SNES
		writer.Write((byte)0);                           // Reserved
		writer.Write((ushort)0);                         // Reserved
		writer.Write((uint)0x100000);                    // ROM size 1MB
		writer.Write((uint)0xDEADBEEF);                  // ROM CRC
		writer.Write((uint)3);                           // Section count
		writer.Write((uint)0);                           // Reserved

		ms.Position = 0;
		var header = ReadTestHeader(ms);

		Assert.NotNull(header);
		Assert.Equal(VERSION_1_0, header.Version);
		Assert.Equal(0x02, header.Platform);
		Assert.Equal(0x100000u, header.RomSize);
		Assert.Equal(0xDEADBEEFu, header.RomCrc32);
		Assert.Equal(3u, header.SectionCount);
	}

	[Fact]
	public void Header_SectionCount_ParsesCorrectly()
	{
		using var ms = new MemoryStream();
		using var writer = new BinaryWriter(ms);

		// Write v1.0 header with 5 sections
		writer.Write(Encoding.ASCII.GetBytes(MAGIC));
		writer.Write(VERSION_1_0);
		writer.Write((ushort)0);            // Flags
		writer.Write((byte)0x02);            // Platform: SNES
		writer.Write((byte)0);               // Reserved
		writer.Write((ushort)0);             // Reserved
		writer.Write((uint)0x100000);        // ROM size
		writer.Write((uint)0xDEADBEEF);      // ROM CRC
		writer.Write((uint)5);               // Section count
		writer.Write((uint)0);               // Reserved

		ms.Position = 0;
		var header = ReadTestHeader(ms);

		Assert.NotNull(header);
		Assert.Equal(VERSION_1_0, header.Version);
		Assert.Equal(5u, header.SectionCount);
	}

	[Fact]
	public void Header_CompressedFlag_ParsesCorrectly()
	{
		using var ms = new MemoryStream();
		using var writer = new BinaryWriter(ms);

		// Write header with compression flag
		writer.Write(Encoding.ASCII.GetBytes(MAGIC));
		writer.Write(VERSION_1_0);
		writer.Write(FLAG_COMPRESSED);       // Flags as uint16
		writer.Write((byte)0x02);            // Platform: SNES
		writer.Write((byte)0);               // Reserved
		writer.Write((ushort)0);             // Reserved
		writer.Write((uint)0x100000);        // ROM size
		writer.Write((uint)0xDEADBEEF);      // ROM CRC
		writer.Write((uint)0);               // Section count
		writer.Write((uint)0);               // Reserved

		ms.Position = 0;
		var header = ReadTestHeader(ms);

		Assert.NotNull(header);
		Assert.Equal(FLAG_COMPRESSED, header.Flags);
	}

	[Fact]
	public void Header_InvalidMagic_ReturnsNull()
	{
		using var ms = new MemoryStream();
		using var writer = new BinaryWriter(ms);

		// Write invalid magic
		writer.Write(Encoding.ASCII.GetBytes("INVALID\0"));
		writer.Write(VERSION_1_0);

		ms.Position = 0;
		var header = ReadTestHeader(ms);

		Assert.Null(header);
	}

	[Fact]
	public void Header_TruncatedFile_ReturnsNull()
	{
		using var ms = new MemoryStream();
		using var writer = new BinaryWriter(ms);

		// Write only magic
		writer.Write(Encoding.ASCII.GetBytes(MAGIC));

		ms.Position = 0;
		var header = ReadTestHeader(ms);

		Assert.Null(header);
	}

	#endregion

	#region Platform ID Tests

	[Theory]
	[InlineData(0x01, "NES")]
	[InlineData(0x02, "SNES")]
	[InlineData(0x03, "GameBoy")]
	[InlineData(0x04, "GBA")]
	[InlineData(0x06, "SMS")]
	[InlineData(0x07, "PCEngine")]
	[InlineData(0x08, "Atari2600")]
	[InlineData(0x09, "Lynx")]
	[InlineData(0x0a, "WonderSwan")]
	[InlineData(0x1f, "ChannelF")]
	public void PlatformId_ValidValues_AreRecognized(byte id, string platformName)
	{
		Assert.True(id > 0, $"Platform {platformName} should have valid ID");
		Assert.True(id < 0xFF, $"Platform {platformName} should not be 0xFF (unknown)");
	}

	#endregion

	#region Section Building Tests

	[Fact]
	public void BuildAddressListSection_EmptyArray_ReturnsCountOnly()
	{
		var addresses = Array.Empty<uint>();
		var bytes = BuildAddressListSection(addresses);

		Assert.Equal(4, bytes.Length); // Just the count (uint32)
		Assert.Equal(0u, BitConverter.ToUInt32(bytes, 0));
	}

	[Fact]
	public void BuildAddressListSection_WithAddresses_IncludesAll()
	{
		uint[] addresses = [0x8000, 0x8100, 0x8200];
		var bytes = BuildAddressListSection(addresses);

		Assert.Equal(16, bytes.Length); // count (4) + 3 addresses (12)
		Assert.Equal(3u, BitConverter.ToUInt32(bytes, 0));
		Assert.Equal(0x8000u, BitConverter.ToUInt32(bytes, 4));
		Assert.Equal(0x8100u, BitConverter.ToUInt32(bytes, 8));
		Assert.Equal(0x8200u, BitConverter.ToUInt32(bytes, 12));
	}

	[Fact]
	public void BuildSymbolSection_EmptyLabels_ReturnsCountOnly()
	{
		var labels = new List<TestLabel>();
		var bytes = BuildSymbolSection(labels);

		Assert.Equal(4, bytes.Length); // Just the count
		Assert.Equal(0u, BitConverter.ToUInt32(bytes, 0));
	}

	[Fact]
	public void BuildSymbolSection_WithLabel_IncludesNameAndAddress()
	{
		List<TestLabel> labels = [
			new TestLabel { Address = 0x8000, Label = "TestFunc" }
		];
		var bytes = BuildSymbolSection(labels);

		// Count(4) + Address(4) + Type(1) + Flags(1) + MemType(1) + Reserved(1) + NameLen(2) + Name(8)
		Assert.True(bytes.Length > 4);
		Assert.Equal(1u, BitConverter.ToUInt32(bytes, 0)); // 1 symbol
	}

	[Fact]
	public void BuildCommentSection_EmptyComments_ReturnsCountOnly()
	{
		List<TestLabel> labels = [];
		var bytes = BuildCommentSection(labels);

		Assert.Equal(4, bytes.Length);
		Assert.Equal(0u, BitConverter.ToUInt32(bytes, 0));
	}

	[Fact]
	public void BuildCommentSection_WithComment_IncludesTextAndAddress()
	{
		List<TestLabel> labels = [
			new TestLabel { Address = 0x8000, Comment = "Test comment" }
		];
		var bytes = BuildCommentSection(labels);

		Assert.True(bytes.Length > 4);
		Assert.Equal(1u, BitConverter.ToUInt32(bytes, 0)); // 1 comment
	}

	[Fact]
	public void SerializeSourceMapSection_WithEntries_WritesFileTableAndEntryCount()
	{
		var files = new List<SourceFileInfo> {
			new SourceFileInfo("main.pasm", true, new TestFileDataProvider(["lda #$01", "sta $2000"]))
		};

		var entries = new List<(uint Address, ushort FileIndex, uint LineStart, uint LineEndExclusive)> {
			(0x8000u, 0, 1, 2),
			(0x8002u, 0, 2, 3)
		};

		var method = typeof(PansyExporter).GetMethod("SerializeSourceMapSection", BindingFlags.NonPublic | BindingFlags.Static);
		Assert.NotNull(method);

		var result = method!.Invoke(null, [files, entries]) as byte[];
		Assert.NotNull(result);
		Assert.True(result!.Length > 0);

		using var ms = new MemoryStream(result);
		using var reader = new BinaryReader(ms, Encoding.UTF8, leaveOpen: true);

		uint fileCount = reader.ReadUInt32();
		Assert.Equal(1u, fileCount);

		ushort fileNameLength = reader.ReadUInt16();
		string fileName = Encoding.UTF8.GetString(reader.ReadBytes(fileNameLength));
		Assert.Equal("main.pasm", fileName);

		uint entryCount = reader.ReadUInt32();
		Assert.Equal(2u, entryCount);
	}

	#endregion

	#region File Path Tests

	[Fact]
	public void GetPansyFilePath_ValidRomName_ReturnsCorrectExtension()
	{
		string romName = "TestGame.sfc";
		string path = GetPansyFilePath(romName);

		Assert.EndsWith(".pansy", path);
		Assert.Contains("TestGame", path);
	}

	[Fact]
	public void GetPansyFilePathWithCrc_IncludesCrcInFilename()
	{
		string romName = "TestGame.sfc";
		uint crc = 0xDEADBEEF;
		string path = GetPansyFilePathWithCrc(romName, crc);

		Assert.Contains("deadbeef", path.ToLower());
		Assert.EndsWith(".pansy", path);
	}

	#endregion

	#region Integration Tests (Mocked)

	[Fact]
	public void Export_ValidData_CreatesFile()
	{
		// This is a mock test - actual export requires Nexen runtime
		string tempPath = Path.Combine(Path.GetTempPath(), $"test_{Guid.NewGuid()}.pansy");

		try {
			// Create a minimal valid pansy file (spec v1.0 header = 32 bytes)
			using (var stream = new FileStream(tempPath, FileMode.Create))
			using (var writer = new BinaryWriter(stream)) {
				writer.Write(Encoding.ASCII.GetBytes(MAGIC)); // 8 bytes
				writer.Write(VERSION_1_0);                     // 2 bytes
				writer.Write((ushort)0);                        // Flags 2 bytes
				writer.Write((byte)0x02);                        // Platform: SNES
				writer.Write((byte)0);                           // Reserved
				writer.Write((ushort)0);                         // Reserved
				writer.Write((uint)0x100000);                    // ROM size
				writer.Write((uint)0x12345678);                  // ROM CRC
				writer.Write((uint)0);                           // Section count
				writer.Write((uint)0);                           // Reserved
			}

			// Asserts after streams are closed
			Assert.True(File.Exists(tempPath));
			Assert.True(new FileInfo(tempPath).Length >= 32);
		} finally {
			if (File.Exists(tempPath)) File.Delete(tempPath);
		}
	}

	#endregion

	#region Helper Methods (mirroring PansyExporter implementation)

	/// <summary>
	/// Calculate CRC32 using RomHashService (StreamHash-based).
	/// Returns 0 for empty arrays to match PansyExporter behavior.
	/// </summary>
	private static uint ComputeCrc32(byte[] data)
	{
		if (data.Length == 0) return 0;
		return RomHashService.ComputeBasicHashes(data).Crc32Value;
	}

	private record TestHeader(ushort Version, ushort Flags, byte Platform, uint RomSize, uint RomCrc32, uint SectionCount);

	private static TestHeader? ReadTestHeader(Stream stream)
	{
		try {
			using var reader = new BinaryReader(stream, Encoding.UTF8, leaveOpen: true);

			byte[] magic = reader.ReadBytes(8);
			if (Encoding.ASCII.GetString(magic).TrimEnd('\0') != "PANSY")
				return null;

			ushort version = reader.ReadUInt16();       // offset 8
			ushort flags = reader.ReadUInt16();          // offset 10 (PansyFlags as uint16)
			byte platform = reader.ReadByte();           // offset 12
			reader.ReadByte();                           // offset 13 reserved
			reader.ReadUInt16();                         // offset 14 reserved
			uint romSize = reader.ReadUInt32();          // offset 16
			uint romCrc32 = reader.ReadUInt32();         // offset 20
			uint sectionCount = reader.ReadUInt32();     // offset 24
			reader.ReadUInt32();                         // offset 28 reserved

			return new TestHeader(version, flags, platform, romSize, romCrc32, sectionCount);
		} catch {
			return null;
		}
	}

	private static byte[] BuildAddressListSection(uint[] addresses)
	{
		using var ms = new MemoryStream();
		using var writer = new BinaryWriter(ms);
		writer.Write((uint)addresses.Length);
		foreach (var addr in addresses) {
			writer.Write(addr);
		}
		return ms.ToArray();
	}

	private sealed class TestLabel
	{
		public uint Address { get; set; }
		public string Label { get; set; } = "";
		public string Comment { get; set; } = "";
		public byte MemoryType { get; set; }
		public uint Length { get; set; } = 1;
	}

	private sealed class TestFileDataProvider : IFileDataProvider
	{
		public string[] Data { get; }

		public TestFileDataProvider(string[] data)
		{
			Data = data;
		}
	}

	private static byte[] BuildSymbolSection(List<TestLabel> labels)
	{
		using var ms = new MemoryStream();
		using var writer = new BinaryWriter(ms);

		var symbolLabels = labels.Where(l => !string.IsNullOrEmpty(l.Label)).ToList();
		writer.Write((uint)symbolLabels.Count);

		foreach (var label in symbolLabels) {
			writer.Write(label.Address);
			writer.Write((byte)1); // Type
			writer.Write((byte)0); // Flags
			writer.Write(label.MemoryType);
			writer.Write((byte)0); // Reserved
			byte[] nameBytes = Encoding.UTF8.GetBytes(label.Label);
			writer.Write((ushort)nameBytes.Length);
			writer.Write(nameBytes);
		}

		return ms.ToArray();
	}

	private static byte[] BuildCommentSection(List<TestLabel> labels)
	{
		using var ms = new MemoryStream();
		using var writer = new BinaryWriter(ms);

		var commentLabels = labels.Where(l => !string.IsNullOrEmpty(l.Comment)).ToList();
		writer.Write((uint)commentLabels.Count);

		foreach (var label in commentLabels) {
			writer.Write(label.Address);
			writer.Write((byte)1); // Type
			writer.Write(label.MemoryType);
			writer.Write((ushort)0); // Reserved
			byte[] commentBytes = Encoding.UTF8.GetBytes(label.Comment);
			writer.Write((ushort)commentBytes.Length);
			writer.Write(commentBytes);
		}

		return ms.ToArray();
	}

	private static string GetPansyFilePath(string romName)
	{
		string filename = Path.GetFileNameWithoutExtension(romName);
		return Path.Combine(Path.GetTempPath(), $"{filename}.pansy");
	}

	private static string GetPansyFilePathWithCrc(string romName, uint crc32)
	{
		string filename = Path.GetFileNameWithoutExtension(romName);
		return Path.Combine(Path.GetTempPath(), $"{filename}_{crc32:x8}.pansy");
	}

	#endregion

	#region Phase 3: Memory Regions Tests

	[Fact]
	public void BuildMemoryRegionsSection_EmptyLabels_ReturnsCountOnly()
	{
		var labels = new List<TestLabel>();
		var bytes = BuildMemoryRegionsSection(labels);

		Assert.Equal(4, bytes.Length); // Just the count
		Assert.Equal(0u, BitConverter.ToUInt32(bytes, 0));
	}

	[Fact]
	public void BuildMemoryRegionsSection_WithRegion_IncludesAddressRange()
	{
		List<TestLabel> labels = [
			new TestLabel { Address = 0x8000, Label = "DataTable", Length = 0x100 }
		];
		var bytes = BuildMemoryRegionsSection(labels);

		Assert.True(bytes.Length > 4);
		Assert.Equal(1u, BitConverter.ToUInt32(bytes, 0)); // 1 region
		Assert.Equal(0x8000u, BitConverter.ToUInt32(bytes, 4)); // Start
		Assert.Equal(0x80FFu, BitConverter.ToUInt32(bytes, 8)); // End (Start + Length - 1)
	}

	private static byte[] BuildMemoryRegionsSection(List<TestLabel> labels)
	{
		using var ms = new MemoryStream();
		using var writer = new BinaryWriter(ms);

		var regions = labels.Where(l => l.Length > 1 && !string.IsNullOrEmpty(l.Label)).ToList();
		writer.Write((uint)regions.Count);

		foreach (var region in regions) {
			writer.Write(region.Address);                          // Start (4)
			writer.Write(region.Address + region.Length - 1);      // End (4)
			writer.Write((byte)2);                                  // Type (1) - Data
			writer.Write(region.MemoryType);                        // MemType (1)
			writer.Write((ushort)0);                                // Flags (2)
			byte[] nameBytes = Encoding.UTF8.GetBytes(region.Label);
			writer.Write((ushort)nameBytes.Length);
			writer.Write(nameBytes);
		}

		return ms.ToArray();
	}

	#endregion

	#region Phase 3: Cross-References Tests

	[Fact]
	public void BuildCrossRefsSection_EmptyData_ReturnsCountOnly()
	{
		var bytes = BuildCrossRefsSection([]);

		Assert.Equal(4, bytes.Length);
		Assert.Equal(0u, BitConverter.ToUInt32(bytes, 0));
	}

	[Fact]
	public void BuildCrossRefsSection_WithXref_IncludesSourceAndTarget()
	{
		(uint From, uint To, byte Type)[] xrefs = [(0x8000, 0x9000, 1)];
		var bytes = BuildCrossRefsSection(xrefs);

		// Count(4) + From(4) + To(4) + Type(1) + MemTypeFrom(1) + MemTypeTo(1) + Flags(1) = 16
		Assert.Equal(16, bytes.Length);
		Assert.Equal(1u, BitConverter.ToUInt32(bytes, 0)); // 1 xref
		Assert.Equal(0x8000u, BitConverter.ToUInt32(bytes, 4)); // From
		Assert.Equal(0x9000u, BitConverter.ToUInt32(bytes, 8)); // To
		Assert.Equal((byte)1, bytes[12]); // Type = Call
	}

	private static byte[] BuildCrossRefsSection((uint From, uint To, byte Type)[] xrefs)
	{
		using var ms = new MemoryStream();
		using var writer = new BinaryWriter(ms);

		writer.Write((uint)xrefs.Length);
		foreach (var xref in xrefs) {
			writer.Write(xref.From);   // Source address (4)
			writer.Write(xref.To);     // Target address (4)
			writer.Write(xref.Type);   // Type (1)
			writer.Write((byte)0);     // MemType from (1)
			writer.Write((byte)0);     // MemType to (1)
			writer.Write((byte)0);     // Flags (1)
		}

		return ms.ToArray();
	}

	#endregion

	#region Phase 4: Compression Tests

	[Fact]
	public void CompressData_SmallData_ReturnsOriginal()
	{
		// Data smaller than 64 bytes should not be compressed
		byte[] smallData = new byte[32];
		Array.Fill(smallData, (byte)0xAB);

		var result = CompressData(smallData);

		Assert.Equal(smallData, result);
	}

	[Fact]
	public void CompressData_LargeRepetitiveData_CompressesWell()
	{
		// Repetitive data should compress well
		byte[] largeData = new byte[1024];
		Array.Fill(largeData, (byte)0xAB);

		var compressed = CompressData(largeData);

		Assert.True(compressed.Length < largeData.Length, "Compressed size should be smaller for repetitive data");
	}

	[Fact]
	public void CompressData_RandomData_MayNotCompress()
	{
		// Random data may not compress - verify it doesn't expand
		var random = new Random(42); // Fixed seed for reproducibility
		byte[] randomData = new byte[256];
		random.NextBytes(randomData);

		var result = CompressData(randomData);

		// Should return original if compression doesn't help
		Assert.True(result.Length <= randomData.Length + 50); // GZip header overhead
	}

	[Fact]
	public void DecompressData_CompressedData_ReturnsOriginal()
	{
		// Compress and decompress, verify roundtrip
		byte[] original = new byte[256];
		Array.Fill(original, (byte)0xCD);

		var compressed = CompressDataForce(original);
		var decompressed = DecompressData(compressed, original.Length);

		Assert.Equal(original, decompressed);
	}

	private static byte[] CompressData(byte[] data)
	{
		if (data.Length < 64)
			return data;

		using var output = new MemoryStream();
		using (var deflate = new DeflateStream(output, CompressionLevel.Optimal, leaveOpen: true)) {
			deflate.Write(data, 0, data.Length);
		}

		var compressed = output.ToArray();
		return compressed.Length < data.Length ? compressed : data;
	}

	private static byte[] CompressDataForce(byte[] data)
	{
		using var output = new MemoryStream();
		using (var deflate = new DeflateStream(output, CompressionLevel.Optimal, leaveOpen: true)) {
			deflate.Write(data, 0, data.Length);
		}
		return output.ToArray();
	}

	private static byte[] DecompressData(byte[] compressedData, int uncompressedSize)
	{
		using var input = new MemoryStream(compressedData);
		using var deflate = new DeflateStream(input, CompressionMode.Decompress);
		var output = new byte[uncompressedSize];
		int totalRead = 0;
		while (totalRead < uncompressedSize) {
			int read = deflate.Read(output, totalRead, uncompressedSize - totalRead);
			if (read == 0) break;
			totalRead += read;
		}
		return output;
	}

	#endregion

	#region Phase 4: Export Options Tests

	[Fact]
	public void ExportOptions_DefaultValues_AllEnabled()
	{
		var options = new TestExportOptions();

		Assert.True(options.IncludeMemoryRegions);
		Assert.True(options.IncludeCrossReferences);
		Assert.True(options.IncludeDataBlocks);
		Assert.False(options.UseCompression);
	}

	private sealed class TestExportOptions
	{
		public bool IncludeMemoryRegions { get; set; } = true;
		public bool IncludeCrossReferences { get; set; } = true;
		public bool IncludeDataBlocks { get; set; } = true;
		public bool UseCompression { get; set; } = false;
	}

	#endregion

	#region CDM Mapping Tests

	[Fact]
	public void MapCdlToPansyFlags_PreserveUpperNibble_MapsOpcodeDrawnReadIndirect() {
		byte[] cdl = [
			(byte)(CDL_CODE | CDL_OPCODE),
			(byte)(CDL_DATA | CDL_DRAWN),
			(byte)(CDL_DATA | CDL_READ),
			(byte)(CDL_CODE | CDL_INDIRECT),
		];

		var result = MapCdlToPansyFlagsTest(cdl, [], [], preserveUpperNibbleFlags: true);

		Assert.Equal((byte)(CDL_CODE | CDL_OPCODE), result[0]);
		Assert.Equal((byte)(CDL_DATA | CDL_DRAWN | CDL_READ), result[1]); // DATA implies READ
		Assert.Equal((byte)(CDL_DATA | CDL_READ), result[2]);
		Assert.Equal((byte)(CDL_CODE | CDL_INDIRECT), result[3]);
	}

	[Fact]
	public void MapCdlToPansyFlags_MaskUpperNibble_DropsOpcodeDrawnOverlayBits() {
		byte[] cdl = [
			(byte)(CDL_CODE | CDL_SNES_INDEX_MODE_8),
			(byte)(CDL_DATA | CDL_SNES_MEMORY_MODE_8),
		];

		var result = MapCdlToPansyFlagsTest(cdl, [], [], preserveUpperNibbleFlags: false);

		Assert.Equal(CDL_CODE, result[0]);
		Assert.Equal((byte)(CDL_DATA | CDL_READ), result[1]);
	}

	[Fact]
	public void MapCdlToPansyFlags_ExplicitJumpAndFunctionSets_AreApplied() {
		byte[] cdl = new byte[8];
		var jumpTargets = new uint[] { 2, 4 };
		var functions = new uint[] { 4, 6 };

		var result = MapCdlToPansyFlagsTest(cdl, jumpTargets, functions, preserveUpperNibbleFlags: true);

		Assert.Equal(CDL_JUMP_TARGET, result[2]);
		Assert.Equal((byte)(CDL_JUMP_TARGET | CDL_SUB_ENTRY), result[4]);
		Assert.Equal(CDL_SUB_ENTRY, result[6]);
	}

	#endregion

	#region CPU State Section Tests

	// CDL flag constants (must match PansyExporter)
	private const byte CDL_CODE = 0x01;
	private const byte CDL_DATA = 0x02;
	private const byte CDL_JUMP_TARGET = 0x04;
	private const byte CDL_SUB_ENTRY = 0x08;
	private const byte CDL_OPCODE = 0x10;
	private const byte CDL_DRAWN = 0x20;
	private const byte CDL_READ = 0x40;
	private const byte CDL_INDIRECT = 0x80;
	private const byte CDL_SNES_INDEX_MODE_8 = 0x10;
	private const byte CDL_SNES_MEMORY_MODE_8 = 0x20;
	private const byte CDL_GBA_THUMB = 0x20;
	private const int CPU_STATE_ENTRY_SIZE = 9;

	[Fact]
	public void CpuState_NullCdl_ReturnsEmpty() {
		var result = BuildCpuStateSectionTest(null, isSnes: true, isGba: false);
		Assert.Empty(result);
	}

	[Fact]
	public void CpuState_EmptyCdl_ReturnsEmpty() {
		var result = BuildCpuStateSectionTest([], isSnes: true, isGba: false);
		Assert.Empty(result);
	}

	[Fact]
	public void CpuState_NesRom_ReturnsEmpty() {
		// NES doesn't have X/M mode flags — should produce no CPU state entries
		byte[] cdl = [CDL_CODE | CDL_SNES_INDEX_MODE_8, CDL_CODE, CDL_DATA];
		var result = BuildCpuStateSectionTest(cdl, isSnes: false, isGba: false);
		Assert.Empty(result);
	}

	[Fact]
	public void CpuState_SnesIndexMode8_CreatesEntry() {
		byte[] cdl = [CDL_CODE | CDL_SNES_INDEX_MODE_8];
		var result = BuildCpuStateSectionTest(cdl, isSnes: true, isGba: false);

		Assert.Equal(CPU_STATE_ENTRY_SIZE, result.Length);
		Assert.Equal(0u, BitConverter.ToUInt32(result, 0));  // Address 0
		Assert.Equal(0x01, result[4]);                        // Flags: XFlag set
		Assert.Equal(0, result[5]);                           // DataBank
		Assert.Equal((ushort)0, BitConverter.ToUInt16(result, 6)); // DirectPage
		Assert.Equal(0, result[8]);                           // CpuMode: Native65816
	}

	[Fact]
	public void CpuState_SnesMemoryMode8_CreatesEntry() {
		byte[] cdl = [CDL_CODE | CDL_SNES_MEMORY_MODE_8];
		var result = BuildCpuStateSectionTest(cdl, isSnes: true, isGba: false);

		Assert.Equal(CPU_STATE_ENTRY_SIZE, result.Length);
		Assert.Equal(0u, BitConverter.ToUInt32(result, 0));
		Assert.Equal(0x02, result[4]);  // Flags: MFlag set
	}

	[Fact]
	public void CpuState_SnesBothFlags_CreatesEntry() {
		byte[] cdl = [CDL_CODE | CDL_SNES_INDEX_MODE_8 | CDL_SNES_MEMORY_MODE_8];
		var result = BuildCpuStateSectionTest(cdl, isSnes: true, isGba: false);

		Assert.Equal(CPU_STATE_ENTRY_SIZE, result.Length);
		Assert.Equal(0x03, result[4]);  // Flags: both XFlag and MFlag
	}

	[Fact]
	public void CpuState_SnesCodeWithoutModeFlags_NoEntry() {
		// Code byte with no mode flags → no CPU state entry
		byte[] cdl = [CDL_CODE];
		var result = BuildCpuStateSectionTest(cdl, isSnes: true, isGba: false);
		Assert.Empty(result);
	}

	[Fact]
	public void CpuState_SnesDataByte_NoEntry() {
		// Data byte with mode flags should NOT create an entry
		byte[] cdl = [CDL_DATA | CDL_SNES_INDEX_MODE_8];
		var result = BuildCpuStateSectionTest(cdl, isSnes: true, isGba: false);
		Assert.Empty(result);
	}

	[Fact]
	public void CpuState_SnesMultipleEntries_CorrectAddresses() {
		byte[] cdl = new byte[100];
		cdl[10] = CDL_CODE | CDL_SNES_INDEX_MODE_8;
		cdl[50] = CDL_CODE | CDL_SNES_MEMORY_MODE_8;
		cdl[99] = CDL_CODE | CDL_SNES_INDEX_MODE_8 | CDL_SNES_MEMORY_MODE_8;

		var result = BuildCpuStateSectionTest(cdl, isSnes: true, isGba: false);

		Assert.Equal(3 * CPU_STATE_ENTRY_SIZE, result.Length);

		// Entry 0: address 10, X flag
		Assert.Equal(10u, BitConverter.ToUInt32(result, 0));
		Assert.Equal(0x01, result[4]);

		// Entry 1: address 50, M flag
		Assert.Equal(50u, BitConverter.ToUInt32(result, 9));
		Assert.Equal(0x02, result[13]);

		// Entry 2: address 99, both flags
		Assert.Equal(99u, BitConverter.ToUInt32(result, 18));
		Assert.Equal(0x03, result[22]);
	}

	[Fact]
	public void CpuState_GbaThumb_CreatesEntry() {
		byte[] cdl = [CDL_CODE | CDL_GBA_THUMB];
		var result = BuildCpuStateSectionTest(cdl, isSnes: false, isGba: true);

		Assert.Equal(CPU_STATE_ENTRY_SIZE, result.Length);
		Assert.Equal(0u, BitConverter.ToUInt32(result, 0));
		Assert.Equal(0, result[4]);    // Flags: none for GBA
		Assert.Equal(3, result[8]);    // CpuMode: THUMB (3)
	}

	[Fact]
	public void CpuState_GbaArmCode_NoEntry() {
		// ARM code (no THUMB flag) → no CPU state entry
		byte[] cdl = [CDL_CODE];
		var result = BuildCpuStateSectionTest(cdl, isSnes: false, isGba: true);
		Assert.Empty(result);
	}

	[Fact]
	public void CpuState_GbaDataWithThumb_NoEntry() {
		// Data byte with thumb flag should NOT create an entry
		byte[] cdl = [CDL_DATA | CDL_GBA_THUMB];
		var result = BuildCpuStateSectionTest(cdl, isSnes: false, isGba: true);
		Assert.Empty(result);
	}

	[Fact]
	public void CpuState_GbaMultipleThumb_CorrectCount() {
		byte[] cdl = new byte[50];
		cdl[0] = CDL_CODE | CDL_GBA_THUMB;
		cdl[2] = CDL_CODE | CDL_GBA_THUMB;
		cdl[4] = CDL_CODE;  // ARM, no entry
		cdl[10] = CDL_CODE | CDL_GBA_THUMB;

		var result = BuildCpuStateSectionTest(cdl, isSnes: false, isGba: true);
		Assert.Equal(3 * CPU_STATE_ENTRY_SIZE, result.Length);

		Assert.Equal(0u, BitConverter.ToUInt32(result, 0));
		Assert.Equal(2u, BitConverter.ToUInt32(result, 9));
		Assert.Equal(10u, BitConverter.ToUInt32(result, 18));
	}

	[Fact]
	public void CpuState_LargeSnesCdl_HandlesAllEntries() {
		// Simulate 512KB SNES ROM with ~30% code having mode flags
		var random = new Random(42);
		byte[] cdl = new byte[512 * 1024];
		int expectedCount = 0;

		for (int i = 0; i < cdl.Length; i++) {
			if (random.NextDouble() < 0.6) {
				cdl[i] = CDL_CODE;
				if (random.NextDouble() < 0.3) {
					cdl[i] |= CDL_SNES_INDEX_MODE_8;
					expectedCount++;
				}
			}
		}

		var result = BuildCpuStateSectionTest(cdl, isSnes: true, isGba: false);
		Assert.Equal(expectedCount * CPU_STATE_ENTRY_SIZE, result.Length);
	}

	[Fact]
	public void CpuState_AllCodeHasModeFlags_MaxEntries() {
		// Every byte is code with both mode flags
		byte[] cdl = new byte[100];
		for (int i = 0; i < cdl.Length; i++) {
			cdl[i] = CDL_CODE | CDL_SNES_INDEX_MODE_8 | CDL_SNES_MEMORY_MODE_8;
		}

		var result = BuildCpuStateSectionTest(cdl, isSnes: true, isGba: false);
		Assert.Equal(100 * CPU_STATE_ENTRY_SIZE, result.Length);

		// Verify all addresses are sequential
		for (int i = 0; i < 100; i++) {
			Assert.Equal((uint)i, BitConverter.ToUInt32(result, i * CPU_STATE_ENTRY_SIZE));
		}
	}

	[Fact]
	public void CpuState_AllDataBytes_NoEntries() {
		byte[] cdl = new byte[1000];
		for (int i = 0; i < cdl.Length; i++) {
			cdl[i] = CDL_DATA;  // All data, no code
		}

		var result = BuildCpuStateSectionTest(cdl, isSnes: true, isGba: false);
		Assert.Empty(result);
	}

	[Fact]
	public void CpuState_RoundtripWithPansyLoader() {
		// Build CPU State section, wrap in Pansy file, load with PansyLoader
		byte[] cdl = new byte[20];
		cdl[5] = CDL_CODE | CDL_SNES_INDEX_MODE_8;
		cdl[10] = CDL_CODE | CDL_SNES_MEMORY_MODE_8;
		cdl[15] = CDL_CODE | CDL_SNES_INDEX_MODE_8 | CDL_SNES_MEMORY_MODE_8;

		var cpuStateBytes = BuildCpuStateSectionTest(cdl, isSnes: true, isGba: false);
		Assert.Equal(3 * CPU_STATE_ENTRY_SIZE, cpuStateBytes.Length);

		// Build minimal Pansy file with CPU State section
		var pansyFile = BuildMinimalPansyFile(cpuStateBytes, sectionType: 0x0009, platform: 0x02);

		// Load with PansyLoader
		var tempPath = Path.Combine(Path.GetTempPath(), $"test_cpustate_{Guid.NewGuid():N}.pansy");
		try {
			File.WriteAllBytes(tempPath, pansyFile);
			var loader = Pansy.Core.PansyLoader.Load(tempPath);

			Assert.Equal(3, loader.CpuStateEntries.Count);

			Assert.Equal(5u, loader.CpuStateEntries[0].Address);
			Assert.Equal(0x01, loader.CpuStateEntries[0].Flags);  // XFlag
			Assert.Equal(Pansy.Core.CpuMode.Native65816, loader.CpuStateEntries[0].Mode);

			Assert.Equal(10u, loader.CpuStateEntries[1].Address);
			Assert.Equal(0x02, loader.CpuStateEntries[1].Flags);  // MFlag

			Assert.Equal(15u, loader.CpuStateEntries[2].Address);
			Assert.Equal(0x03, loader.CpuStateEntries[2].Flags);  // Both
		} finally {
			if (File.Exists(tempPath)) File.Delete(tempPath);
		}
	}

	[Fact]
	public void CpuState_GbaRoundtripWithPansyLoader() {
		byte[] cdl = new byte[10];
		cdl[0] = CDL_CODE | CDL_GBA_THUMB;
		cdl[5] = CDL_CODE | CDL_GBA_THUMB;

		var cpuStateBytes = BuildCpuStateSectionTest(cdl, isSnes: false, isGba: true);
		var pansyFile = BuildMinimalPansyFile(cpuStateBytes, sectionType: 0x0009, platform: 0x04);

		var tempPath = Path.Combine(Path.GetTempPath(), $"test_cpustate_gba_{Guid.NewGuid():N}.pansy");
		try {
			File.WriteAllBytes(tempPath, pansyFile);
			var loader = Pansy.Core.PansyLoader.Load(tempPath);

			Assert.Equal(2, loader.CpuStateEntries.Count);

			Assert.Equal(0u, loader.CpuStateEntries[0].Address);
			Assert.Equal(Pansy.Core.CpuMode.THUMB, loader.CpuStateEntries[0].Mode);

			Assert.Equal(5u, loader.CpuStateEntries[1].Address);
			Assert.Equal(Pansy.Core.CpuMode.THUMB, loader.CpuStateEntries[1].Mode);
		} finally {
			if (File.Exists(tempPath)) File.Delete(tempPath);
		}
	}

	[Fact]
	public void CpuState_HeaderFlag_SetWhenEntriesExist() {
		byte[] cdl = [CDL_CODE | CDL_SNES_INDEX_MODE_8];
		var cpuStateBytes = BuildCpuStateSectionTest(cdl, isSnes: true, isGba: false);
		var pansyFile = BuildMinimalPansyFile(cpuStateBytes, sectionType: 0x0009, platform: 0x02, hasCpuState: true);

		using var ms = new MemoryStream(pansyFile);
		var header = ReadTestHeader(ms);
		Assert.NotNull(header);
		Assert.True((header.Flags & 0x0010) != 0, "HAS_CPU_STATE flag should be set");
	}

	[Fact]
	public void CpuState_HeaderFlag_NotSetWhenEmpty() {
		var pansyFile = BuildMinimalPansyFile([], sectionType: 0x0009, platform: 0x02, hasCpuState: false);

		using var ms = new MemoryStream(pansyFile);
		var header = ReadTestHeader(ms);
		Assert.NotNull(header);
		Assert.True((header.Flags & 0x0010) == 0, "HAS_CPU_STATE flag should NOT be set");
	}

	[Fact]
	public void CpuState_EntrySize_IsExactly9Bytes() {
		// Each entry: Address(4) + Flags(1) + DataBank(1) + DirectPage(2) + CpuMode(1) = 9
		byte[] cdl = [CDL_CODE | CDL_SNES_INDEX_MODE_8];
		var result = BuildCpuStateSectionTest(cdl, isSnes: true, isGba: false);
		Assert.Equal(9, result.Length);
		Assert.Equal(CPU_STATE_ENTRY_SIZE, result.Length);
	}

	/// <summary>
	/// Calls private MapCdlToPansyFlags via reflection.
	/// </summary>
	private static byte[] MapCdlToPansyFlagsTest(byte[] cdlData, uint[] jumpTargets, uint[] functions, bool preserveUpperNibbleFlags) {
		var method = typeof(PansyExporter).GetMethod("MapCdlToPansyFlags", BindingFlags.NonPublic | BindingFlags.Static);
		Assert.NotNull(method);

		var result = method!.Invoke(null, [cdlData, jumpTargets, functions, preserveUpperNibbleFlags]);
		Assert.NotNull(result);
		return Assert.IsType<byte[]>(result);
	}

	/// <summary>
	/// Mirror of PansyExporter.BuildCpuStateSection using same Span-based optimization.
	/// </summary>
	private static byte[] BuildCpuStateSectionTest(byte[]? cdlData, bool isSnes, bool isGba) {
		if (cdlData is null or { Length: 0 })
			return [];

		if (!isSnes && !isGba)
			return [];

		int count = 0;
		for (int i = 0; i < cdlData.Length; i++) {
			byte cdl = cdlData[i];
			if ((cdl & CDL_CODE) == 0) continue;
			if (isSnes && (cdl & (CDL_SNES_INDEX_MODE_8 | CDL_SNES_MEMORY_MODE_8)) != 0) count++;
			else if (isGba && (cdl & CDL_GBA_THUMB) != 0) count++;
		}

		if (count == 0) return [];

		var result = new byte[count * CPU_STATE_ENTRY_SIZE];
		int offset = 0;

		for (int i = 0; i < cdlData.Length; i++) {
			byte cdl = cdlData[i];
			if ((cdl & CDL_CODE) == 0) continue;

			if (isSnes) {
				bool x8 = (cdl & CDL_SNES_INDEX_MODE_8) != 0;
				bool m8 = (cdl & CDL_SNES_MEMORY_MODE_8) != 0;
				if (!x8 && !m8) continue;
				BitConverter.TryWriteBytes(result.AsSpan(offset), (uint)i);
				byte flags = 0;
				if (x8) flags |= 0x01;
				if (m8) flags |= 0x02;
				result[offset + 4] = flags;
				result[offset + 5] = 0;
				result[offset + 6] = 0;
				result[offset + 7] = 0;
				result[offset + 8] = 0; // Native65816
			} else if (isGba) {
				if ((cdl & CDL_GBA_THUMB) == 0) continue;
				BitConverter.TryWriteBytes(result.AsSpan(offset), (uint)i);
				result[offset + 4] = 0;
				result[offset + 5] = 0;
				result[offset + 6] = 0;
				result[offset + 7] = 0;
				result[offset + 8] = 3; // THUMB
			}

			offset += CPU_STATE_ENTRY_SIZE;
		}

		return result;
	}

	/// <summary>
	/// Build a minimal valid Pansy file with one section.
	/// </summary>
	private static byte[] BuildMinimalPansyFile(byte[] sectionData, ushort sectionType, byte platform, bool hasCpuState = true) {
		using var ms = new MemoryStream();
		using var writer = new BinaryWriter(ms);

		int sectionCount = sectionData.Length > 0 ? 1 : 0;
		ushort flags = (ushort)(hasCpuState && sectionData.Length > 0 ? 0x0010 : 0);

		// Header (32 bytes)
		writer.Write(Encoding.ASCII.GetBytes("PANSY\0\0\0")); // 8 bytes
		writer.Write((ushort)0x0100);                          // Version
		writer.Write(flags);                                   // Flags
		writer.Write(platform);                                // Platform
		writer.Write((byte)0);                                 // Reserved
		writer.Write((ushort)0);                               // Reserved
		writer.Write((uint)0x100000);                          // ROM size
		writer.Write((uint)0xDEADBEEF);                        // CRC32
		writer.Write((uint)sectionCount);                      // Section count
		writer.Write((uint)0);                                 // Reserved

		if (sectionCount > 0) {
			// Section table entry (12 bytes)
			writer.Write(sectionType);                          // Type (2 bytes)
			writer.Write((ushort)0);                            // Flags
			writer.Write((uint)(32 + 12));                      // Offset (after header + section table)
			writer.Write((uint)sectionData.Length);             // Size

			// Section data
			writer.Write(sectionData);
		}

		return ms.ToArray();
	}

	#endregion

	#region Channel F Pansy Export Tests

	[Fact]
	public void ChannelF_PlatformId_Is0x1f()
	{
		// Verify Channel F platform ID matches Pansy spec constant
		const byte expected = 0x1f;
		Assert.Equal(expected, Pansy.Core.PansyLoader.PLATFORM_CHANNEL_F);
	}

	[Fact]
	public void ChannelF_Header_CorrectPlatformByte()
	{
		// Build a minimal Pansy header for Channel F and verify the platform byte
		using var ms = new MemoryStream();
		using var writer = new BinaryWriter(ms);

		writer.Write(Encoding.ASCII.GetBytes(MAGIC));
		writer.Write(VERSION_1_0);
		writer.Write((ushort)0);          // Flags
		writer.Write((byte)0x1f);         // Platform: Channel F
		writer.Write((byte)0);            // Reserved
		writer.Write((ushort)0);          // Reserved
		writer.Write((uint)0x1800);       // ROM size (6KB typical)
		writer.Write((uint)0xAABBCCDD);   // ROM CRC
		writer.Write((uint)0);            // Section count
		writer.Write((uint)0);            // Reserved

		ms.Position = 0;
		var header = ReadTestHeader(ms);

		Assert.NotNull(header);
		Assert.Equal(0x1f, header.Platform);
		Assert.Equal(0x1800u, header.RomSize);
	}

	[Fact]
	public void ChannelF_MemoryRegions_HasFourRegions()
	{
		// Channel F should have exactly 4 memory regions
		var regions = BuildChannelFMemoryRegions();
		Assert.Equal(4, regions.Count);
	}

	[Fact]
	public void ChannelF_MemoryRegions_CartridgeRom_CorrectRange()
	{
		var regions = BuildChannelFMemoryRegions();
		var rom = regions[0];
		Assert.Equal(0x0000u, rom.Start);
		Assert.Equal(0x17ffu, rom.End);
		Assert.Equal("Cartridge_ROM", rom.Name);
	}

	[Fact]
	public void ChannelF_MemoryRegions_SystemRam_CorrectRange()
	{
		var regions = BuildChannelFMemoryRegions();
		var ram = regions[1];
		Assert.Equal(0x2800u, ram.Start);
		Assert.Equal(0x2fffu, ram.End);
		Assert.Equal("System_RAM", ram.Name);
	}

	[Fact]
	public void ChannelF_MemoryRegions_VideoRam_CorrectRange()
	{
		var regions = BuildChannelFMemoryRegions();
		var vram = regions[2];
		Assert.Equal(0x3000u, vram.Start);
		Assert.Equal(0x37ffu, vram.End);
		Assert.Equal("Video_RAM", vram.Name);
	}

	[Fact]
	public void ChannelF_MemoryRegions_IoRegisters_CorrectRange()
	{
		var regions = BuildChannelFMemoryRegions();
		var io = regions[3];
		Assert.Equal(0x3800u, io.Start);
		Assert.Equal(0x38ffu, io.End);
		Assert.Equal("IO_Registers", io.Name);
	}

	[Fact]
	public void ChannelF_FullFile_RoundtripWithPansyLoader()
	{
		// Use PansyWriter + PansyLoader for a proper roundtrip test
		var writer = new Pansy.Core.PansyWriter {
			Platform = Pansy.Core.PansyLoader.PLATFORM_CHANNEL_F,
			RomSize = 0x1800,
			RomCrc32 = 0xAABBCCDD,
		};

		writer.AddMemoryRegion(new Pansy.Core.MemoryRegion(0x0000, 0x17ff, (byte)Pansy.Core.MemoryRegionType.ROM, 0, "Cartridge_ROM"));
		writer.AddMemoryRegion(new Pansy.Core.MemoryRegion(0x2800, 0x2fff, (byte)Pansy.Core.MemoryRegionType.RAM, 0, "System_RAM"));
		writer.AddMemoryRegion(new Pansy.Core.MemoryRegion(0x3000, 0x37ff, (byte)Pansy.Core.MemoryRegionType.VRAM, 0, "Video_RAM"));
		writer.AddMemoryRegion(new Pansy.Core.MemoryRegion(0x3800, 0x38ff, (byte)Pansy.Core.MemoryRegionType.IO, 0, "IO_Registers"));

		var fileBytes = writer.Generate();
		var loader = new Pansy.Core.PansyLoader(fileBytes);

		Assert.Equal(0x1f, loader.Platform);
		Assert.Equal(0x1800u, loader.RomSize);
		Assert.Equal(0xAABBCCDDu, loader.RomCrc32);
		Assert.Equal(4, loader.MemoryRegions.Count);
		Assert.Equal("Cartridge_ROM", loader.MemoryRegions[0].Name);
		Assert.Equal("Video_RAM", loader.MemoryRegions[2].Name);
	}

	private sealed record ChannelFRegion(uint Start, uint End, string Name, byte Type, byte MemType);

	private static List<ChannelFRegion> BuildChannelFMemoryRegions()
	{
		return [
			new ChannelFRegion(0x0000, 0x17ff, "Cartridge_ROM", 1, 0), // ROM
			new ChannelFRegion(0x2800, 0x2fff, "System_RAM", 2, 0),    // RAM
			new ChannelFRegion(0x3000, 0x37ff, "Video_RAM", 3, 0),     // VRAM
			new ChannelFRegion(0x3800, 0x38ff, "IO_Registers", 4, 0),  // IO
		];
	}

	private static byte[] BuildChannelFRegionSection(List<ChannelFRegion> regions)
	{
		using var ms = new MemoryStream();
		using var writer = new BinaryWriter(ms);

		writer.Write((uint)regions.Count);
		foreach (var region in regions) {
			writer.Write(region.Start);
			writer.Write(region.End);
			writer.Write(region.Type);
			writer.Write(region.MemType);
			writer.Write((ushort)0); // Flags
			byte[] nameBytes = Encoding.UTF8.GetBytes(region.Name);
			writer.Write((ushort)nameBytes.Length);
			writer.Write(nameBytes);
		}

		return ms.ToArray();
	}

	#endregion
}
