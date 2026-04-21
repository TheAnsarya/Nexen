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

## UX Changes in This Slice

- Controller mapping customization is no longer a required first-run question.
- Default flow now keeps the faster path by hiding mapping-grid detail until explicitly requested.
- Guidance text points users to post-setup Input settings for later adjustments.

## Notes

- Existing deterministic default key mappings remain unchanged when customization is skipped.
- This keeps first-run choices focused on intent and storage, reducing low-value decision points.
