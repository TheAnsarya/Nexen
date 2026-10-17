"""Static catalog of the Mesen 2 MCP tool surface.

Mirrors `Mesen2/UI/Utilities/Mcp/McpTools.cs:Descriptions`. Kept in sync
by hand (the surface only changes a few times per release). Agents
exploring the package can introspect via:

    python -m mesen_mcp.tools                      # categorized listing
    python -m mesen_mcp.tools --names              # tool names only
    python -m mesen_mcp.tools --filter state       # tools matching substring

Use the `CATEGORIES` and `TOOLS` dicts from Python:

    from mesen_mcp.tools import TOOLS, CATEGORIES
    print(TOOLS["render_palette"]["summary"])
"""
from __future__ import annotations

import argparse
import sys

# Tool catalog. Keys mirror the JSON-RPC tool name. Each value is a 2-tuple
# (category, summary). The summary is a one-line agent-facing description
# pulled from the C# tool descriptions in McpTools.cs — keep them tight.
_RAW: list[tuple[str, str, str]] = [
    # category, name, summary
    ("state",      "ping",                "Echo back. Verify the MCP session is alive."),
    ("state",      "get_state",           "Snapshot emulator state (isRunning, isPaused, frameCount)."),
    ("state",      "get_cpu_state",       "CPU registers (PC/A/X/Y/SP/D/DBR) for the main 'Snes' CPU or the 'Sa1' coprocessor."),
    ("state",      "set_cpu_state",       "Write selected architectural registers for the main 'Snes' CPU or the 'Sa1' coprocessor; pause first for deterministic seeding."),
    ("state",      "pause",               "Pause emulation. Required for race-free multi-call reads."),
    ("state",      "resume",              "Resume emulation at full speed."),
    ("state",      "run_frames",          "Advance N frames deterministically (frame-counter poll, not wall-clock)."),
    ("state",      "reset_emulator",      "Soft-reset (state wiped, ROM stays loaded)."),

    ("memory",     "read_memory",         "Read N hex-encoded bytes from a memory region (memoryType + address)."),
    ("memory",     "write_memory",        "Write hex-encoded bytes to a memory region."),
    ("memory",     "memory_diff",         "Snapshot regions, advance N frames, snapshot, return changed bytes."),
    ("memory",     "read_dma_state",      "Snapshot the 8 SNES DMA channels' control/source/dest/count."),

    ("screenshot", "take_screenshot",     "Capture current PPU frame as PNG. Returns path or base64."),
    ("screenshot", "crop_screenshot",     "Capture + crop to (x,y,w,h). Returns path or base64."),
    ("screenshot", "render_filmstrip",    "N screenshots M frames apart, stitched into one PNG with optional frame labels."),

    ("savestate",  "save_state",          "Save emulator state to a named file path."),
    ("savestate",  "load_state",          "Load emulator state from a named file path."),
    ("savestate",  "save_state_slot",     "Save to numbered slot (0..9)."),
    ("savestate",  "load_state_slot",     "Load from numbered slot."),

    ("movies",     "record_movie",        "Start recording a Mesen movie (.mmo). 'from'=CurrentState/StartWithSaveData/StartWithoutSaveData."),
    ("movies",     "play_movie",          "Play back a Mesen movie (.mmo). Errors if another movie is active."),
    ("movies",     "stop_movie",          "Stop the active movie. No-op if none active."),
    ("movies",     "movie_state",         "Return whether a movie is currently recording or playing."),

    ("ppu",        "get_ppu_state",       "PPU register snapshot (forced-blank, brightness, BG mode, layer enables, scroll, window)."),
    ("ppu",        "render_tilemap",      "Render a BG layer's full tilemap as PNG using live PPU config."),
    ("ppu",        "render_tile_sheet",   "Render a VRAM tile region as a sheet using chosen palette + bpp."),
    ("ppu",        "render_oam",          "Render OAM as PNG. mode='positioned' (current screen layout) or 'sheet'."),
    ("ppu",        "render_palette",      "Render CGRAM as a 16x16 swatch grid or 1x256 strip; optional highlight outline."),

    ("hooks",      "add_exec_hook",       "Fire on CPU exec at address (or [start..end]). Returns handle. Optional value-mask filter."),
    ("hooks",      "add_read_hook",       "Fire on CPU memory reads. Notification carries the byte read."),
    ("hooks",      "add_write_hook",      "Fire on CPU memory writes. Notification carries the byte written."),
    ("hooks",      "add_frame_hook",      "Fire once per frame (or every N). Replaces per-frame Lua poll loops."),
    ("hooks",      "remove_hook",         "Detach a hook by handle."),
    ("hooks",      "list_hooks",          "List currently-registered hooks."),
    ("hooks",      "hook_diag",           "Diagnostic counters for the MCP hook hot-path."),
    ("hooks",      "run_until",           "Resume until N frames pass OR a hook fires. Atomic pause/resume/wait."),

    ("debugging",  "lookup_symbol",       "Resolve symbol names from a WLA-DX .sym file by regex. Cached + mtime-checked."),
    ("debugging",  "symbolic_dump",       "Resolve every byte/word/long in a memory range to its nearest .sym symbol + offset."),
    ("debugging",  "lookup_pansy",        "Open a TheAnsarya/pansy v1.0 metadata file. Returns SYMBOLS, COMMENTS, MEMORY_REGIONS."),
    ("debugging",  "disassemble",         "Disassemble N instructions starting at address."),
    ("debugging",  "trace_log",           "Return last N executed instructions with PC, opcode, disasm, registers."),
    ("debugging",  "watch_addresses",     "Watch a list of WRAM addresses for changes over N frames; returns timeline."),

    ("input",      "set_input",           "Inject controller buttons (bitmask) for the next N frames."),

    ("audio",      "record_audio",        "Start recording audio to a WAV file."),
    ("audio",      "stop_audio",          "Stop the active audio recording."),
    ("audio",      "get_audio_state",     "SPC700 + S-DSP register snapshot (per-voice volume, pitch, ADSR, ...)."),
    ("audio",      "audio_fingerprint",   "SHA-256 + per-second RMS levels of a recorded WAV (for regression testing)."),
    ("audio",      "audio_waveform_png",  "Render a WAV's amplitude envelope to PNG via min/max-per-column bucketing."),
]

