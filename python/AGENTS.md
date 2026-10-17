# Agent onboarding for `mesen_mcp`

You're an agent (Claude Code, Cursor, or similar) and the project you're
working on has `mesen_mcp` installed. This file is your reference for
what's available and how to use it. Read it once at session start;
afterwards, treat it like a man page.

In this repo, this onboarding document is `python/AGENTS.md`; the package
source is `python/mesen_mcp/`.

## What this gives you

A live, paused-by-default debugger for an SNES (or any Mesen 2-supported
console) with **48 MCP tools** organised into 10 categories: state,
memory, screenshot, savestate, movies, ppu, hooks, debugging, input,
audio. The C# server runs **inside Mesen 2** (our `astrobleem/Mesen2`
fork's `--mcp` mode). This Python package is the client + stdio bridge.

You drive it from two surfaces:

1. **As MCP tools** — your agent runtime sees `mcp__mesen-inproc__*`
   tools. Use them for one-shot queries: `read_memory`, `take_screenshot`,
   `add_exec_hook`. The MCP runtime handles transport.
2. **From Python** via `from mesen_mcp import McpSession` — for scripts
   you write to the project (test rigs, smoke tests, regression checks).

The two are equivalent — same tool set, same return shapes.

## First call protocol

**Always start by listing the surface and pausing the emulator.** Run
`mesen-mcp-tools` before each session for discovery, then pause so reads
are coherent.

```python
from mesen_mcp import McpSession
with McpSession.from_env() as m:
    m.pause()                                  # required for race-free reads
    state = m.get_state()
    print(state)                                # isRunning, isPaused, frameCount
```

Or as MCP tool calls:
1. `mcp__mesen-inproc__pause`
2. `mcp__mesen-inproc__get_state`

### Seeding SA-1 architectural state

Use `set_cpu_state` while paused to write selected registers without disturbing
the fields omitted from the request. It accepts `cpuType="Sa1"` and returns
the written PC; follow with `get_cpu_state("Sa1")` when the full state must be
authenticated.

```python
m.pause()
before = m.get_cpu_state("Sa1")
m.set_cpu_state("Sa1", pc=0x80F8, k=0, d=0, sp=before["sp"], ps=before["ps"])
after = m.get_cpu_state("Sa1")
```

The equivalent MCP call is `set_cpu_state` with arguments such as
`{"cpuType":"Sa1","pc":33016,"k":0,"d":0}`. This is a register write,
not a ROM redirect; do not treat the write alone as proof that the target
instruction executed.

## Discovering what's available

```bash
python -m mesen_mcp.tools                 # full categorised listing
python -m mesen_mcp.tools --names         # just names, pipeable
python -m mesen_mcp.tools --filter hook   # substring search
python -m mesen_mcp.tools --category ppu  # one category
```

Or programmatically:

```python
from mesen_mcp.tools import TOOLS, CATEGORIES, CATEGORY_BLURBS
TOOLS["render_palette"]["summary"]         # one-line description
CATEGORIES["debugging"]                     # ["lookup_symbol", "symbolic_dump", ...]
```

Schemas for arguments live in the C# server; calling a tool with bad
args returns a structured `McpException` you can read.

## Common workflows

### "What is the game doing right now?"

```python
m.pause()
state    = m.get_state()
ppu      = m.get_ppu_state()
shot     = m.take_screenshot()
print(f"frame {state['frameCount']}, room {m.read_u8(0x7EF967):02x}")
print(f"screen at {shot['path']}")
```

### "What changes between frame N and frame N+60?"

```python
diffs = m.memory_diff(
    regions=[{"memoryType": "snesWorkRam", "address": 0x0, "length": 0x2000}],
    frames=60,
)
for r in diffs["regions"]:
    print(f"{r['memoryType']} {r['changedCount']} bytes changed")
```

### "Capture an animation cycle"

```python
m.pause()
strip = m.render_filmstrip(count=8, frame_step=4, columns=4, label=True)
print(strip["path"])
```

### "Watch this CPU address while the boot runs"

```python
handle = m.add_exec_hook(0xC08000)            # NMI handler
m.run_frames(120)                              # advance 120 frames
events = m.drain_notifications(timeout=0.5)
print(f"got {len(events)} hits")
m.remove_hook(handle)
```

For high-volume PCs, filter at the server with `match_value` + value mask
so you don't drown the socket.

### "Replay the same boot path from a movie"

```python
m.reset_emulator()
m.play_movie("smoke_boot.mmo")
m.run_frames(600)                              # advances inputs from the movie
m.stop_movie()
shot = m.take_screenshot()                     # check we're at the right room
```

### "Audio regression"

```python
m.record_audio("intro_music.wav")
m.run_frames(300)
m.stop_audio()

fp = m.audio_fingerprint("intro_music.wav")
assert fp["sha256"] == EXPECTED_SHA, "audio changed"

png = m.audio_waveform_png("intro_music.wav")  # human-readable visual
```

### "Resolve $7E0010..$7E0030 to symbols"

```python
dump = m.symbolic_dump(
    sym_file="build/yourgame.sym",
    address=0x7E0010, length=32, unit="byte",
)
for e in dump["entries"]:
    print(f"  ${e['address']:06X}  {e['symbol']}+{e['offset']}")
```

