# Epic 21 Onboarding Flow Map Update

## Scope

Issue: #1403
Epic: #1402

This update documents the latest first-run flow simplification pass.

## Updated Flow

1. Primary usage profile (Playing or Debugging)
2. Storage location decision with consequence text and live resolved path preview
3. Input mappings: optional customization toggle (off by default)
4. Misc options (shortcut, update checks)
5. Confirm and write config

## Onboarding State Model

- New install state:
	- No `settings.json` in portable or documents location.
	- Setup wizard opens with default selections.
- Cancel/resume state:
	- Closing setup without confirming writes `setup-wizard-state.json` draft state.
	- Next setup launch restores saved selections and shows a resume notice.
	- `Start fresh` clears draft state and restores defaults.
- Migrated settings state:
	- Existing installs bypass setup wizard and continue directly to main window.
	- Migration-specific setting moves remain explicit future work (no implicit file moves during onboarding).

## UX Changes in This Slice

- Controller mapping customization is no longer a required first-run question.
- Default flow now keeps the faster path by hiding mapping-grid detail until explicitly requested.
- Guidance text points users to post-setup Input settings for later adjustments.

## Notes

- Existing deterministic default key mappings remain unchanged when customization is skipped.
- This keeps first-run choices focused on intent and storage, reducing low-value decision points.

## Measurability

- Wizard telemetry is persisted in `setup-wizard-metrics.json` under the default documents root.
- Launches are tracked with resumed/non-resumed separation.
- Completion and cancel actions both accumulate total backtrack count.
- Start-fresh usage is tracked explicitly.
- Profile/storage/customize toggles are tracked to support future conversion analysis.
