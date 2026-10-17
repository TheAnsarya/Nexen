# TAS UI Comprehensive Plan

> **Epic:** [Epic 14] TAS Editor UI — Comprehensive Functionality
> **Status:** Active
> **Created:** 2026-02-17
> **Author:** AI Copilot

## Overview

Make the Nexen TAS Editor UI awesome, fast, and comprehensively functional. This plan covers all identified gaps, improvements, and new features based on a complete audit of the TAS UI codebase.

## Current State

### Architecture

- **TasEditorViewModel.cs** — ~1700 LOC, 40+ reactive properties, full MVVM
- **TasEditorWindow.axaml/.cs** — Menu bar, toolbar, frame list, properties panel, piano roll, status bar
- **PianoRollControl.cs** — Custom Avalonia control with cached rendering, zoom, painting
- **TasInputDialog** — Numeric/text input dialogs
- **GreenzoneSettingsDialog** — Greenzone configuration
- **BranchManagementDialog** — Branch CRUD operations
- **PlaybackInterruptDialog** — Joypad interrupt handling

### Test Coverage

| Test File | Tests |
|-----------|-------|
| BulkOperationTests.cs | 10 |
| ClipboardAndFrameOperationTests.cs | 34 |
| GreenzoneManagerOptimizationTests.cs | 39 |
| GreenzoneManagerTests.cs | 11 |
| InputRecorderTests.cs | 16 |
| PianoRollCachingTests.cs | 5 |
| TasEditorApiDocAndGuardTests.cs | 17 |
| TasEditorViewModelBranchAndLayoutTests.cs | 41 |
| TasEditorViewModelIntegrationTests.cs | 67 |
| TasEditorViewModelTests.cs | 29 |
| TasLuaApiComprehensiveTests.cs | 75 |
| TasLuaApiTests.cs | 8 |
| UndoableActionTests.cs | 38 |
| **Total** | **390** |

### Existing Benchmarks

- `TasOptimizationBenchmarks.cs` — TAS optimization paths
- `TasHotPathBenchmarks.cs` — Hot path perf
- `GreenzoneManagerBenchmarks.cs` — Greenzone operations
- `ControllerInputBenchmarks.cs` — Controller input handling

### Well-Implemented Areas

- Incremental frame updates (O(1) vs O(n))
- Comprehensive undo/redo with smart dispatch
- Piano roll rendering optimization (caching, viewport culling)
- 14 keyboard shortcuts, well-distributed
- Dialog-based workflows (clean separation)
- Task-based async I/O (non-blocking)
- Lua scripting API with templates

## Gap Analysis

### Critical Missing Features

| # | Feature | Location | Severity |
|---|---------|----------|----------|
| 1 | Emulation playback integration | ViewModel L1126 — stub only | 🔴 Critical |
| 2 | Multi-frame selection | ListBox SelectionMode="Single" | 🔴 Critical |
| 3 | Frame range bulk operations UI | No visible UI for batch ops | 🔴 Critical |
| 4 | Frame search/filter | No search panel exists | 🟡 Important |
| 5 | Greenzone state visualization | No per-frame state indicator | 🟡 Important |
| 6 | Auto-save functionality | Not implemented | 🟡 Important |
| 7 | Piano roll right-click context menu | Not implemented | 🟡 Important |
| 8 | Marker/comment list panel | Can set markers but no list UI | 🟡 Important |
| 9 | Input preview pane (hex dump) | Not implemented | 🟢 Nice-to-have |
| 10 | Animation preview viewport | Not implemented | 🟢 Nice-to-have |

### Quality & Polish

| # | Issue | Location | Priority |
|---|-------|----------|----------|
| 1 | MarkerClicked event reserved but unused | PianoRoll L128-130 | Medium |
| 2 | Playback UI throttle (every 10 frames) | TasEditorWindow L~200 | Medium |
| 3 | No keyboard frame selection in piano roll | PianoRoll keyboard handler | Medium |
| 4 | Script template hardcoded addresses | ViewModel L1603 | Low |
| 5 | No lag frame indicator legend | TasEditorWindow.axaml | Low |
| 6 | No movie metadata display (Author, System) | Properties panel | Low |
| 7 | FormattedText cache clears entirely on zoom | PianoRoll rendering | Low |

## Implementation Plan

### Phase 1 — Core Functionality (Critical)

#### 1.1 Multi-Frame Selection

**Goal:** Enable selecting multiple frames in the frame list and piano roll.

- Change ListBox `SelectionMode` from `"Single"` to `"Extended"`
- Add `SelectedFrames` collection property to ViewModel
- Wire selection changed handler to update `SelectedFrames`
- Update all frame operations (delete, insert, clear, cut, copy) to work on ranges
- Piano roll: support Shift+Click for range, Ctrl+Click for toggle

#### 1.2 Frame Range Bulk Operations

**Goal:** UI for batch operations on frame ranges.

- Add "Select Range" dialog (from-frame, to-frame)
- Add toolbar buttons: "Select All" (Ctrl+A), "Select None"
- Bulk delete, bulk insert (N frames at position), bulk clear input
- Bulk set/clear specific buttons across range
- Pattern fill (e.g., repeat input every N frames)

#### 1.3 Piano Roll Context Menu

