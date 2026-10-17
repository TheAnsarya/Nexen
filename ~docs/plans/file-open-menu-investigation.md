# File Open Menu Investigation

## Problem Statement

The File → Open menu item crashes Nexen when a file is selected. This has been attempted multiple times with different approaches, all failing.

## What We Know Works

1. **Recent Games screen** - Clicking a game on the main screen loads it
2. **Drag and drop** - Dropping a ROM file loads it
3. **Command line** - Passing a ROM file as argument loads it
4. **Other menu items** - Settings→Input, Settings→Video etc. all work with OnClick

## What Crashes

- File → Open → Select a ROM → Crash

## Investigation Needed

### 1. How do working ROM load paths work?

#### Recent Games (RecentGamesViewModel)

Need to check how clicking a recent game triggers ROM loading.

#### Drag and Drop (MainWindow)

Need to check how drag-drop triggers ROM loading.

#### Command Line

Need to check how command line args trigger ROM loading.

### 2. What's unique about the File Open path?

- Uses FileDialogHelper.OpenFile (async)
- Calls LoadRomHelper.LoadFile
- Runs from menu click context

### 3. Potential Issues to Investigate

- Async context issues
- Thread marshaling issues
- Window reference issues
- Exception swallowing in async void
- Menu command binding lifecycle

## Files to Examine

- UI/ViewModels/RecentGamesViewModel.cs - How recent games load
- UI/Windows/MainWindow.axaml.cs - Drag/drop, command line
- UI/Utilities/LoadRomHelper.cs - The actual load logic
- UI/Utilities/FileDialogHelper.cs - File dialog implementation
- UI/Menus/MenuActionBase.cs - Menu click handling

## Next Steps

1. Find a working code path that loads ROMs
2. Trace exactly how it works
3. Compare to the broken File Open path
4. Identify the exact difference that causes the crash
