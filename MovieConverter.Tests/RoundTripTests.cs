using System.IO.Compression;
using System.Text;

namespace Nexen.MovieConverter.Tests;

/// <summary>
/// Round-trip tests for all writable converters.
/// Write→Read→Verify that metadata and input frames are preserved.
/// </summary>
public class RoundTripTests {
	/// <summary>Creates a test movie with rich metadata and input data.</summary>
	private static MovieData CreateTestMovie(SystemType system, int frameCount = 50) {
		var movie = new MovieData {
			Author = "RoundTrip Tester",
			Description = "Automated round-trip test",
			GameName = "Test Game",
			RomFileName = "test-rom.nes",
			SystemType = system,
			Region = RegionType.NTSC,
			RerecordCount = 42,
			ControllerCount = 2
		};

		for (int i = 0; i < frameCount; i++) {
			var frame = new InputFrame(i);
			frame.Controllers[0].A = i % 2 == 0;
			frame.Controllers[0].B = i % 3 == 0;
			frame.Controllers[0].Up = i % 5 == 0;
			frame.Controllers[0].Down = i % 7 == 0;
			frame.Controllers[0].Left = i % 11 == 0;
			frame.Controllers[0].Right = i % 13 == 0;
			frame.Controllers[0].Start = i == 0;
			frame.Controllers[0].Select = i == frameCount - 1;

			// SNES-specific buttons (X, Y, L, R)
			if (system is SystemType.Snes or SystemType.Gba) {
				frame.Controllers[0].X = i % 4 == 0;
				frame.Controllers[0].Y = i % 6 == 0;
				frame.Controllers[0].L = i % 9 == 0;
				frame.Controllers[0].R = i % 10 == 0;
			}

			// P2 input on some frames
			if (i % 4 == 0) {
				frame.Controllers[1].A = true;
				frame.Controllers[1].Up = true;
			}

			movie.AddFrame(frame);
		}

		return movie;
	}

	/// <summary>Verifies input frames match between original and loaded movies.</summary>
	private static void AssertFramesMatch(MovieData original, MovieData loaded, int controllerCount = 1) {
		Assert.Equal(original.TotalFrames, loaded.TotalFrames);

		for (int i = 0; i < original.TotalFrames; i++) {
			for (int c = 0; c < controllerCount; c++) {
				var origCtrl = original.InputFrames[i].Controllers[c];
				var loadCtrl = loaded.InputFrames[i].Controllers[c];
				Assert.Equal(origCtrl.ButtonBits, loadCtrl.ButtonBits);
			}
		}
	}

	#region Nexen Round-Trip

	[Fact]
	public void NexenConverter_RoundTrip_PreservesMetadata() {
		var movie = CreateTestMovie(SystemType.Snes);
		var converter = new Converters.NexenMovieConverter();

		using var stream = new MemoryStream();
		converter.Write(movie, stream);
		stream.Position = 0;
		var loaded = converter.Read(stream, "test.nexen-movie");

		Assert.Equal(movie.Author, loaded.Author);
		Assert.Equal(movie.Description, loaded.Description);
		Assert.Equal(movie.GameName, loaded.GameName);
		Assert.Equal(movie.SystemType, loaded.SystemType);
		Assert.Equal(movie.Region, loaded.Region);
		Assert.Equal(movie.RerecordCount, loaded.RerecordCount);
		AssertFramesMatch(movie, loaded, 2);
	}

	[Fact]
	public void NexenConverter_RoundTrip_ZeroFrames() {
		var movie = CreateTestMovie(SystemType.Nes, 0);
		var converter = new Converters.NexenMovieConverter();

		using var stream = new MemoryStream();
		converter.Write(movie, stream);
		stream.Position = 0;
		var loaded = converter.Read(stream, "test.nexen-movie");

		Assert.Equal(0, loaded.TotalFrames);
	}

	[Fact]
	public void NexenConverter_RoundTrip_LargeMovie() {
		var movie = CreateTestMovie(SystemType.Nes, 10000);
		var converter = new Converters.NexenMovieConverter();

		using var stream = new MemoryStream();
		converter.Write(movie, stream);
		stream.Position = 0;
		var loaded = converter.Read(stream, "test.nexen-movie");

		AssertFramesMatch(movie, loaded);
	}

	#endregion

	#region FM2 Round-Trip

