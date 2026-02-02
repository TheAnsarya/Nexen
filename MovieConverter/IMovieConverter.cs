namespace Nexen.MovieConverter;

/// <summary>
/// Interface for movie format converters.
/// Each supported format implements this interface for reading and writing.
/// </summary>
public interface IMovieConverter {
	/// <summary>
	/// File extension(s) this converter handles (e.g., [".bk2"], [".lsmv"])
	/// </summary>
	string[] Extensions { get; }

	/// <summary>
	/// Human-readable name of the format (e.g., "BizHawk Movie")
	/// </summary>
	string FormatName { get; }

	/// <summary>
	/// The MovieFormat enum value for this converter
	/// </summary>
	MovieFormat Format { get; }

	/// <summary>
	/// Whether this converter can read movies
	/// </summary>
	bool CanRead { get; }

	/// <summary>
	/// Whether this converter can write movies
	/// </summary>
	bool CanWrite { get; }

	/// <summary>
	/// Read a movie file and return common MovieData
	/// </summary>
	/// <param name="filePath">Path to the movie file</param>
	/// <returns>Parsed movie data</returns>
	MovieData Read(string filePath);

	/// <summary>
	/// Read a movie from a stream
	/// </summary>
	/// <param name="stream">Stream containing movie data</param>
	/// <param name="fileName">Optional filename for format hints</param>
	/// <returns>Parsed movie data</returns>
	MovieData Read(Stream stream, string? fileName = null);

	/// <summary>
	/// Read a movie file asynchronously
	/// </summary>
	/// <param name="filePath">Path to the movie file</param>
	/// <param name="cancellationToken">Cancellation token</param>
	/// <returns>Parsed movie data</returns>
	ValueTask<MovieData> ReadAsync(string filePath, CancellationToken cancellationToken = default);

	/// <summary>
	/// Read a movie from a stream asynchronously
	/// </summary>
	/// <param name="stream">Stream containing movie data</param>
	/// <param name="fileName">Optional filename for format hints</param>
	/// <param name="cancellationToken">Cancellation token</param>
	/// <returns>Parsed movie data</returns>
	ValueTask<MovieData> ReadAsync(Stream stream, string? fileName = null, CancellationToken cancellationToken = default);

	/// <summary>
	/// Write MovieData to a file in this format
	/// </summary>
	/// <param name="movie">Movie data to write</param>
	/// <param name="filePath">Destination file path</param>
	void Write(MovieData movie, string filePath);

	/// <summary>
	/// Write MovieData to a stream in this format
	/// </summary>
	/// <param name="movie">Movie data to write</param>
	/// <param name="stream">Destination stream</param>
	void Write(MovieData movie, Stream stream);

	/// <summary>
	/// Write MovieData to a file asynchronously
	/// </summary>
	/// <param name="movie">Movie data to write</param>
	/// <param name="filePath">Destination file path</param>
	/// <param name="cancellationToken">Cancellation token</param>
	ValueTask WriteAsync(MovieData movie, string filePath, CancellationToken cancellationToken = default);

	/// <summary>
	/// Write MovieData to a stream asynchronously
	/// </summary>
	/// <param name="movie">Movie data to write</param>
	/// <param name="stream">Destination stream</param>
	/// <param name="cancellationToken">Cancellation token</param>
	ValueTask WriteAsync(MovieData movie, Stream stream, CancellationToken cancellationToken = default);
}

/// <summary>
/// Base class with common functionality for movie converters
/// </summary>
public abstract class MovieConverterBase : IMovieConverter {
	/// <inheritdoc/>
	public abstract string[] Extensions { get; }

	/// <inheritdoc/>
	public abstract string FormatName { get; }

	/// <inheritdoc/>
	public abstract MovieFormat Format { get; }

	/// <inheritdoc/>
	public virtual bool CanRead => true;

	/// <inheritdoc/>
	public virtual bool CanWrite => true;

	/// <inheritdoc/>
	public virtual MovieData Read(string filePath) {
		using FileStream stream = File.OpenRead(filePath);
		return Read(stream, Path.GetFileName(filePath));
	}

	/// <inheritdoc/>
	public abstract MovieData Read(Stream stream, string? fileName = null);

	/// <inheritdoc/>
	public virtual async ValueTask<MovieData> ReadAsync(string filePath, CancellationToken cancellationToken = default) {
		FileStream stream = new(filePath, FileMode.Open, FileAccess.Read, FileShare.Read, 4096, useAsync: true);
		await using (stream.ConfigureAwait(false)) {
			return await ReadAsync(stream, Path.GetFileName(filePath), cancellationToken).ConfigureAwait(false);
		}
	}

