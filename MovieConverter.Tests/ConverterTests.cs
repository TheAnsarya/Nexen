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
		frame.Controllers[0].SetButton("BOOSTER", true);
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
		Assert.True(ctrl.C, "Booster (C) should be set");
		Assert.True(ctrl.Up, "Up should be set");
		Assert.False(ctrl.Down, "Down should not be set");
		Assert.False(ctrl.Left, "Left should not be set");
	}

	/// <summary>
	/// Verifies that extended buttons (C/Z/Mode) ARE preserved by the expanded
	/// 15-char NexenFormat text serializer (BYsSUDLRAXLRcZM).
	/// </summary>
	[Fact]
	public void NexenConverter_ExtendedButtonsPreservedInTextFormat() {
		var movie = new MovieData {
			SystemType = SystemType.A2600,
			ControllerCount = 1
		};
		movie.PortTypes[0] = ControllerType.Atari2600BoosterGrip;

		var frame = new InputFrame(0);
		frame.Controllers[0].C = true; // Booster
		frame.Controllers[0].Z = true;
		frame.Controllers[0].Mode = true;
		frame.Controllers[0].Type = ControllerType.Atari2600BoosterGrip;
		movie.AddFrame(frame);

		using var stream = new MemoryStream();
		var converter = new Converters.NexenMovieConverter();
		converter.Write(movie, stream);

		stream.Position = 0;
		var loaded = converter.Read(stream);

		var ctrl = loaded.InputFrames[0].Controllers[0];
		Assert.True(ctrl.C, "C (Booster) should be preserved in 15-char format");
		Assert.True(ctrl.Z, "Z should be preserved in 15-char format");
		Assert.True(ctrl.Mode, "Mode should be preserved in 15-char format");
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

	[Fact]
	public void NexenConverter_RoundTripsA2600ConsoleSwitchesAndPaddle() {
		var movie = new MovieData {
			SystemType = SystemType.A2600,
			ControllerCount = 2
		};
		movie.PortTypes[0] = ControllerType.Atari2600Paddle;
		movie.PortTypes[1] = ControllerType.Atari2600Joystick;

		// Frame 0: paddle fire + position 64, Select pressed, P2 joystick up
		var frame0 = new InputFrame(0);
		frame0.Controllers[0].SetButton("FIRE", true);
		frame0.Controllers[0].PaddlePosition = 64;
		frame0.Controllers[0].Type = ControllerType.Atari2600Paddle;
		frame0.Controllers[1].SetButton("UP", true);
		frame0.Controllers[1].Type = ControllerType.Atari2600Joystick;
		frame0.Command = FrameCommand.Atari2600Select;
		movie.AddFrame(frame0);

		// Frame 1: paddle position 200, Reset pressed, no buttons
		var frame1 = new InputFrame(1);
		frame1.Controllers[0].PaddlePosition = 200;
		frame1.Controllers[0].Type = ControllerType.Atari2600Paddle;
		frame1.Controllers[1].Type = ControllerType.Atari2600Joystick;
		frame1.Command = FrameCommand.Atari2600Reset;
		movie.AddFrame(frame1);

		// Frame 2: both switches, paddle at 0, P2 fire+down
		var frame2 = new InputFrame(2);
		frame2.Controllers[0].PaddlePosition = 0;
		frame2.Controllers[0].Type = ControllerType.Atari2600Paddle;
		frame2.Controllers[1].SetButton("FIRE", true);
		frame2.Controllers[1].SetButton("DOWN", true);
		frame2.Controllers[1].Type = ControllerType.Atari2600Joystick;
		frame2.Command = FrameCommand.Atari2600Select | FrameCommand.Atari2600Reset;
		movie.AddFrame(frame2);

		using var stream = new MemoryStream();
		var converter = new Converters.NexenMovieConverter();
		converter.Write(movie, stream);

		stream.Position = 0;
		var loaded = converter.Read(stream);

		Assert.Equal(3, loaded.TotalFrames);

		// Frame 0: paddle + fire + Select
		Assert.True(loaded.InputFrames[0].Controllers[0].A, "P1 Fire");
		Assert.Equal((byte)64, loaded.InputFrames[0].Controllers[0].PaddlePosition);
		Assert.True(loaded.InputFrames[0].Controllers[1].Up, "P2 Up");
		Assert.True(loaded.InputFrames[0].Command.HasFlag(FrameCommand.Atari2600Select));
		Assert.False(loaded.InputFrames[0].Command.HasFlag(FrameCommand.Atari2600Reset));

		// Frame 1: paddle + Reset
		Assert.Equal((byte)200, loaded.InputFrames[1].Controllers[0].PaddlePosition);
		Assert.True(loaded.InputFrames[1].Command.HasFlag(FrameCommand.Atari2600Reset));
		Assert.False(loaded.InputFrames[1].Command.HasFlag(FrameCommand.Atari2600Select));

		// Frame 2: paddle + both switches + P2 fire+down
		Assert.Equal((byte)0, loaded.InputFrames[2].Controllers[0].PaddlePosition);
		Assert.True(loaded.InputFrames[2].Controllers[1].A, "P2 Fire");
		Assert.True(loaded.InputFrames[2].Controllers[1].Down, "P2 Down");
		Assert.True(loaded.InputFrames[2].Command.HasFlag(FrameCommand.Atari2600Select));
		Assert.True(loaded.InputFrames[2].Command.HasFlag(FrameCommand.Atari2600Reset));
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

	#region BK2 Atari 2600 Console Switches

	[Fact]
	public void Bk2Converter_ParsesA2600JoystickInput() {
		// BK2 A2600 format: |command|UDLRB|UDLRB|Sr|
		using var stream = new MemoryStream();
		using (var archive = new ZipArchive(stream, ZipArchiveMode.Create, leaveOpen: true)) {
			var headerEntry = archive.CreateEntry("Header.txt");
			using (var writer = new StreamWriter(headerEntry.Open())) {
				writer.WriteLine("Platform A26");
				writer.WriteLine("GameName Test A2600");
			}

			var inputEntry = archive.CreateEntry("Input Log.txt");
			using (var writer = new StreamWriter(inputEntry.Open())) {
				writer.WriteLine("[Input]");
				writer.WriteLine("|.|U.LR.|.....|..|");
				writer.WriteLine("|.|....B|UD...|..|");
			}
		}

		stream.Position = 0;
		var converter = new Converters.Bk2MovieConverter();
		var movie = converter.Read(stream, "test.bk2");

		Assert.Equal(SystemType.A2600, movie.SystemType);
		Assert.Equal(2, movie.TotalFrames);

		// Frame 0: P1 has Up+Left+Right
		Assert.True(movie.InputFrames[0].Controllers[0].Up);
		Assert.False(movie.InputFrames[0].Controllers[0].Down);
		Assert.True(movie.InputFrames[0].Controllers[0].Left);
		Assert.True(movie.InputFrames[0].Controllers[0].Right);
		Assert.False(movie.InputFrames[0].Controllers[0].A); // Fire

		// Frame 1: P1 has Fire, P2 has Up+Down
		Assert.True(movie.InputFrames[1].Controllers[0].A);
		Assert.True(movie.InputFrames[1].Controllers[1].Up);
		Assert.True(movie.InputFrames[1].Controllers[1].Down);
	}

	[Fact]
	public void Bk2Converter_ParsesA2600ConsoleSwitches() {
		using var stream = new MemoryStream();
		using (var archive = new ZipArchive(stream, ZipArchiveMode.Create, leaveOpen: true)) {
			var headerEntry = archive.CreateEntry("Header.txt");
			using (var writer = new StreamWriter(headerEntry.Open())) {
				writer.WriteLine("Platform A26");
			}

			var inputEntry = archive.CreateEntry("Input Log.txt");
			using (var writer = new StreamWriter(inputEntry.Open())) {
				writer.WriteLine("[Input]");
				writer.WriteLine("|.|.....|.....|..|");  // No switches
				writer.WriteLine("|.|.....|.....|S.|");  // Select only
				writer.WriteLine("|.|.....|.....|.r|");  // Reset only
				writer.WriteLine("|.|.....|.....|Sr|");  // Both
			}
		}

		stream.Position = 0;
		var converter = new Converters.Bk2MovieConverter();
		var movie = converter.Read(stream, "test.bk2");

		Assert.Equal(4, movie.TotalFrames);

		// Frame 0: no switches
		Assert.Equal(FrameCommand.None, movie.InputFrames[0].Command);

		// Frame 1: Select only
		Assert.True(movie.InputFrames[1].Command.HasFlag(FrameCommand.Atari2600Select));
		Assert.False(movie.InputFrames[1].Command.HasFlag(FrameCommand.Atari2600Reset));

		// Frame 2: Reset only
		Assert.False(movie.InputFrames[2].Command.HasFlag(FrameCommand.Atari2600Select));
		Assert.True(movie.InputFrames[2].Command.HasFlag(FrameCommand.Atari2600Reset));

		// Frame 3: Both
		Assert.True(movie.InputFrames[3].Command.HasFlag(FrameCommand.Atari2600Select));
		Assert.True(movie.InputFrames[3].Command.HasFlag(FrameCommand.Atari2600Reset));
	}

	[Fact]
	public void Bk2Converter_A2600_RoundTripsConsoleSwitches() {
		var original = new MovieData {
			SystemType = SystemType.A2600,
			ControllerCount = 2,
			GameName = "Console Switch Roundtrip"
		};

		// Frame 0: normal input, no switches
		var frame0 = new InputFrame(0);
		frame0.Controllers[0].Up = true;
		frame0.Controllers[0].A = true; // Fire
		original.AddFrame(frame0);

		// Frame 1: Select pressed
		var frame1 = new InputFrame(1);
		frame1.Command = FrameCommand.Atari2600Select;
		original.AddFrame(frame1);

		// Frame 2: Reset pressed with joystick input
		var frame2 = new InputFrame(2);
		frame2.Command = FrameCommand.Atari2600Reset;
		frame2.Controllers[0].Down = true;
		frame2.Controllers[1].Left = true;
		original.AddFrame(frame2);

		// Frame 3: Both switches
		var frame3 = new InputFrame(3);
		frame3.Command = FrameCommand.Atari2600Select | FrameCommand.Atari2600Reset;
		original.AddFrame(frame3);

		// Write and read back
		using var stream = new MemoryStream();
		var converter = new Converters.Bk2MovieConverter();
		converter.Write(original, stream);

		stream.Position = 0;
		var loaded = converter.Read(stream, "test.bk2");

		Assert.Equal(SystemType.A2600, loaded.SystemType);
		Assert.Equal(4, loaded.TotalFrames);

		// Frame 0: input preserved, no switches
		Assert.True(loaded.InputFrames[0].Controllers[0].Up);
		Assert.True(loaded.InputFrames[0].Controllers[0].A);
		Assert.False(loaded.InputFrames[0].Command.HasFlag(FrameCommand.Atari2600Select));
		Assert.False(loaded.InputFrames[0].Command.HasFlag(FrameCommand.Atari2600Reset));

		// Frame 1: Select
		Assert.True(loaded.InputFrames[1].Command.HasFlag(FrameCommand.Atari2600Select));
		Assert.False(loaded.InputFrames[1].Command.HasFlag(FrameCommand.Atari2600Reset));

		// Frame 2: Reset + joystick input
		Assert.True(loaded.InputFrames[2].Command.HasFlag(FrameCommand.Atari2600Reset));
		Assert.True(loaded.InputFrames[2].Controllers[0].Down);
		Assert.True(loaded.InputFrames[2].Controllers[1].Left);

		// Frame 3: Both
		Assert.True(loaded.InputFrames[3].Command.HasFlag(FrameCommand.Atari2600Select));
		Assert.True(loaded.InputFrames[3].Command.HasFlag(FrameCommand.Atari2600Reset));
	}

	[Fact]
	public void Bk2Converter_A2600_FormatIncludesConsoleSwitchColumn() {
		var movie = new MovieData {
			SystemType = SystemType.A2600,
			ControllerCount = 1
		};

		var frame = new InputFrame(0);
		frame.Controllers[0].Up = true;
		frame.Controllers[0].A = true;
		frame.Command = FrameCommand.Atari2600Select;
		movie.AddFrame(frame);

		using var stream = new MemoryStream();
		var converter = new Converters.Bk2MovieConverter();
		converter.Write(movie, stream);

		// Read the Input Log.txt content from the BK2 ZIP
		stream.Position = 0;
		using var archive = new ZipArchive(stream, ZipArchiveMode.Read);
		var inputEntry = archive.GetEntry("Input Log.txt")!;
		using var reader = new StreamReader(inputEntry.Open());
		string content = reader.ReadToEnd();

		// Should contain the console switches column (S. = Select pressed, no Reset)
		Assert.Contains("S.", content);
		// Should contain proper A2600 button format (U...B = Up + Fire)
		Assert.Contains("U...B", content);
	}

	[Fact]
	public void Bk2Converter_A2600_NoConsoleSwitchesWhenNotA2600() {
		var movie = new MovieData {
			SystemType = SystemType.Nes,
			ControllerCount = 2
		};

		var frame = new InputFrame(0);
		frame.Controllers[0].A = true;
		movie.AddFrame(frame);

		using var stream = new MemoryStream();
		var converter = new Converters.Bk2MovieConverter();
		converter.Write(movie, stream);

		// Read the Input Log.txt content
		stream.Position = 0;
		using var archive = new ZipArchive(stream, ZipArchiveMode.Read);
		var inputEntry = archive.GetEntry("Input Log.txt")!;
		using var reader = new StreamReader(inputEntry.Open());
		string content = reader.ReadToEnd();

		// NES format should have |.|RLDUTSBA|RLDUTSBA| — no extra console switch column
		string[] lines = content.Split('\n', StringSplitOptions.RemoveEmptyEntries);
		// Input line should have exactly 4 pipe segments (|cmd|P1|P2|)
		string inputLine = lines.First(l => l.StartsWith("|."));
		int pipeCount = inputLine.Count(c => c == '|');
		Assert.Equal(4, pipeCount); // |cmd|P1|P2|
	}

	#endregion

	#region BK2 Atari 2600 Paddle Position

	[Fact]
	public void Bk2Converter_ParsesA2600PaddlePosition() {
		// BK2 A2600 paddle format: |command|UDLRB,PPP|UDLRB|Sr|
		using var stream = new MemoryStream();
		using (var archive = new ZipArchive(stream, ZipArchiveMode.Create, leaveOpen: true)) {
			var headerEntry = archive.CreateEntry("Header.txt");
			using (var writer = new StreamWriter(headerEntry.Open())) {
				writer.WriteLine("Platform A26");
			}

			var inputEntry = archive.CreateEntry("Input Log.txt");
			using (var writer = new StreamWriter(inputEntry.Open())) {
				writer.WriteLine("[Input]");
				writer.WriteLine("|.|....B,  128|.....|..|");  // P1 paddle at 128 with fire
				writer.WriteLine("|.|UDLRB,    0|.....|..|");  // P1 all dirs + fire, paddle at 0
				writer.WriteLine("|.|.....,  255|.....|..|");  // P1 no buttons, paddle at 255
			}
		}

		stream.Position = 0;
		var converter = new Converters.Bk2MovieConverter();
		var movie = converter.Read(stream, "test.bk2");

		Assert.Equal(3, movie.TotalFrames);

		// Frame 0: fire + paddle 128
		Assert.True(movie.InputFrames[0].Controllers[0].A);
		Assert.Equal((byte)128, movie.InputFrames[0].Controllers[0].PaddlePosition);

		// Frame 1: all directions + fire + paddle 0
		Assert.True(movie.InputFrames[1].Controllers[0].Up);
		Assert.True(movie.InputFrames[1].Controllers[0].Down);
		Assert.True(movie.InputFrames[1].Controllers[0].Left);
		Assert.True(movie.InputFrames[1].Controllers[0].Right);
		Assert.True(movie.InputFrames[1].Controllers[0].A);
		Assert.Equal((byte)0, movie.InputFrames[1].Controllers[0].PaddlePosition);

		// Frame 2: no buttons, paddle 255
		Assert.False(movie.InputFrames[2].Controllers[0].A);
		Assert.Equal((byte)255, movie.InputFrames[2].Controllers[0].PaddlePosition);
	}

	[Fact]
	public void Bk2Converter_A2600_RoundTripsPaddlePosition() {
		var original = new MovieData {
			SystemType = SystemType.A2600,
			ControllerCount = 2,
			GameName = "Paddle Roundtrip"
		};

		// Frame 0: P1 paddle at 128 with fire
		var frame0 = new InputFrame(0);
		frame0.Controllers[0].A = true;
		frame0.Controllers[0].PaddlePosition = 128;
		original.AddFrame(frame0);

		// Frame 1: P1 paddle at 0, P2 paddle at 255
		var frame1 = new InputFrame(1);
		frame1.Controllers[0].PaddlePosition = 0;
		frame1.Controllers[1].PaddlePosition = 255;
		frame1.Controllers[1].A = true;
		original.AddFrame(frame1);

		// Frame 2: P1 no paddle (joystick), P2 paddle at 100
		var frame2 = new InputFrame(2);
		frame2.Controllers[0].Up = true;
		frame2.Controllers[0].A = true;
		frame2.Controllers[1].PaddlePosition = 100;
		original.AddFrame(frame2);

		using var stream = new MemoryStream();
		var converter = new Converters.Bk2MovieConverter();
		converter.Write(original, stream);

		stream.Position = 0;
		var loaded = converter.Read(stream, "test.bk2");

		Assert.Equal(3, loaded.TotalFrames);

		// Frame 0
		Assert.True(loaded.InputFrames[0].Controllers[0].A);
		Assert.Equal((byte)128, loaded.InputFrames[0].Controllers[0].PaddlePosition);
		Assert.Null(loaded.InputFrames[0].Controllers[1].PaddlePosition);

		// Frame 1
		Assert.Equal((byte)0, loaded.InputFrames[1].Controllers[0].PaddlePosition);
		Assert.Equal((byte)255, loaded.InputFrames[1].Controllers[1].PaddlePosition);
		Assert.True(loaded.InputFrames[1].Controllers[1].A);

		// Frame 2
		Assert.True(loaded.InputFrames[2].Controllers[0].Up);
		Assert.True(loaded.InputFrames[2].Controllers[0].A);
		Assert.Null(loaded.InputFrames[2].Controllers[0].PaddlePosition);
		Assert.Equal((byte)100, loaded.InputFrames[2].Controllers[1].PaddlePosition);
	}

	[Fact]
	public void Bk2Converter_A2600_PaddleWithConsoleSwitches() {
		var original = new MovieData {
			SystemType = SystemType.A2600,
			ControllerCount = 1,
			GameName = "Paddle+Switches"
		};

		// Frame 0: paddle at 64, Select pressed
		var frame0 = new InputFrame(0);
		frame0.Controllers[0].A = true;
		frame0.Controllers[0].PaddlePosition = 64;
		frame0.Command = FrameCommand.Atari2600Select;
		original.AddFrame(frame0);

		// Frame 1: paddle at 200, Reset pressed, fire + left
		var frame1 = new InputFrame(1);
		frame1.Controllers[0].A = true;
		frame1.Controllers[0].Left = true;
		frame1.Controllers[0].PaddlePosition = 200;
		frame1.Command = FrameCommand.Atari2600Reset;
		original.AddFrame(frame1);

		// Frame 2: paddle at 0, both switches
		var frame2 = new InputFrame(2);
		frame2.Controllers[0].PaddlePosition = 0;
		frame2.Command = FrameCommand.Atari2600Select | FrameCommand.Atari2600Reset;
		original.AddFrame(frame2);

		using var stream = new MemoryStream();
		var converter = new Converters.Bk2MovieConverter();
		converter.Write(original, stream);

		stream.Position = 0;
		var loaded = converter.Read(stream, "test.bk2");

		Assert.Equal(3, loaded.TotalFrames);

		// Frame 0: paddle + Select
		Assert.True(loaded.InputFrames[0].Controllers[0].A);
		Assert.Equal((byte)64, loaded.InputFrames[0].Controllers[0].PaddlePosition);
		Assert.True(loaded.InputFrames[0].Command.HasFlag(FrameCommand.Atari2600Select));
		Assert.False(loaded.InputFrames[0].Command.HasFlag(FrameCommand.Atari2600Reset));

		// Frame 1: paddle + Reset + fire + left
		Assert.True(loaded.InputFrames[1].Controllers[0].A);
		Assert.True(loaded.InputFrames[1].Controllers[0].Left);
		Assert.Equal((byte)200, loaded.InputFrames[1].Controllers[0].PaddlePosition);
		Assert.True(loaded.InputFrames[1].Command.HasFlag(FrameCommand.Atari2600Reset));

		// Frame 2: paddle + both switches
		Assert.Equal((byte)0, loaded.InputFrames[2].Controllers[0].PaddlePosition);
		Assert.True(loaded.InputFrames[2].Command.HasFlag(FrameCommand.Atari2600Select));
		Assert.True(loaded.InputFrames[2].Command.HasFlag(FrameCommand.Atari2600Reset));
	}

	[Fact]
	public void Bk2Converter_A2600_NoPaddlePreservesJoystickFormat() {
		// Verify backward compatibility — joystick-only A2600 movies keep 5-char format
		var movie = new MovieData {
			SystemType = SystemType.A2600,
			ControllerCount = 1
		};

		var frame = new InputFrame(0);
		frame.Controllers[0].Up = true;
		frame.Controllers[0].A = true;
		movie.AddFrame(frame);

		using var stream = new MemoryStream();
		var converter = new Converters.Bk2MovieConverter();
		converter.Write(movie, stream);

		// Read raw Input Log.txt content
		stream.Position = 0;
		using var archive = new ZipArchive(stream, ZipArchiveMode.Read);
		var inputEntry = archive.GetEntry("Input Log.txt")!;
		using var reader = new StreamReader(inputEntry.Open());
		string content = reader.ReadToEnd();

		// Should contain standard 5-char joystick format without comma
		Assert.Contains("U...B", content);
		Assert.DoesNotContain(",", content);
	}

	[Fact]
	public void Bk2Converter_A2600_PaddleFormatInRawOutput() {
		var movie = new MovieData {
			SystemType = SystemType.A2600,
			ControllerCount = 1
		};

		var frame = new InputFrame(0);
		frame.Controllers[0].A = true;
		frame.Controllers[0].PaddlePosition = 128;
		movie.AddFrame(frame);

		using var stream = new MemoryStream();
		var converter = new Converters.Bk2MovieConverter();
		converter.Write(movie, stream);

		// Read raw Input Log.txt content
		stream.Position = 0;
		using var archive = new ZipArchive(stream, ZipArchiveMode.Read);
		var inputEntry = archive.GetEntry("Input Log.txt")!;
		using var reader = new StreamReader(inputEntry.Open());
		string content = reader.ReadToEnd();

		// Should contain paddle format: ....B,  128
		Assert.Contains("....B,  128", content);
	}

	#endregion

	#region BK2 Atari 2600 Controller Subtypes

	[Fact]
	public void Bk2Converter_A2600_RoundTripsKeypadSubtype() {
		var movie = new MovieData {
			SystemType = SystemType.A2600,
			ControllerCount = 1
		};
		movie.PortTypes[0] = ControllerType.Atari2600Keypad;

		var frame = new InputFrame(0);
		frame.Controllers[0].Y = true;      // "1"
		frame.Controllers[0].A = true;      // "5"
		frame.Controllers[0].Right = true;  // "#"
		movie.AddFrame(frame);

		using var stream = new MemoryStream();
		var converter = new Converters.Bk2MovieConverter();
		converter.Write(movie, stream);

		stream.Position = 0;
		var loaded = converter.Read(stream, "test.bk2");

		Assert.Equal(ControllerType.Atari2600Keypad, loaded.PortTypes[0]);
		Assert.True(loaded.InputFrames[0].Controllers[0].Y);
		Assert.True(loaded.InputFrames[0].Controllers[0].A);
		Assert.True(loaded.InputFrames[0].Controllers[0].Right);
	}

	[Fact]
	public void Bk2Converter_A2600_RoundTripsDrivingSubtype() {
		var movie = new MovieData {
			SystemType = SystemType.A2600,
			ControllerCount = 1
		};
		movie.PortTypes[0] = ControllerType.Atari2600DrivingController;

		var frame = new InputFrame(0);
		frame.Controllers[0].Left = true;
		frame.Controllers[0].A = true;
		frame.Controllers[0].PaddlePosition = 144;
		movie.AddFrame(frame);

		using var stream = new MemoryStream();
		var converter = new Converters.Bk2MovieConverter();
		converter.Write(movie, stream);

		stream.Position = 0;
		var loaded = converter.Read(stream, "test.bk2");

		Assert.Equal(ControllerType.Atari2600DrivingController, loaded.PortTypes[0]);
		Assert.True(loaded.InputFrames[0].Controllers[0].Left);
		Assert.True(loaded.InputFrames[0].Controllers[0].A);
		Assert.Equal((byte)144, loaded.InputFrames[0].Controllers[0].PaddlePosition);
	}

	[Fact]
	public void Bk2Converter_A2600_RoundTripsBoosterGripSubtype() {
		var movie = new MovieData {
			SystemType = SystemType.A2600,
			ControllerCount = 1
		};
		movie.PortTypes[0] = ControllerType.Atari2600BoosterGrip;

		var frame = new InputFrame(0);
		frame.Controllers[0].Up = true;
		frame.Controllers[0].A = true; // Fire
		frame.Controllers[0].B = true; // Trigger
		frame.Controllers[0].C = true; // Booster
		movie.AddFrame(frame);

		using var stream = new MemoryStream();
		var converter = new Converters.Bk2MovieConverter();
		converter.Write(movie, stream);

		stream.Position = 0;
		var loaded = converter.Read(stream, "test.bk2");

		Assert.Equal(ControllerType.Atari2600BoosterGrip, loaded.PortTypes[0]);
		Assert.True(loaded.InputFrames[0].Controllers[0].Up);
		Assert.True(loaded.InputFrames[0].Controllers[0].A);
		Assert.True(loaded.InputFrames[0].Controllers[0].B);
		Assert.True(loaded.InputFrames[0].Controllers[0].C);
	}

	[Fact]
	public void Bk2Converter_A2600_WritesSyncSettingsPortTypesMetadata() {
		var movie = new MovieData {
			SystemType = SystemType.A2600,
			ControllerCount = 2
		};
		movie.PortTypes[0] = ControllerType.Atari2600Keypad;
		movie.PortTypes[1] = ControllerType.Atari2600BoosterGrip;
		movie.AddFrame(new InputFrame(0));

		using var stream = new MemoryStream();
		var converter = new Converters.Bk2MovieConverter();
		converter.Write(movie, stream);

		stream.Position = 0;
		using var archive = new ZipArchive(stream, ZipArchiveMode.Read);
		var syncEntry = archive.GetEntry("SyncSettings.json");
		Assert.NotNull(syncEntry);

		using var reader = new StreamReader(syncEntry!.Open());
		string sync = reader.ReadToEnd();
		Assert.Contains("A2600PortTypes", sync);
		Assert.Contains("Atari2600Keypad", sync);
		Assert.Contains("Atari2600BoosterGrip", sync);
	}

	#endregion
}
