#include "pch.h"
#include <benchmark/benchmark.h>
#include <string>
#include <vector>
#include <format>
#include <deque>
#include <sstream>

// ============================================================================
// Benchmark: Serializer::UpdatePrefix — concat vs append
// ============================================================================

static void BM_UpdatePrefix_Old(benchmark::State& state) {
	std::vector<std::string> prefixes = { "Cpu", "Registers", "State", "Mapper", "Audio" };
	std::string prefix;
	for (auto _ : state) {
		prefix.clear();
		for (auto& p : prefixes) {
			if (p.size()) {
				prefix += p + ".";
			}
		}
		benchmark::DoNotOptimize(prefix);
	}
}
BENCHMARK(BM_UpdatePrefix_Old);

static void BM_UpdatePrefix_New(benchmark::State& state) {
	std::vector<std::string> prefixes = { "Cpu", "Registers", "State", "Mapper", "Audio" };
	std::string prefix;
	for (auto _ : state) {
		prefix.clear();
		for (auto& p : prefixes) {
			if (p.size()) {
				prefix.append(p);
				prefix += '.';
			}
		}
		benchmark::DoNotOptimize(prefix);
	}
}
BENCHMARK(BM_UpdatePrefix_New);

// ============================================================================
// Benchmark: CombinePath — concat vs reserve+append
// ============================================================================

static void BM_CombinePath_Old(benchmark::State& state) {
	std::string folder = "C:/Users/me/Documents/Games/SaveStates";
	std::string filename = "game_001.state";
	for (auto _ : state) {
		std::string result;
		if (folder.back() != '/') {
			result = folder + "/" + filename;
		} else {
			result = folder + filename;
		}
		benchmark::DoNotOptimize(result);
	}
}
BENCHMARK(BM_CombinePath_Old);

static void BM_CombinePath_New(benchmark::State& state) {
	std::string folder = "C:/Users/me/Documents/Games/SaveStates";
	std::string filename = "game_001.state";
	for (auto _ : state) {
		std::string result;
		bool needSlash = folder.back() != '/';
		result.reserve(folder.size() + filename.size() + (needSlash ? 1 : 0));
		result.append(folder);
		if (needSlash) result += '/';
		result.append(filename);
		benchmark::DoNotOptimize(result);
	}
}
BENCHMARK(BM_CombinePath_New);

// ============================================================================
// Benchmark: HUD string concat vs std::format (per-frame patterns)
// ============================================================================

static void BM_HudConcat_Old(benchmark::State& state) {
	std::string title = "Frame";
	uint32_t frameCount = 123456;
	uint32_t lagCount = 42;
	uint32_t fps = 60;
	for (auto _ : state) {
		std::string fc = title + ": " + std::to_string(frameCount);
		std::string lc = std::string("Lag") + ": " + std::to_string(lagCount);
		std::string fp = std::string("FPS: ") + std::to_string(fps);
		benchmark::DoNotOptimize(fc);
		benchmark::DoNotOptimize(lc);
		benchmark::DoNotOptimize(fp);
	}
}
BENCHMARK(BM_HudConcat_Old);

static void BM_HudConcat_New(benchmark::State& state) {
	std::string title = "Frame";
	uint32_t frameCount = 123456;
	uint32_t lagCount = 42;
	uint32_t fps = 60;
	for (auto _ : state) {
		std::string fc = std::format("{}: {}", title, frameCount);
		std::string lc = std::format("Lag: {}", lagCount);
		std::string fp = std::format("FPS: {}", fps);
		benchmark::DoNotOptimize(fc);
		benchmark::DoNotOptimize(lc);
		benchmark::DoNotOptimize(fp);
	}
}
BENCHMARK(BM_HudConcat_New);

// ============================================================================
// Benchmark: DrawMessage concat vs std::format
// ============================================================================

static void BM_DrawMessage_Old(benchmark::State& state) {
	std::string title = "Emulator";
	std::string message = "State saved to slot 1";
	for (auto _ : state) {
		std::string text = "[" + title + "] " + message;
		benchmark::DoNotOptimize(text);
	}
}
BENCHMARK(BM_DrawMessage_Old);

static void BM_DrawMessage_New(benchmark::State& state) {
	std::string title = "Emulator";
	std::string message = "State saved to slot 1";
	for (auto _ : state) {
		std::string text = std::format("[{}] {}", title, message);
		benchmark::DoNotOptimize(text);
	}
}
BENCHMARK(BM_DrawMessage_New);

// ============================================================================
// Benchmark: DebugStats concat vs std::format
// ============================================================================

static void BM_DebugStats_Old(benchmark::State& state) {
	uint32_t underruns = 3;
	uint32_t bufferSize = 65536;
	uint32_t rate = 48000;
	for (auto _ : state) {
		std::string s1 = "Underruns: " + std::to_string(underruns);
		std::string s2 = "Buffer Size: " + std::to_string(bufferSize / 1024) + "kb";
		std::string s3 = "Rate: " + std::to_string(rate) + "Hz";
		benchmark::DoNotOptimize(s1);
		benchmark::DoNotOptimize(s2);
		benchmark::DoNotOptimize(s3);
	}
}
BENCHMARK(BM_DebugStats_Old);

