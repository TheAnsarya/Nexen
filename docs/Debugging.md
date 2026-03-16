# Debugging Guide

## Overview

Nexen includes integrated debugging tools for tracing execution, inspecting memory, and validating emulator behavior.

## Primary Tools

- Disassembler: inspect instructions and labels.
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

## Related Links

- [Debugger Performance](DEBUGGER-PERFORMANCE.md)
- [Movie System Guide](Movie-System.md)
- [Systems Documentation](systems/README.md)
