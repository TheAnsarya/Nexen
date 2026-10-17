# Plan: Fix Nexen #1176 — Instance Management and First-Run Issues

## Problem Statement

Users report that after completing the setup wizard, Nexen "vanishes" and:

1. The setup wizard process starts a new Nexen instance via `Process.Start()`, but the old process may not have released the mutex yet → new instance sees mutex held → exits silently
2. If a zombie Nexen process is left in Task Manager, any future launch attempt is silently blocked by the single-instance check

## Root Cause Analysis

### Issue A: Setup Wizard Restart Race Condition

In `Program.cs` lines 104-109:

```csharp
if (File.Exists(portableConfig) || File.Exists(documentsConfig)) {
	Process.Start(Program.ExePath);  // ← No error handling, no wait
}
return 0;  // ← May not release mutex fast enough
```

The setup wizard runs WITHOUT SingleInstance (it returns before reaching that code), but:
- `Process.Start()` has no error handling
- The new process may start before the old one fully exits
- No `ProcessStartInfo` configuration

### Issue B: Silent Exit on Existing Instance

In `Program.cs` lines 131-135:

```csharp
if (instance.FirstInstance) {
	// ... start app
} else {
	Log.Info("Another instance is already running");
	// ← SILENT EXIT — no dialog, no feedback
}
```

## Solution Design

### Fix 1: Add "Already Running" Dialog

When a second instance is detected, show a dialog with two options:
- **"Close and Restart Nexen"** — Kill the existing process and restart
- **"Leave Current Version Running"** — Exit the new instance gracefully

Implementation:
- Before the normal Avalonia app starts, show a lightweight message box
- Use Avalonia's message box system (already available via `NexenMsgBox`)
- Need a minimal Avalonia app to host the dialog (since we haven't called `BuildAvaloniaApp().Start...` yet)

### Fix 2: Improve Setup Wizard Restart

- Add `try/catch` around `Process.Start()`
- Use `ProcessStartInfo` with `UseShellExecute = true` for reliability
- Add a small delay or release mechanism before restart

## Files to Modify

1. `UI/Program.cs` — Add dialog for existing instance, fix restart
2. `UI/Utilities/SingleInstance.cs` — Add method to kill existing instance
3. `UI/Localization/resources.en.xml` — Add localization strings

## Testing

- Build Release x64
- Run Nexen normally
- Try to open a second Nexen while first is running → should see dialog
- Kill first instance from Task Manager, run Nexen → should start normally
- Delete config files, run → setup wizard → confirm → should restart cleanly
