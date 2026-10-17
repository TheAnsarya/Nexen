using System.IO.Compression;
using System.Text;

namespace Nexen.MovieConverter.Tests;

/// <summary>
/// Comprehensive integration tests for NexenFormat (.nexen-movie).
/// Tests all optional features: markers, scripts, binary data, port types,
/// extended metadata, and cross-format conversion fidelity.
/// </summary>
public class NexenFormatIntegrationTests {
	private static readonly Converters.NexenMovieConverter Converter = new();

	/// <summary>
	/// Helper: write movie to MemoryStream, rewind, read back.
	/// </summary>
	private static MovieData RoundTrip(MovieData movie) {
		using var stream = new MemoryStream();
		Converter.Write(movie, stream);
		stream.Position = 0;
		return Converter.Read(stream, "test.nexen-movie");
	}

	/// <summary>Creates a minimal test movie.</summary>
	private static MovieData MakeMovie(SystemType system = SystemType.Nes, int frames = 10) {
		var movie = new MovieData {
			Author = "Integration Tester",
			Description = "NexenFormat integration test",
			GameName = "Test Game",
			RomFileName = "test-rom.nes",
			SystemType = system,
			Region = RegionType.NTSC,
			RerecordCount = 100,
			ControllerCount = 2
		};

		for (int i = 0; i < frames; i++) {
			var frame = new InputFrame(i);
			frame.Controllers[0].A = i % 2 == 0;
			frame.Controllers[0].B = i % 3 == 0;
			frame.Controllers[0].Up = i % 5 == 0;
			movie.AddFrame(frame);
		}

		return movie;
	}

	#region Markers Roundtrip

	[Fact]
	public void RoundTrip_PreservesMarkers() {
		var movie = MakeMovie();
		movie.Markers =
		[
			new MovieMarker { FrameNumber = 0, Label = "Start", Type = MarkerType.Bookmark },
			new MovieMarker { FrameNumber = 5, Label = "Mid", Description = "Midpoint", Color = "#ff0000", Type = MarkerType.Chapter },
			new MovieMarker { FrameNumber = 9, Label = "End", Type = MarkerType.Segment }
		];

		var loaded = RoundTrip(movie);

		Assert.NotNull(loaded.Markers);
		Assert.Equal(3, loaded.Markers.Count);
		Assert.Equal("Start", loaded.Markers[0].Label);
		Assert.Equal(MarkerType.Bookmark, loaded.Markers[0].Type);
		Assert.Equal(5, loaded.Markers[1].FrameNumber);
		Assert.Equal("Midpoint", loaded.Markers[1].Description);
		Assert.Equal("#ff0000", loaded.Markers[1].Color);
		Assert.Equal(MarkerType.Chapter, loaded.Markers[1].Type);
		Assert.Equal(MarkerType.Segment, loaded.Markers[2].Type);
	}

	[Fact]
	public void RoundTrip_NullMarkers_StaysNull() {
		var movie = MakeMovie();
		movie.Markers = null;

		var loaded = RoundTrip(movie);

		Assert.Null(loaded.Markers);
	}

	[Fact]
	public void RoundTrip_EmptyMarkers_StaysEmpty() {
		var movie = MakeMovie();
		movie.Markers = [];

		var loaded = RoundTrip(movie);

		// Empty list is not written, so on read it returns null
		Assert.Null(loaded.Markers);
	}

	#endregion

	#region Embedded Scripts Roundtrip

	[Fact]
	public void RoundTrip_PreservesLuaScript() {
		var movie = MakeMovie();
		movie.Scripts =
		[
			new EmbeddedScript {
				Name = "display.lua",
				Content = "-- Display HUD\ngui.text(10, 10, 'Hello')\n",
				Language = "lua",
				AutoRun = true
			}
		];

		var loaded = RoundTrip(movie);

		Assert.NotNull(loaded.Scripts);
		Assert.Single(loaded.Scripts);
		Assert.Equal("display.lua", loaded.Scripts[0].Name);
		Assert.Contains("Display HUD", loaded.Scripts[0].Content);
		Assert.Equal("lua", loaded.Scripts[0].Language);
	}

	[Fact]
	public void RoundTrip_PreservesMultipleScripts() {
		var movie = MakeMovie();
		movie.Scripts =
		[
			new EmbeddedScript { Name = "a.lua", Content = "-- script A", Language = "lua" },
			new EmbeddedScript { Name = "b.py", Content = "# script B", Language = "python" },
			new EmbeddedScript { Name = "c.js", Content = "// script C", Language = "javascript" }
		];

		var loaded = RoundTrip(movie);

		Assert.NotNull(loaded.Scripts);
		Assert.Equal(3, loaded.Scripts.Count);
		Assert.Contains(loaded.Scripts, s => s.Language == "lua");
		Assert.Contains(loaded.Scripts, s => s.Language == "python");
		Assert.Contains(loaded.Scripts, s => s.Language == "javascript");
	}

