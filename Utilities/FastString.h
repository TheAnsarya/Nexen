#pragma once
#include "pch.h"
#include <cctype>

/// <summary>
/// High-performance stack-allocated string builder with 1000-byte fixed buffer.
/// Avoids heap allocations for temporary string construction in performance-critical paths.
/// Designed for building short strings (debugger output, hex dumps, status messages).
/// </summary>
/// <remarks>
/// Performance characteristics:
/// - Stack-allocated: No malloc/free overhead
/// - Fixed 1000-byte buffer: Sufficient for most use cases, overflow is undefined behavior
/// - Optional automatic lowercase conversion during writes
/// - Variadic template WriteAll() for multiple arguments in one call
/// WARNING: No bounds checking on most Write() methods - caller must ensure buffer capacity.
/// Use WriteSafe() if buffer overflow is a concern.
/// </remarks>
class FastString {
private:
	char _buffer[1000];      ///< Fixed-size stack buffer (1000 bytes)
	uint16_t _pos = 0;       ///< Current write position (0-999)
	bool _lowerCase = false; ///< Automatic lowercase conversion flag

	/// <summary>
	/// Variadic template recursion base case (no-op).
	/// </summary>
	void WriteAll() {}

public:
#ifdef _MSC_VER
#pragma warning(disable : 26495)
#endif
	/// <summary>
	/// Construct empty FastString with optional automatic lowercase conversion.
	/// </summary>
	/// <param name="lowerCase">If true, all written text is converted to lowercase</param>
	FastString(bool lowerCase = false) { _lowerCase = lowerCase; }

	/// <summary>
	/// Construct FastString initialized with C-string data.
	/// </summary>
	/// <param name="str">Pointer to character data</param>
	/// <param name="size">Number of characters to copy</param>
	FastString(const char* str, int size) { Write(str, size); }

	/// <summary>
	/// Construct FastString initialized with std::string data.
	/// </summary>
	/// <param name="str">String to copy</param>
	FastString(string& str) { Write(str); }
#ifdef _MSC_VER
#pragma warning(default : 26495)
#endif

	/// <summary>
	/// Write single character with bounds checking (safe for untrusted input).
	/// </summary>
	/// <param name="c">Character to write</param>
	/// <remarks>
	/// Silently ignores write if buffer is full (at position 999).
	/// Use this variant when buffer overflow is possible.
	/// </remarks>
	void WriteSafe(char c) {
		if (_pos < 999) {
			_buffer[_pos++] = c;
		}
	}

	/// <summary>
	/// Write single character with optional lowercase conversion.
	/// </summary>
	/// <param name="c">Character to write</param>
	/// <remarks>
	/// NO BOUNDS CHECKING - undefined behavior if buffer is full.
	/// Applies lowercase conversion if enabled in constructor.
	/// Uses unsigned char cast to avoid undefined behavior for values > 127.
	/// </remarks>
	void Write(char c) {
		if (_lowerCase) {
			_buffer[_pos++] = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		} else {
			_buffer[_pos++] = c;
		}
	}

	/// <summary>
	/// Write fixed-length C-string with optional lowercase conversion.
	/// </summary>
	/// <param name="str">Pointer to character data</param>
	/// <param name="size">Number of characters to write</param>
	/// <remarks>
	/// NO BOUNDS CHECKING - undefined behavior if size exceeds remaining buffer space.
	/// If lowercase enabled, converts each character individually.
	/// If lowercase disabled, uses fast memcpy for optimal performance.
	/// </remarks>
	void Write(const char* str, int size) {
		if (_lowerCase) {
			for (int i = 0; i < size; i++) {
				_buffer[_pos + i] = static_cast<char>(std::tolower(static_cast<unsigned char>(str[i])));
			}
		} else {
			memcpy(_buffer + _pos, str, size);
		}
		_pos += static_cast<uint16_t>(size);
	}

	/// <summary>
	/// Write delimiter string only if buffer is non-empty (avoids leading delimiters).
	/// </summary>
	/// <param name="str">Null-terminated delimiter string (e.g., ", ", " | ")</param>
	/// <remarks>
	/// Common pattern for comma-separated output: Delimiter(", ") between elements.
	/// Automatically measures string length with strlen().
	/// </remarks>
	void Delimiter(const char* str) {
		if (_pos > 0) {
			Write(str, (uint16_t)strlen(str));
		}
	}

