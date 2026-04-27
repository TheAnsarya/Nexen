# Nexen Developer Documentation

This directory contains internal development documentation for Nexen contributors and maintainers.

> **Note:** For user documentation, see [docs/](../docs/README.md).

## 📖 Core Documentation

| Document | Description |
| ---------- | ------------- |
| [Architecture Overview](ARCHITECTURE-OVERVIEW.md) | High-level system design and component interaction |
| [C++ Development Guide](CPP-DEVELOPMENT-GUIDE.md) | C++23 coding practices and standards |
| [Code Documentation Style](CODE-DOCUMENTATION-STYLE.md) | XML comment style for Doxygen |
| [Profiling Guide](PROFILING-GUIDE.md) | Performance profiling techniques |
| [ASan Guide](ASAN-GUIDE.md) | AddressSanitizer for memory debugging |

## 🎮 Emulation Core Documentation

| Document | Description |
| ---------- | ------------- |
| [NES Core](NES-CORE.md) | 6502 CPU, PPU, APU, mappers |
| [SNES Core](SNES-CORE.md) | 65816 CPU, PPU, SPC700, coprocessors |
| [GB/GBA Cores](GB-GBA-CORE.md) | LR35902, ARM7TDMI, PPU |
| [SMS/PCE/WS Cores](SMS-PCE-WS-CORE.md) | Z80, HuC6280, V30MZ |
| [Debugger Subsystem](DEBUGGER.md) | Breakpoints, CDL, scripting |
| [Utilities Library](UTILITIES-LIBRARY.md) | Common utility classes |

## 🎛️ Peripheral System Documentation

| Document | Description |
| ---------- | ------------- |
| [Input Subsystem](INPUT-SUBSYSTEM.md) | Controllers, input handling, polling |
| [Audio Subsystem](AUDIO-SUBSYSTEM.md) | Audio mixing, effects, recording |
| [Video Rendering](VIDEO-RENDERING.md) | Filters, HUD, shaders, recording |
| [Movie/TAS System](MOVIE-TAS.md) | Movie recording, TAS features |

## 🌼 Pansy Export Feature

The Pansy export feature enables exporting and importing debug metadata in a universal format compatible with the Peony disassembler and Poppy assembler.

### Getting Started

| Document | Description |
| ---------- | ------------- |
| **[📚 Documentation Index](pansy-export-index.md)** | Start here! Complete overview |
| [User Guide](pansy-export-user-guide.md) | End-user documentation |
| [Tutorials](pansy-export-tutorials.md) | Step-by-step workflows |

### Technical Reference

| Document | Description |
| ---------- | ------------- |
| [API Reference](pansy-export-api.md) | C# and C++ API documentation |
| [Developer Guide](pansy-export-developer-guide.md) | Contributing to Pansy export |

### Design Documents

| Document | Description |
| ---------- | ------------- |
| [Integration Design](pansy-integration.md) | Original design document |
| [Roadmap](pansy-roadmap.md) | Future plans and phases |
| [Phase 7.5 Sync](phase-7.5-pansy-sync.md) | File sync feature design |

## Platform Parity Research