	[Fact]
	public void RoundTrip_NullScripts_StaysNull() {
		var movie = MakeMovie();
		movie.Scripts = null;

		var loaded = RoundTrip(movie);

		Assert.Null(loaded.Scripts);
	}

	#endregion

	#region Binary Data Roundtrip (InitialState / SRAM / RTC)

	[Fact]
	public void RoundTrip_PreservesInitialState() {
		var movie = MakeMovie();
		movie.InitialState = new byte[] { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77 };
		movie.StartType = StartType.Savestate;

		var loaded = RoundTrip(movie);

		Assert.NotNull(loaded.InitialState);
		Assert.Equal(movie.InitialState, loaded.InitialState);
		Assert.Equal(StartType.Savestate, loaded.StartType);
	}

	[Fact]
	public void RoundTrip_PreservesInitialSram() {
		var movie = MakeMovie();
		var sram = new byte[8192];
		// Fill with non-trivial pattern
		for (int i = 0; i < sram.Length; i++) {
			sram[i] = (byte)(i ^ 0xa5);
		}
		movie.InitialSram = sram;
		movie.StartType = StartType.Sram;

		var loaded = RoundTrip(movie);

		Assert.NotNull(loaded.InitialSram);
		Assert.Equal(sram.Length, loaded.InitialSram.Length);
		Assert.Equal(sram, loaded.InitialSram);
		Assert.Equal(StartType.Sram, loaded.StartType);
	}

	[Fact]
	public void RoundTrip_PreservesInitialRtc() {
		var movie = MakeMovie();
		movie.InitialRtc = [0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08];

		var loaded = RoundTrip(movie);

		Assert.NotNull(loaded.InitialRtc);
		Assert.Equal(movie.InitialRtc, loaded.InitialRtc);
	}

	[Fact]
	public void RoundTrip_NoBinaryData_NullPreserved() {
		var movie = MakeMovie();
		// Ensure no binary data is set
		movie.InitialState = null;
		movie.InitialSram = null;
		movie.InitialRtc = null;

		var loaded = RoundTrip(movie);

		Assert.Null(loaded.InitialState);
		Assert.Null(loaded.InitialSram);
		Assert.Null(loaded.InitialRtc);
	}

	[Fact]
	public void RoundTrip_AllBinaryDataTogether() {
		var movie = MakeMovie();
		movie.InitialState = [0xaa, 0xbb, 0xcc, 0xdd];
		movie.InitialSram = [0x11, 0x22, 0x33];
		movie.InitialRtc = [0x44, 0x55];
		movie.StartType = StartType.Savestate;

		var loaded = RoundTrip(movie);

		Assert.Equal(movie.InitialState, loaded.InitialState);
		Assert.Equal(movie.InitialSram, loaded.InitialSram);
		Assert.Equal(movie.InitialRtc, loaded.InitialRtc);
	}

	#endregion

	#region PortTypes Roundtrip

	[Fact]
	public void RoundTrip_PreservesPortTypes() {
		var movie = MakeMovie(SystemType.A2600);
		movie.PortTypes[0] = ControllerType.Atari2600Joystick;
		movie.PortTypes[1] = ControllerType.Atari2600Paddle;

		var loaded = RoundTrip(movie);

		Assert.Equal(ControllerType.Atari2600Joystick, loaded.PortTypes[0]);
		Assert.Equal(ControllerType.Atari2600Paddle, loaded.PortTypes[1]);
	}

	[Fact]
	public void RoundTrip_PreservesA2600SubtypePortTypes() {
		var movie = MakeMovie(SystemType.A2600);
		movie.PortTypes[0] = ControllerType.Atari2600Keypad;
		movie.PortTypes[1] = ControllerType.Atari2600DrivingController;

		var loaded = RoundTrip(movie);

		Assert.Equal(ControllerType.Atari2600Keypad, loaded.PortTypes[0]);
		Assert.Equal(ControllerType.Atari2600DrivingController, loaded.PortTypes[1]);
	}

	[Theory]
	[InlineData(ControllerType.Gamepad)]
	[InlineData(ControllerType.Atari2600Joystick)]
	[InlineData(ControllerType.Atari2600Paddle)]
	[InlineData(ControllerType.Atari2600Keypad)]
	[InlineData(ControllerType.Atari2600DrivingController)]
	[InlineData(ControllerType.Atari2600BoosterGrip)]
	public void RoundTrip_IndividualPortType_Preserved(ControllerType type) {
		var movie = MakeMovie(SystemType.A2600);
		movie.PortTypes[0] = type;

		var loaded = RoundTrip(movie);

		Assert.Equal(type, loaded.PortTypes[0]);
	}

