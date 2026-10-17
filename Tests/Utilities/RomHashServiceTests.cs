using System.Text;
using Nexen.Utilities;
using Xunit;

namespace Nexen.Tests.Utilities;

/// <summary>
/// Tests for <see cref="RomHashService"/> and <see cref="RomHashInfo"/>.
/// Validates hash computation correctness against known test vectors and
/// verifies the CRC32 endianness correction.
/// </summary>
public class RomHashServiceTests {
	#region ComputeBasicHashes Tests

	[Fact]
	public void ComputeBasicHashes_NullData_ReturnsEmpty() {
		var result = RomHashService.ComputeBasicHashes(null!);

		Assert.Same(RomHashInfo.Empty, result);
	}

	[Fact]
	public void ComputeBasicHashes_EmptyArray_ReturnsEmpty() {
		var result = RomHashService.ComputeBasicHashes([]);

		Assert.Same(RomHashInfo.Empty, result);
	}

	[Fact]
	public void ComputeBasicHashes_KnownData_ReturnsCrc32() {
		// "123456789" has known CRC32 = 0xCBF43926
		byte[] data = Encoding.ASCII.GetBytes("123456789");

		var result = RomHashService.ComputeBasicHashes(data);

		Assert.Equal("CBF43926", result.Crc32);
	}

	[Fact]
	public void ComputeBasicHashes_KnownData_ReturnsMd5() {
		// "123456789" MD5 = 25F9E794323B453885F5181F1B624D0B
		byte[] data = Encoding.ASCII.GetBytes("123456789");

		var result = RomHashService.ComputeBasicHashes(data);

		Assert.Equal("25F9E794323B453885F5181F1B624D0B", result.Md5);
	}

	[Fact]
	public void ComputeBasicHashes_KnownData_ReturnsSha1() {
		// "123456789" SHA-1 = F7C3BC1D808E04732ADF679965CCC34CA7AE3441
		byte[] data = Encoding.ASCII.GetBytes("123456789");

		var result = RomHashService.ComputeBasicHashes(data);

		Assert.Equal("F7C3BC1D808E04732ADF679965CCC34CA7AE3441", result.Sha1);
	}

	[Fact]
	public void ComputeBasicHashes_KnownData_ReturnsSha256() {
		// "123456789" SHA-256 = 15E2B0D3C33891EBB0F1EF609EC419420C20E320CE94C65FBC8C3312448EB225
		byte[] data = Encoding.ASCII.GetBytes("123456789");

		var result = RomHashService.ComputeBasicHashes(data);

		Assert.Equal("15E2B0D3C33891EBB0F1EF609EC419420C20E320CE94C65FBC8C3312448EB225", result.Sha256);
	}

	[Fact]
	public void ComputeBasicHashes_SingleByte_ProducesValidHashes() {
		byte[] data = [0x42];

		var result = RomHashService.ComputeBasicHashes(data);

		Assert.True(result.IsValid);
		Assert.Equal(8, result.Crc32.Length);
		Assert.Equal(32, result.Md5.Length);
		Assert.Equal(40, result.Sha1.Length);
		Assert.Equal(64, result.Sha256.Length);
	}

	[Fact]
	public void ComputeBasicHashes_DifferentData_ProducesDifferentHashes() {
		byte[] data1 = Encoding.ASCII.GetBytes("Hello");
		byte[] data2 = Encoding.ASCII.GetBytes("World");

		var result1 = RomHashService.ComputeBasicHashes(data1);
		var result2 = RomHashService.ComputeBasicHashes(data2);

		Assert.NotEqual(result1.Crc32, result2.Crc32);
		Assert.NotEqual(result1.Md5, result2.Md5);
		Assert.NotEqual(result1.Sha1, result2.Sha1);
		Assert.NotEqual(result1.Sha256, result2.Sha256);
	}

	[Fact]
	public void ComputeBasicHashes_SameDataTwice_ProducesIdenticalHashes() {
		byte[] data = Encoding.ASCII.GetBytes("deterministic");

		var result1 = RomHashService.ComputeBasicHashes(data);
		var result2 = RomHashService.ComputeBasicHashes(data);

		Assert.Equal(result1.Crc32, result2.Crc32);
		Assert.Equal(result1.Md5, result2.Md5);
		Assert.Equal(result1.Sha1, result2.Sha1);
		Assert.Equal(result1.Sha256, result2.Sha256);
	}

	[Fact]
	public void ComputeBasicHashes_AllHashesAreUppercase() {
		byte[] data = Encoding.ASCII.GetBytes("test uppercase");

		var result = RomHashService.ComputeBasicHashes(data);

		Assert.Equal(result.Crc32, result.Crc32.ToUpperInvariant());
		Assert.Equal(result.Md5, result.Md5.ToUpperInvariant());
		Assert.Equal(result.Sha1, result.Sha1.ToUpperInvariant());
		Assert.Equal(result.Sha256, result.Sha256.ToUpperInvariant());
	}

	#endregion

	#region RomHashInfo Record Tests

	[Fact]
	public void RomHashInfo_Empty_HasEmptyStrings() {
		var empty = RomHashInfo.Empty;

		Assert.Equal("", empty.Crc32);
		Assert.Equal("", empty.Md5);
		Assert.Equal("", empty.Sha1);
		Assert.Equal("", empty.Sha256);
	}

	[Fact]
	public void RomHashInfo_Empty_IsNotValid() {
		Assert.False(RomHashInfo.Empty.IsValid);
	}

	[Fact]
	public void RomHashInfo_WithCrc32_IsValid() {
		var info = new RomHashInfo("DEADBEEF", "", "", "");

		Assert.True(info.IsValid);
	}

	[Fact]
	public void RomHashInfo_Crc32Value_ParsesHexCorrectly() {
		var info = new RomHashInfo("CBF43926", "", "", "");

		Assert.Equal(0xCBF43926u, info.Crc32Value);
	}

	[Fact]
	public void RomHashInfo_Crc32Value_ZeroValue() {
		var info = new RomHashInfo("00000000", "", "", "");

		Assert.Equal(0u, info.Crc32Value);
	}

	[Fact]
	public void RomHashInfo_Crc32Value_MaxValue() {
		var info = new RomHashInfo("FFFFFFFF", "", "", "");

		Assert.Equal(0xFFFFFFFFu, info.Crc32Value);
	}

	[Fact]
	public void RomHashInfo_RecordEquality_SameValues() {
		var a = new RomHashInfo("AA", "BB", "CC", "DD");
		var b = new RomHashInfo("AA", "BB", "CC", "DD");

		Assert.Equal(a, b);
	}

	[Fact]
	public void RomHashInfo_RecordEquality_DifferentValues() {
		var a = new RomHashInfo("AA", "BB", "CC", "DD");
		var b = new RomHashInfo("XX", "BB", "CC", "DD");

		Assert.NotEqual(a, b);
	}

	#endregion

	#region InvalidateCache Tests

	[Fact]
	public void InvalidateCache_DoesNotThrow() {
		// Ensure InvalidateCache is safe to call even when there's nothing cached
		RomHashService.InvalidateCache();
		RomHashService.InvalidateCache();
	}

	#endregion
}
