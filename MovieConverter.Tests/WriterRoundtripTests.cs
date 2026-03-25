using System.IO.Compression;
using System.Text;
using Nexen.MovieConverter.Converters;

namespace Nexen.MovieConverter.Tests;

/// <summary>
/// Write-path roundtrip tests for LSMV, FM2, and SMV converters.
/// Verifies metadata + input survive a Write → Read cycle.
/// </summary>
public sealed class WriterRoundtripTests {
	#region Helpers

	private static MovieData CreateTestMovie(
		SystemType system = SystemType.Snes,
		RegionType region = RegionType.NTSC,
		int frames = 10,
		int controllers = 2) {
		var movie = new MovieData {
			SystemType = system,
			Region = region,
			GameName = "Test Game",
			Author = "TestAuthor",
			RerecordCount = 42,
			ControllerCount = controllers
		};

		for (int i = 0; i < frames; i++) {
			var frame = new InputFrame(i);
			frame.Controllers[0].A = i % 2 == 0;
			frame.Controllers[0].B = i % 3 == 0;
			frame.Controllers[0].Right = i % 5 == 0;
			if (controllers >= 2) {
				frame.Controllers[1].Start = i % 4 == 0;
			}
			movie.AddFrame(frame);
		}

		return movie;
	}

	private static MovieData RoundTrip(MovieConverterBase converter, MovieData original) {
		using var stream = new MemoryStream();
		converter.Write(original, stream);
		stream.Position = 0;
		return converter.Read(stream);
	}

	#endregion

	#region LSMV Roundtrip

	[Fact]
	public void Lsmv_Metadata_RoundTrips() {
		var converter = new LsmvMovieConverter();
		var original = CreateTestMovie();
		original.EmulatorVersion = "lsnes rr2-β24";

		var loaded = RoundTrip(converter, original);

		Assert.Equal(original.GameName, loaded.GameName);
		Assert.Equal(original.Author, loaded.Author);
		Assert.Equal(original.RerecordCount, loaded.RerecordCount);
		Assert.Equal(SystemType.Snes, loaded.SystemType);
	}

	[Fact]
	public void Lsmv_InputFrames_RoundTrip() {
		var converter = new LsmvMovieConverter();
		var original = CreateTestMovie(frames: 50);

		var loaded = RoundTrip(converter, original);

		Assert.Equal(original.TotalFrames, loaded.TotalFrames);
		for (int i = 0; i < original.TotalFrames; i++) {
			Assert.Equal(original.InputFrames[i].Controllers[0].A, loaded.InputFrames[i].Controllers[0].A);
			Assert.Equal(original.InputFrames[i].Controllers[0].B, loaded.InputFrames[i].Controllers[0].B);
			Assert.Equal(original.InputFrames[i].Controllers[0].Right, loaded.InputFrames[i].Controllers[0].Right);
			Assert.Equal(original.InputFrames[i].Controllers[1].Start, loaded.InputFrames[i].Controllers[1].Start);
		}
	}

	[Fact]
	public void Lsmv_AllSnesButtons_RoundTrip() {
		var converter = new LsmvMovieConverter();
		var movie = new MovieData { SystemType = SystemType.Snes, ControllerCount = 2 };
		var frame = new InputFrame(0);
		frame.Controllers[0].B = true;
		frame.Controllers[0].Y = true;
		frame.Controllers[0].Select = true;
		frame.Controllers[0].Start = true;
		frame.Controllers[0].Up = true;
		frame.Controllers[0].Down = true;
		frame.Controllers[0].Left = true;
		frame.Controllers[0].Right = true;
		frame.Controllers[0].A = true;
		frame.Controllers[0].X = true;
		frame.Controllers[0].L = true;
		frame.Controllers[0].R = true;
		movie.AddFrame(frame);

		var loaded = RoundTrip(converter, movie);

		var c = loaded.InputFrames[0].Controllers[0];
		Assert.True(c.B && c.Y && c.Select && c.Start);
		Assert.True(c.Up && c.Down && c.Left && c.Right);
		Assert.True(c.A && c.X && c.L && c.R);
	}

