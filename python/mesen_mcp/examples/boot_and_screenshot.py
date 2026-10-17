#!/usr/bin/env python3
"""Example: boot a ROM, run 600 frames, save a screenshot.

Set MESEN_EXE and MESEN_ROM env vars before running, e.g.:

    MESEN_EXE=/path/to/Mesen.exe \\
    MESEN_ROM=/path/to/game.sfc \\
        python boot_and_screenshot.py
"""
from __future__ import annotations

from mesen_mcp import McpSession


def main() -> int:
    with McpSession.from_env() as m:
        m.run_frames(600)
        shot = m.take_screenshot()
        print(f"saved: {shot['path']}  ({shot['width']}x{shot['height']}, {shot['unique_colors']} unique colors)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
