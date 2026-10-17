# DataBox Usage Inventory

**Issue:** #180 | **Sub-issue:** #182
**Created:** 2026-02-09

## Overview

The vendored DataBox control is used in **13 instances** across **12 AXAML files**, with **28 C# files** referencing DataBox types. This document catalogs every usage to inform migration planning.

## DataBox Source Files (19 files)

Located in `UI/ThirdParty/DataBox/`:

| File | Description | Lines |
|------|-------------|-------|
| DataBox.cs | Main control (extends ListBox) | ~499 |
| DataBoxColumn.cs | Abstract base column | — |
| DataBoxBoundColumn.cs | Abstract bound column (adds Binding, SortMemberPath) | — |
| DataBoxTextColumn.cs | Text column with TextBlock | — |
| DataBoxCheckBoxColumn.cs | CheckBox column | — |
| DataBoxTemplateColumn.cs | Template column (empty subclass) | — |
| DataBoxCell.cs | Cell control | — |
| DataBoxRow.cs | Row control (extends ListBoxItem) | — |
| DataBoxColumnHeader.cs | Column header with sorting/resizing | — |
| DataBoxGridLinesVisibility.cs | Flags enum: None/Horizontal/Vertical/All | — |
| DataBoxVirtualizingStackPanel.cs | VirtualizingStackPanel subclass | — |
| DataBoxRowsPresenter.cs | Rows ListBox presenter | — |
| DataBoxColumnHeadersPresenter.cs | Column headers Panel | — |
| DataBoxCellsPresenter.cs | Cells Panel per row | — |
| DataBoxCellMeasure.cs | Static cell measure/arrange | — |
| DataBoxRowMeasure.cs | Static row measure/arrange | — |
| Themes/Fluent.axaml | Fluent theme styles | — |

## Custom Extensions Beyond Upstream DataBox

| # | Feature | Location | Used By |
|---|---------|----------|---------|
| 1 | `SortState` multi-column sort | DataBox.cs | 9 of 13 instances |
| 2 | `CopyToClipboard()` CSV export | DataBox.cs | ProfilerWindow |
| 3 | `CellClick` event | DataBox.cs | BreakpointListView, AssemblerWindow, CheatListWindow |
| 4 | `CellDoubleClick` event | DataBox.cs | 8 instances |
| 5 | `ColumnWidthId` persistence | DataBoxColumn.cs | 12 of 13 instances |
| 6 | Type-ahead keyboard search (hex prefix `$`) | DataBox.cs | Most instances |
| 7 | `DisableTypeAhead` property | DataBox.cs | WatchListView |
| 8 | `ColumnHeaderToolTip` | DataBoxColumn.cs | Various |
| 9 | `IsChecked` binding on TextColumn | DataBoxTextColumn.cs | BreakpointListView |
| 10 | `IsVisible` binding on columns | DataBoxColumn.cs | Various |
| 11 | Custom row/cell styling (foreground, background, fontstyle) | Themes | 6 views |
| 12 | `GetValue()` binding evaluation helper | DataBox.cs | Search/export |

## All DataBox Instances by View

### Tier 1: Simple (migration candidates first)

#### 1. RegisterTabView

- **File:** `UI/Debugger/Views/RegisterTabView.axaml`
- **Columns:** 4 (Address, Name, Value, ValueHex) — all `DataBoxTextColumn`
- **Features:** Items, Selection, GridLines, ColumnWidths
- **Events:** None
- **Sort:** No
- **Complexity:** ⭐ (lowest)

#### 2. AssemblerWindow

- **File:** `UI/Debugger/Windows/AssemblerWindow.axaml`
- **Columns:** 2 (Line, Error) — all `DataBoxTextColumn`
- **Features:** Items, Selection, GridLines
- **Events:** CellClick
- **Sort:** No
- **Complexity:** ⭐

#### 3. FindResultListView

