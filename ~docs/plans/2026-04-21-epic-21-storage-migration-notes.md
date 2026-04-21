# Epic 21 Storage Migration Notes

## Scope

Issue: #1405
Epic: #1402

This note documents the migration-safe path for onboarding storage location UX improvements.

## Behavior

- Setup wizard still determines config home folder using existing `ConfigManager.CreateConfig(!StoreInUserProfile)` behavior.
- Setup now also persists intent metadata via `Preferences.PreferUserProfileStorage`.
- Metadata does not move files automatically; it records user intent for future tooling/settings UX.

## Rationale

- Avoids risky implicit file moves during first-run and upgrades.
- Preserves backward compatibility with existing folder layouts.
- Enables future explicit migration UX with informed defaults.

## Future Migration Plan

1. Add a settings action that offers explicit migration from current to preferred storage root.
2. Validate write access and free space before copy.
3. Copy saves/config/debug artifacts to target folder.
4. Verify checksums and file counts.
5. Switch active folder pointer only after verification.
6. Keep rollback backup for one launch cycle.
