using System.Collections.Frozen;

namespace Nexen.MovieConverter;

/// <summary>
/// Registry for movie format converters.
/// Provides format detection and converter lookup.
/// </summary>
public static class MovieConverterRegistry {
	private static readonly List<IMovieConverter> _converters = [];
	private static readonly Lock _lock = new();
	private static bool _initialized;

	// Frozen dictionaries for high-performance lookups (created after initialization)
	private static FrozenDictionary<MovieFormat, IMovieConverter>? _formatLookup;
	private static FrozenDictionary<string, IMovieConverter>? _extensionLookup;

	/// <summary>
	/// All registered converters
	/// </summary>
	public static IReadOnlyList<IMovieConverter> Converters {
		get {
			EnsureInitialized();
			lock (_lock) {
				return _converters.ToList();
			}
		}
	}

	/// <summary>
	/// Register a converter
	/// </summary>
	/// <param name="converter">Converter to register</param>
	public static void Register(IMovieConverter converter) {
		lock (_lock) {
			// Remove existing converter for same format
			_converters.RemoveAll(c => c.Format == converter.Format);
			_converters.Add(converter);
		}
	}

	/// <summary>
	/// Get converter by format enum
	/// </summary>
	/// <param name="format">Target format</param>
	/// <returns>Converter for format, or null if not found</returns>
	public static IMovieConverter? GetConverter(MovieFormat format) {
		EnsureInitialized();
		return _formatLookup?.GetValueOrDefault(format);
	}

	/// <summary>
	/// Get converter by file extension
	/// </summary>
	/// <param name="extension">File extension (with or without dot)</param>
	/// <returns>Converter for extension, or null if not found</returns>
	public static IMovieConverter? GetConverterByExtension(string extension) {
		EnsureInitialized();

		// Normalize extension
		if (!extension.StartsWith('.')) {
			extension = "." + extension;
		}

		return _extensionLookup?.GetValueOrDefault(extension.ToLowerInvariant());
	}

	/// <summary>
	/// Detect format from file path
	/// </summary>
	/// <param name="filePath">Path to movie file</param>
	/// <returns>Detected format, or Unknown if not recognized</returns>
	public static MovieFormat DetectFormat(string filePath) {
		string ext = Path.GetExtension(filePath);
		IMovieConverter? converter = GetConverterByExtension(ext);
		return converter?.Format ?? MovieFormat.Unknown;
	}

	/// <summary>
	/// Get all readable formats
	/// </summary>
	public static IEnumerable<IMovieConverter> GetReadableFormats() {
		EnsureInitialized();
		lock (_lock) {
			return _converters.Where(c => c.CanRead).ToList();
		}
	}

	/// <summary>
	/// Get all writable formats
	/// </summary>
	public static IEnumerable<IMovieConverter> GetWritableFormats() {
		EnsureInitialized();
		lock (_lock) {
			return _converters.Where(c => c.CanWrite).ToList();
		}
	}

	/// <summary>
	/// Get file filter string for open file dialogs
	/// </summary>
	/// <returns>Filter string like "BizHawk Movie (*.bk2)|*.bk2|..."</returns>
	public static string GetOpenFileFilter() {
		EnsureInitialized();

		var filters = new List<string>();
		var allExtensions = new List<string>();

		lock (_lock) {
			foreach (IMovieConverter? converter in _converters.Where(c => c.CanRead)) {
				string exts = string.Join(";", converter.Extensions.Select(e => "*" + e));
				filters.Add($"{converter.FormatName} ({exts})|{exts}");
				allExtensions.AddRange(converter.Extensions.Select(e => "*" + e));
			}
		}

		// Add "All Movie Files" option at the beginning
		string allExts = string.Join(";", allExtensions);
		filters.Insert(0, $"All Movie Files ({allExts})|{allExts}");

		return string.Join("|", filters);
	}

	/// <summary>
	/// Get file filter string for save file dialogs
	/// </summary>
	public static string GetSaveFileFilter() {
		EnsureInitialized();

		var filters = new List<string>();

		lock (_lock) {
			foreach (IMovieConverter? converter in _converters.Where(c => c.CanWrite)) {
				string exts = string.Join(";", converter.Extensions.Select(e => "*" + e));
				filters.Add($"{converter.FormatName} ({exts})|{exts}");
			}
		}

		return string.Join("|", filters);
	}

	/// <summary>
	/// Read a movie file, auto-detecting format
	/// </summary>
	/// <param name="filePath">Path to movie file</param>
	/// <returns>Parsed movie data</returns>
	/// <exception cref="NotSupportedException">If format is not recognized</exception>
	public static MovieData Read(string filePath) {
		IMovieConverter? converter = GetConverterByExtension(Path.GetExtension(filePath))
			?? throw new NotSupportedException($"Unsupported movie format: {Path.GetExtension(filePath)}");

		if (!converter.CanRead) {
			throw new NotSupportedException($"Format {converter.FormatName} does not support reading");
		}

		return converter.Read(filePath);
	}

