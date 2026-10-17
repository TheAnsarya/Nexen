# Lynx Cart Shift Register Addressing Audit (2026-03-30)

## Scope

This research audits Lynx cartridge shift-register addressing behavior in Nexen, focused on the concerns raised in #956.

## Code Paths Audited

| File | Area | Notes |
|---|---|---|
| `Core/Lynx/LynxCart.cpp` | Cart addressing and bank/page behavior | Address-bit computation, shift-in sequencing, bank/page selection, ROM address mapping |
| `Core/Lynx/LynxMikey.cpp` | SYSCTL1 strobe integration | Falling-edge strobe feeds address bits into cart logic |
| `Core/Lynx/LynxSuzy.cpp` | RCART0/RCART1 read behavior | Bank selection and sequential cart reads |
| `Core.Tests/Lynx/LynxCartTests.cpp` | Existing cart test coverage | Header parsing, state fields, page/high-byte behavior, basic regressions |

## Findings

1. Shift-register address ingestion is implemented in `LynxCart::CartAddressWrite(bool data)` and commits to `AddressCounter` when `ShiftCount >= _addrBitCount`.
2. `_addrBitCount` is derived from `ceil(log2(max(bank0Size, bank1Size)))` with a floor of 8 bits.
3. ROM lookup is currently page/bank driven and wraps with per-bank masks in `GetCurrentRomAddress()`.
4. `LynxCartTests` currently validate structure and state behavior, but not a full commercial-title differential matrix for unusual bank configurations.

## Risk Assessment

| Risk | Current Status | Impact |
|---|---|---|
| Address-bit count mismatch on uncommon carts | Not proven with title corpus | Medium |
| Header-declared bank sizes exceeding ROM | Clamped/adjusted in implementation | Low |
| Bank/page edge behavior regressions | Partial regression coverage exists | Medium |

## Conclusion

The implementation is coherent and internally consistent for the documented protocol path, but the issue's title-corpus requirement is not fully satisfied by current automated coverage alone.

## Follow-Up Work

- Track commercial-title execution validation in a dedicated follow-up issue.
- Add table-driven test vectors for:
	- non-power-of-two inferred edge conditions,
	- bank-size asymmetry,
	- strobe bit-count boundary transitions.

## Closure Recommendation for #956

Treat #956 as completed research/audit with actionable findings and move empirical title-matrix execution to follow-up implementation issue(s).
