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
	public void NexenConverter_RoundTripsAtari2600PortTypes() {
		var movie = new MovieData {
			SystemType = SystemType.A2600,
			ControllerCount = 2
		};
		movie.PortTypes[0] = ControllerType.Atari2600Keypad;
		movie.PortTypes[1] = ControllerType.Atari2600DrivingController;

		using var stream = new MemoryStream();
		var converter = new Converters.NexenMovieConverter();
		converter.Write(movie, stream);

		stream.Position = 0;
		var loaded = converter.Read(stream);

		Assert.Equal(SystemType.A2600, loaded.SystemType);
		Assert.Equal(ControllerType.Atari2600Keypad, loaded.PortTypes[0]);
		Assert.Equal(ControllerType.Atari2600DrivingController, loaded.PortTypes[1]);
	}

	[Fact]
	public void NexenConverter_RoundTripsAtari2600Metadata() {
		var movie = new MovieData {
			SystemType = SystemType.A2600,
			ControllerCount = 2,
			ControllersSwapped = true,
			Region = RegionType.NTSC,
			GameName = "Atari 2600 Metadata Test"
		};

		movie.PortTypes[0] = ControllerType.Atari2600BoosterGrip;
		movie.PortTypes[1] = ControllerType.Atari2600Paddle;
		movie.ExtraData = new Dictionary<string, string> {
			["Atari2600.Mapper"] = "F8",
			["Atari2600.ControllerProfile"] = "booster+paddle"
		};

		using var stream = new MemoryStream();
		var converter = new Converters.NexenMovieConverter();
		converter.Write(movie, stream);

		stream.Position = 0;
		var loaded = converter.Read(stream);

		Assert.Equal(SystemType.A2600, loaded.SystemType);
		Assert.True(loaded.ControllersSwapped);
		Assert.Equal(ControllerType.Atari2600BoosterGrip, loaded.PortTypes[0]);
		Assert.Equal(ControllerType.Atari2600Paddle, loaded.PortTypes[1]);
		Assert.NotNull(loaded.ExtraData);
		Assert.Equal("F8", loaded.ExtraData!["Atari2600.Mapper"]);
		Assert.Equal("booster+paddle", loaded.ExtraData["Atari2600.ControllerProfile"]);
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

	[Fact]
	public void NexenConverter_RoundTripsAtari2600JoystickInput() {
		var movie = new MovieData {
			SystemType = SystemType.A2600,
			ControllerCount = 1
		};
		movie.PortTypes[0] = ControllerType.Atari2600Joystick;

		var frame = new InputFrame(0);
		frame.Controllers[0].SetButton("FIRE", true);
		frame.Controllers[0].SetButton("UP", true);
		frame.Controllers[0].SetButton("RIGHT", true);
		frame.Controllers[0].Type = ControllerType.Atari2600Joystick;
		movie.AddFrame(frame);

		using var stream = new MemoryStream();
		var converter = new Converters.NexenMovieConverter();
		converter.Write(movie, stream);

		stream.Position = 0;
		var loaded = converter.Read(stream);

		var ctrl = loaded.InputFrames[0].Controllers[0];
		Assert.True(ctrl.A, "Fire (A) should be set");
		Assert.True(ctrl.Up, "Up should be set");
		Assert.True(ctrl.Right, "Right should be set");
		Assert.False(ctrl.Down, "Down should not be set");
		Assert.False(ctrl.Left, "Left should not be set");
		Assert.False(ctrl.B, "B should not be set");
	}

	[Fact]
	public void NexenConverter_RoundTripsAtari2600PaddleInput() {
		var movie = new MovieData {
			SystemType = SystemType.A2600,
			ControllerCount = 1
		};
		movie.PortTypes[0] = ControllerType.Atari2600Paddle;

		var frame = new InputFrame(0);
		frame.Controllers[0].SetButton("FIRE", true);
		frame.Controllers[0].PaddlePosition = 128;
		frame.Controllers[0].Type = ControllerType.Atari2600Paddle;
		movie.AddFrame(frame);

		using var stream = new MemoryStream();
		var converter = new Converters.NexenMovieConverter();
		converter.Write(movie, stream);

		stream.Position = 0;
		var loaded = converter.Read(stream);

		var ctrl = loaded.InputFrames[0].Controllers[0];
		Assert.True(ctrl.A, "Fire (A) should be set");
		Assert.Equal((byte)128, ctrl.PaddlePosition);
	}

	[Fact]
	public void NexenConverter_RoundTripsAtari2600KeypadInput() {
		var movie = new MovieData {
			SystemType = SystemType.A2600,
			ControllerCount = 1
		};
		movie.PortTypes[0] = ControllerType.Atari2600Keypad;

		// Keys map: 1→Y, 5→A, POUND→Right
		var frame = new InputFrame(0);
		frame.Controllers[0].SetButton("1", true);     // → Y
		frame.Controllers[0].SetButton("5", true);     // → A
		frame.Controllers[0].SetButton("POUND", true); // → Right
		frame.Controllers[0].Type = ControllerType.Atari2600Keypad;
		movie.AddFrame(frame);

		using var stream = new MemoryStream();
		var converter = new Converters.NexenMovieConverter();
		converter.Write(movie, stream);

		stream.Position = 0;
		var loaded = converter.Read(stream);

		var ctrl = loaded.InputFrames[0].Controllers[0];
		Assert.True(ctrl.Y, "Key 1 (Y) should be set");
		Assert.True(ctrl.A, "Key 5 (A) should be set");
		Assert.True(ctrl.Right, "Key # (Right) should be set");
		Assert.False(ctrl.B, "Key 4 (B) should not be set");
		Assert.False(ctrl.X, "Key 2 (X) should not be set");
	}

	[Fact]
	public void NexenConverter_RoundTripsAtari2600DrivingInput() {
		var movie = new MovieData {
			SystemType = SystemType.A2600,
			ControllerCount = 1
		};
		movie.PortTypes[0] = ControllerType.Atari2600DrivingController;

		var frame = new InputFrame(0);
		frame.Controllers[0].SetButton("FIRE", true);
		frame.Controllers[0].SetButton("LEFT", true);
		frame.Controllers[0].Type = ControllerType.Atari2600DrivingController;
		movie.AddFrame(frame);

		using var stream = new MemoryStream();
		var converter = new Converters.NexenMovieConverter();
		converter.Write(movie, stream);

		stream.Position = 0;
		var loaded = converter.Read(stream);

		var ctrl = loaded.InputFrames[0].Controllers[0];
		Assert.True(ctrl.A, "Fire (A) should be set");
		Assert.True(ctrl.Left, "Left should be set");
		Assert.False(ctrl.Right, "Right should not be set");
	}

	[Fact]
	public void NexenConverter_RoundTripsAtari2600BoosterGripInput() {
		var movie = new MovieData {
			SystemType = SystemType.A2600,
			ControllerCount = 1
		};
		movie.PortTypes[0] = ControllerType.Atari2600BoosterGrip;

		var frame = new InputFrame(0);
		frame.Controllers[0].SetButton("FIRE", true);
		frame.Controllers[0].SetButton("TRIGGER", true);
		frame.Controllers[0].SetButton("UP", true);
		frame.Controllers[0].Type = ControllerType.Atari2600BoosterGrip;
		movie.AddFrame(frame);

		using var stream = new MemoryStream();
		var converter = new Converters.NexenMovieConverter();
		converter.Write(movie, stream);

		stream.Position = 0;
		var loaded = converter.Read(stream);

		var ctrl = loaded.InputFrames[0].Controllers[0];
		Assert.True(ctrl.A, "Fire (A) should be set");
		Assert.True(ctrl.B, "Trigger (B) should be set");
		Assert.True(ctrl.Up, "Up should be set");
		Assert.False(ctrl.Down, "Down should not be set");
		Assert.False(ctrl.Left, "Left should not be set");
	}

	/// <summary>
	/// Verifies that extended buttons (C/Z/Mode) are NOT preserved by the current
	/// 12-char NexenFormat text serializer. This documents the known limitation
	/// that Booster (C) for BoosterGrip does not survive text roundtrip.
	/// </summary>
	[Fact]
	public void NexenConverter_ExtendedButtonsNotPreservedInTextFormat() {
		var movie = new MovieData {
			SystemType = SystemType.A2600,
			ControllerCount = 1
		};
		movie.PortTypes[0] = ControllerType.Atari2600BoosterGrip;

		var frame = new InputFrame(0);
		frame.Controllers[0].C = true; // Booster
		frame.Controllers[0].Type = ControllerType.Atari2600BoosterGrip;
		movie.AddFrame(frame);

		using var stream = new MemoryStream();
		var converter = new Converters.NexenMovieConverter();
		converter.Write(movie, stream);

		stream.Position = 0;
		var loaded = converter.Read(stream);

		// C (Booster) is NOT in the 12-char "BYsSUDLRAXLR" text format
		Assert.False(loaded.InputFrames[0].Controllers[0].C,
			"Extended button C is not preserved in current text format");
	}

	[Fact]
	public void NexenConverter_RoundTripsAtari2600DualPortInput() {
		var movie = new MovieData {
			SystemType = SystemType.A2600,
			ControllerCount = 2
		};
		movie.PortTypes[0] = ControllerType.Atari2600Joystick;
		movie.PortTypes[1] = ControllerType.Atari2600Paddle;

		var frame = new InputFrame(0);
		// Port 0: Joystick with Fire+Down
		frame.Controllers[0].SetButton("FIRE", true);
		frame.Controllers[0].SetButton("DOWN", true);
		frame.Controllers[0].Type = ControllerType.Atari2600Joystick;
		// Port 1: Paddle with Fire + position 200
		frame.Controllers[1].SetButton("FIRE", true);
		frame.Controllers[1].PaddlePosition = 200;
		frame.Controllers[1].Type = ControllerType.Atari2600Paddle;
		movie.AddFrame(frame);

		using var stream = new MemoryStream();
		var converter = new Converters.NexenMovieConverter();
		converter.Write(movie, stream);

		stream.Position = 0;
		var loaded = converter.Read(stream);

		// Port 0 — Joystick
		var ctrl0 = loaded.InputFrames[0].Controllers[0];
		Assert.True(ctrl0.A, "P1 Fire (A) should be set");
		Assert.True(ctrl0.Down, "P1 Down should be set");
		Assert.False(ctrl0.Up, "P1 Up should not be set");

		// Port 1 — Paddle
		var ctrl1 = loaded.InputFrames[0].Controllers[1];
		Assert.True(ctrl1.A, "P2 Fire (A) should be set");
		Assert.Equal((byte)200, ctrl1.PaddlePosition);
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
