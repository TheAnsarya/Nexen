"""Stdio<->TCP bridge for Mesen 2's MCP server.

MCP clients (Claude Code, Cursor, etc.) speak stdio transport. Mesen's
`--mcp` mode listens on a TCP loopback socket. This bridge spawns
Mesen, waits for the socket, and forwards JSON-RPC line-frames between
stdin/stdout and the Mesen TCP socket until stdin closes.

Usage from a thin shim script (e.g. tools/mesen_inproc_bridge.py):

    from mesen_mcp.bridge import main
    sys.exit(main())

Configuration is read from env vars at run time:

    MESEN_EXE   — path to Mesen.exe (REQUIRED)
    MESEN_ROM   — path to the ROM   (REQUIRED)
    MESEN_CWD   — Mesen working dir (default: ROM's parent directory)
    MESEN_PORT  — TCP port          (default: deterministic per-cwd CRC)

Lifecycle:
  - Spawn Mesen on bridge start.
  - Forward JSON-RPC lines bidirectionally until stdin EOF.
  - On stdin EOF or any error, send `shutdown` to Mesen and wait for
    its process to exit (5s grace before SIGKILL).
"""
from __future__ import annotations

import os
import socket
import subprocess
import sys
import threading
import time
import zlib
from pathlib import Path

from .validate import validate_mesen_build

# Boot/connect/shutdown timing knobs.
_BOOT_WAIT_SECS = 3.0
_CONNECT_TIMEOUT = 15.0
_SHUTDOWN_TIMEOUT = 5.0


def _log(msg: str) -> None:
    """Diagnostic logger — writes to stderr so stdio MCP traffic stays clean."""
    sys.stderr.write(f"[mesen-mcp.bridge] {msg}\n")
    sys.stderr.flush()


def _resolve_config() -> tuple[Path, Path, Path, int]:
    """Read config from env vars; raise on missing required values."""
    exe = os.environ.get("MESEN_EXE")
    rom = os.environ.get("MESEN_ROM")
    if not exe:
        raise RuntimeError("MESEN_EXE env var is required")
    if not rom:
        raise RuntimeError("MESEN_ROM env var is required")
    mesen = Path(exe)
    rom_path = Path(rom)
    cwd = Path(os.environ.get("MESEN_CWD", rom_path.parent))
    if "MESEN_PORT" in os.environ:
        port = int(os.environ["MESEN_PORT"])
    else:
        # Deterministic per-cwd port so multiple checkouts don't collide.
        port = 7350 + (zlib.crc32(str(cwd.resolve()).encode()) % 200)
    return mesen, rom_path, cwd, port


def _spawn_mesen(mesen: Path, rom: Path, cwd: Path, port: int) -> subprocess.Popen:
    validate_mesen_build(mesen)
    if not rom.exists():
        raise RuntimeError(f"ROM not found at {rom}")
    _log(f"spawning {mesen.name} --mcp --mcp-port={port} {rom.name}")
    return subprocess.Popen(
        [str(mesen), "--mcp", f"--mcp-port={port}", str(rom)],
        cwd=str(cwd),
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )


def _connect_mesen(port: int) -> socket.socket:
    deadline = time.time() + _CONNECT_TIMEOUT
    last_err: Exception | None = None
    while time.time() < deadline:
        try:
            s = socket.create_connection(("127.0.0.1", port), timeout=1)
            s.settimeout(None)  # blocking for the bridge loop
            _log(f"connected to Mesen on 127.0.0.1:{port}")
            return s
        except OSError as e:
            last_err = e
            time.sleep(0.2)
    raise RuntimeError(f"connect to 127.0.0.1:{port} timed out: {last_err}")


def _stdin_to_socket(sock: socket.socket) -> None:
    """Read MCP messages from the client (stdin) and forward to Mesen.

    The MCP stdio transport sends framed messages: each request is a
    single line of JSON terminated by \\n. Mesen expects the same line
    framing on the TCP side, so this is a straight byte pipe.
    """
    try:
        while True:
            line = sys.stdin.buffer.readline()
            if not line:
                _log("stdin closed; signalling shutdown")
                try:
                    sock.sendall(b'{"jsonrpc":"2.0","id":99999,"method":"shutdown"}\n')
                except OSError:
                    pass
                break
            try:
                sock.sendall(line)
            except OSError as e:
                _log(f"send to Mesen failed: {e}")
                break
    except Exception as e:
        _log(f"stdin pump crashed: {e}")
    finally:
        try:
            sock.shutdown(socket.SHUT_WR)
        except OSError:
            pass


def _socket_to_stdout(sock: socket.socket) -> None:
    """Read replies and notifications from Mesen, write to client stdout."""
    buf = b""
    try:
        while True:
            chunk = sock.recv(65536)
            if not chunk:
                _log("Mesen closed socket")
                break
            buf += chunk
            while b"\n" in buf:
                line, buf = buf.split(b"\n", 1)
                # Pass through the JSON line untouched (already MCP-formatted).
                sys.stdout.buffer.write(line + b"\n")
                sys.stdout.buffer.flush()
    except Exception as e:
        _log(f"socket pump crashed: {e}")
    finally:
        try:
            sock.close()
        except OSError:
            pass


def _shutdown_mesen(proc: subprocess.Popen) -> None:
    try:
        proc.wait(timeout=_SHUTDOWN_TIMEOUT)
        _log(f"Mesen exited cleanly (code {proc.returncode})")
    except subprocess.TimeoutExpired:
        _log("Mesen didn't exit in time; killing")
        proc.kill()
        try:
            proc.wait(timeout=2)
        except subprocess.TimeoutExpired:
            pass


def main() -> int:
    proc: subprocess.Popen | None = None
    try:
        mesen, rom, cwd, port = _resolve_config()
        proc = _spawn_mesen(mesen, rom, cwd, port)
        # Mesen needs ~2s to load the ROM and start listening. Sleep first,
        # then poll the socket to handle whatever startup time is left.
        time.sleep(_BOOT_WAIT_SECS)
        sock = _connect_mesen(port)

        # Two threads: stdin→socket and socket→stdout. Block on the
        # socket→stdout one (when Mesen disconnects, we exit).
        t_in = threading.Thread(
            target=_stdin_to_socket, args=(sock,), daemon=True, name="stdin-pump"
        )
        t_in.start()
        _socket_to_stdout(sock)
        t_in.join(timeout=1.0)
        return 0
    except Exception as e:
        _log(f"fatal: {e}")
        return 1
    finally:
        if proc is not None:
            _shutdown_mesen(proc)


if __name__ == "__main__":
    sys.exit(main())
