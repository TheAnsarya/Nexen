# Channel F SRAM Persistence — Code Plan (#1006)

## Overview

The Channel F has no battery-backed SRAM in the traditional sense. However, some multicarts and homebrew cartridges include SRAM for save data. This plan covers adding optional SRAM save/load support.

## Hardware Context

### Standard Channel F Cartridges

- ROM only (2 KB – 64 KB depending on cart board type)
- No save capability — games rely on passwords or have no saves
- Cart boards: Schach (chess), Videocart standard, homebrew boards

### SRAM-Equipped Cartridges

Some modern homebrew cartridges include:
- 2 KB – 8 KB battery-backed SRAM
- Mapped into the cartridge address space (typically $2800–$2FFF or $3000–$3FFF)
- Read/write via standard memory-mapped I/O

### Current Implementation

In `ChannelFMemoryManager`:
- Cart ROM is loaded and mapped read-only
- No SRAM mapping exists yet
- Cart board types are partially defined (#976)

## Implementation Steps

### Phase 1: SRAM Region Definition

1. Add `_sram` buffer to `ChannelFMemoryManager` (up to 8 KB)
2. Add `_hasSram` and `_sramSize` fields
3. Define SRAM address range per cart board type
4. Route reads/writes to SRAM buffer when address falls in SRAM range

### Phase 2: Save File I/O

1. On emulator shutdown or save request, write `_sram` to `.sav` file
2. On ROM load, check for existing `.sav` file and load into `_sram`
3. File naming: `{rom_name}.sav` alongside the ROM or in saves directory
4. Use existing Nexen save file infrastructure (same pattern as NES/SNES battery saves)

### Phase 3: Cart Board Detection

Depends on #976 (Cart board types). When cart board type is known:
- Set `_hasSram = true` for boards with SRAM
- Set appropriate `_sramSize`
- Configure address mapping

For unknown boards, allow manual override in game settings.

### Phase 4: Save State Integration

SRAM must be included in save states:
- Serialize `_sram` buffer in `ChannelFMemoryManager::Serialize()`
- Deserialize on state load
- Ensure SRAM state is preserved across save/load cycles

## Files Modified

- `Core/ChannelF/ChannelFMemoryManager.h/cpp` — SRAM buffer, address mapping, serialize
- `Core/ChannelF/ChannelFTypes.h` — SRAM-related state fields
- `Core/ChannelF/ChannelFCartridge.h/cpp` — Cart board SRAM detection (if separate)

## Testing Strategy

- Unit test: SRAM read/write at mapped addresses
- Unit test: SRAM not accessible when `_hasSram = false`
- Unit test: Serialize/deserialize preserves SRAM contents
- Integration: Load ROM with `.sav` file, verify SRAM populated

## Risk Assessment

**Low risk** — SRAM is additive (disabled by default). Standard pattern already used by NES/SNES/GB in Nexen. No impact on games without SRAM.

## Dependencies

- #976 Cart board types (determines which boards have SRAM)
- Save state system (already exists for other platforms)
