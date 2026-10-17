#include "pch.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_set>

// =============================================================================
// Constexpr + Copy Elimination Benchmarks — Phase 16.8
// =============================================================================
// Tests the performance impact of:
// 1. Base64::Encode vector copy elimination (const ref)
// 2. Base64::Decode runtime LUT → constexpr LUT
// 3. Base64::Decode string copy elimination (string_view)
// 4. VirtualFile::CheckFileSignature vector copy elimination
// 5. FolderUtilities::GetFilesInFolder set copy elimination
// =============================================================================

// ---------------------------------------------------------------------------
// 1. Base64 Encode: vector by-value vs const-ref
// ---------------------------------------------------------------------------

static constexpr const char* _base64Chars =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

__declspec(noinline)
static std::string Base64Encode_Old(const std::vector<uint8_t> data) {
	std::string out;
	int val = 0, valb = -6;
	for (uint8_t c : data) {
		val = (val << 8) + c;
		valb += 8;
		while (valb >= 0) {
			out.push_back(_base64Chars[(val >> valb) & 0x3F]);
			valb -= 6;
		}
	}
	if (valb > -6)
		out.push_back(_base64Chars[((val << 8) >> (valb + 8)) & 0x3F]);
	while (out.size() % 4)
		out.push_back('=');
	return out;
}

__declspec(noinline)
static std::string Base64Encode_New(const std::vector<uint8_t>& data) {
	std::string out;
	int val = 0, valb = -6;
	for (uint8_t c : data) {
		val = (val << 8) + c;
		valb += 8;
		while (valb >= 0) {
			out.push_back(_base64Chars[(val >> valb) & 0x3F]);
			valb -= 6;
		}
	}
	if (valb > -6)
		out.push_back(_base64Chars[((val << 8) >> (valb + 8)) & 0x3F]);
	while (out.size() % 4)
		out.push_back('=');
	return out;
}

static void BM_Base64Encode_Old_1KB(benchmark::State& state) {
	std::vector<uint8_t> data(1024, 0x42);
	for (auto _ : state) {
		auto result = Base64Encode_Old(data);
		benchmark::DoNotOptimize(result);
	}
	state.SetBytesProcessed(state.iterations() * 1024);
}
BENCHMARK(BM_Base64Encode_Old_1KB);

static void BM_Base64Encode_New_1KB(benchmark::State& state) {
	std::vector<uint8_t> data(1024, 0x42);
	for (auto _ : state) {
		auto result = Base64Encode_New(data);
		benchmark::DoNotOptimize(result);
	}
	state.SetBytesProcessed(state.iterations() * 1024);
}
BENCHMARK(BM_Base64Encode_New_1KB);

static void BM_Base64Encode_Old_64KB(benchmark::State& state) {
	std::vector<uint8_t> data(65536, 0xAB);
	for (auto _ : state) {
		auto result = Base64Encode_Old(data);
		benchmark::DoNotOptimize(result);
	}
	state.SetBytesProcessed(state.iterations() * 65536);
}
BENCHMARK(BM_Base64Encode_Old_64KB);

static void BM_Base64Encode_New_64KB(benchmark::State& state) {
	std::vector<uint8_t> data(65536, 0xAB);
	for (auto _ : state) {
		auto result = Base64Encode_New(data);
		benchmark::DoNotOptimize(result);
	}
	state.SetBytesProcessed(state.iterations() * 65536);
}
BENCHMARK(BM_Base64Encode_New_64KB);

// ---------------------------------------------------------------------------
// 2. Base64 Decode: runtime LUT vs constexpr LUT
// ---------------------------------------------------------------------------

