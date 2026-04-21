# Nexen - AI Copilot Directives

## Project Overview

**Nexen** is a multi-system emulator (NES, SNES, GB, GBA, SMS, PCE, WS) written in C++, based on Mesen2.

Key features and focus areas:

1. **TAS Editor** - Full-featured movie recording, playback, and editing
2. **Save State System** - Quick slots, thumbnail previews, compression
3. **🌼 Pansy Metadata Export** - Integration with disassembly metadata format
4. **C++ Modernization** - C++20/23 features
5. **Performance Improvements** - Hot path optimization

### ⚠️ CRITICAL: Nexen Is NOT Mesen2

**Nexen is its own emulator.** While it originated from Mesen2, it has diverged significantly. Do NOT treat upstream Mesen2 as authoritative.

- **Never adopt upstream Mesen2 code to "fix" emulation bugs** — if a bug exists in both Mesen2 AND Nexen, upstream code is by definition not the fix
- **Never revert Nexen-specific changes** to match upstream — only move forward
- **Mesen2 is an occasional reference only** — use it to understand original intent, not as a source of correctness
- **For emulation accuracy fixes**, research hardware documentation and other emulators (ares, mednafen, MAME, etc.) that have independent implementations
- **When fixing emulation bugs**, prefer: (1) hardware test results, (2) ares/mednafen reference, (3) WSdev/NESDev documentation, (4) Mesen2 as last resort context only

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

### ⚠️ MANDATORY: Issue-First Workflow

**Always create GitHub issues BEFORE starting implementation work.** This is non-negotiable.

1. **Before Implementation:**
   - Create a GitHub issue describing the planned work
   - Include scope, approach, and acceptance criteria
   - Add appropriate labels

2. **During Implementation:**
   - Reference issue number in commits: `git commit -m "Fix piano roll rendering - #247"`
   - Update issue with progress comments if work spans multiple sessions
   - Add sub-issues for discovered work

3. **After Implementation:**
   - Close issue with completion comment including commit hash
   - Link related issues if applicable

**Workflow Pattern:**
```powershell
# 1. Create issue FIRST
gh issue create --repo TheAnsarya/Nexen --title "Description" --body "Details" --label "label"

# 2. Add prompt tracking comment (for AI-created issues)
gh issue comment <number> --repo TheAnsarya/Nexen --body "Prompt for work:`n{original user prompt}"

# 3. Implement the fix/feature

# 4. Commit with issue reference
git add .
git commit -m "Brief description - #<issue-number>"

# 5. Close issue with summary
gh issue close <number> --repo TheAnsarya/Nexen --comment "Completed in <commit-hash>"
```

### ⚠️ MANDATORY: Prompt Tracking for AI-Created Issues

When creating GitHub issues from AI prompts, **IMMEDIATELY** add the original user prompt as the **FIRST comment** right after creating the issue - before doing any implementation work:

```powershell
# Create issue
$issueUrl = gh issue create --repo TheAnsarya/Nexen --title "Description" --body "Details" --label "label"
$issueNum = ($issueUrl -split '/')[-1]

# IMMEDIATELY add prompt as first comment (before any other work)
gh issue comment $issueNum --repo TheAnsarya/Nexen --body "Prompt for work:
<original user prompt that triggered this work>"
```

**For sub-issues created during analysis/audit:**
When creating sub-issues that stem from a parent issue or broader prompt, reference both:

```powershell
gh issue comment <number> --repo TheAnsarya/Nexen --body "Prompt for work:
Parent: #<parent-issue-number>
Original prompt: <the original user prompt that started the work>

