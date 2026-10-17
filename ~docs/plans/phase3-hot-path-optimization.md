# Phase 3: Hot-Path Optimization Plan

## Overview

Phase 3 extends branchless flag optimizations to the remaining CPU cores (PCE HuC6280, SMS Z80)
and adds `[[likely]]/[[unlikely]]` annotations to GBA memory access paths.

## Phase 2 Results (50-rep, 2s min time)

| Benchmark | Branching | Branchless | Improvement |
|-----------|-----------|------------|-------------|
| NES CMP | 4.82 ns | 4.15 ns | **-13.9%** |
| NES ADD | 9.23 ns | ~7.4 ns | **~-19.8%** |
| NES SetZeroNeg | - | 4.25 ns | baseline |
| SNES TestBits8 | - | - | **-44%** (prior) |

## Phase 3 Targets

### Tier 1: Proven Patterns (LOW RISK)

#### 3.1 PCE CPU — Branchless `SetZeroNegativeFlags`

**File:** `Core/PCE/PceCpu.cpp` line 287

PCE flags match NES layout exactly:

- Carry=0x01, Zero=0x02, Negative=0x80

Current branching:

```cpp
void PceCpu::SetZeroNegativeFlags(uint8_t value) {
	if (value == 0) {
		SetFlags(PceCpuFlags::Zero);
	} else if (value & 0x80) {
		SetFlags(PceCpuFlags::Negative);
	}
}
```

Branchless replacement:

```cpp
void PceCpu::SetZeroNegativeFlags(uint8_t value) {
	_state.PS |= (value == 0) ? PceCpuFlags::Zero : 0;
	_state.PS |= (value & PceCpuFlags::Negative);
}
```

**Call sites:** ~12+ (every ALU/load/shift/compare instruction)

#### 3.2 SMS CPU — Branchless `SetFlagState`

**File:** `Core/SMS/SmsCpu.cpp` line 1597

SMS flags: Carry=0x01, Zero=0x40, Sign=0x80

Current branching:

```cpp
void SmsCpu::SetFlagState(uint8_t flag, bool state) {
	if (state) {
		SetFlag(flag);  // _state.Flags |= flag; _state.FlagsChanged |= 0x01;
	} else {
		ClearFlag(flag);  // _state.Flags &= ~flag; _state.FlagsChanged |= 0x01;
	}
}
```

Branchless replacement (matches GB pattern):

```cpp
void SmsCpu::SetFlagState(uint8_t flag, bool state) {
	_state.Flags = (_state.Flags & ~flag) | (-static_cast<uint8_t>(state) & flag);
	_state.FlagsChanged |= 0x01;
}
```

**Call sites:** ~20+ (every ALU, shift, compare, block op)

### Tier 2: Additional Improvements

#### 3.3 GBA `_ldmGlitch` — `[[unlikely]]` annotation

Every GBA memory access checks `_ldmGlitch` (almost always 0).
Add `[[unlikely]]` to the check.

#### 3.4 PCE Consolidated Flag Patterns

Merge separate clear/set/SetZeroNeg into single expressions for
ASL, LSR, ROL, ROR, CMP operations.

## Testing Strategy

- Comparison tests for all PCE flag operations (branching vs branchless)
- Comparison tests for all SMS flag operations (branching vs branchless)
- Benchmark pairs for PCE and SMS (branching vs branchless)
- 30+ repetitions for statistical validity
