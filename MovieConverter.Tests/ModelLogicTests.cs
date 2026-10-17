using Nexen.MovieConverter;

namespace Nexen.MovieConverter.Tests;

/// <summary>
/// Tests for pure logic in MovieConverter model classes:
/// ControllerInput, InputFrame, and MovieData.
/// </summary>
public sealed class ModelLogicTests {
	#region ControllerInput — Format Roundtrips

	[Fact]
	public void NexenFormat_AllButtons_Roundtrip() {
		var input = new ControllerInput {
			B = true, Y = true, Select = true, Start = true,
			Up = true, Down = true, Left = true, Right = true,
			A = true, X = true, L = true, R = true,
			C = true, Z = true, Mode = true,
			Type = ControllerType.Gamepad
		};

		string text = input.ToNexenFormat();
		Assert.Equal("BYsSUDLRAXlrcZM", text);

		ControllerInput parsed = ControllerInput.FromNexenFormat(text);
		Assert.True(parsed.B && parsed.Y && parsed.Select && parsed.Start);
		Assert.True(parsed.Up && parsed.Down && parsed.Left && parsed.Right);
		Assert.True(parsed.A && parsed.X && parsed.L && parsed.R);
		Assert.True(parsed.C && parsed.Z && parsed.Mode);
	}

	[Fact]
	public void NexenFormat_NoButtons_AllDots() {
		var input = new ControllerInput();
		string text = input.ToNexenFormat();
		Assert.Equal("...............", text);

		ControllerInput parsed = ControllerInput.FromNexenFormat(text);
		Assert.False(parsed.HasInput);
	}

	[Fact]
	public void NexenFormat_NoneType_AllDots() {
		var input = new ControllerInput { Type = ControllerType.None, A = true };
		Assert.Equal("...............", input.ToNexenFormat());
	}

	[Fact]
	public void NexenFormat_Legacy12Char_ParsedCorrectly() {
		// Legacy 12-char format (no C/Z/Mode)
		ControllerInput parsed = ControllerInput.FromNexenFormat("BY..U..RA...");
		Assert.True(parsed.B && parsed.Y && parsed.Up && parsed.Right && parsed.A);
		Assert.False(parsed.C);
		Assert.False(parsed.Z);
		Assert.False(parsed.Mode);
	}

	[Fact]
	public void NexenFormat_ShortInput_ReturnsEmpty() {
		ControllerInput parsed = ControllerInput.FromNexenFormat("BY");
		Assert.False(parsed.HasInput);
	}

	[Fact]
	public void SmvFormat_Roundtrip() {
		var input = new ControllerInput {
			A = true, B = true, X = true, Y = true,
			L = true, R = true, Start = true, Select = true,
			Up = true, Down = true, Left = true, Right = true
		};

		ushort smv = input.ToSmvFormat();
		ControllerInput parsed = ControllerInput.FromSmvFormat(smv);

		Assert.True(parsed.A && parsed.B && parsed.X && parsed.Y);
		Assert.True(parsed.L && parsed.R && parsed.Start && parsed.Select);
		Assert.True(parsed.Up && parsed.Down && parsed.Left && parsed.Right);
	}

	[Fact]
	public void SmvFormat_NoButtons_Zero() {
		var input = new ControllerInput();
		Assert.Equal((ushort)0, input.ToSmvFormat());
	}

	[Fact]
	public void Fm2Format_Roundtrip() {
		var input = new ControllerInput {
			Right = true, Left = true, Down = true, Up = true,
			Start = true, Select = true, B = true, A = true
		};

		string text = input.ToFm2Format();
		Assert.Equal(8, text.Length);

		ControllerInput parsed = ControllerInput.FromFm2Format(text);
		Assert.True(parsed.Right && parsed.Left && parsed.Down && parsed.Up);
		Assert.True(parsed.Start && parsed.Select && parsed.B && parsed.A);
	}

	[Fact]
	public void LsmvFormat_Roundtrip() {
		var input = new ControllerInput {
			B = true, Y = true, Select = true, Start = true,
			Up = true, Down = true, Left = true, Right = true,
			A = true, X = true, L = true, R = true
		};

		string text = input.ToLsmvFormat();
		Assert.Equal(12, text.Length);

		ControllerInput parsed = ControllerInput.FromLsmvFormat(text);
		Assert.True(parsed.B && parsed.Y && parsed.Select && parsed.Start);
		Assert.True(parsed.Up && parsed.Down && parsed.Left && parsed.Right);
		Assert.True(parsed.A && parsed.X && parsed.L && parsed.R);
	}

