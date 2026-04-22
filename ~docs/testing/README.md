# Testing and Manual Validation Index

This index unifies all testing and manual validation documentation.

## Start Path

1. User-facing start page: [docs/manual-testing/README.md](../../docs/manual-testing/README.md)
2. Release ranking and blocker order: [docs/manual-testing/manual-step-priority-ranking.md](../../docs/manual-testing/manual-step-priority-ranking.md)
3. Full developer test tree: this page

## Atari 2600 Release Validation

| Document | Purpose |
|---|---|
| [atari2600-production-readiness-matrix.md](atari2600-production-readiness-matrix.md) | Consolidated release readiness matrix and sign-off criteria |
| [atari2600-tas-ui-coverage-audit.md](atari2600-tas-ui-coverage-audit.md) | Automated and manual TAS/movie UI coverage assessment |
| [docs/manual-testing/atari2600-debug-and-tas-manual-test.md](../../docs/manual-testing/atari2600-debug-and-tas-manual-test.md) | Operator checklist for debugger UI and TAS/movie workflows |

## Channel F Release Validation

| Document | Purpose |
|---|---|
| [channelf-test-strategy-matrix-2026-03-30.md](channelf-test-strategy-matrix-2026-03-30.md) | Consolidated quality gates across Channel F core, UI, debugger, TAS/movie, and persistence |
| [channelf-production-readiness-matrix.md](channelf-production-readiness-matrix.md) | Go/no-go readiness checklist and required evidence gates for first-class Channel F support |
| [channelf-benchmark-suite-matrix.md](channelf-benchmark-suite-matrix.md) | Channel F benchmark inventory, CI smoke coverage, and local full-run evidence protocol |
| [channelf-memory-budget-and-regression-budgets.md](channelf-memory-budget-and-regression-budgets.md) | Channel F memory budget baselines, alert thresholds, and regression triage policy |
| [channelf-golden-rom-corpus-inventory.md](channelf-golden-rom-corpus-inventory.md) | Golden ROM corpus inventory, metadata schema, and checksum verification contract |
| [channelf-troubleshooting-known-issues-playbook.md](channelf-troubleshooting-known-issues-playbook.md) | Practical known-issues matrix and triage playbook for Channel F runtime/debugger/TAS workflows |
| [channelf-tas-gesture-widget-triage-2026-03-30.md](channelf-tas-gesture-widget-triage-2026-03-30.md) | #1012 triage checkpoint for Channel F TAS gesture widget/timeline readiness and blockers |
| [channelf-tas-gesture-widget-checkpoint-2026-03-30.md](channelf-tas-gesture-widget-checkpoint-2026-03-30.md) | #1012 implementation and validation checkpoint for Channel F twist/pull/push TAS lane widgets |
| [docs/manual-testing/channelf-debug-and-tas-manual-test.md](../../docs/manual-testing/channelf-debug-and-tas-manual-test.md) | Operator checklist for Channel F runtime, debugger, TAS, and movie workflows |
| [docs/manual-testing/channelf-controller-profile-presets.md](../../docs/manual-testing/channelf-controller-profile-presets.md) | Standard and accessibility-oriented controller profile presets for Channel F |

## Atari Lynx Release Validation

| Document | Purpose |
|---|---|
| [lynx-manual-testing-guide.md](lynx-manual-testing-guide.md) | Developer-facing Lynx manual workflow and expected behavior checks |
| [docs/manual-testing/lynx-debug-and-tas-manual-test.md](../../docs/manual-testing/lynx-debug-and-tas-manual-test.md) | Operator checklist for Lynx debugger, TAS, save states, and audio |

## Manual Testing Checklists

| Document | Purpose |
|---|---|
| [MANUAL-TESTING-CHECKLIST.md](MANUAL-TESTING-CHECKLIST.md) | Main historical full-feature manual checklist |
| [MANUAL-TESTING-CHECKLIST-2026-02-02.md](MANUAL-TESTING-CHECKLIST-2026-02-02.md) | Versioned snapshot checklist |
| [tas-movie-testing.md](tas-movie-testing.md) | TAS and movie focused manual test cases |
| [lynx-manual-testing-guide.md](lynx-manual-testing-guide.md) | Lynx manual workflow and expected behavior checks |
| [phase-1.5-manual-testing-plan.md](phase-1.5-manual-testing-plan.md) | Transitional manual testing plan and execution notes |
| [reference-rom-validation-guide.md](reference-rom-validation-guide.md) | Reference ROM setup and validation steps |

