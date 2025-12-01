namespace Mesen.MovieConverter;

/// <summary>
/// MesenMovieConverter - TAS Movie Format Conversion Tool
/// 
/// Converts between various emulator movie formats:
/// - Mesen (.mmm)
/// - Snes9x (.smv)
/// - lsnes (.lsmv)
/// - FCEUX (.fm2)
/// - BizHawk (.bk2)
/// 
/// Usage:
///   MesenMovieConverter <input> <output> [options]
///   MesenMovieConverter --info <input>
///   MesenMovieConverter --list-formats
/// 
/// Options:
///   --author <name>      Set author name for output
///   --description <text> Set description for output
///   --verbose            Show detailed output
///   --force              Overwrite existing output file
/// </summary>
class Program
{
    static int Main(string[] args)
    {
        Console.WriteLine("Mesen Movie Converter v1.0");
        Console.WriteLine("TAS Movie Format Conversion Tool");
        Console.WriteLine();

        if (args.Length == 0)
        {
            PrintUsage();
            return 1;
        }

        try
        {
            return ProcessCommand(args);
        }
        catch (Exception ex)
        {
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine($"Error: {ex.Message}");
            Console.ResetColor();
            
            if (args.Contains("--verbose"))
            {
                Console.WriteLine(ex.StackTrace);
            }
            
            return 1;
        }
    }

    static int ProcessCommand(string[] args)
    {
        // Check for special commands
        if (args[0] == "--help" || args[0] == "-h")
        {
            PrintUsage();
            return 0;
        }

        if (args[0] == "--list-formats")
        {
            ListFormats();
            return 0;
        }

        if (args[0] == "--info" && args.Length >= 2)
        {
            return ShowMovieInfo(args[1]);
        }

        // Standard conversion: input output [options]
        if (args.Length < 2)
        {
            Console.WriteLine("Error: Please specify both input and output files.");
            PrintUsage();
            return 1;
        }

        string inputPath = args[0];
        string outputPath = args[1];
        
        // Parse options
        bool verbose = args.Contains("--verbose");
        bool force = args.Contains("--force");
        string? author = GetOption(args, "--author");
        string? description = GetOption(args, "--description");

        return ConvertMovie(inputPath, outputPath, verbose, force, author, description);
    }

    static int ConvertMovie(string inputPath, string outputPath, bool verbose, bool force,
        string? author, string? description)
    {
        // Validate input
        if (!File.Exists(inputPath))
        {
            Console.WriteLine($"Error: Input file not found: {inputPath}");
            return 1;
        }

        // Check output
        if (File.Exists(outputPath) && !force)
        {
            Console.WriteLine($"Error: Output file already exists: {outputPath}");
            Console.WriteLine("Use --force to overwrite.");
            return 1;
        }

        // Get converters
        var inputFormat = MovieConverterRegistry.DetectFormat(inputPath);
        var outputFormat = MovieConverterRegistry.DetectFormat(outputPath);

        if (inputFormat == MovieFormat.Unknown)
        {
            Console.WriteLine($"Error: Unknown input format: {Path.GetExtension(inputPath)}");
            return 1;
        }

        if (outputFormat == MovieFormat.Unknown)
        {
            Console.WriteLine($"Error: Unknown output format: {Path.GetExtension(outputPath)}");
            return 1;
        }

        var reader = MovieConverterRegistry.GetConverter(inputFormat);
        var writer = MovieConverterRegistry.GetConverter(outputFormat);

        if (reader == null || !reader.CanRead)
        {
            Console.WriteLine($"Error: Cannot read {inputFormat} format.");
            return 1;
        }

        if (writer == null || !writer.CanWrite)
        {
            Console.WriteLine($"Error: Cannot write {outputFormat} format.");
            return 1;
        }

        // Read input
        Console.WriteLine($"Reading: {inputPath}");
        Console.WriteLine($"Format: {reader.FormatName}");
        
        var movie = reader.Read(inputPath);

        if (verbose)
        {
            Console.WriteLine();
            Console.WriteLine(movie.GetSummary());
            Console.WriteLine();
        }

        // Apply modifications
        if (!string.IsNullOrEmpty(author))
        {
            movie.Author = author;
            Console.WriteLine($"Set author: {author}");
        }

        if (!string.IsNullOrEmpty(description))
        {
            movie.Description = description;
            Console.WriteLine($"Set description: {description}");
        }

        // Write output
        Console.WriteLine($"Writing: {outputPath}");
        Console.WriteLine($"Format: {writer.FormatName}");

        writer.Write(movie, outputPath);

        Console.ForegroundColor = ConsoleColor.Green;
        Console.WriteLine();
        Console.WriteLine("Conversion successful!");
        Console.ResetColor();

        Console.WriteLine($"  Frames: {movie.TotalFrames:N0}");
        Console.WriteLine($"  Duration: {movie.Duration:hh\\:mm\\:ss\\.fff}");
        Console.WriteLine($"  Rerecords: {movie.RerecordCount:N0}");

        return 0;
    }

