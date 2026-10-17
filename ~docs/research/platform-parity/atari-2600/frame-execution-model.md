# Atari 2600 Frame Execution Model

## Objective

Define a deterministic execution loop suitable for accurate racing-the-beam behavior and reproducible debugging.

## Model Requirements

- Maintain a strict 3:1 TIA color clock to CPU cycle relationship.
- Keep TIA and RIOT updates visible at per-step boundaries.
- Preserve deterministic ordering of memory writes and delayed effects.

## Research Findings

- Gopher2600 documents an explicit CPU-driven step model that advances TIA three times per CPU cycle.
- Stella advances TIA state relative to system cycles and performs explicit emulation updates before register effects.

## Reference Evidence

- Gopher2600 step model commentary: [https://github.com/jetsetilly/gopher2600/blob/main/hardware/step.go](https://github.com/jetsetilly/gopher2600/blob/main/hardware/step.go)
- Gopher2600 run loop: [https://github.com/jetsetilly/gopher2600/blob/main/hardware/run.go](https://github.com/jetsetilly/gopher2600/blob/main/hardware/run.go)
- Stella emulation update and cycle flow: [https://github.com/stella-emu/stella/blob/main/src/emucore/tia/TIA.cxx](https://github.com/stella-emu/stella/blob/main/src/emucore/tia/TIA.cxx)

## Nexen Execution Template

1. CPU executes one cycle unit.
2. TIA advances three color-clock steps.
3. RIOT advances one CPU-cycle step.
4. Deferred side effects are resolved in deterministic order.
5. Trace hooks emit line/cycle stamps for harness comparison.
