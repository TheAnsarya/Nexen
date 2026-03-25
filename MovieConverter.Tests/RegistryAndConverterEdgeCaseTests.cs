using System.Buffers.Binary;
using System.IO.Compression;
using System.Text;

namespace Nexen.MovieConverter.Tests;

/// <summary>
/// Tests for MovieConverterRegistry error handling, SMV write edge cases,
/// LSMV tolerance and savestate preservation, and async cancellation.
/// </summary>
public class RegistryAndConverterEdgeCaseTests {
	#region Registry Error Handling

	[Fact]
	public void Registry_Write_ReadOnlyFormat_ThrowsNotSupported() {
		// VBM is read-only
		var movie = new MovieData { SystemType = SystemType.Gba };
		Assert.Throws<NotSupportedException>(() =>
			MovieConverterRegistry.Write(movie, "output.vbm", MovieFormat.Vbm));
	}

	[Fact]
	public void Registry_Write_ReadOnlyFormatGmv_ThrowsNotSupported() {
		// GMV is read-only
		var movie = new MovieData { SystemType = SystemType.Genesis };
		Assert.Throws<NotSupportedException>(() =>
			MovieConverterRegistry.Write(movie, "output.gmv", MovieFormat.Gmv));
	}

	[Fact]
	public void Registry_Write_UnknownExtension_ThrowsNotSupported() {
		var movie = new MovieData();
		Assert.Throws<NotSupportedException>(() =>
			MovieConverterRegistry.Write(movie, "output.xyz"));
	}

	[Fact]
	public void Registry_Read_UnknownExtension_ThrowsNotSupported() {
		Assert.Throws<NotSupportedException>(() =>
			MovieConverterRegistry.Read("nonexistent.xyz"));
	}

	[Fact]
	public void Registry_GetWritableFormats_ExcludesVbmAndGmv() {
		var writable = MovieConverterRegistry.GetWritableFormats();

		Assert.DoesNotContain(writable, c => c.Format == MovieFormat.Vbm);
		Assert.DoesNotContain(writable, c => c.Format == MovieFormat.Gmv);
	}

	[Fact]
	public void Registry_GetReadableFormats_IncludesVbmAndGmv() {
		var readable = MovieConverterRegistry.GetReadableFormats();

		Assert.Contains(readable, c => c.Format == MovieFormat.Vbm);
		Assert.Contains(readable, c => c.Format == MovieFormat.Gmv);
	}

	#endregion

	#region SMV Write Edge Cases

	[Fact]
	public void SmvConverter_Write_ZeroFrames_RoundTrips() {
		var movie = new MovieData {
			SystemType = SystemType.Snes,
			ControllerCount = 1,
			Author = "Tester"
		};
		var converter = new Converters.SmvMovieConverter();

		using var stream = new MemoryStream();
		converter.Write(movie, stream);
		stream.Position = 0;
		var loaded = converter.Read(stream, "test.smv");

		Assert.Equal(0, loaded.TotalFrames);
	}

	[Fact]
	public void SmvConverter_Write_SingleFrame_RoundTrips() {
		var movie = new MovieData {
			SystemType = SystemType.Snes,
			ControllerCount = 1
		};
		var frame = new InputFrame(0);
		frame.Controllers[0].A = true;
		frame.Controllers[0].Start = true;
		movie.AddFrame(frame);

		var converter = new Converters.SmvMovieConverter();

		using var stream = new MemoryStream();
		converter.Write(movie, stream);
		stream.Position = 0;
		var loaded = converter.Read(stream, "test.smv");

		Assert.Equal(1, loaded.TotalFrames);
		Assert.True(loaded.InputFrames[0].Controllers[0].A);
		Assert.True(loaded.InputFrames[0].Controllers[0].Start);
	}

	[Fact]
	public void SmvConverter_Write_InitialState_PreservesData() {
		var movie = new MovieData {
			SystemType = SystemType.Snes,
			ControllerCount = 1,
			StartType = StartType.Savestate,
			InitialState = new byte[] { 0x01, 0x02, 0x03, 0xca, 0xfe }
		};
		var frame = new InputFrame(0);
		movie.AddFrame(frame);

		var converter = new Converters.SmvMovieConverter();

		using var stream = new MemoryStream();
		converter.Write(movie, stream);
		stream.Position = 0;
		var loaded = converter.Read(stream, "test.smv");

		Assert.Equal(StartType.Savestate, loaded.StartType);
		Assert.NotNull(loaded.InitialState);
		Assert.Equal(movie.InitialState, loaded.InitialState);
	}

