# C++ Modernization Opportunities

This document tracks potential C++ modernization opportunities for Mesen2, with careful consideration of performance implications.

> **CRITICAL**: Mesen2 is an emulator where performance is paramount. Any modernization that introduces runtime overhead in hot paths should be carefully evaluated or avoided entirely.

## Priority Legend
- 🟢 **Safe** - No performance impact, pure readability/safety improvement
- 🟡 **Cautious** - May have minor overhead, profile before adopting widely
- 🔴 **Avoid in Hot Paths** - Significant overhead, only use in cold code
- ⏸️ **Deferred** - Requires more investigation or C++26 features

---

## 1. std::format (C++20/23)

**Current State:** ~199 uses of sprintf/snprintf/std::to_string

**Analysis:**
- std::format provides type-safe, locale-independent formatting
- **Performance concern:** std::format can be slower than sprintf in some implementations
- MSVC's std::format is reasonably optimized but still has overhead vs raw sprintf

**Recommendation:** 🔴 **Avoid in Hot Paths**
- Use in UI code, logging, and error messages (cold paths)
- Keep sprintf in performance-critical code paths
- Consider `std::format_to` with pre-allocated buffers for better performance

**Safe Adoption Areas:**
- `MessageManager::Log()` calls
- Error handling paths
- Debug output (already not in release builds)
- String formatting for display purposes

**Avoid In:**
- Disassembler inner loops
- Per-frame processing
- Memory access hot paths

---

## 2. std::span (C++20)

**Current State:** Many `(pointer, size)` parameter pairs identified

**Analysis:**
- std::span is a non-owning view over contiguous memory
- Zero-overhead abstraction when used correctly
- Provides bounds checking in debug builds

**Recommendation:** 🟡 **Cautious - Profile Before Wide Adoption**

**Performance Characteristics:**
- In Release builds: Should be zero-overhead (same as pointer+size)
- In Debug builds: Bounds checking adds overhead
- Compiler may not always optimize span iteration as well as raw pointers

**Safe Adoption Areas:**
```cpp
// Before
void ProcessData(uint8_t* data, size_t length);

// After
void ProcessData(std::span<const uint8_t> data);
```

Functions that could benefit:
- `MemoryDumper::SetMemoryState()`
- `CdlManager::SetCdlData()`
- `CodeDataLogger::SetCdlData()`
- `PpuTools::GetTileView()` (for source buffer)

**Avoid In:**
- CPU emulation core (keep raw pointers)
- Memory mapping hot paths
- DMA transfer code

**Testing Required:**
Before adopting, benchmark:
1. NES CPU emulation with span vs raw pointer
2. Memory dump operations
3. Tile rendering loops

---

## 3. std::views / Ranges (C++20/23)

**Current State:** Some std::ranges algorithms adopted, no std::views yet

**Analysis:**
- Views enable lazy evaluation and composition
- **Performance concern:** View adaptor chains can prevent vectorization
- Compiler optimization of views is still maturing

**Recommendation:** 🔴 **Avoid in Hot Paths, Use Sparingly**

**Why Avoid in Emulators:**
```cpp
// This innocent-looking code:
for (auto x : vec | std::views::filter(pred) | std::views::transform(fn)) {
    // ...
}

// May generate worse code than:
for (auto& x : vec) {
    if (pred(x)) {
        auto y = fn(x);
        // ...
    }
}
```

**Safe Adoption Areas:**
- Configuration parsing (one-time startup)
- File listing/filtering
- UI data presentation
- Debug/diagnostic code

**Example Safe Uses:**
```cpp
// Filtering game folders (cold path)
auto validFolders = folders | std::views::filter([](auto& f) {
    return std::filesystem::exists(f);
});

// Processing cheats (user interaction, not per-frame)
auto enabledCheats = cheats | std::views::filter(&Cheat::enabled);
```

---

## 4. Smart Pointers

**Current State:**
- 692 uses of unique_ptr/shared_ptr
- 1040 uses of raw `new`

**Analysis:**
- Project already uses smart pointers extensively
- Many raw `new` are for:
  - Array allocations (should often stay raw for performance)
  - Placement new
  - Interop with C APIs

**Recommendation:** 🟢 **Safe for New Code, Selective Migration**

**Safe Migration Targets:**
- Objects created and destroyed infrequently
- RAII resource management
- Exception safety improvements

