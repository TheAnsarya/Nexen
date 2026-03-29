# FFMQ Full Roundtrip Plan

> **Goal:** Run Final Fantasy: Mystic Quest through the complete Flower Toolchain roundtrip:
> `ROM → Nexen (CDL/Pansy export) → Peony (disassemble) → source/assets → Poppy (compile) → ROM → Nexen (verify)`

## Current Status

| Component | Status | Notes |
|-----------|--------|-------|
| ROM | ✅ | CRC32 `2c52c792`, 512KB SNES |
| CDL from Nexen | ✅ | 48.7% coverage, 797 subroutines |
| Pansy export (Nexen) | ✅ | 2,928 symbols, 1,797 comments |
| Peony disassembly | ✅ | 1,826 blocks, `main.pasm` generated |
| Assets extracted | ✅ | 10 tile regions, 81 palettes, text tables, game data |
| Poppy rebuild | ❌ | Blocked by peony#41, peony#42, pansy#16 |
| Byte-identical verify | ❌ | Blocked |

## Blocking Issues

### 1. Unknown Opcodes in Disassembly — [peony#41](https://github.com/TheAnsarya/peony/issues/41)

- **Problem:** Peony decodes data-region bytes as instructions, emitting `???` for invalid opcodes (e.g., `0xff`)
- **Impact:** Poppy parser fails on `???` tokens
- **Fix:** Peony must use CDL flags to distinguish CODE vs DATA bytes. DATA bytes should emit `.db $xx` directives, not attempt instruction decode

### 2. CDL Not Used in Pansy Export — [peony#42](https://github.com/TheAnsarya/peony/issues/42)

- **Problem:** `peony export --format pansy` does not accept `--cdl` flag
- **Impact:** Exported `.pansy` files have 0 code/data offsets
- **Fix:** Wire CDL loading into the export path so code/data classification propagates into the Pansy file

### 3. Pansy Merge Not Implemented — [pansy#16](https://github.com/TheAnsarya/pansy/issues/16)

- **Problem:** The Nexen-exported `.pansy` has symbols + comments, while the Peony CDL has code/data map — no way to merge them
- **Impact:** Cannot create a single `.pansy` file with all metadata
- **Fix:** Implement `pansy merge` CLI command to combine sections from multiple `.pansy` files

## Roundtrip Pipeline — Step by Step

### Phase 1: Nexen → Pansy Export

```powershell
# 1. Load FFMQ ROM in Nexen
# 2. Play through (or load existing CDL) to generate code/data log
# 3. Export via Ctrl+Shift+P → Pansy Export
#    Output: ffmq.pansy (symbols, comments, CDL, memory regions, cross-refs)

$romPath = "C:\~reference-roms\snes\Final Fantasy - Mystic Quest (U) (V1.1).sfc"
# Manual: Open in Nexen, import CDL, play/trace, export Pansy
```

**Verification:**

```powershell
# Inspect exported Pansy file
dotnet run --project C:\Users\me\source\repos\pansy\src\Pansy.Cli -- inspect ffmq.pansy
# Should show: platform=SNES, symbols>0, code/data offsets>0, memory regions>0
```

### Phase 2: Peony Disassembly

```powershell
# Disassemble ROM using Pansy hints
dotnet run --project C:\Users\me\source\repos\peony\src\Peony.Cli -- disasm `
    "$romPath" `
    --output "C:\Users\me\source\repos\game-garden\games\snes\ffmq\src\" `
    --pansy "ffmq.pansy" `
    --format poppy
# Output: main.pasm + bank files
```

**Verification:**

- All bytes accounted for (no gaps)
- DATA regions emit `.db`/`.dw` directives, not `???`
- CODE regions have valid 65816 instructions
- Symbols applied from Pansy metadata

### Phase 3: Asset Extraction (Already Done)

The extracted assets live in `game-garden/games/snes/ffmq/assets/editable/`:

- `data/` — enemies, attacks, spells, maps (JSON/CSV)
- `graphics/` — tiles, sprites (PNG)
- `palettes/` — all game palettes (JSON)
- `text/` — encoding tables (.tbl)

No additional extraction needed unless new regions are discovered.

### Phase 4: Editing (Optional)

- Edit `.pasm` source files in `src/`
- Edit assets in `assets/editable/`
- Update metadata in `metadata/`

### Phase 5: Poppy Compile

```powershell
# Build ROM from source
cd C:\Users\me\source\repos\game-garden\games\snes\ffmq
.\build.ps1
# Or directly:
dotnet run --project C:\Users\me\source\repos\poppy\src\Poppy.CLI -- build `
    src/main.pasm `
    --output build/ffmq.sfc `
    --pansy metadata/ffmq.pansy
```

**Verification:**

```powershell
# Verify byte-identical to original
$orig = Get-FileHash "$romPath" -Algorithm SHA256
$built = Get-FileHash "build/ffmq.sfc" -Algorithm SHA256
if ($orig.Hash -eq $built.Hash) { "✅ Byte-identical!" } else { "❌ Mismatch" }
```

### Phase 6: Nexen Verification

```powershell
# Load built ROM in Nexen and verify gameplay
# 1. Open build/ffmq.sfc in Nexen
# 2. Import saved CDL/Pansy for comparison
# 3. Run automated test if available, or manual spot-check
# 4. Verify: title screen loads, overworld renders, battles work
```

## Unblocking Roadmap

### Priority Order

1. **Fix peony#41** (CRITICAL) — CDL-aware code/data classification
	- Without this, disassembly produces invalid assembly that won't compile
	- Scope: Modify `DisassemblyEngine` to check CDL flags before decoding

2. **Fix peony#42** (CRITICAL) — Wire CDL into Pansy export
	- Needed for standalone Peony-generated Pansy files
	- Scope: Add `--cdl` option to `peony export` command

3. **Fix pansy#16** (HIGH) — Pansy merge command
	- Needed to combine Nexen-exported symbols with Peony CDL
	- Scope: `pansy merge file1.pansy file2.pansy -o merged.pansy`

4. **Poppy SNES 65816 completeness** — Ensure all directives compile
	- Verify: `.db`, `.dw`, `.dl`, `.org`, `.base`, bank directives
	- Test with FFMQ-scale file

### Quick Test After Unblocking

```powershell
# After fixing peony#41 and #42:
dotnet run --project C:\Users\me\source\repos\peony\src\Peony.Cli -- disasm `
    "$romPath" --output src/ --pansy ffmq.pansy --format poppy

# Then compile:
dotnet run --project C:\Users\me\source\repos\poppy\src\Poppy.CLI -- build `
    src/main.pasm --output build/ffmq.sfc

# Verify:
python C:\Users\me\source\repos\game-garden\tools\verify-rom.py `
    --original "$romPath" --built build/ffmq.sfc
```

## ROM Reference

- **Path:** `C:\~reference-roms\snes\Final Fantasy - Mystic Quest (U) (V1.1).sfc`
- **CRC32:** `2c52c792`
- **Size:** 524,288 bytes (512 KB)
- **Platform:** SNES (65816)
- **Region:** US

## Related Issues

- [peony#41](https://github.com/TheAnsarya/peony/issues/41) — Unknown opcodes in disassembly
- [peony#42](https://github.com/TheAnsarya/peony/issues/42) — CDL not used in Pansy export
- [pansy#16](https://github.com/TheAnsarya/pansy/issues/16) — Pansy merge functionality
- [Nexen #1067](https://github.com/TheAnsarya/Nexen/issues/1067) — BOOKMARKS section export
- [Nexen #1068](https://github.com/TheAnsarya/Nexen/issues/1068) — Cross-ref performance
- [Nexen #1069](https://github.com/TheAnsarya/Nexen/issues/1069) — SOURCE_MAP section export