	[Fact]
	public void SmvConverter_Write_LongAuthorName_Preserved() {
		var movie = new MovieData {
			SystemType = SystemType.Snes,
			ControllerCount = 1,
			Author = new string('A', 200),
			GameName = new string('G', 150)
		};
		movie.AddFrame(new InputFrame(0));

		var converter = new Converters.SmvMovieConverter();

		using var stream = new MemoryStream();
		converter.Write(movie, stream);
		stream.Position = 0;
		var loaded = converter.Read(stream, "test.smv");

		Assert.Equal(movie.Author, loaded.Author);
		Assert.Equal(movie.GameName, loaded.GameName);
	}

	[Fact]
	public void SmvConverter_Write_FiveControllers_Supported() {
		var movie = new MovieData {
			SystemType = SystemType.Snes,
			ControllerCount = 5
		};
		var frame = new InputFrame(0);
		for (int c = 0; c < 5 && c < InputFrame.MaxPorts; c++) {
			frame.Controllers[c].A = true;
		}
		movie.AddFrame(frame);

		var converter = new Converters.SmvMovieConverter();

		using var stream = new MemoryStream();
		converter.Write(movie, stream);
		stream.Position = 0;
		var loaded = converter.Read(stream, "test.smv");

		Assert.Equal(1, loaded.TotalFrames);
	}

	[Fact]
	public void SmvConverter_Write_PalRegion_SetsFlag() {
		var movie = new MovieData {
			SystemType = SystemType.Snes,
			ControllerCount = 1,
			Region = RegionType.PAL
		};
		movie.AddFrame(new InputFrame(0));

		var converter = new Converters.SmvMovieConverter();

		using var stream = new MemoryStream();
		converter.Write(movie, stream);
		stream.Position = 0;
		var loaded = converter.Read(stream, "test.smv");

		Assert.Equal(RegionType.PAL, loaded.Region);
	}

	#endregion

	#region LSMV Tolerance and Edge Cases

	[Fact]
	public void LsmvConverter_MissingInput_ReadsEmptyMovie() {
		using var zipStream = new MemoryStream();
		using (var archive = new ZipArchive(zipStream, ZipArchiveMode.Create, leaveOpen: true)) {
			// Manifest but no input entry
			var entry = archive.CreateEntry("manifest");
			using var writer = new StreamWriter(entry.Open());
			writer.Write("systemid snes\n");
		}

		zipStream.Position = 0;
		var converter = new Converters.LsmvMovieConverter();
		var movie = converter.Read(zipStream, "test.lsmv");

		Assert.Equal(0, movie.TotalFrames);
	}

	[Fact]
	public void LsmvConverter_MissingManifest_ReadsGracefully() {
		using var zipStream = new MemoryStream();
		using (var archive = new ZipArchive(zipStream, ZipArchiveMode.Create, leaveOpen: true)) {
			// Input only, no manifest
			var entry = archive.CreateEntry("input");
			using var writer = new StreamWriter(entry.Open());
			writer.Write("|..............||\n");
		}

		zipStream.Position = 0;
		var converter = new Converters.LsmvMovieConverter();
		var movie = converter.Read(zipStream, "test.lsmv");

		Assert.Equal(MovieFormat.Lsmv, movie.SourceFormat);
	}

	[Fact]
	public void LsmvConverter_SavestateEntry_PreservesData() {
		byte[] savestateData = [0xca, 0xfe, 0xba, 0xbe, 0xde, 0xad];
		using var zipStream = new MemoryStream();
		using (var archive = new ZipArchive(zipStream, ZipArchiveMode.Create, leaveOpen: true)) {
			var manifest = archive.CreateEntry("manifest");
			using (var w = new StreamWriter(manifest.Open())) {
				w.Write("systemid snes\n");
			}

			var input = archive.CreateEntry("input");
			using (var w = new StreamWriter(input.Open())) {
				w.Write("|..............||\n");
			}

			var savestate = archive.CreateEntry("savestate");
			using (var ss = savestate.Open()) {
				ss.Write(savestateData);
			}
		}

		zipStream.Position = 0;
		var converter = new Converters.LsmvMovieConverter();
		var movie = converter.Read(zipStream, "test.lsmv");

		Assert.Equal(StartType.Savestate, movie.StartType);
		Assert.NotNull(movie.InitialState);
		Assert.Equal(savestateData, movie.InitialState);
	}