| Document | Description |
| ---------- | ------------- |
| [Research Home](research/README.md) | Entry point for deep research artifacts |
| [Platform Parity Program Index](research/platform-parity/README.md) | Atari 2600 and Genesis research tree with source links |
| [Lynx Cart Shift Register Addressing Audit (2026-03-30)](research/lynx-cart-shift-register-addressing-audit-2026-03-30.md) | Audit findings and follow-up plan for Lynx cart shift-register bank addressing (#956) |
| [Lynx Commercial Bank-Addressing Validation Matrix (2026-03-30)](research/lynx-commercial-bank-addressing-validation-matrix-2026-03-30.md) | Commercial-title empirical validation matrix scaffold for Lynx bank/page addressing follow-up (#1105) |
| [Lynx Commercial Bank-Addressing ROM Manifest (2026-03-30)](research/lynx-commercial-bank-addressing-rom-manifest-2026-03-30.md) | Canonical selected GoodLynx ROM identities and checksums used by the #1105 validation matrix |
| [Lynx Commercial Bank-Addressing Headless Boot Smoke (2026-03-30)](research/lynx-commercial-bank-addressing-headless-boot-smoke-2026-03-30.md) | Headless test-runner smoke execution summary for the #1105 selected commercial corpus |
| [Atari 2600 Breakdown](research/platform-parity/atari-2600/README.md) | Subsystem-by-subsystem Atari 2600 documentation |
| [Genesis Breakdown](research/platform-parity/genesis/README.md) | Comparative Genesis emulator architecture research |
| [Compatibility Path](research/platform-parity/compatibility/sonic1-jurassic-park.md) | Sonic 1 and Jurassic Park execution milestones |

## Fairchild Channel F Program

| Document | Description |
| ---------- | ------------- |
| [Channel F Master Program Plan](plans/channel-f-master-program-plan.md) | End-to-end multi-epic roadmap and quality gates |
| [Channel F Implementation Architecture](plans/channel-f-implementation-architecture.md) | Core and integration design for CPU/bus/video/audio/input/tooling |
| [Channel F Testing and Benchmark Plan](plans/channel-f-testing-and-benchmark-plan.md) | Correctness, performance, and memory validation framework |
| [Channel F Production Readiness Matrix](testing/channelf-production-readiness-matrix.md) | Go/no-go checklist and evidence gates for first-class Channel F release readiness |
| [Channel F Benchmark Suite Matrix](testing/channelf-benchmark-suite-matrix.md) | Benchmark inventory and CI/local execution gates for Channel F smoke and full runs |
| [Channel F Memory Budget and Regression Budgets](testing/channelf-memory-budget-and-regression-budgets.md) | Memory baseline, budget thresholds, and regression response policy for Channel F |
| [Channel F Golden ROM Corpus Inventory](testing/channelf-golden-rom-corpus-inventory.md) | Canonical ROM corpus inventory and checksum metadata contract for deterministic validation |
| [Channel F Troubleshooting Playbook](testing/channelf-troubleshooting-known-issues-playbook.md) | Known issues, triage workflow, and issue reporting template for Channel F |
| [Channel F TAS Gesture Widget Triage (2026-03-30)](testing/channelf-tas-gesture-widget-triage-2026-03-30.md) | #1012 readiness/blocker checkpoint for Channel F twist/pull/push TAS lane visualization |
| [Channel F TAS Gesture Widget Checkpoint (2026-03-30)](testing/channelf-tas-gesture-widget-checkpoint-2026-03-30.md) | #1012 implementation and test-evidence checkpoint for Channel F TAS gesture lanes |
| [Channel F Ordered Prompt Pack](plans/channel-f-implementation-prompts.md) | Session-by-session execution prompts |
| [Channel F Source Index](research/channel-f-source-index.md) | External technical references and mapping to issue tree |
| [Channel F Origin Prompt](plans/channel-f-origin-prompt.md) | Canonical request tracked on all related issues |

## 📁 Subfolders

| Folder | Description |
| -------- | ------------- |
| [modernization/](modernization/) | C++ modernization tracking and roadmaps |
| [testing/](testing/README.md) | Manual testing index, test plans, and benchmarking documentation |
| [plans/](plans/) | Planning documents for future features |
| [research/](research/) | Source-cited platform-parity and subsystem research |
| [session-logs/](session-logs/) | Development session logs |
| [chat-logs/](chat-logs/) | AI conversation logs |

## 📋 Project Tracking

| Document | Description |
| ---------- | ------------- |
| [GitHub Issues](github-issues.md) | Issue tracking notes |
| [Keyboard Shortcuts](keyboard-shortcuts-save-states.md) | Save state shortcut design |
| [Q2 Future Work Program](plans/future-work-program-2026-q2.md) | Milestone-driven execution plan for Epic #673 and sub-issues |
| [Q3 Platform Parity Research Program](plans/platform-parity-research-program-2026-q3.md) | Multi-session research and execution plan for #704-#707 |
| [Scaffold Future-Work Backlog](plans/platform-parity-scaffold-future-work-backlog.md) | Deferred issue tree for converting Atari/Genesis scaffold phases into production implementation work |
| [Atari 2600 Feasibility and Harness Plan](plans/atari-2600-feasibility-and-harness-plan.md) | Spike architecture boundaries, risk register, and harness milestones |
| [Genesis Architecture and Incremental Plan](plans/genesis-architecture-and-incremental-plan.md) | Phase plan, risk register, and benchmark/correctness gates for Genesis |
| [Atari CPU/RIOT Bring-Up Skeleton Plan](plans/atari-2600-cpu-riot-bringup-skeleton.md) | Issue-backed implementation scaffold for Atari CPU and RIOT boundaries (#697) |
| [Atari TIA Timing Spike Harness Plan](plans/atari-2600-tia-timing-spike-harness.md) | Deterministic TIA timing checkpoints and smoke harness contract (#698) |
| [Atari Mapper Order and Regression Matrix](plans/atari-2600-mapper-order-and-regression-matrix.md) | Mapper priority and regression matrix for staged cartridge support (#699) |
| [Atari 2600 Production Readiness Matrix](testing/atari2600-production-readiness-matrix.md) | Consolidated build/test/TAS/debugger/export validation checklist for Atari 2600 |
| [Genesis M68000 Boundary and Bring-Up Plan](plans/genesis-m68000-boundary-and-bringup.md) | M68000 contract and phased bring-up scaffold (#700) |
| [Genesis VDP DMA Milestone Plan](plans/genesis-vdp-dma-milestones-and-harness.md) | VDP timing and DMA checkpoint matrix for staged execution (#701) |
| [Genesis Z80 Audio Integration Staging](plans/genesis-z80-audio-integration-staging.md) | Phased Z80/YM2612/SN76489 integration and risk controls (#702) |
| [Issue #1426 Genesis Research Dossier (2026-04-27)](plans/2026-04-27-issue-1426-genesis-research-dossier.md) | Hardware reference baseline, comparative emulator module map, and validation corpus for Genesis fundamentals |
| [Issue #1427 Genesis Core Integration Checkpoint (2026-04-27)](plans/2026-04-27-issue-1427-genesis-core-track-checkpoint.md) | Core integration coverage checkpoint with decomposition into active child implementation tracks |
| [Issue #1428 Genesis UX Track Checkpoint (2026-04-27)](plans/2026-04-27-issue-1428-genesis-ux-track-checkpoint.md) | Controller, TAS, save-state, and UX integration checkpoint mapped to active backlog issues |
| [Epic 26 Genesis Family Living Plan (2026-04-27)](plans/2026-04-27-epic-26-genesis-family-living-plan.md) | Living decomposition and measurable gate index for Mega Drive, 32X, Sega CD, and Power Base Converter tracks |
| [Issue #1460 Genesis Baseline Decomposition (2026-04-27)](plans/2026-04-27-issue-1460-genesis-baseline-decomposition.md) | Child-slice decomposition and execution order for Genesis baseline completion work |
| [Issue #1461 SMS-PBC Foundation Decomposition (2026-04-27)](plans/2026-04-27-issue-1461-sms-pbc-foundation-decomposition.md) | Child-slice decomposition for SMS maturity and Power Base Converter foundation work |
| [Issue #1462 PBC Support Decomposition (2026-04-27)](plans/2026-04-27-issue-1462-pbc-support-decomposition.md) | Child-slice decomposition for Genesis-hosted SMS compatibility mode and PBC support work |
| [Issue #1463 Sega CD Integration Decomposition (2026-04-27)](plans/2026-04-27-issue-1463-sega-cd-integration-decomposition.md) | Child-slice decomposition for Sega CD staging, deterministic checkpoints, and tooling contracts |
| [Issue #1464 32X Integration Decomposition (2026-04-27)](plans/2026-04-27-issue-1464-32x-integration-decomposition.md) | Child-slice decomposition for dual-SH2 staging, VDP composition sync, and tooling contracts |
| [Issue #1465 Controller/Peripheral Matrix Decomposition (2026-04-27)](plans/2026-04-27-issue-1465-controller-peripheral-matrix-decomposition.md) | Child-slice decomposition for controller/peripheral matrix, deterministic TAS/input coverage, and UI parity |
| [Issue #1466 Tooling and UX Parity Decomposition (2026-04-27)](plans/2026-04-27-issue-1466-tooling-ux-parity-decomposition.md) | Child-slice decomposition for debugger gaps, TAS/cheat deterministic parity targets, and UX/config parity |
| [Issue #1467 Epic 24 Full-Stack Decomposition (2026-04-27)](plans/2026-04-27-issue-1467-epic-24-full-stack-decomposition.md) | Epic-track decomposition for Sega CD, 32X/Power Base, and controllers/UX tooling closure |
| [Issue #1468 32X Architecture and Validation Decomposition (2026-04-27)](plans/2026-04-27-issue-1468-32x-architecture-validation-decomposition.md) | Child-slice decomposition for SH2 arbitration, VDP/passthrough matrix, and PWM correctness gates |
| [Platform Parity Benchmark and Correctness Gates](plans/platform-parity-benchmark-and-correctness-gates.md) | Cross-phase quality gates and evidence framework (#703) |
| [Atari 2600 + Genesis Parity Tracker](plans/atari2600-genesis-parity-tracker.md) | Active multi-phase checklist, issue linkage, and closure criteria for parity execution (#750) |
| [UI Settings Completeness Plan](plans/ui-settings-completeness-epic-18.md) | Epic #1040 execution plan for cross-platform settings/input/movie/savestate UX completeness |
| [Epic 22 Stability and Modernization Plan](plans/epic-22-stability-modernization-plan-2026-03-28.md) | Execution plan for crash/segfault mitigation, runtime compatibility, warning hardening, and modernization gates (#1048-#1055) |
| [Modern Library Baseline and Upgrade Policy (2026-03-29)](plans/modern-library-baseline-policy-2026-03-29.md) | Dependency baseline snapshot, upgrade cadence, and rollback criteria for #1050 |
| [Epic 22 Validation Pack](testing/epic-22-validation-pack-2026-03-28.md) | Build/test/benchmark/runtime evidence snapshot for the 2026-03-28 stabilization pass |
| [CI and Release Pipeline Fixes (v1.4.5 to v1.4.8)](testing/ci-release-pipeline-fixes-v1.4.5-v1.4.8.md) | Consolidated timeline and root-cause/fix record for Epic 22 CI-release stabilization (#1066) |
| [Linux Runtime Crash Matrix](testing/linux-runtime-crash-matrix-2026-03-28.md) | Distro-by-distro crash/segfault signature matrix with current mitigation status for #1049 |
| [Linux Crash Hardening Fix Order](plans/linux-crash-hardening-order-2026-03-28.md) | Ordered stabilization sequence for runtime dependencies, warnings, and cross-distro validation (#1049, #1051, #1054) |
| [Platform Parity Source Index](research/platform-parity/source-index.md) | External hardware and emulator code references used by research docs |
| [Atari 2600 TAS and Movie UI Coverage Audit](testing/atari2600-tas-ui-coverage-audit.md) | Automated and manual validation status for Atari 2600 TAS and movie UI release readiness |
| [UI Settings Coverage Matrix (2026-03-28)](testing/ui-settings-coverage-matrix-2026-03-28.md) | Initial cross-platform settings/config/input/movie/savestate UI gap matrix and action checklist |
| [UI Settings Responsiveness Benchmarks (2026-03-30)](testing/ui-settings-responsiveness-benchmarks-2026-03-30.md) | Startup-adjacent and settings-navigation responsiveness benchmark checkpoint for #1046 |
| [Epic 21 UI and UX Master Plan (2026-04-21)](plans/2026-04-21-epic-21-ui-ux-master-plan.md) | Program-level roadmap, risk register, milestones, and execution strategy for #1402 |
| [Epic 21 Research and Design Notes (2026-04-21)](plans/2026-04-21-epic-21-research-and-design.md) | Research summary, IA direction, metrics, and decision log for UI modernization |
| [Epic 21 UI Mockups and Interaction Flows (2026-04-21)](plans/2026-04-21-epic-21-ui-mockups-and-flows.md) | Text-wireframe mockups and core interaction flows for onboarding/settings improvements |
| [Epic 21 Onboarding Flow Map Update (2026-04-21)](plans/2026-04-21-epic-21-onboarding-flow-map-update.md) | Updated first-run flow map with optional input customization path and reduced default-step friction for #1403 |
| [Epic 21 Storage Migration Notes (2026-04-21)](plans/2026-04-21-epic-21-storage-migration-notes.md) | Migration-safe strategy for storage preference persistence and explicit future relocation flow for #1405 |
| [Issue #1406 Settings Information Architecture Redesign (2026-04-22)](plans/2026-04-22-issue-1406-settings-ia-redesign.md) | Before/after IA map, stable route-ID model, migration notes, and validation record for settings routing redesign |
| [Epic 21 UI Benchmark and Test Plan (2026-04-21)](testing/2026-04-21-epic-21-ui-benchmark-and-test-plan.md) | UX benchmark definitions and automated/manual regression matrix for #1414 and #1415 |
| [Epic 21 UI Test Strategy Matrix (2026-04-21)](testing/2026-04-21-epic-21-ui-test-strategy-matrix.md) | Functional and visual regression matrix with runnable workflow and task list for #1415 |
| [Settings Route Index and Deep-Link Checkpoint (2026-04-22)](testing/2026-04-22-settings-route-index-and-deeplink-checkpoint.md) | Canonical `settings.*` route index and deep-link stability contract for #1406 |
| [Epic 21 Settings Visual Snapshot Baseline (2026-04-21)](testing/2026-04-21-epic-21-settings-visual-snapshot-baseline.md) | Baseline capture inventory and visual comparison workflow for #1408 settings-window snapshots |
| [Epic 21 Theme and Tab Refresh Checkpoint (2026-04-21)](testing/2026-04-21-epic-21-theme-tab-refresh-checkpoint.md) | Theme-tokenized tab-strip styling and affordance validation checkpoint for #1409 |
| [Epic 21 Speed Slider Prototype Checkpoint (2026-04-21)](testing/2026-04-21-epic-21-speed-slider-prototype-checkpoint.md) | Speed-slider prototype evidence pack, benchmark results, and ship/no-ship decision for #1410 |
| [Epic 21 UI Design System and Component Spec (2026-04-21)](plans/2026-04-21-epic-21-ui-design-system-spec.md) | Design tokens, component anatomy, Avalonia implementation mapping, and sign-off checklist for #1416 |
| [Epic 23 Menu, TAS, and Input Validation Matrix (2026-03-29)](testing/epic-23-menu-tas-input-validation-matrix-2026-03-29.md) | Consolidated validation matrix and evidence index for #1071-#1075 |
| [Input Mapping Coverage Checkpoint (2026-03-30)](testing/input-mapping-coverage-checkpoint-2026-03-30.md) | Focused per-system controller mapping decode coverage checkpoint and validation evidence for #1073 |
| [UI Quality Modernization Checkpoint (2026-03-30)](testing/ui-quality-modernization-checkpoint-2026-03-30.md) | Measurable UI consistency and menu/dialog no-regression checkpoint for #1074 |
| [Issue #1290 Menu/Config Pause Should Not Trigger On ROM Load (2026-04-15)](plans/2026-04-15-issue-1290-menu-config-pause-should-not-trigger-on-rom-load.md) | Root-cause analysis and fix plan for menu/config pause incorrectly triggering during ROM load |
| [Issue #1289 Notification Box Bottom-Left Layout Plan (2026-04-15)](plans/2026-04-15-issue-1289-notification-box-bottom-left-layout.md) | Implementation plan and validation notes for moving save/action notification boxes to bottom-left with margins |
| [Issue #1291 Settings Persistence Root Cause + Plan (2026-04-15)](plans/2026-04-15-issue-1291-settings-persistence-root-cause.md) | Root-cause analysis and implemented fix plan for settings not being persisted after exit/relaunch |
| [Issue #1295 Writable Config Fallback And Migration Plan (2026-04-15)](plans/2026-04-15-issue-1295-writable-config-fallback-and-migration.md) | Startup fallback and settings migration strategy when portable config path is not writable |
| [Issue #1296 v1.4.32 Stabilization And Release Plan (2026-04-15)](plans/2026-04-15-issue-1296-v1.4.32-stabilization-and-release.md) | Release-stabilization plan for commit-all workflow, warning verification, and v1.4.32 publication |
| [Release Notes v1.4.38](release-notes/release-notes-v1.4.38.md) | Stable-release notes and validation summary for v1.4.38 |
| [Release Notes v1.4.37](release-notes/release-notes-v1.4.37.md) | Testing-release notes, caution banner, and full release description for v1.4.37 |
| [Issue #1297 Warning Inventory And CI Gate Strategy (2026-04-15)](plans/2026-04-15-issue-1297-warning-inventory-and-ci-gate-strategy.md) | Warning hotspot inventory and staged CI gate strategy for first-party versus third-party warning debt |
| [Issue #1298 Pansy Dependency Decoupling Plan (2026-04-15)](plans/2026-04-15-issue-1298-pansy-dependency-decoupling-plan.md) | Root-cause analysis and conditional ProjectReference/PackageReference strategy for removing hard sibling Pansy dependency |
| [Issue #1302 Pansy Package-Only Enforcement Plan (2026-04-15)](plans/2026-04-15-issue-1302-pansy-package-only-enforcement.md) | Final package-only migration plan removing sibling checkout assumptions from project files and CI |
| [Issue #1299 Breakpoint Unsupported CPU Type Root Cause And Fix (2026-04-15)](plans/2026-04-15-issue-1299-breakpoint-unsupported-cpu-type-root-cause-and-fix.md) | Mapping gap analysis and regression-test plan for breakpoint editor failures caused by unmapped memory types |
| [Pansy Package-Only Validation Benchmark (2026-04-15)](testing/pansy-package-only-validation-benchmark-2026-04-15.md) | Local build/test timing checkpoint and package-only dependency validation evidence |
| [User Future Work Index](../docs/FUTURE-WORK.md) | Public roadmap entry for active and upcoming tracks |
| [User Tutorials Index](../docs/TUTORIALS.md) | Step-by-step user and contributor workflow tutorials |

GitHub issue template for gate-driven phase execution:

- `.github/ISSUE_TEMPLATE/platform-parity-phase-gate.yml`

## 📊 Current Status

### Feature Status

| Feature | Status | Notes |
| --------- | -------- | ------- |
| C++23 Modernization | ✅ Complete | Smart pointers, ranges, format |
| Unit Tests | ✅ Complete | 2790 tests (1633 C++, 826 .NET, 331 MovieConverter) |
| Pansy Export | ✅ Complete | All 7 phases |
| Infinite Save States | ✅ Complete | Visual picker, timestamped |
| TAS Editor | ✅ Complete | Piano roll, greenzone, Lua |
| Documentation | ✅ Complete | User and developer docs |

### Branch Overview

| Branch | Purpose | Status |
| -------- | --------- | -------- |
| `master` | Stable releases | 🔒 Protected |
| `cpp-modernization` | C++ updates | ✅ Merged |
| `pansy-export` | Pansy integration | ✅ Merged |
| `feature/tas-movie-system` | TAS Editor | 🔄 Active |

## 🔗 External Links

| Resource | URL |
| ---------- | ----- |
| Repository | [github.com/TheAnsarya/Nexen](https://github.com/TheAnsarya/Nexen) |
| Issues | [GitHub Issues](https://github.com/TheAnsarya/Nexen/issues) |
| Actions | [CI/CD Builds](https://github.com/TheAnsarya/Nexen/actions) |
| Pansy | [github.com/TheAnsarya/pansy](https://github.com/TheAnsarya/pansy) |
| Peony | [github.com/TheAnsarya/peony](https://github.com/TheAnsarya/peony) |
| Poppy | [github.com/TheAnsarya/poppy](https://github.com/TheAnsarya/poppy) |
