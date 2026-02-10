using System;
using System.IO;
using Microsoft.Extensions.Logging;
using Nexen.Config;

namespace Nexen.Utilities;

/// <summary>
/// Simple logging wrapper using Microsoft.Extensions.Logging for file and console output.
/// Logs are written to the Nexen home folder as nexen-log.txt.
/// </summary>
public static class Log {
	private static bool _initialized = false;
	private static readonly object _lock = new();
	private static ILoggerFactory? _loggerFactory;
	private static ILogger? _logger;
	private static StreamWriter? _fileWriter;

	/// <summary>
	/// Gets the path to the log file.
	/// </summary>
	public static string LogFilePath => Path.Combine(
		ConfigManager.HomeFolder ?? Path.GetTempPath(),
		"nexen-log.txt"
	);

	/// <summary>
	/// Initializes the logging system. Safe to call multiple times.
	/// </summary>
	public static void Initialize() {
		if (_initialized) return;

		lock (_lock) {
			if (_initialized) return;

			try {
				string logPath = LogFilePath;

				// Ensure directory exists
				string? dir = Path.GetDirectoryName(logPath);
				if (dir is not null) {
					Directory.CreateDirectory(dir);
				}

				// Open log file for append
				_fileWriter = new StreamWriter(logPath, append: true) {
					AutoFlush = true
				};

				_loggerFactory = LoggerFactory.Create(builder => {
					builder
						.SetMinimumLevel(LogLevel.Debug)
						.AddConsole(options => {
							options.FormatterName = "simple";
						})
						.AddProvider(new FileLoggerProvider(_fileWriter));
				});

				_logger = _loggerFactory.CreateLogger("Nexen");
				_initialized = true;
				Info("=== Nexen logging initialized ===");
				Info($"Log file: {logPath}");
			} catch (Exception ex) {
				// If logging fails to initialize, write to debug output
				System.Diagnostics.Debug.WriteLine($"Failed to initialize logging: {ex}");
			}
		}
	}

	/// <summary>
	/// Ensures logging is initialized before writing.
	/// </summary>
	private static void EnsureInitialized() {
		if (!_initialized) {
			Initialize();
		}
	}

	/// <summary>
	/// Logs a debug message.
	/// </summary>
	public static void Debug(string message) {
		EnsureInitialized();
		_logger?.LogDebug("{Message}", message);
	}

	/// <summary>
	/// Logs an informational message.
	/// </summary>
	public static void Info(string message) {
		EnsureInitialized();
		_logger?.LogInformation("{Message}", message);
	}

	/// <summary>
	/// Logs a warning message.
	/// </summary>
	public static void Warn(string message) {
		EnsureInitialized();
		_logger?.LogWarning("{Message}", message);
	}

	/// <summary>
	/// Logs an error message.
	/// </summary>
	public static void Error(string message) {
		EnsureInitialized();
		_logger?.LogError("{Message}", message);
	}

	/// <summary>
	/// Logs an error with exception details.
	/// </summary>
	public static void Error(Exception ex, string message) {
		EnsureInitialized();
		_logger?.LogError(ex, "{Message}", message);
	}

	/// <summary>
	/// Logs a fatal error that will likely crash the application.
	/// </summary>
	public static void Fatal(string message) {
		EnsureInitialized();
		_logger?.LogCritical("{Message}", message);
	}

	/// <summary>
	/// Logs a fatal error with exception details.
	/// </summary>
	public static void Fatal(Exception ex, string message) {
		EnsureInitialized();
		_logger?.LogCritical(ex, "{Message}", message);
	}

	/// <summary>
	/// Flushes any buffered log entries and closes the log.
	/// Call this before application exit.
	/// </summary>
	public static void CloseAndFlush() {
		_loggerFactory?.Dispose();
		_fileWriter?.Flush();
		_fileWriter?.Dispose();
		_fileWriter = null;
		_loggerFactory = null;
		_logger = null;
		_initialized = false;
	}
}

/// <summary>
/// Simple file logging provider for Microsoft.Extensions.Logging.
/// Writes formatted log entries to a StreamWriter.
/// </summary>
internal sealed class FileLoggerProvider : ILoggerProvider {
	private readonly StreamWriter _writer;

	public FileLoggerProvider(StreamWriter writer) {
		_writer = writer;
	}

	public ILogger CreateLogger(string categoryName) => new FileLogger(_writer, categoryName);

	public void Dispose() { }
}

/// <summary>
/// Simple file logger that writes formatted entries to a stream.
/// </summary>
internal sealed class FileLogger : ILogger {
	private readonly StreamWriter _writer;
	private readonly string _category;

	public FileLogger(StreamWriter writer, string category) {
		_writer = writer;
		_category = category;
	}

	public IDisposable? BeginScope<TState>(TState state) where TState : notnull => null;

	public bool IsEnabled(LogLevel logLevel) => logLevel >= LogLevel.Debug;

	public void Log<TState>(LogLevel logLevel, EventId eventId, TState state, Exception? exception, Func<TState, Exception?, string> formatter) {
		if (!IsEnabled(logLevel)) return;

		string level = logLevel switch {
			LogLevel.Trace => "TRC",
			LogLevel.Debug => "DBG",
			LogLevel.Information => "INF",
			LogLevel.Warning => "WRN",
			LogLevel.Error => "ERR",
			LogLevel.Critical => "CRT",
			_ => "???"
		};

		string timestamp = DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss.fff");
		string message = formatter(state, exception);

		try {
			_writer.WriteLine($"{timestamp} [{level}] {message}");
			if (exception is not null) {
				_writer.WriteLine(exception.ToString());
			}
		} catch {
			// Silently ignore write failures
		}
	}
}
