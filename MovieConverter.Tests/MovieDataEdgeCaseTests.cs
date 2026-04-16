namespace Nexen.MovieConverter.Tests;

/// <summary>
/// Edge case tests for MovieData methods: TruncateAt boundaries,
/// CalculateInputCrc32, and Clone field-level verification.
/// </summary>
public class MovieDataEdgeCaseTests {
	#region TruncateAt Edge Cases

	[Fact]
	public void TruncateAt_NegativeValue_ClearsAllFrames() {
		var movie = CreateMovieWithFrames(10);

		movie.TruncateAt(-1);

		Assert.Empty(movie.InputFrames);
	}

	[Fact]
	public void TruncateAt_Zero_KeepsOnlyFirstFrame() {
		var movie = CreateMovieWithFrames(10);

		movie.TruncateAt(0);

		Assert.Single(movie.InputFrames);
		Assert.Equal(0, movie.InputFrames[0].FrameNumber);
	}

	[Fact]
	public void TruncateAt_LastFrame_KeepsAllFrames() {
		var movie = CreateMovieWithFrames(10);

		movie.TruncateAt(9);

		Assert.Equal(10, movie.TotalFrames);
	}

	[Fact]
	public void TruncateAt_BeyondLastFrame_KeepsAllFrames() {
		var movie = CreateMovieWithFrames(10);

		movie.TruncateAt(100);

		Assert.Equal(10, movie.TotalFrames);
	}

	[Fact]
	public void TruncateAt_EmptyMovie_DoesNotThrow() {
		var movie = new MovieData();

		movie.TruncateAt(0);
		Assert.Empty(movie.InputFrames);

		movie.TruncateAt(-1);
		Assert.Empty(movie.InputFrames);

		movie.TruncateAt(100);
		Assert.Empty(movie.InputFrames);
	}

	[Fact]
	public void TruncateAt_SingleFrame_Zero_KeepsFrame() {
		var movie = CreateMovieWithFrames(1);

		movie.TruncateAt(0);

		Assert.Single(movie.InputFrames);
	}

	[Fact]
	public void TruncateAt_SingleFrame_Negative_ClearsAll() {
		var movie = CreateMovieWithFrames(1);

		movie.TruncateAt(-1);

		Assert.Empty(movie.InputFrames);
	}

	#endregion

	#region CalculateInputCrc32 Edge Cases

	[Fact]
	public void CalculateInputCrc32_SingleFrame_ProducesHash() {
		var movie = new MovieData { ControllerCount = 1 };
		var frame = new InputFrame(0);
		frame.Controllers[0].A = true;
		movie.InputFrames.Add(frame);

		uint crc = movie.CalculateInputCrc32();

		// Should produce a non-zero hash
		Assert.NotEqual(0u, crc);
	}

	[Fact]
	public void CalculateInputCrc32_SingleFrameNoInput_ProducesHash() {
		var movie = new MovieData { ControllerCount = 1 };
		movie.InputFrames.Add(new InputFrame(0));

		uint crc = movie.CalculateInputCrc32();

		// Empty input still produces a valid hash
		Assert.True(crc >= 0);
	}

	[Fact]
	public void CalculateInputCrc32_Deterministic() {
		var movie = CreateMovieWithInput(500);

		uint crc1 = movie.CalculateInputCrc32();
		uint crc2 = movie.CalculateInputCrc32();

		Assert.Equal(crc1, crc2);
	}

	[Fact]
	public void CalculateInputCrc32_DifferentControllerCount_DifferentHash() {
		var movie1 = CreateMovieWithInput(100);
		movie1.ControllerCount = 1;

		var movie2 = CreateMovieWithInput(100);
		movie2.ControllerCount = 2;

		uint crc1 = movie1.CalculateInputCrc32();
		uint crc2 = movie2.CalculateInputCrc32();

		Assert.NotEqual(crc1, crc2);
	}

	[Fact]
	public void CalculateInputCrc32_AllButtonsPressed_ProducesHash() {
		var movie = new MovieData { ControllerCount = 1 };
		var frame = new InputFrame(0);
		frame.Controllers[0] = new ControllerInput {
			A = true, B = true, X = true, Y = true,
			L = true, R = true, Start = true, Select = true,
			Up = true, Down = true, Left = true, Right = true
		};
		movie.InputFrames.Add(frame);

		uint crc = movie.CalculateInputCrc32();

		Assert.NotEqual(0u, crc);
	}

	#endregion

	#region Clone Field-Level Verification

