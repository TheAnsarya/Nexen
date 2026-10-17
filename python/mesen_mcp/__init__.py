"""mesen_mcp — Python client + stdio bridge for the Mesen 2 MCP server.

The C# server lives in our Mesen2 fork (UI/Utilities/Mcp/) and is
launched with `Mesen.exe --mcp [--mcp-port=N]`. This package provides:

  - mesen_mcp.McpSession  — high-level Python client (spawn, connect,
                            typed tool wrappers, hook event drain).
  - mesen_mcp.bridge.main() — stdio<->TCP bridge for MCP clients
                              (Claude Code, Cursor, etc.) that speak
                              stdio transport.

Both are parameterized by env vars or constructor arguments:

  MESEN_EXE   — path to Mesen.exe        (no default)
  MESEN_ROM   — path to ROM file         (no default)
  MESEN_CWD   — Mesen process working dir (default: ROM's parent)

Drop the package into another project, set the env vars, and you have
an MCP-driven Mesen 2 debugger harness with no source edits required.
"""
from .session import McpError, McpSession

__all__ = ["McpSession", "McpError"]
__version__ = "0.1.0"
