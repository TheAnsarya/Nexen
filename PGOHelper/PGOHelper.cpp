#include <vector>
#include <string>
#include <algorithm>
#include <unordered_set>
#if __has_include(<filesystem>)
	#include <filesystem>
	namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
	#include <experimental/filesystem>
	namespace fs = std::experimental::filesystem;
#endif

using std::string;
using std::vector;

// C++20 compatible path utilities
namespace PathUtil {

inline fs::path FromUtf8(const std::string& utf8str) {
#if __cplusplus >= 202002L || _MSVC_LANG >= 202002L
	return fs::path(reinterpret_cast<const char8_t*>(utf8str.c_str()));
#else
	return fs::u8path(utf8str);
#endif
}

inline std::string ToUtf8(const fs::path& p) {
#if __cplusplus >= 202002L || _MSVC_LANG >= 202002L
	auto u8str = p.u8string();
	return std::string(reinterpret_cast<const char*>(u8str.data()), u8str.size());
#else
	return p.u8string();
#endif
}

} // namespace PathUtil

extern "C" {
	void __stdcall PgoRunTest(vector<string> testRoms, bool enableDebugger);
}

vector<string> GetFilesInFolder(string rootFolder, std::unordered_set<string> extensions)
{
	vector<string> files;
	vector<string> folders = { { rootFolder } };

	std::error_code errorCode;
	if(!fs::is_directory(PathUtil::FromUtf8(rootFolder), errorCode)) {
		return files;
	}

	for(string folder : folders) {
		for(fs::directory_iterator i(PathUtil::FromUtf8(folder)), end; i != end; i++) {
			string extension = PathUtil::ToUtf8(i->path().extension());
			std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
			if(extensions.find(extension) != extensions.end()) {
				files.push_back(PathUtil::ToUtf8(i->path()));
			}
		}
	}

	return files;
}

int main(int argc, char* argv[])
{
	string romFolder = "../PGOGames";
	if(argc >= 2) {
		romFolder = argv[1];
	}

	vector<string> testRoms = GetFilesInFolder(romFolder, { ".sfc", ".gb", ".gbc", ".gbx", ".nes", ".pce", ".cue", ".sms", ".gg", ".sg", ".gba", ".col", ".ws", ".wsc" });
	PgoRunTest(testRoms, true);
	return 0;
}

