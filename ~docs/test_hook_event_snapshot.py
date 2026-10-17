"""Regression for atomic MCP hook CPU snapshots.

Run with:
  MESEN_EXE=/path/to/Nexen MESEN_ROM=/path/to/interp.sfc \
    python3 ~docs/test_hook_event_snapshot.py
"""
from __future__ import annotations

import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "python"))
from mesen_mcp import McpSession


def main() -> None:
    exe = os.environ["MESEN_EXE"]
    rom = os.environ["MESEN_ROM"]
    with McpSession(rom=rom, mesen=exe, port=7621, boot_wait=5.0, socket_timeout=30) as m:
        m.pause()
        m.reset_emulator()
        m.pause()
        diag = m.tool("reset_diag")
        assert {"snesResets", "sa1Resets", "frameCount"} <= diag.keys(), diag
        assert all(isinstance(diag[key], int) and diag[key] >= 0 for key in ("snesResets", "sa1Resets", "frameCount")), diag
        assert diag["sa1Resets"] >= 1, diag
        handle = m.add_write_hook(0x000F, 0x000F, cpu_type="Sa1")
        result = m.run_until(max_frames=60, hook_handle=handle)
        events = [e for e in result["observedEvents"] if e["handle"] == handle]
        assert result["reason"] == "hookFired", result
        assert events, result
        event = events[0]
        required = {
            "hostPc", "hostSp", "hostP", "hostE", "hostM", "hostX",
            "hostPbr", "hostD", "hostDbr", "hostA", "hostXReg", "hostY",
            "hostCycleCount",
        }
        assert required <= event.keys(), event
        assert event["cpuType"] == "Sa1", event
        assert event["address"] == 0x000F, event
        assert event["hostPbr"] == 0, event
        assert 0x8000 <= event["hostPc"] <= 0xFFFF, event
        assert event["hostSp"] <= 0x07FF, event
        post = m.get_cpu_state("Sa1")
        assert (event["hostPc"], event["hostSp"]) != (post["pc"], post["sp"]), (event, post)
        print({"event": event, "postState": post})


if __name__ == "__main__":
    main()
