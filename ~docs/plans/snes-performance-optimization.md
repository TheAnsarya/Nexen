# SNES Emulation Performance Optimization Plan

> Epic: [#1361](https://github.com/TheAnsarya/Nexen/issues/1361)

## Objective

Maximize SNES emulation throughput without any changes to emulation behavior. Every optimization must be correctness-preserving and verified by the full test suite (1495+ C++ tests).

## Research Summary

### Hot Path Analysis

The SNES emulator processes ~21.477 million master clocks per second. The `SnesMemoryManager::Exec()` function runs every 2 master clocks (~10.7M calls/sec), making everything it touches a critical hot path.

**Call frequency per frame (60 FPS):**

| Function | Calls/Frame | Notes |
|----------|-------------|-------|
| `Exec()` | ~178,000 | Every 2 master clocks |
| `Read()` / `Write()` | ~60,000+ | Every CPU memory access |
| `CheckFlag()` | ~120,000+ | 2-4x per instruction |
| `SetZeroNegativeFlags()` | ~30,000+ | After most ALU ops |
| `SyncCoprocessors()` | ~178,000 | Already `[[unlikely]]` guarded |
| `ProcessPpuCycle()` | ~89,000 | Already `[[unlikely]]` guarded |

### Areas Already Optimized (No Changes Needed)

| Area | Technique | Status |
|------|-----------|--------|
| PPU rendering | Heavy template specialization | Optimal |
| Memory map lookup | O(1) array table (`_handlers[addr >> 12]`) | Optimal |
| Notification system | Snapshot pattern, pre-reserved buffer | Optimal |
| Debugger hooks | `[[unlikely]]` + `__forceinline` template guards | Optimal |
| Coprocessor sync | `[[unlikely]]` early-exit | Optimal |
| SPC700 timing | `static constexpr` arrays, `[[unlikely]]` hints | Optimal |
| CPU instruction dispatch | 256-case switch (compiler jump table) | Optimal |

## Optimizations Planned

### 1. Replace Function Pointer Dispatch (#1362)

**Impact: MODERATE (2-5% estimated)**

**Current code (SnesMemoryManager.cpp):**

```cpp
uint8_t SnesMemoryManager::Read(uint32_t addr, MemoryOperationType type) {
	(this->*_execRead)();  // Indirect member function pointer call
	// ...
}
```

**Problem:** Member function pointers (`_execRead`/`_execWrite`) require an indirect call instruction. While modern CPUs have indirect branch target buffers, the prediction isn't perfect — especially during transitions between speed modes.

**Optimized code:**

```cpp
uint8_t SnesMemoryManager::Read(uint32_t addr, MemoryOperationType type) {
	ExecReadTiming();  // Inline switch on _cpuSpeed
	// ...
}
```

Where `ExecReadTiming()` is:

```cpp
__forceinline void ExecReadTiming() {
	switch (_cpuSpeed) {
		case 8: [[likely]] IncMasterClock<4>(); break;
		case 6: IncMasterClock<2>(); break;
		case 12: IncMasterClock<8>(); break;
	}
}
```

**Why this is better:**

- Direct template calls (compiler can inline `IncMasterClock<N>`)
- 3-case switch is highly predictable (CPU speed rarely changes)
- `[[likely]]` on case 8 (most common speed mode)
- No pointer dereference overhead

### 2. CPU Method Inlining (#1363)

**Impact: LOW (0.5-1% estimated)**

Add `__forceinline` to these high-frequency CPU methods:

- `CheckFlag()` — 1-line bitwise check
- `SetZeroNegativeFlags(uint8_t)` — 3-line branchless flag set
- `SetZeroNegativeFlags(uint16_t)` — 3-line branchless flag set
- `ClearFlags()` — 1-line bitwise AND
- `SetFlags()` — 1-line bitwise OR

These are already likely inlined in Release builds, but `__forceinline` ensures it across all configurations.

### 3. [[nodiscard]] Attributes (#1364)

**Impact: ZERO (compile-time only)**

Add `[[nodiscard]]` to pure query methods to catch accidental value drops:

- `CheckFlag()` — returns bool
- `GetOpCode()` — returns uint8_t
- `GetByteValue()` / `GetWordValue()` — return operand values

### 4. Memory Dispatch Benchmark (#1365)

**Impact: MEASUREMENT ONLY**

Add benchmark comparing:

- Function pointer dispatch overhead
- Switch-based dispatch overhead
- Simulated full Read path

## Verification Plan

1. Run full C++ test suite: `Core.Tests.exe --gtest_brief=1`
2. Run .NET tests: `dotnet test --no-build -c Release`
3. Run SNES benchmarks before and after changes
4. Compare benchmark results for measurable improvement
5. Verify zero build warnings introduced

## Risk Assessment

| Change | Risk | Mitigation |
|--------|------|------------|
| Function pointer → switch | SAFE | Exact same semantics |
| `__forceinline` additions | SAFE | Compiler hint only |
| `[[nodiscard]]` additions | SAFE | Compile-time annotation |
| New benchmark | SAFE | Test code only |