- **File:** `UI/Debugger/Views/FindResultListView.axaml`
- **Columns:** 2 (Address, Result) — all `DataBoxTextColumn`
- **Features:** Items, Selection, GridLines, ColumnWidths, SortState
- **Events:** CellDoubleClick
- **Sort:** Multiple
- **Complexity:** ⭐⭐

#### 4. CallStackView

- **File:** `UI/Debugger/Views/CallStackView.axaml`
- **Columns:** 3 (Function, PcAddress, RomAddress) — all `DataBoxTextColumn`
- **Features:** Items, GridLines, ColumnWidths
- **Events:** CellDoubleClick
- **Sort:** No
- **Custom styling:** DataBoxCell foreground/fontstyle bound to ViewModel
- **Complexity:** ⭐⭐

#### 5. CheatListWindow

- **File:** `UI/Windows/CheatListWindow.axaml`
- **Columns:** 3 (Enabled as CheckBox, Description, Codes) — mixed column types
- **Features:** Items, Selection, GridLines, ColumnWidths, SortState
- **Events:** CellClick, CellDoubleClick
- **Sort:** Multiple
- **Complexity:** ⭐⭐

### Tier 2: Moderate

#### 6. BreakpointListView

- **File:** `UI/Debugger/Views/BreakpointListView.axaml`
- **Columns:** 5 (Enabled CheckBox, Marked CheckBox, Type, Address, Condition) — mixed types
- **Features:** Items, Selection, GridLines, ColumnWidths, SortState
- **Events:** CellClick, CellDoubleClick
- **Sort:** Multiple
- **Complexity:** ⭐⭐⭐

#### 7. LabelListView

- **File:** `UI/Debugger/Views/LabelListView.axaml`
- **Columns:** 4 (Label, RelAddr, AbsAddr, Comment as Template) — mixed types
- **Features:** Items, Selection, GridLines, ColumnWidths, SortState
- **Events:** CellDoubleClick
- **Sort:** Multiple
- **Custom styling:** DataBoxRow foreground/fontstyle, monospace tooltip
- **Complexity:** ⭐⭐⭐

#### 8. FunctionListView

- **File:** `UI/Debugger/Views/FunctionListView.axaml`
- **Columns:** 3 (Function, RelAddr, AbsAddr) — all `DataBoxTextColumn`
- **Features:** Items, Selection, GridLines, ColumnWidths, SortState
- **Events:** CellDoubleClick
- **Sort:** Multiple
- **Custom styling:** DataBoxRow foreground/fontstyle
- **Complexity:** ⭐⭐⭐

### Tier 3: Complex

#### 9. MemorySearchWindow

- **File:** `UI/Debugger/Windows/MemorySearchWindow.axaml`
- **Columns:** 10 (Address, Value, PrevValue, ReadCount, LastRead, WriteCount, LastWrite, ExecCount, LastExec, Match CheckBox)
- **Features:** Items, Selection, GridLines, ColumnWidths, SortState
- **Sort:** Multiple
- **Complexity:** ⭐⭐⭐

#### 10. ProfilerWindow

- **File:** `UI/Debugger/Windows/ProfilerWindow.axaml`
- **Columns:** 9 (FunctionName, CallCount, InclusiveTime, InclusiveTimePercent, ExclusiveTime, ExclusiveTimePercent, AvgCycles, MinCycles, MaxCycles)
- **Features:** Items, Selection, GridLines, ColumnWidths, SortState, CopyToClipboard
- **Events:** CellDoubleClick
- **Sort:** Multiple
- **Complexity:** ⭐⭐⭐

#### 11. EventViewerWindow

- **File:** `UI/Debugger/Windows/EventViewerWindow.axaml`
- **Columns:** 8 (Color as Template with colored border, Scanline, Cycle, ProgramCounter, Type, Address, Value, Details as Template)
- **Features:** Items, Selection, GridLines, ColumnWidths, SortState
- **Sort:** Multiple
- **Custom styling:** DataBoxRow TextBlock margin
- **Complexity:** ⭐⭐⭐⭐

