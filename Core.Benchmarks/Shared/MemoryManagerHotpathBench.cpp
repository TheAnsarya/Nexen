#include "pch.h"
#include <array>
#include "Shared/Emulator.h"
#include "GBA/GbaMemoryManager.h"
#include "GBA/GbaDmaController.h"
#include "NES/BaseMapper.h"

namespace {

class BenchmarkMapper final : public BaseMapper {
private:
	std::array<uint8_t, 0x2000> _chrData = {};

protected:
	void InitMapper() override {
	}

	uint16_t GetPrgPageSize() override {
		return 0x2000;
	}

	uint16_t GetChrPageSize() override {
		return 0x0400;
	}

public:
	void Setup(Emulator* emu) {
		for (uint32_t i = 0; i < _chrData.size(); i++) {
			_chrData[i] = static_cast<uint8_t>((i * 13 + 0x5A) & 0xFF);
		}

		_emu = emu;

		SetPpuMemoryMapping(0x0000, 0x1FFF, _chrData.data(), 0, static_cast<uint32_t>(_chrData.size()), MemoryAccessType::Read);
	}
};
struct GbaInternalCycleBranchModelState {
	bool HasPendingUpdates = false;
	bool HasPendingLateUpdates = false;
	uint8_t IrqLine = 0;
	uint8_t IrqFirstAccessCycle = 0;
	uint64_t MasterClock = 0;
};

// Branch model of GbaMemoryManager::ProcessInternalCycle for distribution analysis.
template <bool firstAccessCycle>
__forceinline void ProcessInternalCycleBranchModel(GbaInternalCycleBranchModelState& state) {
	if (state.HasPendingUpdates) {
		benchmark::DoNotOptimize(state.HasPendingUpdates);
	} else {
		state.MasterClock++;
	}

	if constexpr (firstAccessCycle) {
		state.IrqFirstAccessCycle = state.IrqLine;
	}

	if (state.HasPendingLateUpdates) {
		benchmark::DoNotOptimize(state.HasPendingLateUpdates);
	}
}

// Real hotpath from GBA memory manager: ProcessDma() calls HasPendingDma() every cycle.
static void BM_GbaMemMgr_ProcessDma_NoPending(benchmark::State& state) {
	Emulator emu;
	GbaDmaController dma;
	GbaMemoryManager memManager(&emu, nullptr, nullptr, &dma, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

	for (auto _ : state) {
		memManager.ProcessDma();
		benchmark::DoNotOptimize(memManager.GetMasterClock());
	}

	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaMemMgr_ProcessDma_NoPending);

// ProcessInternalCycle with 0% pending updates (steady-state fast path).
static void BM_GbaMemMgr_ProcessInternalCycle_Pending0Pct(benchmark::State& state) {
	GbaInternalCycleBranchModelState model;
	uint32_t iteration = 1;

	for (auto _ : state) {
		benchmark::DoNotOptimize(iteration++);
		model.HasPendingUpdates = false;
		ProcessInternalCycleBranchModel<false>(model);
		benchmark::DoNotOptimize(model.MasterClock);
	}

	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaMemMgr_ProcessInternalCycle_Pending0Pct);

// ProcessInternalCycle with ~5% pending updates (1 in 20 cycles).
static void BM_GbaMemMgr_ProcessInternalCycle_Pending5Pct(benchmark::State& state) {
	GbaInternalCycleBranchModelState model;
	uint32_t iteration = 1;

	for (auto _ : state) {
		model.HasPendingUpdates = (iteration++ % 20) == 0;
		ProcessInternalCycleBranchModel<false>(model);
		benchmark::DoNotOptimize(model.MasterClock);
	}

	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaMemMgr_ProcessInternalCycle_Pending5Pct);

// ProcessInternalCycle with 50% pending updates (alternating cycles).
static void BM_GbaMemMgr_ProcessInternalCycle_Pending50Pct(benchmark::State& state) {
	GbaInternalCycleBranchModelState model;
	uint32_t iteration = 1;

	for (auto _ : state) {
		model.HasPendingUpdates = (iteration++ & 0x01) == 0;
		ProcessInternalCycleBranchModel<false>(model);
		benchmark::DoNotOptimize(model.MasterClock);
	}

	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaMemMgr_ProcessInternalCycle_Pending50Pct);

// Real hotpath from BaseMapper: branch on _hasCustomReadVram in ReadVram().
static void BM_NesMapper_ReadVram_CommonPath(benchmark::State& state) {
	Emulator emu;
	BenchmarkMapper mapper;
	mapper.Setup(&emu);

	uint16_t addr = 0;
	for (auto _ : state) {
		uint8_t value = mapper.ReadVram(addr, MemoryOperationType::PpuRenderingRead);
		benchmark::DoNotOptimize(value);
		addr = (addr + 17) & 0x1FFF;
	}

	state.SetBytesProcessed(state.iterations());
}
BENCHMARK(BM_NesMapper_ReadVram_CommonPath);

} // namespace
