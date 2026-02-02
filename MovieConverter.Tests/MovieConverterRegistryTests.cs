namespace Nexen.MovieConverter.Tests;

/// <summary>
/// Tests for MovieConverterRegistry
/// </summary>
public class MovieConverterRegistryTests {
	[Fact]
	public void Converters_ReturnsAllRegisteredConverters() {
		var converters = MovieConverterRegistry.Converters;

		Assert.NotEmpty(converters);
		Assert.Contains(converters, c => c.Format == MovieFormat.Nexen);
		Assert.Contains(converters, c => c.Format == MovieFormat.Bk2);
		Assert.Contains(converters, c => c.Format == MovieFormat.Fm2);
		Assert.Contains(converters, c => c.Format == MovieFormat.Smv);
		Assert.Contains(converters, c => c.Format == MovieFormat.Lsmv);
		Assert.Contains(converters, c => c.Format == MovieFormat.Vbm);
		Assert.Contains(converters, c => c.Format == MovieFormat.Gmv);
	}

	[Theory]
	[InlineData(MovieFormat.Nexen)]
	[InlineData(MovieFormat.Bk2)]
	[InlineData(MovieFormat.Fm2)]
	[InlineData(MovieFormat.Smv)]
	[InlineData(MovieFormat.Lsmv)]
	[InlineData(MovieFormat.Vbm)]
	[InlineData(MovieFormat.Gmv)]
	public void GetConverter_ReturnsConverterForKnownFormat(MovieFormat format) {
		var converter = MovieConverterRegistry.GetConverter(format);

		Assert.NotNull(converter);
		Assert.Equal(format, converter.Format);
	}

	[Fact]
	public void GetConverter_ReturnsNullForUnknownFormat() {
		var converter = MovieConverterRegistry.GetConverter(MovieFormat.Unknown);

		Assert.Null(converter);
	}

	[Theory]
	[InlineData(".nexen-movie", MovieFormat.Nexen)]
	[InlineData(".bk2", MovieFormat.Bk2)]
	[InlineData(".fm2", MovieFormat.Fm2)]
	[InlineData(".smv", MovieFormat.Smv)]
	[InlineData(".lsmv", MovieFormat.Lsmv)]
	[InlineData(".vbm", MovieFormat.Vbm)]
	[InlineData(".gmv", MovieFormat.Gmv)]
	public void GetConverterByExtension_ReturnsCorrectConverter(string extension, MovieFormat expectedFormat) {
		var converter = MovieConverterRegistry.GetConverterByExtension(extension);

		Assert.NotNull(converter);
		Assert.Equal(expectedFormat, converter.Format);
	}

	[Fact]
	public void GetConverterByExtension_HandlesExtensionWithoutDot() {
		var converter = MovieConverterRegistry.GetConverterByExtension("bk2");

		Assert.NotNull(converter);
		Assert.Equal(MovieFormat.Bk2, converter.Format);
	}

	[Fact]
	public void GetConverterByExtension_IsCaseInsensitive() {
		var converter = MovieConverterRegistry.GetConverterByExtension(".BK2");

		Assert.NotNull(converter);
		Assert.Equal(MovieFormat.Bk2, converter.Format);
	}

	[Fact]
	public void GetConverterByExtension_ReturnsNullForUnknownExtension() {
		var converter = MovieConverterRegistry.GetConverterByExtension(".xyz");

		Assert.Null(converter);
	}

	[Theory]
	[InlineData("movie.bk2", MovieFormat.Bk2)]
	[InlineData("test.fm2", MovieFormat.Fm2)]
	[InlineData("/path/to/movie.smv", MovieFormat.Smv)]
	[InlineData("C:\\Movies\\test.lsmv", MovieFormat.Lsmv)]
	public void DetectFormat_DetectsFromFilePath(string filePath, MovieFormat expectedFormat) {
		var format = MovieConverterRegistry.DetectFormat(filePath);

		Assert.Equal(expectedFormat, format);
	}

	[Fact]
	public void DetectFormat_ReturnsUnknownForUnknownExtension() {
		var format = MovieConverterRegistry.DetectFormat("movie.xyz");

		Assert.Equal(MovieFormat.Unknown, format);
	}