	[Fact]
	public void Lsmv_GbSystem_RoundTrips() {
		var converter = new LsmvMovieConverter();
		var movie = new MovieData { SystemType = SystemType.Gb, ControllerCount = 1 };
		var frame = new InputFrame(0);
		frame.Controllers[0].A = true;
		frame.Controllers[0].B = true;
		frame.Controllers[0].Select = true;
		frame.Controllers[0].Start = true;
		frame.Controllers[0].Up = true;
		frame.Controllers[0].Down = true;
		frame.Controllers[0].Left = true;
		frame.Controllers[0].Right = true;
		movie.AddFrame(frame);

		var loaded = RoundTrip(converter, movie);

		Assert.Equal(SystemType.Gb, loaded.SystemType);
		var c = loaded.InputFrames[0].Controllers[0];
		Assert.True(c.A && c.B && c.Select && c.Start);
		Assert.True(c.Up && c.Down && c.Left && c.Right);
	}

	[Fact]
	public void Lsmv_Savestate_RoundTrips() {
		var converter = new LsmvMovieConverter();
		var movie = CreateTestMovie(frames: 5);
		movie.StartType = StartType.Savestate;
		movie.InitialState = [0xde, 0xad, 0xbe, 0xef];

		var loaded = RoundTrip(converter, movie);

		Assert.Equal(StartType.Savestate, loaded.StartType);
		Assert.NotNull(loaded.InitialState);
		Assert.Equal(movie.InitialState, loaded.InitialState);
	}

	[Fact]
	public void Lsmv_Sram_RoundTrips() {
		var converter = new LsmvMovieConverter();
		var movie = CreateTestMovie(frames: 5);
		movie.InitialSram = [0x01, 0x02, 0x03, 0x04];

		var loaded = RoundTrip(converter, movie);

		Assert.NotNull(loaded.InitialSram);
		Assert.Equal(movie.InitialSram, loaded.InitialSram);
	}

	[Fact]
	public void Lsmv_Sha256Hash_RoundTrips() {
		var converter = new LsmvMovieConverter();
		var movie = CreateTestMovie(frames: 2);
		movie.Sha256Hash = "abc123def456";

		var loaded = RoundTrip(converter, movie);

		Assert.Equal("abc123def456", loaded.Sha256Hash);
	}

	[Fact]
	public void Lsmv_SoftReset_RoundTrips() {
		var converter = new LsmvMovieConverter();
		var movie = new MovieData { SystemType = SystemType.Snes, ControllerCount = 1 };
		var frame = new InputFrame(0) { Command = FrameCommand.SoftReset };
		movie.AddFrame(frame);

		var loaded = RoundTrip(converter, movie);

		Assert.True(loaded.InputFrames[0].Command.HasFlag(FrameCommand.SoftReset));
	}

	[Fact]
	public void Lsmv_HardReset_RoundTrips() {
		var converter = new LsmvMovieConverter();
		var movie = new MovieData { SystemType = SystemType.Snes, ControllerCount = 1 };
		var frame = new InputFrame(0) { Command = FrameCommand.HardReset };
		movie.AddFrame(frame);

		var loaded = RoundTrip(converter, movie);

		Assert.True(loaded.InputFrames[0].Command.HasFlag(FrameCommand.HardReset));
	}

	[Fact]
	public void Lsmv_Subtitles_RoundTrips() {
		var converter = new LsmvMovieConverter();
		var movie = CreateTestMovie(frames: 5);
		movie.Markers = [
			new MovieMarker { FrameNumber = 0, Label = "Start", Type = MarkerType.Subtitle },
			new MovieMarker { FrameNumber = 100, Label = "Boss fight", Type = MarkerType.Subtitle }
		];

		var loaded = RoundTrip(converter, movie);

		Assert.NotNull(loaded.Markers);
		Assert.Equal(2, loaded.Markers.Count);
		Assert.Equal("Start", loaded.Markers[0].Label);
		Assert.Equal("Boss fight", loaded.Markers[1].Label);
	}

	[Fact]
	public void Lsmv_EmptyMovie_RoundTrips() {
		var converter = new LsmvMovieConverter();
		var movie = new MovieData { SystemType = SystemType.Snes, ControllerCount = 1 };

		var loaded = RoundTrip(converter, movie);

		Assert.Equal(0, loaded.TotalFrames);
		Assert.Equal(SystemType.Snes, loaded.SystemType);
	}

