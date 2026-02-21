# Atari Lynx — Known Hardware Bugs

The Atari Lynx has several documented hardware bugs in the Suzy math coprocessor and sprite engine. These bugs exist in real hardware and **must be emulated** for software compatibility. This document catalogs each bug with its official reference number from the Epyx Developer's Guide.

## Math Coprocessor Bugs

### Bug 13.8 — `$8000` Treated as Positive

**Reference:** Epyx Developer's Guide, Section 13, Note 8

**Description:** The Lynx uses sign-magnitude representation (NOT two's complement) for signed math operations. The sign is determined by bit 7 of the most significant byte. For 16-bit operands, `$8000` has bit 15 set (sign bit = 1) and all magnitude bits = 0. In sign-magnitude, this represents "negative zero" which is treated the same as positive zero.

**Impact:** Any software performing signed multiplication with `$8000` as an operand will get a positive result (zero), not a negative value. Two's complement code would expect `$8000 = -32768`.

**Emulation:** Nexen correctly implements sign-magnitude handling. The sign bit is stripped, magnitude is computed, and the sign is applied only if the magnitude is non-zero.

**Test:** `LynxHardwareBugs.Bug13_8_8000IsPositive`

---

### Bug 13.9 — Division Remainder Always Positive

**Reference:** Epyx Developer's Guide, Section 13, Note 9

**Description:** When performing signed division, the quotient has the correct sign, but the remainder is always returned as a positive value regardless of the dividend's sign.

**Example:**
- `-17 / 5` → Quotient: `-3`, Remainder: `+2` (not `-2`)
- `-100 / 7` → Quotient: `-14`, Remainder: `+2`

**Impact:** Software must not rely on remainder sign matching dividend sign.

**Emulation:** Nexen takes the absolute value of the remainder after division.

**Test:** `LynxHardwareBugs.Bug13_9_RemainderAlwaysPositive`

---

### Bug 13.10 — Math Overflow Flag Overwritten

**Reference:** Epyx Developer's Guide, Section 13, Note 10

**Description:** The math status flags (overflow, carry) are overwritten on each math operation rather than being OR'd with previous values. This means that in accumulate mode, where multiple multiply results are summed, an overflow in an earlier multiply can be masked by a non-overflowing later multiply.

**Impact:** Software cannot reliably detect overflow when using accumulate mode with multiple operations.

**Emulation:** Nexen overwrites (not ORs) the status flags for each individual math operation.

**Test:** `LynxHardwareBugs.Bug13_10_OverflowOverwritten`

---

### Bug 13.12 — SCB NEXT Pointer Upper Byte Only

**Reference:** Epyx Developer's Guide, Section 13, Note 12

**Description:** When the sprite engine reads the NEXT pointer from a Sprite Control Block to find the next sprite in the linked list, it only checks the upper byte to determine end-of-chain. The chain terminates when the upper byte of the NEXT pointer is `$00`.

**Impact:** A NEXT pointer of `$0001` through `$00FF` will be treated as end-of-chain even though it points to valid RAM in zero page. SCBs cannot be placed in zero page addresses `$0000`-`$00FF`.

**Emulation:** Nexen checks `(nextAddr >> 8) == 0` for chain termination.

**Test:** `LynxHardwareBugs.Bug13_12_NextPointerUpperByte`

---

## Sign-Magnitude Math

### Not a Bug: Sign-Magnitude vs Two's Complement

**Reference:** Epyx Developer's Guide, Math Coprocessor section

**Description:** The Lynx math coprocessor uses sign-magnitude representation, not two's complement. This is a design choice, not a bug, but it catches many developers off guard.

**Key differences:**
- Sign bit (bit 15 for 16-bit) is separate from magnitude
- Two representations of zero: `+0` (`$0000`) and `-0` (`$8000`)
- Maximum negative value is `-32767` (`$FFFF`), NOT `-32768`
- Magnitude is always stored as positive, sign is applied separately

**Emulation:** All Nexen math operations strip the sign bit, compute on magnitudes, then reapply sign.

**Test:** `LynxHardwareRef.MathIsSignMagnitude`

---

## Sprite Engine Quirks

### Collision Depository Buffer

The collision detection buffer is a 160×102 byte buffer in system RAM. Each byte stores the collision number (pen index) of the last sprite to write to that pixel. Collision results are read back from registers `$FC04`-`$FC0F`.

**Quirk:** Collision numbers are 4-bit (0-15), matching the pen palette size. Non-collidable sprite types (type 1, 5) write pixels but do not update the collision buffer.

---

### Sprite Safety Limit

**Current implementation:** Nexen caps sprite processing at 256 SCBs per frame as a safety measure to prevent infinite loops if a game has a corrupted SCB chain.

**Real hardware:** No limit — the sprite engine will process SCBs until it encounters a NEXT pointer with upper byte = `$00` or until the CPU interrupts Suzy.

**Impact:** Games with more than 256 sprites per frame (theoretical, none known) would render incorrectly.

---

## Timer Quirks

### Clock Source Change

**Issue #417:** When software changes a timer's clock source (prescaler) mid-countdown via the CTLA register, the implementation may fire a burst of ticks to "catch up" to the new clock rate. Real hardware would simply switch the clock source without retroactive compensation.

**Status:** Tracked as issue #417.

---

## Display Quirks

### No PAL Variant

Unlike most game consoles of the era, the Lynx has no PAL variant. It always runs at 75 Hz with a 160×102 display. The Lynx was designed as a portable system and its display refresh is determined by the LCD panel, not a TV standard.

**Emulation note:** Nexen correctly uses 75 Hz for both NTSC and "PAL" settings when the Lynx console type is active.

---

## References

1. **Epyx Developer's Guide** — Official hardware documentation (1989)
2. **Handy Technical Reference** — K. Wilkins, comprehensive hardware specs
3. **BLL (Behind the Lynx Line) Documentation** — Community technical docs
4. **Mednafen Lynx source** — Reference implementation
5. **New Lynx Programmer's Guide** — Community-maintained technical reference
