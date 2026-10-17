using Nexen.MovieConverter.Converters;

namespace Nexen.MovieConverter.Tests;

/// <summary>
/// Integration tests for Lynx TAS support — input format serialization,
/// movie recording round-trips, and Lynx-specific controller mappings.
/// </summary>
public class LynxTasTests {
	// ========================================================================
	// BK2 Format Round-Trip Tests
	// ========================================================================

	[Fact]
	public void ToBk2Format_Lynx_EncodesAllButtons() {
		var input = new ControllerInput {
			Up = true, Down = true, Left = true, Right = true,
			A = true, B = true, L = true, R = true, Start = true
		};

		string bk2 = input.ToBk2Format(SystemType.Lynx);

		Assert.Equal("UDLRABOoP", bk2);
	}

	[Fact]
	public void ToBk2Format_Lynx_NoInput_ReturnsDotsOnly() {
		var input = new ControllerInput();

		string bk2 = input.ToBk2Format(SystemType.Lynx);

		Assert.Equal(".........", bk2);
		Assert.Equal(9, bk2.Length);
	}

	[Fact]
	public void ToBk2Format_Lynx_SingleButton_A() {
		var input = new ControllerInput { A = true };
		string bk2 = input.ToBk2Format(SystemType.Lynx);
		Assert.Equal("....A....", bk2);
	}

	[Fact]
	public void ToBk2Format_Lynx_SingleButton_B() {
		var input = new ControllerInput { B = true };
		string bk2 = input.ToBk2Format(SystemType.Lynx);
		Assert.Equal(".....B...", bk2);
	}

	[Fact]
	public void ToBk2Format_Lynx_Option1_MapsToL() {
		var input = new ControllerInput { L = true };
		string bk2 = input.ToBk2Format(SystemType.Lynx);
		// Option 1 is at position 6 with char 'O'
		Assert.Equal("......O..", bk2);
	}

	[Fact]
	public void ToBk2Format_Lynx_Option2_MapsToR() {
		var input = new ControllerInput { R = true };
		string bk2 = input.ToBk2Format(SystemType.Lynx);
		// Option 2 is at position 7 with char 'o'
		Assert.Equal(".......o.", bk2);
	}

	[Fact]
	public void ToBk2Format_Lynx_Pause_MapsToStart() {
		var input = new ControllerInput { Start = true };
		string bk2 = input.ToBk2Format(SystemType.Lynx);
		// Pause is at position 8 with char 'P'
		Assert.Equal("........P", bk2);
	}

	[Fact]
	public void ToBk2Format_Lynx_DPadOnly() {
		var input = new ControllerInput { Up = true, Right = true };
		string bk2 = input.ToBk2Format(SystemType.Lynx);
		Assert.Equal("U..R.....", bk2);
	}

	[Fact]
	public void FromBk2Format_Lynx_AllButtons() {
		var input = ControllerInput.FromBk2Format("UDLRABOoP", SystemType.Lynx);

		Assert.True(input.Up);
		Assert.True(input.Down);
		Assert.True(input.Left);
		Assert.True(input.Right);
		Assert.True(input.A);
		Assert.True(input.B);
		Assert.True(input.L);     // Option 1
		Assert.True(input.R);     // Option 2
		Assert.True(input.Start); // Pause
	}

	[Fact]
	public void FromBk2Format_Lynx_NoInput() {
		var input = ControllerInput.FromBk2Format(".........", SystemType.Lynx);

		Assert.False(input.Up);
		Assert.False(input.Down);
		Assert.False(input.Left);
		Assert.False(input.Right);
		Assert.False(input.A);
		Assert.False(input.B);
		Assert.False(input.L);
		Assert.False(input.R);
		Assert.False(input.Start);
	}

	[Fact]
	public void FromBk2Format_Lynx_PartialInput() {
		var input = ControllerInput.FromBk2Format("U...A..oP", SystemType.Lynx);

		Assert.True(input.Up);
		Assert.False(input.Down);
		Assert.False(input.Left);
		Assert.False(input.Right);
		Assert.True(input.A);
		Assert.False(input.B);
		Assert.False(input.L);
		Assert.True(input.R);
		Assert.True(input.Start);
	}

	[Fact]
	public void Bk2Format_Lynx_RoundTrip_PreservesInput() {
		var original = new ControllerInput {
			Up = true, A = true, L = true, Start = true
		};

		string bk2 = original.ToBk2Format(SystemType.Lynx);
		var restored = ControllerInput.FromBk2Format(bk2, SystemType.Lynx);

		Assert.Equal(original.Up, restored.Up);
		Assert.Equal(original.Down, restored.Down);
		Assert.Equal(original.Left, restored.Left);
		Assert.Equal(original.Right, restored.Right);
		Assert.Equal(original.A, restored.A);
		Assert.Equal(original.B, restored.B);
		Assert.Equal(original.L, restored.L);
		Assert.Equal(original.R, restored.R);
		Assert.Equal(original.Start, restored.Start);
	}

