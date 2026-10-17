"""High-level Python client for Mesen 2's MCP server.

Usage:
    from mesen_mcp import McpSession
    with McpSession(rom='path/to/game.sfc', mesen='path/to/Mesen.exe') as m:
        m.pause()
        m.run_frames(500)
        b = m.read_u8(0x7E0010)
        m.take_screenshot()

Or read all paths from environment:

    with McpSession.from_env() as m:
        ...

Reads MESEN_EXE, MESEN_ROM, MESEN_CWD. Raises McpError if a required
env var is missing.
"""
from __future__ import annotations

import json
import os
import socket
import subprocess
import threading
import time
from pathlib import Path
from typing import Any

from .validate import MesenBuildError, validate_mesen_build


class McpError(RuntimeError):
    pass


class McpSession:
    """Spawn Mesen --mcp, connect, expose a typed tool API.

    The session is the connection lifecycle (spawn → connect → use →
    shutdown) wrapped as a context manager. Every method call goes
    over the wire as a JSON-RPC tools/call request.
    """

    def __init__(
        self,
        rom: Path | str,
        mesen: Path | str,
        cwd: Path | str | None = None,
        port: int = 7350,
        boot_wait: float = 2.0,
        socket_timeout: float = 30.0,
        stderr_log: Path | str | None = None,
    ) -> None:
        self._port = port
        self._rom = str(rom)
        self._mesen = str(mesen)
        # Default cwd to the ROM's parent so MSU-1 / save / sram files
        # resolve relative to the ROM. Override for projects that want
        # Mesen's working directory elsewhere.
        self._cwd = str(cwd) if cwd is not None else str(Path(self._rom).parent)
        self._boot_wait = boot_wait
        self._socket_timeout = socket_timeout
        self._stderr_log = stderr_log
        self._proc: subprocess.Popen | None = None
        self._sock: socket.socket | None = None
        self._buf = b""
        self._next_id = 1
        self._notifications: list[dict] = []
        self._stderr_lines: list[bytes] = []
        self._stderr_thread: threading.Thread | None = None

    @classmethod
    def from_env(cls, **overrides) -> "McpSession":
        """Construct from MESEN_EXE / MESEN_ROM / MESEN_CWD env vars.

        Keyword overrides take precedence; useful for test harnesses
        that want to share env-var defaults but vary the port."""
        rom = overrides.pop("rom", None) or os.environ.get("MESEN_ROM")
        mesen = overrides.pop("mesen", None) or os.environ.get("MESEN_EXE")
        cwd = overrides.pop("cwd", None) or os.environ.get("MESEN_CWD")
        if not rom:
            raise McpError("MESEN_ROM env var or rom= keyword is required")
        if not mesen:
            raise McpError("MESEN_EXE env var or mesen= keyword is required")
        return cls(rom=rom, mesen=mesen, cwd=cwd, **overrides)

    # ---------------------------------------------------------------
    # Lifecycle
    # ---------------------------------------------------------------

    def __enter__(self) -> "McpSession":
        try:
            validate_mesen_build(self._mesen)
        except MesenBuildError as exc:
            raise McpError(str(exc)) from exc
        # We *capture* stderr (uninit warnings, [mcp] log lines) for
        # post-mortem visibility, but never block Mesen on a full pipe.
        # Drain in a daemon thread; if stderr isn't drained, Mesen's
        # Console.Error.WriteLine inside the MCP handler blocks the very
        # thread that's processing requests, and `initialize` hangs.
        self._proc = subprocess.Popen(
            [self._mesen, "--mcp", f"--mcp-port={self._port}", self._rom],
            cwd=self._cwd,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.PIPE,
        )

        def _drain() -> None:
            try:
                assert self._proc is not None and self._proc.stderr is not None
                for line in iter(self._proc.stderr.readline, b""):
                    if not line:
                        break
                    self._stderr_lines.append(line)
            except Exception:
                pass

        self._stderr_thread = threading.Thread(target=_drain, daemon=True)
        self._stderr_thread.start()

        time.sleep(self._boot_wait)
        self._sock = self._connect()
        self.call("initialize", {})
        return self

    def __exit__(self, *exc) -> None:
        try:
            if self._sock is not None:
                try:
                    self.call("shutdown", {})
                except Exception:
                    pass
                self._sock.close()
        finally:
            if self._proc is not None:
                try:
                    self._proc.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    self._proc.kill()
                if self._stderr_log:
                    Path(self._stderr_log).write_bytes(
                        b"".join(self._stderr_lines)
                    )

    # ---------------------------------------------------------------
    # Raw JSON-RPC
    # ---------------------------------------------------------------

    def _connect(self) -> socket.socket:
        deadline = time.time() + 15
        last_err: Exception | None = None
        while time.time() < deadline:
            try:
                s = socket.create_connection(("127.0.0.1", self._port), timeout=1)
                s.settimeout(self._socket_timeout)
                return s
            except OSError as e:
                last_err = e
                time.sleep(0.2)
        raise McpError(f"connect to 127.0.0.1:{self._port} timed out: {last_err}")

    def call(self, method: str, params: dict | None = None) -> dict:
        """Send a request, stash any notifications that arrive while
        we wait for the matching response, return the response."""
        if self._sock is None:
            raise McpError("session not open")
        msg_id = self._next_id
        self._next_id += 1
        msg: dict[str, Any] = {"jsonrpc": "2.0", "id": msg_id, "method": method}
        if params is not None:
            msg["params"] = params
        self._sock.sendall((json.dumps(msg, separators=(",", ":")) + "\n").encode())
        while True:
            while b"\n" not in self._buf:
                chunk = self._sock.recv(65536)
                if not chunk:
                    raise McpError("server closed connection mid-call")
                self._buf += chunk
            line, self._buf = self._buf.split(b"\n", 1)
            resp = json.loads(line)
            if "id" in resp and resp["id"] == msg_id:
                if "error" in resp:
                    raise McpError(f"{method}: {resp['error']}")
                return resp
            if "method" in resp:
                self._notifications.append(resp)
                continue
            # Out-of-order response ID — drop it and keep waiting.

    def tool(self, name: str, args: dict | None = None) -> Any:
        resp = self.call("tools/call", {"name": name, "arguments": args or {}})
        content = resp["result"].get("content", [])
        if not content:
            return None
        text = content[0].get("text", "")
        try:
            return json.loads(text)
        except json.JSONDecodeError:
            return text

    # ---------------------------------------------------------------
    # Typed tool wrappers
    # ---------------------------------------------------------------

    def ping(self, **kwargs) -> dict:
        return self.tool("ping", kwargs or {})

    def get_state(self) -> dict:
        return self.tool("get_state")

    def get_cpu_state(self, cpu_type: str = "Snes") -> dict:
        """CPU register state (pc, k, a, x, y, sp, d, dbr, ps, emulationMode,
        stopState, cycleCount). cpu_type='Sa1' reads the SA-1 coprocessor."""
        return self.tool("get_cpu_state", {"cpuType": cpu_type})

    def set_cpu_state(self, cpu_type: str = "Snes", **registers: int | bool) -> dict:
        """Write selected architectural registers and return the written state.

        Only supplied fields change. For SA-1 register seeding use
        ``set_cpu_state("Sa1", pc=..., k=..., d=..., sp=..., ps=...)``.
        Pause first when the write must take effect at a deterministic boundary.
        """
        args: dict[str, object] = {"cpuType": cpu_type}
        args.update(registers)
        return self.tool("set_cpu_state", args)

    def pause(self) -> dict:
        return self.tool("pause")

    def resume(self) -> dict:
        return self.tool("resume")

    def step_cpu(self, cpu_type: str = "Snes", count: int = 1,
                 step_type: str = "Step") -> dict:
        """Execute exactly ``count`` debugger CPU instructions, then pause."""
        return self.tool("step_cpu", {
            "cpuType": cpu_type,
            "count": count,
            "stepType": step_type,
        })

    def run_frames(self, count: int) -> dict:
        return self.tool("run_frames", {"count": count})

    def read_memory(self, mem_type: str, address: int, length: int) -> bytes:
        r = self.tool("read_memory", {
            "memoryType": mem_type,
            "address": address,
            "length": length,
        })
        return bytes.fromhex(r["hex"])

    def read_u8(self, address: int, mem_type: str = "snesMemory") -> int:
        return self.read_memory(mem_type, address, 1)[0]

    def read_u16(self, address: int, mem_type: str = "snesMemory") -> int:
        b = self.read_memory(mem_type, address, 2)
        return b[0] | (b[1] << 8)

    def write_hex(self, address: int, hex_bytes: str, mem_type: str = "snesMemory") -> dict:
        return self.tool("write_memory", {
            "memoryType": mem_type,
            "address": address,
            "hex": hex_bytes,
        })

    # Alias for catalog-name uniformity. Most callers should prefer write_hex
    # / write_u8 / write_u16 — they're more ergonomic.
    def write_memory(self, mem_type: str, address: int, hex_bytes: str) -> dict:
        return self.write_hex(address, hex_bytes, mem_type)

    def write_u8(self, address: int, value: int, mem_type: str = "snesMemory") -> dict:
        return self.write_hex(address, f"{value & 0xFF:02x}", mem_type)

    def write_u16(self, address: int, value: int, mem_type: str = "snesMemory") -> dict:
        return self.write_hex(address, f"{value & 0xFF:02x}{(value >> 8) & 0xFF:02x}", mem_type)

    def take_screenshot(self, format: str = "path") -> dict:
        return self.tool("take_screenshot", {"format": format})

    def save_state(self, path: str | Path) -> dict:
        return self.tool("save_state", {"path": str(path)})

    def load_state(self, path: str | Path) -> dict:
        return self.tool("load_state", {"path": str(path)})

    # Button bitmask constants for set_input.
    BTN_A, BTN_B = 0x001, 0x002
    BTN_SELECT, BTN_START = 0x004, 0x008
    BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT = 0x010, 0x020, 0x040, 0x080
    BTN_X, BTN_L, BTN_R, BTN_Y = 0x100, 0x200, 0x400, 0x800

    def set_input(self, buttons: int, frames: int, port: int = 0) -> dict:
        return self.tool("set_input", {
            "port": port,
            "buttons": buttons,
            "frames": frames,
        })

    def get_ppu_state(self) -> dict:
        return self.tool("get_ppu_state")

    # ---------------------------------------------------------------
    # Hooks
    # ---------------------------------------------------------------

    def _add_hook(self, tool_name: str, address: int,
                  end_address: int | None = None,
                  cpu_type: str = "Snes",
                  match_value: int = 0,
                  match_value_mask: int = 0,
                  x_value: int = 0, x_mask: int = 0) -> int:
        args: dict = {"address": address, "cpuType": cpu_type}
        if end_address is not None:
            args["endAddress"] = end_address
        if match_value_mask != 0:
            args["matchValue"] = match_value
            args["matchValueMask"] = match_value_mask
        if x_mask != 0:
            args["xValue"] = x_value
            args["xMask"] = x_mask
        return self.tool(tool_name, args)["handle"]

    def add_exec_hook(self, address: int, end_address: int | None = None,
                      cpu_type: str = "Snes",
                      match_value: int = 0, match_value_mask: int = 0,
                      x_value: int = 0, x_mask: int = 0) -> int:
        """Register an exec hook; returns the handle. Each time the
        CPU executes an instruction in [address, end_address], the
        server pushes a notifications/mesen/hookFired message. Call
        drain_notifications() to collect them. Notifications and run_until
        observedEvents include event-time hostPc, hostSp, hostP, hostE,
        hostM, hostX, hostPbr, hostD, hostDbr, hostA, hostXReg, hostY,
        and hostCycleCount. match_value_mask=0 disables the value filter
        (every hit fires)."""
        return self._add_hook("add_exec_hook", address, end_address,
                              cpu_type, match_value, match_value_mask,
                              x_value, x_mask)

    def add_read_hook(self, address: int, end_address: int | None = None,
                      cpu_type: str = "Snes",
                      match_value: int = 0, match_value_mask: int = 0) -> int:
        """Same shape as add_exec_hook, but fires on memory reads.
        value in the notification is the byte read."""
        return self._add_hook("add_read_hook", address, end_address,
                              cpu_type, match_value, match_value_mask)

    def add_write_hook(self, address: int, end_address: int | None = None,
                       cpu_type: str = "Snes",
                       match_value: int = 0, match_value_mask: int = 0) -> int:
        """Same shape as add_exec_hook, but fires on memory writes."""
        return self._add_hook("add_write_hook", address, end_address,
                              cpu_type, match_value, match_value_mask)

    def lookup_symbol(self, sym_file: str, pattern: str, max_results: int = 64) -> dict:
        return self.tool("lookup_symbol", {
            "symFile": sym_file,
            "pattern": pattern,
            "maxResults": max_results,
        })

    def lookup_pansy(self, pansy_file: str, pattern: str = "",
                     max_results: int = 256, sections_only: bool = False) -> dict:
        """Read a TheAnsarya/pansy v1.0 metadata file. Returns sections,
        SYMBOLS, COMMENTS, MEMORY_REGIONS (filtered by `pattern` if given)."""
        return self.tool("lookup_pansy", {
            "pansyFile": pansy_file,
            "pattern": pattern,
            "maxResults": max_results,
            "sectionsOnly": sections_only,
        })

    def disassemble(self, address: int, count: int = 16, cpu_type: str = "Snes") -> list[dict]:
        return self.tool("disassemble", {
            "address": address,
            "count": count,
            "cpuType": cpu_type,
        })["lines"]

    def run_until(self, max_frames: int = 600, hook_handle: int = 0) -> dict:
        return self.tool("run_until", {
            "maxFrames": max_frames,
            "hookHandle": hook_handle,
        })

    def crop_screenshot(self, x: int, y: int, width: int, height: int,
                        format: str = "path") -> dict:
        return self.tool("crop_screenshot", {
            "x": x, "y": y, "width": width, "height": height,
            "format": format,
        })

    def render_palette(self, mode: str = "grid", swatch: int = 16,
                       highlight: int | None = None,
                       format: str = "path") -> dict:
        args: dict = {"mode": mode, "swatch": swatch, "format": format}
        if highlight is not None:
            args["highlight"] = highlight
        return self.tool("render_palette", args)

    def render_tilemap(self, layer: int = 1, scale: int = 1,
                       format: str = "path") -> dict:
        return self.tool("render_tilemap", {
            "layer": layer, "scale": scale, "format": format,
        })

    def render_tile_sheet(self, address: int = 0, count: int = 256,
                          bpp: int = 4, palette: int = 0,
                          columns: int = 16, scale: int = 1,
                          format: str = "path") -> dict:
        return self.tool("render_tile_sheet", {
            "address": address, "count": count, "bpp": bpp,
            "palette": palette, "columns": columns, "scale": scale,
            "format": format,
        })

    def render_oam(self, mode: str = "positioned", scale: int = 1,
                   format: str = "path") -> dict:
        return self.tool("render_oam", {
            "mode": mode, "scale": scale, "format": format,
        })

    def trace_log(self, count: int = 32, cpu_type: str = "Snes") -> dict:
        return self.tool("trace_log", {
            "count": count, "cpuType": cpu_type,
        })

    def watch_addresses(self, addresses: list[dict], frames: int = 60) -> dict:
        """Watch a list of addresses for changes over `frames` frames.

        addresses = list of {"address": int, "name": str, "memoryType": str?}.
        Returns a timeline of {frame, address, oldValue, newValue, name} entries.
        """
        return self.tool("watch_addresses", {
            "addresses": addresses, "frames": frames,
        })

    def render_filmstrip(self, count: int, frame_step: int = 1,
                         columns: int = 0, scale: int = 1,
                         label: bool = True, format: str = "path") -> dict:
        return self.tool("render_filmstrip", {
            "count": count, "frameStep": frame_step,
            "columns": columns, "scale": scale,
            "label": label, "format": format,
        })

    def memory_diff(self, regions: list[dict], frames: int,
                    max_changes: int = 256) -> dict:
        """Snapshot, run frames, snapshot, return diffs.

        regions = list of {"memoryType": str, "address": int, "length": int}.
        """
        return self.tool("memory_diff", {
            "regions": regions, "frames": frames, "maxChanges": max_changes,
        })

    def symbolic_dump(self, sym_file: str, address: int, length: int,
                      unit: str = "byte", max_distance: int = 64) -> dict:
        return self.tool("symbolic_dump", {
            "symFile": sym_file, "address": address, "length": length,
            "unit": unit, "maxDistance": max_distance,
        })

    def save_state_slot(self, slot: int) -> dict:
        return self.tool("save_state_slot", {"slot": slot})

    def load_state_slot(self, slot: int) -> dict:
        return self.tool("load_state_slot", {"slot": slot})

    def read_dma_state(self) -> list[dict]:
        return self.tool("read_dma_state")["channels"]

    def add_frame_hook(self, every_n: int = 1, cpu_type: str = "Snes") -> int:
        return self.tool("add_frame_hook", {
            "everyN": every_n,
            "cpuType": cpu_type,
        })["handle"]

    def reset_emulator(self) -> dict:
        return self.tool("reset_emulator")

    def record_audio(self, path: str | Path) -> dict:
        return self.tool("record_audio", {"path": str(path)})

    def stop_audio(self) -> dict:
        return self.tool("stop_audio")

    def get_audio_state(self) -> dict:
        return self.tool("get_audio_state")

    # ---------------------------------------------------------------
    # Movie record / playback
    # ---------------------------------------------------------------

    def record_movie(self, path: str | Path, author: str = "mesen-mcp",
                     description: str = "", from_: str = "CurrentState") -> dict:
        return self.tool("record_movie", {
            "path": str(path), "author": author,
            "description": description, "from": from_,
        })

    def play_movie(self, path: str | Path) -> dict:
        return self.tool("play_movie", {"path": str(path)})

    def stop_movie(self) -> dict:
        return self.tool("stop_movie")

    def movie_state(self) -> dict:
        return self.tool("movie_state")

    # ---------------------------------------------------------------
    # Audio analysis
    # ---------------------------------------------------------------

    def audio_fingerprint(self, path: str | Path) -> dict:
        return self.tool("audio_fingerprint", {"path": str(path)})

    def audio_waveform_png(self, path: str | Path,
                           output_path: str | Path | None = None,
                           width: int = 1024, height: int = 256,
                           format: str = "path") -> dict:
        args: dict = {
            "path": str(path), "width": width, "height": height, "format": format,
        }
        if output_path is not None:
            args["outputPath"] = str(output_path)
        return self.tool("audio_waveform_png", args)

    def remove_hook(self, handle: int) -> bool:
        return bool(self.tool("remove_hook", {"handle": handle})["removed"])

    def list_hooks(self) -> list[dict]:
        return self.tool("list_hooks")["hooks"]

    def hook_diag(self) -> dict:
        return self.tool("hook_diag")

    def drain_notifications(self, timeout: float = 0.1) -> list[dict]:
        """Read any pending notifications without blocking long. MCP
        notifications have no `id`, so call() stashes them in
        self._notifications; this empties that queue plus any in the
        socket buffer."""
        got = list(self._notifications)
        self._notifications.clear()
        if self._sock is None:
            return got
        deadline = time.time() + timeout
        self._sock.settimeout(0.05)
        try:
            while time.time() < deadline:
                try:
                    while b"\n" not in self._buf:
                        chunk = self._sock.recv(65536)
                        if not chunk:
                            return got
                        self._buf += chunk
                except (socket.timeout, TimeoutError):
                    break
                line, self._buf = self._buf.split(b"\n", 1)
                r = json.loads(line)
                if "method" in r:
                    got.append(r)
        finally:
            self._sock.settimeout(self._socket_timeout)
        return got