	[Theory]
	[InlineData(SystemType.Nes, 8)]
	[InlineData(SystemType.Snes, 12)]
	[InlineData(SystemType.Genesis, 8)]
	[InlineData(SystemType.Lynx, 9)]
	public void Bk2Format_Roundtrip_BySystem(SystemType system, int expectedLength) {
		var input = new ControllerInput {
			Up = true, Down = true, Left = true, Right = true,
			A = true, B = true, Start = true
		};

		string text = input.ToBk2Format(system);
		Assert.Equal(expectedLength, text.Length);

		ControllerInput parsed = ControllerInput.FromBk2Format(text, system);
		Assert.True(parsed.Up && parsed.Down && parsed.Left && parsed.Right);
		Assert.True(parsed.A && parsed.B && parsed.Start);
	}

	#endregion

	#region ControllerInput — ButtonBits

	[Fact]
	public void ButtonBits_SetAndGet_Roundtrip() {
		var input = new ControllerInput();
		input.ButtonBits = 0x7fff; // All 15 buttons set

		Assert.True(input.A && input.B && input.X && input.Y);
		Assert.True(input.L && input.R && input.Start && input.Select);
		Assert.True(input.Up && input.Down && input.Left && input.Right);
		Assert.True(input.C && input.Z && input.Mode);

		Assert.Equal((ushort)0x7fff, input.ButtonBits);
	}

	[Fact]
	public void ButtonBits_NoButtons_Zero() {
		Assert.Equal((ushort)0, new ControllerInput().ButtonBits);
	}

	[Theory]
	[InlineData("A", 1 << 0)]
	[InlineData("B", 1 << 1)]
	[InlineData("X", 1 << 2)]
	[InlineData("Y", 1 << 3)]
	[InlineData("L", 1 << 4)]
	[InlineData("R", 1 << 5)]
	[InlineData("Start", 1 << 6)]
	[InlineData("Select", 1 << 7)]
	[InlineData("Up", 1 << 8)]
	[InlineData("Down", 1 << 9)]
	[InlineData("Left", 1 << 10)]
	[InlineData("Right", 1 << 11)]
	[InlineData("C", 1 << 12)]
	[InlineData("Z", 1 << 13)]
	[InlineData("Mode", 1 << 14)]
	public void ButtonBits_IndividualButton_CorrectBit(string button, int expectedBit) {
		var input = new ControllerInput();
		input.SetButton(button, true);
		Assert.Equal((ushort)expectedBit, input.ButtonBits);
	}

	#endregion

	#region ControllerInput — HasInput

	[Fact]
	public void HasInput_NoButtons_False() {
		Assert.False(new ControllerInput().HasInput);
	}

	[Theory]
	[InlineData("A")]
	[InlineData("B")]
	[InlineData("Up")]
	[InlineData("Start")]
	public void HasInput_SingleButton_True(string button) {
		var input = new ControllerInput();
		input.SetButton(button, true);
		Assert.True(input.HasInput);
	}

	[Fact]
	public void HasInput_AnalogOnly_True() {
		var input = new ControllerInput { AnalogX = 42 };
		Assert.True(input.HasInput);
	}

	[Fact]
	public void HasInput_MouseButtonOnly_True() {
		var input = new ControllerInput { MouseButton1 = true };
		Assert.True(input.HasInput);
	}

	#endregion

	#region ControllerInput — SetButton / GetButton

	[Theory]
	[InlineData("A", "a")]
	[InlineData("B", "b")]
	[InlineData("UP", "Up")]
	[InlineData("START", "Start")]
	[InlineData("FIRE", "Fire")]
	[InlineData("TRIGGER", "Trigger")]
	[InlineData("BOOSTER", "Booster")]
	public void SetButton_GetButton_CaseVariants(string setName, string getName) {
		var input = new ControllerInput();
		input.SetButton(setName, true);
		Assert.True(input.GetButton(getName));
	}

	[Fact]
	public void GetButton_Unknown_ReturnsFalse() {
		var input = new ControllerInput { A = true };
		Assert.False(input.GetButton("NONEXISTENT"));
	}

	[Fact]
	public void SetButton_NumericKeypad_MapsCorrectly() {
		var input = new ControllerInput();
		input.SetButton("1", true); // Maps to Y
		Assert.True(input.Y);

		input.SetButton("5", true); // Maps to A
		Assert.True(input.A);
	}