    static int ShowMovieInfo(string inputPath)
    {
        if (!File.Exists(inputPath))
        {
            Console.WriteLine($"Error: File not found: {inputPath}");
            return 1;
        }

        var format = MovieConverterRegistry.DetectFormat(inputPath);
        if (format == MovieFormat.Unknown)
        {
            Console.WriteLine($"Error: Unknown format: {Path.GetExtension(inputPath)}");
            return 1;
        }

        var reader = MovieConverterRegistry.GetConverter(format);
        if (reader == null)
        {
            Console.WriteLine($"Error: No converter for {format}");
            return 1;
        }

        Console.WriteLine($"File: {inputPath}");
        Console.WriteLine();

        var movie = reader.Read(inputPath);
        Console.WriteLine(movie.GetSummary());

        return 0;
    }

    static void ListFormats()
    {
        Console.WriteLine("Supported Formats:");
        Console.WriteLine();

        foreach (var converter in MovieConverterRegistry.Converters)
        {
            Console.Write($"  {converter.FormatName,-20}");
            Console.Write($" {string.Join(", ", converter.Extensions),-15}");
            Console.Write($" Read:{(converter.CanRead ? "Yes" : "No"),-4}");
            Console.WriteLine($" Write:{(converter.CanWrite ? "Yes" : "No")}");
        }

        Console.WriteLine();
        Console.WriteLine("Format Details:");
        Console.WriteLine("  .mmm  - Mesen2 native format (ZIP archive with text input log)");
        Console.WriteLine("  .smv  - Snes9x movie (binary format, SNES)");
        Console.WriteLine("  .lsmv - lsnes movie (ZIP archive, SNES)");
        Console.WriteLine("  .fm2  - FCEUX movie (text format, NES)");
        Console.WriteLine("  .bk2  - BizHawk movie (ZIP archive, multi-system)");
    }

    static void PrintUsage()
    {
        Console.WriteLine("Usage:");
        Console.WriteLine("  MesenMovieConverter <input> <output> [options]");
        Console.WriteLine("  MesenMovieConverter --info <input>");
        Console.WriteLine("  MesenMovieConverter --list-formats");
        Console.WriteLine();
        Console.WriteLine("Options:");
        Console.WriteLine("  --author <name>      Set author name for output");
        Console.WriteLine("  --description <text> Set description for output");
        Console.WriteLine("  --verbose            Show detailed output");
        Console.WriteLine("  --force              Overwrite existing output file");
        Console.WriteLine("  --help               Show this help");
        Console.WriteLine();
        Console.WriteLine("Examples:");
        Console.WriteLine("  MesenMovieConverter movie.smv movie.mmm");
        Console.WriteLine("  MesenMovieConverter movie.mmm movie.lsmv --author \"MyName\"");
        Console.WriteLine("  MesenMovieConverter --info movie.smv");
        Console.WriteLine();
        Console.WriteLine("Supported formats:");
        
        foreach (var converter in MovieConverterRegistry.Converters)
        {
            Console.WriteLine($"  {string.Join(", ", converter.Extensions),-12} {converter.FormatName}");
        }
    }

    static string? GetOption(string[] args, string option)
    {
        for (int i = 0; i < args.Length - 1; i++)
        {
            if (args[i] == option)
                return args[i + 1];
        }
        return null;
    }
}
