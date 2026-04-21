# Epic 21 UI and UX Master Plan (2026-04-21)

## Scope

Parent epic: #1402

Sub-issues:

- #1403 onboarding flow redesign
- #1404 primary usage profiles
- #1405 storage location UX
- #1406 settings IA redesign
- #1407 input settings context-first landing
- #1408 resize and touch-target improvements
- #1409 theme and tab visual refresh
- #1410 speed slider prototype
- #1411 universal scrolling in dense settings windows
- #1412 debugger defaults and Pansy/CDL UX copy
- #1413 icon coverage audit
- #1414 UX telemetry and benchmark harness
- #1415 functional and visual regression test matrix
- #1416 design system and component specs
- #1417 UI mockups and interaction flows

## Program Goals

- Reduce navigation depth for high-frequency tasks.
- Improve first-run clarity and reduce setup friction.
- Improve readability, affordance, and visual consistency.
- Improve touch and low-DPI usability with larger targets.
- Add measurable UX metrics and regression gates.

## Milestones

1. Discovery and design baseline
2. Navigation and layout upgrades
3. Theming and visual affordance upgrades
4. Instrumentation and benchmark baselines
5. Regression test hardening and release rollout

## Architecture Decisions

- Keep existing Avalonia MVVM patterns and avoid broad framework churn in this epic.
- Favor route-level and container-level changes over large component rewrites first.
- Ship in slices with measurable before and after metrics.

## Risk Register

- UI breakage in high-traffic settings pages
- Regression in keyboard or controller navigation
- Theme token drift across windows and controls
- Visual inconsistency during partial rollout

## Mitigations

- Add visual and interaction smoke checks for each slice.
- Keep each implementation slice small and issue-scoped.
- Add rollback notes in each sub-issue close comment.

## First Implementation Slice

- #1411 phase 1: Add explicit scrolling containers to dense Debugger settings tabs.
- Validate with manual checks at smaller window sizes and full managed tests.

## Exit Criteria

- All sub-issues closed with evidence.
- UX benchmark comparison documented.
- Regression test matrix green for critical settings and onboarding flows.
- Updated UI docs linked from `~docs/README.md`.
