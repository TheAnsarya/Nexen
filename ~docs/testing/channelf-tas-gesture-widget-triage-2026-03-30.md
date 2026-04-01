# Channel F TAS Gesture Widget Triage (2026-03-30)

## Scope

Triage checkpoint for issue #1012:

- `[20.2.1] TAS input visualization widgets for Channel F gestures`

Issue asks for TAS editor widgets/timelines for Channel F gestures (`twist`, `pull`, `push`) under parent #970.

## Findings

1. Channel F is still explicitly gated out of TAS and movie tooling.

- `UI/Interop/ConsoleTypeExtensions.cs`
	- `SupportsTasEditor(ConsoleType.ChannelF) => false`
	- `SupportsMovieTools(ConsoleType.ChannelF) => false`

2. TAS editor input label-to-button mapping already includes gesture-compatible names.

- `UI/Windows/TasEditorWindow.axaml.cs`
	- `MapButtonLabelToName` maps: `↺ => TWISTCCW`, `↻ => TWISTCW`, `PL => PULL`, `PS => PUSH`

3. Default button labels are still generic (`A/B/X/Y/...`) and not Channel F gesture-centric.

- `UI/Windows/TasEditorWindow.axaml.cs`
	- `GetDefaultButtonLabels()` returns non-Channel-F defaults.

4. Existing interop tests confirm current gate behavior.

- `Tests/Interop/ConsoleTypeExtensionsTests.cs`
	- Asserts Channel F TAS support is currently `false`.

## Triage Conclusion

- #1012 is **not closeable** in current state.
- Core gesture naming hooks are present in the TAS editor path, but Channel F is still blocked at console capability gates.
- The likely minimal path is:
	1. Introduce Channel F-specific TAS button label source.
	2. Enable Channel F in `SupportsTasEditor` and `SupportsMovieTools` once labels/input path are wired.
	3. Add focused TAS editor tests for Channel F gesture lanes/timelines.

## Suggested Validation Additions

- Extend `Tests/Interop/ConsoleTypeExtensionsTests.cs` for the new expected Channel F TAS/movie capability.
- Add `Tests/TAS` coverage to verify gesture-lane labels and button-name mapping for Channel F timeline interactions.
- Add manual check pass in `docs/manual-testing/channelf-debug-and-tas-manual-test.md` for twist/pull/push lane editing behavior.