Created during: <description of analysis/audit work>"
```

This provides:
- **Traceability** - Know where each issue originated
- **Context** - Understand the user's original intent
- **History** - Track AI-assisted development workflow
- **Lineage** - Track parent/child relationships for sub-issues

### ⚠️ MANDATORY: GitHub Comment Formatting

When posting issue/PR comments with `gh` CLI, always use real newline characters in the comment body.

- Do **NOT** post escaped literal sequences like ``\n`` in published comments
- Prefer multiline body input so GitHub renders proper paragraphs and lists
- If a malformed comment with literal ``\n`` is posted, edit it immediately to replace literals with true newlines

### ⚠️ MANDATORY: Commit/Push Autonomy

When the user asks to implement, fix, continue, or ship work, do not ask for commit confirmation.

- Stage only files relevant to the requested task
- Commit immediately with an issue reference
- Push immediately after commit
- Do not pause to ask "should I commit" or "which files should I include" unless the user explicitly asked to review first
- Exclude clearly unrelated modified files from the commit unless the user explicitly requests including them
- Do not pause due to a dirty working tree; stage the relevant files and continue
- Do not interrupt execution to ask about unrelated local modifications; leave them untouched and keep shipping requested work
- If the user explicitly says to stop asking about commit file selection or unexpected modified files, include those modified files and continue shipping without additional confirmation prompts

### ⚠️ MANDATORY: Stray File Commit Handling

If the user indicates that stray/unrelated modified files should be included, do not ask follow-up questions about file selection.

- Stage those modified files as requested and continue
- Commit and push without pausing for confirmation about stray files
- Assume these are intentional workspace changes unless the user says otherwise

### ⚠️ MANDATORY: Persistent Unexpected-File Preference

If the user says to never stop for unexpected modified files, treat that as an active preference for all subsequent work.

- Do not pause implementation because unrelated or unexpected files are modified
- Do not ask for file-selection confirmation when the user preference is explicit
- Include those modified files in commits and continue shipping

### ⚠️ MANDATORY: "Run Nexen" Means Launch Executable

When the user asks to "run nexen", "open nexen", or equivalent, launch the actual Nexen application executable (`Nexen.exe`).

- Do not open a web browser or documentation page for this request
- Prefer existing built executable paths under `bin/` or build output locations
- If no executable exists yet, build first, then launch `Nexen.exe`

## Coding Standards

### Indentation
- **TABS for indentation** - enforced by `.editorconfig`
- **Tab width: 4 spaces** - ALWAYS use 4-space-equivalent tabs
- **Applies to all file types** - C++, C#, JSON, YAML, Markdown, scripts, and config files
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

### ⚠️ MANDATORY: No Silent Version Downgrades

**Never downgrade versions without explicit user approval.**

- Do not reduce project/app/package/tool versions unless the user explicitly approves the downgrade
- Before proposing any downgrade, explain why it is needed and what compatibility impact it has
- Prefer forward upgrades and modern defaults whenever possible
- Keep internal compatibility versions separate from marketing/release versions when both are required

### ⚠️ MANDATORY: Always Prefer Modern Versions

- Always prefer the most modern stable versions of libraries, tooling, language features, and project settings
- Use current best practices unless a compatibility constraint is explicitly documented
- If an older version is required for compatibility, document the reason in code comments or docs

### ⚠️ Comment Safety Rule
**When adding or modifying comments, NEVER change the actual code.**

- Changes to comments must not alter code logic, structure, or formatting
- When adding XML documentation or inline comments, preserve all existing code exactly
- Verify code integrity after adding documentation

## Performance Guidelines

### ⚠️ ABSOLUTE RULE: Emulator Accuracy is Sacred

**NEVER sacrifice emulator accuracy for performance.** This is the #1 non-negotiable rule.

- **Every performance optimization MUST be correctness-preserving** — if a change could alter emulation behavior in any way, it is rejected
- **Run ALL tests after every change** — 1495+ C++ tests (Core.Tests.exe) and .NET tests must pass before any commit
- **Benchmark BEFORE and AFTER** — prove the improvement with data, not just theory
- **When in doubt, don't optimize** — a slower correct emulator is infinitely better than a faster broken one

#### Verification Checklist (for EVERY performance change):
1. ✅ All C++ tests pass (`Core.Tests.exe --gtest_brief=1`)
2. ✅ All .NET tests pass (`dotnet test --no-build -c Release`)
3. ✅ Benchmark shows measurable improvement (Google Benchmark with `--benchmark_repetitions=3`)
4. ✅ Change is semantics-preserving (same observable behavior)
5. ✅ No new warnings in build output
6. ✅ Hot path changes reviewed for correctness (CPU cores, PPU, memory handlers)

#### Focused Validation Commands (NotificationManager)

- Build Release x64:
	- `& "C:\Program Files\Microsoft Visual Studio\18\Insiders\MSBuild\Current\Bin\MSBuild.exe" Nexen.sln /p:Configuration=Release /p:Platform=x64 /t:Build /m /nologo /v:m`
- Run targeted NotificationManager gtests:
	- `.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=NotificationManagerTests.* --gtest_brief=1`

#### Types of Safe Performance Changes:
- **Data structure swaps** (e.g., `unordered_map` → sorted `vector`) — same semantics, different performance
- **Avoiding copies** (e.g., `const string&` instead of `string`) — identical behavior
- **Eliminating allocations** (e.g., `emplace_back` vs `push_back` of temporary) — identical behavior
- **Inlining** (e.g., moving small templates to headers) — identical behavior
- **Caching** (e.g., precomputing immutable data at init) — identical results

#### Types of DANGEROUS Changes (require extra scrutiny):
- Anything touching CPU instruction execution
- Memory mapping or bank switching logic
- PPU timing or rendering order
- Audio sample generation or mixing
- Interrupt handling or cycle counting
- Lock removal or reordering (thread safety)

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

## Release Process

### ⚠️ CRITICAL: Update README Download Links on Every Release

When publishing a new release (e.g., `v1.2.0`), you **MUST** update `README.md` to reference the new version in:

1. **Download Table Links** — Update all `v1.1.4` references to `v1.2.0`:
   - `Nexen-Windows-x64-v1.1.4.exe` → `Nexen-Windows-x64-v1.2.0.exe`
   - All Linux, macOS assets similarly
   - Both link text AND URL paths

2. **Quick Start Section** — Update version references in:
   - Download filenames (e.g., `Nexen-Linux-x64-v1.1.4.AppImage`)
   - Release page links

### Release Checklist

```powershell
# 1. Tag the release
git tag -a v1.2.0 -m "Release v1.2.0 - Description"
git push origin v1.2.0

