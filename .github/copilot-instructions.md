# Nexen - AI Copilot Directives

## Project Overview

**Nexen** is a multi-system emulator (NES, SNES, GB, GBA, SMS, PCE, WS) written in C++, based on Mesen2.

Key features and focus areas:

1. **TAS Editor** - Full-featured movie recording, playback, and editing
2. **Save State System** - Quick slots, thumbnail previews, compression
3. **🌼 Pansy Metadata Export** - Integration with disassembly metadata format
4. **C++ Modernization** - C++20/23 features
5. **Performance Improvements** - Hot path optimization

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
- **Tab width: 4 spaces** - ALWAYS use 4-space-equivalent tabs
- NEVER use spaces for indentation - only tabs
- Inside code blocks in markdown, use spaces for alignment of ASCII art/diagrams

### Brace Style — K&R (One True Brace)
- **Opening braces on the SAME LINE** as the statement — ALWAYS
- This applies to ALL constructs: `if`, `else`, `for`, `while`, `switch`, `try`, `catch`, functions, methods, classes, structs, namespaces, lambdas, properties, enum declarations, etc.
- `else` and `else if` go on the same line as the closing brace: `} else {`
- `catch` goes on the same line as the closing brace: `} catch (...) {`
- **NEVER use Allman style** (brace on its own line)
- **NEVER put an opening brace on a new line** — not even for long parameter lists

#### C++ Examples
```cpp
// ✅ CORRECT — K&R style
if (condition) {
	DoSomething();
} else if (other) {
	DoOther();
} else {
	DoFallback();
}

for (int i = 0; i < count; i++) {
	Process(i);
}

void MyClass::Execute(int param) {
	// body
}

class MyClass : public Base {
public:
	void Method() {
		// body
	}
};

// ❌ WRONG — Allman style (DO NOT USE)
if (condition)
{
	DoSomething();
}
else
{
	DoFallback();
}
```

#### C# Examples
```csharp
// ✅ CORRECT — K&R style
if (condition) {
	DoSomething();
} else {
	DoFallback();
}

public void Execute(int param) {
	// body
}

public class MyClass : Base {
	public string Name { get; set; }

	public void Method() {
		// body
	}
}

// ❌ WRONG — Allman style (DO NOT USE)
if (condition)
{
	DoSomething();
}
```

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

## Problem-Solving Philosophy

### ⚠️ NEVER GIVE UP on Hard Problems

When a task is complex or seems difficult:

1. **NEVER declare something "too hard" or "not worth it"** and close the issue
2. **Break it down** - Create multiple smaller sub-issues for research, prototyping, and incremental progress
3. **Research first** - Create research issues to investigate approaches, alternatives, and prior art
4. **Document everything** - Create docs, code-plans, and analysis documents in `~docs/plans/`
5. **Prototype** - Create spike/prototype branches to test approaches before committing
6. **Incremental progress** - Even partial progress (e.g., replacing 3 of 15 usages) is valuable
7. **Create issues for future work** - If something can't be done now, create well-documented issues with clear context for later

### Issue Decomposition Pattern
When facing a large task:
- `[N.X.1]` Research/Investigation - Analyze scope, dependencies, alternatives
- `[N.X.2]` Document findings - Create technical analysis docs
- `[N.X.3]` Create prototype - Spike branch to test feasibility
- `[N.X.4]` Implement Phase 1 - Lowest-risk subset first
- `[N.X.5]` Implement Phase 2 - Next batch of changes
- `[N.X.N]` Final cleanup - Remove old code, update docs

### What "Closing Too Soon" Looks Like
- ❌ "This is deeply integrated, keeping as-is" → Instead: break it into phases
- ❌ "Migration cost-prohibitive" → Instead: create research issues and prototype
- ❌ "High regression risk" → Instead: create test plan and incremental migration
- ✅ Close only when the work is **actually complete** or **truly impossible** (not just hard)

## Related Projects

- **Pansy** - Metadata format for disassembly data
- **Peony** - Disassembler using Pansy format
- **GameInfo** - ROM hacking toolkit

