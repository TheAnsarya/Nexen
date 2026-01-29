#pragma once

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

/**
 * @brief Utility functions for C++20/23 compatible filesystem path operations.
 *
 * In C++20, the char8_t type was introduced which changes the behavior of:
 * - fs::path::u8string() now returns std::u8string instead of std::string
 * - fs::u8path() is deprecated
 * - u8"..." literals now return char8_t[] instead of char[]
 *
 * These utilities provide a compatibility layer that works with both C++17 and C++20/23.
 */
namespace PathUtil {

/**
 * @brief Create a filesystem path from a UTF-8 encoded string.
 *
 * In C++17, this uses fs::u8path().
 * In C++20+, this uses the fs::path(u8string) constructor.
 *
 * @param utf8str UTF-8 encoded path string
 * @return fs::path The filesystem path
 */
inline fs::path FromUtf8(const std::string& utf8str) {
#if __cplusplus >= 202002L || _MSVC_LANG >= 202002L
	// C++20 and later: use path constructor with char8_t
	return fs::path(reinterpret_cast<const char8_t*>(utf8str.c_str()));
#else
	// C++17: use deprecated u8path
	return fs::u8path(utf8str);
#endif
}

/**
 * @brief Convert a filesystem path to a UTF-8 encoded string.
 *
 * In C++17, this returns path::u8string() directly.
 * In C++20+, this converts the u8string to a regular string.
 *
 * @param p The filesystem path
 * @return std::string UTF-8 encoded path string
 */
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
