using System.Buffers.Binary;
using System.IO.Compression;
using System.Text;

namespace Nexen.MovieConverter.Tests;

/// <summary>
/// Tests for converter error handling: corrupt data, truncated files, invalid headers.
/// </summary>
public class ErrorHandlingTests {
	#region SMV Error Handling

	[Fact]
	public void SmvConverter_EmptyStream_ThrowsException() {
		var converter = new Converters.SmvMovieConverter();
		using var stream = new MemoryStream([]);

		Assert.ThrowsAny<Exception>(() => converter.Read(stream, "test.smv"));
	}

	[Fact]
	public void SmvConverter_TruncatedHeader_ThrowsException() {
		var converter = new Converters.SmvMovieConverter();
		// Only 16 bytes instead of required 32
		using var stream = new MemoryStream(new byte[16]);

		Assert.ThrowsAny<Exception>(() => converter.Read(stream, "test.smv"));
	}

	[Fact]
	public void SmvConverter_InvalidSignature_ThrowsMovieFormatException() {
		var converter = new Converters.SmvMovieConverter();
		var data = new byte[32];
		// Wrong signature
		BinaryPrimitives.WriteUInt32LittleEndian(data.AsSpan(0), 0xdeadbeef);

		using var stream = new MemoryStream(data);

		Assert.Throws<MovieFormatException>(() => converter.Read(stream, "test.smv"));
	}

	[Fact]
	public void SmvConverter_ValidMinimalHeader_ReadsZeroFrames() {
		var converter = new Converters.SmvMovieConverter();
		var data = new byte[32];
		BinaryPrimitives.WriteUInt32LittleEndian(data.AsSpan(0), 0x1a564d53); // SMV\x1A
		BinaryPrimitives.WriteUInt32LittleEndian(data.AsSpan(4), 5);          // Version 5
		BinaryPrimitives.WriteUInt32LittleEndian(data.AsSpan(16), 0);         // 0 frames
		data[20] = 0x01; // 1 controller
		BinaryPrimitives.WriteUInt32LittleEndian(data.AsSpan(28), 32);        // Input at offset 32

		using var stream = new MemoryStream(data);
		var movie = converter.Read(stream, "test.smv");

		Assert.Equal(MovieFormat.Smv, movie.SourceFormat);
		Assert.Equal(0, movie.TotalFrames);
	}

	#endregion

	#region VBM Error Handling

	[Fact]
	public void VbmConverter_EmptyStream_ThrowsMovieFormatException() {
		var converter = new Converters.VbmMovieConverter();
		using var stream = new MemoryStream([]);

		Assert.Throws<MovieFormatException>(() => converter.Read(stream, "test.vbm"));
	}

	[Fact]
	public void VbmConverter_TooSmallForHeader_ThrowsMovieFormatException() {
		var converter = new Converters.VbmMovieConverter();
		// 32 bytes — less than 64-byte minimum
		using var stream = new MemoryStream(new byte[32]);

		Assert.Throws<MovieFormatException>(() => converter.Read(stream, "test.vbm"));
	}

	[Fact]
	public void VbmConverter_InvalidSignature_ThrowsMovieFormatException() {
		var converter = new Converters.VbmMovieConverter();
		var data = new byte[128];
		BinaryPrimitives.WriteUInt32LittleEndian(data.AsSpan(0), 0xdeadbeef);

		using var stream = new MemoryStream(data);

		Assert.Throws<MovieFormatException>(() => converter.Read(stream, "test.vbm"));
	}

	[Fact]
	public void VbmConverter_ValidMinimalHeader_ReadsZeroFrames() {
		var converter = new Converters.VbmMovieConverter();
		var data = new byte[128];
		BinaryPrimitives.WriteUInt32LittleEndian(data.AsSpan(0), 0x1a4d4256); // VBM\x1A
		BinaryPrimitives.WriteUInt32LittleEndian(data.AsSpan(12), 0);          // 0 frames
		data[21] = 0x01; // Controller 1 enabled
		data[22] = 0x01; // GBA
		BinaryPrimitives.WriteUInt32LittleEndian(data.AsSpan(28), 64);         // Input at offset 64

		using var stream = new MemoryStream(data);
		var movie = converter.Read(stream, "test.vbm");

		Assert.Equal(MovieFormat.Vbm, movie.SourceFormat);
		Assert.Equal(0, movie.TotalFrames);
	}

	#endregion

	#region GMV Error Handling

	[Fact]
	public void GmvConverter_EmptyStream_ThrowsException() {
		var converter = new Converters.GmvMovieConverter();
		using var stream = new MemoryStream([]);

		Assert.ThrowsAny<Exception>(() => converter.Read(stream, "test.gmv"));
	}

	[Fact]
	public void GmvConverter_InvalidSignature_ThrowsMovieFormatException() {
		var converter = new Converters.GmvMovieConverter();
		var data = new byte[64];
		Encoding.ASCII.GetBytes("Wrong Magic").CopyTo(data, 0);

		using var stream = new MemoryStream(data);

		Assert.Throws<MovieFormatException>(() => converter.Read(stream, "test.gmv"));
	}

