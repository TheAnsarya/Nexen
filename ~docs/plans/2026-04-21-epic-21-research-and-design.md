# Epic 21 Research and Design Notes (2026-04-21)

## Research Inputs

- Issue #1402 problem statements and pain points
- Existing settings and debugger windows in Avalonia XAML
- Existing UI coverage and responsiveness docs in `~docs/testing/`

## Key Findings

- Settings pages have inconsistent scroll behavior and can clip controls.
- Navigation hierarchy has deep paths for common controller tasks.
- Visual affordance for active vs disabled entries is inconsistent.
- Some first-run questions have low perceived value for most users.

## Design Principles

- Context-first defaults: show most relevant settings first.
- Progressive disclosure: keep initial screens short and focused.
- Visible state and affordance: active, hover, disabled, selected.
- Large-target interaction: touch and mouse both benefit.
- Consistency before novelty: unify layouts, spacing, and flows.

## Proposed Information Architecture

Top-level settings structure:

- System and Input
- Video and Audio
- Debugger and Integration
- Interface and Accessibility
- Advanced and Diagnostics

## UX Metrics To Track

- Median click count to controller config for active system
- First-run completion time
- Settings task completion time (representative tasks)
- Backtrack count per task
- Error or confusion events (cancel/reopen loops)

## Recommended Implementation Strategy

1. Fix critical usability blockers first (scrolling, window sizing, context landing)
2. Improve navigation structure second
3. Apply visual refresh and theming convergence third
4. Enforce benchmark and regression gates across all slices

## Open Design Questions

- Speed slider placement in menu vs dedicated flyout panel
- Default profile migration behavior for existing users
- Theme persistence model and per-window override policy

## Decision Log

- 2026-04-21: Start with #1411 as the first low-risk, high-impact implementation slice.
