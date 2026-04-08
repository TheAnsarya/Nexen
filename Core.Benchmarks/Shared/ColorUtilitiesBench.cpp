#include "pch.h"
#include "Shared/ColorUtilities.h"

// Benchmark single color conversion
static void BM_ColorUtilities_Rgb555ToArgb_Single(benchmark::State& state) {
	uint16_t color = 0x7FFF;
	for (auto _ : state) {
		benchmark::DoNotOptimize(ColorUtilities::Rgb555ToArgb(color));
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ColorUtilities_Rgb555ToArgb_Single);

// Benchmark loop of conversions (typical PPU rendering scenario)
static void BM_ColorUtilities_Rgb555ToArgb_Loop(benchmark::State& state) {
	for (auto _ : state) {
		for (uint16_t i = 0; i < 0x8000; i++) {
			benchmark::DoNotOptimize(ColorUtilities::Rgb555ToArgb(i));
		}
	}
	state.SetItemsProcessed(state.iterations() * 0x8000);
}
BENCHMARK(BM_ColorUtilities_Rgb555ToArgb_Loop);

// Benchmark constexpr version (should compile to constant in release)
static void BM_ColorUtilities_Rgb555ToArgb_Constexpr(benchmark::State& state) {
	for (auto _ : state) {
		constexpr uint32_t black = ColorUtilities::Rgb555ToArgb(0x0000);
		constexpr uint32_t white = ColorUtilities::Rgb555ToArgb(0x7FFF);
		constexpr uint32_t red = ColorUtilities::Rgb555ToArgb(0x001F);
		auto b = black; benchmark::DoNotOptimize(b);
		auto w = white; benchmark::DoNotOptimize(w);
		auto r = red; benchmark::DoNotOptimize(r);
	}
}
BENCHMARK(BM_ColorUtilities_Rgb555ToArgb_Constexpr);

// Benchmark RGB555 to RGB conversion
static void BM_ColorUtilities_Rgb555ToRgb(benchmark::State& state) {
	uint8_t r, g, b;
	uint16_t color = 0x7FFF;
	for (auto _ : state) {
		ColorUtilities::Rgb555ToRgb(color, r, g, b);
		benchmark::DoNotOptimize(r);
		benchmark::DoNotOptimize(g);
		benchmark::DoNotOptimize(b);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ColorUtilities_Rgb555ToRgb);

// Benchmark with varying color values
static void BM_ColorUtilities_Rgb555ToArgb_Varying(benchmark::State& state) {
	std::vector<uint16_t> colors;
	for (uint16_t i = 0; i < 256; i++) {
		colors.push_back(static_cast<uint16_t>(i * 128));
	}
	
	size_t index = 0;
	for (auto _ : state) {
		benchmark::DoNotOptimize(ColorUtilities::Rgb555ToArgb(colors[index % colors.size()]));
		index++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ColorUtilities_Rgb555ToArgb_Varying);