## Automated Testing and Benchmark Plans

| Document | Purpose |
|---|---|
| [CPP-TESTING-PLAN.md](CPP-TESTING-PLAN.md) | C++ automated test strategy and execution scope |
| [CPP-BENCHMARKING-PLAN.md](CPP-BENCHMARKING-PLAN.md) | C++ benchmarking approach and quality gates |
| [tas-release-hardening-2026-03-25.md](tas-release-hardening-2026-03-25.md) | Epic 23 TAS stability hardening status, evidence, and next steps |
| [epic-23-menu-tas-input-validation-matrix-2026-03-29.md](epic-23-menu-tas-input-validation-matrix-2026-03-29.md) | Consolidated Epic 23 validation matrix for menu reliability, TAS stability, and input mapping coverage |
| [input-mapping-coverage-checkpoint-2026-03-30.md](input-mapping-coverage-checkpoint-2026-03-30.md) | Focused per-system controller mapping decode coverage checkpoint and validation evidence for #1073 |
| [ui-quality-modernization-checkpoint-2026-03-30.md](ui-quality-modernization-checkpoint-2026-03-30.md) | Measurable UI consistency and menu/dialog no-regression checkpoint for #1074 |
| [wonderswan-test-coverage-matrix-2026-03-30.md](wonderswan-test-coverage-matrix-2026-03-30.md) | WonderSwan subsystem coverage snapshot and incremental execution plan |
| [ci-release-pipeline-fixes-v1.4.5-v1.4.8.md](ci-release-pipeline-fixes-v1.4.5-v1.4.8.md) | CI and release stabilization timeline with root-cause and fix mapping for Epic 22 |
| [ui-settings-responsiveness-benchmarks-2026-03-30.md](ui-settings-responsiveness-benchmarks-2026-03-30.md) | Focused startup-adjacent and settings-navigation responsiveness benchmark checkpoint for #1046 |
| [2026-04-21-epic-21-ui-benchmark-and-test-plan.md](2026-04-21-epic-21-ui-benchmark-and-test-plan.md) | Reproducible telemetry + benchmark harness, baseline evidence, and regression thresholds for #1414 |
| [2026-04-21-epic-21-icon-coverage-audit.md](2026-04-21-epic-21-icon-coverage-audit.md) | Icon inventory, resolved coverage gaps, and UI QA checklist integration for #1413 |
| [2026-04-21-epic-21-ui-test-strategy-matrix.md](2026-04-21-epic-21-ui-test-strategy-matrix.md) | Functional and visual regression matrix with runnable workflow for #1415 |
| [2026-04-21-epic-21-settings-visual-snapshot-baseline.md](2026-04-21-epic-21-settings-visual-snapshot-baseline.md) | Baseline capture set, storage paths, and comparison workflow for settings-window visual snapshots in #1408 |
| [2026-04-21-epic-21-theme-tab-refresh-checkpoint.md](2026-04-21-epic-21-theme-tab-refresh-checkpoint.md) | Theme-tokenized tab-strip chrome and affordance checkpoint for #1409 |
| [2026-04-21-epic-21-speed-slider-prototype-checkpoint.md](2026-04-21-epic-21-speed-slider-prototype-checkpoint.md) | Speed-slider prototype UX notes, precision checks, latency benchmarks, and ship/no-ship decision for #1410 |

## Machine-Readable Testing Assets

| File | Purpose |
|---|---|
| [reference-rom-validation-manifest.schema.json](reference-rom-validation-manifest.schema.json) | Validation manifest schema |
| [reference-rom-validation-manifest.example.json](reference-rom-validation-manifest.example.json) | Example manifest template |
| [reference-rom-validation-manifest.json](reference-rom-validation-manifest.json) | Current manifest |