TOOLS: dict[str, dict[str, str]] = {
    name: {"category": cat, "summary": summary}
    for cat, name, summary in _RAW
}

CATEGORIES: dict[str, list[str]] = {}
for cat, name, _ in _RAW:
    CATEGORIES.setdefault(cat, []).append(name)

CATEGORY_BLURBS: dict[str, str] = {
    "state":      "Lifecycle: pause/resume, frame stepping, reset.",
    "memory":     "Read, write, diff. Pair with `pause` for race-free reads.",
    "screenshot": "PNG capture: full frame, crop, multi-frame filmstrip.",
    "savestate":  "Snapshot the entire emulator state to disk for fast replay.",
    "movies":     "Mesen native .mmo input recording + playback.",
    "ppu":        "PPU register reads + tilemap/sprite/palette renders.",
    "hooks":      "Async exec/read/write/frame notifications + value-mask filters.",
    "debugging":  "Symbol resolution (WLA-DX .sym + Pansy), disasm, trace, watch.",
    "input":      "Button bitmask injection for the next N frames.",
    "audio":      "WAV record + analysis (fingerprint, waveform PNG, S-DSP state).",
}


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(
        prog="python -m mesen_mcp.tools",
        description="List the Mesen 2 MCP tool surface (47 tools across 10 categories).",
    )
    p.add_argument("--names", action="store_true",
                   help="Print just the tool names, one per line. Pipeable.")
    p.add_argument("--filter", metavar="SUBSTR",
                   help="Show only tools whose name or summary contains SUBSTR (case-insensitive).")
    p.add_argument("--category", metavar="CAT",
                   help="Show only tools in the given category.")
    args = p.parse_args(argv)

    needle = args.filter.lower() if args.filter else None
    cat_filter = args.category.lower() if args.category else None

    if args.names:
        for name, info in sorted(TOOLS.items()):
            if cat_filter and info["category"] != cat_filter:
                continue
            if needle and needle not in name.lower() and needle not in info["summary"].lower():
                continue
            print(name)
        return 0

    print(f"mesen_mcp tool surface ({len(TOOLS)} tools)")
    print("=" * 60)
    for cat, names in CATEGORIES.items():
        if cat_filter and cat != cat_filter:
            continue
        # Filter by needle if any entries match.
        if needle:
            kept = [n for n in names
                    if needle in n.lower() or needle in TOOLS[n]["summary"].lower()]
            if not kept:
                continue
            names = kept
        blurb = CATEGORY_BLURBS.get(cat, "")
        print()
        print(f"[{cat}]  {blurb}")
        for n in names:
            print(f"  {n:24}  {TOOLS[n]['summary']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