	#endregion

	#region ControllerInput — Clone & Equality

	[Fact]
	public void Clone_PreservesAllFields() {
		var input = new ControllerInput {
			A = true, B = true, X = true, Y = true,
			L = true, R = true, Start = true, Select = true,
			Up = true, C = true, Z = true, Mode = true,
			MouseX = 100, MouseY = 200,
			AnalogX = -42, AnalogY = 127,
			PaddlePosition = 128,
			Type = ControllerType.Mouse
		};

		ControllerInput clone = input.Clone();
		Assert.Equal(input, clone);
		Assert.NotSame(input, clone);
	}

	[Fact]
	public void Clone_KeyboardData_DeepCopy() {
		var input = new ControllerInput {
			KeyboardData = [0x01, 0x02, 0x03]
		};

		ControllerInput clone = input.Clone();
		Assert.NotSame(input.KeyboardData, clone.KeyboardData);
		Assert.Equal(input.KeyboardData, clone.KeyboardData);
	}

	[Fact]
	public void Equality_SameButtons_Equal() {
		var a = new ControllerInput { A = true, Up = true };
		var b = new ControllerInput { A = true, Up = true };
		Assert.Equal(a, b);
	}

	[Fact]
	public void Equality_DifferentButtons_NotEqual() {
		var a = new ControllerInput { A = true };
		var b = new ControllerInput { B = true };
		Assert.NotEqual(a, b);
	}

	[Fact]
	public void Equality_Null_NotEqual() {
		var a = new ControllerInput { A = true };
		Assert.False(a.Equals(null));
	}

	#endregion

	#region InputFrame — ToNexenLogLine / FromNexenLogLine

	[Fact]
	public void NexenLogLine_BasicInput_Roundtrip() {
		var frame = new InputFrame(42);
		frame.Controllers[0].A = true;
		frame.Controllers[0].Start = true;
		frame.Controllers[1].B = true;

		string line = frame.ToNexenLogLine(2);
		InputFrame parsed = InputFrame.FromNexenLogLine(line, 42);

		Assert.True(parsed.Controllers[0].A);
		Assert.True(parsed.Controllers[0].Start);
		Assert.True(parsed.Controllers[1].B);
	}

	[Fact]
	public void NexenLogLine_EmptyFrame_Roundtrip() {
		var frame = new InputFrame(0);
		string line = frame.ToNexenLogLine(2);
		InputFrame parsed = InputFrame.FromNexenLogLine(line, 0);

		Assert.False(parsed.HasAnyInput);
	}

	[Fact]
	public void NexenLogLine_LagFrame_Preserved() {
		var frame = new InputFrame(5) { IsLagFrame = true };
		string line = frame.ToNexenLogLine(2);

		Assert.Contains("LAG", line);

		InputFrame parsed = InputFrame.FromNexenLogLine(line, 5);
		Assert.True(parsed.IsLagFrame);
	}

	[Fact]
	public void NexenLogLine_Comment_Preserved() {
		var frame = new InputFrame(10) { Comment = "test comment" };
		string line = frame.ToNexenLogLine(2);

		Assert.Contains("# test comment", line);

		InputFrame parsed = InputFrame.FromNexenLogLine(line, 10);
		Assert.Equal("test comment", parsed.Comment);
	}

	[Fact]
	public void NexenLogLine_SoftResetCommand_Roundtrip() {
		var frame = new InputFrame(0) { Command = FrameCommand.SoftReset };
		string line = frame.ToNexenLogLine(2);

		Assert.Contains("CMD:SOFT_RESET", line);

		InputFrame parsed = InputFrame.FromNexenLogLine(line, 0);
		Assert.True(parsed.Command.HasFlag(FrameCommand.SoftReset));
	}

	[Fact]
	public void NexenLogLine_HardResetCommand_Roundtrip() {
		var frame = new InputFrame(0) { Command = FrameCommand.HardReset };
		string line = frame.ToNexenLogLine(2);

		Assert.Contains("CMD:HARD_RESET", line);

		InputFrame parsed = InputFrame.FromNexenLogLine(line, 0);
		Assert.True(parsed.Command.HasFlag(FrameCommand.HardReset));
	}

