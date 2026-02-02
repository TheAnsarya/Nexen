using System.IO.Compression;
using System.Text;

namespace Nexen.MovieConverter.Tests;

/// <summary>
/// Tests for individual movie format converters
/// </summary>
public class ConverterTests {
	[Fact]
	public void NexenConverter_ReadWrite_RoundTrips() {
		// Create a test movie
		var original = new MovieData {
			Author = "Test Author",
			Description = "Test Description",
			GameName = "Test Game",
			RomFileName = "test.sfc",
			SystemType = SystemType.Snes,
			Region = RegionType.NTSC,
			RerecordCount = 1234
		};

		// Add some frames
		for (int i = 0; i < 100; i++) {
			var frame = new InputFrame(i);
			frame.Controllers[0].A = i % 2 == 0;
			frame.Controllers[0].B = i % 3 == 0;
			original.AddFrame(frame);
		}

		// Write to memory stream
		using var stream = new MemoryStream();
		var converter = new Converters.NexenMovieConverter();
		converter.Write(original, stream);

		// Reset stream and read back
		stream.Position = 0;
		var loaded = converter.Read(stream, "test.nexen-movie");

		// Verify metadata
		Assert.Equal(original.Author, loaded.Author);
		Assert.Equal(original.Description, loaded.Description);
		Assert.Equal(original.GameName, loaded.GameName);
		Assert.Equal(original.RomFileName, loaded.RomFileName);
		Assert.Equal(original.SystemType, loaded.SystemType);
		Assert.Equal(original.Region, loaded.Region);
		Assert.Equal(original.RerecordCount, loaded.RerecordCount);

		// Verify frames
		Assert.Equal(original.TotalFrames, loaded.TotalFrames);
		for (int i = 0; i < original.TotalFrames; i++) {
			Assert.Equal(original.InputFrames[i].Controllers[0].A, loaded.InputFrames[i].Controllers[0].A);
			Assert.Equal(original.InputFrames[i].Controllers[0].B, loaded.InputFrames[i].Controllers[0].B);
		}
	}

	[Fact]
	public void NexenConverter_HandlesMarkers() {
		var movie = new MovieData();
		movie.AddMarker(100, "Start", "Beginning of run");
		movie.AddMarker(500, "Boss", "Boss fight");

		using var stream = new MemoryStream();
		var converter = new Converters.NexenMovieConverter();
		converter.Write(movie, stream);

		stream.Position = 0;
		var loaded = converter.Read(stream);

		Assert.NotNull(loaded.Markers);
		Assert.Equal(2, loaded.Markers.Count);
		Assert.Equal(100, loaded.Markers[0].FrameNumber);
		Assert.Equal("Start", loaded.Markers[0].Label);
	}

	[Fact]
	public void NexenConverter_HandlesSavestate() {
		var movie = new MovieData {
			StartType = StartType.Savestate,
			InitialState = new byte[] { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 }
		};

		using var stream = new MemoryStream();
		var converter = new Converters.NexenMovieConverter();
		converter.Write(movie, stream);

		stream.Position = 0;
		var loaded = converter.Read(stream);

		Assert.Equal(StartType.Savestate, loaded.StartType);
		Assert.NotNull(loaded.InitialState);
		Assert.Equal(movie.InitialState, loaded.InitialState);
	}

	[Fact]
	public void Fm2Converter_ParsesHeader() {
		// Create a minimal FM2 file content
		string fm2Content = """
			version 3
			emuVersion 25000
			rerecordCount 1000
			palFlag 0
			romFilename Test ROM.nes
			romChecksum base64:AAAAAAAAAAAAAAAAAAAAAA==
			guid 12345678-1234-1234-1234-123456789012
			fourscore 0
			microphone 0
			port0 1
			port1 1
			port2 0
			FDS 0
			NewPPU 0
			|0|........|........||
			|0|A.......|........||
			|0|.B......|........||
			""";

		using var stream = new MemoryStream(Encoding.UTF8.GetBytes(fm2Content));
		var converter = new Converters.Fm2MovieConverter();
		var movie = converter.Read(stream, "test.fm2");

		Assert.Equal(MovieFormat.Fm2, movie.SourceFormat);
		Assert.Equal(SystemType.Nes, movie.SystemType);
		Assert.Equal((ulong)1000, movie.RerecordCount);
		Assert.Equal(RegionType.NTSC, movie.Region);
		Assert.Equal(3, movie.TotalFrames);
	}

	[Fact]
	public void Bk2Converter_ParsesHeader() {
		// Create a minimal BK2 archive in memory
		using var stream = new MemoryStream();
		using (var archive = new ZipArchive(stream, ZipArchiveMode.Create, leaveOpen: true)) {
			// Add Header.txt
			var headerEntry = archive.CreateEntry("Header.txt");
			using (var writer = new StreamWriter(headerEntry.Open())) {
				writer.WriteLine("Author Test Author");
				writer.WriteLine("GameName Test Game");
				writer.WriteLine("Platform SNES");
				writer.WriteLine("rerecordCount 5000");
			}

			// Add Input Log.txt
			var inputEntry = archive.CreateEntry("Input Log.txt");
			using (var writer = new StreamWriter(inputEntry.Open())) {
				writer.WriteLine("[Input]");
				writer.WriteLine("|...............|");
				writer.WriteLine("|B..............|");
			}
		}

		stream.Position = 0;
		var converter = new Converters.Bk2MovieConverter();
		var movie = converter.Read(stream, "test.bk2");

		Assert.Equal(MovieFormat.Bk2, movie.SourceFormat);
		Assert.Equal(SystemType.Snes, movie.SystemType);
		Assert.Equal((ulong)5000, movie.RerecordCount);
	}

	[Fact]
	public void AllConverters_HaveUniqueExtensions() {
		var converters = MovieConverterRegistry.Converters;
		var allExtensions = converters.SelectMany(c => c.Extensions).ToList();

		// Check for duplicates
		var duplicates = allExtensions.GroupBy(e => e.ToLowerInvariant())
			.Where(g => g.Count() > 1)
			.Select(g => g.Key);

		Assert.Empty(duplicates);
	}

	[Fact]
	public void AllConverters_HaveUniqueFormats() {
		var converters = MovieConverterRegistry.Converters;
		var formats = converters.Select(c => c.Format).ToList();

		var duplicates = formats.GroupBy(f => f)
			.Where(g => g.Count() > 1)
			.Select(g => g.Key);

		Assert.Empty(duplicates);
	}

	[Theory]
	[InlineData(typeof(Converters.NexenMovieConverter), true, true)]
	[InlineData(typeof(Converters.Bk2MovieConverter), true, true)]
	[InlineData(typeof(Converters.Fm2MovieConverter), true, true)]
	[InlineData(typeof(Converters.LsmvMovieConverter), true, true)]
	[InlineData(typeof(Converters.SmvMovieConverter), true, true)]
	[InlineData(typeof(Converters.VbmMovieConverter), true, false)]
	[InlineData(typeof(Converters.GmvMovieConverter), true, false)]
	public void Converter_HasExpectedCapabilities(Type converterType, bool canRead, bool canWrite) {
		var converter = (IMovieConverter)Activator.CreateInstance(converterType)!;

		Assert.Equal(canRead, converter.CanRead);
		Assert.Equal(canWrite, converter.CanWrite);
	}
}
