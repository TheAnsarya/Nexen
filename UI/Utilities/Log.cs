using System;
using System.IO;
using Microsoft.Extensions.Logging;
using Mesen.Config;

namespace Mesen.Utilities;

/// <summary>
/// Logging wrapper over Microsoft.Extensions.Logging that writes to the console and to nexen-log.txt in the home folder.
/// </summary>
public static class Log {
	private static bool _initialized = false;
	private static readonly object _lock = new();
	private static ILoggerFactory? _loggerFactory;
	private static ILogger? _logger;
	private static StreamWriter? _fileWriter;

	public static string LogFilePath => Path.Combine(
		ConfigManager.HomeFolder ?? Path.GetTempPath(),
		"nexen-log.txt"
	);

	/// <summary>
	/// Starts console-only logging; does not touch ConfigManager.HomeFolder so no directories are created.
	/// </summary>
	public static void Initialize() {
		if (_initialized) {
			return;
		}

		lock (_lock) {
			if (_initialized) {
				return;
			}

			try {
				_loggerFactory = LoggerFactory.Create(builder => {
					builder
						.SetMinimumLevel(LogLevel.Debug)
						.AddConsole(options => {
							options.FormatterName = "simple";
						});
				});

				_logger = _loggerFactory.CreateLogger("Nexen");
				_initialized = true;
				Info("=== Nexen logging initialized (console) ===");
			} catch (Exception ex) {
				System.Diagnostics.Debug.WriteLine($"Failed to initialize logging: {ex}");
			}
		}
	}

	/// <summary>
	/// Adds the file provider once the home folder is known; safe to call repeatedly.
	/// </summary>
	public static void InitializeFileLogging() {
		if (_fileWriter is not null) {
			return;
		}

		lock (_lock) {
			if (_fileWriter is not null) {
				return;
			}

			try {
				string logPath = LogFilePath;

				string? dir = Path.GetDirectoryName(logPath);
				if (dir is not null) {
					Directory.CreateDirectory(dir);
				}

				_fileWriter = new StreamWriter(logPath, append: true) {
					AutoFlush = true
				};

				_loggerFactory?.Dispose();
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
				Info($"Log file: {logPath}");
			} catch (Exception ex) {
				System.Diagnostics.Debug.WriteLine($"Failed to initialize file logging: {ex}");
			}
		}
	}

	private static void EnsureInitialized() {
		if (!_initialized) {
			Initialize();
		}
	}

	public static void Debug(string message) {
		EnsureInitialized();
		_logger?.LogDebug("{Message}", message);
	}

	public static void Info(string message) {
		EnsureInitialized();
		_logger?.LogInformation("{Message}", message);
	}

	public static void Warn(string message) {
		EnsureInitialized();
		_logger?.LogWarning("{Message}", message);
	}

	public static void Error(string message) {
		EnsureInitialized();
		_logger?.LogError("{Message}", message);
	}

	public static void Error(Exception ex, string message) {
		EnsureInitialized();
		_logger?.LogError(ex, "{Message}", message);
	}

	public static void Fatal(string message) {
		EnsureInitialized();
		_logger?.LogCritical("{Message}", message);
	}

	public static void Fatal(Exception ex, string message) {
		EnsureInitialized();
		_logger?.LogCritical(ex, "{Message}", message);
	}

	/// <summary>
	/// Flushes and closes the log; call before application exit.
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

internal sealed class FileLoggerProvider : ILoggerProvider {
	private readonly StreamWriter _writer;

	public FileLoggerProvider(StreamWriter writer) {
		_writer = writer;
	}

	public ILogger CreateLogger(string categoryName) => new FileLogger(_writer, categoryName);

	public void Dispose() { }
}

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
		if (!IsEnabled(logLevel)) {
			return;
		}

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
			lock (_writer) {
				_writer.WriteLine($"{timestamp} [{level}] {message}");
				if (exception is not null) {
					_writer.WriteLine(exception.ToString());
				}
			}
		} catch (Exception writeEx) {
			// Avoid recursive logging; write to debug output only
			System.Diagnostics.Debug.WriteLine($"Log write failure: {writeEx.Message}");
		}
	}
}