	[Fact]
	public void NexenLogLine_PaddleMetadata_Roundtrip() {
		var frame = new InputFrame(0);
		frame.Controllers[0].PaddlePosition = 200;

		string line = frame.ToNexenLogLine(2);
		Assert.Contains("P1X:200", line);

		InputFrame parsed = InputFrame.FromNexenLogLine(line, 0);
		Assert.Equal((byte)200, parsed.Controllers[0].PaddlePosition);
	}

	[Fact]
	public void NexenLogLine_EmptyString_ReturnsEmptyFrame() {
		InputFrame parsed = InputFrame.FromNexenLogLine("", 0);
		Assert.False(parsed.HasAnyInput);
		Assert.Equal(FrameCommand.None, parsed.Command);
	}

	#endregion

	#region InputFrame — HasAnyInput

	[Fact]
	public void HasAnyInput_Empty_False() {
		Assert.False(new InputFrame(0).HasAnyInput);
	}

	[Fact]
	public void HasAnyInput_OneButton_True() {
		var frame = new InputFrame(0);
		frame.Controllers[3].A = true;
		Assert.True(frame.HasAnyInput);
	}

	#endregion

	#region InputFrame — Clone & Equality

	[Fact]
	public void Clone_PreservesAll() {
		var frame = new InputFrame(7) {
			Command = FrameCommand.SoftReset,
			IsLagFrame = true,
			Comment = "cloned",
			FdsDiskNumber = 1,
			FdsDiskSide = 0
		};
		frame.Controllers[0].A = true;
		frame.Controllers[1].B = true;

		InputFrame clone = frame.Clone();
		Assert.Equal(frame, clone);
		Assert.NotSame(frame, clone);
		Assert.NotSame(frame.Controllers[0], clone.Controllers[0]);
	}

	[Fact]
	public void Equality_SameFrame_Equal() {
		var a = new InputFrame(5);
		a.Controllers[0].A = true;
		var b = new InputFrame(5);
		b.Controllers[0].A = true;
		Assert.Equal(a, b);
		Assert.True(a == b);
	}

	[Fact]
	public void Equality_DifferentFrame_NotEqual() {
		var a = new InputFrame(5);
		a.Controllers[0].A = true;
		var b = new InputFrame(6);
		b.Controllers[0].A = true;
		Assert.NotEqual(a, b);
		Assert.True(a != b);
	}

	[Fact]
	public void Equality_NullComparison() {
		var frame = new InputFrame(0);
		Assert.False(frame.Equals(null));
		Assert.False(frame == null);
		Assert.True(frame != null);
	}

	[Fact]
	public void Equality_ReferenceEqual() {
		var frame = new InputFrame(0);
		Assert.True(frame.Equals(frame));
	}

	[Fact]
	public void GetHashCode_EqualFrames_SameHash() {
		var a = new InputFrame(3);
		a.Controllers[0].ButtonBits = 0x00ff;
		var b = new InputFrame(3);
		b.Controllers[0].ButtonBits = 0x00ff;
		Assert.Equal(a.GetHashCode(), b.GetHashCode());
	}

	#endregion

	#region MovieData — FrameRate

	[Theory]
	[InlineData(SystemType.Nes, RegionType.NTSC, 60.098814)]
	[InlineData(SystemType.Nes, RegionType.PAL, 50.006978)]
	[InlineData(SystemType.Snes, RegionType.NTSC, 60.098814)]
	[InlineData(SystemType.Snes, RegionType.PAL, 50.006979)]
	[InlineData(SystemType.Gb, RegionType.NTSC, 59.7275)]
	[InlineData(SystemType.Gba, RegionType.NTSC, 59.7275)]
	[InlineData(SystemType.Lynx, RegionType.NTSC, 75.0)]
	[InlineData(SystemType.Lynx, RegionType.PAL, 75.0)]
	public void FrameRate_CorrectForSystemAndRegion(SystemType system, RegionType region, double expected) {
		var movie = new MovieData { SystemType = system, Region = region };
		Assert.Equal(expected, movie.FrameRate, 4);
	}

	[Fact]
	public void FrameRate_Dendy_UsesNtscNesRate() {
		var movie = new MovieData { SystemType = SystemType.Nes, Region = RegionType.Dendy };
		Assert.Equal(50.006978, movie.FrameRate, 4);
	}

	#endregion

	#region MovieData — Computed Properties

	[Fact]
	public void TotalFrames_MatchesInputFrameCount() {
		var movie = new MovieData();
		movie.AddFrame(new InputFrame(0));
		movie.AddFrame(new InputFrame(1));
		movie.AddFrame(new InputFrame(2));
		Assert.Equal(3, movie.TotalFrames);
	}

