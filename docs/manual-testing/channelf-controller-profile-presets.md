# Channel F Controller Profile Presets

Parent issue: [#1014](https://github.com/TheAnsarya/Nexen/issues/1014)

## Purpose

Define standard and accessibility-oriented controller presets for Fairchild Channel F so users can quickly select stable mappings for stick gestures and console panel buttons.

## Preset Catalog

| Preset | Target User | Mapping Focus |
|---|---|---|
| Default Stick + Panel | Standard controllers | Direct mapping for Left/Right/Forward/Back + Twist CW/CCW + Pull/Push + TIME/HOLD/MODE/START |
| Reduced Twist Load | Users who struggle with rotational input precision | Keep directional and push/pull on primary input, move Twist CW/CCW to easier secondary buttons |
| One-Hand Friendly | Users with one-hand operation constraints | Group high-frequency actions on one side and map panel actions to low-reach keys |
| Keyboard Compact | Keyboard-only users | Cluster directional and panel controls in a tight key neighborhood |
| Left-Handed Variant | Left-handed preference | Mirror core directional and action cluster from right-hand defaults |

## Accessibility Guidance

- Prefer low-travel mappings for repeated actions.
- Avoid sharing a physical key between Twist and Pull/Push actions.
- Keep START/TIME/HOLD/MODE on distinct keys to reduce accidental game-state changes.
- Validate each preset using the Channel F manual workflow before publishing as default.

## Validation Checklist

1. Open a Channel F title and verify every mapped action is detectable in input HUD/debugger input view.
2. Confirm each preset can complete menu navigation and gameplay start without remapping.
3. Verify movie recording and playback preserve the mapped inputs for at least a short deterministic loop.
4. Confirm no preset introduces conflicting bindings for panel buttons.

## Related Documents

- [Channel F Debug and TAS Manual Test](channelf-debug-and-tas-manual-test.md)
- [Manual Step Priority Ranking](manual-step-priority-ranking.md)
- [Developer Testing and Manual Validation Index](../../~docs/testing/README.md)
