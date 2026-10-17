# Theme Profile Chrome Customization Manual Test

## Goal

Validate that theme profiles can customize centralized menu/chrome/accent tokens through Settings and import/export.

## Preconditions

- Nexen launches normally.
- Preferences window is accessible.
- At least one theme profile exists.

## Steps

1. Open `Settings -> Preferences -> General`.
2. In `Theme Profiles`, select an existing profile (for example `Default Dark`).
3. Change each of these using the color pickers:
	- Menu Background
	- Menu Highlight
	- Accent Color
	- Control Hover Background
	- Control Pressed Background
	- Control Hover Border
	- Control Pressed Border
	- Sidebar Border Color
	- Dock Tab Strip Background
	- Dock Tab Hover Background
	- Dock Tab Active Background
	- Dock Tab Active Border
	- Checkbox Hover Border
	- Checkbox Pressed Background
	- Checkbox Pressed Border
	- Radio Button Hover Border
	- Radio Button Pressed Background
	- Radio Button Pressed Border
	- Slider Hover Track
	- Slider Pressed Track
	- Text Selection
	- Disabled Text Background
	- Tooltip Background
	- Menu Flyout Background
	- Menu Flyout Border
	- ComboBox Dropdown Background
	- ComboBox Dropdown Border
	- DataGrid Header Background
	- DataGrid Header Foreground
	- DataGrid Selected Row Foreground
	- ListBox Hover Background
	- ListBox Selected Background
	- ListBox Selected Foreground
	- TreeView Hover Background
	- TreeView Selected Background
	- TreeView Selected Foreground
4. Click `Apply` for the selected profile.
5. Verify the top menu bar and menu selection visuals reflect the new values.
6. Verify button/combo/repeat hover and pressed visuals reflect the updated control semantic tokens.
7. Verify settings left-tab sidebar border reflects the sidebar border token.
8. Verify docked tool tab strip/hover/active visuals reflect the dock tab tokens.
9. Verify checkbox hover and pressed visuals reflect checkbox semantic tokens.
10. Verify radio button hover and pressed visuals reflect radio semantic tokens.
11. Verify slider hover and pressed track visuals reflect slider semantic tokens.
12. Verify text selection and disabled text background reflect text-input semantic tokens.
13. Verify tooltip and menu flyout surfaces reflect semantic background/border tokens.
14. Verify ComboBox dropdown and DataGrid header/selected-row surfaces reflect semantic token values.
15. Verify ListBox/TreeView hover and selected visuals reflect semantic token values.
16. Click `Export Profile` and save the `.nexen-theme.json` file.
17. Modify the exported file name and import it back with `Import Profile`.
18. Confirm the imported profile appears in the profile dropdown.
19. Select and apply the imported profile.
20. Confirm menu/chrome/accent/control/sidebar/tab/check/radio/slider/text/flyout/combo/datagrid/list/tree visuals match the imported token values.
21. Use `Reset Defaults` and verify colors return to canonical defaults for the profile theme mode.

## Expected Results

- Picker changes apply to menu/chrome/accent visuals when profile is active.
- Picker changes apply to menu/chrome/accent/control hover+pressed visuals when profile is active.
- Picker changes apply to sidebar/tab strip hover/active visuals when profile is active.
- Picker changes apply to checkbox/radio hover+pressed and slider hover+pressed track visuals when profile is active.
- Picker changes apply to text-input and tooltip/menu-flyout semantic visuals when profile is active.
- Picker changes apply to ComboBox dropdown and DataGrid header/selected-row semantic visuals when profile is active.
- Picker changes apply to ListBox and TreeView hover/selected semantic visuals when profile is active.
- Exported profile includes UI chrome token values.
- Imported valid profile is accepted and selectable.
- Reset Defaults restores canonical values for light/dark profile mode.

## Failure Notes

Capture these on failure:

- Which token changed and which surface did not update.
- Whether import failed validation for expected-valid files.
- Whether reset failed to restore canonical values.