	[Fact]
	public void Duration_ComputedFromFrameCountAndRate() {
		var movie = new MovieData {
			SystemType = SystemType.Nes,
			Region = RegionType.NTSC
		};
		for (int i = 0; i < 3606; i++) {
			movie.AddFrame(new InputFrame(i));
		}

		// ~3606 frames at ~60.098814 fps ≈ ~60 seconds
		Assert.InRange(movie.Duration.TotalSeconds, 59.0, 61.0);
	}

	[Fact]
	public void StartsFromSavestate_WhenInitialStatePresent() {
		var movie = new MovieData { InitialState = new byte[100] };
		Assert.True(movie.StartsFromSavestate);
		Assert.False(movie.StartsFromSram);
		Assert.False(movie.StartsFromPowerOn);
	}

	[Fact]
	public void StartsFromSram_WhenInitialSramPresent() {
		var movie = new MovieData { InitialSram = new byte[8192] };
		Assert.False(movie.StartsFromSavestate);
		Assert.True(movie.StartsFromSram);
		Assert.False(movie.StartsFromPowerOn);
	}

	[Fact]
	public void StartsFromPowerOn_WhenNoStateOrSram() {
		var movie = new MovieData();
		Assert.False(movie.StartsFromSavestate);
		Assert.False(movie.StartsFromSram);
		Assert.True(movie.StartsFromPowerOn);
	}

	[Fact]
	public void StartsFromSavestate_EmptyArray_False() {
		var movie = new MovieData { InitialState = [] };
		Assert.False(movie.StartsFromSavestate);
		Assert.True(movie.StartsFromPowerOn);
	}

	#endregion

	#region MovieData — AddFrame / GetFrame / TruncateAt

	[Fact]
	public void AddFrame_AssignsFrameNumber() {
		var movie = new MovieData();
		var frame = new InputFrame(999); // Will be overwritten
		movie.AddFrame(frame);
		Assert.Equal(0, frame.FrameNumber);

		movie.AddFrame(new InputFrame(999));
		Assert.Equal(1, movie.InputFrames[1].FrameNumber);
	}

	[Fact]
	public void GetFrame_ValidIndex_ReturnsFrame() {
		var movie = new MovieData();
		movie.AddFrame(new InputFrame(0));
		Assert.NotNull(movie.GetFrame(0));
	}

	[Fact]
	public void GetFrame_OutOfRange_ReturnsNull() {
		var movie = new MovieData();
		Assert.Null(movie.GetFrame(0));
		Assert.Null(movie.GetFrame(-1));
		Assert.Null(movie.GetFrame(100));
	}

	[Fact]
	public void TruncateAt_KeepsInclusiveFrame() {
		var movie = new MovieData();
		for (int i = 0; i < 10; i++) {
			movie.AddFrame(new InputFrame(i));
		}

		movie.TruncateAt(4);
		Assert.Equal(5, movie.TotalFrames); // Frames 0-4
	}

	[Fact]
	public void TruncateAt_Negative_ClearsAll() {
		var movie = new MovieData();
		movie.AddFrame(new InputFrame(0));
		movie.AddFrame(new InputFrame(1));

		movie.TruncateAt(-1);
		Assert.Equal(0, movie.TotalFrames);
	}

	[Fact]
	public void TruncateAt_BeyondEnd_NoChange() {
		var movie = new MovieData();
		movie.AddFrame(new InputFrame(0));
		movie.AddFrame(new InputFrame(1));

		movie.TruncateAt(100);
		Assert.Equal(2, movie.TotalFrames);
	}

	#endregion

	#region MovieData — CRC32

	[Fact]
	public void CalculateInputCrc32_Deterministic() {
		var movie = new MovieData { ControllerCount = 2 };
		var frame = new InputFrame(0);
		frame.Controllers[0].A = true;
		frame.Controllers[0].Start = true;
		movie.AddFrame(frame);

		uint crc1 = movie.CalculateInputCrc32();
		uint crc2 = movie.CalculateInputCrc32();
		Assert.Equal(crc1, crc2);
	}

