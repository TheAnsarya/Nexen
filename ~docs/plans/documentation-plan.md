# Nexen C# Documentation Plan

**Created:** 2026-02-01
**Status:** In Progress

## Overview

This plan covers adding comprehensive XML documentation comments and inline code comments to all C# files in the Nexen .NET projects. The goal is to improve code maintainability, enable IntelliSense documentation, and support automated documentation generation.

## ⚠️ Critical Rule

**When adding or modifying comments, NEVER change the actual code.**

- Changes to comments must not alter code logic, structure, or formatting
- When adding XML documentation or inline comments, preserve all existing code exactly
- Verify code integrity after adding documentation

## File Categories

### Priority 1: Core Infrastructure (High Impact)

Files that are foundational to the codebase:

| File | Status | Description |
| ------ | -------- | ------------- |
| `UI/Config/ConfigManager.cs` | 🔴 | Central configuration management |
| `UI/Config/Configuration.cs` | 🔴 | Main configuration class |
| `UI/Config/BaseConfig.cs` | 🔴 | Configuration base class |
| `UI/Program.cs` | 🔴 | Application entry point |
| `UI/App.axaml.cs` | 🔴 | Avalonia app initialization |
| `UI/NexenWindow.cs` | 🔴 | Base window class |

### Priority 2: Pansy Integration (Project-Specific)

Our custom Pansy metadata format integration:

| File | Status | Description |
| ------ | -------- | ------------- |
| `UI/Debugger/Labels/PansyExporter.cs` | 🟡 | Pansy file export (partial docs) |
| `UI/Debugger/Labels/BackgroundPansyExporter.cs` | 🟢 | Background CDL recording |
| `UI/Debugger/Labels/PansyImporter.cs` | 🔴 | Pansy file import |
| `UI/Debugger/Labels/SyncManager.cs` | 🔴 | Symbol synchronization |
| `UI/Debugger/Labels/DebugFolderManager.cs` | 🔴 | Debug folder management |
| `UI/Debugger/Labels/NexenLabelFile.cs` | 🔴 | Nexen native label format |
| `UI/Debugger/Integration/DbgToPansyConverter.cs` | 🔴 | DBG to Pansy conversion |

### Priority 3: Label & Symbol Management

Core debugger label functionality:

| File | Status | Description |
| ------ | -------- | ------------- |
| `UI/Debugger/Labels/CodeLabel.cs` | 🔴 | Code label data model |
| `UI/Debugger/Labels/LabelManager.cs` | 🔴 | Label storage and lookup |
| `UI/Debugger/Labels/DefaultLabelHelper.cs` | 🔴 | Default label generation |

### Priority 4: Debugger Utilities

Helper classes for debugger features:

| File | Status | Description |
| ------ | -------- | ------------- |
| `UI/Debugger/Utilities/DebugWorkspaceManager.cs` | 🔴 | Workspace state management |
| `UI/Debugger/Utilities/DebugWindowManager.cs` | 🔴 | Debugger window management |
| `UI/Debugger/Utilities/DebugShortcutManager.cs` | 🔴 | Keyboard shortcuts |
| `UI/Debugger/Utilities/HexUtilities.cs` | 🔴 | Hex conversion utilities |
| `UI/Debugger/Utilities/SearchHelper.cs` | 🔴 | Search functionality |
| `UI/Debugger/Utilities/NavigationHistory.cs` | 🔴 | Back/forward navigation |

### Priority 5: Interop Layer

C++/C# interop definitions:

| File | Status | Description |
| ------ | -------- | ------------- |
| `UI/Interop/DebugApi.cs` | 🔴 | Debug API P/Invoke |
| `UI/Interop/EmuApi.cs` | 🔴 | Emulation API P/Invoke |
| `UI/Interop/ConfigApi.cs` | 🔴 | Config API P/Invoke |
| `UI/Interop/BaseState.cs` | 🔴 | Console state structures |

### Priority 6: ViewModels

MVVM view models:

| File | Status | Description |
| ------ | -------- | ------------- |
| `UI/ViewModels/MainWindowViewModel.cs` | 🔴 | Main window VM |
| `UI/ViewModels/ViewModelBase.cs` | 🔴 | ViewModel base class |
| `UI/Debugger/ViewModels/DebuggerWindowViewModel.cs` | 🔴 | Debugger VM |
| `UI/Debugger/ViewModels/LabelListViewModel.cs` | 🔴 | Label list VM |

### Priority 7: UI Utilities

Helper classes for UI:

| File | Status | Description |
| ------ | -------- | ------------- |
| `UI/Utilities/JsonHelper.cs` | 🔴 | JSON serialization |
| `UI/Utilities/FileHelper.cs` | 🔴 | File operations |
| `UI/Utilities/FolderHelper.cs` | 🔴 | Folder management |
| `UI/Utilities/ColorHelper.cs` | 🔴 | Color conversion |

## Documentation Standards

### XML Documentation Format

```csharp
/// <summary>
/// Brief description of the class/method/property.
/// </summary>
/// <remarks>
/// Additional details, usage notes, or implementation notes.
/// </remarks>
/// <param name="paramName">Description of the parameter.</param>
/// <returns>Description of the return value.</returns>
/// <exception cref="ArgumentNullException">When parameter is null.</exception>
/// <example>
/// <code>
/// var result = MethodName(param);
/// </code>
/// </example>
public ReturnType MethodName(ParamType paramName) { ... }
```

### Inline Comment Guidelines

- Use `//` for single-line comments
- Explain "why", not "what" (the code shows what)
- Comment complex algorithms and business logic
- Mark performance-critical code with `// Performance:`
- Mark TODO items with `// TODO:`
- Mark workarounds with `// HACK:` or `// WORKAROUND:`

## Progress Tracking

- 🔴 Not started
- 🟡 Partial (some documentation exists)
- 🟢 Complete

## Related Issues

- GitHub Issue: [To be created]

## Estimated Effort

- Priority 1 (Core): ~4 hours
- Priority 2 (Pansy): ~3 hours
- Priority 3 (Labels): ~2 hours
- Priority 4 (Utilities): ~3 hours
- Priority 5 (Interop): ~2 hours
- Priority 6 (ViewModels): ~4 hours
- Priority 7 (UI Utilities): ~2 hours

**Total: ~20 hours**