	[Fact]
	public void Lsmv_ZipStructure_ContainsExpectedEntries() {
		var converter = new LsmvMovieConverter();
		var movie = CreateTestMovie(frames: 5);
		movie.Sha256Hash = "abc123";
		movie.Markers = [new MovieMarker { FrameNumber = 0, Label = "X", Type = MarkerType.Subtitle }];

		using var stream = new MemoryStream();
		converter.Write(movie, stream);
		stream.Position = 0;

		using var archive = new ZipArchive(stream, ZipArchiveMode.Read);
		Assert.NotNull(archive.GetEntry("manifest"));
		Assert.NotNull(archive.GetEntry("gamename"));
		Assert.NotNull(archive.GetEntry("authors"));
		Assert.NotNull(archive.GetEntry("rerecords"));
		Assert.NotNull(archive.GetEntry("input"));
		Assert.NotNull(archive.GetEntry("slot1.sha256"));
		Assert.NotNull(archive.GetEntry("subtitles"));
	}

	#endregion

	#region FM2 Roundtrip

	[Fact]
	public void Fm2_Metadata_RoundTrips() {
		var converter = new Fm2MovieConverter();
		var original = CreateTestMovie(system: SystemType.Nes, controllers: 2);
		original.RomFileName = "test.nes";
		original.Description = "A test movie";

		var loaded = RoundTrip(converter, original);

		Assert.Equal(original.RerecordCount, loaded.RerecordCount);
		Assert.Equal(SystemType.Nes, loaded.SystemType);
	}

	[Fact]
	public void Fm2_InputFrames_RoundTrip() {
		var converter = new Fm2MovieConverter();
		var original = CreateTestMovie(system: SystemType.Nes, frames: 30);

		var loaded = RoundTrip(converter, original);

		Assert.Equal(original.TotalFrames, loaded.TotalFrames);
		for (int i = 0; i < original.TotalFrames; i++) {
			Assert.Equal(original.InputFrames[i].Controllers[0].A, loaded.InputFrames[i].Controllers[0].A);
			Assert.Equal(original.InputFrames[i].Controllers[0].B, loaded.InputFrames[i].Controllers[0].B);
			Assert.Equal(original.InputFrames[i].Controllers[0].Right, loaded.InputFrames[i].Controllers[0].Right);
		}
	}

	[Fact]
	public void Fm2_AllNesButtons_RoundTrip() {
		var converter = new Fm2MovieConverter();
		var movie = new MovieData { SystemType = SystemType.Nes, ControllerCount = 1 };
		var frame = new InputFrame(0);
		frame.Controllers[0].Right = true;
		frame.Controllers[0].Left = true;
		frame.Controllers[0].Down = true;
		frame.Controllers[0].Up = true;
		frame.Controllers[0].Start = true;
		frame.Controllers[0].Select = true;
		frame.Controllers[0].B = true;
		frame.Controllers[0].A = true;
		movie.AddFrame(frame);

		var loaded = RoundTrip(converter, movie);

		var c = loaded.InputFrames[0].Controllers[0];
		Assert.True(c.Right && c.Left && c.Down && c.Up);
		Assert.True(c.Start && c.Select && c.B && c.A);
	}

	[Fact]
	public void Fm2_ResetCommand_RoundTrips() {
		var converter = new Fm2MovieConverter();
		var movie = new MovieData { SystemType = SystemType.Nes, ControllerCount = 1 };
		movie.AddFrame(new InputFrame(0) { Command = FrameCommand.Reset });

		var loaded = RoundTrip(converter, movie);

		Assert.True(loaded.InputFrames[0].Command.HasFlag(FrameCommand.Reset));
	}

	[Fact]
	public void Fm2_PowerCommand_RoundTrips() {
		var converter = new Fm2MovieConverter();
		var movie = new MovieData { SystemType = SystemType.Nes, ControllerCount = 1 };
		movie.AddFrame(new InputFrame(0) { Command = FrameCommand.Power });

		var loaded = RoundTrip(converter, movie);

		Assert.True(loaded.InputFrames[0].Command.HasFlag(FrameCommand.Power));
	}