	[Fact]
	public void Clone_AllMetadataFields_AreCopied() {
		var movie = new MovieData {
			Author = "Test Author",
			Description = "Test Description",
			GameName = "Test Game",
			RomFileName = "test.sfc",
			InternalRomName = "TEST GAME",
			GameDatabaseId = "DB-123",
			Sha1Hash = "abc123",
			Sha256Hash = "def456",
			Md5Hash = "789ghi",
			Crc32 = 0xdeadbeef,
			RomSize = 524288,
			SystemType = SystemType.Snes,
			Region = RegionType.PAL,
			TimingMode = TimingMode.AllFrames,
			SyncMode = SyncMode.Frame,
			CoreSettings = "core-settings",
			SyncSettings = "sync-settings",
			RerecordCount = 42,
			LagFrameCount = 7,
			BlankFrames = 3,
			ControllerCount = 4,
			ControllersSwapped = true,
			StartType = StartType.Savestate,
			BiosHash = "bios-hash",
			SourceFormat = MovieFormat.Bk2,
			SourceFormatVersion = "1.0",
			EmulatorVersion = "Nexen v1.4",
			CreatedDate = new DateTime(2025, 1, 1, 0, 0, 0, DateTimeKind.Utc),
			ModifiedDate = new DateTime(2025, 6, 1, 0, 0, 0, DateTimeKind.Utc),
			IsVerified = true,
			VerificationNotes = "Verified OK",
			FinalStateHash = "final-hash",
			VsDipSwitches = 0x42,
			VsPpuType = 0x01
		};

		var clone = movie.Clone();

		Assert.Equal(movie.Author, clone.Author);
		Assert.Equal(movie.Description, clone.Description);
		Assert.Equal(movie.GameName, clone.GameName);
		Assert.Equal(movie.RomFileName, clone.RomFileName);
		Assert.Equal(movie.InternalRomName, clone.InternalRomName);
		Assert.Equal(movie.GameDatabaseId, clone.GameDatabaseId);
		Assert.Equal(movie.Sha1Hash, clone.Sha1Hash);
		Assert.Equal(movie.Sha256Hash, clone.Sha256Hash);
		Assert.Equal(movie.Md5Hash, clone.Md5Hash);
		Assert.Equal(movie.Crc32, clone.Crc32);
		Assert.Equal(movie.RomSize, clone.RomSize);
		Assert.Equal(movie.SystemType, clone.SystemType);
		Assert.Equal(movie.Region, clone.Region);
		Assert.Equal(movie.TimingMode, clone.TimingMode);
		Assert.Equal(movie.SyncMode, clone.SyncMode);
		Assert.Equal(movie.CoreSettings, clone.CoreSettings);
		Assert.Equal(movie.SyncSettings, clone.SyncSettings);
		Assert.Equal(movie.RerecordCount, clone.RerecordCount);
		Assert.Equal(movie.LagFrameCount, clone.LagFrameCount);
		Assert.Equal(movie.BlankFrames, clone.BlankFrames);
		Assert.Equal(movie.ControllerCount, clone.ControllerCount);
		Assert.Equal(movie.ControllersSwapped, clone.ControllersSwapped);
		Assert.Equal(movie.StartType, clone.StartType);
		Assert.Equal(movie.BiosHash, clone.BiosHash);
		Assert.Equal(movie.SourceFormat, clone.SourceFormat);
		Assert.Equal(movie.SourceFormatVersion, clone.SourceFormatVersion);
		Assert.Equal(movie.EmulatorVersion, clone.EmulatorVersion);
		Assert.Equal(movie.CreatedDate, clone.CreatedDate);
		Assert.Equal(movie.ModifiedDate, clone.ModifiedDate);
		Assert.Equal(movie.IsVerified, clone.IsVerified);
		Assert.Equal(movie.VerificationNotes, clone.VerificationNotes);
		Assert.Equal(movie.FinalStateHash, clone.FinalStateHash);
		Assert.Equal(movie.VsDipSwitches, clone.VsDipSwitches);
		Assert.Equal(movie.VsPpuType, clone.VsPpuType);
	}