static void BM_DebugStats_New(benchmark::State& state) {
	uint32_t underruns = 3;
	uint32_t bufferSize = 65536;
	uint32_t rate = 48000;
	for (auto _ : state) {
		std::string s1 = std::format("Underruns: {}", underruns);
		std::string s2 = std::format("Buffer Size: {}kb", bufferSize / 1024);
		std::string s3 = std::format("Rate: {}Hz", rate);
		benchmark::DoNotOptimize(s1);
		benchmark::DoNotOptimize(s2);
		benchmark::DoNotOptimize(s3);
	}
}
BENCHMARK(BM_DebugStats_New);

// ============================================================================
// Benchmark: VirtualFile::operator string concat vs std::format
// ============================================================================

static void BM_VirtualFileString_Old(benchmark::State& state) {
	std::string path = "C:/Games/ROM.zip";
	std::string innerFile = "game.nes";
	int innerFileIndex = 0;
	for (auto _ : state) {
		std::string result;
		if (innerFileIndex >= 0) {
			result = path + "\x1" + innerFile + "\x1" + std::to_string(innerFileIndex);
		} else {
			result = path + "\x1" + innerFile;
		}
		benchmark::DoNotOptimize(result);
	}
}
BENCHMARK(BM_VirtualFileString_Old);

static void BM_VirtualFileString_New(benchmark::State& state) {
	std::string path = "C:/Games/ROM.zip";
	std::string innerFile = "game.nes";
	int innerFileIndex = 0;
	for (auto _ : state) {
		std::string result;
		if (innerFileIndex >= 0) {
			result = std::format("{}\x1{}\x1{}", path, innerFile, innerFileIndex);
		} else {
			result = std::format("{}\x1{}", path, innerFile);
		}
		benchmark::DoNotOptimize(result);
	}
}
BENCHMARK(BM_VirtualFileString_New);

// ============================================================================
// Benchmark: MessageManager::Log fallback concat vs std::format
// ============================================================================

static void BM_LogFallback_Old(benchmark::State& state) {
	std::string title = "Movie";
	std::string message = "Recording started for player 1";
	for (auto _ : state) {
		std::string log = "[" + title + "] " + message;
		benchmark::DoNotOptimize(log);
	}
}
BENCHMARK(BM_LogFallback_Old);

static void BM_LogFallback_New(benchmark::State& state) {
	std::string title = "Movie";
	std::string message = "Recording started for player 1";
	for (auto _ : state) {
		std::string log = std::format("[{}] {}", title, message);
		benchmark::DoNotOptimize(log);
	}
}
BENCHMARK(BM_LogFallback_New);

// ============================================================================
// Benchmark: PcmReader reserve impact
// ============================================================================

static void BM_PcmPushNoReserve(benchmark::State& state) {
	for (auto _ : state) {
		std::vector<int16_t> buf;
		for (int i = 0; i < 2048; i++) {
			buf.push_back(static_cast<int16_t>(i));
			buf.push_back(static_cast<int16_t>(-i));
		}
		benchmark::DoNotOptimize(buf);
	}
}
BENCHMARK(BM_PcmPushNoReserve);

static void BM_PcmPushWithReserve(benchmark::State& state) {
	for (auto _ : state) {
		std::vector<int16_t> buf;
		buf.reserve(2048 * 2);
		for (int i = 0; i < 2048; i++) {
			buf.push_back(static_cast<int16_t>(i));
			buf.push_back(static_cast<int16_t>(-i));
		}
		benchmark::DoNotOptimize(buf);
	}
}
BENCHMARK(BM_PcmPushWithReserve);

// ============================================================================
// Benchmark: DebugStats stringstream vs std::format
// ============================================================================

static void BM_DebugStatsStream_Old(benchmark::State& state) {
	double latency = 15.73;
	double fps = 59.9412;
	double lastFrame = 16.72;
	for (auto _ : state) {
		std::stringstream ss;
		ss << std::fixed << std::setprecision(2) << latency << " ms";
		std::string s1 = ss.str();
		ss.str(""); ss.clear();
		ss << "FPS: " << std::fixed << std::setprecision(4) << fps;
		std::string s2 = ss.str();
		ss.str(""); ss.clear();
		ss << "Last Frame: " << std::fixed << std::setprecision(2) << lastFrame << " ms";
		std::string s3 = ss.str();
		benchmark::DoNotOptimize(s1);
		benchmark::DoNotOptimize(s2);
		benchmark::DoNotOptimize(s3);
	}
}
BENCHMARK(BM_DebugStatsStream_Old);

static void BM_DebugStatsStream_New(benchmark::State& state) {
	double latency = 15.73;
	double fps = 59.9412;
	double lastFrame = 16.72;
	for (auto _ : state) {
		std::string s1 = std::format("{:.2f} ms", latency);
		std::string s2 = std::format("FPS: {:.4f}", fps);
		std::string s3 = std::format("Last Frame: {:.2f} ms", lastFrame);
		benchmark::DoNotOptimize(s1);
		benchmark::DoNotOptimize(s2);
		benchmark::DoNotOptimize(s3);
	}
}
BENCHMARK(BM_DebugStatsStream_New);
