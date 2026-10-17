# Atari 2600 RIOT 6532 Timer and I/O

## Scope

RIOT is responsible for timer and input/output behavior and must remain in lockstep with CPU and TIA state transitions.

## Functional Areas

- Timer decrement, wrap, and read behavior.
- Port A and Port B direction/data semantics.
- VBLANK-related interactions with input pathways.

## Research Findings

- Stella explicitly documents RIOT IRQ-related limitations due to 6507 constraints.
- Gopher2600 separates RIOT timer and ports while preserving per-cycle stepping and change propagation.

## Reference Evidence

- Stella RIOT model and commentary: [https://github.com/stella-emu/stella/blob/main/src/emucore/M6532.hxx](https://github.com/stella-emu/stella/blob/main/src/emucore/M6532.hxx)
- Stella RIOT debugger signals: [https://github.com/stella-emu/stella/blob/main/src/debugger/RiotDebug.cxx](https://github.com/stella-emu/stella/blob/main/src/debugger/RiotDebug.cxx)
- Gopher2600 RIOT top-level flow: [https://github.com/jetsetilly/gopher2600/blob/main/hardware/riot/riot.go](https://github.com/jetsetilly/gopher2600/blob/main/hardware/riot/riot.go)
- Gopher2600 RIOT ports model: [https://github.com/jetsetilly/gopher2600/blob/main/hardware/riot/ports/ports.go](https://github.com/jetsetilly/gopher2600/blob/main/hardware/riot/ports/ports.go)

## Nexen Execution Guidance

1. Implement RIOT timer first with deterministic cycle stepping.
2. Add port direction and latch semantics second.
3. Integrate with TIA/CPU only after independent RIOT checks pass.
