# Lynx Collision System: Nexen vs Handy Comparison

> **Status**: All 10 bugs identified below have been **FIXED** as of commits
> 56e199dc (#378), 63efb717 (#379), a61bb3a7 (#380), 53f9647e (#379/#381).
> This document is retained as a reference for the architecture.

## 1. Summary of Handy's Collision Implementation (Reference)

### Architecture
Handy implements the collision system using **three components**:

1. **Per-pixel collision buffer in RAM** (`mCOLLBAS`): A parallel framebuffer the same size as the video buffer (160×102/2 = 8160 bytes). Each pixel position stores a 4-bit collision number as a packed nibble, exactly like the video buffer. Address per scanline: `mCOLLBAS.Word + (voff × 80)`.

2. **Per-sprite collision accumulator** (`mCollision`): An integer tracking the **maximum** collision number found in the collision buffer across all pixels this sprite overwrites. Reset to 0 at the start of each sprite.

3. **Collision depositary**: After rendering all 4 quadrants, the accumulated `mCollision` value is written to RAM at `SCBAddr + COLLOFF` for certain sprite types. This is the byte the game reads to detect collisions.

### Per-Pixel Algorithm (in `ProcessPixel()`)
For each visible pixel of a collidable sprite:
```
1. collision = ReadCollision(hoff)   // Read existing value from collision buffer
2. if (collision > mCollision)       // Track max collision seen
      mCollision = collision
3. WriteCollision(hoff, mSPRCOLL_Number)  // Write this sprite's number to buffer
```

### Collision Buffer Read/Write
- `WriteCollision(hoff, pixel)`: Writes 4-bit value to `COLLBAS + voff*80 + hoff/2`
- `ReadCollision(hoff)`: Reads 4-bit value from same address
- Nibble layout matches video buffer (upper nibble for even x, lower for odd x)

### Sprite Type Collision Rules
| Type | Value | Draws pixel | Collision detect | Collision buffer write | Notes |
|------|-------|------------|-----------------|----------------------|-------|
| background_shadow | 0 | ALL (even 0) | No detect | Write if pixel≠0xE | No ReadCollision |
| background_noncollide | 1 | ALL (even 0) | No | No | Writes pixel only |
| boundary_shadow | 2 | Skip 0,E,F | Detect if ≠0,≠E | Write if ≠0,≠E | |
| boundary | 3 | Skip 0,F | Detect if ≠0 | Write if ≠0 | F not drawn but collidable |
| normal | 4 | Skip 0 | Detect if ≠0 | Write if ≠0 | Most common |
| noncollide | 5 | Skip 0 | No | No | |
| xor_shadow | 6 | Skip 0(XOR) | Detect if ≠0,≠E | Write if ≠0,≠E | XOR with existing |
| shadow | 7 | Skip 0 | Detect if ≠0,≠E | Write if ≠0,≠E | |

### Collision Depositary Write
Only for types: `boundary`, `normal`, `boundary_shadow`, `shadow`, `xor_shadow`.
Not written for: `background_shadow`, `background_noncollide`, `noncollide`.

### Key Flags
- `mSPRCOLL_Collide` (SPRCOLL bit 5): Per-sprite "don't collide" — checked at **every pixel**
- `mSPRSYS_NoCollide` (SPRSYS bit 5): Global collision disable — checked at **every pixel**
- `mSPRCOLL_Number` (SPRCOLL bits 3:0): 4-bit collision number written to buffer

---

## 2. Summary of Nexen's Collision Implementation

### Architecture
Nexen uses a fundamentally different approach:

1. **16-slot internal array** (`_state.CollisionBuffer[16]`): NOT a RAM-based per-pixel buffer. Just 16 bytes in the Suzy state struct.

2. **No per-pixel collision buffer**: `CollisionBase` field exists but is unused. A TODO comment confirms: "*Implement proper per-pixel collision tracking via COLLBAS buffer*".

3. **Video pixel as collision ID**: `WriteSpritePixel()` reads the **existing video pixel color** from the framebuffer and uses it as a collision identifier, which is conceptually wrong.

### Per-Pixel Algorithm (in `WriteSpritePixel()`)
```
1. existingPixel = read from VIDEO framebuffer (not collision buffer!)
2. if (existingPixel > 0 && existingPixel > _state.CollisionBuffer[collNum])
      _state.CollisionBuffer[collNum] = existingPixel     // WRONG: uses color, not collision#
3. if (existingPixel > 0 && existingPixel < 16)
      if (collNum > _state.CollisionBuffer[existingPixel])
         _state.CollisionBuffer[existingPixel] = collNum  // Mutual update (not in Handy)
```

### Collision Depositary Write
Always writes **0** to the depositary regardless of actual collision results:
```cpp
WriteRam(collDep, 0);  // Placeholder — always 0
```

### Sprite Type Handling
All sprite types go through a single `WriteSpritePixel()` with simplified logic:
- Pixel 0 filtered out before entry (`if (pixel != 0)` in ProcessSprite)
- `doCollision` based on `spriteType != NonCollidable`
- No per-type collision rules matching Handy

### Sprite Type Enum vs Hardware
| Hardware Value | Handy Name | Nexen Name | Behavior Match? |
|------|------------|------------|:---:|
| 0 | background_shadow | Background | ❌ WRONG |
| 1 | background_noncollide | Normal | ❌ WRONG |
| 2 | boundary_shadow | Boundary | ❌ WRONG |
| 3 | boundary | NormalShadow | ❌ WRONG |
| 4 | normal | BoundaryShadow | ❌ WRONG |
| 5 | noncollide | NonCollidable | ✅ Correct |
| 6 | xor_shadow | XorShadow | ⚠️ Partial |
| 7 | shadow | Shadow | ⚠️ Partial |

---

## 3. All Discrepancies Found

### BUG 1: No RAM-Based Per-Pixel Collision Buffer
**Severity: 🔴 CRITICAL**

**Handy**: Maintains a full collision buffer in RAM at `COLLBAS`, same dimensions as the video framebuffer (8160 bytes of 4-bit nibbles). Each pixel position stores the collision number of the last sprite that wrote there.

**Nexen**: Has only a 16-slot `CollisionBuffer[16]` array in state. The `CollisionBase` register value is stored but never used for any per-pixel collision tracking. The per-pixel collision buffer simply doesn't exist.

**Impact**: The entire collision system cannot function. Games that rely on pixel-level sprite overlap detection (most action/arcade games) will get no collision data.

---

### BUG 2: Collision Depositary Always Writes Zero
**Severity: 🔴 CRITICAL**

**Handy**: After rendering a sprite, writes `mCollision` (the max collision number encountered across all pixels) to RAM at `SCBAddr + COLLOFF`:
```cpp
UWORD coldep = mSCBADR.Word + mCOLLOFF.Word;
RAM_POKE(coldep, (UBYTE)mCollision);
```

**Nexen**: Always writes 0:
```cpp
WriteRam(collDep, 0);  // "currently a placeholder"
```

**Impact**: Games that read the collision depositary byte to detect sprite overlaps will always see 0 (no collision). This breaks collision detection for ALL games using hardware collision.

---

### BUG 3: Wrong Collision Algorithm — Video Pixel vs Collision Number
**Severity: 🔴 CRITICAL**

**Handy**: Reads collision numbers from a **dedicated collision buffer** in RAM. The value at each pixel is the collision number (`mSPRCOLL_Number`, 0-15) of the last sprite to render there.

**Nexen**: Reads the **existing video pixel color** from the framebuffer as if it were a collision identifier. A video pixel color (palette index) is NOT the same as a sprite collision number. A sprite with collision number 3 could draw pixel color 0x0A — Nexen would treat 0x0A as the "collider", which is meaningless.

**Impact**: Complete corruption of collision logic. Even if the depositary write were fixed, the accumulated values would be wrong.

---

### BUG 4: Sprite Type Enum Values Misnamed/Misbehaviored (Types 0-4)
**Severity: 🔴 CRITICAL**

The hardware defines 8 sprite types (SPRCTL0 bits 2:0). The values 0-7 are loaded directly from the SCB. Nexen's enum assigns **wrong names and wrong behavior** for types 0-4:

| Value | Hardware Behavior | Nexen Behavior | Bug |
|-------|------------------|----------------|-----|
| **0** | **background_shadow**: Draws ALL pixels (including 0), writes collision for ≠E, no detect | **Background**: Skips pixel 0, only draws on transparent, no collision | Draw logic completely wrong |
| **1** | **background_noncollide**: Draws ALL pixels (including 0), no collision | **Normal**: Skips pixel 0, draws normally, collision enabled | Draw direction reversed; collision wrongly enabled |
| **2** | **boundary_shadow**: Skips 0/E/F for draw, collision for ≠0/≠E | **Boundary**: Skips 0, draws all others, collision for all | Missing E/F pixel handling |
| **3** | **boundary**: Skips 0/F for draw, collision for ≠0 | **NormalShadow**: Skips 0, XOR draw, collision for all | Shadow/XOR wrongly applied; F not handled |
| **4** | **normal**: Skips 0 for draw, collision for ≠0 | **BoundaryShadow**: Skips 0, XOR draw, collision for all | Should NOT be shadow/XOR |

**Impact**: Every game using sprite types 0-4 (which is virtually all games) will have wrong sprite rendering. Background sprites won't render backgrounds. Normal sprites will XOR. Boundary sprites won't honor F/E transparency.

---

### BUG 5: Background Sprites Skip Pixel 0
**Severity: 🟠 HIGH**

**Handy**: Types 0 (background_shadow) and 1 (background_noncollide) draw ALL pixels including pixel 0. They are background fill sprites.

**Nexen**: The outer render loop filters `pixel != 0` before calling `WriteSpritePixel()`, which means pixel 0 is never drawn for any type. For background types, this is wrong — they should draw pixel 0 as a drawable color.

**Impact**: Background fill sprites won't clear/fill areas as expected. Games using background sprites for screen clearing or solid fills will have rendering artifacts.

---

### BUG 6: Missing Boundary Pixel Handling (F and E Transparency)
**Severity: 🟠 HIGH**

**Handy**: Different sprite types have specific rules for pixels 0x0E and 0x0F:
- Boundary (3): pixel 0x0F is NOT drawn but IS collidable
- Boundary_shadow (2): pixels 0x0E and 0x0F are NOT drawn; 0x0E also excludes collision
- Shadow (7), XOR_shadow (6): pixel 0x0E excludes collision

**Nexen**: No per-type pixel filtering for 0x0E or 0x0F. All non-zero pixels are treated identically for all types (except the shadow XOR).

**Impact**: Games using boundary sprites for pen-index-based masking/layering will render incorrectly. The F-as-collision-only feature is completely missing.

---

### BUG 7: SPRCOLL "Don't Collide" Flag Not Checked Per-Pixel
**Severity: 🟡 MEDIUM**

**Handy**: `mSPRCOLL_Collide` (bit 5 of SPRCOLL) is checked at **every pixel** within `ProcessPixel()`. When set, both ReadCollision and WriteCollision are skipped.

**Nexen**: `dontCollide` is only checked for the collision depositary write at the end of the sprite. The per-pixel collision logic in `WriteSpritePixel()` uses `doCollision` which is based solely on sprite type, ignoring `dontCollide` entirely.

**Impact**: A sprite with bit 5 set in SPRCOLL would still update Nexen's internal collision buffer per-pixel (if it worked), then just not write the depositary. This could cause spurious collision entries.

---

### BUG 8: Collision Depositary Written for Wrong Sprite Types
**Severity: 🟢 LOW** (masked by BUG 2)

**Handy**: Only writes depositary for 5 types: boundary, normal, boundary_shadow, shadow, xor_shadow. Does NOT write for: background_shadow, background_noncollide, noncollide.

**Nexen**: Writes depositary for ALL sprites when `!dontCollide && !NoCollide`, regardless of sprite type.

**Impact**: Currently masked by BUG 2 (always writes 0). Would become relevant once collisions are properly implemented.

---

### BUG 9: "Mutual Collision" Update Not In Hardware
**Severity: 🟡 MEDIUM** (would need to be removed if collision is reimplemented)

**Nexen** performs a mutual collision update:
```cpp
_state.CollisionBuffer[existingPixel] = collNum;  // Also update other sprite's entry
```

**Handy**: Only updates the PER-PIXEL collision buffer and the mCollision accumulator. There is no concept of "also update the other sprite's slot". The collision depositary is written once per sprite from the accumulated max.

**Impact**: This mutual update logic is an invention that doesn't match hardware. It's moot while the overall system is broken, but would need to be removed in a rewrite.

---

### BUG 10: SpriteToSpriteCollision Flag — Invented State
**Severity: 🟢 LOW**

**Nexen** maintains `_state.SpriteToSpriteCollision` as a sticky flag. This doesn't correspond to any known Lynx hardware register or behavior.

**Handy**: Has no such concept. The collision depositary and EVERON bit are the only collision outputs.

**Impact**: If exposed via SPRSYS read, could confuse games polling for status. Likely unused by games but should be removed for accuracy.

---

## 4. Severity Assessment Summary

| # | Bug | Severity | Games Affected |
|---|-----|----------|---------------|
| 1 | No RAM-based collision buffer | 🔴 CRITICAL | All games using hardware collision |
| 2 | Depositary always writes 0 | 🔴 CRITICAL | All games using hardware collision |
| 3 | Wrong collision source (video pixel vs collision buffer) | 🔴 CRITICAL | All games using hardware collision |
| 4 | Sprite types 0-4 have wrong behavior | 🔴 CRITICAL | Almost all games |
| 5 | Background sprites skip pixel 0 | 🟠 HIGH | Games using background fill |
| 6 | Missing boundary F/E pixel handling | 🟠 HIGH | Games using boundary sprites |
| 7 | SPRCOLL don't-collide not per-pixel | 🟡 MEDIUM | Games with mixed collision sprites |
| 8 | Depositary written for wrong types | 🟢 LOW | Masked by BUG 2 |
| 9 | Invented mutual collision update | 🟡 MEDIUM | Accuracy concern |
| 10 | Invented SpriteToSpriteCollision flag | 🟢 LOW | Accuracy concern |

### Root Cause Analysis
The fundamental issue is that Nexen's collision system was never fully implemented. The code has an explicit TODO: "*Implement proper per-pixel collision tracking via COLLBAS buffer*". The current implementation is a placeholder that:
1. Has no per-pixel collision buffer (should be in RAM at COLLBAS)
2. Misuses the video framebuffer as a collision source
3. Uses an invented 16-slot internal array instead of the RAM buffer
4. Always writes 0 to the collision depositary

Additionally, the sprite type enum is misnamed and the `WriteSpritePixel()` function conflates all 8 sprite types into a simplified draw model that doesn't match the hardware's per-type behavior at all.

### Recommended Fix Priority
1. **Phase 1**: Fix sprite type enum names and per-type draw/collision behavior (BUG 4, 5, 6)
2. **Phase 2**: Implement RAM-based per-pixel collision buffer using COLLBAS (BUG 1, 3)
3. **Phase 3**: Implement correct collision depositary writes (BUG 2, 7, 8)
4. **Phase 4**: Remove invented state (BUG 9, 10)
