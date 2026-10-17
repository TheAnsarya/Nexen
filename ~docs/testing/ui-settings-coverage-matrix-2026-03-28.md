# UI Settings Coverage Matrix (2026-03-28)

## Legend

- Complete: labels + descriptive context + confirmed bindings
- Partial: mostly functional but missing guidance/localization/depth
- Gap: placeholder text, hardcoded text, or uncertain behavior

## Platform Settings Views

| Platform | General | Audio | Emulation | Input | Video | Notes |
| ---------- | --------- | ------- | ----------- | ------- | ------- | ------- |
| NES | Complete | Complete | Complete | Complete | Complete | Mature surface |
| SNES | Complete | Complete | Complete | Complete | Complete | Mature surface |
| Game Boy | Complete | Complete | Complete | Complete | Complete | Mature surface |
| GBA | Complete | Complete | Complete | Complete | Complete | Mature surface |
| SMS | Complete | Complete | Complete | Complete | Complete | Mature surface |
| PCE | Complete | Complete | Complete | Complete | Complete | Mature surface |
| WonderSwan | Complete | Complete | Complete | Complete | Complete | Mature surface |
| Lynx | Partial | Partial | Partial | Partial | Partial | Missing form-localized resources and contextual guidance (fixed in-session for primary labels/help text) |
| Atari 2600 | Partial | Complete | Partial | Gap | Partial | Input form localization context missing; descriptive guidance limited (improved in-session) |
| Channel F | Gap | Partial | Partial | Gap | Partial | New system; missing form-localization blocks and enum labels (addressed in-session) |

## Cross-System UX Surfaces

| Surface | Status | Notes |
| --------- | -------- | ------- |
| Global Input Settings | Partial | Functional; more platform-aware guidance needed |
| Movie Record/Playback | Partial | Functional baseline; validation/wording consistency pass pending |
| Savestates/Greenzone | Partial | Feature-rich but mixed localization style and discoverability |
| Debugger Settings Integration | Partial | Broad coverage, some newer platform gaps remain |

## Session Actions Completed

1. Added Channel F, Lynx, and Atari 2600 descriptive UI helper text.
2. Added missing form localization mappings for:
- ChannelFConfigView
- ChannelFInputConfigView
- LynxConfigView
- Atari2600InputConfigView

3. Added missing ControllerType enum labels for newer systems/controllers.
4. Fixed a malformed binding typo in DefaultControllerView.
5. Removed duplicate Avalonia.Svg.Skia package reference warning.

## Follow-Up Validation Checklist

1. Manual walkthrough of Config window tabs for all systems.
2. Verify no [View:Key] placeholders appear in UI.
3. Confirm controller setup dialogs open and persist correctly.
4. Execute targeted tests for config clone/apply behavior.
5. Add responsiveness checkpoints for settings open/switch/close paths.
