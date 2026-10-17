# mesen_mcp

Python client + stdio bridge for the [Mesen 2](https://github.com/SourMesen/Mesen2)
emulator's MCP debugger server.

This package connects MCP-aware agents (Claude Code, Cursor, Claude
Desktop) and Python scripts to a long-lived Mesen 2 instance, giving
you **46 debugger tools** over JSON-RPC: state inspection, memory
read/write/diff, screenshots and filmstrip captures, save states, movie
record/playback, PPU rendering (tilemaps, tile sheets, OAM, palette),
exec/read/write/frame hooks with async notifications, symbol resolution
(WLA-DX `.sym` and TheAnsarya/pansy v1.0), input injection, and audio
record/analysis.

The C# server lives in our
[`astrobleem/Mesen2`](https://github.com/astrobleem/Mesen2) fork
(`UI/Utilities/Mcp/`). Stock SourMesen/Mesen2 doesn't have `--mcp` mode
yet — patches not yet upstreamed.

---

## Quick start

```bash
# 1. Install the package (editable while iterating)
pip install -e tools/mesen_mcp

# 2. Tell it where Mesen + your ROM live
export MESEN_EXE=/path/to/Mesen.exe
export MESEN_ROM=/path/to/yourgame.sfc

# 3. List the tool surface (offline, no Mesen launch needed)
mesen-mcp-tools --filter palette

# 4. Run the boot-and-screenshot example
python -m mesen_mcp.examples.boot_and_screenshot
```

Or wire it into an agent's `.mcp.json`:

```json
{
  "mcpServers": {
    "mesen-inproc": {
      "command": "mesen-mcp-bridge",
      "env": {
        "MESEN_EXE": "C:/Mesen/Mesen.exe",
        "MESEN_ROM": "C:/games/yourgame.sfc"
      }
    }
  }
}
```

The agent will see `mcp__mesen-inproc__*` tools after restart.

---

## What's in the box

| Component | Purpose |
|---|---|
| `mesen_mcp.McpSession` | Python client. Spawns Mesen, manages the TCP session, exposes typed wrappers. |
| `mesen_mcp.bridge` | Stdio↔TCP bridge for stdio-transport MCP clients. Set env vars, plug in. |
| `mesen_mcp.tools` | Tool catalog + CLI. `python -m mesen_mcp.tools` to list. |
| `mesen-mcp-bridge` | Console-script entry point for the bridge. |
| `mesen-mcp-tools`  | Console-script entry point for the catalog. |
| `mesen_mcp.examples.*` | Three drop-in example scripts: boot+screenshot, hook+trace, memory_diff. |
| `AGENTS.md` | Agent-onboarding reference. Read this if you're an LLM driving the toolchain. |

---

## Configuration

Three env vars (the `bridge` requires them; the Python session prefers
explicit constructor args but falls back to env via `from_env()`):

| Env var | Default | Purpose |
|---|---|---|
| `MESEN_EXE`  | (required) | Absolute path to `Mesen.exe`. Must be a fork build with `--mcp` support. |
| `MESEN_ROM`  | (required) | Path to the ROM. |
| `MESEN_CWD`  | parent dir of `MESEN_ROM` | Mesen process working directory. Save data, MSU-1 files, screenshots resolve here. |
| `MESEN_PORT` | CRC of `MESEN_CWD` | TCP port. Deterministic per-cwd so multiple checkouts don't collide. |

---

## Tool surface (46)

| Category | Tools |
|---|---|
| **state**      | `ping`, `get_state`, `pause`, `resume`, `run_frames` (frame-exact), `reset_emulator` |
| **memory**     | `read_memory`, `write_memory`, `memory_diff`, `read_dma_state` |
| **screenshot** | `take_screenshot`, `crop_screenshot`, `render_filmstrip` |
| **savestate**  | `save_state`, `load_state`, `save_state_slot`, `load_state_slot` |
| **movies**     | `record_movie`, `play_movie`, `stop_movie`, `movie_state` |
| **ppu**        | `get_ppu_state`, `render_tilemap`, `render_tile_sheet`, `render_oam`, `render_palette` |
| **hooks**      | `add_exec_hook`, `add_read_hook`, `add_write_hook`, `add_frame_hook`, `remove_hook`, `list_hooks`, `hook_diag`, `run_until` |
| **debugging**  | `lookup_symbol`, `symbolic_dump`, `lookup_pansy`, `disassemble`, `trace_log`, `watch_addresses` |
| **input**      | `set_input` |
| **audio**      | `record_audio`, `stop_audio`, `get_audio_state`, `audio_fingerprint`, `audio_waveform_png` |

Run `mesen-mcp-tools` for one-line summaries of each, or
`mesen-mcp-tools --filter <substr>` to search.

---

## Use it from Python

```python
from mesen_mcp import McpSession

# Explicit paths
with McpSession(rom='game.sfc', mesen='/path/to/Mesen.exe') as m:
    m.pause()
    m.run_frames(600)
    pc = m.get_state()['cpuState']['pc']
    img = m.take_screenshot()
    m.add_exec_hook(0xC08000)
    events = m.drain_notifications()

# Or with env vars (MESEN_EXE / MESEN_ROM):
with McpSession.from_env() as m:
    ...
```

### Recipes

**Boot smoke test** — record-then-replay deterministic boot:

```python
with McpSession.from_env() as m:
    m.run_frames(60)
    m.save_state_slot(0)              # checkpoint
    m.record_movie("path/boot.mmo", from_="StartWithSaveData")
    m.set_input(McpSession.BTN_START, frames=2)
    m.run_frames(120)
    m.stop_movie()
```

**What changed in WRAM during a transition?**

```python
diffs = m.memory_diff(
    regions=[{"memoryType": "snesWorkRam", "address": 0, "length": 0x2000}],
    frames=60,
    max_changes=512,
)
print(f"{diffs['totalChanges']} bytes changed")
```

**Audio regression check:**

```python
m.record_audio("intro.wav")
m.run_frames(180)
m.stop_audio()
fp = m.audio_fingerprint("intro.wav")
assert fp["sha256"] == EXPECTED_HASH
```

**Resolve a memory range to symbols:**

```python
dump = m.symbolic_dump(
    sym_file="build/yourgame.sym",
    address=0x7E0010, length=64,
)
for e in dump["entries"]:
    print(f"  ${e['address']:06X}  {e['symbol']}+{e['offset']}")
```

**Pansy metadata** (richer than `.sym` — has comments + memory regions):

```python
p = m.lookup_pansy("build/yourgame.pansy", pattern="actor.*")
for s in p["symbols"]:
    print(f"  ${s['address']:06X}  [{s['typeName']}] {s['name']}")
```

See `examples/` for three full runnable scripts.

---

## Why two MCP servers in the SuperMonkeyIsland repo?

That project also keeps a project-scoped MCP server
(`tools/smi_workflow_server.py`) for one-shot workflows that don't fit
the long-lived `--mcp` model: `build_rom`, `validate_rom`, `run_test`
(Mesen `--testrunner` Lua), `visual_regression_check`. That's
project-specific glue, not part of this package — every project should
keep its own version-named workflow server.

This package only owns the **generic** Mesen 2 debugger surface.

---

## Building Mesen 2 with `--mcp` support

This package is a *client*. You need a Mesen 2 build that has `--mcp`
mode. Either:

- **Binary release**: download from
  https://github.com/astrobleem/Mesen2/releases (when tagged), or
- **Build from source**:
  ```bash
  git clone https://github.com/astrobleem/Mesen2
  cd Mesen2
  dotnet build UI/UI.csproj -c Release
  ```
  Outputs `UI/bin/Release/net*/Mesen.exe`.

---

## License

GPL-3.0-or-later, matching Mesen 2.
