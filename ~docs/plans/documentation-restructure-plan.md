# Nexen Documentation Restructure Plan

## Goal

Restructure `README.md` and the docs hierarchy to focus on **how features work and how to use them** rather than describing performance improvements inline. Create a consistent per-system documentation hierarchy like the existing Atari Lynx docs.

## Current State

- README is well-organized but Key Features section describes *what* features do, not *how to use them*
- Performance details are scattered (README, CHANGELOG, PROFILING-GUIDE)
- Atari Lynx has 4 dedicated docs in `docs/` — other systems only have `~docs/` core references
- No user-facing feature guides (save states, rewind, movie system GUI walkthrough)

## Target State

### README.md Changes

1. **Key Features** — Keep concise bullet lists describing each feature
2. **Using Nexen** — New section with links to per-feature user guides
3. **System Documentation** — Unified table linking to per-system doc pages (not just Lynx)
4. **Performance** — Single link to a consolidated performance page
5. **Atari Lynx Documentation** — Remove inline section, fold into unified System Documentation table

### New User-Facing Docs (`docs/`)

| Document | Content |
|----------|---------|
| `docs/Save-States.md` | Save state system: quick save, visual picker, designated slots, auto-save, per-game org |
| `docs/Rewind.md` | Rewind feature: how it works, controls, buffer size |
| `docs/Movie-System.md` | Movie recording/playback UI walkthrough, format overview, import/export |
| `docs/Debugging.md` | Debugger overview: disassembler, memory viewer, breakpoints, trace logger, CDL |
| `docs/Video-Audio.md` | Shaders, filters, scaling, audio settings, recording |
| `docs/Performance.md` | Consolidated performance page: all optimization details from README/Changelog |

### Per-System Docs (`docs/systems/`)

Create a consistent hierarchy for each system mirroring the Lynx structure:

| System | Overview Doc | Status |
|--------|-------------|--------|
| NES/Famicom | `docs/systems/NES.md` | New |
| SNES/Super Famicom | `docs/systems/SNES.md` | New |
| Game Boy / GBC | `docs/systems/GB.md` | New |
| Game Boy Advance | `docs/systems/GBA.md` | New |
| Sega Master System / GG | `docs/systems/SMS.md` | New |
| PC Engine / TG16 | `docs/systems/PCE.md` | New |
| WonderSwan / WSC | `docs/systems/WS.md` | New |
| Atari Lynx | `docs/systems/Lynx.md` | Move from `docs/LYNX-*.md` |

Each system page includes:

- Hardware overview (CPU, PPU, audio, memory map)
- Emulation status and accuracy
- Supported mappers/hardware variants
- Known compatibility issues
- Links to detailed docs (existing `~docs/*-CORE.md`)
- System-specific features (e.g., coprocessors for SNES)

### Move Existing Lynx Docs

- `docs/LYNX-EMULATION.md` → folded into `docs/systems/Lynx.md`
- `docs/LYNX-ACCURACY.md` → linked from `docs/systems/Lynx.md`
- `docs/LYNX-TESTING.md` → linked from `docs/systems/Lynx.md`
- `docs/LYNX-HARDWARE-BUGS.md` → linked from `docs/systems/Lynx.md`
- `docs/ATARI-LYNX-FORMAT.md` → stays (file format spec, linked from Lynx page)

## Issue Structure

### Epic: Documentation Restructure

- **[Epic 15] Nexen Documentation Restructure**
  - `[15.1]` Create per-system doc pages (`docs/systems/`)
  - `[15.2]` Create feature user guides (Save States, Rewind, Movie, Debugging, Video/Audio)
  - `[15.3]` Create consolidated Performance page
  - `[15.4]` Restructure README.md (update sections, links, remove inline Lynx)
  - `[15.5]` Move Lynx docs into unified system hierarchy
  - `[15.6]` Update link-tree (README → docs/ → ~docs/)

## Implementation Order

1. Create `docs/systems/` directory and per-system pages (15.1)
2. Create feature user guides (15.2)
3. Create Performance page (15.3)
4. Move Lynx docs (15.5)
5. Restructure README (15.4)
6. Verify link-tree (15.6)