### "Use Pansy metadata if the project ships it"

```python
p = m.lookup_pansy("build/yourgame.pansy", pattern="actor.*")
print(f"matched {len(p['symbols'])} symbols, {len(p['comments'])} comments")
print(f"{p['memoryRegionCount']} memory regions on file")
```

## Pitfalls (read once, save yourself an hour)

- **Pause before multi-call inspection.** Without `pause()`, two
  consecutive `read_memory` calls see different frames. Race city.
- **`run_frames(N)` is now frame-exact** (was wall-clock approximate
  before #24). The return shape includes `framesAdvanced` and
  `timedOut`. Trust those over `requested`.
- **`get_state()['frameCount']` is the ground truth** for "what frame
  am I on?" — wall-clock counters lie under max-speed mode.
- **Drain stderr.** Mesen's `[CPU]` warnings can fill the stderr pipe
  and stall the emulator thread. The bridge handles this for you, but
  if you're spawning Mesen yourself outside `McpSession`, drain it.
- **Filter hot hooks.** Putting an exec hook on a per-frame loop entry
  point can fire 1000+ times/frame. Use `match_value`/`match_value_mask`
  or `add_frame_hook(every_n=N)` instead.
- **Symbol/Pansy lookups cache by mtime.** Edit + rebuild your ROM and
  the next call sees the new symbols automatically; no `clear_cache`
  needed.
- **WRAM reads via `snesMemory` work for $7E/$7F directly.** The server
  auto-routes to `snesWorkRam` to dodge the SA-1 Peek bug.
  `read_u8(0x7EF967)` Just Works.

## Configuration

The Python session and bridge both read three env vars:

| Env var      | Purpose                                            |
|--------------|----------------------------------------------------|
| `MESEN_EXE`  | Absolute path to `Mesen.exe` (REQUIRED)            |
| `MESEN_ROM`  | Path to the ROM file (REQUIRED)                    |
| `MESEN_CWD`  | Mesen working directory (default: ROM's parent)    |
| `MESEN_PORT` | TCP port (default: deterministic per-cwd CRC)      |

Set them once in your project's `.envrc` / `Makefile` / `.mcp.json`
`env:` block and forget about them.

## Fresh Windows build checklist

For a fresh `astrobleem/Mesen2` checkout, build both the native core and
the managed UI:

```powershell
msbuild Mesen.sln /p:Configuration=Release /p:Platform=x64 /m
dotnet build UI/UI.csproj -c Release -p:SolutionDir="$PWD\"
python -m pip install -e python
```

`dotnet build UI/UI.csproj -c Release` by itself is not enough when the
native `MesenCore.dll` has not been built yet. Also pass `SolutionDir`
when building `UI/UI.csproj` directly; otherwise output can land in a
drive-level `E:\bin\...` folder instead of this repo's
`bin\win-x64\Release\`.

Point `MESEN_EXE` at the folder containing a matched `Mesen.exe`,
`Mesen.dll`, and `MesenCore.dll`. Stale publish folders can launch and
then time out without opening the MCP port.

## Wiring into a fresh project

Install the package from the fork with `python -m pip install -e
path/to/Mesen2/python`. Add to the project's `.mcp.json`:

```json
{
  "mcpServers": {
    "mesen-inproc": {
      "command": "mesen-mcp-bridge",
      "env": {
        "MESEN_EXE": "C:/Mesen2/bin/win-x64/Release/Mesen.exe",
        "MESEN_ROM": "C:/games/yourgame.sfc",
        "MESEN_CWD": "C:/games/your-project"
      }
    }
  }
}
```

Restart the agent runtime. You should see `mcp__mesen-inproc__*` tools
in the namespace.

For Python-side use, drop a `tools/mcp_client.py` shim or just `from
mesen_mcp import McpSession` directly.

## When something breaks

- **"connect timed out"**: Mesen didn't open the TCP listener in time.
  Make sure `Mesen.exe` is the `astrobleem/Mesen2` fork build, not stock
  Mesen - only the fork has `--mcp` mode. Also make sure the sibling
  `Mesen.dll` includes `McpRunner` and the sibling `MesenCore.dll` exports
  `McpDrainEvents`; rebuild native `Release|x64` plus UI if they do not.
- **"sym file not found"**: pass the absolute path. Relative paths
  resolve against Mesen's cwd, which is `MESEN_CWD` (= ROM dir by
  default), not your project root.
- **Hook never fires**: check `hook_diag()` to confirm the hot-path
  is alive. If `totalCalls > 0` but `totalMatches == 0`, your address
  range or value mask is wrong.
- **Tool returns `null` or empty**: check the C# log file at
  `mcp_server.log` in `ConfigManager.HomeFolder`. Tool exceptions land
  there with the full call stack.

## Versioning + compatibility

This package follows Mesen 2 fork releases. Tagged releases live at
https://github.com/astrobleem/Mesen2/releases. The Python package
version (`mesen_mcp.__version__`) tracks the C# server's tool surface
shape; minor bumps may add tools but won't change argument names of
existing ones.
