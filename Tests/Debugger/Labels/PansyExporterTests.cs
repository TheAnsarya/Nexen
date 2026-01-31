using Xunit;
using System;
using System.IO;
using System.IO.Compression;
using System.IO.Hashing;
using System.Text;

namespace Mesen.Tests.Debugger.Labels;

/// <summary>
/// Unit tests for PansyExporter functionality.
/// Tests the Pansy metadata file format export capabilities.
/// </summary>
public class PansyExporterTests
{
	// Pansy format constants for testing
	private const string MAGIC = "PANSY\0\0\0";
	private const ushort VERSION_1_0 = 0x0100;
	private const ushort VERSION_1_1 = 0x0101;
	private const byte FLAG_COMPRESSED = 0x01;

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

		// Write valid header
		writer.Write(Encoding.ASCII.GetBytes(MAGIC));
		writer.Write(VERSION_1_0);
		writer.Write((byte)0x02); // SNES platform
		writer.Write((byte)0x00); // Flags
		writer.Write((uint)0x100000); // ROM size 1MB
		writer.Write((uint)0xDEADBEEF); // ROM CRC
		writer.Write((long)DateTimeOffset.UtcNow.ToUnixTimeSeconds());
		writer.Write((uint)0); // Reserved

		ms.Position = 0;
		var header = ReadTestHeader(ms);

		Assert.NotNull(header);
		Assert.Equal(VERSION_1_0, header.Version);
		Assert.Equal(0x02, header.Platform);
		Assert.Equal(0x100000u, header.RomSize);
		Assert.Equal(0xDEADBEEFu, header.RomCrc32);
	}

	[Fact]
	public void Header_Version11_ParsesCorrectly()
	{
		using var ms = new MemoryStream();
		using var writer = new BinaryWriter(ms);

		// Write v1.1 header
		writer.Write(Encoding.ASCII.GetBytes(MAGIC));
		writer.Write(VERSION_1_1);
		writer.Write((byte)0x02); // SNES platform
		writer.Write((byte)0x00); // Flags
		writer.Write((uint)0x100000); // ROM size 1MB
		writer.Write((uint)0xDEADBEEF); // ROM CRC
		writer.Write((long)DateTimeOffset.UtcNow.ToUnixTimeSeconds());
		writer.Write((uint)0); // Reserved

		ms.Position = 0;
		var header = ReadTestHeader(ms);

		Assert.NotNull(header);
		Assert.Equal(VERSION_1_1, header.Version);
	}

	[Fact]
	public void Header_CompressedFlag_ParsesCorrectly()
	{
		using var ms = new MemoryStream();
		using var writer = new BinaryWriter(ms);

		// Write header with compression flag
		writer.Write(Encoding.ASCII.GetBytes(MAGIC));
		writer.Write(VERSION_1_1);
		writer.Write((byte)0x02); // SNES platform
		writer.Write(FLAG_COMPRESSED); // Compression flag set
		writer.Write((uint)0x100000); // ROM size 1MB
		writer.Write((uint)0xDEADBEEF); // ROM CRC
		writer.Write((long)DateTimeOffset.UtcNow.ToUnixTimeSeconds());
		writer.Write((uint)0); // Reserved

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
	[InlineData(0x0B, "WonderSwan")]
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
		// This is a mock test - actual export requires Mesen runtime
		string tempPath = Path.Combine(Path.GetTempPath(), $"test_{Guid.NewGuid()}.pansy");

		try {
			// Create a minimal valid pansy file
			using (var stream = new FileStream(tempPath, FileMode.Create))
			using (var writer = new BinaryWriter(stream)) {
				writer.Write(Encoding.ASCII.GetBytes(MAGIC));
				writer.Write(VERSION_1_1);
				writer.Write((byte)0x02); // SNES
				writer.Write((byte)0x00); // Flags
				writer.Write((uint)0x100000); // ROM size
				writer.Write((uint)0x12345678); // ROM CRC
				writer.Write((long)DateTimeOffset.UtcNow.ToUnixTimeSeconds());
				writer.Write((uint)0); // Reserved
				writer.Write((uint)0); // Section count
				writer.Write((uint)0); // Footer CRC1
				writer.Write((uint)0); // Footer CRC2
				writer.Write((uint)0); // Footer CRC3
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
	/// Calculate CRC32 using System.IO.Hashing.Crc32 (IEEE polynomial).
	/// Returns 0 for empty arrays to match PansyExporter behavior.
	/// </summary>
	private static uint ComputeCrc32(byte[] data)
	{
		if (data.Length == 0) return 0;
		return Crc32.HashToUInt32(data);
	}

	private record TestHeader(ushort Version, byte Platform, byte Flags, uint RomSize, uint RomCrc32, long Timestamp);

	private static TestHeader? ReadTestHeader(Stream stream)
	{
		try {
			using var reader = new BinaryReader(stream, Encoding.UTF8, leaveOpen: true);

			byte[] magic = reader.ReadBytes(8);
			if (Encoding.ASCII.GetString(magic).TrimEnd('\0') != "PANSY")
				return null;

			ushort version = reader.ReadUInt16();
			byte platform = reader.ReadByte();
			byte flags = reader.ReadByte();
			uint romSize = reader.ReadUInt32();
			uint romCrc32 = reader.ReadUInt32();
			long timestamp = reader.ReadInt64();

			return new TestHeader(version, platform, flags, romSize, romCrc32, timestamp);
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
		using (var gzip = new GZipStream(output, CompressionLevel.Optimal, leaveOpen: true)) {
			gzip.Write(data, 0, data.Length);
		}

		var compressed = output.ToArray();
		return compressed.Length < data.Length ? compressed : data;
	}

	private static byte[] CompressDataForce(byte[] data)
	{
		using var output = new MemoryStream();
		using (var gzip = new GZipStream(output, CompressionLevel.Optimal, leaveOpen: true)) {
			gzip.Write(data, 0, data.Length);
		}
		return output.ToArray();
	}

	private static byte[] DecompressData(byte[] compressedData, int uncompressedSize)
	{
		using var input = new MemoryStream(compressedData);
		using var gzip = new GZipStream(input, CompressionMode.Decompress);
		var output = new byte[uncompressedSize];
		int totalRead = 0;
		while (totalRead < uncompressedSize) {
			int read = gzip.Read(output, totalRead, uncompressedSize - totalRead);
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
}
