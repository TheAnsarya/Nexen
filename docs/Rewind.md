# Rewind Guide

## Overview

Rewind lets you move backward through recent gameplay to retry a section, inspect outcomes, or adjust inputs.

## Basic Controls

- `Backspace`: rewind while held or step backward based on active input mode.
- `Escape`: pause and resume while planning rewind and frame-step sequences.
- Backtick key: frame advance when paused.

## Typical Usage

1. Pause when needed.
2. Rewind to a stable point.
3. Resume or frame-advance to test an alternate action.
4. Save state when you find a better route.

## GUI Tips

- Combine rewind with save states for repeatable testing loops.
- Use rewind before creating a TAS branch to reduce setup time.
- Use the movie timeline and TAS editor for deterministic revisions.

## Panel Walkthrough (Screenshot Anchors)

### Walkthrough A: Rewind to a Checkpoint and Retry

1. Pause gameplay with `Escape` in the main window. Expected result: emulation pauses on the current frame.
2. Hold or tap `Backspace` to rewind to the desired frame range. Expected result: frame state moves backward and lands near the target retry point.
3. Resume from the rewound position. Expected result: gameplay continues from the selected rewind frame.
4. Save a new state once the retry route is validated. Expected result: the validated route is persisted as a reusable save point.

| Step | Panel | Screenshot Anchor ID | Capture Target |
|---|---|---|---|
| 1 | Main Emulation Window (Paused) | `rewind-a-01-paused` | `docs/screenshots/rewind/a-01-paused.png` |
| 2 | Main Emulation Window (Rewind In Progress) | `rewind-a-02-rewinding` | `docs/screenshots/rewind/a-02-rewinding.png` |
| 3 | Main Emulation Window (Post-Rewind Resume) | `rewind-a-03-post-rewind` | `docs/screenshots/rewind/a-03-post-rewind.png` |

### Walkthrough B: Frame-Step Validation

1. Pause gameplay. Expected result: the frame is locked for deterministic step analysis.
2. Use the backtick key to frame-advance while observing timing. Expected result: each key press advances exactly one frame.
3. Use rewind to step back if a frame decision is incorrect. Expected result: you can return to an earlier frame and test an alternate input.

| Step | Panel | Screenshot Anchor ID | Capture Target |
|---|---|---|---|
| 1 | Main Emulation Window (Frame Advance) | `rewind-b-01-frame-advance` | `docs/screenshots/rewind/b-01-frame-advance.png` |
| 2 | Main Emulation Window (Frame Correction) | `rewind-b-02-frame-correction` | `docs/screenshots/rewind/b-02-frame-correction.png` |

## Related Links

- [Save States Guide](Save-States.md)
- [TAS Editor Manual](TAS-Editor-Manual.md)
- [Movie System Guide](Movie-System.md)
