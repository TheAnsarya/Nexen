#pragma once
#include "pch.h"

/// <summary>
/// Trace log file saver with buffered writing.
/// </summary>
/// <remarks>
/// Architecture:
/// - Buffers trace log entries before writing to disk
/// - Flushes buffer when full (32KB threshold)
/// - Binary output mode for performance
/// 
/// Buffering strategy:
/// - Accumulates log entries in-memory
/// - Writes to disk when buffer > 32KB
/// - Reduces disk I/O overhead for high-frequency logging
/// 
/// Use cases:
/// - Instruction trace logging (CPU execution)
/// - PPU trace logging (PPU cycles)
/// - Custom trace logs via Lua scripts
/// </remarks>
class TraceLogFileSaver {
private:
	bool _enabled = false;      ///< True if logging active
	string _outputFilepath;     ///< Output file path
	string _outputBuffer;       ///< In-memory buffer
	ofstream _outputFile;       ///< Output file stream

public:
	/// <summary>
	/// Destructor - stops logging and flushes buffer.
	/// </summary>
	~TraceLogFileSaver() {
		StopLogging();
	}

	/// <summary>
	/// Start logging to file.
	/// </summary>
	/// <param name="filename">Output file path</param>
	void StartLogging(string filename) {
		_outputBuffer.clear();
		_outputFile.open(filename, ios::out | ios::binary);
		_enabled = true;
	}

	/// <summary>
	/// Stop logging and flush buffer.
	/// </summary>
	void StopLogging() {
		if (_enabled) {
			_enabled = false;
			if (_outputFile) {
				if (!_outputBuffer.empty()) {
					_outputFile << _outputBuffer;
				}
				_outputFile.close();
			}
		}
	}

	/// <summary>
	/// Check if logging enabled (hot path).
	/// </summary>
	/// <remarks>
	/// Inline for performance - called frequently to skip logging.
	/// </remarks>
	__forceinline bool IsEnabled() { return _enabled; }

	/// <summary>
	/// Log entry with buffering.
	/// </summary>
	/// <param name="log">Log entry text</param>
	void Log(string& log) {
		_outputBuffer += log + '\n';
		if (_outputBuffer.size() > 32768) {
			_outputFile << _outputBuffer;
			_outputBuffer.clear();
		}
	}
};