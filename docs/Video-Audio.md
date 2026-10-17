# Video and Audio Guide

## Overview

Nexen includes configurable rendering and audio options for both play and analysis workflows.

## Video Features

- Scaling controls and aspect management.
- Filter and shader options.
- Overlay and HUD support for debugging.

## Audio Features

- Per-channel volume controls.
- Audio quality and output settings.
- WAV recording support.

## Typical Setup

1. Open Settings.
2. Select preferred video filter and scaling behavior.
3. Tune audio channels for clarity.
4. Save configuration profile for repeat use.

## GUI Tips

- Use simple filters while debugging to reduce distraction.
- Enable overlays when validating timing or HUD-driven behavior.
- Use audio recording for regression comparisons.

## Panel Walkthrough (Screenshot Anchors)

### Walkthrough A: Video Configuration

1. Open Settings and navigate to Video options. Expected result: the Video tab is active with rendering controls visible.
2. Select scaling mode and aspect behavior. Expected result: selected scaling and aspect values are reflected in current options.
3. Choose filter or shader preset. Expected result: the chosen filter/shader appears as the active selection.
4. Apply and verify in gameplay view. Expected result: gameplay output reflects the selected video settings.

| Step | Panel | Screenshot Anchor ID | Capture Target |
|---|---|---|---|
| 1 | Settings Window (Video Tab) | `video-audio-a-01-video-tab` | `docs/screenshots/video-audio/a-01-video-tab.png` |
| 2 | Video Options (Scale and Aspect) | `video-audio-a-02-scale-aspect` | `docs/screenshots/video-audio/a-02-scale-aspect.png` |
| 3 | Shader and Filter Selection | `video-audio-a-03-shader-filter` | `docs/screenshots/video-audio/a-03-shader-filter.png` |
| 4 | Gameplay View (Video Applied) | `video-audio-a-04-video-applied` | `docs/screenshots/video-audio/a-04-video-applied.png` |

### Walkthrough B: Audio Configuration and Recording

1. Navigate to Audio settings panel in Settings. Expected result: Audio controls and device options are visible.
2. Adjust per-channel volume and output options. Expected result: channel levels update and audio mix changes are audible.
3. Start WAV recording. Expected result: recording state indicates active audio capture.
4. Stop recording and verify output file. Expected result: a WAV file is created and playable with captured session audio.

| Step | Panel | Screenshot Anchor ID | Capture Target |
|---|---|---|---|
| 1 | Settings Window (Audio Tab) | `video-audio-b-01-audio-tab` | `docs/screenshots/video-audio/b-01-audio-tab.png` |
| 2 | Channel Mixer / Volume Controls | `video-audio-b-02-channel-mixer` | `docs/screenshots/video-audio/b-02-channel-mixer.png` |
| 3 | Audio Recording Control | `video-audio-b-03-record-control` | `docs/screenshots/video-audio/b-03-record-control.png` |

## Related Links

- [Performance Guide](PERFORMANCE.md)
- [Debugging Guide](Debugging.md)
- [Systems Documentation](systems/README.md)
