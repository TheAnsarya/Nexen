# Background CDL Recording Plan

**Created:** 2026-01-26
**Completed:** 2026-01-26 09:00 UTC
**Status:** ✅ IMPLEMENTED
**Commit:** `e95a7858`
**Priority:** High

## Overview

Currently, Pansy export only captures CDL (Code/Data Logger) data when the debugger is opened. This requires users to manually open the debugger to start recording execution data.

**Goal:** CDL recording should start automatically when a ROM loads and save periodically in the background, so users can simply play games and have their execution paths documented automatically.

## Current Behavior

1. User loads ROM
2. CDL is idle (no recording unless debugger opened)
3. User opens debugger (F12)
4. CDL starts recording executed code/data
5. User closes debugger
6. Pansy file exports with recorded data

## Desired Behavior

1. User loads ROM
2. CDL automatically starts recording (if option enabled)
3. User plays game normally
4. Pansy file auto-saves periodically (every N minutes)
5. User closes Nexen
6. Final Pansy export on shutdown

## Configuration Options

### New Settings in IntegrationConfig

```csharp
// Enable background CDL recording without debugger
[Reactive] public bool BackgroundCdlRecording { get; set; } = false;

// Auto-save interval in minutes (0 = disabled)
[Reactive] public int AutoSaveIntervalMinutes { get; set; } = 5;

// Save Pansy on ROM unload
[Reactive] public bool SavePansyOnRomUnload { get; set; } = true;
```

## Implementation Steps

### Step 1: Enable CDL Without Debugger

Location: `UI/Debugger/Utilities/DebugWorkspaceManager.cs` or new file

- Hook into ROM load event
- Start CDL recording via `DebugApi.SetCdlData()` or equivalent
- Don't require debugger window to be open

### Step 2: Background Save Timer

Location: New file `UI/Debugger/Labels/BackgroundPansyExporter.cs`

```csharp
public static class BackgroundPansyExporter
{
	private static Timer? _autoSaveTimer;
	private static RomInfo _currentRomInfo;
	
	public static void OnRomLoaded(RomInfo romInfo)
	{
		_currentRomInfo = romInfo;
		
		if (ConfigManager.Config.Debug.Integration.BackgroundCdlRecording)
		{
			StartCdlRecording();
			StartAutoSaveTimer();
		}
	}
	
	public static void OnRomUnloaded()
	{
		StopAutoSaveTimer();
		
		if (ConfigManager.Config.Debug.Integration.SavePansyOnRomUnload)
		{
			ExportPansy();
		}
	}
	
	private static void StartAutoSaveTimer()
	{
		int intervalMs = ConfigManager.Config.Debug.Integration.AutoSaveIntervalMinutes * 60 * 1000;
		if (intervalMs > 0)
		{
			_autoSaveTimer = new Timer(OnAutoSaveTick, null, intervalMs, intervalMs);
		}
	}
	
	private static void OnAutoSaveTick(object? state)
	{
		// Run on UI thread to avoid threading issues
		Dispatcher.UIThread.Post(() => ExportPansy());
	}
	
	private static void ExportPansy()
	{
		if (string.IsNullOrEmpty(_currentRomInfo.RomPath)) return;
		
		var memoryType = _currentRomInfo.ConsoleType.GetMainCpuType().GetPrgRomMemoryType();
		PansyExporter.AutoExport(_currentRomInfo, memoryType);
	}
}
```

### Step 3: Hook ROM Load/Unload Events

Location: `UI/Windows/MainWindow.axaml.cs` or `UI/Utilities/LoadRomHelper.cs`

Find ROM load handlers and add:

```csharp
BackgroundPansyExporter.OnRomLoaded(romInfo);
```

Find ROM unload handlers and add:

```csharp
BackgroundPansyExporter.OnRomUnloaded();
```

### Step 4: Enable CDL Recording Without Debugger

The CDL is normally controlled by the debugger. Need to:

1. Call `DebugApi.SetDebuggerFlag()` to enable CDL logging
2. May need to call `DebugApi.Step()` periodically or hook emulation

Research needed on Nexen's CDL API.

### Step 5: UI Configuration

Location: `UI/Debugger/Windows/DebuggerConfigWindow.axaml`

Add new section:

```xml
<c:OptionSection Header="{l:Translate lblBackgroundRecording}">
	<CheckBox
		IsChecked="{Binding Integration.BackgroundCdlRecording}"
		Content="{l:Translate chkBackgroundCdlRecording}"
	/>
	<StackPanel Orientation="Horizontal" Margin="20 5 0 0">
		<TextBlock Text="{l:Translate lblAutoSaveInterval}" VerticalAlignment="Center" />
		<NumericUpDown Value="{Binding Integration.AutoSaveIntervalMinutes}" 
					   Minimum="0" Maximum="60" Width="80" Margin="5 0" />
		<TextBlock Text="{l:Translate lblMinutes}" VerticalAlignment="Center" />
	</StackPanel>
	<CheckBox
		IsChecked="{Binding Integration.SavePansyOnRomUnload}"
		Content="{l:Translate chkSavePansyOnRomUnload}"
		Margin="20 5 0 0"
	/>
</c:OptionSection>
```

### Step 6: Localization Strings

Location: `UI/Localization/resources.en.xml`

```xml
<Entry key="lblBackgroundRecording">Background Recording</Entry>
<Entry key="chkBackgroundCdlRecording">Record CDL data in background (no debugger required)</Entry>
<Entry key="lblAutoSaveInterval">Auto-save interval:</Entry>
<Entry key="lblMinutes">minutes (0 = disabled)</Entry>
<Entry key="chkSavePansyOnRomUnload">Save Pansy file when ROM is unloaded</Entry>
```

## Testing Plan

1. **Basic Recording:**
   - Enable background recording
   - Load ROM, DON'T open debugger
   - Play for 5+ minutes
   - Close Nexen
   - Verify .pansy file exists with CDL data

2. **Auto-Save:**
   - Set auto-save to 1 minute
   - Load ROM and play
   - Check file timestamps update every minute

3. **ROM Switch:**
   - Load ROM A, play briefly
   - Load ROM B (triggers unload of A)
   - Verify ROM A's pansy was saved
   - Verify ROM B starts fresh recording

4. **Debugger Interaction:**
   - Verify background recording works alongside debugger
   - Verify no conflicts or double-saving

## Risks

1. **Performance Impact:** CDL recording has minimal overhead, but verify
2. **File Locking:** Ensure periodic saves don't conflict with manual exports
3. **Threading:** Auto-save timer must dispatch to UI thread for file I/O
4. **Memory:** CDL data could grow large for long play sessions

## Timeline

- Day 1: Research CDL API, implement basic recording
- Day 2: Implement auto-save timer and ROM hooks
- Day 3: UI configuration and localization
- Day 4: Testing and bug fixes

## Related Issues

- Nexen #1: Background CDL Recording
- Pansy integration: Auto-export enhancements