	[Fact]
	public void LsmvConverter_SramEntry_PreservesData() {
		byte[] sramData = [0x01, 0x02, 0x03, 0x04];
		using var zipStream = new MemoryStream();
		using (var archive = new ZipArchive(zipStream, ZipArchiveMode.Create, leaveOpen: true)) {
			var manifest = archive.CreateEntry("manifest");
			using (var w = new StreamWriter(manifest.Open())) {
				w.Write("systemid snes\n");
			}

			var input = archive.CreateEntry("input");
			using (var w = new StreamWriter(input.Open())) {
				w.Write("|..............||\n");
			}

			var sram = archive.CreateEntry("moviesram");
			using (var s = sram.Open()) {
				s.Write(sramData);
			}
		}

		zipStream.Position = 0;
		var converter = new Converters.LsmvMovieConverter();
		var movie = converter.Read(zipStream, "test.lsmv");

		Assert.NotNull(movie.InitialSram);
		Assert.Equal(sramData, movie.InitialSram);
	}

	[Fact]
	public void LsmvConverter_Write_SavestateAndSram_Roundtrip() {
		var movie = new MovieData {
			SystemType = SystemType.Snes,
			ControllerCount = 2,
			StartType = StartType.Savestate,
			InitialState = [0xaa, 0xbb, 0xcc],
			InitialSram = [0x11, 0x22, 0x33, 0x44]
		};
		movie.AddFrame(new InputFrame(0));

		var converter = new Converters.LsmvMovieConverter();

		using var stream = new MemoryStream();
		converter.Write(movie, stream);
		stream.Position = 0;
		var loaded = converter.Read(stream, "test.lsmv");

		Assert.Equal(StartType.Savestate, loaded.StartType);
		Assert.Equal(movie.InitialState, loaded.InitialState);
		Assert.Equal(movie.InitialSram, loaded.InitialSram);
	}

	[Fact]
	public void LsmvConverter_UnicodeAuthor_Preserved() {
		var movie = new MovieData {
			SystemType = SystemType.Snes,
			ControllerCount = 2,
			Author = "日本語テスター 🎮"
		};
		movie.AddFrame(new InputFrame(0));

		var converter = new Converters.LsmvMovieConverter();

		using var stream = new MemoryStream();
		converter.Write(movie, stream);
		stream.Position = 0;
		var loaded = converter.Read(stream, "test.lsmv");

		Assert.Equal(movie.Author, loaded.Author);
	}

	[Fact]
	public void LsmvConverter_EmptyZipArchive_ReadsGracefully() {
		using var zipStream = new MemoryStream();
		using (var archive = new ZipArchive(zipStream, ZipArchiveMode.Create, leaveOpen: true)) {
			// Empty archive — no entries at all
		}

		zipStream.Position = 0;
		var converter = new Converters.LsmvMovieConverter();
		var movie = converter.Read(zipStream, "test.lsmv");

		Assert.Equal(MovieFormat.Lsmv, movie.SourceFormat);
		Assert.Equal(0, movie.TotalFrames);
	}

	#endregion

	#region Async Cancellation

	[Fact]
	public async Task ReadAsync_CancelledToken_ThrowsOperationCancelled() {
		var converter = new Converters.NexenMovieConverter();
		using var cts = new CancellationTokenSource();
		cts.Cancel();

		using var stream = new MemoryStream();

		await Assert.ThrowsAnyAsync<OperationCanceledException>(() =>
			converter.ReadAsync(stream, "test.nexen-movie", cts.Token).AsTask());
	}

	[Fact]
	public async Task WriteAsync_CancelledToken_ThrowsOperationCancelled() {
		var converter = new Converters.NexenMovieConverter();
		using var cts = new CancellationTokenSource();
		cts.Cancel();

		using var stream = new MemoryStream();
		var movie = new MovieData();

		await Assert.ThrowsAnyAsync<OperationCanceledException>(() =>
			converter.WriteAsync(movie, stream, cts.Token).AsTask());
	}

	#endregion
}
