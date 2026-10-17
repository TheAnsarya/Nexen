#pragma once

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

/// <summary>
/// C++20/23 compatible filesystem path operations.
/// Provides compatibility layer for char8_t changes in C++20.
/// </summary>
/// <remarks>
/// In C++20, char8_t type was introduced changing behavior:
/// - fs::path::u8string() returns std::u8string instead of std::string
/// - fs::u8path() deprecated in favor of fs::path(u8string) constructor
/// - u8"..." literals return char8_t[] instead of char[]
///
/// These utilities provide compatible API for C++17 and C++20/23.
/// </remarks>
namespace PathUtil {

/// <summary>
/// Create filesystem path from UTF-8 encoded string.
/// </summary>
/// <param name="utf8str">UTF-8 encoded path string</param>
/// <returns>Filesystem path object</returns>
/// <remarks>
/// C++17: Uses deprecated fs::u8path()
/// C++20+: Uses fs::path constructor with char8_t cast
/// </remarks>
inline fs::path FromUtf8(const std::string& utf8str) {
#if __cplusplus >= 202002L || _MSVC_LANG >= 202002L
	// C++20 and later: use path constructor with char8_t
	return fs::path(reinterpret_cast<const char8_t*>(utf8str.c_str()));
#else
	// C++17: use deprecated u8path
	return fs::u8path(utf8str);
#endif
}

/// <summary>
/// Convert filesystem path to UTF-8 encoded string.
/// </summary>
/// <param name="p">Filesystem path to convert</param>
/// <returns>UTF-8 encoded path string</returns>
/// <remarks>
/// C++17: Returns path::u8string() directly (already std::string)
/// C++20+: Converts std::u8string to std::string via reinterpret_cast
/// </remarks>
inline std::string ToUtf8(const fs::path& p) {
#if __cplusplus >= 202002L || _MSVC_LANG >= 202002L
	// C++20 and later: u8string() returns std::u8string, need to convert
	auto u8str = p.u8string();
	return std::string(reinterpret_cast<const char*>(u8str.data()), u8str.size());
#else
	// C++17: u8string() returns std::string directly
	return p.u8string();
#endif
}

} // namespace PathUtil
