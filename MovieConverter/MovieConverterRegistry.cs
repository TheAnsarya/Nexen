using Mesen.MovieConverter.Converters;

namespace Mesen.MovieConverter;

/// <summary>
/// Registry of all available movie converters
/// </summary>
public static class MovieConverterRegistry
{
    private static readonly List<IMovieConverter> _converters = new()
    {
        new MesenMovieConverter(),
        new SmvMovieConverter(),
        new LsmvMovieConverter(),
        new Fm2MovieConverter(),
        new Bk2MovieConverter()
    };

    /// <summary>
    /// Get all registered converters
    /// </summary>
    public static IReadOnlyList<IMovieConverter> Converters => _converters;

    /// <summary>
    /// Get converter by format
    /// </summary>
    public static IMovieConverter? GetConverter(MovieFormat format)
    {
        return _converters.FirstOrDefault(c => c.Format == format);
    }

    /// <summary>
    /// Get converter by file extension
    /// </summary>
    public static IMovieConverter? GetConverterByExtension(string extension)
    {
        extension = extension.ToLower();
        if (!extension.StartsWith("."))
            extension = "." + extension;

        return _converters.FirstOrDefault(c => 
            c.Extensions.Any(e => e.Equals(extension, StringComparison.OrdinalIgnoreCase)));
    }

    /// <summary>
    /// Detect format from file
    /// </summary>
    public static MovieFormat DetectFormat(string filePath)
    {
        var ext = Path.GetExtension(filePath).ToLower();
        var converter = GetConverterByExtension(ext);
        return converter?.Format ?? MovieFormat.Unknown;
    }

    /// <summary>
    /// Get all supported extensions for reading
    /// </summary>
    public static IEnumerable<string> GetReadableExtensions()
    {
        return _converters.Where(c => c.CanRead).SelectMany(c => c.Extensions);
    }

    /// <summary>
    /// Get all supported extensions for writing
    /// </summary>
    public static IEnumerable<string> GetWritableExtensions()
    {
        return _converters.Where(c => c.CanWrite).SelectMany(c => c.Extensions);
    }

    /// <summary>
    /// Get format display string
    /// </summary>
    public static string GetFormatDescription(MovieFormat format)
    {
        var converter = GetConverter(format);
        if (converter == null) return format.ToString();
        
        return $"{converter.FormatName} ({string.Join(", ", converter.Extensions)})";
    }

    /// <summary>
    /// Register a custom converter
    /// </summary>
    public static void RegisterConverter(IMovieConverter converter)
    {
        _converters.Add(converter);
    }
}
