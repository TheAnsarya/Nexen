# Issue #2320 Plan - Delete Single Profile Guard Regression

## Scope

Add regression coverage for the guard path that prevents deleting the last remaining theme profile.

## Acceptance Criteria

- Add a test that removes one default profile first, then attempts to delete the final remaining profile.
- Verify deletion of the final profile is rejected.
- Verify the remaining profile still exists after the rejected delete.
- Keep existing multi-profile delete behavior unchanged.
