#include "pch.h"
#include <array>
#include <memory>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <random>
#include <cstring>
#include "Debugger/DebugTypes.h"
#include "Debugger/CodeDataLogger.h"
#include "Shared/MemoryType.h"

// =============================================================================
// Debugger Pipeline Benchmarks
// =============================================================================
// Measures individual components of the debugger hot path to identify
// bottlenecks and validate optimizations.
//
// The full debugger pipeline adds 160-350ns per instruction (no trace/BPs).
// These benchmarks measure each component in isolation.
//
// Reference: ~docs/plans/debugger-performance-optimization.md
// =============================================================================

// --- Local benchmark-only structs -------------------------------------------
// These mirror the real structures but avoid pulling in heavy debugger deps.

/// Mirrors ProfiledFunction from Profiler.h (for isolated benchmarks)
struct BenchProfiledFunction {
	uint64_t ExclusiveCycles = 0;
	uint64_t InclusiveCycles = 0;
	uint64_t CallCount = 0;
	uint64_t MinCycles = UINT64_MAX;
	uint64_t MaxCycles = 0;
	AddressInfo Address = {};
	StackFrameFlags Flags = {};
};

// --- Helpers ----------------------------------------------------------------

// Simulates a flat CDL data array (same as CodeDataLogger._cdlData)
static constexpr uint32_t kRomSize = 512 * 1024; // 512KB (large SNES ROM)
static constexpr uint32_t kSmallRomSize = 32 * 1024; // 32KB (NES PRG ROM)

// Pre-generate random addresses for realistic access patterns
static std::vector<uint32_t> GenerateRandomAddresses(uint32_t count, uint32_t maxAddr) {
	std::mt19937 rng(42); // Fixed seed for reproducibility
	std::uniform_int_distribution<uint32_t> dist(0, maxAddr - 1);
	std::vector<uint32_t> addrs(count);
	for (auto& a : addrs) a = dist(rng);
	return addrs;
}

// Pre-generate sequential addresses (realistic - CPU executes sequentially)
static std::vector<uint32_t> GenerateSequentialAddresses(uint32_t count, uint32_t startAddr) {
	std::vector<uint32_t> addrs(count);
	for (uint32_t i = 0; i < count; i++) {
		addrs[i] = (startAddr + i) % kSmallRomSize;
	}
	return addrs;
}

// =============================================================================
// 1. CDL Benchmarks — Baseline (the minimum possible overhead)
// =============================================================================

// CDL SetCode: single byte OR operation — this is the target floor
static void BM_CDL_SetCode_SingleByte(benchmark::State& state) {
	auto cdlData = std::make_unique<uint8_t[]>(kSmallRomSize);
	std::memset(cdlData.get(), 0, kSmallRomSize);
	auto addrs = GenerateSequentialAddresses(10000, 0x8000 % kSmallRomSize);
	uint32_t idx = 0;

	for (auto _ : state) {
		uint32_t addr = addrs[idx++ % addrs.size()];
		cdlData[addr] |= CdlFlags::Code;
		benchmark::DoNotOptimize(cdlData[addr]);
	}
	state.SetItemsProcessed(state.iterations());
	state.SetLabel("floor: 1 byte OR");
}
BENCHMARK(BM_CDL_SetCode_SingleByte);

// CDL SetData: single byte OR with Data flag
static void BM_CDL_SetData_SingleByte(benchmark::State& state) {
	auto cdlData = std::make_unique<uint8_t[]>(kSmallRomSize);
	std::memset(cdlData.get(), 0, kSmallRomSize);
	auto addrs = GenerateRandomAddresses(10000, kSmallRomSize);
	uint32_t idx = 0;

	for (auto _ : state) {
		uint32_t addr = addrs[idx++ % addrs.size()];
		cdlData[addr] |= CdlFlags::Data;
		benchmark::DoNotOptimize(cdlData[addr]);
	}
	state.SetItemsProcessed(state.iterations());
	state.SetLabel("floor: 1 byte OR (random)");
}
BENCHMARK(BM_CDL_SetData_SingleByte);

// CDL SetCode: 3 bytes (typical NES instruction: opcode + 2 operands)
static void BM_CDL_SetCode_ThreeBytes(benchmark::State& state) {
	auto cdlData = std::make_unique<uint8_t[]>(kSmallRomSize);
	std::memset(cdlData.get(), 0, kSmallRomSize);
	auto addrs = GenerateSequentialAddresses(10000, 0);
	uint32_t idx = 0;

	for (auto _ : state) {
		uint32_t addr = addrs[idx++ % addrs.size()];
		cdlData[addr] |= CdlFlags::Code | CdlFlags::SubEntryPoint;
		cdlData[addr + 1] |= CdlFlags::Code;
		cdlData[addr + 2] |= CdlFlags::Code;
		benchmark::DoNotOptimize(cdlData[addr]);
	}
	state.SetItemsProcessed(state.iterations() * 3);
	state.SetLabel("3-byte instruction");
}
BENCHMARK(BM_CDL_SetCode_ThreeBytes);

