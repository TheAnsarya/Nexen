#include "pch.h"
// MoveSemanticsBench.cpp
// Benchmarks proving move semantics gains for RenderedFrame, DrawStringCommand,
// and HUD string operations.
// Related issues: #444 (RenderedFrame move), #447 (HUD string copy elimination)

#include "benchmark/benchmark.h"
#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <cstring>

// =============================================================================
// RenderedFrame Move Semantics Benchmarks
// =============================================================================
// RenderedFrame contains a vector<ControllerData> (InputData) which benefits
// from move instead of copy. In the video pipeline, frames are passed by value
// and moved into the decoder.

/// <summary>Simulates ControllerData with realistic size</summary>
struct MockControllerData {
	uint32_t buttons = 0;
	int16_t analogX = 0;
	int16_t analogY = 0;
	uint8_t port = 0;
	uint8_t padding[3] = {};
};

/// <summary>Simulates RenderedFrame with InputData vector</summary>
struct MockRenderedFrame {
	uint32_t* frameBuffer = nullptr;          // Pointer to shared buffer (not owned)
	uint8_t* osdBuffer = nullptr;             // OSD overlay
	uint32_t width = 256;
	uint32_t height = 240;
	double scale = 1.0;
	uint32_t frameNumber = 0;
	std::vector<MockControllerData> inputData;

	MockRenderedFrame() = default;
	MockRenderedFrame(const MockRenderedFrame&) = default;
	MockRenderedFrame(MockRenderedFrame&&) = default;
	MockRenderedFrame& operator=(const MockRenderedFrame&) = default;
	MockRenderedFrame& operator=(MockRenderedFrame&&) = default;

	// Constructor taking input data by value (enables move from caller)
	MockRenderedFrame(uint32_t* fb, uint32_t w, uint32_t h, uint32_t frame,
		std::vector<MockControllerData> input)
		: frameBuffer(fb), width(w), height(h), frameNumber(frame),
		  inputData(std::move(input)) {
	}
};