	[Fact]
	public void Fm2_FdsCommands_RoundTrip() {
		var converter = new Fm2MovieConverter();
		var movie = new MovieData { SystemType = SystemType.Nes, ControllerCount = 1 };
		movie.AddFrame(new InputFrame(0) { Command = FrameCommand.FdsInsert });
		movie.AddFrame(new InputFrame(1) { Command = FrameCommand.FdsSide });

		var loaded = RoundTrip(converter, movie);

		Assert.True(loaded.InputFrames[0].Command.HasFlag(FrameCommand.FdsInsert));
		Assert.True(loaded.InputFrames[1].Command.HasFlag(FrameCommand.FdsSide));
	}

	[Fact]
	public void Fm2_PalRegion_RoundTrips() {
		var converter = new Fm2MovieConverter();
		var movie = CreateTestMovie(system: SystemType.Nes, region: RegionType.PAL, frames: 5);

		var loaded = RoundTrip(converter, movie);

		Assert.Equal(RegionType.PAL, loaded.Region);
	}

	[Fact]
	public void Fm2_FoursCore_RoundTrips() {
		var converter = new Fm2MovieConverter();
		var movie = CreateTestMovie(system: SystemType.Nes, controllers: 4, frames: 5);
		var frame = movie.InputFrames[0];
		frame.Controllers[2].A = true;
		frame.Controllers[3].B = true;

		var loaded = RoundTrip(converter, movie);

		Assert.True(loaded.InputFrames[0].Controllers[2].A);
		Assert.True(loaded.InputFrames[0].Controllers[3].B);
	}

	[Fact]
	public void Fm2_Markers_WrittenAsSubtitles() {
		var converter = new Fm2MovieConverter();
		var movie = CreateTestMovie(system: SystemType.Nes, frames: 5);
		movie.Markers = [new MovieMarker { FrameNumber = 10, Label = "Boss" }];

		using var stream = new MemoryStream();
		converter.Write(movie, stream);
		stream.Position = 0;

		using var reader = new StreamReader(stream);
		string content = reader.ReadToEnd();

		Assert.Contains("subtitle 10 Boss", content);
	}

	[Fact]
	public void Fm2_EmptyMovie_RoundTrips() {
		var converter = new Fm2MovieConverter();
		var movie = new MovieData { SystemType = SystemType.Nes, ControllerCount = 1 };

		var loaded = RoundTrip(converter, movie);

		Assert.Equal(0, loaded.TotalFrames);
	}

	[Fact]
	public void Fm2_HeaderFormat_ContainsRequiredFields() {
		var converter = new Fm2MovieConverter();
		var movie = CreateTestMovie(system: SystemType.Nes, frames: 1);
		movie.RomFileName = "mario.nes";

		using var stream = new MemoryStream();
		converter.Write(movie, stream);
		stream.Position = 0;

		using var reader = new StreamReader(stream);
		string content = reader.ReadToEnd();

		Assert.Contains("version 3", content);
		Assert.Contains("rerecordCount 42", content);
		Assert.Contains("romFilename mario.nes", content);
		Assert.Contains("guid ", content);
		Assert.Contains("port0 1", content);
	}

	#endregion

	#region SMV Roundtrip

	[Fact]
	public void Smv_Metadata_RoundTrips() {
		var converter = new SmvMovieConverter();
		var original = CreateTestMovie(frames: 10);

		var loaded = RoundTrip(converter, original);

		Assert.Equal(original.RerecordCount, loaded.RerecordCount);
		Assert.Equal(SystemType.Snes, loaded.SystemType);
	}

	[Fact]
	public void Smv_InputFrames_RoundTrip() {
		var converter = new SmvMovieConverter();
		var original = CreateTestMovie(frames: 50);

		var loaded = RoundTrip(converter, original);

		Assert.Equal(original.TotalFrames, loaded.TotalFrames);
		for (int i = 0; i < original.TotalFrames; i++) {
			Assert.Equal(original.InputFrames[i].Controllers[0].A, loaded.InputFrames[i].Controllers[0].A);
			Assert.Equal(original.InputFrames[i].Controllers[0].B, loaded.InputFrames[i].Controllers[0].B);
			Assert.Equal(original.InputFrames[i].Controllers[0].Right, loaded.InputFrames[i].Controllers[0].Right);
			Assert.Equal(original.InputFrames[i].Controllers[1].Start, loaded.InputFrames[i].Controllers[1].Start);
		}
	}

