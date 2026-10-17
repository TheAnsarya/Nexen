"""Validation helpers for locating a usable Mesen MCP build."""
from __future__ import annotations

from pathlib import Path


class MesenBuildError(RuntimeError):
    """Raised when MESEN_EXE points at a build that cannot serve MCP."""


def _contains_marker(path: Path, marker: bytes) -> bool:
    try:
        return marker in path.read_bytes()
    except OSError as exc:
        raise MesenBuildError(f"could not read {path}: {exc}") from exc


def _find_managed_assembly(build_dir: Path) -> Path | None:
    """Find the managed assembly (.dll) that contains the MCP runner.

    Traditional Mesen builds have Mesen.dll; Nexen builds have Nexen.dll.
    Search for any DLL containing the McpRunner marker if the expected
    names are missing.
    """
    for candidate_name in ("Mesen.dll", "Nexen.dll"):
        candidate = build_dir / candidate_name
        if candidate.exists() and _contains_marker(candidate, b"McpRunner"):
            return candidate
    # Fallback: scan all .dll files for the marker
    for dll in sorted(build_dir.glob("*.dll")):
        if _contains_marker(dll, b"McpRunner"):
            return dll
    return None


def _find_native_core(build_dir: Path) -> Path | None:
    """Find the native core (.so or .dll) that exports the MCP hook bridge.

    May live alongside the exe or in a linux-x64/publish/ subdirectory.
    """
    for candidate_name in ("MesenCore.so", "MesenCore.dll"):
        candidate = build_dir / candidate_name
        if candidate.exists() and _contains_marker(candidate, b"McpDrainEvents"):
            return candidate
    # Check publish subdirectory (Nexen Linux layout)
    for candidate_name in ("MesenCore.so", "MesenCore.dll"):
        candidate = build_dir / "linux-x64" / "publish" / candidate_name
        if candidate.exists() and _contains_marker(candidate, b"McpDrainEvents"):
            return candidate
    # Fallback: scan all .so/.dll files for the marker
    for ext in ("*.so", "*.dll"):
        for lib in sorted(build_dir.glob(ext)):
            if _contains_marker(lib, b"McpDrainEvents"):
                return lib
    return None


def validate_mesen_build(mesen: Path | str) -> None:
    """Fail fast when MESEN_EXE points at an incomplete/stale build.

    Windows source builds produce a small apphost `Mesen.exe` beside
    `Mesen.dll` and `MesenCore.dll`. The Nexen build (astrobleem/Mesen2
    fork) uses `Nexen.dll` and may place `MesenCore.so` in a
    `linux-x64/publish/` subdirectory.
    """
    exe = Path(mesen)
    if not exe.exists():
        raise MesenBuildError(f"Mesen.exe not found at {exe}")

    build_dir = exe.parent
    managed = _find_managed_assembly(build_dir)
    native = _find_native_core(build_dir)

    missing = []
    if managed is None:
        missing.append("managed assembly (Mesen.dll or Nexen.dll with McpRunner)")
    if native is None:
        missing.append("native core (MesenCore.so or MesenCore.dll with McpDrainEvents)")

    if missing:
        raise MesenBuildError(
            f"MESEN_EXE points at {exe}, but required file(s) are missing or "
            f"lack MCP markers: {', '.join(missing)}. Point MESEN_EXE at the "
            "build directory containing a matched Mesen.exe/build, Mesen.dll "
            "(or Nexen.dll), and MesenCore.dll (or .so on Linux)."
        )