	[Fact]
	public void GmvConverter_ValidMinimalHeader_ReadsZeroFrames() {
		var converter = new Converters.GmvMovieConverter();
		var data = new byte[64];
		Encoding.ASCII.GetBytes("Gens Movie ").CopyTo(data, 0);
		data[16] = 0x40; // 3-button flag
		data[17] = 0x01; // P1 enabled

		using var stream = new MemoryStream(data);
		var movie = converter.Read(stream, "test.gmv");

		Assert.Equal(MovieFormat.Gmv, movie.SourceFormat);
		Assert.Equal(SystemType.Genesis, movie.SystemType);
	}

	#endregion

	#region FM2 Error Handling

	[Fact]
	public void Fm2Converter_EmptyStream_ReadsEmptyMovie() {
		var converter = new Converters.Fm2MovieConverter();
		using var stream = new MemoryStream(Encoding.UTF8.GetBytes(""));

		var movie = converter.Read(stream, "test.fm2");

		Assert.Equal(MovieFormat.Fm2, movie.SourceFormat);
		Assert.Equal(0, movie.TotalFrames);
	}

	[Fact]
	public void Fm2Converter_HeaderOnly_NoFrames() {
		string fm2Content = """
			version 3
			emuVersion 25000
			romFilename TestRom.nes
			""";

		var converter = new Converters.Fm2MovieConverter();
		using var stream = new MemoryStream(Encoding.UTF8.GetBytes(fm2Content));
		var movie = converter.Read(stream, "test.fm2");

		Assert.Equal(0, movie.TotalFrames);
		Assert.Equal("TestRom.nes", movie.RomFileName);
	}

	[Fact]
	public void Fm2Converter_MalformedInputLine_Skips() {
		string fm2Content = """
			version 3
			|0|A.......|........||
			not a valid line
			|0|.B......|........||
			""";

		var converter = new Converters.Fm2MovieConverter();
		using var stream = new MemoryStream(Encoding.UTF8.GetBytes(fm2Content));
		var movie = converter.Read(stream, "test.fm2");

		// Should parse the valid lines (possibly skipping malformed)
		Assert.True(movie.TotalFrames >= 2);
	}

	#endregion

	#region BK2 Error Handling

	[Fact]
	public void Bk2Converter_EmptyStream_ThrowsException() {
		var converter = new Converters.Bk2MovieConverter();
		using var stream = new MemoryStream([]);

		Assert.ThrowsAny<Exception>(() => converter.Read(stream, "test.bk2"));
	}

	[Fact]
	public void Bk2Converter_InvalidZip_ThrowsException() {
		var converter = new Converters.Bk2MovieConverter();
		using var stream = new MemoryStream(Encoding.UTF8.GetBytes("not a zip file"));

		Assert.ThrowsAny<Exception>(() => converter.Read(stream, "test.bk2"));
	}

	[Fact]
	public void Bk2Converter_MissingHeaderEntry_ThrowsMovieFormatException() {
		using var zipStream = new MemoryStream();
		using (var archive = new ZipArchive(zipStream, ZipArchiveMode.Create, leaveOpen: true)) {
			// Only add Input Log, no Header.txt
			var entry = archive.CreateEntry("Input Log.txt");
			using var writer = new StreamWriter(entry.Open());
			writer.WriteLine("[Input]");
			writer.WriteLine("|...|");
		}

		zipStream.Position = 0;
		var converter = new Converters.Bk2MovieConverter();

		Assert.Throws<MovieFormatException>(() => converter.Read(zipStream, "test.bk2"));
	}

	[Fact]
	public void Bk2Converter_MissingInputLogEntry_ThrowsMovieFormatException() {
		using var zipStream = new MemoryStream();
		using (var archive = new ZipArchive(zipStream, ZipArchiveMode.Create, leaveOpen: true)) {
			// Only add Header, no Input Log.txt
			var entry = archive.CreateEntry("Header.txt");
			using var writer = new StreamWriter(entry.Open());
			writer.WriteLine("Platform NES");
		}

		zipStream.Position = 0;
		var converter = new Converters.Bk2MovieConverter();

		Assert.Throws<MovieFormatException>(() => converter.Read(zipStream, "test.bk2"));
	}

	#endregion

	#region LSMV Error Handling

	[Fact]
	public void LsmvConverter_EmptyStream_ThrowsException() {
		var converter = new Converters.LsmvMovieConverter();
		using var stream = new MemoryStream([]);

		Assert.ThrowsAny<Exception>(() => converter.Read(stream, "test.lsmv"));
	}

	[Fact]
	public void LsmvConverter_InvalidZip_ThrowsException() {
		var converter = new Converters.LsmvMovieConverter();
		using var stream = new MemoryStream(Encoding.UTF8.GetBytes("not a zip file"));

		Assert.ThrowsAny<Exception>(() => converter.Read(stream, "test.lsmv"));
	}

	#endregion
}