	[Fact]
	public void GetReadableFormats_ReturnsOnlyReadableConverters() {
		var readable = MovieConverterRegistry.GetReadableFormats();

		Assert.All(readable, c => Assert.True(c.CanRead));
	}

	[Fact]
	public void GetWritableFormats_ReturnsOnlyWritableConverters() {
		var writable = MovieConverterRegistry.GetWritableFormats();

		Assert.All(writable, c => Assert.True(c.CanWrite));
	}

	[Fact]
	public void GetOpenFileFilter_ContainsAllReadableFormats() {
		string filter = MovieConverterRegistry.GetOpenFileFilter();

		Assert.Contains(".bk2", filter);
		Assert.Contains(".fm2", filter);
		Assert.Contains(".smv", filter);
		Assert.Contains(".lsmv", filter);
		Assert.Contains(".nexen-movie", filter);
	}

	[Fact]
	public void GetSaveFileFilter_ContainsWritableFormats() {
		string filter = MovieConverterRegistry.GetSaveFileFilter();

		Assert.Contains(".nexen-movie", filter);
		Assert.Contains(".bk2", filter);
	}

	[Fact]
	public void Register_AddsConverterToList() {
		// Get the initial count
		int initialCount = MovieConverterRegistry.Converters.Count;

		// Register a custom converter for a new format (use Unknown)
		var customConverter = new TestUnknownConverter();
		MovieConverterRegistry.Register(customConverter);

		// Converters list should include our new converter
		var converters = MovieConverterRegistry.Converters;
		Assert.Contains(converters, c => c.Format == MovieFormat.Unknown);
	}

	/// <summary>
	/// Test converter for registration tests
	/// </summary>
	private sealed class TestNexenConverter : IMovieConverter {
		public string[] Extensions => [".nexen-movie"];
		public string FormatName => "Test Nexen";
		public MovieFormat Format => MovieFormat.Nexen;
		public bool CanRead => true;
		public bool CanWrite => true;

		public MovieData Read(string filePath) => new();
		public MovieData Read(Stream stream, string? fileName = null) => new();
		public ValueTask<MovieData> ReadAsync(string filePath, CancellationToken cancellationToken = default) => ValueTask.FromResult(new MovieData());
		public ValueTask<MovieData> ReadAsync(Stream stream, string? fileName = null, CancellationToken cancellationToken = default) => ValueTask.FromResult(new MovieData());
		public void Write(MovieData movie, string filePath) { }
		public void Write(MovieData movie, Stream stream) { }
		public ValueTask WriteAsync(MovieData movie, string filePath, CancellationToken cancellationToken = default) => ValueTask.CompletedTask;
		public ValueTask WriteAsync(MovieData movie, Stream stream, CancellationToken cancellationToken = default) => ValueTask.CompletedTask;
	}

	/// <summary>
	/// Test converter for Unknown format registration tests
	/// </summary>
	private sealed class TestUnknownConverter : IMovieConverter {
		public string[] Extensions => [".test-unknown"];
		public string FormatName => "Test Unknown";
		public MovieFormat Format => MovieFormat.Unknown;
		public bool CanRead => true;
		public bool CanWrite => true;

		public MovieData Read(string filePath) => new();
		public MovieData Read(Stream stream, string? fileName = null) => new();
		public ValueTask<MovieData> ReadAsync(string filePath, CancellationToken cancellationToken = default) => ValueTask.FromResult(new MovieData());
		public ValueTask<MovieData> ReadAsync(Stream stream, string? fileName = null, CancellationToken cancellationToken = default) => ValueTask.FromResult(new MovieData());
		public void Write(MovieData movie, string filePath) { }
		public void Write(MovieData movie, Stream stream) { }
		public ValueTask WriteAsync(MovieData movie, string filePath, CancellationToken cancellationToken = default) => ValueTask.CompletedTask;
		public ValueTask WriteAsync(MovieData movie, Stream stream, CancellationToken cancellationToken = default) => ValueTask.CompletedTask;
	}
}