	[Fact]
	public void Bk2Format_Lynx_RoundTrip_AllDirectionCombinations() {
		// Test all 16 D-pad combinations
		bool[] bools = [false, true];

		foreach (bool up in bools) {
			foreach (bool down in bools) {
				foreach (bool left in bools) {
					foreach (bool right in bools) {
						var original = new ControllerInput {
							Up = up, Down = down, Left = left, Right = right
						};

						string bk2 = original.ToBk2Format(SystemType.Lynx);
						var restored = ControllerInput.FromBk2Format(bk2, SystemType.Lynx);

						Assert.Equal(up, restored.Up);
						Assert.Equal(down, restored.Down);
						Assert.Equal(left, restored.Left);
						Assert.Equal(right, restored.Right);
					}
				}
			}
		}
	}

	[Fact]
	public void Bk2Format_Lynx_RoundTrip_AllButtonCombinations() {
		// Test A/B/L/R/Start button combinations (2^5 = 32)
		for (int bits = 0; bits < 32; bits++) {
			var original = new ControllerInput {
				A = (bits & 1) != 0,
				B = (bits & 2) != 0,
				L = (bits & 4) != 0,
				R = (bits & 8) != 0,
				Start = (bits & 16) != 0
			};

			string bk2 = original.ToBk2Format(SystemType.Lynx);
			var restored = ControllerInput.FromBk2Format(bk2, SystemType.Lynx);

			Assert.Equal(original.A, restored.A);
			Assert.Equal(original.B, restored.B);
			Assert.Equal(original.L, restored.L);
			Assert.Equal(original.R, restored.R);
			Assert.Equal(original.Start, restored.Start);
		}
	}

	// ========================================================================
	// Lynx Frame Rate Tests
	// ========================================================================

	[Fact]
	public void FrameRate_Lynx_NTSC_Is75FPS() {
		var movie = new MovieData { SystemType = SystemType.Lynx, Region = RegionType.NTSC };
		Assert.Equal(75.0, movie.FrameRate);
	}

	[Fact]
	public void FrameRate_Lynx_PAL_StillIs75FPS() {
		// Lynx is region-free — always 75 Hz regardless of region setting
		var movie = new MovieData { SystemType = SystemType.Lynx, Region = RegionType.PAL };
		Assert.Equal(75.0, movie.FrameRate);
	}

	// ========================================================================
	// Lynx Movie Data Tests
	// ========================================================================

	[Fact]
	public void MovieData_Lynx_AddFrames_TracksCount() {
		var movie = new MovieData { SystemType = SystemType.Lynx };

		for (int i = 0; i < 100; i++) {
			movie.AddFrame(new InputFrame(i));
		}

		Assert.Equal(100, movie.TotalFrames);
	}

	[Fact]
	public void MovieData_Lynx_Clone_PreservesSystem() {
		var movie = new MovieData { SystemType = SystemType.Lynx, Region = RegionType.NTSC };
		var frame = new InputFrame(0);
		frame.Controllers[0] = new ControllerInput { A = true };
		movie.AddFrame(frame);

		var clone = movie.Clone();

		Assert.Equal(SystemType.Lynx, clone.SystemType);
		Assert.Equal(1, clone.TotalFrames);
		Assert.True(clone.InputFrames[0].GetPort(0).A);
	}

	[Fact]
	public void MovieData_Lynx_TruncateAt_RemovesFrames() {
		var movie = new MovieData { SystemType = SystemType.Lynx };

		for (int i = 0; i < 50; i++) {
			movie.AddFrame(new InputFrame(i));
		}

		movie.TruncateAt(24); // Keeps frames 0-24 inclusive = 25 frames
		Assert.Equal(25, movie.TotalFrames);
	}

	// ========================================================================
	// Nexen Movie Format Round-Trip Tests
	// ========================================================================