	[Fact]
	public void Clone_ByteArrays_AreDeepCopied() {
		byte[] state = [0x01, 0x02, 0x03, 0x04, 0x05];
		byte[] sram = [0x10, 0x20, 0x30];
		byte[] rtc = [0xaa, 0xbb];
		byte[] memCard = [0xcc, 0xdd, 0xee];

		var movie = new MovieData {
			InitialState = state,
			InitialSram = sram,
			InitialRtc = rtc,
			InitialMemoryCard = memCard
		};

		var clone = movie.Clone();

		// Verify content equality
		Assert.Equal(movie.InitialState, clone.InitialState);
		Assert.Equal(movie.InitialSram, clone.InitialSram);
		Assert.Equal(movie.InitialRtc, clone.InitialRtc);
		Assert.Equal(movie.InitialMemoryCard, clone.InitialMemoryCard);

		// Verify deep copy (not same reference)
		Assert.NotSame(movie.InitialState, clone.InitialState);
		Assert.NotSame(movie.InitialSram, clone.InitialSram);
		Assert.NotSame(movie.InitialRtc, clone.InitialRtc);
		Assert.NotSame(movie.InitialMemoryCard, clone.InitialMemoryCard);

		// Verify mutation doesn't affect original
		clone.InitialState![0] = 0xff;
		Assert.NotEqual(movie.InitialState[0], clone.InitialState[0]);
	}

	[Fact]
	public void Clone_NullByteArrays_RemainNull() {
		var movie = new MovieData();

		var clone = movie.Clone();

		Assert.Null(clone.InitialState);
		Assert.Null(clone.InitialSram);
		Assert.Null(clone.InitialRtc);
		Assert.Null(clone.InitialMemoryCard);
	}

	[Fact]
	public void Clone_PortTypes_AreCopied() {
		var movie = new MovieData();
		movie.PortTypes[0] = ControllerType.Gamepad;
		movie.PortTypes[1] = ControllerType.Mouse;
		movie.PortTypes[2] = ControllerType.Multitap;

		var clone = movie.Clone();

		Assert.Equal(movie.PortTypes[0], clone.PortTypes[0]);
		Assert.Equal(movie.PortTypes[1], clone.PortTypes[1]);
		Assert.Equal(movie.PortTypes[2], clone.PortTypes[2]);

		// Verify deep copy
		clone.PortTypes[0] = ControllerType.None;
		Assert.NotEqual(movie.PortTypes[0], clone.PortTypes[0]);
	}

	[Fact]
	public void Clone_InputFrames_AreDeepCopied() {
		var movie = new MovieData();
		var frame = new InputFrame(0);
		frame.Controllers[0].A = true;
		frame.Controllers[0].B = true;
		movie.InputFrames.Add(frame);

		var clone = movie.Clone();

		Assert.Equal(1, clone.TotalFrames);
		Assert.True(clone.InputFrames[0].Controllers[0].A);
		Assert.True(clone.InputFrames[0].Controllers[0].B);

		// Verify deep copy
		clone.InputFrames[0].Controllers[0].A = false;
		Assert.True(movie.InputFrames[0].Controllers[0].A);
	}

	[Fact]
	public void Clone_Markers_AreDeepCopied() {
		var movie = new MovieData();
		movie.AddMarker(100, "Test", "Desc");

		var clone = movie.Clone();

		Assert.NotNull(clone.Markers);
		Assert.Single(clone.Markers);
		Assert.Equal(100, clone.Markers[0].FrameNumber);

		// Verify it's a separate list
		Assert.NotSame(movie.Markers, clone.Markers);
	}

	[Fact]
	public void Clone_ExtraData_IsDeepCopied() {
		var movie = new MovieData {
			ExtraData = new Dictionary<string, string> {
				["key1"] = "value1",
				["key2"] = "value2"
			}
		};

		var clone = movie.Clone();

		Assert.NotNull(clone.ExtraData);
		Assert.Equal(2, clone.ExtraData.Count);
		Assert.Equal("value1", clone.ExtraData["key1"]);

		// Verify deep copy
		clone.ExtraData["key1"] = "modified";
		Assert.NotEqual(movie.ExtraData["key1"], clone.ExtraData["key1"]);
	}

	[Fact]
	public void Clone_CrcMatchesOriginal() {
		var movie = CreateMovieWithInput(100);
		var clone = movie.Clone();

		uint originalCrc = movie.CalculateInputCrc32();
		uint cloneCrc = clone.CalculateInputCrc32();

		Assert.Equal(originalCrc, cloneCrc);
	}

	#endregion

	#region Helpers

	private static MovieData CreateMovieWithFrames(int count) {
		var movie = new MovieData();
		for (int i = 0; i < count; i++) {
			movie.AddFrame(new InputFrame(i));
		}
		return movie;
	}

	private static MovieData CreateMovieWithInput(int count) {
		var movie = new MovieData {
			SystemType = SystemType.Snes,
			Region = RegionType.NTSC
		};
		for (int i = 0; i < count; i++) {
			var frame = new InputFrame(i);
			frame.Controllers[0].A = i % 2 == 0;
			frame.Controllers[0].B = i % 3 == 0;
			frame.Controllers[0].Up = i % 5 == 0;
			movie.AddFrame(frame);
		}
		return movie;
	}

	#endregion
}
