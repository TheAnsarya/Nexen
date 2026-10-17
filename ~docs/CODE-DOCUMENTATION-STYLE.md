# C++ Code Documentation Style Guide

This guide establishes consistent documentation standards for the Nexen C++ codebase.

## Overview

Documentation is critical for:

- **Maintenance**: Understanding code months/years later
- **Onboarding**: Helping new contributors understand the codebase
- **API usage**: Documenting public interfaces
- **Debugging**: Understanding design decisions and edge cases

## Comment Types and When to Use Them

### File Headers

Every source file should have a header comment:

```cpp
// =============================================================================
// NesCpu.cpp - NES 6502 CPU Emulation
// =============================================================================
// Implements the Ricoh 2A03/2A07 CPU used in NES/Famicom consoles.
// Based on the MOS 6502 with minor differences (no decimal mode).
//
// Key components:
// - Instruction execution with cycle-accurate timing
// - Memory read/write through the bus
// - Interrupt handling (NMI, IRQ, RESET)
// =============================================================================
```

### Class/Struct Documentation

Document the purpose, responsibilities, and usage:

```cpp
/// <summary>
/// Manages NES CPU state and instruction execution.
/// </summary>
/// <remarks>
/// The NES uses a Ricoh 2A03 (NTSC) or 2A07 (PAL) which is a modified
/// MOS 6502 without decimal mode support. This class handles:
/// - All 56 documented 6502 opcodes
/// - Unofficial/undocumented opcodes for compatibility
/// - Cycle-accurate timing for PPU synchronization
/// - DMA handling for OAM transfers
/// </remarks>
class NesCpu : public ISerializable {
```

### Method Documentation

Use triple-slash comments for public methods:

```cpp
/// <summary>
/// Executes a single CPU instruction.
/// </summary>
/// <returns>Number of cycles consumed by the instruction.</returns>
/// <remarks>
/// The cycle count includes any page-crossing penalties.
/// DMA cycles are not included in the return value but are
/// tracked separately via GetDmaCycles().
/// </remarks>
uint8_t Exec();

/// <summary>
/// Reads a byte from the specified memory address.
/// </summary>
/// <param name="addr">16-bit memory address (0x0000-0xFFFF)</param>
/// <returns>Byte value at the address</returns>
/// <remarks>
/// This method triggers mapper read handling and updates
/// the open bus value. Reading from certain addresses may
/// have side effects (e.g., clearing IRQ flags).
/// </remarks>
uint8_t Read(uint16_t addr);
```

### Implementation Comments

Use single-line or block comments for implementation details:

```cpp
// Good: Explains WHY, not WHAT
// BCD mode is disabled on the NES 2A03 - skip decimal adjustment
if (GetCpuType() != CpuType::Nes) {
	ApplyDecimalAdjustment();
}

// Good: Documents edge case
// Writing to $4016 bit 0 controls both controller strobe AND
// expansion port bits - must preserve upper bits
_memoryManager->Write(addr, value);

// Bad: States the obvious
// Increment the program counter
_state.PC++;
```

### Constants and Magic Numbers

Always document magic numbers or extract to named constants:

```cpp
// Good: Named constants with documentation
static constexpr uint16_t NMI_VECTOR = 0xfffa;   // NMI handler address
static constexpr uint16_t RESET_VECTOR = 0xfffc; // Power-on entry point
static constexpr uint16_t IRQ_VECTOR = 0xfffe;   // IRQ/BRK handler

// Good: Inline comment for values
_state.SP = 0xfd;  // Stack pointer after reset (3 phantom pushes)

// Bad: Unexplained magic number
if (scanline == 241) {  // Why 241?
```

## Special Documentation Tags

### TODO Comments

Format with category and description:

```cpp
// TODO(perf): Cache decoded instruction to avoid re-decoding on repeated execution
// TODO(accuracy): Implement half-cycle accuracy for DMA timing
// TODO(refactor): Extract common addressing mode logic to helper
```

### HACK Comments

Document workarounds with context:

```cpp
// HACK: Some games (e.g., Battletoads) rely on specific open bus behavior
// during mid-scanline reads. This workaround preserves compatibility.
_openBus = (addr >> 8);
```

### NOTE Comments

Highlight non-obvious behavior:

```cpp
// NOTE: The unofficial LAX instruction combines LDA and LDX,
// but sets flags based on the loaded value (affects N and Z).
```

## Section Separators

Use consistent separators for logical sections:

```cpp
// =============================================================================
// Initialization
// =============================================================================

void NesCpu::Init() { ... }
void NesCpu::Reset() { ... }

// =============================================================================
// Instruction Execution
// =============================================================================

uint8_t NesCpu::Exec() { ... }

// =============================================================================
// Memory Access
// =============================================================================

uint8_t NesCpu::Read(uint16_t addr) { ... }
void NesCpu::Write(uint16_t addr, uint8_t value) { ... }
```

## Inline Documentation vs External Docs

### Inline (in code)

- API contracts (parameters, return values)
- Side effects and edge cases
- Algorithm explanations
- Platform-specific behavior

### External (in ~docs/)

- Architecture overviews
- Design decisions and rationale
- Cross-cutting concerns
- Getting started guides

## What NOT to Document

Avoid stating the obvious:

```cpp
// Bad: Obvious from the code
int x = 0;  // Initialize x to zero
count++;	// Increment count

// Bad: Restating the method name
/// <summary>Gets the program counter.</summary>
uint16_t GetPC() { return _state.PC; }

// Good: Add value when there's something non-obvious
/// <summary>Gets the program counter after applying bank mapping.</summary>
uint16_t GetMappedPC() { return _mapper->MapAddress(_state.PC); }
```

## Hexadecimal Formatting

Always use lowercase hex values:

```cpp
// Good
constexpr uint16_t addr = 0x2000;
if (value & 0x80) { ... }

// Bad
constexpr uint16_t addr = 0x2000;  // Uppercase not allowed
```

## Example: Well-Documented Method

```cpp
/// <summary>
/// Handles the CPU interrupt sequence (NMI, IRQ, or BRK).
/// </summary>
/// <param name="type">Type of interrupt to process</param>
/// <remarks>
/// The interrupt sequence takes 7 cycles:
/// 1. Two internal cycles (reading current instruction)
/// 2. Push PCH to stack
/// 3. Push PCL to stack
/// 4. Push status with B flag (set for BRK, clear for IRQ/NMI)
/// 5. Read low byte of handler address
/// 6. Read high byte of handler address
///
/// Note: NMI has higher priority than IRQ. If both are pending,
/// NMI is serviced first but the IRQ will still be acknowledged.
/// </remarks>
void NesCpu::ProcessInterrupt(InterruptType type) {
	// BRK sets B flag in pushed status; hardware interrupts don't
	uint8_t statusToPush = _state.PS | ProcessorFlags::Reserved;
	if (type == InterruptType::Brk) {
		statusToPush |= ProcessorFlags::Break;
	}

	// Push return address (current PC) to stack
	// NOTE: BRK pushes PC+2, but interrupts push current PC
	Push(static_cast<uint8_t>(_state.PC >> 8));
	Push(static_cast<uint8_t>(_state.PC & 0xff));
	Push(statusToPush);

	// Set interrupt disable flag to prevent nested interrupts
	_state.PS |= ProcessorFlags::Interrupt;

	// Fetch handler address from appropriate vector
	uint16_t vector = GetInterruptVector(type);
	_state.PC = Read(vector) | (Read(vector + 1) << 8);
}
```

## Tools and Enforcement

- Use consistent style across all files
- IDE extensions can help highlight inconsistencies
- Code reviews should check documentation quality
- Consider Doxygen for generated API docs (future)

## Related Documentation

- [CPP-DEVELOPMENT-GUIDE.md](CPP-DEVELOPMENT-GUIDE.md) - General C++ coding standards
- [PROFILING-GUIDE.md](PROFILING-GUIDE.md) - Performance documentation practices