### Tier 4: Most Complex

#### 12. SpriteViewerWindow

- **File:** `UI/Debugger/Windows/SpriteViewerWindow.axaml`
- **Columns:** 9 (SpriteIndex, X hex+decimal template, Y hex+decimal template, Size WxH template, TileIndex hex template, Palette, Priority enum, Flags, Visible inverted binding)
- **Features:** Items, Selection, GridLines, ColumnWidths, SortState, CanUserResizeColumns=False
- **Sort:** Multiple
- **Custom styling:** DataBoxRow selected TextBlock styles
- **Complexity:** ⭐⭐⭐⭐

#### 13. WatchListView

- **File:** `UI/Debugger/Views/WatchListView.axaml`
- **Columns:** 2 (Name editable TextBox, Value with .changed class) — both `DataBoxTemplateColumn`
- **Features:** Items, Selection, GridLines, ColumnWidths, DisableTypeAhead=True
- **Custom styling:** Extensive TextBox styling, error colors, editable cells with key/focus handlers
- **Complexity:** ⭐⭐⭐⭐⭐ (highest)

## C# Code-Behind References (28 files)

| Category | Files | Key Usage |
|----------|-------|-----------|
| Direct DataBox interaction | CheatListWindow, BreakpointListView, WatchListView, etc. | CellClick/CellDoubleClick handlers, Selection |
| Visual tree search | BaseConfigWindow | Finds DataBox via GetVisualDescendants() |
| Selection management | Various ViewModels | Creates/reads SelectionModel |
| About dialog | AboutWindow | Credits DataBox as MIT dependency |
| Enum references | ActionType.cs | DataBox action type value |

## Migration Complexity Matrix

| Feature | Avalonia DataGrid Native? | Extension Needed? | Effort |
|---------|:------------------------:|:-----------------:|--------|
| Text columns | ✅ Yes | No | Trivial |
| CheckBox columns | ✅ Yes | No | Trivial |
| Template columns | ✅ Yes | No | Low |
| Column resize | ✅ Yes | No | Trivial |
| Single selection | ✅ Yes | No | Trivial |
| Multi selection | ✅ Yes | No | Trivial |
| Sorting (single) | ✅ Yes | No | Low |
| Grid lines | ✅ Yes | No | Low |
| **Multi-column sort** | ❌ No | **Yes** | Medium |
| **CellClick event** | ❌ No | **Yes** | Medium |
| **CellDoubleClick event** | ❌ No | **Yes** | Medium |
| **ColumnWidthId persistence** | ❌ No | **Yes** | Medium |
| **CopyToClipboard CSV** | ❌ No | **Yes** | Low |
| **Type-ahead search** | ❌ No | **Yes** | High |
| **Hex prefix search** | ❌ No | **Yes** | High |
| **DisableTypeAhead** | ❌ No | **Yes** | Low |
| **Custom row styling** | ⚠️ Partial | **Yes** | Medium |
| **Editable cells (Watch)** | ⚠️ Partial | **Yes** | High |

## Recommended Migration Order

1. RegisterTabView (⭐)
2. AssemblerWindow (⭐)
3. FindResultListView (⭐⭐)
4. CallStackView (⭐⭐)
5. CheatListWindow (⭐⭐)
6. FunctionListView (⭐⭐⭐)
7. LabelListView (⭐⭐⭐)
8. BreakpointListView (⭐⭐⭐)
9. MemorySearchWindow (⭐⭐⭐)
10. ProfilerWindow (⭐⭐⭐)
11. EventViewerWindow (⭐⭐⭐⭐)
12. SpriteViewerWindow (⭐⭐⭐⭐)
13. WatchListView (⭐⭐⭐⭐⭐) — last, most complex