	[Fact]
	public void Smv_AllSnesButtons_RoundTrip() {
		var converter = new SmvMovieConverter();
		var movie = new MovieData { SystemType = SystemType.Snes, ControllerCount = 1 };
		var frame = new InputFrame(0);
		frame.Controllers[0].B = true;
		frame.Controllers[0].Y = true;
		frame.Controllers[0].Select = true;
		frame.Controllers[0].Start = true;
		frame.Controllers[0].Up = true;
		frame.Controllers[0].Down = true;
		frame.Controllers[0].Left = true;
		frame.Controllers[0].Right = true;
		frame.Controllers[0].A = true;
		frame.Controllers[0].X = true;
		frame.Controllers[0].L = true;
		frame.Controllers[0].R = true;
		movie.AddFrame(frame);

		var loaded = RoundTrip(converter, movie);

		var c = loaded.InputFrames[0].Controllers[0];
		Assert.True(c.B && c.Y && c.Select && c.Start);
		Assert.True(c.Up && c.Down && c.Left && c.Right);
		Assert.True(c.A && c.X && c.L && c.R);
	}

	[Fact]
	public void Smv_PalRegion_RoundTrips() {
		var converter = new SmvMovieConverter();
		var movie = CreateTestMovie(region: RegionType.PAL, frames: 5);

		var loaded = RoundTrip(converter, movie);

		Assert.Equal(RegionType.PAL, loaded.Region);
	}

	[Fact]
	public void Smv_Savestate_RoundTrips() {
		var converter = new SmvMovieConverter();
		var movie = CreateTestMovie(frames: 5);
		movie.StartType = StartType.Savestate;
		movie.InitialState = [0xca, 0xfe, 0xba, 0xbe];

		var loaded = RoundTrip(converter, movie);

		Assert.Equal(StartType.Savestate, loaded.StartType);
		Assert.NotNull(loaded.InitialState);
		Assert.Equal(movie.InitialState, loaded.InitialState);
	}

	[Fact]
	public void Smv_GameNameAndAuthor_RoundTrips() {
		var converter = new SmvMovieConverter();
		var movie = CreateTestMovie(frames: 3);

		var loaded = RoundTrip(converter, movie);

		Assert.Equal("Test Game", loaded.GameName);
		Assert.Equal("TestAuthor", loaded.Author);
	}

	[Fact]
	public void Smv_FrameCount_Precise() {
		var converter = new SmvMovieConverter();
		var movie = CreateTestMovie(frames: 137);

		var loaded = RoundTrip(converter, movie);

		Assert.Equal(137, loaded.TotalFrames);
	}

	[Fact]
	public void Smv_EmptyMovie_RoundTrips() {
		var converter = new SmvMovieConverter();
		var movie = new MovieData { SystemType = SystemType.Snes, ControllerCount = 1 };

		var loaded = RoundTrip(converter, movie);

		Assert.Equal(0, loaded.TotalFrames);
	}

	[Fact]
	public void Smv_Uid_IsWritten() {
		var converter = new SmvMovieConverter();
		var movie = CreateTestMovie(frames: 1);

		var loaded = RoundTrip(converter, movie);

		Assert.True(loaded.ExtraData?.ContainsKey("UID") == true);
		Assert.True(ulong.TryParse(loaded.ExtraData!["UID"], out var uid));
		Assert.NotEqual(0UL, uid);
	}

	[Fact]
	public void Smv_MultipleControllers_FiveMax() {
		var converter = new SmvMovieConverter();
		var movie = new MovieData { SystemType = SystemType.Snes, ControllerCount = 5 };
		var frame = new InputFrame(0);
		frame.Controllers[0].A = true;
		frame.Controllers[4].B = true;
		movie.AddFrame(frame);

		var loaded = RoundTrip(converter, movie);

		Assert.Equal(5, loaded.ControllerCount);
		Assert.True(loaded.InputFrames[0].Controllers[0].A);
		Assert.True(loaded.InputFrames[0].Controllers[4].B);
	}

	#endregion

	#region VBM Read-from-bytes

	[Fact]
	public void Vbm_CanWrite_IsFalse() {
		var converter = new VbmMovieConverter();
		Assert.False(converter.CanWrite);
	}

