# Issue #2318 Plan - Theme Profile Rename/Unique Trim Conflict Regression

## Scope

Add regression coverage for trimmed-name behavior in theme profile naming workflows used by semantic-token profile customization.

## Acceptance Criteria

- Add a test ensuring `GenerateUniqueThemeProfileName` trims incoming names before conflict detection/suffixing.
- Add a test ensuring `RenameThemeProfile` rejects a rename when the trimmed target collides with an existing profile name.
- Verify active profile name remains unchanged when rename is rejected.
- Keep existing naming and duplicate-profile behavior unchanged.