# 2. Create GitHub release (CI auto-uploads assets)
gh release create v1.2.0 --repo TheAnsarya/Nexen --title "Nexen v1.2.0" --notes "Release notes..."

# 3. After CI completes, update README.md download links
# Search and replace: v1.1.4 → v1.2.0 (use regex or find-replace)

# 4. Commit README update
git add README.md
git commit -m "docs: update README download links to v1.2.0"
git push origin master
```

### Asset Naming Convention

Release assets include version numbers in filenames:

| Platform | Filename Pattern |
|----------|------------------|
| Windows | `Nexen-Windows-x64-vX.Y.Z.exe` |
| Windows AOT | `Nexen-Windows-x64-AoT-vX.Y.Z.exe` |
| Linux AppImage | `Nexen-Linux-x64-vX.Y.Z.AppImage` |
| Linux ARM64 AppImage | `Nexen-Linux-ARM64-vX.Y.Z.AppImage` |
| Linux Binary (clang) | `Nexen-Linux-x64-vX.Y.Z.tar.gz` |
| Linux Binary (gcc) | `Nexen-Linux-x64-gcc-vX.Y.Z.tar.gz` |
| Linux ARM64 Binary (clang) | `Nexen-Linux-ARM64-vX.Y.Z.tar.gz` |
| Linux ARM64 Binary (gcc) | `Nexen-Linux-ARM64-gcc-vX.Y.Z.tar.gz` |
| Linux AOT | `Nexen-Linux-x64-AoT-vX.Y.Z.tar.gz` |
| macOS | `Nexen-macOS-ARM64-vX.Y.Z.zip` |

## Documentation

### ⚠️ MANDATORY: Session Logs

**Always create a session log at the end of every conversation that involves code changes, issue creation, or significant research.** This is non-negotiable.

- File: `~docs/session-logs/YYYY-MM-DD-session-NN.md`
- Increment `NN` if a log already exists for that date
- Include: summary of work done, issues created/closed, commits made, files changed, and next steps
- Commit the session log as part of the final commit

### Paths

- Session logs: `~docs/session-logs/YYYY-MM-DD-session-NN.md`
- Plans: `~docs/plans/`
- Modernization tracking: `~docs/modernization/`
- `~docs/nexen-manual-prompts-log.txt` is user-maintained and write-protected for Copilot workflows: **never edit, rewrite, format, or truncate this file**.
- If `~docs/nexen-manual-prompts-log.txt` has user-authored changes in the working tree, **include it in commits as-is** (no content modifications by Copilot).

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

## Markdown Formatting

### ⚠️ MANDATORY: Fix Markdownlint Warnings

**Always fix markdownlint warnings when editing or creating markdown files.** This is non-negotiable.

Key rules to enforce:

- **MD022** — Blank lines above and below headings
- **MD031** — Blank lines around fenced code blocks
- **MD032** — Blank lines around lists (ordered and unordered)
- **MD047** — Files must end with a single newline character
- **MD007** — Disabled (tab indentation is 1 character, not 4)
- **MD010** — Disabled (hard tabs are REQUIRED per our indentation rules)

Always generate new markdown content with proper blank lines around headings, lists, and code fences so MD022/MD031/MD032 are satisfied on first write.

When generating new markdown content, **always include proper blank line spacing** around headings, lists, and code blocks.

### ⚠️ MANDATORY: Documentation Link-Tree

**Every markdown file in the repository must be reachable from `README.md` through a hierarchical link structure.**

- The main `README.md` must link to all documentation directories and key files
- Subdirectory docs should link back to parent and to sibling docs
- No orphan markdown files — if a `.md` file exists, it must be discoverable from the root README
- When adding new documentation, always update `README.md` with a link to it
- Internal docs (`~docs/`) should have their own index linked from the main README

## Related Projects

- **Pansy** - Metadata format for disassembly data
- **Peony** - Disassembler using Pansy format
- **GameInfo** - ROM hacking toolkit

