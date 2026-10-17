# Lynx Commercial Title Validation Matrix

## Overview

This matrix tracks validation of the Nexen Lynx emulator against commercial titles. The goal is systematic testing of different cart formats, hardware features, and edge cases across the Lynx library.

Related: #1105

## Test Categories

| Category | Description |
|----------|-------------|
| Boot | Game loads and reaches title screen |
| Gameplay | Core gameplay loop functions correctly |
| Graphics | No rendering glitches, correct colors/rotation |
| Audio | Sound effects and music play correctly |
| Controls | All inputs mapped and responsive |
| Save | EEPROM save/load works correctly |
| ComLynx | Multi-player features (requires #955) |
| Cart Format | ROM loads with correct mapper/bank config |

## Validation Status Legend

- ✅ Tested and working
- ⚠️ Works with minor issues (document in notes)
- ❌ Fails or has major issues
- ❓ Not yet tested

## Priority Titles

### Tier 1 — Core Validation (test first)

| Title | Boot | Play | Gfx | Audio | Save | Cart | Notes |
|-------|------|------|-----|-------|------|------|-------|
| California Games | ❓ | ❓ | ❓ | ❓ | ❓ | Standard | Launch title, good baseline |
| Chip's Challenge | ❓ | ❓ | ❓ | ❓ | ❓ | Standard | Simple, good regression test |
| Todd's Adventures in Slime World | ❓ | ❓ | ❓ | ❓ | ❓ | Standard | ComLynx capable |
| Blue Lightning | ❓ | ❓ | ❓ | ❓ | ❓ | Standard | Scaling/rotation stress test |
| Gates of Zendocon | ❓ | ❓ | ❓ | ❓ | ❓ | Standard | Scrolling, many sprites |

### Tier 2 — Feature Coverage

| Title | Boot | Play | Gfx | Audio | Save | Cart | Notes |
|-------|------|------|-----|-------|------|------|-------|
| Lynx Casino | ❓ | ❓ | ❓ | ❓ | ❓ | EEPROM | Save game via EEPROM |
| Dirt Trax FX | ❓ | ❓ | ❓ | ❓ | ❓ | Standard | 3D rendering, heavy CPU |
| Batman Returns | ❓ | ❓ | ❓ | ❓ | ❓ | Standard | Complex sprites, scrolling |
| Lemmings | ❓ | ❓ | ❓ | ❓ | ❓ | Standard | Precise timing required |
| Rampart | ❓ | ❓ | ❓ | ❓ | ❓ | Standard | ComLynx capable |

### Tier 3 — Edge Cases

| Title | Boot | Play | Gfx | Audio | Save | Cart | Notes |
|-------|------|------|-----|-------|------|------|-------|
| Shadow of the Beast | ❓ | ❓ | ❓ | ❓ | ❓ | Standard | Parallax scrolling |
| Alien vs Predator | ❓ | ❓ | ❓ | ❓ | ❓ | Standard | Large ROM |
| Power Factor | ❓ | ❓ | ❓ | ❓ | ❓ | Standard | Rotation heavy |
| Warbirds | ❓ | ❓ | ❓ | ❓ | ❓ | Standard | 3D flight sim |
| Desert Strike | ❓ | ❓ | ❓ | ❓ | ❓ | Standard | Isometric rendering |

## Cart Format Breakdown

| Format | Description | Example Titles | Test Count |
|--------|-------------|---------------|------------|
| Standard | No special hardware, straightforward ROM | Most titles | ~40 titles |
| EEPROM | 93C46/93C66 save data | Lynx Casino, etc. | ~5 titles |
| Large ROM | >128KB requiring banking | Alien vs Predator | ~3 titles |
| ComLynx | Multi-player over serial | Todd's Slime World, Rampart | ~15 titles |

## Hardware Features to Test

| Feature | Test Method | Titles |
|---------|-------------|--------|
| Sprite collision | Use titles with collision detection | Most action games |
| Hardware scaling | Zoom/rotate sprites | Blue Lightning, Dirt Trax |
| Suzy math coprocessor | 16×16 multiply, divide | Games with 3D/scaling |
| Timer interrupts | Frame timing accuracy | All titles |
| Audio mixing | 4-channel + DAC | Music-heavy titles |
| Palette switching | Mid-screen palette changes | Shadow of the Beast |
| EEPROM persistence | Save → power off → reload | EEPROM titles |

## Rotation / Orientation

The Lynx supports physical rotation. Test these orientations:

- **Normal** (horizontal, default)
- **Left rotation** (vertical, rotated left)
- **Right rotation** (vertical, rotated right)

Test titles: Klax (vertical), Gauntlet III (horizontal), any title with rotation support.

## Execution Plan

1. **Phase 1** — Boot test all Tier 1 titles (verify they reach title screen)
2. **Phase 2** — Gameplay test Tier 1 titles (5-10 minutes each)
3. **Phase 3** — Boot + play test Tier 2 titles
4. **Phase 4** — Edge case testing with Tier 3 titles
5. **Phase 5** — EEPROM persistence and ComLynx testing (after #955)

## Notes

- Lynx BIOS required for all testing
- Keep test screenshots for regression comparison
- Log any timing-sensitive issues for future investigation
- Lynx test suite already has 22 test files — strongest platform test coverage in Nexen