	#endregion

	#region All System Types

	[Theory]
	[InlineData(SystemType.Nes)]
	[InlineData(SystemType.Snes)]
	[InlineData(SystemType.Gb)]
	[InlineData(SystemType.Gba)]
	[InlineData(SystemType.A2600)]
	[InlineData(SystemType.Genesis)]
	[InlineData(SystemType.Pce)]
	[InlineData(SystemType.Sms)]
	public void RoundTrip_AllSystems_PreservesInputAndMetadata(SystemType system) {
		var movie = MakeMovie(system, 20);

		var loaded = RoundTrip(movie);

		Assert.Equal(system, loaded.SystemType);
		Assert.Equal(20, loaded.TotalFrames);
		Assert.Equal("Integration Tester", loaded.Author);

		// Verify input integrity
		for (int i = 0; i < 20; i++) {
			Assert.Equal(
				movie.InputFrames[i].Controllers[0].ButtonBits,
				loaded.InputFrames[i].Controllers[0].ButtonBits);
		}
	}

	#endregion

	#region Extended Metadata

	[Fact]
	public void RoundTrip_PreservesHashMetadata() {
		var movie = MakeMovie();
		movie.Sha1Hash = "da39a3ee5e6b4b0d3255bfef95601890afd80709";
		movie.Sha256Hash = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
		movie.Md5Hash = "d41d8cd98f00b204e9800998ecf8427e";
		movie.Crc32 = 0xdeadbeef;
		movie.RomSize = 262144;

		var loaded = RoundTrip(movie);

		Assert.Equal(movie.Sha1Hash, loaded.Sha1Hash);
		Assert.Equal(movie.Sha256Hash, loaded.Sha256Hash);
		Assert.Equal(movie.Md5Hash, loaded.Md5Hash);
		Assert.Equal(movie.Crc32, loaded.Crc32);
		Assert.Equal(262144L, loaded.RomSize);
	}

	[Fact]
	public void RoundTrip_PreservesVerificationData() {
		var movie = MakeMovie();
		movie.IsVerified = true;
		movie.VerificationNotes = "Verified on real hardware 2026-03-24";
		movie.FinalStateHash = "abc123def456";

		var loaded = RoundTrip(movie);

		Assert.True(loaded.IsVerified);
		Assert.Equal(movie.VerificationNotes, loaded.VerificationNotes);
		Assert.Equal(movie.FinalStateHash, loaded.FinalStateHash);
	}

	[Fact]
	public void RoundTrip_PreservesTimingAndSyncMode() {
		var movie = MakeMovie();
		movie.TimingMode = TimingMode.InputFrames;
		movie.SyncMode = SyncMode.Cycle;

		var loaded = RoundTrip(movie);

		Assert.Equal(TimingMode.InputFrames, loaded.TimingMode);
		Assert.Equal(SyncMode.Cycle, loaded.SyncMode);
	}

	[Fact]
	public void RoundTrip_PreservesVsSystemData() {
		var movie = MakeMovie(SystemType.Nes);
		movie.VsDipSwitches = 0x42;
		movie.VsPpuType = 3;

		var loaded = RoundTrip(movie);

		Assert.Equal((byte)0x42, loaded.VsDipSwitches);
		Assert.Equal((byte)3, loaded.VsPpuType);
	}

	[Fact]
	public void RoundTrip_PreservesExtraData() {
		var movie = MakeMovie();
		movie.ExtraData = new Dictionary<string, string> {
			["custom_key"] = "custom_value",
			["tool_version"] = "1.0.0"
		};

		var loaded = RoundTrip(movie);

		Assert.NotNull(loaded.ExtraData);
		Assert.Equal("custom_value", loaded.ExtraData["custom_key"]);
		Assert.Equal("1.0.0", loaded.ExtraData["tool_version"]);
	}

	[Fact]
	public void RoundTrip_PreservesControllersSwapped() {
		var movie = MakeMovie();
		movie.ControllersSwapped = true;

		var loaded = RoundTrip(movie);

		Assert.True(loaded.ControllersSwapped);
	}

	[Fact]
	public void RoundTrip_PreservesLagFrameAndBlankFrameCounts() {
		var movie = MakeMovie();
		movie.LagFrameCount = 42;
		movie.BlankFrames = 7;

		var loaded = RoundTrip(movie);

		Assert.Equal(42UL, loaded.LagFrameCount);
		Assert.Equal(7, loaded.BlankFrames);
	}