	/// <summary>
	/// Read a movie file asynchronously, auto-detecting format
	/// </summary>
	/// <param name="filePath">Path to movie file</param>
	/// <param name="cancellationToken">Cancellation token</param>
	/// <returns>Parsed movie data</returns>
	/// <exception cref="NotSupportedException">If format is not recognized</exception>
	public static async ValueTask<MovieData> ReadAsync(string filePath, CancellationToken cancellationToken = default) {
		IMovieConverter? converter = GetConverterByExtension(Path.GetExtension(filePath))
			?? throw new NotSupportedException($"Unsupported movie format: {Path.GetExtension(filePath)}");

		if (!converter.CanRead) {
			throw new NotSupportedException($"Format {converter.FormatName} does not support reading");
		}

		return await converter.ReadAsync(filePath, cancellationToken).ConfigureAwait(false);
	}

	/// <summary>
	/// Write a movie file in the specified format
	/// </summary>
	/// <param name="movie">Movie data to write</param>
	/// <param name="filePath">Destination path</param>
	/// <param name="format">Target format (if not specified, detected from extension)</param>
	/// <exception cref="NotSupportedException">If format cannot be written</exception>
	public static void Write(MovieData movie, string filePath, MovieFormat? format = null) {
		IMovieConverter converter = (format.HasValue ? GetConverter(format.Value) : GetConverterByExtension(Path.GetExtension(filePath)))
			?? throw new NotSupportedException($"Cannot write format: {format ?? MovieFormat.Unknown}");

		if (!converter.CanWrite) {
			throw new NotSupportedException($"Format {converter.FormatName} does not support writing");
		}

		converter.Write(movie, filePath);
	}

	/// <summary>
	/// Write a movie file asynchronously in the specified format
	/// </summary>
	/// <param name="movie">Movie data to write</param>
	/// <param name="filePath">Destination path</param>
	/// <param name="format">Target format (if not specified, detected from extension)</param>
	/// <param name="cancellationToken">Cancellation token</param>
	/// <exception cref="NotSupportedException">If format cannot be written</exception>
	public static async ValueTask WriteAsync(MovieData movie, string filePath, MovieFormat? format = null, CancellationToken cancellationToken = default) {
		IMovieConverter converter = (format.HasValue ? GetConverter(format.Value) : GetConverterByExtension(Path.GetExtension(filePath)))
			?? throw new NotSupportedException($"Cannot write format: {format ?? MovieFormat.Unknown}");

		if (!converter.CanWrite) {
			throw new NotSupportedException($"Format {converter.FormatName} does not support writing");
		}

		await converter.WriteAsync(movie, filePath, cancellationToken).ConfigureAwait(false);
	}

	/// <summary>
	/// Convert a movie from one format to another
	/// </summary>
	/// <param name="inputPath">Source file path</param>
	/// <param name="outputPath">Destination file path</param>
	/// <param name="targetFormat">Target format (optional, auto-detected from extension)</param>
	/// <returns>The converted movie data</returns>
	public static MovieData Convert(string inputPath, string outputPath, MovieFormat? targetFormat = null) {
		MovieData movie = Read(inputPath);
		Write(movie, outputPath, targetFormat);
		return movie;
	}

	/// <summary>
	/// Convert a movie asynchronously from one format to another
	/// </summary>
	/// <param name="inputPath">Source file path</param>
	/// <param name="outputPath">Destination file path</param>
	/// <param name="targetFormat">Target format (optional, auto-detected from extension)</param>
	/// <param name="cancellationToken">Cancellation token</param>
	/// <returns>The converted movie data</returns>
	public static async ValueTask<MovieData> ConvertAsync(string inputPath, string outputPath, MovieFormat? targetFormat = null, CancellationToken cancellationToken = default) {
		MovieData movie = await ReadAsync(inputPath, cancellationToken).ConfigureAwait(false);
		await WriteAsync(movie, outputPath, targetFormat, cancellationToken).ConfigureAwait(false);
		return movie;
	}

	/// <summary>
	/// Initialize with built-in converters
	/// </summary>
	private static void EnsureInitialized() {
		if (_initialized) {
			return;
		}

		lock (_lock) {
			if (_initialized) {
				return;
			}

			// Register built-in converters
			_converters.Add(new Converters.NexenMovieConverter());
			_converters.Add(new Converters.Bk2MovieConverter());
			_converters.Add(new Converters.Fm2MovieConverter());
			_converters.Add(new Converters.SmvMovieConverter());
			_converters.Add(new Converters.LsmvMovieConverter());
			_converters.Add(new Converters.VbmMovieConverter());
			_converters.Add(new Converters.GmvMovieConverter());

			// Build frozen dictionaries for O(1) lookups
			_formatLookup = _converters.ToFrozenDictionary(c => c.Format);
			_extensionLookup = _converters
				.SelectMany(c => c.Extensions.Select(ext => (Extension: ext.ToLowerInvariant(), Converter: c)))
				.ToFrozenDictionary(x => x.Extension, x => x.Converter);

			_initialized = true;
		}
	}
}
