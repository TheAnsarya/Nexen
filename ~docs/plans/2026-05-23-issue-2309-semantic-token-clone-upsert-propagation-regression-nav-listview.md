# Issue #2309 Semantic Token Clone/Upsert Propagation Regression for NavigationView/ListView (2026-05-23)

## Scope

Add regression tests that validate duplication and upsert pipelines preserve and update NavigationView/ListView semantic token values without dropping data.

## Acceptance Criteria

1. Duplicate profile operation preserves NavigationView/ListView semantic token values.
2. Upsert operation updates existing profile NavigationView/ListView semantic token values deterministically.
3. Focused tests and Release x64 build pass.