	/// <summary>
	/// Write null-terminated C-string with automatic length measurement.
	/// </summary>
	/// <param name="str">Null-terminated string to write</param>
	/// <remarks>
	/// Calls strlen() to measure length, then delegates to Write(const char*, int).
	/// NO BOUNDS CHECKING on result.
	/// </remarks>
	void Write(const char* str) {
		Write(str, (uint16_t)strlen(str));
	}

	/// <summary>
	/// Write std::string with optional case preservation override.
	/// </summary>
	/// <param name="str">String to write</param>
	/// <param name="preserveCase">If true, disables lowercase conversion for this write only</param>
	/// <remarks>
	/// If lowercase enabled and preserveCase=false, converts each character individually.
	/// If lowercase disabled or preserveCase=true, uses fast memcpy.
	/// NO BOUNDS CHECKING - undefined behavior if string exceeds remaining buffer space.
	/// </remarks>
	void Write(string& str, bool preserveCase = false) {
		if (_lowerCase && !preserveCase) {
			for (size_t i = 0; i < str.size(); i++) {
				_buffer[_pos + i] = static_cast<char>(std::tolower(static_cast<unsigned char>(str[i])));
			}
		} else {
			memcpy(_buffer + _pos, str.c_str(), str.size());
		}
		_pos += (uint16_t)str.size();
	}

	/// <summary>
	/// Write entire contents of another FastString.
	/// </summary>
	/// <param name="str">Source FastString to copy from</param>
	/// <remarks>
	/// Uses fast memcpy to copy buffer contents (no character processing).
	/// NO BOUNDS CHECKING - undefined behavior if source exceeds remaining buffer space.
	/// </remarks>
	void Write(FastString& str) {
		memcpy(_buffer + _pos, str._buffer, str._pos);
		_pos += str._pos;
	}

	/// <summary>
	/// Get null-terminated C-string representation of buffer contents.
	/// </summary>
	/// <returns>Pointer to internal buffer (valid until next write or destruction)</returns>
	/// <remarks>
	/// Writes null terminator at current position (does NOT increment position).
	/// Returned pointer is only valid until next Write() call or FastString destruction.
	/// Do NOT free() the returned pointer (stack-allocated buffer).
	/// </remarks>
	const char* ToString() {
		_buffer[_pos] = 0;
		return _buffer;
	}

	/// <summary>
	/// Get current number of characters in buffer.
	/// </summary>
	/// <returns>Character count (0-999)</returns>
	uint16_t GetSize() {
		return _pos;
	}

	/// <summary>
	/// Reset buffer to empty state without clearing memory.
	/// </summary>
	/// <remarks>
	/// Sets position to 0 but does not zero buffer contents (for performance).
	/// Next ToString() will null-terminate at position 0.
	/// </remarks>
	void Reset() {
		_pos = 0;
	}

	/// <summary>
	/// Write multiple arguments in sequence (variadic template).
	/// </summary>
	/// <typeparam name="T">Type of first argument (must have Write() overload)</typeparam>
	/// <typeparam name="Args">Types of remaining arguments</typeparam>
	/// <param name="first">First argument to write</param>
	/// <param name="args">Remaining arguments to write</param>
	/// <remarks>
	/// Convenience method for chaining writes: WriteAll("Value: ", value, ", Status: ", status).
	/// Requires Write() overload for each argument type.
	/// </remarks>
	template <typename T, typename... Args>
	void WriteAll(T first, Args... args) {
		Write(first);
		WriteAll(args...);
	}

	/// <summary>
	/// Array subscript operator for read-only character access.
	/// </summary>
	/// <param name="idx">Character index (0-based)</param>
	/// <returns>Character at specified index</returns>
	/// <remarks>
	/// NO BOUNDS CHECKING - undefined behavior if idx >= _pos.
	/// Returns by value (not reference) - cannot be used for modification.
	/// </remarks>
	const char operator[](int idx) {
		return _buffer[idx];
	}
};