	[Fact]
	public void Fm2Converter_RoundTrip_PreservesInputFrames() {
		var movie = CreateTestMovie(SystemType.Nes);
		var converter = new Converters.Fm2MovieConverter();

		using var stream = new MemoryStream();
		converter.Write(movie, stream);
		stream.Position = 0;
		var loaded = converter.Read(stream, "test.fm2");

		Assert.Equal(MovieFormat.Fm2, loaded.SourceFormat);
		Assert.Equal(movie.TotalFrames, loaded.TotalFrames);

		// Verify P1 input preserved
		for (int i = 0; i < movie.TotalFrames; i++) {
			Assert.Equal(
				movie.InputFrames[i].Controllers[0].A,
				loaded.InputFrames[i].Controllers[0].A);
			Assert.Equal(
				movie.InputFrames[i].Controllers[0].B,
				loaded.InputFrames[i].Controllers[0].B);
			Assert.Equal(
				movie.InputFrames[i].Controllers[0].Up,
				loaded.InputFrames[i].Controllers[0].Up);
		}
	}

	[Fact]
	public void Fm2Converter_RoundTrip_PreservesRerecordCount() {
		var movie = CreateTestMovie(SystemType.Nes);
		movie.RerecordCount = 9999;
		var converter = new Converters.Fm2MovieConverter();

		using var stream = new MemoryStream();
		converter.Write(movie, stream);
		stream.Position = 0;
		var loaded = converter.Read(stream, "test.fm2");

		Assert.Equal(9999UL, loaded.RerecordCount);
	}

	[Fact]
	public void Fm2Converter_RoundTrip_ZeroFrames() {
		var movie = CreateTestMovie(SystemType.Nes, 0);
		var converter = new Converters.Fm2MovieConverter();

		using var stream = new MemoryStream();
		converter.Write(movie, stream);
		stream.Position = 0;
		var loaded = converter.Read(stream, "test.fm2");

		Assert.Equal(0, loaded.TotalFrames);
	}

	[Fact]
	public void Fm2Converter_RoundTrip_AllButtonsPreserved() {
		var movie = CreateTestMovie(SystemType.Nes);
		var converter = new Converters.Fm2MovieConverter();

		using var stream = new MemoryStream();
		converter.Write(movie, stream);
		stream.Position = 0;
		var loaded = converter.Read(stream, "test.fm2");

		AssertFramesMatch(movie, loaded, 2);
	}

	#endregion

	#region BK2 Round-Trip

	[Fact]
	public void Bk2Converter_RoundTrip_PreservesData() {
		var movie = CreateTestMovie(SystemType.Snes);
		var converter = new Converters.Bk2MovieConverter();

		using var stream = new MemoryStream();
		converter.Write(movie, stream);
		stream.Position = 0;
		var loaded = converter.Read(stream, "test.bk2");

		Assert.Equal(MovieFormat.Bk2, loaded.SourceFormat);
		Assert.Equal(movie.TotalFrames, loaded.TotalFrames);
		Assert.Equal(movie.Author, loaded.Author);
		Assert.Equal(movie.GameName, loaded.GameName);
	}

	[Fact]
	public void Bk2Converter_RoundTrip_NesSystem() {
		var movie = CreateTestMovie(SystemType.Nes);
		var converter = new Converters.Bk2MovieConverter();

		using var stream = new MemoryStream();
		converter.Write(movie, stream);
		stream.Position = 0;
		var loaded = converter.Read(stream, "test.bk2");

		Assert.Equal(SystemType.Nes, loaded.SystemType);
	}

	[Theory]
	[InlineData(SystemType.Nes)]
	[InlineData(SystemType.Snes)]
	[InlineData(SystemType.Gb)]
	[InlineData(SystemType.Gba)]
	[InlineData(SystemType.Genesis)]
	public void Bk2Converter_RoundTrip_MultipleSystems(SystemType system) {
		var movie = CreateTestMovie(system);
		var converter = new Converters.Bk2MovieConverter();

		using var stream = new MemoryStream();
		converter.Write(movie, stream);
		stream.Position = 0;
		var loaded = converter.Read(stream, "test.bk2");

		Assert.Equal(movie.TotalFrames, loaded.TotalFrames);
	}

	[Fact]
	public void Bk2Converter_RoundTrip_AllButtonsPreserved() {
		var movie = CreateTestMovie(SystemType.Snes);
		var converter = new Converters.Bk2MovieConverter();

		using var stream = new MemoryStream();
		converter.Write(movie, stream);
		stream.Position = 0;
		var loaded = converter.Read(stream, "test.bk2");

		AssertFramesMatch(movie, loaded, 2);
	}