**Keep Raw Pointers For:**
- Large memory buffers (video, audio, ROM data)
- Arrays accessed in tight loops
- Memory-mapped regions

**Pattern to Adopt:**
```cpp
// Factory functions returning unique_ptr
static std::unique_ptr<IMapper> CreateMapper(int mapperId);

// Use make_unique over new
auto console = std::make_unique<NesConsole>();
```

---

## 5. constexpr Improvements (C++20/23)

**Current State:** Limited constexpr usage

**Analysis:**
- C++20/23 greatly expanded what can be constexpr
- Pure compile-time optimization, no runtime cost

**Recommendation:** 🟢 **Safe - Adopt Liberally**

**Opportunities:**
- Lookup tables (opcode tables, timing tables)
- CRC calculation (for compile-time constants)
- Bit manipulation utilities
- String hashing for compile-time switch

**Example:**
```cpp
// Opcode cycle counts could be constexpr arrays
constexpr std::array<uint8_t, 256> CpuCycleCounts = { /* ... */ };

// constexpr bit manipulation
constexpr uint8_t MirrorBits(uint8_t value) {
    // Implementation
}
```

---

## 6. [[likely]] / [[unlikely]] Attributes (C++20)

**Current State:** Not used

**Analysis:**
- Hints to compiler for branch prediction
- Zero runtime cost, purely compile-time hint

**Recommendation:** 🟢 **Safe - Use in Hot Paths**

**Ideal Candidates:**
```cpp
// CPU emulation - most memory accesses are not hardware registers
if ([[likely]] address < 0x2000) {
    return _ram[address & 0x7FF];
}

// PPU rendering - most scanlines are visible
if ([[unlikely]] scanline >= 240) {
    // VBlank handling
}

// Error conditions
if ([[unlikely]] error) {
    HandleError();
}
```

---

## 7. std::bit_cast (C++20)

**Current State:** Uses reinterpret_cast and union type punning

**Analysis:**
- std::bit_cast is type-safe and constexpr-capable
- Defined behavior (unlike some reinterpret_cast uses)

**Recommendation:** 🟢 **Safe - Adopt Where Applicable**

**Use Cases:**
```cpp
// Before
uint32_t asInt = *reinterpret_cast<uint32_t*>(&floatValue);

// After
uint32_t asInt = std::bit_cast<uint32_t>(floatValue);
```

---

## 8. Testing Infrastructure

**Current State:** No formal test framework identified in Core

**Opportunities:**
- Unit tests for CPU instruction decoding
- Property-based testing for assembler/disassembler roundtrip
- Fuzzing for ROM loader security

**Recommendation:** ⏸️ **Deferred - Requires Architectural Discussion**

---

## Implementation Phases

### Phase 1: Zero-Cost Improvements ✅ (Completed)
- [x] std::ranges algorithms (std::ranges::transform, sort, etc.)
- [x] C++20 string methods (starts_with, ends_with, contains)
- [x] Fix C4244 warnings (tolower/toupper)

### Phase 2: Safe Modernization (Current)
- [ ] [[likely]]/[[unlikely]] attributes in hot paths
- [ ] constexpr expansion for lookup tables
- [ ] std::bit_cast adoption
- [ ] Smart pointer migration for cold paths

### Phase 3: Measured Modernization (Requires Profiling)
- [ ] std::span for non-critical APIs
- [ ] std::format for cold paths
- [ ] Further std::ranges in cold code

### Phase 4: Future Consideration
- [ ] std::views (need better compiler support)
- [ ] Testing infrastructure
- [ ] Module support (C++20 modules)

---

## Performance Testing Requirements

Before adopting any change in Phase 3+, measure:

1. **Frame time variance** - Should not increase
2. **CPU emulation cycles/second** - Critical benchmark
3. **Memory throughput** - For span/format changes
4. **Compile time impact** - Some features slow compilation

**Benchmark suite needed:**
- NES: Battletoads (heavy CPU/PPU)
- SNES: Star Fox (heavy math)
- GB: Pokémon (large world, lots of VRAM)

---

## References

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
- [std::format Performance Analysis](https://vitaut.net/posts/2024/format-strings/)
- [std::span Design Rationale](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0122r7.pdf)
- [Ranges and Views Performance](https://www.youtube.com/watch?v=d_E-VLyUnzc)