	[Fact]
	public void RoundTrip_PreservesBiosHash() {
		var movie = MakeMovie(SystemType.Gba);
		movie.BiosHash = "gba_bios_hash_value";

		var loaded = RoundTrip(movie);

		Assert.Equal("gba_bios_hash_value", loaded.BiosHash);
	}

	#endregion

	#region Full Feature Roundtrip

	[Fact]
	public void RoundTrip_AllFeaturesCombined() {
		var movie = MakeMovie(SystemType.Snes, 50);
		movie.Sha1Hash = "abcdef1234567890";
		movie.RerecordCount = 9999;
		movie.IsVerified = true;
		movie.VerificationNotes = "Full feature test";
		movie.TimingMode = TimingMode.InputFrames;
		movie.SyncMode = SyncMode.Cycle;
		movie.ControllersSwapped = true;
		movie.LagFrameCount = 10;
		movie.BlankFrames = 2;
		movie.InitialState = new byte[256];
		for (int i = 0; i < 256; i++) movie.InitialState[i] = (byte)i;
		movie.InitialSram = [0x01, 0x02, 0x03];
		movie.InitialRtc = [0x10, 0x20];
		movie.StartType = StartType.Savestate;
		movie.Markers =
		[
			new MovieMarker { FrameNumber = 0, Label = "Start" },
			new MovieMarker { FrameNumber = 25, Label = "Mid" },
			new MovieMarker { FrameNumber = 49, Label = "End" }
		];
		movie.Scripts =
		[
			new EmbeddedScript { Name = "overlay.lua", Content = "gui.text(0,0,'test')", Language = "lua" }
		];
		movie.ExtraData = new Dictionary<string, string> { ["k"] = "v" };

		var loaded = RoundTrip(movie);

		// Metadata
		Assert.Equal(SystemType.Snes, loaded.SystemType);
		Assert.Equal(50, loaded.TotalFrames);
		Assert.Equal("abcdef1234567890", loaded.Sha1Hash);
		Assert.Equal(9999UL, loaded.RerecordCount);
		Assert.True(loaded.IsVerified);
		Assert.Equal(TimingMode.InputFrames, loaded.TimingMode);
		Assert.Equal(SyncMode.Cycle, loaded.SyncMode);
		Assert.True(loaded.ControllersSwapped);
		Assert.Equal(10UL, loaded.LagFrameCount);
		Assert.Equal(2, loaded.BlankFrames);

		// Binary data
		Assert.NotNull(loaded.InitialState);
		Assert.Equal(256, loaded.InitialState.Length);
		Assert.Equal(movie.InitialState, loaded.InitialState);
		Assert.Equal(movie.InitialSram, loaded.InitialSram);
		Assert.Equal(movie.InitialRtc, loaded.InitialRtc);
		Assert.Equal(StartType.Savestate, loaded.StartType);

		// Markers
		Assert.NotNull(loaded.Markers);
		Assert.Equal(3, loaded.Markers.Count);

		// Scripts
		Assert.NotNull(loaded.Scripts);
		Assert.Single(loaded.Scripts);

		// Extra data
		Assert.NotNull(loaded.ExtraData);
		Assert.Equal("v", loaded.ExtraData["k"]);

		// Input integrity
		for (int i = 0; i < 50; i++) {
			Assert.Equal(
				movie.InputFrames[i].Controllers[0].ButtonBits,
				loaded.InputFrames[i].Controllers[0].ButtonBits);
		}
	}

	#endregion

	#region ZIP Archive Structure Verification

	[Fact]
	public void Write_CreatesValidZipWithExpectedEntries() {
		var movie = MakeMovie();
		movie.Markers = [new MovieMarker { FrameNumber = 0, Label = "Test" }];
		movie.Scripts = [new EmbeddedScript { Name = "test.lua", Content = "-- test", Language = "lua" }];
		movie.InitialState = [0x01];
		movie.InitialSram = [0x02];
		movie.InitialRtc = [0x03];
		movie.StartType = StartType.Savestate;

		using var stream = new MemoryStream();
		Converter.Write(movie, stream);
		stream.Position = 0;

		using var archive = new ZipArchive(stream, ZipArchiveMode.Read);
		var entryNames = archive.Entries.Select(e => e.FullName).ToHashSet();

		Assert.Contains("movie.json", entryNames);
		Assert.Contains("input.txt", entryNames);
		Assert.Contains("markers.json", entryNames);
		Assert.Contains("savestate.bin", entryNames);
		Assert.Contains("sram.bin", entryNames);
		Assert.Contains("rtc.bin", entryNames);
		Assert.Contains("scripts/test.lua", entryNames);
	}

