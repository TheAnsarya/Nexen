# Nexen C++ Modernization - AI Copilot Directives

## Project Overview

**Nexen** is a multi-system emulator (NES, SNES, GB, GBA, SMS, PCE, WS) written in C++.

This fork (`TheAnsarya/Nexen`) focuses on:
1. C++ modernization (C++20/23 features)
2. 🌼 Pansy metadata integration
3. Performance improvements

## GitHub Issue Management

### ⚠️ CRITICAL: Always Create Issues on GitHub Directly

**NEVER just document issues in markdown files.** Always create actual GitHub issues using the `gh` CLI:

```powershell
# Create an issue
gh issue create --repo TheAnsarya/Nexen --title "[X.Y] Issue Title" --body "Description" --label "label1,label2"

# Add labels
gh issue edit <number> --repo TheAnsarya/Nexen --add-label "label"

# Close issue
gh issue close <number> --repo TheAnsarya/Nexen --comment "Completed in commit abc123"
```

### Issue Numbering Convention
- Epic issues: `[Epic N]` (e.g., `[Epic 11] C++ Standard Library Modernization`)
- Sub-issues: `[N.X]` (e.g., `[11.4] Apply [[likely]]/[[unlikely]] Attributes`)

### Required Labels
- `cpp` - C++ related
- `modernization` - Modernization work
- `performance` - Performance related
- Priority: `high-priority`, `medium-priority`, `low-priority`

## Coding Standards

### Indentation
- **TABS for indentation** - enforced by `.editorconfig`
- Tab width: 4 spaces equivalent

### Brace Style
- **K&R style** - Opening braces on same line

### Hexadecimal Values
- **Always lowercase**: `0xff00`, not `0xFF00`

### C++ Standard
- **C++23** (`/std:c++latest`)
- Platform toolset: **v145** (VS 2026)

### C# Standard
- **.NET 10** with latest C# features
- File-scoped namespaces where applicable
- Nullable reference types enabled

### ⚠️ Comment Safety Rule
**When adding or modifying comments, NEVER change the actual code.**
- Changes to comments must not alter code logic, structure, or formatting
- When adding XML documentation or inline comments, preserve all existing code exactly
- Verify code integrity after adding documentation

## Performance Guidelines

### Hot Path Rules
**NEVER add overhead to:**
- CPU emulation cores (`NesCpu`, `SnesCpu`, etc.)
- Memory read/write functions
- PPU rendering loops
- Per-frame processing

### Safe Modernization (Zero Cost)
- `[[likely]]` / `[[unlikely]]` attributes
- `constexpr` expansion
- `[[nodiscard]]` attributes
- `std::bit_cast`

### Cautious Modernization (Profile First)
- `std::span` - may have debug bounds-check overhead
- `std::format` - use only in cold paths
- `std::views` - avoid in hot paths (prevents vectorization)

## Branch Strategy

- `cpp-modernization` - Active modernization work
- `pansy-export` - Pansy integration features
- `master` - Stable (synced with upstream)

## Build Commands

```powershell
# Build Release x64
& "C:\Program Files\Microsoft Visual Studio\18\Insiders\MSBuild\Current\Bin\MSBuild.exe" `
	Nexen.sln /p:Configuration=Release /p:Platform=x64 /t:Build

# Clean build
& "C:\Program Files\Microsoft Visual Studio\18\Insiders\MSBuild\Current\Bin\MSBuild.exe" `
	Nexen.sln /p:Configuration=Release /p:Platform=x64 /t:Clean
```

## Documentation

- Session logs: `~docs/session-logs/YYYY-MM-DD-session-NN.md`
- Plans: `~docs/plans/`
- Modernization tracking: `~docs/modernization/`

## Related Projects

- **Pansy** - Metadata format for disassembly data
- **Peony** - Disassembler using Pansy format
- **GameInfo** - ROM hacking toolkit

