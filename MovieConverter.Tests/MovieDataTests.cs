namespace Nexen.MovieConverter.Tests;

/// <summary>
/// Tests for the MovieData class
/// </summary>
public class MovieDataTests {
	[Fact]
	public void Constructor_SetsDefaultValues() {
		var movie = new MovieData();

		Assert.Equal("", movie.Author);
		Assert.Equal("", movie.Description);
		Assert.Equal("", movie.GameName);
		Assert.Equal(SystemType.Snes, movie.SystemType);
		Assert.Equal(RegionType.NTSC, movie.Region);
		Assert.Equal(2, movie.ControllerCount);
		Assert.Equal(StartType.PowerOn, movie.StartType);
		Assert.Empty(movie.InputFrames);
	}

	[Fact]
	public void AddFrame_IncrementsFrameNumber() {
		var movie = new MovieData();

		movie.AddFrame(new InputFrame());
		movie.AddFrame(new InputFrame());
		movie.AddFrame(new InputFrame());

		Assert.Equal(3, movie.TotalFrames);
		Assert.Equal(0, movie.InputFrames[0].FrameNumber);
		Assert.Equal(1, movie.InputFrames[1].FrameNumber);
		Assert.Equal(2, movie.InputFrames[2].FrameNumber);
	}

	[Fact]
	public void AddMarker_CreatesMarkersListIfNull() {
		var movie = new MovieData();
		Assert.Null(movie.Markers);

		movie.AddMarker(100, "Test Marker", "Description");

		Assert.NotNull(movie.Markers);
		Assert.Single(movie.Markers);
		Assert.Equal(100, movie.Markers[0].FrameNumber);
		Assert.Equal("Test Marker", movie.Markers[0].Label);
	}

	[Fact]
	public void GetFrame_ReturnsNullForOutOfRange() {
		var movie = new MovieData();
		movie.AddFrame(new InputFrame());

		Assert.NotNull(movie.GetFrame(0));
		Assert.Null(movie.GetFrame(1));
		Assert.Null(movie.GetFrame(-1));
	}

	[Fact]
	public void TruncateAt_RemovesFramesAfterIndex() {
		var movie = new MovieData();
		for (int i = 0; i < 10; i++) {
			movie.AddFrame(new InputFrame());
		}

		movie.TruncateAt(5);

		Assert.Equal(6, movie.TotalFrames); // 0-5 inclusive
	}

	[Fact]
	public void Clone_CreatesDeepCopy() {
		var movie = new MovieData {
			Author = "Test Author",
			GameName = "Test Game",
			RerecordCount = 1000
		};
		movie.AddFrame(new InputFrame { Controllers = [new ControllerInput { A = true }] });

		var clone = movie.Clone();

		Assert.Equal(movie.Author, clone.Author);
		Assert.Equal(movie.GameName, clone.GameName);
		Assert.Equal(movie.RerecordCount, clone.RerecordCount);
		Assert.Equal(movie.TotalFrames, clone.TotalFrames);

		// Verify it's a deep copy
		clone.Author = "Modified";
		Assert.NotEqual(movie.Author, clone.Author);
	}

	[Theory]
	[InlineData(RegionType.NTSC, SystemType.Nes, 60.098814)]
	[InlineData(RegionType.PAL, SystemType.Nes, 50.006978)]
	[InlineData(RegionType.NTSC, SystemType.Snes, 60.098814)]
	[InlineData(RegionType.PAL, SystemType.Snes, 50.006979)]
	[InlineData(RegionType.NTSC, SystemType.Gb, 59.7275)]
	public void FrameRate_ReturnsCorrectRate(RegionType region, SystemType system, double expectedRate) {
		var movie = new MovieData {
			Region = region,
			SystemType = system
		};

		Assert.Equal(expectedRate, movie.FrameRate);
	}

	[Fact]
	public void StartsFromSavestate_DetectsCorrectly() {
		var movie = new MovieData();
		Assert.False(movie.StartsFromSavestate);

		movie.InitialState = [0x00, 0x01, 0x02];
		Assert.True(movie.StartsFromSavestate);
	}

	[Fact]
	public void StartsFromSram_DetectsCorrectly() {
		var movie = new MovieData();
		Assert.False(movie.StartsFromSram);

		movie.InitialSram = [0x00, 0x01, 0x02];
		Assert.True(movie.StartsFromSram);
	}
}
