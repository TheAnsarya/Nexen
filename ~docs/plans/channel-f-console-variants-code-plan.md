# Channel F Console Variants — Code Plan (#1007)

## Overview

The Fairchild Channel F shipped in two main hardware revisions, plus several third-party clones with minor timing/behavior differences. This plan covers implementing variant-aware emulation for accurate reproduction.

## Console Variants

| Variant | Model | Clock | BIOS | Notes |
|---------|-------|-------|------|-------|
| Fairchild Channel F | Original (1976) | 1.7897725 MHz | SL31253/SL31254 | NTSC only |
| Fairchild Channel F System II | Revised (1978) | 1.7897725 MHz | SL31253/SL31254 | New case, same internals |
| Luxor Video Entertainment System | European clone | ~1.7734475 MHz | Custom/modified | PAL timing |
| Grandstand Video Entertainment Computer | UK clone | ~1.7734475 MHz | Custom/modified | PAL timing |
| SABA Videoplay / Nordmende Teleplay | German clone | ~1.7734475 MHz | Custom/modified | PAL timing |
| Isonics "Doodlecade" | US clone | 1.7897725 MHz | SL31253/SL31254 | NTSC, same as original |

## Implementation Steps

### Step 1: Add `ChannelFModel` enum to ChannelFTypes.h

```cpp
enum class ChannelFModel : uint8_t {
	ChannelF      = 0, // Original Fairchild (NTSC)
	ChannelFII    = 1, // System II (NTSC)
	LuxorVES      = 2, // Luxor (PAL)
	GrandstandVEC = 3, // Grandstand (PAL)
	SabaVideoplay = 4, // SABA/Nordmende (PAL)
	Auto          = 0xff // Auto-detect from BIOS CRC
};
```

### Step 2: Add timing constants per model

```cpp
struct ChannelFTimingProfile {
	uint32_t CpuClockHz;
	uint32_t CyclesPerFrame;
	double Fps;
	bool IsPal;
};

// NTSC: 1.7897725 MHz, ~60 fps, 29829 cycles/frame
// PAL:  1.7734475 MHz, ~50 fps, 35469 cycles/frame
```

### Step 3: Add to ChannelFConfig (settings)

Add `ChannelFModel Model = ChannelFModel::Auto;` and expose in UI.

### Step 4: Wire into ChannelFConsole initialization

On console boot:
1. If `Auto`, detect from BIOS CRC32 (NTSC vs PAL BIOS hashes)
2. Select timing profile based on model
3. Pass profile to F8 CPU and memory manager

### Step 5: Update frame timing in ChannelFMemoryManager

Replace hardcoded `CyclesPerFrame = 29829` and `Fps = 60.0` with profile values.
The `CpuClockHz` constant also changes for PAL.

### Step 6: Testing

- Unit tests: Verify timing profiles (NTSC vs PAL cycle counts)
- Unit tests: Verify auto-detection from known BIOS CRCs
- Enum tests: Verify all model values distinct

## Files Modified

- `Core/ChannelF/ChannelFTypes.h` — Add enum, timing profile
- `Core/ChannelF/ChannelFConstants.h` or inline in Types — Timing tables
- `Core/ChannelF/ChannelFConsole.h/cpp` — Model selection on boot
- `Core/ChannelF/ChannelFMemoryManager.h/cpp` — Use profile timing
- `Shared/SettingTypes.h` — Add `ChannelFModel` to config
- `UI/` — Expose model selector in Channel F settings

## Risk Assessment

**Low risk** — timing changes are self-contained within Channel F core and don't affect other systems. Test with existing Channel F test ROMs to verify no regressions.

## Dependencies

- #977 Video pipeline (interacts with frame timing)
- #979 Audio timing (interacts with clock rate)