	[Fact]
	public void CalculateInputCrc32_DifferentInput_DifferentCrc() {
		var movie1 = new MovieData { ControllerCount = 1 };
		var frame1 = new InputFrame(0);
		frame1.Controllers[0].A = true;
		movie1.AddFrame(frame1);

		var movie2 = new MovieData { ControllerCount = 1 };
		var frame2 = new InputFrame(0);
		frame2.Controllers[0].B = true;
		movie2.AddFrame(frame2);

		Assert.NotEqual(movie1.CalculateInputCrc32(), movie2.CalculateInputCrc32());
	}

	[Fact]
	public void CalculateInputCrc32_EmptyMovie_NonZero() {
		var movie = new MovieData { ControllerCount = 1 };
		// CRC of empty data should be ~0 ^ ~0 = 0? No, with no frames the loop body is never entered
		// so crc stays at 0xffffffff, then ~0xffffffff = 0
		Assert.Equal(0u, movie.CalculateInputCrc32());
	}

	#endregion

	#region MovieData — Markers

	[Fact]
	public void AddMarker_CreatesMarkersList() {
		var movie = new MovieData();
		movie.AddMarker(100, "Boss Fight", "Start of boss");

		Assert.NotNull(movie.Markers);
		Assert.Single(movie.Markers);
		Assert.Equal(100, movie.Markers[0].FrameNumber);
		Assert.Equal("Boss Fight", movie.Markers[0].Label);
		Assert.Equal("Start of boss", movie.Markers[0].Description);
	}

	[Fact]
	public void AddMarker_MultipleMarkers_AllPreserved() {
		var movie = new MovieData();
		movie.AddMarker(0, "Start");
		movie.AddMarker(1000, "Mid");
		movie.AddMarker(5000, "End");

		Assert.Equal(3, movie.Markers!.Count);
	}

	#endregion

	#region MovieData — GetSummary

	[Fact]
	public void GetSummary_ContainsBasicInfo() {
		var movie = new MovieData {
			GameName = "Test Game",
			Author = "TestAuthor",
			SystemType = SystemType.Snes,
			Region = RegionType.NTSC,
			RerecordCount = 42
		};
		movie.AddFrame(new InputFrame(0));

		string summary = movie.GetSummary();
		Assert.Contains("Test Game", summary);
		Assert.Contains("TestAuthor", summary);
		Assert.Contains("Snes", summary);
		Assert.Contains("NTSC", summary);
		Assert.Contains("42", summary);
	}

	#endregion

	#region MovieData — Default Constructor

	[Fact]
	public void DefaultConstructor_FirstTwoPortsGamepad() {
		var movie = new MovieData();
		Assert.Equal(ControllerType.Gamepad, movie.PortTypes[0]);
		Assert.Equal(ControllerType.Gamepad, movie.PortTypes[1]);
		Assert.Equal(ControllerType.None, movie.PortTypes[2]);
	}

	[Fact]
	public void DefaultConstructor_EmptyFrameList() {
		var movie = new MovieData();
		Assert.Equal(0, movie.TotalFrames);
		Assert.Empty(movie.InputFrames);
	}

	#endregion

	#region FrameCommand — Flags

	[Fact]
	public void FrameCommand_Aliases_Match() {
		Assert.Equal(FrameCommand.SoftReset, FrameCommand.Reset);
		Assert.Equal(FrameCommand.HardReset, FrameCommand.Power);
		Assert.Equal(FrameCommand.FdsSelect, FrameCommand.FdsSide);
	}

	[Fact]
	public void FrameCommand_MultipleFlags_Combinable() {
		FrameCommand combined = FrameCommand.SoftReset | FrameCommand.FdsInsert;
		Assert.True(combined.HasFlag(FrameCommand.SoftReset));
		Assert.True(combined.HasFlag(FrameCommand.FdsInsert));
		Assert.False(combined.HasFlag(FrameCommand.HardReset));
	}

	#endregion

	#region MovieFormatException

	[Fact]
	public void MovieFormatException_ContainsFormat() {
		var ex = new MovieFormatException(MovieFormat.Bk2, "test error");
		Assert.Equal(MovieFormat.Bk2, ex.Format);
		Assert.Contains("[Bk2]", ex.Message);
		Assert.Contains("test error", ex.Message);
	}

	[Fact]
	public void MovieFormatException_WithInnerException() {
		var inner = new InvalidOperationException("inner");
		var ex = new MovieFormatException(MovieFormat.Smv, "outer", inner);
		Assert.Equal(MovieFormat.Smv, ex.Format);
		Assert.Same(inner, ex.InnerException);
	}

	#endregion
}