	[Fact]
	public void Vbm_Write_ThrowsNotSupported() {
		var converter = new VbmMovieConverter();
		using var stream = new MemoryStream();
		Assert.Throws<NotSupportedException>(() => converter.Write(new MovieData(), stream));
	}

	[Fact]
	public void Vbm_InvalidSignature_Throws() {
		var converter = new VbmMovieConverter();
		using var stream = new MemoryStream(new byte[256]);
		Assert.Throws<MovieFormatException>(() => converter.Read(stream));
	}

	#endregion

	#region GMV Read-only

	[Fact]
	public void Gmv_CanWrite_IsFalse() {
		var converter = new GmvMovieConverter();
		Assert.False(converter.CanWrite);
	}

	[Fact]
	public void Gmv_Write_ThrowsNotSupported() {
		var converter = new GmvMovieConverter();
		using var stream = new MemoryStream();
		Assert.Throws<NotSupportedException>(() => converter.Write(new MovieData(), stream));
	}

	[Fact]
	public void Gmv_InvalidSignature_Throws() {
		var converter = new GmvMovieConverter();
		using var stream = new MemoryStream(new byte[256]);
		Assert.Throws<MovieFormatException>(() => converter.Read(stream));
	}

	#endregion

	#region Cross-format

	[Fact]
	public void CrossFormat_NexenToLsmv_PreservesInput() {
		var original = CreateTestMovie(frames: 20);

		// Write as Nexen
		using var nexenStream = new MemoryStream();
		new NexenMovieConverter().Write(original, nexenStream);
		nexenStream.Position = 0;
		var fromNexen = new NexenMovieConverter().Read(nexenStream);

		// Write as LSMV, read back
		using var lsmvStream = new MemoryStream();
		new LsmvMovieConverter().Write(fromNexen, lsmvStream);
		lsmvStream.Position = 0;
		var fromLsmv = new LsmvMovieConverter().Read(lsmvStream);

		Assert.Equal(original.TotalFrames, fromLsmv.TotalFrames);
		for (int i = 0; i < original.TotalFrames; i++) {
			Assert.Equal(original.InputFrames[i].Controllers[0].A, fromLsmv.InputFrames[i].Controllers[0].A);
			Assert.Equal(original.InputFrames[i].Controllers[0].B, fromLsmv.InputFrames[i].Controllers[0].B);
		}
	}

	[Fact]
	public void CrossFormat_NexenToSmv_PreservesInput() {
		var original = CreateTestMovie(frames: 20);

		using var nexenStream = new MemoryStream();
		new NexenMovieConverter().Write(original, nexenStream);
		nexenStream.Position = 0;
		var fromNexen = new NexenMovieConverter().Read(nexenStream);

		using var smvStream = new MemoryStream();
		new SmvMovieConverter().Write(fromNexen, smvStream);
		smvStream.Position = 0;
		var fromSmv = new SmvMovieConverter().Read(smvStream);

		Assert.Equal(original.TotalFrames, fromSmv.TotalFrames);
		for (int i = 0; i < original.TotalFrames; i++) {
			Assert.Equal(original.InputFrames[i].Controllers[0].A, fromSmv.InputFrames[i].Controllers[0].A);
			Assert.Equal(original.InputFrames[i].Controllers[0].B, fromSmv.InputFrames[i].Controllers[0].B);
		}
	}

	[Fact]
	public void CrossFormat_NexenToFm2_PreservesNesInput() {
		var original = CreateTestMovie(system: SystemType.Nes, frames: 20);

		using var nexenStream = new MemoryStream();
		new NexenMovieConverter().Write(original, nexenStream);
		nexenStream.Position = 0;
		var fromNexen = new NexenMovieConverter().Read(nexenStream);

		using var fm2Stream = new MemoryStream();
		new Fm2MovieConverter().Write(fromNexen, fm2Stream);
		fm2Stream.Position = 0;
		var fromFm2 = new Fm2MovieConverter().Read(fm2Stream);

		Assert.Equal(original.TotalFrames, fromFm2.TotalFrames);
		for (int i = 0; i < original.TotalFrames; i++) {
			Assert.Equal(original.InputFrames[i].Controllers[0].A, fromFm2.InputFrames[i].Controllers[0].A);
			Assert.Equal(original.InputFrames[i].Controllers[0].B, fromFm2.InputFrames[i].Controllers[0].B);
		}
	}

	#endregion
}