**Goal:** Right-click context menu for common operations.

- Insert Frame(s) Above/Below
- Delete Selected Frame(s)
- Cut / Copy / Paste
- Set Comment / Marker
- Clear Input
- Select Range To Here
- Go To Frame...

### Phase 2 — Workflow Features (Important)

#### 2.1 Frame Search & Filter

**Goal:** Find frames by input content, comments, or properties.

- Search panel (collapsible, above or below frame list)
- Search by: button name pressed, comment text, frame number range
- Filter modes: highlight matches, show only matches
- Navigate between matches (F3 / Shift+F3)

#### 2.2 Greenzone State Visualization

**Goal:** Show which frames have savestate data.

- Add green dots/bars in frame list gutter for frames with states
- Piano roll: greenzone overlay shows exact coverage
- Status bar: "GZ: 45/120 frames" indicator
- Click greenzone indicator to seek to state

#### 2.3 Marker/Comment List Panel

**Goal:** Centralized view of all frame comments and markers.

- Dockable side panel or tab
- List: Frame# | Comment | Type (comment/marker/TODO)
- Double-click to navigate to frame
- Add/Edit/Delete from panel
- Filter by type
- Export markers to text file

#### 2.4 Auto-Save

**Goal:** Prevent loss of work during long editing sessions.

- Configurable interval (default: 5 minutes)
- Auto-save to `{filename}.autosave.nm2`
- Recovery prompt on next open
- Toggle in Settings: enable/disable, interval
- Don't auto-save during playback (performance)

### Phase 3 — Polish & UX

#### 3.1 Enhanced Piano Roll Keyboard Navigation

- Arrow keys: move selection
- Shift+Arrow: extend selection
- Tab: next button lane
- Enter: toggle button at cursor
- Ctrl+G: Go to frame

#### 3.2 Input Preview Pane

- Show selected frame's raw input as hex bytes
- Show button states as visual diagram
- Show controller type-specific layout

#### 3.3 Movie Metadata Display

- Show Author, System, ROM name in properties panel
- Edit metadata dialog
- Show format version and compatibility

#### 3.4 Lag Frame Legend

- Color legend in status bar or toolbar
- Green = greenzone, Red = lag, Blue = pressed
- Tooltip on hover explaining each color

### Phase 4 — Performance

#### 4.1 Piano Roll Large Movie Optimization

- Virtual scrolling for 10k+ frame movies
- Incremental zoom cache updates (don't clear all)
- Off-thread FormattedText preparation
- Benchmark: target < 16ms render at 60fps for 100k frame movies

#### 4.2 Playback UI Throttle Optimization

- Current: update UI every 10 frames
- Improvement: adaptive throttling based on frame rate
- At 60fps: update every frame; at 240fps: update every 4 frames
- Ensure no frame skipping in visual feedback

## Benchmark Plan

### New Benchmarks to Add

| Benchmark | Measures | Target |
|-----------|----------|--------|
| PianoRollRenderBenchmark | Full render cycle at various zoom levels | < 8ms |
| MultiFrameSelectionBenchmark | Select 1k frames, operate on range | < 5ms |
| FrameSearchBenchmark | Search 100k frames by button state | < 50ms |
| LargeMovieScrollBenchmark | Scroll through 100k frame movie | < 2ms/scroll |
| AutoSaveBenchmark | Serialize 50k frame movie | < 200ms |
| UndoRedoBenchmark | 1000 undo/redo operations | < 1ms each |
| GreenzoneSeekBenchmark | Seek to frame with greenzone | < 10ms |
| ClipboardBenchmark | Copy/paste 1000 frames | < 20ms |

### Benchmark Methodology

- BenchmarkDotNet with `[MemoryDiagnoser]`
- 3+ warmup iterations, 5+ measurement iterations
- Test on 1k, 10k, 100k frame movies
- Track allocations (target zero allocs on hot paths)

## Test Plan

### New Test Areas

| Area | Est. Tests | Priority |
|------|-----------|----------|
| Multi-frame selection | 15 | High |
| Frame range bulk operations | 20 | High |
| Context menu actions | 12 | High |
| Frame search/filter | 15 | Medium |
| Greenzone visualization | 10 | Medium |
| Marker list panel | 10 | Medium |
| Auto-save/recovery | 12 | Medium |
| Piano roll keyboard nav | 8 | Medium |
| Movie metadata editing | 6 | Low |
| Large movie perf (integration) | 5 | Low |
| **Total New** | **~113** | — |

### Test Conventions

- All tests in `Tests/TAS/` directory
- File naming: `{Feature}Tests.cs`
- Use `TasTestHelper` for common setup
- Async dialog tests guard `_window is null`
- No windowing platform dependency in unit tests

## Success Criteria

1. All existing 390 TAS tests continue passing
2. 100+ new tests added and passing
3. All benchmarks meet target performance thresholds
4. Multi-frame selection works in both frame list and piano roll
5. Frame search can find any input pattern in < 100ms for 100k frames
6. Auto-save recovers cleanly after simulated crash
7. Context menu provides all common operations
8. Piano roll renders at 60fps with 100k frame movies at any zoom level

## Issue Decomposition

See GitHub Epic #656 and sub-issues #657-#670 for tracked work items.