	#endregion

	#region SMV Round-Trip

	[Fact]
	public void SmvConverter_RoundTrip_PreservesInputFrames() {
		var movie = CreateTestMovie(SystemType.Snes);
		var converter = new Converters.SmvMovieConverter();

		using var stream = new MemoryStream();
		converter.Write(movie, stream);
		stream.Position = 0;
		var loaded = converter.Read(stream, "test.smv");

		Assert.Equal(MovieFormat.Smv, loaded.SourceFormat);
		Assert.Equal(movie.TotalFrames, loaded.TotalFrames);

		// SMV uses different button encoding, so verify individual buttons
		for (int i = 0; i < movie.TotalFrames; i++) {
			Assert.Equal(
				movie.InputFrames[i].Controllers[0].A,
				loaded.InputFrames[i].Controllers[0].A);
			Assert.Equal(
				movie.InputFrames[i].Controllers[0].B,
				loaded.InputFrames[i].Controllers[0].B);
		}
	}

	[Fact]
	public void SmvConverter_RoundTrip_PreservesRerecordCount() {
		var movie = CreateTestMovie(SystemType.Snes);
		movie.RerecordCount = 12345;
		var converter = new Converters.SmvMovieConverter();

		using var stream = new MemoryStream();
		converter.Write(movie, stream);
		stream.Position = 0;
		var loaded = converter.Read(stream, "test.smv");

		Assert.Equal(12345UL, loaded.RerecordCount);
	}

	[Fact]
	public void SmvConverter_RoundTrip_AllButtonsPreserved() {
		var movie = CreateTestMovie(SystemType.Snes);
		var converter = new Converters.SmvMovieConverter();

		using var stream = new MemoryStream();
		converter.Write(movie, stream);
		stream.Position = 0;
		var loaded = converter.Read(stream, "test.smv");

		AssertFramesMatch(movie, loaded, 2);
	}

	#endregion

	#region LSMV Round-Trip

	[Fact]
	public void LsmvConverter_RoundTrip_PreservesInputFrames() {
		var movie = CreateTestMovie(SystemType.Snes);
		var converter = new Converters.LsmvMovieConverter();

		using var stream = new MemoryStream();
		converter.Write(movie, stream);
		stream.Position = 0;
		var loaded = converter.Read(stream, "test.lsmv");

		Assert.Equal(MovieFormat.Lsmv, loaded.SourceFormat);
		Assert.Equal(movie.TotalFrames, loaded.TotalFrames);
	}

	[Fact]
	public void LsmvConverter_RoundTrip_PreservesAuthor() {
		var movie = CreateTestMovie(SystemType.Snes);
		movie.Author = "Test Author äöü";
		var converter = new Converters.LsmvMovieConverter();

		using var stream = new MemoryStream();
		converter.Write(movie, stream);
		stream.Position = 0;
		var loaded = converter.Read(stream, "test.lsmv");

		Assert.Equal(movie.Author, loaded.Author);
	}

	[Fact]
	public void LsmvConverter_RoundTrip_AllButtonsPreserved() {
		var movie = CreateTestMovie(SystemType.Snes);
		var converter = new Converters.LsmvMovieConverter();

		using var stream = new MemoryStream();
		converter.Write(movie, stream);
		stream.Position = 0;
		var loaded = converter.Read(stream, "test.lsmv");

		AssertFramesMatch(movie, loaded, 2);
	}

	#endregion

	#region Cross-Format Round-Trip

	[Theory]
	[InlineData(typeof(Converters.NexenMovieConverter))]
	[InlineData(typeof(Converters.Fm2MovieConverter))]
	[InlineData(typeof(Converters.Bk2MovieConverter))]
	[InlineData(typeof(Converters.SmvMovieConverter))]
	[InlineData(typeof(Converters.LsmvMovieConverter))]
	public void WritableConverter_RoundTrip_FrameCountPreserved(Type converterType) {
		var movie = CreateTestMovie(SystemType.Snes, 25);
		var converter = (IMovieConverter)Activator.CreateInstance(converterType)!;

		Assert.True(converter.CanWrite, $"{converterType.Name} should support writing");

		using var stream = new MemoryStream();
		converter.Write(movie, stream);
		stream.Position = 0;
		var loaded = converter.Read(stream);

		Assert.Equal(25, loaded.TotalFrames);
	}

	#endregion
}
