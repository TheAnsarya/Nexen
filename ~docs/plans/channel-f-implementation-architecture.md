# Channel F Implementation Architecture

## Goals

- Implement a correctness-first Channel F core that matches Nexen subsystem patterns.
- Keep hot paths efficient without sacrificing timing behavior.
- Enable debugger/TAS/movie/persistence integrations from day one of core design.

## Proposed Core Layout

- `Core/ChannelF/ChannelFConsole.*`
- `Core/ChannelF/ChannelFCpuF8.*`
- `Core/ChannelF/ChannelFMemoryBus.*`
- `Core/ChannelF/ChannelFVideo.*`
- `Core/ChannelF/ChannelFAudio.*`
- `Core/ChannelF/ChannelFInput.*`
- `Core/ChannelF/ChannelFCart.*`
- `Core/ChannelF/ChannelFBiosDb.*`

## Execution Model

- Master tick loop coordinates CPU, video scanline timing, and audio cadence.
- CPU cycle accounting is explicit and consumed by subcomponents.
- Console variant profile selects clocks/quirks at initialization.

## CPU/F8 Strategy

- Table-driven opcode decode with fixed metadata entries.
- Separate decode metadata from execution handlers to aid debugger and disassembler reuse.
- Explicit register struct for A/W/IS/PC/DC and flags.
- Bus access abstraction mediates ROMC-like behavior and external counter interactions.

## Memory and Cartridge Model

- BIOS region mapping with strict hash checks.
- Cartridge type abstraction for standard and RAM-extended boards.
- Serialize cartridge RAM where applicable.
- Expose deterministic memory snapshots for save states and movie checkpoints.

## Video Model

- Model scanline progression and visible window behavior.
- Implement palette behavior and line-level constraints.
- Add frame hash helper for differential testing.

## Audio Model

- Implement baseline tone generator behavior with variant-specific routing differences.
- Maintain event timing tied to emulation clock.

## Input Model

- Dual controllers with directional, twist, pull, and push actions.
- Front-panel TIME/HOLD/MODE/START actions as platform events.
- Mapping layer should expose both digital bindings and gesture-friendly defaults.

## Debugger and Tooling Contracts

- CPU step/trace uses same opcode metadata table as execution path.
- Disassembler integrated with code/data maps and symbol labeling.
- Pansy export receives code/data + symbol + cross-ref streams from Channel F core.

## Save States and Determinism

- Serialize all mutable core state including timing counters and queues.
- Replay determinism validated by frame hash and movie re-sim checks.

## Performance Plan

- Add focused microbenchmarks for opcode dispatch, frame composition, and debugger overhead.
- Measure memory allocations and enforce zero-allocation hot paths where feasible.
- Mark likely/unlikely branches only after profiling confirms branch bias.
