# Avalonia 11 DataGrid Capabilities Research

**Issue:** #180 | **Related:** #182 (DataBox Usage Inventory)
**Created:** 2026-02-09

## Summary

The Avalonia DataGrid is a **separate NuGet package** (`Avalonia.Controls.DataGrid`) hosted in its own repository ([AvaloniaUI/Avalonia.Controls.DataGrid](https://github.com/AvaloniaUI/Avalonia.Controls.DataGrid)). It is a full-featured data grid control ported from Silverlight/WPF heritage, supporting columns, sorting, selection, editing, clipboard, virtualization, and extensive customization.

### Setup Requirements

```xml
<!-- NuGet package -->
<PackageReference Include="Avalonia.Controls.DataGrid" Version="$(AvaloniaVersion)" />

<!-- Style include in App.axaml -->
<StyleInclude Source="avares://Avalonia.Controls.DataGrid/Themes/Fluent.xaml"/>
```

---

## Feature Comparison Table

| # | Feature | DataGrid Support | Notes | Extension Needed? |
|---|---------|:---:|-------|:-:|
| 1 | **Column types** | ✅ Full | `DataGridTextColumn`, `DataGridCheckBoxColumn`, `DataGridTemplateColumn` — direct equivalents of DataBox columns | No |
| 2 | **Sorting** | ✅ Full | Single-column built-in; multi-sort via `DataGridCollectionView.SortDescriptions` + `Sorting` event | Partial — multi-sort needs event handler |
| 3 | **Selection modes** | ✅ Full | `DataGridSelectionMode.Single` / `.Extended`; `SelectedItem`, `SelectedItems`, `SelectedIndex` | No |
| 4 | **Column resize / width** | ✅ Full | `CanUserResizeColumns`, `CanUserResize` per column, `Width`/`MinWidth`/`MaxWidth`, star sizing (`*`, `2*`) | Width **persistence** needs extension |
| 5 | **Grid lines** | ✅ Full | `GridLinesVisibility` enum: `None`, `Horizontal`, `Vertical`, `All`; brushes: `HorizontalGridLinesBrush`, `VerticalGridLinesBrush` | No |
| 6 | **Virtualization** | ✅ Built-in | Custom `DataGridRowsPresenter` with row recycling (not `VirtualizingStackPanel`); `LoadingRow`/`UnloadingRow` events for row pooling | No |
| 7 | **Cell click / double-click** | ✅ Partial | `CellPointerPressed` event provides Cell, Row, Column, PointerPressedEventArgs; double-click detectable via `e.ClickCount` | Double-click convenience wrapper may help |
| 8 | **Clipboard support** | ✅ Full | `ClipboardCopyMode`: `None`, `ExcludeHeader` (default), `IncludeHeader`; `CopyingRowClipboardContent` event for customization; `ClipboardContentBinding` per column | No |
| 9 | **Row styling / alternating** | ✅ Full | `RowTheme`/`CellTheme` on DataGrid; `LoadingRow` event for per-row styling; pseudo-classes `:selected`, `:editing`, `:invalid` on `DataGridRow`; alternating rows via `AlternatingRowBackground` | No |
| 10 | **Template columns** | ✅ Full | `DataGridTemplateColumn` with `CellTemplate` and `CellEditingTemplate`; supports `SortMemberPath` for sorting; `[InheritDataTypeFromItems]` for compiled bindings | No |
| 11 | **Keyboard navigation** | ✅ Full | Arrow keys, Tab, Enter (commit + move down), Escape (cancel edit), F2 (begin edit), Home/End, PageUp/PageDown, Ctrl+A (select all), Ctrl+C (copy) | Type-ahead search needs extension |
| 12 | **Extensibility** | ✅ Good | Subclass `DataGridColumn`/`DataGridBoundColumn`; `AutoGeneratingColumn` event; `LoadingRow`/`UnloadingRow`; `BeginningEdit`/`CellEditEnding`/`PreparingCellForEdit` | No |
| 13 | **Column visibility / binding** | ✅ Full | `IsVisible` property on `DataGridColumn` (uses `Control.IsVisibleProperty.AddOwner<DataGridColumn>()`) — bindable | No |

---

## Detailed Feature Analysis

### 1. Column Types

The DataGrid provides three column types plus an abstract base:

| Column Class | Base Class | Display Element | Edit Element | Key Properties |
|---|---|---|---|---|
| `DataGridTextColumn` | `DataGridBoundColumn` | `TextBlock` | `TextBox` | `FontFamily`, `FontSize`, `FontStyle`, `FontWeight`, `FontStretch`, `Foreground` |
| `DataGridCheckBoxColumn` | `DataGridBoundColumn` | `CheckBox` (read-only) | `CheckBox` | `IsThreeState` (for `bool?`) |
| `DataGridTemplateColumn` | `DataGridColumn` | `CellTemplate` | `CellEditingTemplate` | `SortMemberPath`, no `Binding` property |
| `DataGridBoundColumn` (abstract) | `DataGridColumn` | — | — | `Binding`, `ClipboardContentBinding` (auto-falls-back to `Binding`) |

**Common column properties** (on `DataGridColumn` base):

- `Header` / `HeaderTemplate`
- `IsReadOnly` / `IsVisible` / `IsFrozen`
- `Width` / `MinWidth` / `MaxWidth` (supports `DataGridLength`: Auto, Pixel, Star)
- `CanUserSort` / `CanUserResize` / `CanUserReorder`
- `SortMemberPath` / `CustomSortComparer` (`IComparer`)
- `DisplayIndex` / `CellTheme` / `CellStyleClasses` / `Tag`
- `HeaderPointerPressed` / `HeaderPointerReleased` events

**Migration impact:** Direct 1:1 mapping from `DataBoxTextColumn` → `DataGridTextColumn`, etc.

### 2. Sorting

**Built-in capabilities:**

- `CanUserSortColumns` on DataGrid (default: `true`)
- `CanUserSort` per column (default: `true`)
- `SortMemberPath` — property path for sorting (required on `DataGridTemplateColumn`)
- `CustomSortComparer` — `IComparer` per column for custom sort logic
- `Sorting` event — fires when user clicks column header, can be canceled/customized

**Advanced sorting via `DataGridCollectionView`:**

```csharp
var view = new DataGridCollectionView(items);
view.SortDescriptions.Add(DataGridSortDescription.FromPath("Name"));
view.SortDescriptions.Add(DataGridSortDescription.FromComparer(new CustomComparer()));
dataGrid.ItemsSource = view;
```

**Multi-column sort:** Not automatic from UI clicks, but achievable by:

1. Handling the `Sorting` event
2. Manually managing `DataGridCollectionView.SortDescriptions`
3. This is exactly what the existing DataBox `SortState` system does

**Migration impact:** The multi-column sort helper (`SortState`) used in 9/13 DataBox instances would need to be adapted to work with `DataGridCollectionView.SortDescriptions` instead of the current DataBox sort API. The core concept is the same.

### 3. Selection Modes

| Property | Type | Notes |
|---|---|---|
| `SelectionMode` | `DataGridSelectionMode` | `Single` (default) or `Extended` (multi-select with Ctrl/Shift) |
| `SelectedItem` | `object` | Two-way bindable |
| `SelectedItems` | `IList` | Read-only collection, only usable in Extended mode |
| `SelectedIndex` | `int` | Two-way bindable |
| `SelectionChanged` | `RoutedEvent` | `SelectionChangedEventArgs` with AddedItems/RemovedItems |
| `CurrentCellChanged` | `EventHandler<EventArgs>` | Fires when current cell changes |

**Note:** The DataGrid uses `DataGridSelectionMode` (Single/Extended), not Avalonia's base `SelectionMode` flags enum. There is no `Toggle` or `AlwaysSelected` mode. Extended mode supports Ctrl+Click (toggle individual), Shift+Click (range), Ctrl+A (select all).

**Migration impact:** Straightforward. Current DataBox uses `ListBox` selection model. DataGrid's selection is similar but uses `SelectedItem`/`SelectedItems` directly instead of `SelectionModel<T>`.

### 4. Column Resize & Width

| Property | Scope | Default | Notes |
|---|---|---|---|
| `CanUserResizeColumns` | DataGrid | `true` (code) | Enables drag-resize on column headers |
| `CanUserResize` | Per column | `true` | Override per column |
| `CanUserReorderColumns` | DataGrid | `true` (code) | Enables drag-reorder |
| `CanUserReorder` | Per column | `true` | Override per column |
| `Width` | Per column | Auto | `DataGridLength`: pixel (`200`), auto (`Auto`), star (`*`, `2*`) |
| `MinWidth` | Per column | — | Minimum pixel width |
| `MaxWidth` | Per column | 65536 | Maximum pixel width |
| `ColumnWidth` | DataGrid | — | Default width for all columns |
| `FrozenColumnCount` | DataGrid | 0 | Number of non-scrollable left columns |

**Width persistence:** Not built-in. The current `ColumnWidthId` system would need to be reimplemented as an extension (attached behavior or helper) that saves/restores `DataGridColumn.Width` values.

**Migration impact:** Resize/reorder is native. Width persistence extension (used in 12/13 instances) must be ported.

### 5. Grid Lines

```csharp
public enum DataGridGridLinesVisibility
{
    None = 0,        // No gridlines
    Horizontal = 1,  // Horizontal only
    Vertical = 2,    // Vertical only
    All = 3,         // Both (Horizontal | Vertical)
}
```

**Additional properties:**

- `HorizontalGridLinesBrush` — brush for horizontal lines
- `VerticalGridLinesBrush` — brush for vertical lines

**Migration impact:** Direct enum replacement. `DataBoxGridLinesVisibility` → `DataGridGridLinesVisibility` (identical values).

### 6. Virtualization

The DataGrid uses its own **custom row-based virtualization** via `DataGridRowsPresenter` (not `VirtualizingStackPanel`). Key aspects:

- **Row recycling:** Rows are pooled and reused as the user scrolls
- `LoadingRow` event — fires when a row is prepared (from pool or new)
- `UnloadingRow` event — fires when a row returns to the pool
- Only visible rows (plus a small buffer) are materialized
- `ScrollIntoView(item, column)` — programmatic scroll to a specific cell

**Migration impact:** The existing `DataBoxVirtualizingStackPanel` is replaced by the DataGrid's built-in row recycling. This is likely **more** efficient since it's purpose-built for the grid layout.

### 7. Cell Click / Double-Click Events

**Built-in event:**

```csharp
// Fires on any pointer press on a cell
public event EventHandler<DataGridCellPointerPressedEventArgs> CellPointerPressed;
```

**`DataGridCellPointerPressedEventArgs` provides:**

- `Cell` — the `DataGridCell`
- `Row` — the `DataGridRow`
- `Column` — the `DataGridColumn`
- `PointerPressedEventArgs` — full pointer info including `ClickCount`

**Double-click detection:**

```csharp
dataGrid.CellPointerPressed += (s, e) =>
{
    if (e.PointerPressedEventArgs.ClickCount == 2)
    {
        // Handle double-click
        var item = e.Row.DataContext;
        var column = e.Column;
    }
};
```

**Migration impact:** The existing `CellClick` and `CellDoubleClick` events can be replaced with a single `CellPointerPressed` handler that checks `ClickCount`. Alternatively, convenience extension events can be created. The DataGrid also has `CurrentCellChanged` for tracking cell focus changes.

### 8. Clipboard Support

| Property/Event | Type | Notes |
|---|---|---|
| `ClipboardCopyMode` | `DataGridClipboardCopyMode` | `None`, `ExcludeHeader` (default), `IncludeHeader` |
| `CopyingRowClipboardContent` | Event | Customize clipboard content per row |
| `ClipboardContentBinding` | Per column | What value to copy (defaults to `Binding` on bound columns) |

**Built-in behavior:**

- Ctrl+C copies selected rows as tab-delimited text
- Each column's `ClipboardContentBinding` determines cell text
- `CopyingRowClipboardContent` event allows modifying/filtering clipboard content
- Headers optionally included based on `ClipboardCopyMode`

**Migration impact:** The existing `CopyToClipboard()` CSV export (used in ProfilerWindow) can leverage the built-in clipboard system, potentially with custom formatting in the `CopyingRowClipboardContent` event handler.

### 9. Row Styling / Alternating Rows

**Built-in mechanisms:**

| Mechanism | Description |
|---|---|
| `RowTheme` | `ControlTheme` applied to all `DataGridRow` instances |
| `CellTheme` (DataGrid) | `ControlTheme` applied to all `DataGridCell` instances |
| `CellTheme` (Column) | `ControlTheme` per column |
| `CellStyleClasses` (Column) | CSS-like class names per column |
| `LoadingRow` event | Set properties on each row as it's prepared |
| Pseudo-classes | `:selected`, `:editing`, `:invalid` on rows; `:selected`, `:current`, `:edited`, `:invalid`, `:focus` on cells |
| `AlternatingRowBackground` | Built-in alternating row color |

**Row details:**

- `RowDetailsTemplate` — expandable detail section per row
- `RowDetailsVisibilityMode` — `Collapsed`, `Visible`, `VisibleWhenSelected`

**Headers:**

- `HeadersVisibility` — `None`, `Column`, `Row`, `All`
- `RowHeaderWidth` / `ColumnHeaderHeight`

**Migration impact:** The current DataBox row styling (foreground/fontstyle bound to ViewModel in CallStackView, LabelListView, FunctionListView, SpriteViewerWindow) can be achieved via `LoadingRow` event or Avalonia styles targeting `DataGridRow`/`DataGridCell` pseudo-classes.

### 10. Template Columns

`DataGridTemplateColumn` provides full control over cell rendering:

```xml
<DataGridTemplateColumn Header="Age" SortMemberPath="AgeInYears">
    <DataGridTemplateColumn.CellTemplate>
        <DataTemplate DataType="model:Person">
            <TextBlock Text="{Binding AgeInYears, StringFormat='{}{0} years'}"
                       VerticalAlignment="Center" HorizontalAlignment="Center" />
        </DataTemplate>
    </DataGridTemplateColumn.CellTemplate>
    <DataGridTemplateColumn.CellEditingTemplate>
        <DataTemplate DataType="model:Person">
            <NumericUpDown Value="{Binding AgeInYears}" FormatString="N0"
                           Minimum="0" Maximum="120" HorizontalAlignment="Stretch"/>
        </DataTemplate>
    </DataGridTemplateColumn.CellEditingTemplate>
</DataGridTemplateColumn>
```

**Key points:**

- `CellTemplate` — display mode (required)
- `CellEditingTemplate` — edit mode (optional; column is read-only without it)
- `SortMemberPath` — must be explicitly set (no `Binding` property like bound columns)
- Both are `IDataTemplate`, supporting `DataType` for compiled bindings
- `[InheritDataTypeFromItems]` attribute available for compiled binding support

**Migration impact:** Existing `DataBoxTemplateColumn` usages (EventViewerWindow color column, SpriteViewerWindow hex templates, WatchListView editable cells, LabelListView comment column) map directly. The DataGrid's template column is more feature-rich with separate edit templates.

### 11. Keyboard Navigation

The DataGrid has comprehensive built-in keyboard handling:

| Key | Action |
|---|---|
| ↑ / ↓ | Move current cell up/down |
| ← / → | Move current cell left/right |
| Tab / Shift+Tab | Move to next/previous cell (wraps rows) |
| Enter | Commit edit and move down |
| Escape | Cancel cell edit, then cancel row edit |
| F2 | Begin editing current cell (caret at end) |
| Home / End | First/last column (or first/last row with Ctrl) |
| Page Up / Page Down | Scroll one page |
| Ctrl+A | Select all (Extended mode) |
| Ctrl+C | Copy to clipboard |
| Space | Toggle checkbox in CheckBox columns |

**Shift modifiers:** Extend selection range (in Extended mode)
**Ctrl modifiers:** Move without changing selection (in Extended mode)

**Type-ahead search:** **Not built-in.** The current DataBox type-ahead feature (including hex prefix `$` search) would need to be reimplemented as an extension.

**Migration impact:** Core navigation is excellent. Type-ahead search (including hex prefix) needs a custom extension — this is the highest-effort custom feature.

### 12. Extensibility

**Creating custom columns:** Subclass `DataGridColumn` or `DataGridBoundColumn`:

- Override `GenerateElement()` — create display element
- Override `GenerateEditingElement()` — create editing element
- Override `PrepareCellForEdit()` — setup on entering edit mode
- Override `CancelCellEdit()` — revert on cancel
- Override `CommitCellEdit()` — save on commit (only on `DataGridBoundColumn`)

**Events for customization:**

| Event | When | Use Case |
|---|---|---|
| `AutoGeneratingColumn` | Column auto-generated | Customize/cancel auto columns |
| `BeginningEdit` | Cell about to enter edit mode | Cancel or prepare editing |
| `PreparingCellForEdit` | Cell editing element is ready | Configure edit control |
| `CellEditEnding` | Cell about to leave edit mode | Validate, cancel commit |
| `CellEditEnded` | Cell has left edit mode | Post-edit cleanup |
| `RowEditEnding` / `RowEditEnded` | Row edit commit/cancel | Row-level validation |
| `LoadingRow` / `UnloadingRow` | Row prepared / returned to pool | Per-row customization |
| `ColumnDisplayIndexChanged` | Column reordered | Track reordering |
| `ColumnReordering` / `ColumnReordered` | Column drag events | Custom reorder behavior |
| `Sorting` | Column header clicked for sort | Custom sort logic |
| `CellPointerPressed` | Cell clicked | Handle cell interaction |
| `SelectionChanged` | Selection changes | React to selection |
| `CurrentCellChanged` | Current cell changes | Track cell focus |

**Migration impact:** The DataGrid has significantly more extensibility points than DataBox. All current custom behaviors can be achieved through events or subclassing.

### 13. Column Visibility & Binding

`IsVisible` on `DataGridColumn` is a full `StyledProperty`:

```csharp
public static readonly StyledProperty<bool> IsVisibleProperty =
    Control.IsVisibleProperty.AddOwner<DataGridColumn>();
```

This means it's **fully bindable** in XAML:

```xml
<DataGridTextColumn Header="Details" IsVisible="{Binding ShowDetails}" />
```

**Migration impact:** Direct replacement. Same property name and behavior as current DataBox.

---

## DataBox → DataGrid Migration Gap Analysis

### Features Requiring No Extension

| Feature | DataBox | DataGrid | Notes |
|---|---|---|---|
| Text columns | `DataBoxTextColumn` | `DataGridTextColumn` | 1:1 replacement |
| CheckBox columns | `DataBoxCheckBoxColumn` | `DataGridCheckBoxColumn` | 1:1 replacement |
| Template columns | `DataBoxTemplateColumn` | `DataGridTemplateColumn` | More powerful (edit template) |
| Column resize | Custom | `CanUserResizeColumns` | Native support |
| Column reorder | Not in DataBox | `CanUserReorderColumns` | New capability |
| Single selection | `ListBox` selection | `DataGridSelectionMode.Single` | Simpler API |
| Multi selection | `SelectionModel<T>` | `DataGridSelectionMode.Extended` | Different API |
| Grid lines | `DataBoxGridLinesVisibility` | `DataGridGridLinesVisibility` | Identical enum |
| Column visibility | `IsVisible` | `IsVisible` | Same property |
| Row details | Not in DataBox | `RowDetailsTemplate` | New capability |
| Clipboard copy | — | `ClipboardCopyMode` | Built-in Ctrl+C |
| Column headers | — | `HeadersVisibility`, `HeaderTemplate` | More options |
| Frozen columns | Not in DataBox | `FrozenColumnCount` | New capability |
| Cell editing | Not in DataBox | Full edit cycle with events | New capability |

### Features Requiring Extension / Custom Code

| Feature | Current DataBox Implementation | DataGrid Extension Strategy | Effort |
|---|---|---|---|
| **Multi-column sort (`SortState`)** | Custom `SortState` class manages sort stack | Adapt to `DataGridCollectionView.SortDescriptions` in `Sorting` event handler | Medium |
| **`CellDoubleClick` event** | Custom event on DataBox | Check `ClickCount == 2` in `CellPointerPressed` handler | Low |
| **`CellClick` event** | Custom event on DataBox | Use `CellPointerPressed` directly | Low |
| **Column width persistence (`ColumnWidthId`)** | Custom `ColumnWidthId` + save/restore | Attached behavior saving `DataGridColumn.Width` to settings keyed by ID | Medium |
| **`CopyToClipboard()` CSV export** | Custom method | Override `CopyingRowClipboardContent` or standalone helper | Low |
| **Type-ahead search** | Custom keyboard handler with hex prefix (`$`) | Attached behavior on DataGrid handling `KeyDown`, text matching against bound data | High |
| **`DisableTypeAhead` property** | Simple flag | Skip if no type-ahead extension installed | Trivial |
| **Custom row styling (foreground, fontstyle)** | DataBox theme styles | `LoadingRow` event or Avalonia styles with selectors | Medium |
| **`IsChecked` on TextColumn** | Custom property | Use `DataGridCheckBoxColumn` or template column instead | Low |
| **`GetValue()` binding evaluation** | Custom helper | Use `DataGridColumn.GetCellValue()` (internal) or DataContext inspection | Low |
| **Editable cells (WatchListView)** | Template column with TextBox + key handlers | `DataGridTemplateColumn.CellEditingTemplate` with custom TextBox | Medium-High |

### New Capabilities from DataGrid (Not in DataBox)

1. **Cell editing lifecycle** — Full BeginEdit/CommitEdit/CancelEdit with events
2. **Row details** — Expandable detail section per row
3. **Column reordering** — Drag to reorder columns
4. **Frozen columns** — Pin left columns from scrolling
5. **Auto-generate columns** — Automatic column generation from data properties
6. **Row grouping** — `DataGridRowGroupHeader` support
7. **Validation** — Cell/row level validation with `:invalid` pseudo-class
8. **Accessibility** — Full automation peers for screen readers

---

## Architecture Comparison

| Aspect | DataBox | Avalonia DataGrid |
|---|---|---|
| **Base class** | `ListBox` → `ItemsControl` | `TemplatedControl` (standalone) |
| **Row class** | `DataBoxRow` → `ListBoxItem` | `DataGridRow` → `TemplatedControl` |
| **Cell class** | `DataBoxCell` → `ContentControl` | `DataGridCell` → `ContentControl` |
| **Virtualization** | `DataBoxVirtualizingStackPanel` | `DataGridRowsPresenter` (custom) |
| **Selection** | `ListBox.SelectionModel<T>` | `SelectedItem`/`SelectedItems` direct |
| **Data source** | `Items` (IEnumerable) | `ItemsSource` (IEnumerable), `DataGridCollectionView` |
| **Sorting** | Custom multi-sort stack | `DataGridCollectionView.SortDescriptions` |
| **Package** | Vendored in `UI/ThirdParty/DataBox/` | NuGet: `Avalonia.Controls.DataGrid` |
| **Maintenance** | Manual, no upstream | Community maintained, official Avalonia |
| **Source size** | ~19 files | ~40+ files (much more comprehensive) |

---

## Recommendation

The Avalonia DataGrid covers **all core features** needed by Nexen's 13 DataBox instances. The main gaps are:

1. **Multi-column sort** — Achievable with `DataGridCollectionView` + `Sorting` event (medium effort)
2. **Column width persistence** — Needs attached behavior (medium effort)
3. **Type-ahead search with hex prefix** — Most complex custom feature (high effort)
4. **CellDoubleClick convenience** — Trivial wrapper around `CellPointerPressed`

The migration is **strongly recommended** because:

- The DataGrid is significantly more feature-rich (editing, validation, row details, accessibility)
- It's maintained by the Avalonia community (vs. vendored frozen code)
- Row virtualization is purpose-built and likely more efficient
- It eliminates ~19 vendored source files from the codebase