// Old: builds 256-entry vector every call
__declspec(noinline)
static std::vector<uint8_t> Base64Decode_Old(std::string in) {
	std::vector<uint8_t> out;
	std::vector<int> T(256, -1);
	for (int i = 0; i < 64; i++)
		T["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i;

	int val = 0, valb = -8;
	for (uint8_t c : in) {
		if (T[c] == -1) break;
		val = (val << 6) + T[c];
		valb += 6;
		if (valb >= 0) {
			out.push_back(static_cast<uint8_t>(val >> valb));
			valb -= 8;
		}
	}
	return out;
}

// Constexpr LUT built at compile time
static constexpr auto MakeBase64DecodeLut() {
	std::array<int8_t, 256> lut{};
	for (int i = 0; i < 256; i++) lut[i] = -1;
	const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	for (int i = 0; i < 64; i++) lut[static_cast<uint8_t>(chars[i])] = static_cast<int8_t>(i);
	return lut;
}
static constexpr auto _base64DecodeLut = MakeBase64DecodeLut();

// New: uses constexpr LUT + string_view param
__declspec(noinline)
static std::vector<uint8_t> Base64Decode_New(std::string_view in) {
	std::vector<uint8_t> out;
	int val = 0, valb = -8;
	for (uint8_t c : in) {
		if (_base64DecodeLut[c] == -1) break;
		val = (val << 6) + _base64DecodeLut[c];
		valb += 6;
		if (valb >= 0) {
			out.push_back(static_cast<uint8_t>(val >> valb));
			valb -= 8;
		}
	}
	return out;
}

static void BM_Base64Decode_Old_1KB(benchmark::State& state) {
	// Create a base64 string (~1365 chars for 1024 bytes)
	std::string encoded = Base64Encode_New(std::vector<uint8_t>(1024, 0x42));
	for (auto _ : state) {
		auto result = Base64Decode_Old(encoded);
		benchmark::DoNotOptimize(result);
	}
	state.SetBytesProcessed(state.iterations() * encoded.size());
}
BENCHMARK(BM_Base64Decode_Old_1KB);

static void BM_Base64Decode_New_1KB(benchmark::State& state) {
	std::string encoded = Base64Encode_New(std::vector<uint8_t>(1024, 0x42));
	for (auto _ : state) {
		auto result = Base64Decode_New(encoded);
		benchmark::DoNotOptimize(result);
	}
	state.SetBytesProcessed(state.iterations() * encoded.size());
}
BENCHMARK(BM_Base64Decode_New_1KB);

static void BM_Base64Decode_Old_64KB(benchmark::State& state) {
	std::string encoded = Base64Encode_New(std::vector<uint8_t>(65536, 0xAB));
	for (auto _ : state) {
		auto result = Base64Decode_Old(encoded);
		benchmark::DoNotOptimize(result);
	}
	state.SetBytesProcessed(state.iterations() * encoded.size());
}
BENCHMARK(BM_Base64Decode_Old_64KB);

static void BM_Base64Decode_New_64KB(benchmark::State& state) {
	std::string encoded = Base64Encode_New(std::vector<uint8_t>(65536, 0xAB));
	for (auto _ : state) {
		auto result = Base64Decode_New(encoded);
		benchmark::DoNotOptimize(result);
	}
	state.SetBytesProcessed(state.iterations() * encoded.size());
}
BENCHMARK(BM_Base64Decode_New_64KB);

// ---------------------------------------------------------------------------
// 3. Signature check: vector<string> by-value vs const-ref
// ---------------------------------------------------------------------------

__declspec(noinline)
static bool CheckSignature_Old(std::vector<std::string> signatures, const uint8_t* data, size_t len) {
	for (auto& sig : signatures) {
		if (len >= sig.size() && memcmp(data, sig.data(), sig.size()) == 0)
			return true;
	}
	return false;
}

__declspec(noinline)
static bool CheckSignature_New(const std::vector<std::string>& signatures, const uint8_t* data, size_t len) {
	for (auto& sig : signatures) {
		if (len >= sig.size() && memcmp(data, sig.data(), sig.size()) == 0)
			return true;
	}
	return false;
}

static void BM_CheckSignature_Old(benchmark::State& state) {
	std::vector<std::string> sigs = {"NES\x1A", "FDS\x1A", "UNIF", "NSF", "NSFE"};
	uint8_t data[] = {'N', 'E', 'S', 0x1A, 0x00};
	for (auto _ : state) {
		auto result = CheckSignature_Old(sigs, data, sizeof(data));
		benchmark::DoNotOptimize(result);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CheckSignature_Old);

static void BM_CheckSignature_New(benchmark::State& state) {
	std::vector<std::string> sigs = {"NES\x1A", "FDS\x1A", "UNIF", "NSF", "NSFE"};
	uint8_t data[] = {'N', 'E', 'S', 0x1A, 0x00};
	for (auto _ : state) {
		auto result = CheckSignature_New(sigs, data, sizeof(data));
		benchmark::DoNotOptimize(result);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CheckSignature_New);

// ---------------------------------------------------------------------------
// 4. Extension set: unordered_set by-value vs const-ref
// ---------------------------------------------------------------------------

__declspec(noinline)
static bool HasExtension_Old(std::unordered_set<std::string> extensions, const std::string& ext) {
	return extensions.count(ext) > 0;
}

__declspec(noinline)
static bool HasExtension_New(const std::unordered_set<std::string>& extensions, const std::string& ext) {
	return extensions.count(ext) > 0;
}

static void BM_ExtensionSet_Old(benchmark::State& state) {
	std::unordered_set<std::string> exts = {".nes", ".sfc", ".smc", ".gb", ".gbc", ".gba", ".sms", ".pce", ".ws", ".wsc", ".lnx"};
	std::string ext = ".nes";
	for (auto _ : state) {
		auto result = HasExtension_Old(exts, ext);
		benchmark::DoNotOptimize(result);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ExtensionSet_Old);

static void BM_ExtensionSet_New(benchmark::State& state) {
	std::unordered_set<std::string> exts = {".nes", ".sfc", ".smc", ".gb", ".gbc", ".gba", ".sms", ".pce", ".ws", ".wsc", ".lnx"};
	std::string ext = ".nes";
	for (auto _ : state) {
		auto result = HasExtension_New(exts, ext);
		benchmark::DoNotOptimize(result);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ExtensionSet_New);

// ---------------------------------------------------------------------------
// 5. const member array vs static constexpr (BisqwitNtscFilter pattern)
// ---------------------------------------------------------------------------

struct WithConstMember {
	const uint16_t bitmaskLut[12] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x400, 0x800};
	uint16_t lookup(int i) const { return bitmaskLut[i]; }
};

struct WithStaticConstexpr {
	static constexpr uint16_t bitmaskLut[12] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x400, 0x800};
	uint16_t lookup(int i) const { return bitmaskLut[i]; }
};

static void BM_ConstMemberLookup(benchmark::State& state) {
	WithConstMember obj;
	int idx = 0;
	for (auto _ : state) {
		auto v = obj.lookup(idx % 12);
		benchmark::DoNotOptimize(v);
		idx++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ConstMemberLookup);

static void BM_StaticConstexprLookup(benchmark::State& state) {
	WithStaticConstexpr obj;
	int idx = 0;
	for (auto _ : state) {
		auto v = obj.lookup(idx % 12);
		benchmark::DoNotOptimize(v);
		idx++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_StaticConstexprLookup);
