#!/usr/bin/env python3
"""Example: install an exec hook, run frames, collect hits.

Hooks fire asynchronously on the emulator thread and arrive as MCP
notifications. drain_notifications() pulls them off the wire without
blocking.

Pick an address that's actually executed by your ROM. The default
(0xC08000) is the NMI vector start in HiROM SNES carts — it'll fire
once per frame.

    MESEN_EXE=... MESEN_ROM=... python hook_and_trace.py
"""
from __future__ import annotations

import argparse

from mesen_mcp import McpSession


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--addr", type=lambda s: int(s, 0), default=0xC08000,
                   help="exec hook address (default: HiROM NMI vector)")
    p.add_argument("--frames", type=int, default=120,
                   help="frames to run while collecting hits")
    args = p.parse_args()

    with McpSession.from_env() as m:
        handle = m.add_exec_hook(args.addr)
        print(f"installed exec hook at ${args.addr:06X} (handle {handle})")
        m.run_frames(args.frames)
        events = m.drain_notifications(timeout=0.5)
        print(f"got {len(events)} hits in {args.frames} frames")
        for e in events[:8]:
            params = e.get("params", {})
            print(f"  frame={params.get('frame')} pc=${params.get('address',0):06X}")
        m.remove_hook(handle)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
