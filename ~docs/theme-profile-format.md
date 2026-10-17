# Theme Profile Format

This document describes the Nexen theme profile import and export JSON schema used by the Preferences UI.

## File Extension

- Preferred extension: `.nexen-theme.json`
- Alternate extension: `.json`

## Root Schema

```json
{
	"format": "nexen-theme-profile",
	"version": 1,
	"profile": {
		"name": "Default Dark",
		"theme": "Dark",
		"uiFontFamily": "Microsoft Sans Serif",
		"uiFontSize": 11,
		"menuFontFamily": "Segoe UI",
		"menuFontSize": 12,
		"startupWindowBackgroundColor": "#ff1d130d",
		"startupSurfaceBackgroundColor": "#ff2a1a10",
		"startupBorderColor": "#ff6b3f24",
		"startupCardBackgroundColor": "#ff2a1a10",
		"startupCardCheckedBackgroundColor": "#ffb65e18",
		"startupTextColor": "#ffffffff",
		"startupMutedTextColor": "#ffefcfb6",
		"startupActionBorderColor": "#ffffce87",
		"startupPrimaryActionColor": "#ffd47a22",
		"startupDividerColor": "#553f2b1f",
		"setupSubtitleColor": "#ffefcfb6",
		"setupTitleFontSize": 34,
		"setupSubtitleFontSize": 16,
		"setupPrimaryActionFontSize": 34
	}
}
```

## Notes

- `format` must be `nexen-theme-profile`.
- `version` is currently `1`.
- Color values use ARGB hex format: `#aarrggbb`.
- `theme` supports `Light` or `Dark`.
- Imported profiles are upserted by `profile.name`.
- Import rejects files when:
	- `format` is not `nexen-theme-profile`
	- `version` is not `1`
	- `profile.name` is empty
	- Any color field is invalid

## Profile Management UX

- Preferences now supports:
	- Select and apply profile
	- Save current settings into selected profile
	- Rename profile
	- Duplicate profile
	- Delete profile
	- Import/export profile JSON
	- Live swatch preview for startup background/text/primary action colors
	- Import conflict handling (overwrite existing profile or import as uniquely named copy)
	- Reset selected profile to centralized defaults for its light/dark variant
	- Apply canonical light/dark preset defaults to selected profile in one click
	- Show whether selected profile matches canonical defaults or has customized token count
	- Show a per-token customized summary list for the selected profile
	- Group customized summary by category (Fonts, Startup, Setup)

## Runtime Application

When a profile is activated, Nexen applies:

- Base light/dark theme variant.
- UI font family and size.
- Menu font family and size.
- Startup semantic brush overrides for:
	- `NexenStartupWindowBackgroundBrush`
	- `NexenStartupSurfaceBackgroundBrush`
	- `NexenStartupBorderBrush`
	- `NexenStartupCardBackgroundBrush`
	- `NexenStartupCardCheckedBackgroundBrush`
	- `NexenStartupTextBrush`
	- `NexenStartupMutedTextBrush`
	- `NexenStartupActionBorderBrush`
	- `NexenStartupPrimaryActionBackgroundBrush`
	- `NexenStartupDividerBrush`
	- `NexenSetupSubtitleBrush`
- Setup typography overrides for:
	- `NexenSetupTitleFontSize`
	- `NexenSetupSubtitleFontSize`
	- `NexenSetupPrimaryActionFontSize`
