# Joypad Interrupt Feature - Implementation Plan

## Issue: #249 [12.2] Joypad interrupt during movie playback

## Overview

When playing a `.nexen-movie` file in playback mode, pressing a joypad button should interrupt playback and present a dialog with options to fork, edit, or continue.

## User Flow

1. User opens a movie file (`.nexen-movie`, `.bk2`, etc.)
2. Movie starts playing back in the emulator
3. User presses a joypad button (differing from expected movie input)
4. **Immediately pause** before the frame executes with the wrong input
5. Show the interrupt dialog with large card-style buttons:
   - **Fork**: Create a branch from here and start recording
   - **Edit**: Open TAS Editor at this frame for manual editing
   - **Continue**: Resume playback, ignoring the pressed input

## Architecture

### Components to Create

1. **PlaybackInterruptDialog.axaml** - The UI dialog with card buttons
2. **PlaybackInterruptViewModel.cs** - Dialog logic

### Components to Modify

1. **TasEditorViewModel.cs** - Add interrupt detection logic
2. **InputRecorder.cs** - Support forking from playback
3. **MainWindow.axaml.cs** - Integration with interrupt detection
4. **MovieManager** (Core) - Hook into input polling

## Implementation Steps

### Step 1: Create Interrupt Dialog UI

```axaml
<!-- UI/Windows/PlaybackInterruptDialog.axaml -->
<Window Title="Playback Interrupted">
    <Panel Background="#80000000">
        <StackPanel HorizontalAlignment="Center" VerticalAlignment="Center">
            <TextBlock Text="Input detected during playback" />
            <TextBlock Text="Frame: {frame}" />
            
            <!-- Card buttons -->
            <UniformGrid Columns="3">
                <Button Classes="card-button" Content="🔀 Fork" Command="{Binding ForkCommand}" />
                <Button Classes="card-button" Content="✏️ Edit" Command="{Binding EditCommand}" />
                <Button Classes="card-button" Content="▶️ Continue" Command="{Binding ContinueCommand}" />
            </UniformGrid>
        </StackPanel>
    </Panel>
</Window>
```

### Step 2: Implement Input Mismatch Detection

During movie playback, before each frame executes:

```csharp
// In playback loop
public void CheckInputMismatch(int frame, ControllerInput[] movieInput, ControllerInput[] userInput)
{
    for (int i = 0; i < movieInput.Length; i++)
    {
        if (userInput[i].ButtonBits != 0 && userInput[i].ButtonBits != movieInput[i].ButtonBits)
        {
            // User pressed something different than the movie
            OnPlaybackInterrupt(frame, movieInput, userInput);
            return;
        }
    }
}
```

### Step 3: Handle Interrupt Actions

```csharp
public async Task<PlaybackAction> HandleInterrupt(int frame)
{
    // Pause emulation immediately
    EmuApi.Pause();
    
    // Show dialog
    var dialog = new PlaybackInterruptDialog(frame);
    var result = await dialog.ShowDialog<PlaybackAction>(MainWindow);
    
    switch (result)
    {
        case PlaybackAction.Fork:
            // Create branch from current state
            Recorder.CreateBranch($"Fork at frame {frame}");
            Recorder.StartRecording(RecordingMode.Overwrite, frame);
            break;
            
        case PlaybackAction.Edit:
            // Open TAS Editor at this frame
            ShowTasEditor();
            TasEditor.SelectedFrameIndex = frame;
            break;
            
        case PlaybackAction.Continue:
            // Resume playback
            EmuApi.Resume();
            break;
    }
    
    return result;
}
```

### Step 4: Integration Points

1. **MovieManager.cpp/h** - Add callback for input mismatch detection
2. **InputProvider** - Expose user input during playback
3. **MainWindow** - Show dialog when interrupt triggered

## Technical Considerations

### Thread Safety

- Emulation runs on a different thread than UI
- Must pause emulation before showing dialog
- Use dispatcher to show dialog on UI thread

### Performance

- Only check input when movie is playing (not recording)
- Minimal overhead for the comparison

### Savestate Integration

- Fork action needs to save current state
- Greenzone should have state for current frame

## Testing Plan

1. Unit tests for input mismatch detection
2. Integration test for fork workflow
3. Test dialog behavior on various screen sizes
4. Test all three action paths

## Future Enhancements

- Show visual diff of expected vs actual input
- Option to auto-insert the pressed input
- Configurable interrupt sensitivity (ignore d-pad, etc.)
- Suppress interrupts for specific frames/ranges
