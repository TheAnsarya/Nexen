# Issue #2302 Theme Group Ordering Regression Test (2026-05-23)

## Scope

Add a markup regression test that verifies semantic section headers in Preferences theme customization appear in the expected order.

## Ordered Sections

1. Startup
2. Chrome and Accent
3. Control States
4. Inputs and Flyouts
5. Combo and DataGrid
6. List and Tree
7. Navigation and ListView

## Acceptance Criteria

1. A dedicated regression test checks group order using deterministic index comparisons.
2. The test fails if grouped section order regresses.
3. Existing theme customization markup coverage remains green.
