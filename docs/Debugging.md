# Debugging Guide

## Overview

Nexen includes integrated debugging tools for tracing execution, inspecting memory, and validating emulator behavior.

## Primary Tools

- Debugger (disassembly view): inspect instructions and labels.
- Memory Viewer: inspect and edit memory values.
- Breakpoints: execution, read, and write stops.
- Trace Logger: export execution logs for analysis.
- Code/Data Logger: inspect ROM usage patterns.

## Typical Debug Session

1. Load ROM and open debugger windows.
2. Set breakpoints near target code.
3. Run until break.
4. Inspect registers, memory, and call flow.
5. Use trace logging for deeper sequence analysis.

## GUI Tips

- Keep memory and disassembly windows side-by-side.
- Use breakpoints and rewind together for fast iteration.
- Save states before risky edits or long trace sessions.

## Atari 2600 Debugger UI Checklist

Use this focused checklist when validating Atari 2600 debugger readiness:

1. Breakpoint precision: execution breakpoints stop exactly at expected addresses.
2. Disassembly stepping: single-step updates PC and instruction context correctly.
3. Memory parity: TIA and RIOT register writes are reflected in memory viewer.
4. Event parity: event viewer entries match observed memory and execution transitions.
5. Trace parity: trace logger ordering and tags are stable across repeated runs.

For full manual execution flow and ranked release priority, use:

- [Manual Testing Hub](manual-testing/README.md)
- [Atari 2600 Debug and TAS Manual Test](manual-testing/atari2600-debug-and-tas-manual-test.md)
- [Manual Step Priority Ranking](manual-testing/manual-step-priority-ranking.md)

## Panel Walkthrough (Screenshot Anchors)

### Walkthrough A: Breakpoint and Disassembly Inspection

1. Open Debugger and switch to the disassembly view. Expected result: disassembly and CPU execution context are visible.
2. Add an execution breakpoint at target code. Expected result: the new breakpoint appears in the breakpoint list.
3. Run until breakpoint is hit. Expected result: execution pauses exactly at the configured breakpoint address.
4. Inspect current instruction context and labels. Expected result: opcode, address, and symbol context are available for analysis.

| Step | Panel | Screenshot Anchor ID | Capture Target |
|---|---|---|---|
| 1 | Debugger (Disassembly View Initial) | `debug-a-01-disassembler-initial` | `docs/screenshots/debugging/a-01-disassembler-initial.png` |
| 2 | Breakpoint Configuration | `debug-a-02-breakpoint-config` | `docs/screenshots/debugging/a-02-breakpoint-config.png` |
| 3 | Debugger (Disassembly View, Breakpoint Hit) | `debug-a-03-breakpoint-hit` | `docs/screenshots/debugging/a-03-breakpoint-hit.png` |

### Walkthrough B: Memory and Trace Correlation

1. Open Memory Viewer and navigate to target address range. Expected result: the requested address range is visible and updating.
2. Open Trace Logger and start capture. Expected result: trace logging starts and capture state is active.
3. Execute scenario and stop trace capture. Expected result: trace output includes the executed sequence for the scenario window.
4. Compare trace events with observed memory transitions. Expected result: memory changes can be mapped to corresponding trace events.

| Step | Panel | Screenshot Anchor ID | Capture Target |
|---|---|---|---|
| 1 | Memory Viewer (Target Region) | `debug-b-01-memory-region` | `docs/screenshots/debugging/b-01-memory-region.png` |
| 2 | Trace Logger (Capture Active) | `debug-b-02-trace-active` | `docs/screenshots/debugging/b-02-trace-active.png` |
| 3 | Trace and Memory Side-by-Side | `debug-b-03-trace-memory-correlation` | `docs/screenshots/debugging/b-03-trace-memory-correlation.png` |

## Related Links

- [Debugger Performance](DEBUGGER-PERFORMANCE.md)
- [Movie System Guide](Movie-System.md)
- [Manual Testing Hub](manual-testing/README.md)
- [Systems Documentation](systems/README.md)
