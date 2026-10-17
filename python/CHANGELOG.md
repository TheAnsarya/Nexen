# Changelog

All notable changes to `mesen_mcp` (the Python client and stdio bridge).
Server-side tool changes ship in the
[`astrobleem/Mesen2`](https://github.com/astrobleem/Mesen2) fork; this
log tracks what's wired up here.

## Unreleased

- Added `McpSession.get_cpu_state(cpu_type="Snes")` for the new server tool
  `get_cpu_state` — live PC/A/X/Y/SP/D/DBR for the main `Snes` CPU or the `Sa1`
  coprocessor (added for SA-1 interpreter-port debugging). Tool count **46 → 47**.

## 0.1.0 — initial public release

The package becomes drop-in-portable. Targets the Mesen 2 fork's
`--mcp` mode with **46 tools** across 10 categories.

### Client surface
- `McpSession.from_env()` factory reads `MESEN_EXE` / `MESEN_ROM` /
  `MESEN_CWD` for project-portable construction.
- Bridge (`mesen-mcp-bridge` console script) configurable purely by env
  var; same defaults.
- Tool catalog (`mesen_mcp.tools` + `mesen-mcp-tools` CLI) gives offline
  agent-friendly discovery.
- AGENTS.md inside the package — agent-onboarding reference for
  downstream projects.
- Three runnable examples (`mesen_mcp.examples.*`).

### Server tools added in this release
- **state**: `run_frames` upgraded to frame-exact (was wall-clock
  approximate). New return shape includes `framesAdvanced` + `timedOut`.
- **memory**: `memory_diff` (snapshot → run → snapshot → diff).
- **screenshot**: `render_filmstrip` (N captures stitched into one PNG).
- **movies**: `record_movie` / `play_movie` / `stop_movie` /
  `movie_state` wrap Mesen's `RecordApi`.
- **ppu**: `render_palette` (CGRAM as swatch grid or strip).
- **debugging**: `symbolic_dump` (range-to-symbols), `lookup_pansy`
  (TheAnsarya/pansy v1.0 metadata).
- **audio**: `audio_fingerprint` (SHA-256 + per-second RMS),
  `audio_waveform_png` (min/max envelope render).
- **doc-only**: `render_tilemap` description clarifies that
  out-of-camera tilemap regions reading as 0 is usually a game's
  ring-buffer scroll pattern, not a tool bug.
