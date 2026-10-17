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
| [UI Theme System Guide](ui-theme-system.md) | Centralized brown/orange brand theme tokens and migration guidance for UI contributors |
| [Theme Profile Format](theme-profile-format.md) | Import/export JSON schema and runtime application rules for customizable UI theme profiles |

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
| [libspng Package Prototype Track](plans/libspng-package-prototype-track.md) | Issue #2188 implementation track for optional package-backed libspng migration scaffolding |
| [kissfft Package Migration Track](plans/kissfft-package-migration-track.md) | Issue #2190 migration track for removing vendored kissfft header and using package-managed dependency |
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
| [Issue #1469 Sega CD Integration Decomposition (2026-04-27)](plans/2026-04-27-issue-1469-sega-cd-integration-decomposition.md) | Child-slice decomposition for CD/memory overlays, sub-CPU+audio determinism gates, and tooling/UX touchpoints |
| [Issue #1470 Power Base Prerequisite Decomposition (2026-04-27)](plans/2026-04-27-issue-1470-power-base-prereq-decomposition.md) | Child-slice decomposition for SMS prerequisite matrix, Power Base risk validation, and Genesis-side sequencing |
| [Issue #1472 UI/TAS/Cheat/Debugger Backlog Decomposition (2026-04-27)](plans/2026-04-27-issue-1472-ui-tas-cheat-debugger-backlog-decomposition.md) | Child-slice decomposition for config UX parity, TAS/cheat deterministic planning, and debugger/tooling/docs closure |
| [Issue #1536 Sega CD Subsystem Track Decomposition (2026-04-27)](plans/2026-04-27-issue-1536-sega-cd-subsystem-track-decomposition.md) | Child-slice decomposition for subsystem task matrix, determinism/replay gates, and closure evidence checklist |
| [Issue #1537 32X and Power Base Closure Track Decomposition (2026-04-27)](plans/2026-04-27-issue-1537-32x-power-base-closure-track-decomposition.md) | Child-slice decomposition for closure matrix refresh, determinism/regression gates, and closure evidence checklist |
| [Issue #1538 Controller and UX Tooling Closure Track Decomposition (2026-04-27)](plans/2026-04-27-issue-1538-controller-ux-closure-track-decomposition.md) | Child-slice decomposition for controller matrix refresh, UX/tooling determinism gates, and closure evidence checklist |
| [Issue #1542 Sega CD CD-Block/Overlay Phase Decomposition (2026-04-27)](plans/2026-04-27-issue-1542-cdblock-overlay-phase-decomposition.md) | Child-slice decomposition for boundary ownership checklist, timing-risk matrix, and phase closure evidence |
| [Issue #1543 Sub-CPU and PCM/CDDA Determinism Decomposition (2026-04-27)](plans/2026-04-27-issue-1543-subcpu-pcm-determinism-decomposition.md) | Child-slice decomposition for sub-CPU sync matrix, PCM/CDDA determinism gates, and phase closure evidence |
| [Issue #1544 Sega CD Tooling Touchpoint Decomposition (2026-04-27)](plans/2026-04-27-issue-1544-sega-cd-tooling-touchpoint-decomposition.md) | Child-slice decomposition for tooling inventory/ownership, UX regression gates, and closure evidence checklist |
| [Issue #1539 SH2 Execution and Arbitration Phase Decomposition (2026-04-27)](plans/2026-04-27-issue-1539-sh2-arbitration-phase-decomposition.md) | Child-slice decomposition for SH2 boundary/invariant matrix, arbitration validation gates, and phase closure evidence |
| [Issue #1540 VDP/Passthrough Phase Decomposition (2026-04-27)](plans/2026-04-27-issue-1540-vdp-passthrough-phase-decomposition.md) | Child-slice decomposition for composition ownership matrix, passthrough validation gates, and phase closure evidence |
| [Issue #1541 PWM Phase Decomposition (2026-04-27)](plans/2026-04-27-issue-1541-pwm-phase-decomposition.md) | Child-slice decomposition for PWM sequencing matrix, correctness/determinism gates, and phase closure evidence |
| [Issue #1545 SMS Prerequisite Phase Decomposition (2026-04-27)](plans/2026-04-27-issue-1545-sms-prereq-phase-decomposition.md) | Child-slice decomposition for SMS compatibility boundaries, prerequisite validation gates, and phase closure evidence |
| [Issue #1546 Power Base Risk Phase Decomposition (2026-04-27)](plans/2026-04-27-issue-1546-power-base-risk-phase-decomposition.md) | Child-slice decomposition for risk taxonomy/ownership, validation checkpoint gates, and phase closure evidence |
| [Issue #1547 Genesis-Side Sequencing Phase Decomposition (2026-04-27)](plans/2026-04-27-issue-1547-genesis-side-sequencing-phase-decomposition.md) | Child-slice decomposition for Genesis-side readiness matrix, sequencing/regression gates, and phase closure evidence |
| [Issue #1548 Config UX Phase Decomposition (2026-04-27)](plans/2026-04-27-issue-1548-config-ux-phase-decomposition.md) | Child-slice decomposition for UX surface/gap matrix, prioritization regression gates, and phase closure evidence |
| [Issue #1549 TAS/Cheat Phase Decomposition (2026-04-27)](plans/2026-04-27-issue-1549-tas-cheat-phase-decomposition.md) | Child-slice decomposition for TAS/cheat surface matrix, deterministic coverage gates, and phase closure evidence |
| [Issue #1550 Debugger Parity Phase Decomposition (2026-04-27)](plans/2026-04-27-issue-1550-debugger-parity-phase-decomposition.md) | Child-slice decomposition for debugger surface matrix, regression acceptance gates, and docs closure evidence |
| [Issue #1551 Subsystem Inventory Phase Decomposition (2026-04-27)](plans/2026-04-27-issue-1551-subsystem-inventory-phase-decomposition.md) | Child-slice decomposition for subsystem task inventory matrix, dependency milestone gates, and phase closure evidence |
| [Issue #1552 Determinism Gate Phase Decomposition (2026-04-27)](plans/2026-04-27-issue-1552-determinism-gate-phase-decomposition.md) | Child-slice decomposition for determinism gate matrix, replay regression checkpoints, and phase closure evidence |
| [Issue #1553 Sega CD Closure Evidence Phase Decomposition (2026-04-27)](plans/2026-04-27-issue-1553-segacd-closure-evidence-phase-decomposition.md) | Child-slice decomposition for closure artifact inventory, evidence mapping checklist gates, and signoff packaging |
| [Issue #1554 32X+Power Base Closure Matrix Decomposition (2026-04-27)](plans/2026-04-27-issue-1554-32x-powerbase-closure-matrix-decomposition.md) | Child-slice decomposition for open integration inventory, dependency milestone gates, and closure publication signoff |
| [Issue #1555 32X/Power Base Determinism Gate Pack Decomposition (2026-04-27)](plans/2026-04-27-issue-1555-32x-powerbase-determinism-gates-decomposition.md) | Child-slice decomposition for deterministic gate inventory, regression checkpoint matrix, and validation gate-pack publication |
| [Issue #1556 32X+Power Base Closure Evidence Decomposition (2026-04-27)](plans/2026-04-27-issue-1556-32x-powerbase-closure-evidence-decomposition.md) | Child-slice decomposition for closure artifact inventory, evidence gate ordering, and final signoff packaging |
| [Issue #1557 Controller/Peripheral Closure Matrix Decomposition (2026-04-27)](plans/2026-04-27-issue-1557-controller-peripheral-closure-matrix-decomposition.md) | Child-slice decomposition for routing gap inventory, dependency milestone gates, and closure publication signoff |
| [Issue #1558 Controller UX/Tooling Determinism Gate Pack Decomposition (2026-04-27)](plans/2026-04-27-issue-1558-controller-ux-determinism-gates-decomposition.md) | Child-slice decomposition for deterministic gate inventory, regression checkpoint matrix, and validation gate-pack publication |
| [Issue #1559 Controllers/UX Closure Evidence Decomposition (2026-04-27)](plans/2026-04-27-issue-1559-controllers-ux-closure-evidence-decomposition.md) | Child-slice decomposition for closure artifact inventory, evidence gate ordering, and final signoff packaging |
| [Issue #1560 CD-Block Boundary Ownership/Readiness Decomposition (2026-04-27)](plans/2026-04-27-issue-1560-cdblock-boundary-readiness-decomposition.md) | Child-slice decomposition for boundary ownership mapping, readiness gate criteria, and follow-up linkage closure path |
| [Issue #1494 v1.4.38 Release Publication Closeout (2026-04-28)](plans/2026-04-28-issue-1494-v1.4.38-release-publication-closeout.md) | Release-closeout evidence pack for v1.4.38 publication status, assets, and acceptance checklist closure |
| [Issues #1518/#1520/#1523 SMS/PBC Readiness and Parity Closeout (2026-04-28)](plans/2026-04-28-issues-1518-1520-1523-sms-pbc-readiness-parity-closeout.md) | Consolidated readiness, UI/runtime parity, and CI/constraint documentation closeout for SMS/PBC phase slices |
| [Issues #1530/#1532 Controller/Input Parity Closeout (2026-04-28)](plans/2026-04-28-issues-1530-1532-controller-input-parity-closeout.md) | Controller/peripheral support matrix and input-configuration parity checklist closeout with issue-tracked follow-up |
| [Issues #1533/#1535 Debugger and UX/Config Parity Closeout (2026-04-28)](plans/2026-04-28-issues-1533-1535-debugger-ux-parity-closeout.md) | Debugger gap matrix and UX/config parity checklist closure evidence with tracked implementation backlog |
| [Issues #1561/#1562 CD-Block/Overlay Phase Closeout (2026-04-28)](plans/2026-04-28-issues-1561-1562-cdblock-overlay-phase-closeout.md) | Memory-overlay timing risk matrix and phase closure checklist documentation with remaining gaps tracked |
| [Issues #1581-#1586 Power Base Risk and Sequencing Closeout (2026-04-28)](plans/2026-04-28-issues-1581-1586-power-base-risk-sequencing-closeout.md) | Risk taxonomy, validation gates, sequencing matrix, and closure evidence checklist pack for Power Base phases |
| [Issues #1587-#1589 Config UX Phase Closeout (2026-04-28)](plans/2026-04-28-issues-1587-1589-config-ux-phase-closeout.md) | Config UX inventory, prioritization/gate pack, and closure evidence documentation with follow-up linkage |
| [Issues #1590-#1592 TAS/Cheat Phase Closeout (2026-04-28)](plans/2026-04-28-issues-1590-1592-tas-cheat-phase-closeout.md) | TAS/cheat feature-surface matrix, deterministic gate pack, and phase closure evidence checklist |
| [Issues #1593-#1595 Debugger Tooling/Docs Closeout (2026-04-28)](plans/2026-04-28-issues-1593-1595-debugger-tooling-docs-closeout.md) | Debugger parity matrix, regression acceptance pack, and docs link-tree closure evidence checklist |
| [Platform Parity Benchmark and Correctness Gates](plans/platform-parity-benchmark-and-correctness-gates.md) | Cross-phase quality gates and evidence framework (#703) |
| [Atari 2600 + Genesis Parity Tracker](plans/atari2600-genesis-parity-tracker.md) | Active multi-phase checklist, issue linkage, and closure criteria for parity execution (#750) |
| [UI Settings Completeness Plan](plans/ui-settings-completeness-epic-18.md) | Epic #1040 execution plan for cross-platform settings/input/movie/savestate UX completeness |
| [Epic 12 UI Theme System Plan (2026-05-21)](plans/2026-05-21-epic-12-ui-theme-system-plan.md) | Centralized UI theme tokens and brown/orange brand application plan for startup/setup surfaces (#2257-#2260) |
| [Issue #2290 UI Theme Chrome Token Centralization (2026-05-22)](plans/2026-05-22-issue-2290-ui-theme-chrome-token-centralization.md) | Extend theme profiles to drive global menu/chrome/accent tokens with settings customization and import/export compatibility |
| [Issue #2291 Control Semantic Theme Tokens (2026-05-23)](plans/2026-05-23-issue-2291-control-semantic-theme-tokens.md) | Centralize button/combo/repeat hover+pressed semantic token families into theme profiles and settings customization |
| [Issue #2294 Sidebar and Tab Chrome Theme Tokens (2026-05-23)](plans/2026-05-23-issue-2294-sidebar-tab-chrome-theme-tokens.md) | Extend theme profiles with sidebar border and dock tab strip/hover/active chrome token customization |
| [Issue #2295 Checkbox/Radio/Slider Semantic Theme Tokens (2026-05-23)](plans/2026-05-23-issue-2295-checkbox-radio-slider-semantic-theme-tokens.md) | Extend theme profiles with checkbox/radio pointer-over+pressed and slider hover+pressed track semantic token customization |
| [Issue #2296 Text-Input and Tooltip/Flyout Semantic Theme Tokens (2026-05-23)](plans/2026-05-23-issue-2296-text-input-tooltip-flyout-semantic-theme-tokens.md) | Extend theme profiles with text selection/disabled and tooltip/menu-flyout semantic token customization |
| [Issue #2297 ComboBox Dropdown and DataGrid Semantic Theme Tokens (2026-05-23)](plans/2026-05-23-issue-2297-combobox-datagrid-semantic-theme-tokens.md) | Extend theme profiles with ComboBox dropdown and DataGrid header/selected-row semantic token customization |
| [Issue #2298 ListBox and TreeView Semantic Theme Tokens (2026-05-23)](plans/2026-05-23-issue-2298-listbox-treeview-semantic-theme-tokens.md) | Extend theme profiles with ListBox/TreeView hover and selected semantic token customization |
| [Issue #2299 Semantic Token Roundtrip Regression (2026-05-23)](plans/2026-05-23-issue-2299-semantic-token-roundtrip-regression.md) | Add regression test to verify semantic token persistence through profile export/import and save-cycle |
| [Issue #2300 Theme Customization Semantic Grouping (2026-05-23)](plans/2026-05-23-issue-2300-theme-customization-semantic-grouping.md) | Reorganize theme customization pickers into semantic component groups with localized section headers |
| [Issue #2301 NavigationView and ListView Semantic Theme Tokens (2026-05-23)](plans/2026-05-23-issue-2301-navigationview-listview-semantic-theme-tokens.md) | Extend theme profiles with NavigationView/ListView hover and selected semantic token customization |
| [Issue #2302 Theme Group Ordering Regression Test (2026-05-23)](plans/2026-05-23-issue-2302-theme-group-ordering-regression-test.md) | Add markup regression coverage to enforce semantic group header ordering in theme customization UI |
| [Issue #2303 Semantic Theme-Mode Default Regression (2026-05-23)](plans/2026-05-23-issue-2303-semantic-theme-mode-default-regression.md) | Add regression coverage that validates semantic token defaults remain valid and mode-specific across light/dark themes |
| [Issue #2304 Semantic Theme Resource Key Presence Regression (2026-05-23)](plans/2026-05-23-issue-2304-semantic-theme-resource-key-presence-regression.md) | Add regression coverage to ensure semantic list/tree/navigation/listview theme keys exist in both light and dark dictionaries |
| [Issue #2305 NexenThemeManager Semantic Override Mapping Regression (2026-05-23)](plans/2026-05-23-issue-2305-nexenthememanager-semantic-override-mapping-regression.md) | Add regression coverage for semantic `ApplyColorOverride`/`ApplyBrushOverride` mappings in NexenThemeManager |
| [Issue #2306 Preferences Semantic Token Wiring Regression (2026-05-23)](plans/2026-05-23-issue-2306-preferences-semantic-token-wiring-regression.md) | Add regression coverage for NavigationView/ListView semantic wiring across preferences viewmodel, bindings, and handlers |
| [Issue #2307 Semantic Token Divergence/Customization Regression for NavigationView/ListView (2026-05-23)](plans/2026-05-23-issue-2307-semantic-token-divergence-customization-regression-nav-listview.md) | Add regression coverage to ensure divergence counting and customized-token reporting include NavigationView/ListView semantic families |
| [Issue #2308 Semantic Token Reset/Preset Propagation Regression for NavigationView/ListView (2026-05-23)](plans/2026-05-23-issue-2308-semantic-token-reset-preset-propagation-regression-nav-listview.md) | Add regression coverage for reset-to-default and preset-apply propagation of NavigationView/ListView semantic tokens |
| [Issue #2309 Semantic Token Clone/Upsert Propagation Regression for NavigationView/ListView (2026-05-23)](plans/2026-05-23-issue-2309-semantic-token-clone-upsert-propagation-regression-nav-listview.md) | Add regression coverage for duplicate/upsert pipelines preserving and updating NavigationView/ListView semantic tokens |
| [Issue #2316 SaveCurrent Fallback Regression for NavigationView/ListView (2026-05-23)](plans/2026-05-23-issue-2316-savecurrent-fallback-regression-nav-listview.md) | Add regression coverage for SaveCurrentToProfile no-resource fallback path preserving NavigationView/ListView semantic token values |
| [Issue #2317 ThemeProfile Validation Regression for NavigationView/ListView Semantic Colors (2026-05-23)](plans/2026-05-23-issue-2317-validation-regression-nav-listview-semantic-colors.md) | Add regression coverage ensuring ThemeProfile file validation rejects invalid NavigationView/ListView semantic color fields |
| [Issue #2318 Theme Profile Rename/Unique Trim Conflict Regression (2026-05-23)](plans/2026-05-23-issue-2318-rename-unique-trim-conflict-regression.md) | Add regression coverage for trimmed-name conflict handling in generate-unique and rename profile workflows |
| [Issue #2319 Delete Active Profile Fallback Regression (2026-05-23)](plans/2026-05-23-issue-2319-delete-active-profile-fallback-regression.md) | Add regression coverage ensuring deleting the active profile falls back correctly while preserving fallback semantic token values |
| [Issue #2320 Delete Single Profile Guard Regression (2026-05-23)](plans/2026-05-23-issue-2320-delete-single-profile-guard-regression.md) | Add regression coverage for protection against deleting the final remaining theme profile |
| [Issue #2321 SetActive Invalid Name No-Op Regression (2026-05-23)](plans/2026-05-23-issue-2321-setactive-invalid-name-noop-regression.md) | Add regression coverage ensuring invalid profile activation requests are no-ops |
| [Epic 25 Architecture Simplification and Redundancy Reduction Roadmap (2026-05-23)](plans/2026-05-23-epic-25-architecture-simplification-roadmap.md) | Program-level architecture simplification plan covering debugger dispatch unification, evaluator consolidation, and notification lifecycle streamlining |
| [Issue #2326 SaveRom Polymorphic Dispatch Unification (2026-05-23)](plans/2026-05-23-issue-2326-saverom-polymorphic-dispatch-unification.md) | Replace central SaveRom switch fanout with IDebugger polymorphic dispatch while preserving SGB routing semantics |
| [Issue #2327 NotificationManager Lifecycle Simplification (2026-05-23)](plans/2026-05-23-issue-2327-notificationmanager-lifecycle-simplification.md) | Plan simplification of listener lifecycle handling and cleanup overhead in NotificationManager |
| [Issue #2328 Architecture Inventory and Phased Simplification Roadmap (2026-05-23)](plans/2026-05-23-issue-2328-architecture-inventory-roadmap.md) | Inventory architecture redundancy hotspots and define phased modernization execution slices |
| [Issue #2331 PauseOnNextFrame Data-Driven Dispatch (2026-05-23)](plans/2026-05-23-issue-2331-pauseonnextframe-data-driven-dispatch.md) | Replace PauseOnNextFrame CPU switch fanout with compact data-driven scanline mapping |
| [Issue #2332 NotificationManager Benchmark Coverage (2026-05-23)](plans/2026-05-23-issue-2332-notificationmanager-benchmark-coverage.md) | Add focused benchmark coverage for NotificationManager dispatch throughput and expired-listener scenarios |
| [Issue #2336 CPU State Size Dispatch Consolidation (2026-05-23)](plans/2026-05-23-issue-2336-cpu-state-size-dispatch-consolidation.md) | Consolidate duplicated Debugger CPU-state memcpy size mapping into one correctness-safe helper |
| [Issue #2337 PauseOnNextFrame Scanline Mapping Tests (2026-05-23)](plans/2026-05-23-issue-2337-pauseonnextframe-scanline-mapping-tests.md) | Add direct regression tests for PauseOnNextFrame scanline mapping and unsupported fallback behavior |
| [Issue #2338 Core.Benchmarks LNK4020 Warning Hygiene (2026-05-23)](plans/2026-05-23-issue-2338-core-benchmarks-lnk4020-warning-hygiene.md) | Apply benchmark-project link-setting hygiene to eliminate repeated LNK4020 warning churn |
| [Issue #2339 CPU State-Size Mapping in Shared Dispatch Utils (2026-05-23)](plans/2026-05-23-issue-2339-cpu-state-size-mapping-shared-dispatch-utils.md) | Centralize debugger CPU-family-to-layout mapping in shared dispatch utilities |
| [Issue #2340 CPU State-Size Mapping Regression Tests (2026-05-23)](plans/2026-05-23-issue-2340-cpu-state-size-mapping-regression-tests.md) | Add direct unit-test coverage for CPU state-layout mapping behavior |
| [Issue #2341 Debug Window Flag Mapping in Shared Dispatch Utils (2026-05-23)](plans/2026-05-23-issue-2341-debug-window-flag-mapping-shared-dispatch-utils.md) | Consolidate IsDebugWindowOpened CPU-to-flag mapping behind shared optional utility lookup |
| [Issue #2351 BreakSource Config Mapping in Shared Dispatch Utils (2026-05-23)](plans/2026-05-23-issue-2351-breaksource-config-mapping-shared-dispatch-utils.md) | Centralize BreakSource-to-DebugConfig mapping behind shared dispatch utility function |
| [Issue #2352 BreakSource Mapping Regression Tests (2026-05-23)](plans/2026-05-23-issue-2352-breaksource-mapping-regression-tests.md) | Add direct utility tests for mapped BreakSource config flags and default-true behavior |
| [Issue #2353 PPU State Routing Deduplication (2026-05-23)](plans/2026-05-23-issue-2353-ppu-state-routing-deduplication.md) | Remove duplicated GetPpuState/SetPpuState CPU routing via shared state-action helper |
| [Issue #2357 PPU Backend Routing Mapping Extraction (2026-05-23)](plans/2026-05-23-issue-2357-ppu-backend-routing-mapping-extraction.md) | Extract CPU-to-PPU-backend routing map into shared debugger dispatch utility |
| [Issue #2358 PPU Backend Routing Mapping Regression Tests (2026-05-23)](plans/2026-05-23-issue-2358-ppu-backend-routing-mapping-regression-tests.md) | Add full utility test coverage for PPU backend mapping across supported and unsupported CPUs |
| [Issue #2359 ProcessPpuStateAction Shared Backend Routing (2026-05-23)](plans/2026-05-23-issue-2359-processppustateaction-shared-backend-routing.md) | Refactor ProcessPpuStateAction to consume shared PPU backend mapping enum |
| [Debugger ProcessEvent Architecture Audit (2026-05-23)](plans/2026-05-23-debugger-process-event-architecture-audit.md) | Architecture analysis of ProcessEvent routing/dispatch coupling and phased simplification plan |
| [Issue #2361 ProcessEvent CPU Routing Helper Extraction (2026-05-23)](plans/2026-05-23-issue-2361-processevent-cpu-routing-helper-extraction.md) | Extract requested-vs-main CPU routing decisions into shared debugger dispatch helpers |
| [Issue #2363 ProcessEvent Routing Helper Regression Tests (2026-05-23)](plans/2026-05-23-issue-2363-processevent-routing-helper-regression-tests.md) | Add direct utility tests for ProcessEvent routing and input fallback decision behavior |
| [Issue #2364 ProcessEvent Refactor to Shared Routing Helpers (2026-05-23)](plans/2026-05-23-issue-2364-processevent-refactor-shared-routing-helpers.md) | Refactor ProcessEvent to consume shared routing helpers and reduce inline branch duplication |
| [Debugger BreakRequest and ProcessEvent Architecture Audit (2026-05-23)](plans/2026-05-23-debugger-breakrequest-processevent-architecture-audit.md) | Audit of ProcessEvent and break-request hotspots with phased simplification strategy |
| [Issue #2365 ProcessEvent Script-Ownership and Event-Target Helpers (2026-05-23)](plans/2026-05-23-issue-2365-processevent-script-ownership-event-target-helpers.md) | Extract script ownership gating and event-target resolution decisions into shared helpers |
| [Issue #2366 ProcessEvent Per-Event Handler Split (2026-05-23)](plans/2026-05-23-issue-2366-processevent-per-event-handler-split.md) | Decompose ProcessEvent switch branches into dedicated handler methods to simplify coordinator flow |
| [Issue #2367 ProcessEvent Integration-Level Regression Coverage (2026-05-23)](plans/2026-05-23-issue-2367-processevent-integration-regression-coverage.md) | Add composed ProcessEvent decision tests for ownership gating, fallback routing, and start-frame branch outcomes |
| [Debugger SleepUntilResume Architecture Audit (2026-05-23)](plans/2026-05-23-debugger-sleepuntilresume-architecture-audit.md) | Audit of SleepUntilResume guard branching with helper-driven simplification strategy |
| [Issue #2368 SleepUntilResume Guard Decision Helpers (2026-05-23)](plans/2026-05-23-issue-2368-sleepuntilresume-guard-decision-helpers.md) | Extract typed guard decision evaluation helpers from SleepUntilResume early-return logic |
| [Issue #2369 SleepUntilResume Guard Regression Coverage (2026-05-23)](plans/2026-05-23-issue-2369-sleepuntilresume-guard-regression-coverage.md) | Add deterministic unit coverage for all SleepUntilResume guard decision outcomes |
| [Issue #2370 SleepUntilResume Shared Decision Refactor (2026-05-23)](plans/2026-05-23-issue-2370-sleepuntilresume-shared-decision-refactor.md) | Refactor SleepUntilResume to evaluate shared guard decisions and dispatch through explicit decision switch |
| [Debugger SleepUntilResume Notification Policy Audit (2026-05-23)](plans/2026-05-23-debugger-sleepuntilresume-notification-policy-audit.md) | Audit of SleepUntilResume notification eligibility and wait-delay policy with helper-driven simplification |
| [Issue #2371 SleepUntilResume Break-Notification Policy Helpers (2026-05-23)](plans/2026-05-23-issue-2371-sleepuntilresume-break-notification-policy-helpers.md) | Extract reusable helpers for break-notification eligibility and resume-loop delay policy |
| [Issue #2381 SleepUntilResume Notification/Loop Policy Regression Tests (2026-05-23)](plans/2026-05-23-issue-2381-sleepuntilresume-notification-loop-policy-regression-tests.md) | Add deterministic helper tests for notification emission rules and loop-delay mapping |
| [Issue #2382 SleepUntilResume Shared Notification Policy Refactor (2026-05-23)](plans/2026-05-23-issue-2382-sleepuntilresume-shared-notification-policy-refactor.md) | Refactor SleepUntilResume coordinator to consume shared notification and delay policy helpers |
| [Debugger SleepUntilResume Pre-Break Side-Effect Audit (2026-05-24)](plans/2026-05-24-debugger-sleepuntilresume-prebreak-side-effect-audit.md) | Audit of pre-break side-effect branching and helper-driven policy extraction strategy |
| [Issue #2383 SleepUntilResume Pre-Break Side-Effect Policy Helpers (2026-05-24)](plans/2026-05-24-issue-2383-sleepuntilresume-prebreak-side-effect-policy-helpers.md) | Extract typed helper context/outcome for pre-break side-effect policy decisions |
| [Issue #2384 SleepUntilResume Pre-Break Side-Effect Regression Tests (2026-05-24)](plans/2026-05-24-issue-2384-sleepuntilresume-prebreak-side-effect-regression-tests.md) | Add deterministic helper tests for emitted vs non-emitted pre-break side-effect outcomes |
| [Issue #2385 SleepUntilResume Shared Pre-Break Side-Effect Refactor (2026-05-24)](plans/2026-05-24-issue-2385-sleepuntilresume-prebreak-side-effect-refactor.md) | Refactor SleepUntilResume coordinator to consume shared pre-break side-effect policy outcomes |
| [Debugger SleepUntilResume Continuation-Loop Policy Audit (2026-05-24)](plans/2026-05-24-debugger-sleepuntilresume-continuation-loop-policy-audit.md) | Audit of continuation-loop branching and helper-driven continuation/delay policy extraction |
| [Issue #2386 SleepUntilResume Continuation-Loop Policy Helpers (2026-05-24)](plans/2026-05-24-issue-2386-sleepuntilresume-continuation-loop-policy-helpers.md) | Extract typed helper context/outcome for continuation-loop and delay policy decisions |
| [Issue #2387 SleepUntilResume Continuation-Loop Regression Tests (2026-05-24)](plans/2026-05-24-issue-2387-sleepuntilresume-continuation-loop-regression-tests.md) | Add deterministic helper tests for continuation-loop truth table and delay mapping |
| [Issue #2388 SleepUntilResume Shared Continuation-Loop Refactor (2026-05-24)](plans/2026-05-24-issue-2388-sleepuntilresume-continuation-loop-refactor.md) | Refactor SleepUntilResume coordinator to consume shared continuation-loop policy outcomes |
| [Debugger SleepUntilResume Post-Loop Side-Effect Policy Audit (2026-05-24)](plans/2026-05-24-debugger-sleepuntilresume-postloop-side-effect-policy-audit.md) | Audit of post-loop side-effect branching and helper-driven screensaver/notification policy extraction |
| [Issue #2391 SleepUntilResume Post-Loop Side-Effect Policy Helpers (2026-05-24)](plans/2026-05-24-issue-2391-sleepuntilresume-postloop-side-effect-policy-helpers.md) | Extract typed helper context/outcome for post-loop side-effect policy decisions |
| [Issue #2392 SleepUntilResume Post-Loop Side-Effect Regression Tests (2026-05-24)](plans/2026-05-24-issue-2392-sleepuntilresume-postloop-side-effect-regression-tests.md) | Add deterministic helper tests for post-loop side-effect outcomes by notification state |
| [Issue #2393 SleepUntilResume Shared Post-Loop Side-Effect Refactor (2026-05-24)](plans/2026-05-24-issue-2393-sleepuntilresume-postloop-side-effect-refactor.md) | Refactor SleepUntilResume coordinator tail to consume shared post-loop side-effect outcomes |
| [Debugger SleepUntilResume Break-Event Payload Policy Audit (2026-05-24)](plans/2026-05-24-debugger-sleepuntilresume-breakevent-payload-policy-audit.md) | Audit of break-event payload assembly and helper-driven optional operation propagation policy |
| [Issue #2396 SleepUntilResume Break-Event Payload Policy Helpers (2026-05-24)](plans/2026-05-24-issue-2396-sleepuntilresume-breakevent-payload-policy-helpers.md) | Extract typed helper context/outcome for break-event payload construction |
| [Issue #2398 SleepUntilResume Break-Event Payload Regression Tests (2026-05-24)](plans/2026-05-24-issue-2398-sleepuntilresume-breakevent-payload-regression-tests.md) | Add deterministic helper tests for break-event payload values and optional operation copy behavior |
| [Issue #2399 SleepUntilResume Shared Break-Event Payload Refactor (2026-05-24)](plans/2026-05-24-issue-2399-sleepuntilresume-breakevent-payload-refactor.md) | Refactor SleepUntilResume coordinator notification payload path to consume shared helper outcomes |
| [Debugger SleepUntilResume Pre-Loop Notification Sequence Audit (2026-05-24)](plans/2026-05-24-debugger-sleepuntilresume-preloop-notification-sequence-audit.md) | Audit of pre-loop notification sequence gating and helper-driven side-effect eligibility policy |
| [Issue #2400 SleepUntilResume Pre-Loop Notification Sequence Helpers (2026-05-24)](plans/2026-05-24-issue-2400-sleepuntilresume-preloop-notification-sequence-helpers.md) | Extract typed helper context/outcome for pre-loop notification sequence policy |
| [Issue #2401 SleepUntilResume Pre-Loop Notification Sequence Regression Tests (2026-05-24)](plans/2026-05-24-issue-2401-sleepuntilresume-preloop-notification-sequence-regression-tests.md) | Add deterministic helper tests for pre-loop sequence and side-effect eligibility outcomes |
| [Issue #2402 SleepUntilResume Shared Pre-Loop Notification Sequence Refactor (2026-05-24)](plans/2026-05-24-issue-2402-sleepuntilresume-preloop-notification-sequence-refactor.md) | Refactor SleepUntilResume pre-loop block to consume shared sequence policy outcomes |
| [Debugger SleepUntilResume Notification-Dispatch Trigger Audit (2026-05-24)](plans/2026-05-24-debugger-sleepuntilresume-notification-dispatch-trigger-audit.md) | Audit of code-break dispatch trigger/sent-state policy and helper-driven coordinator simplification |
| [Issue #2403 SleepUntilResume Notification-Dispatch Trigger Helpers (2026-05-24)](plans/2026-05-24-issue-2403-sleepuntilresume-notification-dispatch-trigger-helpers.md) | Extract typed helper context/outcome for dispatch trigger and sent-state transition policy |
| [Issue #2404 SleepUntilResume Notification-Dispatch Trigger Regression Tests (2026-05-24)](plans/2026-05-24-issue-2404-sleepuntilresume-notification-dispatch-trigger-regression-tests.md) | Add deterministic helper tests for dispatch/process/sent-state trigger outcomes |
| [Issue #2405 SleepUntilResume Notification-Dispatch Trigger Refactor (2026-05-24)](plans/2026-05-24-issue-2405-sleepuntilresume-notification-dispatch-trigger-refactor.md) | Refactor SleepUntilResume dispatch block to consume shared trigger outcomes |
| [Debugger SleepUntilResume Pre-Loop Policy Bundle Audit (2026-05-24)](plans/2026-05-24-debugger-sleepuntilresume-preloop-policy-bundle-audit.md) | Audit of pre-loop helper wiring consolidation via composed bundle outcomes |
| [Issue #2406 SleepUntilResume Pre-Loop Policy Bundle Helpers (2026-05-24)](plans/2026-05-24-issue-2406-sleepuntilresume-preloop-policy-bundle-helpers.md) | Compose pre-break/pre-loop/dispatch policies into a shared bundle helper outcome |
| [Issue #2407 SleepUntilResume Pre-Loop Policy Bundle Regression Tests (2026-05-24)](plans/2026-05-24-issue-2407-sleepuntilresume-preloop-policy-bundle-regression-tests.md) | Add deterministic helper tests for composed pre-loop bundle outcomes |
| [Issue #2408 SleepUntilResume Pre-Loop Policy Bundle Refactor (2026-05-24)](plans/2026-05-24-issue-2408-sleepuntilresume-preloop-policy-bundle-refactor.md) | Refactor SleepUntilResume pre-loop setup to consume composed bundle outcomes |
| [Debugger SleepUntilResume Phase Outcome Model Audit (2026-05-24)](plans/2026-05-24-debugger-sleepuntilresume-phase-outcome-model-audit.md) | Audit of phase-level orchestration boundaries and composed outcome-model simplification strategy |
| [Issue #2409 SleepUntilResume Phase Outcome Model Helpers (2026-05-24)](plans/2026-05-24-issue-2409-sleepuntilresume-phase-outcome-model-helpers.md) | Add phase-level helper context/outcome composition for guard, pre-loop, loop, and post-loop policies |
| [Issue #2410 SleepUntilResume Phase Outcome Model Regression Tests (2026-05-24)](plans/2026-05-24-issue-2410-sleepuntilresume-phase-outcome-model-regression-tests.md) | Add deterministic tests validating composed phase-model decisions and policy outcomes |
| [Issue #2411 SleepUntilResume Phase Outcome Model Refactor (2026-05-24)](plans/2026-05-24-issue-2411-sleepuntilresume-phase-outcome-model-refactor.md) | Refactor SleepUntilResume to consume composed phase outcomes across orchestration phases |
| [Debugger SleepUntilResume Runtime Dispatch Sequence Audit (2026-05-24)](plans/2026-05-24-debugger-sleepuntilresume-runtime-dispatch-sequence-audit.md) | Audit of break-event payload and dispatch-sequence composition opportunities in SleepUntilResume |
| [Issue #2415 SleepUntilResume Runtime Dispatch Sequence Helper (2026-05-24)](plans/2026-05-24-issue-2415-sleepuntilresume-runtime-dispatch-sequence-helper.md) | Extract composed runtime helper outcome for break payload and dispatch-sequence policy decisions |
| [Issue #2416 SleepUntilResume Runtime Dispatch Sequence Regression Tests (2026-05-24)](plans/2026-05-24-issue-2416-sleepuntilresume-runtime-dispatch-sequence-regression-tests.md) | Add deterministic helper tests for emitted and non-emitted runtime dispatch-sequence behavior |
| [Issue #2417 SleepUntilResume Runtime Dispatch Sequence Refactor (2026-05-24)](plans/2026-05-24-issue-2417-sleepuntilresume-runtime-dispatch-sequence-refactor.md) | Refactor SleepUntilResume runtime dispatch block to consume composed helper outcomes |
| [Debugger SleepUntilResume Runtime Side-Effect Transition Audit (2026-05-24)](plans/2026-05-24-debugger-sleepuntilresume-runtime-sideeffect-transition-audit.md) | Audit of runtime wait-arm/screensaver/sent-state transition composition opportunities |
| [Issue #2424 SleepUntilResume Runtime Side-Effect Transition Helper (2026-05-24)](plans/2026-05-24-issue-2424-sleepuntilresume-runtime-sideeffect-transition-helper.md) | Extract composed helper for runtime wait-arm/screensaver/sent-state transition logic |
| [Issue #2425 SleepUntilResume Runtime Side-Effect Transition Regression Tests (2026-05-24)](plans/2026-05-24-issue-2425-sleepuntilresume-runtime-sideeffect-transition-regression-tests.md) | Add deterministic helper tests for emitted and non-emitted runtime side-effect transitions |
| [Issue #2426 SleepUntilResume Runtime Side-Effect Transition Refactor (2026-05-24)](plans/2026-05-24-issue-2426-sleepuntilresume-runtime-sideeffect-transition-refactor.md) | Refactor SleepUntilResume runtime side-effect block to consume composed transition outcomes |
| [Debugger SleepUntilResume Runtime Bundle Audit (2026-05-24)](plans/2026-05-24-debugger-sleepuntilresume-runtime-bundle-audit.md) | Audit of runtime pre-loop bundle composition opportunities across dispatch and side-effect transitions |
| [Issue #2427 SleepUntilResume Runtime Bundle Helper (2026-05-24)](plans/2026-05-24-issue-2427-sleepuntilresume-runtime-bundle-helper.md) | Extract composed runtime bundle helper for dispatch and side-effect transition outcomes |
| [Issue #2428 SleepUntilResume Runtime Bundle Regression Tests (2026-05-24)](plans/2026-05-24-issue-2428-sleepuntilresume-runtime-bundle-regression-tests.md) | Add deterministic tests validating emitted and non-emitted runtime bundle outcomes |
| [Issue #2429 SleepUntilResume Runtime Bundle Refactor (2026-05-24)](plans/2026-05-24-issue-2429-sleepuntilresume-runtime-bundle-refactor.md) | Refactor SleepUntilResume runtime pre-loop orchestration to consume one runtime bundle outcome |
| [Debugger SleepUntilResume Composed-Flow Integration Audit (2026-05-24)](plans/2026-05-24-debugger-sleepuntilresume-composed-flow-integration-audit.md) | Audit of composed emitted/non-emitted flow integration coverage and fixture deduplication opportunities |
| [Issue #2430 SleepUntilResume Composed Emitted-Flow Integration Policy Test (2026-05-24)](plans/2026-05-24-issue-2430-sleepuntilresume-composed-emitted-flow-integration-test.md) | Add deterministic composed-policy test for emitted phase/runtime/loop/post-loop flow behavior |
| [Issue #2431 SleepUntilResume Composed Non-Emitted-Flow Integration Policy Test (2026-05-24)](plans/2026-05-24-issue-2431-sleepuntilresume-composed-nonemitted-flow-integration-test.md) | Add deterministic composed-policy test for non-emitted phase/runtime/loop/post-loop flow behavior |
| [Issue #2432 SleepUntilResume Composed-Flow Fixture Refactor (2026-05-24)](plans/2026-05-24-issue-2432-sleepuntilresume-composed-flow-fixture-refactor.md) | Refactor composed-flow tests to shared fixture builders for reduced setup duplication |
| [Debugger SleepUntilResume Loop/Post-Loop Bundle Audit (2026-05-24)](plans/2026-05-24-debugger-sleepuntilresume-loop-postloop-bundle-audit.md) | Audit of loop/post-loop composed bundle opportunities for reduced coordinator wiring |
| [Issue #2440 SleepUntilResume Loop/Post-Loop Bundle Helper (2026-05-24)](plans/2026-05-24-issue-2440-sleepuntilresume-loop-postloop-bundle-helper.md) | Extract composed helper resolving loop continuation and post-loop side effects from one context |
| [Issue #2441 SleepUntilResume Loop/Post-Loop Bundle Regression Tests (2026-05-24)](plans/2026-05-24-issue-2441-sleepuntilresume-loop-postloop-bundle-regression-tests.md) | Add deterministic tests for composed loop/post-loop bundle outcomes across waiting and notification states |
| [Issue #2442 SleepUntilResume Loop/Post-Loop Bundle Refactor (2026-05-24)](plans/2026-05-24-issue-2442-sleepuntilresume-loop-postloop-bundle-refactor.md) | Refactor SleepUntilResume loop and tail post-loop paths to consume loop/post-loop bundle outcomes |
| [Debugger SleepUntilResume Runtime Context Builder Audit (2026-05-24)](plans/2026-05-24-debugger-sleepuntilresume-runtime-context-builder-audit.md) | Audit for removing coordinator context wiring through a dedicated runtime bundle context builder |
| [Issue #2443 SleepUntilResume Runtime Context Builder Helper (2026-05-24)](plans/2026-05-24-issue-2443-sleepuntilresume-runtime-context-builder-helper.md) | Add helper that composes runtime bundle context from phase outcome and runtime payload |
| [Issue #2444 SleepUntilResume Runtime Context Builder Regression Tests (2026-05-24)](plans/2026-05-24-issue-2444-sleepuntilresume-runtime-context-builder-regression-tests.md) | Add deterministic emitted/non-emitted tests for runtime context builder field mapping |
| [Issue #2445 SleepUntilResume Runtime Context Builder Refactor (2026-05-24)](plans/2026-05-24-issue-2445-sleepuntilresume-runtime-context-builder-refactor.md) | Refactor SleepUntilResume runtime bundle call site to use shared context builder helper |
| [Debugger SleepUntilResume Phase Context Builder Audit (2026-05-24)](plans/2026-05-24-debugger-sleepuntilresume-phase-context-builder-audit.md) | Audit for replacing repeated phase-context coordinator wiring with a dedicated builder helper |
| [Issue #2446 SleepUntilResume Phase Context Builder Helper (2026-05-24)](plans/2026-05-24-issue-2446-sleepuntilresume-phase-context-builder-helper.md) | Add helper that composes phase context from guard state, source, and config toggles |
| [Issue #2447 SleepUntilResume Phase Context Builder Regression Tests (2026-05-24)](plans/2026-05-24-issue-2447-sleepuntilresume-phase-context-builder-regression-tests.md) | Add deterministic tests for phase context builder field mapping and defaults |
| [Issue #2448 SleepUntilResume Phase Context Builder Refactor (2026-05-24)](plans/2026-05-24-issue-2448-sleepuntilresume-phase-context-builder-refactor.md) | Refactor SleepUntilResume to consume phase context builder for both phase-outcome passes |
| [Debugger SleepUntilResume Guard Context Builder Audit (2026-05-24)](plans/2026-05-24-debugger-sleepuntilresume-guard-context-builder-audit.md) | Audit for extracting deterministic guard-context wiring from SleepUntilResume coordinator entry |
| [Issue #2449 SleepUntilResume Guard Context Builder Helper (2026-05-24)](plans/2026-05-24-issue-2449-sleepuntilresume-guard-context-builder-helper.md) | Add helper that composes guard context from runtime guard policy inputs |
| [Issue #2450 SleepUntilResume Guard Context Builder Regression Tests (2026-05-24)](plans/2026-05-24-issue-2450-sleepuntilresume-guard-context-builder-regression-tests.md) | Add deterministic tests for guard-context builder mapping and continuation boundary behavior |
| [Issue #2451 SleepUntilResume Guard Context Builder Refactor (2026-05-24)](plans/2026-05-24-issue-2451-sleepuntilresume-guard-context-builder-refactor.md) | Refactor SleepUntilResume to replace manual guard-context field assignments with helper usage |
| [Debugger SleepUntilResume Loop/Post Context Builder Audit (2026-05-24)](plans/2026-05-24-debugger-sleepuntilresume-loop-post-context-builder-audit.md) | Audit for extracting repeated loop/post context wiring from SleepUntilResume coordinator |
| [Issue #2452 SleepUntilResume Loop/Post Context Builder Helper (2026-05-24)](plans/2026-05-24-issue-2452-sleepuntilresume-loop-post-context-builder-helper.md) | Add helper that composes loop/post bundle context from runtime wait/suspend/break/notification inputs |
| [Issue #2453 SleepUntilResume Loop/Post Context Builder Regression Tests (2026-05-24)](plans/2026-05-24-issue-2453-sleepuntilresume-loop-post-context-builder-regression-tests.md) | Add deterministic tests for loop/post context builder mapping and idle composition behavior |
| [Issue #2454 SleepUntilResume Loop/Post Context Builder Refactor (2026-05-24)](plans/2026-05-24-issue-2454-sleepuntilresume-loop-post-context-builder-refactor.md) | Refactor SleepUntilResume loop/post context wiring to consume shared context builder helper |
| [Debugger SleepUntilResume Coordinator Entry Helper Audit (2026-05-24)](plans/2026-05-24-debugger-sleepuntilresume-coordinator-entry-helper-audit.md) | Audit for consolidating repeated SleepUntilResume entry phase context+outcome orchestration |
| [Issue #2461 SleepUntilResume Coordinator Policy Entry Helper (2026-05-24)](plans/2026-05-24-issue-2461-sleepuntilresume-coordinator-entry-helper.md) | Add helper model resolving phase context and phase outcome from coordinator entry policy inputs |
| [Issue #2462 SleepUntilResume Coordinator Entry Helper Regression Tests (2026-05-24)](plans/2026-05-24-issue-2462-sleepuntilresume-coordinator-entry-regression-tests.md) | Add deterministic continue/skip and emitted/non-emitted tests for coordinator entry helper |
| [Issue #2463 SleepUntilResume Coordinator Entry Refactor (2026-05-24)](plans/2026-05-24-issue-2463-sleepuntilresume-coordinator-entry-refactor.md) | Refactor SleepUntilResume entry path to consume coordinator entry helper in both passes |
| [Debugger SleepUntilResume Pre-Break Action Plan Audit (2026-05-25)](plans/2026-05-25-debugger-sleepuntilresume-prebreak-action-plan-audit.md) | Audit pre-break action choreography and define helper-driven coordinator-thinning follow-up slices |
| [Issue #2473 SleepUntilResume Pre-Break Action Plan Helper (2026-05-25)](plans/2026-05-25-issue-2473-sleepuntilresume-prebreak-action-plan-helper.md) | Add typed helper context/outcome and builder for pre-break action plan composition |
| [Issue #2474 SleepUntilResume Pre-Break Action Plan Regression Tests (2026-05-25)](plans/2026-05-25-issue-2474-sleepuntilresume-prebreak-action-plan-regression-tests.md) | Add deterministic tests for emitted/non-emitted pre-break action plan behavior |
| [Issue #2475 SleepUntilResume Pre-Break Action Plan Refactor (2026-05-25)](plans/2026-05-25-issue-2475-sleepuntilresume-prebreak-action-plan-refactor.md) | Refactor SleepUntilResume pre-break wiring to consume helper action-plan outcomes |
| [Debugger SleepUntilResume Runtime-Bundle Internal Context Builder Audit (2026-05-25)](plans/2026-05-25-debugger-sleepuntilresume-runtime-bundle-internal-context-builder-audit.md) | Audit inline runtime-bundle nested context wiring and define helper-builder extraction slices |
| [Issue #2476 SleepUntilResume Runtime-Bundle Dispatch Context Builder (2026-05-25)](plans/2026-05-25-issue-2476-sleepuntilresume-runtime-bundle-dispatch-context-builder.md) | Extract runtime-bundle dispatch context mapping into explicit helper builder |
| [Issue #2477 SleepUntilResume Runtime-Bundle Internal Context Builder Regression Tests (2026-05-25)](plans/2026-05-25-issue-2477-sleepuntilresume-runtime-bundle-internal-context-builder-regression-tests.md) | Add deterministic unit coverage for runtime-bundle internal context builders |
| [Issue #2478 SleepUntilResume Runtime-Bundle Internal Context Builder Refactor (2026-05-25)](plans/2026-05-25-issue-2478-sleepuntilresume-runtime-bundle-internal-context-builder-refactor.md) | Refactor runtime-bundle resolver to consume extracted internal context builders |
| [Debugger SleepUntilResume Loop-Post Bundle Internal Context Builder Audit (2026-05-25)](plans/2026-05-25-debugger-sleepuntilresume-loop-post-bundle-internal-context-builder-audit.md) | Audit loop-post bundle resolver inline wiring and define internal context-builder extraction slices |
| [Issue #2482 SleepUntilResume Loop-Post Bundle Internal Context Builders (2026-05-25)](plans/2026-05-25-issue-2482-sleepuntilresume-loop-post-bundle-internal-context-builders.md) | Extract internal loop/post context builders from loop-post bundle resolver wiring |
| [Issue #2480 SleepUntilResume Loop-Post Bundle Internal Context Builder Regression Tests (2026-05-25)](plans/2026-05-25-issue-2480-sleepuntilresume-loop-post-bundle-internal-context-builder-regression-tests.md) | Add deterministic field-mapping tests for loop/post internal context builders |
| [Issue #2481 SleepUntilResume Loop-Post Bundle Resolver Refactor (2026-05-25)](plans/2026-05-25-issue-2481-sleepuntilresume-loop-post-bundle-resolver-refactor.md) | Refactor loop-post bundle resolver to consume extracted internal context builders |
| [Debugger SleepUntilResume Pre-Loop Bundle Internal Context Builder Audit (2026-05-25)](plans/2026-05-25-debugger-sleepuntilresume-preloop-bundle-internal-context-builder-audit.md) | Audit pre-loop bundle resolver inline wiring and define internal context-builder extraction slices |
| [Issue #2486 SleepUntilResume Pre-Loop Bundle Internal Context Builders (2026-05-25)](plans/2026-05-25-issue-2486-sleepuntilresume-preloop-bundle-internal-context-builders.md) | Extract internal pre-break/pre-loop/dispatch context builders for pre-loop bundle resolver |
| [Issue #2484 SleepUntilResume Pre-Loop Bundle Internal Context Builder Regression Tests (2026-05-25)](plans/2026-05-25-issue-2484-sleepuntilresume-preloop-bundle-internal-context-builder-regression-tests.md) | Add deterministic field-mapping tests for pre-loop bundle internal context builders |
| [Issue #2485 SleepUntilResume Pre-Loop Bundle Resolver Refactor (2026-05-25)](plans/2026-05-25-issue-2485-sleepuntilresume-preloop-bundle-resolver-refactor.md) | Refactor pre-loop bundle resolver to consume extracted internal context builders |
| [Debugger SleepUntilResume PhaseOutcome Internal Context Builder Audit (2026-05-25)](plans/2026-05-25-debugger-sleepuntilresume-phaseoutcome-internal-context-builder-audit.md) | Audit phase-outcome resolver inline context mapping and define builder extraction slices |
| [Issue #2496 SleepUntilResume PhaseOutcome Internal Context Builders (2026-05-25)](plans/2026-05-25-issue-2496-sleepuntilresume-phaseoutcome-internal-context-builders.md) | Extract pre-loop bundle, loop, and post-loop context builders for phase-outcome resolver |
| [Issue #2497 SleepUntilResume PhaseOutcome Internal Context Builder Regression Tests (2026-05-25)](plans/2026-05-25-issue-2497-sleepuntilresume-phaseoutcome-internal-context-builder-regression-tests.md) | Add deterministic field-mapping tests for phase-outcome internal context builders |
| [Issue #2498 SleepUntilResume PhaseOutcome Resolver Refactor (2026-05-25)](plans/2026-05-25-issue-2498-sleepuntilresume-phaseoutcome-resolver-refactor.md) | Refactor phase-outcome resolver to consume extracted internal context builders |
| [Debugger SleepUntilResume Entry and Pre-Break Execution Simplification Audit (2026-05-25)](plans/2026-05-25-debugger-sleepuntilresume-entry-and-prebreak-execution-simplification-audit.md) | Audit coordinator entry and pre-break execution branching for helper-policy extraction |
| [Issue #2505 SleepUntilResume Coordinator Entry Context Builder (2026-05-25)](plans/2026-05-25-issue-2505-sleepuntilresume-coordinator-entry-context-builder.md) | Extract deterministic coordinator-entry context builder and remove manual wiring |
| [Issue #2506 SleepUntilResume Coordinator Entry Context Builder Regression Tests (2026-05-25)](plans/2026-05-25-issue-2506-sleepuntilresume-coordinator-entry-context-builder-regression-tests.md) | Add deterministic regression tests for coordinator-entry context builder mapping |
| [Issue #2507 SleepUntilResume Pre-Break Execution Helper Refactor (2026-05-25)](plans/2026-05-25-issue-2507-sleepuntilresume-prebreak-execution-helper-refactor.md) | Refactor pre-break execution branching into helper execution outcome |
| [Debugger SleepUntilResume Loop Runtime Snapshot Simplification Audit (2026-05-25)](plans/2026-05-25-debugger-sleepuntilresume-loop-runtime-snapshot-simplification-audit.md) | Audit wait-loop runtime state mapping and define snapshot helper extraction slices |
| [Issue #2508 SleepUntilResume Runtime Loop-State Snapshot Builders (2026-05-25)](plans/2026-05-25-issue-2508-sleepuntilresume-runtime-loop-state-snapshot-builders.md) | Extract deterministic runtime snapshot builders for loop and post-loop contexts |
| [Issue #2509 SleepUntilResume Runtime Loop-State Snapshot Builder Regression Tests (2026-05-25)](plans/2026-05-25-issue-2509-sleepuntilresume-runtime-loop-state-snapshot-builder-regression-tests.md) | Add deterministic regression tests for runtime loop-state snapshot mappings |
| [Issue #2510 SleepUntilResume Wait-Loop Helper-Driven Iteration Policy Refactor (2026-05-25)](plans/2026-05-25-issue-2510-sleepuntilresume-wait-loop-helper-driven-iteration-policy-refactor.md) | Refactor wait-loop and post-loop context wiring to helper-driven snapshot builders |
| [Debugger SleepUntilResume Runtime Side-Effect Application Simplification Audit (2026-05-25)](plans/2026-05-25-debugger-sleepuntilresume-runtime-sideeffect-application-simplification-audit.md) | Audit coordinator runtime side-effect state transitions for helper-policy extraction |
| [Issue #2517 SleepUntilResume Runtime Side-Effect Application Context Builder (2026-05-25)](plans/2026-05-25-issue-2517-sleepuntilresume-runtime-sideeffect-application-context-builder.md) | Extract deterministic runtime side-effect application context builder |
| [Issue #2518 SleepUntilResume Runtime Side-Effect Application Mapping Regression Tests (2026-05-25)](plans/2026-05-25-issue-2518-sleepuntilresume-runtime-sideeffect-application-mapping-regression-tests.md) | Add deterministic regression tests for runtime side-effect application mapping |
| [Issue #2519 SleepUntilResume Runtime Side-Effect State Update Refactor (2026-05-25)](plans/2026-05-25-issue-2519-sleepuntilresume-runtime-sideeffect-state-update-refactor.md) | Refactor coordinator runtime side-effect state updates to helper outcomes |
| [Debugger SleepUntilResume Runtime Dispatch Execution Policy Simplification Audit (2026-05-25)](plans/2026-05-25-debugger-sleepuntilresume-runtime-dispatch-execution-policy-simplification-audit.md) | Audit runtime dispatch callsite payload wiring for execution-policy helper extraction |
| [Issue #2520 SleepUntilResume Runtime Dispatch Execution Policy Context Builder (2026-05-25)](plans/2026-05-25-issue-2520-sleepuntilresume-runtime-dispatch-execution-policy-context-builder.md) | Extract deterministic runtime dispatch execution-policy context builder |
| [Issue #2521 SleepUntilResume Runtime Dispatch Execution Policy Mapping Regression Tests (2026-05-25)](plans/2026-05-25-issue-2521-sleepuntilresume-runtime-dispatch-execution-policy-mapping-regression-tests.md) | Add deterministic regression tests for runtime dispatch execution-policy mapping |
| [Issue #2522 SleepUntilResume Runtime Dispatch Callsite Helper Outcome Refactor (2026-05-25)](plans/2026-05-25-issue-2522-sleepuntilresume-runtime-dispatch-callsite-helper-outcome-refactor.md) | Refactor runtime dispatch callsite to consume helper execution outcomes |
| [Epic 30 DebugUtilities Mapping Consolidation Audit (2026-05-25)](plans/2026-05-25-epic-30-debugutilities-mapping-consolidation-audit.md) | Epic-level audit for non-SleepUntilResume debugger mapping consolidation and redundancy reduction |
| [Epic 22 Startup UX and Accessibility Theme Refresh Plan (2026-05-22)](plans/2026-05-22-epic-22-startup-ux-accessibility-theme-refresh.md) | Issue-linked implementation plan for splash visibility, larger startup defaults, and touch-friendly UI sizing (#2280/#2281) |
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
| [Release Notes v1.4.39](release-notes/release-notes-v1.4.39.md) | Testing-discouraged release notes and diagnostics-focused stabilization summary for v1.4.39 |
| [Genesis Base Console Progress Dashboard (2026-04-28)](plans/2026-04-28-genesis-base-console-progress-dashboard.md) | Living subsystem progress tracker for base Genesis/Mega Drive completion with percentages, done/remaining scope, and week-target priorities |
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