// =============================================================================
// 2. AddressCounters Benchmarks — Current AoS Layout
// =============================================================================

// Current AoS: 36 bytes per address (interleaved hot/cold data)
struct AddressCountersAoS {
	uint64_t ReadStamp;
	uint64_t WriteStamp;
	uint64_t ExecStamp;
	uint32_t ReadCounter;
	uint32_t WriteCounter;
	uint32_t ExecCounter;
};
static_assert(sizeof(AddressCountersAoS) == 36 || sizeof(AddressCountersAoS) == 40,
	"AddressCounters should be 36 or 40 bytes");

// Proposed SoA: separate hot counters from cold stamps
struct AddressCountersSoA {
	std::unique_ptr<uint32_t[]> readCounters;
	std::unique_ptr<uint32_t[]> writeCounters;
	std::unique_ptr<uint32_t[]> execCounters;
	std::unique_ptr<uint64_t[]> readStamps;
	std::unique_ptr<uint64_t[]> writeStamps;
	std::unique_ptr<uint64_t[]> execStamps;
	uint32_t size;

	AddressCountersSoA(uint32_t sz) : size(sz) {
		readCounters = std::make_unique<uint32_t[]>(sz);
		writeCounters = std::make_unique<uint32_t[]>(sz);
		execCounters = std::make_unique<uint32_t[]>(sz);
		readStamps = std::make_unique<uint64_t[]>(sz);
		writeStamps = std::make_unique<uint64_t[]>(sz);
		execStamps = std::make_unique<uint64_t[]>(sz);
		std::memset(readCounters.get(), 0, sz * sizeof(uint32_t));
		std::memset(writeCounters.get(), 0, sz * sizeof(uint32_t));
		std::memset(execCounters.get(), 0, sz * sizeof(uint32_t));
		std::memset(readStamps.get(), 0, sz * sizeof(uint64_t));
		std::memset(writeStamps.get(), 0, sz * sizeof(uint64_t));
		std::memset(execStamps.get(), 0, sz * sizeof(uint64_t));
	}
};

// AoS: ProcessMemoryRead (current layout)
static void BM_MemAccessCounter_AoS_Read(benchmark::State& state) {
	const uint32_t memSize = static_cast<uint32_t>(state.range(0));
	std::vector<AddressCountersAoS> counters(memSize);
	std::memset(counters.data(), 0, memSize * sizeof(AddressCountersAoS));
	auto addrs = GenerateRandomAddresses(10000, memSize);
	uint32_t idx = 0;
	uint64_t clock = 1000;

	for (auto _ : state) {
		uint32_t addr = addrs[idx++ % addrs.size()];
		AddressCountersAoS& c = counters[addr];
		c.ReadStamp = clock++;
		c.ReadCounter++;
		benchmark::DoNotOptimize(c.ReadCounter);
	}
	state.SetItemsProcessed(state.iterations());
	state.SetBytesProcessed(state.iterations() * 12); // Touch 12 bytes per access
}
BENCHMARK(BM_MemAccessCounter_AoS_Read)->Arg(kSmallRomSize)->Arg(kRomSize);

// SoA: ProcessMemoryRead (proposed layout)
static void BM_MemAccessCounter_SoA_Read(benchmark::State& state) {
	const uint32_t memSize = static_cast<uint32_t>(state.range(0));
	AddressCountersSoA counters(memSize);
	auto addrs = GenerateRandomAddresses(10000, memSize);
	uint32_t idx = 0;
	uint64_t clock = 1000;

	for (auto _ : state) {
		uint32_t addr = addrs[idx++ % addrs.size()];
		counters.readStamps[addr] = clock++;
		counters.readCounters[addr]++;
		benchmark::DoNotOptimize(counters.readCounters[addr]);
	}
	state.SetItemsProcessed(state.iterations());
	state.SetBytesProcessed(state.iterations() * 12);
}
BENCHMARK(BM_MemAccessCounter_SoA_Read)->Arg(kSmallRomSize)->Arg(kRomSize);

