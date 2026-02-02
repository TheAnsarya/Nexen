# 🎫 Nexen Modernization Issues

This document tracks the GitHub issues and epics for the Nexen modernization project.

## ✅ Completion Status

### Phase 1: .NET / UI Modernization (Complete)

| Epic | Status | Notes |
| ------ | -------- | ------- |
| Epic 1: .NET 10 Migration | ✅ **Complete** | Upgraded to .NET 10.0 |
| Epic 2: Avalonia Update | ✅ **Complete** | Updated to 11.3.11 |
| Epic 3: Built-in Libraries | ✅ **Complete** | System.IO.Hashing |
| Epic 4: Comprehensive Testing | ✅ **Complete** | 24 tests |
| Epic 5: Lua Runtime Update | ✅ **Complete** | Lua 5.4.4 → 5.4.8 |
| Epic 6: Code Modernization | ✅ **Complete** | K&R, tabs, formatting |
| Epic 7: Documentation | ✅ **Complete** | Updated all docs |

### Phase 2: C++ Core Modernization (Planned)

| Epic | Status | Notes |
| ------ | -------- | ------- |
| Epic 8: Build System | 📋 **Planned** | VS2026, C++23, clang-tidy |
| Epic 9: Testing Infrastructure | 📋 **Planned** | Google Test/Catch2 |
| Epic 10: Memory Safety | 📋 **Planned** | Smart pointers, std::span |
| Epic 11: Standard Library | 📋 **Planned** | ranges, format, filesystem |
| Epic 12: Performance | 📋 **Planned** | Profiling, SIMD |
| Epic 13: Documentation | 📋 **Planned** | Doxygen, API docs |

**See:** [CPP-ISSUES-TRACKING.md](CPP-ISSUES-TRACKING.md) for detailed C++ issues

## 📋 Epics

### Epic 1: .NET 10 Migration ✅ COMPLETE
**Status:** Complete
**Priority:** HIGH

**Completed:**

- ✅ All .csproj files target net10.0
- ✅ Project builds without errors on Windows
- ✅ All existing functionality works correctly

---

### Epic 2: Avalonia Update ✅ COMPLETE
**Status:** Complete
**Priority:** HIGH

**Package Updates Completed:**

- ✅ Avalonia 11.3.1 → 11.3.9
- ✅ Avalonia.Desktop 11.3.1 → 11.3.9
- ✅ Avalonia.Controls.ColorPicker 11.3.1 → 11.3.9
- ✅ Avalonia.Diagnostics 11.3.1 → 11.3.9
- ✅ Avalonia.ReactiveUI 11.3.1 → 11.3.9
- ✅ Avalonia.Themes.Fluent 11.3.1 → 11.3.9

---

### Epic 3: Built-in Libraries Migration ✅ COMPLETE
**Status:** Complete
**Priority:** MEDIUM

**Completed:**

- ✅ CRC32: Custom → System.IO.Hashing.Crc32
- ✅ PansyExporter updated
- ✅ Identical output verified

---

### Epic 4: Comprehensive Testing ✅ COMPLETE
**Status:** Complete
**Priority:** HIGH

**Coverage:**

- ✅ PansyExporter: 24 tests
- ✅ BackgroundPansyExporter: Tested
- ✅ xUnit framework set up

---

### Epic 5: Lua Runtime Update ✅ COMPLETE
**Status:** Complete
**Priority:** MEDIUM

**Completed:**

- ✅ Analyzed current Lua version (5.4.4, released 2022)
- ✅ Downloaded Lua 5.4.8 (released June 2025)
- ✅ Updated 42 core Lua source files
- ✅ Preserved luasocket extension and lbitlib.c
- ✅ Verified build succeeds
- ✅ Committed: `28c5711f`

---

### Epic 6: Code Modernization ✅ COMPLETE
**Status:** Complete
**Priority:** MEDIUM

**Completed:**

- ✅ K&R brace style (opening braces at end of line)
- ✅ Tabs for indentation (4-space width)
- ✅ UTF-8 encoding with CRLF line endings
- ✅ Final newlines on all files
- ✅ Pattern matching enabled
- ✅ Collection expressions preferred
- ✅ 500+ files formatted with dotnet format
- ✅ Comprehensive .editorconfig merged

---

### Epic 7: Documentation & CI/CD ✅ COMPLETE
**Status:** Complete
**Priority:** LOW

**Completed:**

- ✅ MODERNIZATION-ROADMAP.md updated
- ✅ ISSUES-TRACKING.md updated
- ✅ Session logs created
- ✅ All phases documented---

## 🏷️ Labels

| Label | Description | Color |
| ------- | ------------- | ------- |
| `modernization` | Part of modernization effort | Blue |
| `epic` | Epic/parent issue | Purple |
| `.net-10` | .NET 10 related | Green |
| `avalonia` | Avalonia UI related | Cyan |
| `testing` | Test related | Yellow |
| `complete` | Completed task | Green |

---

## 📅 Completion Timeline

| Date | Work Completed |
| ------ | ---------------- |
| Jan 26, 2026 | .NET 10 upgrade, Avalonia 11.3.9, System.IO.Hashing, 24 tests |
| Jan 27, 2026 | K&R formatting, .editorconfig merge, 500+ files formatted, documentation |
| Jan 27, 2026 | Lua runtime 5.4.4 → 5.4.8 |
| Jan 28, 2026 | Avalonia 11.3.11, Dock 11.3.9, VS2026 tooling, C++ roadmap & issues |

---

*Last Updated: January 28, 2026*
