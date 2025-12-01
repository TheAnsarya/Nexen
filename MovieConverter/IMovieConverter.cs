namespace Mesen.MovieConverter;

/// <summary>
/// Base interface for movie format readers/writers
/// </summary>
public interface IMovieConverter
{
    /// <summary>
    /// File extension(s) this converter handles
    /// </summary>
    string[] Extensions { get; }

    /// <summary>
    /// Display name of the format
    /// </summary>
    string FormatName { get; }

    /// <summary>
    /// The movie format enum value
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
    MovieData Read(string filePath);

    /// <summary>
    /// Read a movie from a stream
    /// </summary>
    MovieData Read(Stream stream, string? fileName = null);

    /// <summary>
    /// Write MovieData to a file in this format
    /// </summary>
    void Write(MovieData movie, string filePath);

    /// <summary>
    /// Write MovieData to a stream in this format
    /// </summary>
    void Write(MovieData movie, Stream stream);
}

/// <summary>
/// Base class with common functionality
/// </summary>
public abstract class MovieConverterBase : IMovieConverter
{
    public abstract string[] Extensions { get; }
    public abstract string FormatName { get; }
    public abstract MovieFormat Format { get; }
    public virtual bool CanRead => true;
    public virtual bool CanWrite => true;

    public virtual MovieData Read(string filePath)
    {
        using var stream = File.OpenRead(filePath);
        return Read(stream, Path.GetFileName(filePath));
    }

    public abstract MovieData Read(Stream stream, string? fileName = null);

    public virtual void Write(MovieData movie, string filePath)
    {
        using var stream = File.Create(filePath);
        Write(movie, stream);
    }

    public abstract void Write(MovieData movie, Stream stream);

    protected static byte[] ReadAllBytes(Stream stream)
    {
        if (stream is MemoryStream ms)
            return ms.ToArray();

        using var memStream = new MemoryStream();
        stream.CopyTo(memStream);
        return memStream.ToArray();
    }
}