	/// <inheritdoc/>
	public virtual ValueTask<MovieData> ReadAsync(Stream stream, string? fileName = null, CancellationToken cancellationToken = default) {
		// Default implementation: wrap sync method
		// Override in derived classes for true async I/O
		cancellationToken.ThrowIfCancellationRequested();
		return ValueTask.FromResult(Read(stream, fileName));
	}

	/// <inheritdoc/>
	public virtual void Write(MovieData movie, string filePath) {
		// Ensure directory exists
		string? dir = Path.GetDirectoryName(filePath);
		if (!string.IsNullOrEmpty(dir) && !Directory.Exists(dir)) {
			Directory.CreateDirectory(dir);
		}

		using FileStream stream = File.Create(filePath);
		Write(movie, stream);
	}

	/// <inheritdoc/>
	public abstract void Write(MovieData movie, Stream stream);

	/// <inheritdoc/>
	public virtual async ValueTask WriteAsync(MovieData movie, string filePath, CancellationToken cancellationToken = default) {
		// Ensure directory exists
		string? dir = Path.GetDirectoryName(filePath);
		if (!string.IsNullOrEmpty(dir) && !Directory.Exists(dir)) {
			Directory.CreateDirectory(dir);
		}

		FileStream stream = new(filePath, FileMode.Create, FileAccess.Write, FileShare.None, 4096, useAsync: true);
		await using (stream.ConfigureAwait(false)) {
			await WriteAsync(movie, stream, cancellationToken).ConfigureAwait(false);
		}
	}

	/// <inheritdoc/>
	public virtual ValueTask WriteAsync(MovieData movie, Stream stream, CancellationToken cancellationToken = default) {
		// Default implementation: wrap sync method
		// Override in derived classes for true async I/O
		cancellationToken.ThrowIfCancellationRequested();
		Write(movie, stream);
		return ValueTask.CompletedTask;
	}

	/// <summary>
	/// Read all bytes from a stream into a byte array
	/// </summary>
	/// <param name="stream">Source stream</param>
	/// <returns>Byte array with all stream contents</returns>
	protected static byte[] ReadAllBytes(Stream stream) {
		if (stream is MemoryStream ms) {
			return ms.ToArray();
		}

		using var memStream = new MemoryStream();
		stream.CopyTo(memStream);
		return memStream.ToArray();
	}

	/// <summary>
	/// Read all bytes from a stream asynchronously
	/// </summary>
	/// <param name="stream">Source stream</param>
	/// <param name="cancellationToken">Cancellation token</param>
	/// <returns>Byte array with all stream contents</returns>
	protected static async ValueTask<byte[]> ReadAllBytesAsync(Stream stream, CancellationToken cancellationToken = default) {
		if (stream is MemoryStream ms) {
			return ms.ToArray();
		}

		using var memStream = new MemoryStream();
		await stream.CopyToAsync(memStream, cancellationToken).ConfigureAwait(false);
		return memStream.ToArray();
	}

	/// <summary>
	/// Read all text from a stream
	/// </summary>
	/// <param name="stream">Source stream</param>
	/// <returns>String contents of stream</returns>
	protected static string ReadAllText(Stream stream) {
		using var reader = new StreamReader(stream, leaveOpen: true);
		return reader.ReadToEnd();
	}

	/// <summary>
	/// Read all lines from a stream
	/// </summary>
	/// <param name="stream">Source stream</param>
	/// <returns>Array of lines</returns>
	protected static string[] ReadAllLines(Stream stream) {
		var lines = new List<string>();
		using var reader = new StreamReader(stream, leaveOpen: true);

		while (reader.ReadLine() is { } line) {
			lines.Add(line);
		}

		return [.. lines];
	}

	/// <summary>
	/// Read all lines from a stream asynchronously
	/// </summary>
	/// <param name="stream">Source stream</param>
	/// <param name="cancellationToken">Cancellation token</param>
	/// <returns>Array of lines</returns>
	protected static async ValueTask<string[]> ReadAllLinesAsync(Stream stream, CancellationToken cancellationToken = default) {
		var lines = new List<string>();
		using var reader = new StreamReader(stream, leaveOpen: true);

		while (await reader.ReadLineAsync(cancellationToken).ConfigureAwait(false) is { } line) {
			lines.Add(line);
		}

		return [.. lines];
	}
}

/// <summary>
/// Exception thrown when a movie file is malformed or invalid
/// </summary>
public sealed class MovieFormatException : Exception {
	/// <summary>The format that failed to parse</summary>
	public MovieFormat Format { get; }

	public MovieFormatException(MovieFormat format, string message)
		: base($"[{format}] {message}") {
		Format = format;
	}

	public MovieFormatException(MovieFormat format, string message, Exception innerException)
		: base($"[{format}] {message}", innerException) {
		Format = format;
	}

	public MovieFormatException() {
	}
}
