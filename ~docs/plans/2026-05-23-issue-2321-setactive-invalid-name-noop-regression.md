# Issue #2321 Plan - SetActive Invalid Name No-Op Regression

## Scope

Add regression coverage ensuring `SetActiveThemeProfile` performs a no-op when given a non-existent profile name.

## Acceptance Criteria

- Add a test that sets a known active profile, then calls `SetActiveThemeProfile` with an invalid name.
- Verify active profile name remains unchanged.
- Verify effective theme remains unchanged after the invalid call.
- Keep valid profile activation behavior unchanged.
