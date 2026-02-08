# Menu System Rewrite Plan

## Executive Summary

Complete rewrite of the Nexen menu system to fix fundamental issues with enable/disable state management. The current system has timing issues where menu items are bound before `Update()` is called, causing incorrect initial states.

## Current Architecture Problems

### Root Cause Analysis

1. **XAML Binds Before Update**: In [NexenStyles.xaml](../../UI/Styles/NexenStyles.xaml#L177-L184), menu items bind to `Enabled` property immediately on render, but `Update()` is only called when the submenu opens.

2. **IsShortcutAllowed is too restrictive**: The C++ function `EmuApi.IsShortcutAllowed()` checks `isRunning` for ALL shortcuts, making "Open" disabled when no ROM is loaded.

3. **Inconsistent Enable Logic**: Some items use `IsEnabled` func, some rely on `IsShortcutAllowed()`, some use neither - no clear pattern.

4. **No Global State**: Each menu item independently queries state, leading to redundant checks and inconsistent behavior.

### Current Class Hierarchy

```
BaseMenuAction (abstract)
├── MainMenuAction (for main menu items, uses EmulatorShortcut)
├── ContextMenuAction (for context menus, uses DbgShortKeys)
└── ContextMenuSeparator (divider)
```

## New Architecture

### Design Goals

1. **Simple Enable Rules**: Clear categories - always enabled, requires ROM, requires specific state
2. **Global State Service**: Single source of truth for emulator state
3. **Immediate State**: Menu items show correct state on first render
4. **Every Item Has an Action**: No menu item without a command attached
5. **Modern C# Patterns**: Use C# 13 features where appropriate

### New Class Hierarchy

```
IMenuAction (interface)
├── MenuActionBase (abstract base)
│   ├── SimpleMenuAction (always enabled, custom action)
│   ├── ShortcutMenuAction (linked to EmulatorShortcut)
│   └── DebugMenuAction (linked to DebuggerShortcut)
└── MenuSeparator (divider, never enabled)
```

### Global Emulator State Service

Create `EmulatorState` service that provides reactive properties:

```csharp
public sealed class EmulatorState : ReactiveObject {
    // Singleton for global access
    public static EmulatorState Instance { get; } = new();
    
    // Core state flags - updated by emulator events
    [Reactive] public bool IsRomLoaded { get; private set; }
    [Reactive] public bool IsRunning { get; private set; }
    [Reactive] public bool IsPaused { get; private set; }
    
    // Recording/Playback state
    [Reactive] public bool IsMoviePlaying { get; private set; }
    [Reactive] public bool IsMovieRecording { get; private set; }
    [Reactive] public bool IsAviRecording { get; private set; }
    [Reactive] public bool IsWaveRecording { get; private set; }
    
    // Netplay state
    [Reactive] public bool IsNetplayClient { get; private set; }
    [Reactive] public bool IsNetplayServer { get; private set; }
    [Reactive] public bool IsNetplayActive => IsNetplayClient || IsNetplayServer;
    
    // Debug state
    [Reactive] public bool HasSaveStates { get; private set; }
    [Reactive] public bool HasRecentFiles { get; private set; }
    
    // ROM-specific info
    [Reactive] public ConsoleType ConsoleType { get; private set; }
    [Reactive] public RomFormat RomFormat { get; private set; }
    
    // Computed properties for common checks
    public bool CanSaveState => IsRomLoaded && !IsNetplayClient;
    public bool CanLoadState => IsRomLoaded && !IsMoviePlaying;
    public bool CanRecord => IsRomLoaded && !IsMovieRecording && !IsMoviePlaying;
}
```

### Enable Categories

| Category | Condition | Examples |
|----------|-----------|----------|
| `AlwaysEnabled` | No condition | Open, Exit, Preferences, About |
| `RequiresRom` | `IsRomLoaded` | Pause, Reset, Power Cycle, Save State |
| `RequiresRunning` | `IsRunning` | Debug tools, Cheats |
| `RequiresState` | Custom | Load State (needs save file), Record (not already recording) |

### New Menu Action Interface

```csharp
public interface IMenuAction : INotifyPropertyChanged {
    string Name { get; }
    string? ShortcutText { get; }
    Image? Icon { get; }
    bool Enabled { get; }
    bool Visible { get; }
    ICommand Command { get; }
    IReadOnlyList<IMenuAction>? SubItems { get; }
}
```

## Menu Item Specifications

### File Menu

| Item | Category | Condition |
|------|----------|-----------|
| Open | `AlwaysEnabled` | - |
| Save State | `RequiresRom` | `!IsNetplayClient` |
| Load State | `RequiresRom` | `!IsMoviePlaying` |
| Load Last Session | `RequiresRom` | File exists |
| Quick Save Timestamped | `RequiresRom` | - |
| Open Save State Picker | `RequiresRom` | `HasSaveStates` |
| Recent Files | `AlwaysEnabled` | `HasRecentFiles` |
| Exit | `AlwaysEnabled` | - |

### Game Menu

| Item | Category | Condition |
|------|----------|-----------|
| Pause/Resume | `RequiresRom` | - |
| Reset | `RequiresRom` | - |
| Power Cycle | `RequiresRom` | - |
| Reload ROM | `RequiresRom` | - |
| Power Off | `RequiresRom` | - |
| Game Config | `RequiresRom` | Console supports |

### Options Menu

| Item | Category | Condition |
|------|----------|-----------|
| Speed | `AlwaysEnabled` | - |
| Video Scale | `AlwaysEnabled` | - |
| Video Filter | `AlwaysEnabled` | - |
| Aspect Ratio | `AlwaysEnabled` | - |
| Region | `RequiresRom` | Console supports |
| Audio/Video/Input/Emulation | `AlwaysEnabled` | - |
| Console configs (NES/SNES/etc) | `AlwaysEnabled` | - |
| Preferences | `AlwaysEnabled` | - |

### Tools Menu

| Item | Category | Condition |
|------|----------|-----------|
| Cheats | `RequiresRom` | Console supports |
| History Viewer | `RequiresRom` | History enabled |
| TAS Editor | `RequiresRom` | - |
| Movies → Play/Record/Stop | `RequiresRom` | Various |
| NetPlay | `AlwaysEnabled` | Various |
| Sound/Video Recorder | `RequiresRom` | Various |
| HD Packs | `RequiresRom` | NES only |
| Log Window | `AlwaysEnabled` | - |
| Take Screenshot | `RequiresRom` | - |

### Debug Menu

| Item | Category | Condition |
|------|----------|-----------|
| All debugger windows | `RequiresRom` | Various console checks |
| Debug Settings | `AlwaysEnabled` | - |

### Help Menu

| Item | Category | Condition |
|------|----------|-----------|
| All items | `AlwaysEnabled` | - |

## File Changes Required

### New Files

- `UI/Services/EmulatorState.cs` - Global state service
- `UI/Menus/IMenuAction.cs` - Interface
- `UI/Menus/MenuActionBase.cs` - Abstract base
- `UI/Menus/SimpleMenuAction.cs` - Always enabled actions
- `UI/Menus/ShortcutMenuAction.cs` - EmulatorShortcut actions
- `UI/Menus/DebugMenuAction.cs` - Debug shortcut actions
- `UI/Menus/MenuSeparator.cs` - Separator

### Modified Files

- `UI/ViewModels/MainMenuViewModel.cs` - Use new classes
- `UI/Controls/NexenMenu.cs` - May need simplification
- `UI/Styles/NexenStyles.xaml` - Verify bindings still work

### Removed/Deprecated

- `UI/Debugger/Utilities/ContextMenuAction.cs` - Old classes (keep for debugger context menus initially)

## Implementation Steps

1. Create EmulatorState service
2. Create new menu action classes
3. Update MainMenuViewModel to use new classes for File menu
4. Test File menu thoroughly
5. Migrate remaining menus one by one
6. Remove old code after full migration
7. Update context menus in debugger (separate task)

## Testing Checklist
- [ ] Open menu enabled with no ROM loaded
- [ ] Exit menu enabled with no ROM loaded
- [ ] Save State disabled with no ROM loaded
- [ ] Load State disabled with no ROM loaded  
- [ ] Save State enabled after ROM loaded
- [ ] Load State enabled after ROM loaded
- [ ] Options menu items work correctly
- [ ] Debug menu items work correctly