// Benchmark: Copy a RenderedFrame (old pattern — copies InputData vector)
static void BM_RenderedFrame_Copy(benchmark::State& state) {
	uint32_t dummyBuffer[256] = {};

	// Create source frame with realistic input data (4 controllers × 8 frames of history)
	std::vector<MockControllerData> inputData(32);
	for (size_t i = 0; i < inputData.size(); i++) {
		inputData[i].buttons = static_cast<uint32_t>(i);
		inputData[i].port = static_cast<uint8_t>(i % 4);
	}

	MockRenderedFrame src(dummyBuffer, 256, 240, 100, inputData);

	for (auto _ : state) {
		// COPY: allocates new vector, copies all controller data
		MockRenderedFrame dst = src;
		benchmark::DoNotOptimize(dst.inputData.data());
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_RenderedFrame_Copy);

// Benchmark: Move a RenderedFrame (new pattern — steals InputData vector)
static void BM_RenderedFrame_Move(benchmark::State& state) {
	uint32_t dummyBuffer[256] = {};

	for (auto _ : state) {
		// Create fresh source each iteration (simulates frame production)
		std::vector<MockControllerData> inputData(32);
		for (size_t i = 0; i < inputData.size(); i++) {
			inputData[i].buttons = static_cast<uint32_t>(i);
		}

		MockRenderedFrame src(dummyBuffer, 256, 240, 100, std::move(inputData));

		// MOVE: transfers vector ownership — no allocation, no copy
		MockRenderedFrame dst = std::move(src);
		benchmark::DoNotOptimize(dst.inputData.data());
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_RenderedFrame_Move);

// =============================================================================
// String Copy Elimination Benchmarks
// =============================================================================
// DrawStringCommand and HUD messages now take strings by value + std::move
// instead of const ref + copy.

// Benchmark: old pattern — string copy into member (const ref param)
static void BM_String_CopyIntoMember(benchmark::State& state) {
	for (auto _ : state) {
		std::string source = "Player 1 Score: 1,234,567";
		// Old pattern: const string& → copy to member
		std::string member = source;
		benchmark::DoNotOptimize(member.data());
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_String_CopyIntoMember);

// Benchmark: new pattern — string move into member (value param + move)
static void BM_String_MoveIntoMember(benchmark::State& state) {
	for (auto _ : state) {
		std::string source = "Player 1 Score: 1,234,567";
		// New pattern: string param by value → std::move to member
		std::string member = std::move(source);
		benchmark::DoNotOptimize(member.data());
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_String_MoveIntoMember);

// Benchmark: longer strings (HUD messages with formatting)
static void BM_String_CopyLong(benchmark::State& state) {
	std::string longText = "Frame: 123456 | Lag: 0 | Input: A+B+Start | RerecordCount: 9,876 | Mode: TAS Recording";

	for (auto _ : state) {
		std::string copy = longText;
		benchmark::DoNotOptimize(copy.data());
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_String_CopyLong);

static void BM_String_MoveLong(benchmark::State& state) {
	for (auto _ : state) {
		std::string longText = "Frame: 123456 | Lag: 0 | Input: A+B+Start | RerecordCount: 9,876 | Mode: TAS Recording";
		std::string moved = std::move(longText);
		benchmark::DoNotOptimize(moved.data());
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_String_MoveLong);

// Benchmark: DrawStringCommand-style construction (text + coordinates)
struct MockDrawCommand {
	std::string text;
	int x, y;
	uint32_t color;

	// Old: takes const string& (forces copy)
	MockDrawCommand(const std::string& t, int px, int py, uint32_t c)
		: text(t), x(px), y(py), color(c) {
	}
};

struct MockDrawCommandMove {
	std::string text;
	int x, y;
	uint32_t color;

	// New: takes string by value (enables move from rvalue)
	MockDrawCommandMove(std::string t, int px, int py, uint32_t c)
		: text(std::move(t)), x(px), y(py), color(c) {
	}
};

static void BM_DrawCommand_CopyConstruct(benchmark::State& state) {
	std::string text = "Frame: 123456 / 999999";

	for (auto _ : state) {
		MockDrawCommand cmd(text, 10, 20, 0xffffffff);
		benchmark::DoNotOptimize(cmd.text.data());
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_DrawCommand_CopyConstruct);

static void BM_DrawCommand_MoveConstruct(benchmark::State& state) {
	for (auto _ : state) {
		std::string text = "Frame: 123456 / 999999";
		MockDrawCommandMove cmd(std::move(text), 10, 20, 0xffffffff);
		benchmark::DoNotOptimize(cmd.text.data());
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_DrawCommand_MoveConstruct);

// =============================================================================
// Vector Reserve vs No-Reserve Benchmarks
// =============================================================================
// HermiteResampler reserve optimization — avoids reallocation during audio processing

static void BM_Vector_PushbackNoReserve(benchmark::State& state) {
	constexpr size_t sampleCount = 4096;

	for (auto _ : state) {
		std::vector<int16_t> buffer;
		// No reserve — multiple reallocations during push_back
		for (size_t i = 0; i < sampleCount; i++) {
			buffer.push_back(static_cast<int16_t>(i));
		}
		benchmark::DoNotOptimize(buffer.data());
	}
	state.SetItemsProcessed(state.iterations() * sampleCount);
}
BENCHMARK(BM_Vector_PushbackNoReserve);

static void BM_Vector_PushbackWithReserve(benchmark::State& state) {
	constexpr size_t sampleCount = 4096;

	for (auto _ : state) {
		std::vector<int16_t> buffer;
		buffer.reserve(sampleCount);
		// Reserved — single allocation, no reallocation
		for (size_t i = 0; i < sampleCount; i++) {
			buffer.push_back(static_cast<int16_t>(i));
		}
		benchmark::DoNotOptimize(buffer.data());
	}
	state.SetItemsProcessed(state.iterations() * sampleCount);
}
BENCHMARK(BM_Vector_PushbackWithReserve);

// Benchmark with persistent buffer (run-ahead SaveVideoData pattern)
static void BM_Vector_PersistentBuffer(benchmark::State& state) {
	constexpr size_t bufferSize = 256 * 240 * 4;  // ~245KB (framebuffer)
	std::vector<uint8_t> persistentBuffer;

	for (auto _ : state) {
		// Persistent: clear + resize (retains allocation after first iteration)
		persistentBuffer.clear();
		persistentBuffer.resize(bufferSize);
		std::memset(persistentBuffer.data(), 0x42, bufferSize);
		benchmark::DoNotOptimize(persistentBuffer.data());
	}
	state.SetBytesProcessed(state.iterations() * bufferSize);
}
BENCHMARK(BM_Vector_PersistentBuffer);

static void BM_Vector_FreshAllocation(benchmark::State& state) {
	constexpr size_t bufferSize = 256 * 240 * 4;  // ~245KB (framebuffer)

	for (auto _ : state) {
		// Fresh: new allocation every time (old SaveVideoData pattern)
		std::vector<uint8_t> freshBuffer(bufferSize);
		std::memset(freshBuffer.data(), 0x42, bufferSize);
		benchmark::DoNotOptimize(freshBuffer.data());
		benchmark::ClobberMemory();
	}
	state.SetBytesProcessed(state.iterations() * bufferSize);
}
BENCHMARK(BM_Vector_FreshAllocation);