// AoS: ProcessMemoryWrite (current layout)
static void BM_MemAccessCounter_AoS_Write(benchmark::State& state) {
	const uint32_t memSize = static_cast<uint32_t>(state.range(0));
	std::vector<AddressCountersAoS> counters(memSize);
	std::memset(counters.data(), 0, memSize * sizeof(AddressCountersAoS));
	auto addrs = GenerateRandomAddresses(10000, memSize);
	uint32_t idx = 0;
	uint64_t clock = 1000;

	for (auto _ : state) {
		uint32_t addr = addrs[idx++ % addrs.size()];
		AddressCountersAoS& c = counters[addr];
		c.WriteStamp = clock++;
		c.WriteCounter++;
		benchmark::DoNotOptimize(c.WriteCounter);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MemAccessCounter_AoS_Write)->Arg(kSmallRomSize)->Arg(kRomSize);

// SoA: ProcessMemoryWrite (proposed layout)
static void BM_MemAccessCounter_SoA_Write(benchmark::State& state) {
	const uint32_t memSize = static_cast<uint32_t>(state.range(0));
	AddressCountersSoA counters(memSize);
	auto addrs = GenerateRandomAddresses(10000, memSize);
	uint32_t idx = 0;
	uint64_t clock = 1000;

	for (auto _ : state) {
		uint32_t addr = addrs[idx++ % addrs.size()];
		counters.writeStamps[addr] = clock++;
		counters.writeCounters[addr]++;
		benchmark::DoNotOptimize(counters.writeCounters[addr]);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MemAccessCounter_SoA_Write)->Arg(kSmallRomSize)->Arg(kRomSize);

// AoS: ProcessMemoryExec (current layout)
static void BM_MemAccessCounter_AoS_Exec(benchmark::State& state) {
	const uint32_t memSize = static_cast<uint32_t>(state.range(0));
	std::vector<AddressCountersAoS> counters(memSize);
	std::memset(counters.data(), 0, memSize * sizeof(AddressCountersAoS));
	auto addrs = GenerateSequentialAddresses(10000, 0);
	uint32_t idx = 0;
	uint64_t clock = 1000;

	for (auto _ : state) {
		uint32_t addr = addrs[idx++ % addrs.size()];
		AddressCountersAoS& c = counters[addr];
		c.ExecStamp = clock++;
		c.ExecCounter++;
		benchmark::DoNotOptimize(c.ExecCounter);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MemAccessCounter_AoS_Exec)->Arg(kSmallRomSize)->Arg(kRomSize);

// SoA: ProcessMemoryExec (proposed layout)
static void BM_MemAccessCounter_SoA_Exec(benchmark::State& state) {
	const uint32_t memSize = static_cast<uint32_t>(state.range(0));
	AddressCountersSoA counters(memSize);
	auto addrs = GenerateSequentialAddresses(10000, 0);
	uint32_t idx = 0;
	uint64_t clock = 1000;

	for (auto _ : state) {
		uint32_t addr = addrs[idx++ % addrs.size()];
		counters.execStamps[addr] = clock++;
		counters.execCounters[addr]++;
		benchmark::DoNotOptimize(counters.execCounters[addr]);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MemAccessCounter_SoA_Exec)->Arg(kSmallRomSize)->Arg(kRomSize);

// AoS: Full NES instruction pattern (1 exec + 2 operand reads + 2 data reads + 1 write)
static void BM_MemAccessCounter_AoS_FullInstruction(benchmark::State& state) {
	std::vector<AddressCountersAoS> counters(kSmallRomSize);
	std::memset(counters.data(), 0, kSmallRomSize * sizeof(AddressCountersAoS));
	auto addrs = GenerateSequentialAddresses(10000, 0);
	auto dataAddrs = GenerateRandomAddresses(10000, kSmallRomSize);
	uint32_t pcIdx = 0;
	uint32_t dataIdx = 0;
	uint64_t clock = 1000;

	for (auto _ : state) {
		uint32_t pc = addrs[pcIdx++ % addrs.size()];
		uint32_t dataAddr = dataAddrs[dataIdx++ % dataAddrs.size()];

		// 1 ExecOpCode
		counters[pc].ExecStamp = clock;
		counters[pc].ExecCounter++;
		// 2 ExecOperand (operand bytes)
		counters[pc + 1].ExecStamp = clock;
		counters[pc + 1].ExecCounter++;
		counters[pc + 2].ExecStamp = clock;
		counters[pc + 2].ExecCounter++;
		// 2 Data reads
		counters[dataAddr % kSmallRomSize].ReadStamp = clock;
		counters[dataAddr % kSmallRomSize].ReadCounter++;
		counters[(dataAddr + 1) % kSmallRomSize].ReadStamp = clock;
		counters[(dataAddr + 1) % kSmallRomSize].ReadCounter++;
		// 1 Write
		counters[(dataAddr + 2) % kSmallRomSize].WriteStamp = clock;
		counters[(dataAddr + 2) % kSmallRomSize].WriteCounter++;

		clock++;
		benchmark::DoNotOptimize(counters[pc].ExecCounter);
	}
	state.SetItemsProcessed(state.iterations());
	state.SetLabel("1 exec + 2 operand + 2 read + 1 write");
}
BENCHMARK(BM_MemAccessCounter_AoS_FullInstruction);

// SoA: Full NES instruction pattern
static void BM_MemAccessCounter_SoA_FullInstruction(benchmark::State& state) {
	AddressCountersSoA counters(kSmallRomSize);
	auto addrs = GenerateSequentialAddresses(10000, 0);
	auto dataAddrs = GenerateRandomAddresses(10000, kSmallRomSize);
	uint32_t pcIdx = 0;
	uint32_t dataIdx = 0;
	uint64_t clock = 1000;

	for (auto _ : state) {
		uint32_t pc = addrs[pcIdx++ % addrs.size()];
		uint32_t dataAddr = dataAddrs[dataIdx++ % dataAddrs.size()];

		// 1 ExecOpCode
		counters.execStamps[pc] = clock;
		counters.execCounters[pc]++;
		// 2 ExecOperand
		counters.execStamps[pc + 1] = clock;
		counters.execCounters[pc + 1]++;
		counters.execStamps[pc + 2] = clock;
		counters.execCounters[pc + 2]++;
		// 2 Data reads
		counters.readStamps[dataAddr % kSmallRomSize] = clock;
		counters.readCounters[dataAddr % kSmallRomSize]++;
		counters.readStamps[(dataAddr + 1) % kSmallRomSize] = clock;
		counters.readCounters[(dataAddr + 1) % kSmallRomSize]++;
		// 1 Write
		counters.writeStamps[(dataAddr + 2) % kSmallRomSize] = clock;
		counters.writeCounters[(dataAddr + 2) % kSmallRomSize]++;

		clock++;
		benchmark::DoNotOptimize(counters.execCounters[pc]);
	}
	state.SetItemsProcessed(state.iterations());
	state.SetLabel("1 exec + 2 operand + 2 read + 1 write");
}
BENCHMARK(BM_MemAccessCounter_SoA_FullInstruction);

// Counters-only (skip timestamps entirely — for when heatmap not needed)
static void BM_MemAccessCounter_CountersOnly_FullInstruction(benchmark::State& state) {
	auto readCounters = std::make_unique<uint32_t[]>(kSmallRomSize);
	auto writeCounters = std::make_unique<uint32_t[]>(kSmallRomSize);
	auto execCounters = std::make_unique<uint32_t[]>(kSmallRomSize);
	std::memset(readCounters.get(), 0, kSmallRomSize * sizeof(uint32_t));
	std::memset(writeCounters.get(), 0, kSmallRomSize * sizeof(uint32_t));
	std::memset(execCounters.get(), 0, kSmallRomSize * sizeof(uint32_t));
	auto addrs = GenerateSequentialAddresses(10000, 0);
	auto dataAddrs = GenerateRandomAddresses(10000, kSmallRomSize);
	uint32_t pcIdx = 0;
	uint32_t dataIdx = 0;

	for (auto _ : state) {
		uint32_t pc = addrs[pcIdx++ % addrs.size()];
		uint32_t dataAddr = dataAddrs[dataIdx++ % dataAddrs.size()];

		execCounters[pc]++;
		execCounters[pc + 1]++;
		execCounters[pc + 2]++;
		readCounters[dataAddr % kSmallRomSize]++;
		readCounters[(dataAddr + 1) % kSmallRomSize]++;
		writeCounters[(dataAddr + 2) % kSmallRomSize]++;

		benchmark::DoNotOptimize(execCounters[pc]);
	}
	state.SetItemsProcessed(state.iterations());
	state.SetLabel("counters only, no timestamps");
}
BENCHMARK(BM_MemAccessCounter_CountersOnly_FullInstruction);

// =============================================================================
// 3. Profiler Benchmarks — unordered_map vs flat array
// =============================================================================

// Current: unordered_map<int32_t, BenchProfiledFunction> lookup
static void BM_Profiler_HashMap_Lookup(benchmark::State& state) {
	const uint32_t funcCount = static_cast<uint32_t>(state.range(0));
	std::unordered_map<int32_t, BenchProfiledFunction> functions;

	// Pre-populate with functions
	for (uint32_t i = 0; i < funcCount; i++) {
		functions[i] = BenchProfiledFunction{};
		functions[i].Address = { static_cast<int32_t>(i), MemoryType::NesPrgRom };
	}

	auto addrs = GenerateRandomAddresses(10000, funcCount);
	uint32_t idx = 0;

	for (auto _ : state) {
		int32_t key = addrs[idx++ % addrs.size()];
		BenchProfiledFunction& func = functions[key];
		func.CallCount++;
		func.ExclusiveCycles += 100;
		benchmark::DoNotOptimize(func.CallCount);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Profiler_HashMap_Lookup)->Arg(100)->Arg(1000)->Arg(10000);

// Proposed: flat array indexed by address
static void BM_Profiler_FlatArray_Lookup(benchmark::State& state) {
	const uint32_t funcCount = static_cast<uint32_t>(state.range(0));
	std::vector<BenchProfiledFunction> functions(funcCount);

	auto addrs = GenerateRandomAddresses(10000, funcCount);
	uint32_t idx = 0;

	for (auto _ : state) {
		int32_t key = addrs[idx++ % addrs.size()];
		BenchProfiledFunction& func = functions[key];
		func.CallCount++;
		func.ExclusiveCycles += 100;
		benchmark::DoNotOptimize(func.CallCount);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Profiler_FlatArray_Lookup)->Arg(100)->Arg(1000)->Arg(10000);

// Current: UpdateCycles iterating function stack with hash lookups
static void BM_Profiler_UpdateCycles_HashMap(benchmark::State& state) {
	const int32_t stackDepth = static_cast<int32_t>(state.range(0));
	std::unordered_map<int32_t, BenchProfiledFunction> functions;
	std::deque<int32_t> functionStack;
	std::deque<StackFrameFlags> stackFlags;

	// Build stack
	for (int32_t i = 0; i < stackDepth; i++) {
		functions[i] = BenchProfiledFunction{};
		functionStack.push_back(i);
		stackFlags.push_back(StackFrameFlags::None);
	}
	int32_t currentFunction = stackDepth;
	functions[currentFunction] = BenchProfiledFunction{};

	for (auto _ : state) {
		uint64_t clockGap = 100;
		// Update exclusive cycles for current function
		functions[currentFunction].ExclusiveCycles += clockGap;
		functions[currentFunction].InclusiveCycles += clockGap;

		// Update inclusive cycles for all parents (current hot path)
		for (int32_t i = static_cast<int32_t>(functionStack.size()) - 1; i >= 0; i--) {
			functions[functionStack[i]].InclusiveCycles += clockGap;
			if (stackFlags[i] != StackFrameFlags::None) break;
		}
		benchmark::DoNotOptimize(functions[currentFunction].InclusiveCycles);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Profiler_UpdateCycles_HashMap)->Arg(5)->Arg(10)->Arg(20)->Arg(50);

// Proposed: UpdateCycles with cached pointers (no hash lookup)
static void BM_Profiler_UpdateCycles_CachedPtrs(benchmark::State& state) {
	const int32_t stackDepth = static_cast<int32_t>(state.range(0));
	std::vector<BenchProfiledFunction> functions(stackDepth + 1);

	struct CachedStackEntry {
		BenchProfiledFunction* func;
		StackFrameFlags flags;
	};
	std::vector<CachedStackEntry> functionStack(stackDepth);
	for (int32_t i = 0; i < stackDepth; i++) {
		functionStack[i] = { &functions[i], StackFrameFlags::None };
	}
	BenchProfiledFunction* currentFunc = &functions[stackDepth];

	for (auto _ : state) {
		uint64_t clockGap = 100;
		currentFunc->ExclusiveCycles += clockGap;
		currentFunc->InclusiveCycles += clockGap;

		// Direct pointer access — no hash lookup
		for (int32_t i = static_cast<int32_t>(functionStack.size()) - 1; i >= 0; i--) {
			functionStack[i].func->InclusiveCycles += clockGap;
			if (functionStack[i].flags != StackFrameFlags::None) break;
		}
		benchmark::DoNotOptimize(currentFunc->InclusiveCycles);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Profiler_UpdateCycles_CachedPtrs)->Arg(5)->Arg(10)->Arg(20)->Arg(50);

// =============================================================================
// 4. CallstackManager Benchmarks — deque vs ring buffer
// =============================================================================

// Current: deque<StackFrameInfo> push/pop
static void BM_Callstack_Deque_PushPop(benchmark::State& state) {
	std::deque<StackFrameInfo> callstack;
	StackFrameInfo frame = {};
	frame.Source = 0x8000;
	frame.Target = 0x9000;
	frame.Return = 0x8003;
	frame.ReturnStackPointer = 0xFD;

	for (auto _ : state) {
		callstack.push_back(frame);
		if (callstack.size() > 511) callstack.pop_front();
		benchmark::DoNotOptimize(callstack.back());
		callstack.pop_back();
	}
	state.SetItemsProcessed(state.iterations() * 2); // push + pop
}
BENCHMARK(BM_Callstack_Deque_PushPop);

// Proposed: fixed ring buffer
struct RingBuffer512 {
	std::array<StackFrameInfo, 512> data;
	uint32_t head = 0;
	uint32_t size = 0;

	void push_back(const StackFrameInfo& frame) {
		uint32_t idx = (head + size) & 511;
		data[idx] = frame;
		if (size < 512) {
			size++;
		} else {
			head = (head + 1) & 511;
		}
	}

	const StackFrameInfo& back() const {
		return data[(head + size - 1) & 511];
	}

	void pop_back() {
		if (size > 0) size--;
	}

	bool empty() const { return size == 0; }
};

static void BM_Callstack_RingBuffer_PushPop(benchmark::State& state) {
	RingBuffer512 callstack;
	StackFrameInfo frame = {};
	frame.Source = 0x8000;
	frame.Target = 0x9000;
	frame.Return = 0x8003;
	frame.ReturnStackPointer = 0xFD;

	for (auto _ : state) {
		callstack.push_back(frame);
		benchmark::DoNotOptimize(callstack.back());
		callstack.pop_back();
	}
	state.SetItemsProcessed(state.iterations() * 2);
}
BENCHMARK(BM_Callstack_RingBuffer_PushPop);

// IsReturnAddrMatch: deque reverse scan (current)
static void BM_Callstack_IsReturnMatch_Deque(benchmark::State& state) {
	const int32_t depth = static_cast<int32_t>(state.range(0));
	std::deque<StackFrameInfo> callstack;

	for (int32_t i = 0; i < depth; i++) {
		StackFrameInfo f = {};
		f.Return = 0x8000 + i * 3;
		callstack.push_back(f);
	}

	// Search for last entry (worst case = full scan, but found)
	uint32_t searchAddr = 0x8000;
	bool found = false;

	for (auto _ : state) {
		found = false;
		for (auto it = callstack.rbegin(); it != callstack.rend(); ++it) {
			if (it->Return == searchAddr) {
				found = true;
				break;
			}
		}
		benchmark::DoNotOptimize(found);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Callstack_IsReturnMatch_Deque)->Arg(5)->Arg(20)->Arg(100)->Arg(511);

// IsReturnAddrMatch: ring buffer scan (proposed)
static void BM_Callstack_IsReturnMatch_RingBuffer(benchmark::State& state) {
	const int32_t depth = static_cast<int32_t>(state.range(0));
	RingBuffer512 callstack;

	for (int32_t i = 0; i < depth; i++) {
		StackFrameInfo f = {};
		f.Return = 0x8000 + i * 3;
		callstack.push_back(f);
	}

	uint32_t searchAddr = 0x8000;
	bool found = false;

	for (auto _ : state) {
		found = false;
		for (uint32_t i = callstack.size; i > 0; i--) {
			uint32_t idx = (callstack.head + i - 1) & 511;
			if (callstack.data[idx].Return == searchAddr) {
				found = true;
				break;
			}
		}
		benchmark::DoNotOptimize(found);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Callstack_IsReturnMatch_RingBuffer)->Arg(5)->Arg(20)->Arg(100)->Arg(511);

// =============================================================================
// 5. Breakpoint Fast-Path Benchmarks
// =============================================================================

// Current: bool array check (fast path when no breakpoints)
static void BM_Breakpoint_NoBreakpoints_FastPath(benchmark::State& state) {
	bool hasBreakpointType[8] = {};
	MemoryOperationType opType = MemoryOperationType::Read;

	for (auto _ : state) {
		bool result = hasBreakpointType[(int)opType];
		benchmark::DoNotOptimize(result);
	}
	state.SetItemsProcessed(state.iterations());
	state.SetLabel("bool array check");
}
BENCHMARK(BM_Breakpoint_NoBreakpoints_FastPath);

// Proposed: bitmap fast-reject for address ranges
static void BM_Breakpoint_Bitmap_FastReject(benchmark::State& state) {
	// 64KB address bitset (8KB memory)
	auto bitmap = std::make_unique<uint8_t[]>(8192);
	std::memset(bitmap.get(), 0, 8192);

	// Set a few breakpoint addresses
	bitmap[0x8000 / 8] |= (1 << (0x8000 % 8));
	bitmap[0x9000 / 8] |= (1 << (0x9000 % 8));
	bitmap[0xA000 / 8] |= (1 << (0xA000 % 8));

	auto addrs = GenerateRandomAddresses(10000, 0xFFFF);
	uint32_t idx = 0;

	for (auto _ : state) {
		uint16_t addr = static_cast<uint16_t>(addrs[idx++ % addrs.size()]);
		bool mightHitBp = (bitmap[addr / 8] & (1 << (addr % 8))) != 0;
		benchmark::DoNotOptimize(mightHitBp);
	}
	state.SetItemsProcessed(state.iterations());
	state.SetLabel("8KB bitset check");
}
BENCHMARK(BM_Breakpoint_Bitmap_FastReject);

// =============================================================================
// 6. FrozenAddress Benchmarks
// =============================================================================

// Current: unordered_set<uint32_t> empty check + find
static void BM_FrozenAddr_HashSet_Empty(benchmark::State& state) {
	std::unordered_set<uint32_t> frozenAddresses;
	auto addrs = GenerateRandomAddresses(10000, 0xFFFF);
	uint32_t idx = 0;

	for (auto _ : state) {
		uint32_t addr = addrs[idx++ % addrs.size()];
		bool frozen = frozenAddresses.size() > 0 && frozenAddresses.find(addr) != frozenAddresses.end();
		benchmark::DoNotOptimize(frozen);
	}
	state.SetItemsProcessed(state.iterations());
	state.SetLabel("empty set fast path");
}
BENCHMARK(BM_FrozenAddr_HashSet_Empty);

// Current: unordered_set with some frozen addresses
static void BM_FrozenAddr_HashSet_WithAddrs(benchmark::State& state) {
	std::unordered_set<uint32_t> frozenAddresses;
	for (int i = 0; i < 20; i++) {
		frozenAddresses.insert(0x100 + i);
	}
	auto addrs = GenerateRandomAddresses(10000, 0xFFFF);
	uint32_t idx = 0;

	for (auto _ : state) {
		uint32_t addr = addrs[idx++ % addrs.size()];
		bool frozen = frozenAddresses.size() > 0 && frozenAddresses.find(addr) != frozenAddresses.end();
		benchmark::DoNotOptimize(frozen);
	}
	state.SetItemsProcessed(state.iterations());
	state.SetLabel("20 frozen addresses");
}
BENCHMARK(BM_FrozenAddr_HashSet_WithAddrs);

// Proposed: bitset for 64KB address space (8KB)
static void BM_FrozenAddr_Bitset(benchmark::State& state) {
	auto bitset = std::make_unique<uint8_t[]>(8192);
	std::memset(bitset.get(), 0, 8192);
	for (int i = 0; i < 20; i++) {
		uint32_t addr = 0x100 + i;
		bitset[addr / 8] |= (1 << (addr % 8));
	}
	auto addrs = GenerateRandomAddresses(10000, 0xFFFF);
	uint32_t idx = 0;

	for (auto _ : state) {
		uint32_t addr = addrs[idx++ % addrs.size()];
		bool frozen = (bitset[addr / 8] & (1 << (addr % 8))) != 0;
		benchmark::DoNotOptimize(frozen);
	}
	state.SetItemsProcessed(state.iterations());
	state.SetLabel("8KB bitset");
}
BENCHMARK(BM_FrozenAddr_Bitset);

// =============================================================================
// 7. Event Manager Benchmarks
// =============================================================================

// Local event struct that mirrors DebugEventInfo size (~80+ bytes)
// Avoids pulling in DmaChannelConfig (SNES-specific)
struct BenchEventInfo {
	MemoryOperationInfo Operation;
	uint32_t Type;
	uint32_t ProgramCounter;
	int16_t Scanline;
	uint16_t Cycle;
	int16_t BreakpointId;
	int8_t DmaChannel;
	uint8_t _pad1[32]; // Approximate DmaChannelConfig space
	uint32_t Flags;
	int32_t RegisterId;
	MemoryOperationInfo TargetMemory;
	uint32_t Color;
};

// Current: vector push_back with large structs
static void BM_EventManager_VectorPush(benchmark::State& state) {
	std::vector<BenchEventInfo> events;
	events.reserve(100000);

	BenchEventInfo event = {};
	event.ProgramCounter = 0x8000;
	event.Scanline = 100;
	event.Cycle = 50;

	for (auto _ : state) {
		events.push_back(event);
		if (events.size() >= 100000) events.clear();
		benchmark::DoNotOptimize(events.back());
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_EventManager_VectorPush);

// Proposed: pre-allocated ring buffer (fixed capacity)
static void BM_EventManager_RingBuffer(benchmark::State& state) {
	constexpr uint32_t kCapacity = 65536; // Power of 2 for mask
	std::array<BenchEventInfo, kCapacity> events;
	uint32_t writePos = 0;

	BenchEventInfo event = {};
	event.ProgramCounter = 0x8000;
	event.Scanline = 100;
	event.Cycle = 50;

	for (auto _ : state) {
		events[writePos & (kCapacity - 1)] = event;
		writePos++;
		benchmark::DoNotOptimize(events[(writePos - 1) & (kCapacity - 1)]);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_EventManager_RingBuffer);

// =============================================================================
// 8. Composite: Simulated Full Debugger Pipeline
// =============================================================================

// Simulates what happens per NES instruction in the full debugger pipeline
// (excluding disassembly cache, trace logging, and actual address translation)
static void BM_FullDebuggerPipeline_Current(benchmark::State& state) {
	// Simulate all data structures at current sizes
	std::vector<AddressCountersAoS> memCounters(kSmallRomSize);
	std::memset(memCounters.data(), 0, kSmallRomSize * sizeof(AddressCountersAoS));
	auto cdlData = std::make_unique<uint8_t[]>(kSmallRomSize);
	std::memset(cdlData.get(), 0, kSmallRomSize);
	std::unordered_map<int32_t, BenchProfiledFunction> profilerFunctions;
	profilerFunctions[-1] = BenchProfiledFunction{};
	std::deque<int32_t> profilerStack;
	std::deque<StackFrameFlags> profilerFlags;
	bool hasBreakpointType[8] = {};
	std::unordered_set<uint32_t> frozenAddresses;

	auto addrs = GenerateSequentialAddresses(10000, 0);
	auto dataAddrs = GenerateRandomAddresses(10000, kSmallRomSize);
	uint32_t pcIdx = 0;
	uint32_t dataIdx = 0;
	uint64_t clock = 1000;
	int32_t currentFunction = -1;

	for (auto _ : state) {
		uint32_t pc = addrs[pcIdx++ % addrs.size()];
		uint32_t dataAddr = dataAddrs[dataIdx++ % dataAddrs.size()];

		// --- ProcessInstruction ---
		// CDL SetCode
		cdlData[pc] |= CdlFlags::Code;
		cdlData[pc + 1] |= CdlFlags::Code;
		cdlData[pc + 2] |= CdlFlags::Code;

		// MemoryAccessCounter: exec × 3
		memCounters[pc].ExecStamp = clock;
		memCounters[pc].ExecCounter++;
		memCounters[pc + 1].ExecStamp = clock;
		memCounters[pc + 1].ExecCounter++;
		memCounters[pc + 2].ExecStamp = clock;
		memCounters[pc + 2].ExecCounter++;

		// StepRequest check (single decrement)
		int64_t stepCount = 1000;
		stepCount--;
		benchmark::DoNotOptimize(stepCount);

		// --- ProcessRead (data) × 2 ---
		cdlData[dataAddr % kSmallRomSize] |= CdlFlags::Data;
		memCounters[dataAddr % kSmallRomSize].ReadStamp = clock;
		memCounters[dataAddr % kSmallRomSize].ReadCounter++;
		cdlData[(dataAddr + 1) % kSmallRomSize] |= CdlFlags::Data;
		memCounters[(dataAddr + 1) % kSmallRomSize].ReadStamp = clock;
		memCounters[(dataAddr + 1) % kSmallRomSize].ReadCounter++;

		// --- ProcessWrite × 1 ---
		memCounters[(dataAddr + 2) % kSmallRomSize].WriteStamp = clock;
		memCounters[(dataAddr + 2) % kSmallRomSize].WriteCounter++;

		// Breakpoint checks × 6 (fast path: no breakpoints)
		for (int i = 0; i < 6; i++) {
			benchmark::DoNotOptimize(hasBreakpointType[0]);
		}

		// Frozen address check × 1 (write)
		bool frozen = frozenAddresses.size() > 0;
		benchmark::DoNotOptimize(frozen);

		// Profiler: current function cycles (amortized)
		if ((pcIdx & 0xFF) == 0) { // ~1/256 instructions are JSR
			profilerFunctions[currentFunction].ExclusiveCycles += 100;
		}

		clock++;
	}
	state.SetItemsProcessed(state.iterations());
	state.SetLabel("simulated full pipeline (current)");
}
BENCHMARK(BM_FullDebuggerPipeline_Current);

// Optimized pipeline: SoA counters, flat profiler, no timestamps when not needed
static void BM_FullDebuggerPipeline_Optimized(benchmark::State& state) {
	AddressCountersSoA memCounters(kSmallRomSize);
	auto cdlData = std::make_unique<uint8_t[]>(kSmallRomSize);
	std::memset(cdlData.get(), 0, kSmallRomSize);
	std::vector<BenchProfiledFunction> profilerFunctions(kSmallRomSize);
	bool hasBreakpointType[8] = {};

	auto addrs = GenerateSequentialAddresses(10000, 0);
	auto dataAddrs = GenerateRandomAddresses(10000, kSmallRomSize);
	uint32_t pcIdx = 0;
	uint32_t dataIdx = 0;
	uint64_t clock = 1000;

	for (auto _ : state) {
		uint32_t pc = addrs[pcIdx++ % addrs.size()];
		uint32_t dataAddr = dataAddrs[dataIdx++ % dataAddrs.size()];

		// CDL SetCode (same)
		cdlData[pc] |= CdlFlags::Code;
		cdlData[pc + 1] |= CdlFlags::Code;
		cdlData[pc + 2] |= CdlFlags::Code;

		// SoA counters (no timestamps in hot path)
		memCounters.execCounters[pc]++;
		memCounters.execCounters[pc + 1]++;
		memCounters.execCounters[pc + 2]++;
		memCounters.readCounters[dataAddr % kSmallRomSize]++;
		memCounters.readCounters[(dataAddr + 1) % kSmallRomSize]++;
		memCounters.writeCounters[(dataAddr + 2) % kSmallRomSize]++;

		// StepRequest (single check)
		int64_t stepCount = 1000;
		stepCount--;
		benchmark::DoNotOptimize(stepCount);

		// CDL data flags
		cdlData[dataAddr % kSmallRomSize] |= CdlFlags::Data;
		cdlData[(dataAddr + 1) % kSmallRomSize] |= CdlFlags::Data;

		// Breakpoint fast path (single bool)
		benchmark::DoNotOptimize(hasBreakpointType[0]);

		// Flat profiler lookup (amortized ~1/256)
		if ((pcIdx & 0xFF) == 0) {
			profilerFunctions[pc].ExclusiveCycles += 100;
		}

		clock++;
	}
	state.SetItemsProcessed(state.iterations());
	state.SetLabel("simulated optimized pipeline");
}
BENCHMARK(BM_FullDebuggerPipeline_Optimized);

// CDL-only pipeline (lightweight recorder — for comparison)
static void BM_CDLOnly_Pipeline(benchmark::State& state) {
	auto cdlData = std::make_unique<uint8_t[]>(kSmallRomSize);
	std::memset(cdlData.get(), 0, kSmallRomSize);
	auto addrs = GenerateSequentialAddresses(10000, 0);
	auto dataAddrs = GenerateRandomAddresses(10000, kSmallRomSize);
	uint32_t pcIdx = 0;
	uint32_t dataIdx = 0;

	for (auto _ : state) {
		uint32_t pc = addrs[pcIdx++ % addrs.size()];
		uint32_t dataAddr = dataAddrs[dataIdx++ % dataAddrs.size()];

		// CDL only — no counters, no breakpoints, no profiler
		cdlData[pc] |= CdlFlags::Code;
		cdlData[pc + 1] |= CdlFlags::Code;
		cdlData[pc + 2] |= CdlFlags::Code;
		cdlData[dataAddr % kSmallRomSize] |= CdlFlags::Data;
		cdlData[(dataAddr + 1) % kSmallRomSize] |= CdlFlags::Data;

		benchmark::DoNotOptimize(cdlData[pc]);
	}
	state.SetItemsProcessed(state.iterations());
	state.SetLabel("CDL flags only (floor)");
}
BENCHMARK(BM_CDLOnly_Pipeline);