	[Fact]
	public void NexenFormat_Lynx_RoundTrip_PreservesInputs() {
		var movie = new MovieData {
			SystemType = SystemType.Lynx,
			Region = RegionType.NTSC,
			ControllerCount = 1
		};

		// Frame 1: Up + A
		var frame1 = new InputFrame(0);
		frame1.Controllers[0] = new ControllerInput { Up = true, A = true };
		movie.AddFrame(frame1);

		// Frame 2: Down + B + Start
		var frame2 = new InputFrame(1);
		frame2.Controllers[0] = new ControllerInput { Down = true, B = true, Start = true };
		movie.AddFrame(frame2);

		// Frame 3: Empty
		var frame3 = new InputFrame(2);
		movie.AddFrame(frame3);

		// Write and read back
		var converter = new NexenMovieConverter();
		using var stream = new MemoryStream();
		converter.Write(movie, stream);

		stream.Position = 0;
		var restored = converter.Read(stream);

		Assert.Equal(SystemType.Lynx, restored.SystemType);
		Assert.Equal(3, restored.TotalFrames);

		Assert.True(restored.InputFrames[0].GetPort(0).Up);
		Assert.True(restored.InputFrames[0].GetPort(0).A);
		Assert.False(restored.InputFrames[0].GetPort(0).B);

		Assert.True(restored.InputFrames[1].GetPort(0).Down);
		Assert.True(restored.InputFrames[1].GetPort(0).B);
		Assert.True(restored.InputFrames[1].GetPort(0).Start);

		Assert.False(restored.InputFrames[2].GetPort(0).HasInput);
	}

	[Fact]
	public void NexenFormat_Lynx_LargeMovie_RoundTrip() {
		var movie = new MovieData {
			SystemType = SystemType.Lynx,
			Region = RegionType.NTSC,
			ControllerCount = 1
		};

		// Simulate a 1-minute TAS at 75 FPS = 4500 frames
		var random = new Random(42); // Deterministic seed
		for (int i = 0; i < 4500; i++) {
			var frame = new InputFrame(i);
			int buttons = random.Next(512); // 9 bits for 9 buttons
			frame.Controllers[0] = new ControllerInput {
				Up = (buttons & 1) != 0,
				Down = (buttons & 2) != 0,
				Left = (buttons & 4) != 0,
				Right = (buttons & 8) != 0,
				A = (buttons & 16) != 0,
				B = (buttons & 32) != 0,
				L = (buttons & 64) != 0,
				R = (buttons & 128) != 0,
				Start = (buttons & 256) != 0
			};
			movie.AddFrame(frame);
		}

		var converter = new NexenMovieConverter();
		using var stream = new MemoryStream();
		converter.Write(movie, stream);

		stream.Position = 0;
		var restored = converter.Read(stream);

		Assert.Equal(4500, restored.TotalFrames);

		// Verify every frame matches
		var rng = new Random(42);
		for (int i = 0; i < 4500; i++) {
			int expected = rng.Next(512);
			var port = restored.InputFrames[i].GetPort(0);
			Assert.Equal((expected & 1) != 0, port.Up);
			Assert.Equal((expected & 2) != 0, port.Down);
			Assert.Equal((expected & 4) != 0, port.Left);
			Assert.Equal((expected & 8) != 0, port.Right);
			Assert.Equal((expected & 16) != 0, port.A);
			Assert.Equal((expected & 32) != 0, port.B);
			Assert.Equal((expected & 64) != 0, port.L);
			Assert.Equal((expected & 128) != 0, port.R);
			Assert.Equal((expected & 256) != 0, port.Start);
		}
	}

	// ========================================================================
	// Input Format String Length Tests
	// ========================================================================

	[Fact]
	public void ToBk2Format_Lynx_AlwaysReturns9Chars() {
		// Every Lynx BK2 input line must be exactly 9 characters
		var inputs = new[] {
			new ControllerInput(),
			new ControllerInput { A = true },
			new ControllerInput { Up = true, Down = true, Left = true, Right = true },
			new ControllerInput { A = true, B = true, L = true, R = true, Start = true },
			new ControllerInput { Up = true, Down = true, Left = true, Right = true,
				A = true, B = true, L = true, R = true, Start = true }
		};

		foreach (var input in inputs) {
			string bk2 = input.ToBk2Format(SystemType.Lynx);
			Assert.Equal(9, bk2.Length);
		}
	}

	// ========================================================================
	// Duration Calculation Tests
	// ========================================================================

	[Fact]
	public void Duration_Lynx_CorrectForFrameCount() {
		var movie = new MovieData {
			SystemType = SystemType.Lynx,
			Region = RegionType.NTSC
		};

		// 75 FPS → 75 frames = 1 second
		for (int i = 0; i < 75; i++) {
			movie.AddFrame(new InputFrame(i));
		}

		Assert.Equal(1.0, movie.Duration.TotalSeconds, precision: 6);
	}

	[Fact]
	public void Duration_Lynx_OneMinute() {
		var movie = new MovieData {
			SystemType = SystemType.Lynx,
			Region = RegionType.NTSC
		};

		// 75 FPS × 60 seconds = 4500 frames
		for (int i = 0; i < 4500; i++) {
			movie.AddFrame(new InputFrame(i));
		}

		Assert.Equal(60.0, movie.Duration.TotalSeconds, precision: 6);
	}
}
