#!/usr/bin/env python3
"""Example: snapshot a memory region, advance frames, snapshot again,
print bytes that changed.

Useful for "what is this RAM doing while X happens?" investigations.
A future memory_diff MCP tool will fold this into a single call, but
the building blocks are already in place.

    MESEN_EXE=... MESEN_ROM=... python memory_diff.py
"""
from __future__ import annotations

import argparse

from mesen_mcp import McpSession


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--addr",   type=lambda s: int(s, 0), default=0x7E0000,
                   help="start address (default: WRAM page 0)")
    p.add_argument("--length", type=lambda s: int(s, 0), default=256)
    p.add_argument("--frames", type=int, default=60,
                   help="frames to advance between snapshots")
    p.add_argument("--memtype", default="snesMemory",
                   help="Mesen memoryType (snesMemory, snesWorkRam, ...)")
    args = p.parse_args()

    with McpSession.from_env() as m:
        m.pause()
        before = m.read_memory(args.memtype, args.addr, args.length)
        m.run_frames(args.frames)
        m.pause()
        after = m.read_memory(args.memtype, args.addr, args.length)

    diffs = [(i, before[i], after[i])
             for i in range(args.length) if before[i] != after[i]]
    print(f"{len(diffs)} byte(s) changed across {args.frames} frames "
          f"(${args.addr:06X}..${args.addr + args.length - 1:06X})")
    for off, old, new in diffs[:32]:
        print(f"  ${args.addr + off:06X}  {old:02X} -> {new:02X}")
    if len(diffs) > 32:
        print(f"  ... and {len(diffs) - 32} more")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
