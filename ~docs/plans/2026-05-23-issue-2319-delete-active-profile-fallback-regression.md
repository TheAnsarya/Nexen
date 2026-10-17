# Issue #2319 Plan - Delete Active Profile Fallback Regression

## Scope

Add regression coverage ensuring deleting the active theme profile correctly falls back to a remaining profile and preserves that fallback profile's NavigationView/ListView semantic token values.

## Acceptance Criteria

- Add a test that activates one profile, customizes semantic tokens on the fallback profile, and deletes the active profile.
- Verify active profile name switches to the fallback profile.
- Verify fallback NavigationView/ListView semantic token values remain unchanged.
- Keep existing delete behavior unchanged for non-active profiles.