	[Fact]
	public void Write_MinimalMovie_OnlyRequiredEntries() {
		var movie = MakeMovie();

		using var stream = new MemoryStream();
		Converter.Write(movie, stream);
		stream.Position = 0;

		using var archive = new ZipArchive(stream, ZipArchiveMode.Read);
		var entryNames = archive.Entries.Select(e => e.FullName).ToHashSet();

		Assert.Contains("movie.json", entryNames);
		Assert.Contains("input.txt", entryNames);
		Assert.DoesNotContain("savestate.bin", entryNames);
		Assert.DoesNotContain("sram.bin", entryNames);
		Assert.DoesNotContain("rtc.bin", entryNames);
		Assert.DoesNotContain("markers.json", entryNames);
	}

	#endregion

	#region Cross-Format Conversion Fidelity

	[Fact]
	public void CrossFormat_NexenToBk2_PreservesInputFrames() {
		var movie = MakeMovie(SystemType.Snes, 30);
		var bk2 = new Converters.Bk2MovieConverter();

		// Write as Nexen, read back
		using var nexenStream = new MemoryStream();
		Converter.Write(movie, nexenStream);
		nexenStream.Position = 0;
		var fromNexen = Converter.Read(nexenStream, "test.nexen-movie");

		// Write as BK2, read back
		using var bk2Stream = new MemoryStream();
		bk2.Write(fromNexen, bk2Stream);
		bk2Stream.Position = 0;
		var fromBk2 = bk2.Read(bk2Stream, "test.bk2");

		Assert.Equal(30, fromBk2.TotalFrames);
		for (int i = 0; i < 30; i++) {
			Assert.Equal(
				movie.InputFrames[i].Controllers[0].A,
				fromBk2.InputFrames[i].Controllers[0].A);
			Assert.Equal(
				movie.InputFrames[i].Controllers[0].B,
				fromBk2.InputFrames[i].Controllers[0].B);
			Assert.Equal(
				movie.InputFrames[i].Controllers[0].Up,
				fromBk2.InputFrames[i].Controllers[0].Up);
		}
	}

	[Fact]
	public void CrossFormat_NexenToFm2_PreservesNesInput() {
		var movie = MakeMovie(SystemType.Nes, 20);
		var fm2 = new Converters.Fm2MovieConverter();

		using var nexenStream = new MemoryStream();
		Converter.Write(movie, nexenStream);
		nexenStream.Position = 0;
		var fromNexen = Converter.Read(nexenStream, "test.nexen-movie");

		using var fm2Stream = new MemoryStream();
		fm2.Write(fromNexen, fm2Stream);
		fm2Stream.Position = 0;
		var fromFm2 = fm2.Read(fm2Stream, "test.fm2");

		Assert.Equal(20, fromFm2.TotalFrames);
		for (int i = 0; i < 20; i++) {
			Assert.Equal(
				movie.InputFrames[i].Controllers[0].ButtonBits,
				fromFm2.InputFrames[i].Controllers[0].ButtonBits);
		}
	}

	#endregion

	#region Edge Cases

	[Fact]
	public void RoundTrip_UnicodeMetadata_Preserved() {
		var movie = MakeMovie();
		movie.Author = "テスター 🎮";
		movie.Description = "Ünïcödé tëst with émojis 🌸🌺";
		movie.GameName = "ゲーム名";

		var loaded = RoundTrip(movie);

		Assert.Equal(movie.Author, loaded.Author);
		Assert.Equal(movie.Description, loaded.Description);
		Assert.Equal(movie.GameName, loaded.GameName);
	}

	[Fact]
	public void RoundTrip_EmptyStringMetadata_Preserved() {
		var movie = MakeMovie();
		movie.Author = "";
		movie.Description = "";
		movie.GameName = "";

		var loaded = RoundTrip(movie);

		Assert.Equal("", loaded.Author);
		Assert.Equal("", loaded.Description);
		Assert.Equal("", loaded.GameName);
	}

	[Fact]
	public void RoundTrip_LargeBinaryData_Preserved() {
		var movie = MakeMovie();
		// 64KB savestate
		var state = new byte[65536];
		Random.Shared.NextBytes(state);
		movie.InitialState = state;
		movie.StartType = StartType.Savestate;

		var loaded = RoundTrip(movie);

		Assert.NotNull(loaded.InitialState);
		Assert.Equal(state.Length, loaded.InitialState.Length);
		Assert.Equal(state, loaded.InitialState);
	}

	#endregion
}
