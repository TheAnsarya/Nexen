# Atari 2600 Bankswitching and Cartridge Formats

## Why This Is A Separate Workstream

Bankswitching behavior differs across cartridge families and can invalidate otherwise-correct CPU/TIA/RIOT implementations.

## Initial Scope

- Start with a minimal, high-value mapper subset.
- Add mapper families in measured phases with regression ROM coverage.

## Research Findings

- Gopher2600 exposes mapper-specific implementations and documents cartridge format behavior in source comments.
- Stella historical notes confirm long evolution around bankswitch support and compatibility edge cases.

## Reference Evidence

- Gopher2600 mapper examples: [https://github.com/jetsetilly/gopher2600/tree/main/hardware/memory/cartridge](https://github.com/jetsetilly/gopher2600/tree/main/hardware/memory/cartridge)
- Example mapper commentary (-3F): [https://github.com/jetsetilly/gopher2600/blob/main/hardware/memory/cartridge/mapper_tigervision.go](https://github.com/jetsetilly/gopher2600/blob/main/hardware/memory/cartridge/mapper_tigervision.go)
- Stella historical compatibility notes: [https://github.com/stella-emu/stella/blob/main/Changes.txt](https://github.com/stella-emu/stella/blob/main/Changes.txt)

## Suggested Nexen Phase Order

1. Baseline fixed ROM mapping.
2. Most common mapper families needed for test corpus.
3. Extended and niche mapper families with dedicated regression sets.
