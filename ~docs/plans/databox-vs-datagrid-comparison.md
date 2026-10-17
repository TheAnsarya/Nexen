# DataBox vs Avalonia DataGrid — Feature Gap Analysis

**Issue:** #183 | **Created:** 2026-02-09

## Executive Summary

Avalonia 11's built-in DataGrid covers **~80% of our DataBox usage** natively. The remaining 20% (multi-column sort, cell click events, column width persistence, type-ahead search) can be implemented as reusable attached behaviors. The migration is **feasible and worthwhile**.

## Feature Comparison

### Fully Supported (No Extension Needed)

| Feature | DataBox | Avalonia DataGrid | Migration Effort |
|---------|---------|-------------------|------------------|
| Text columns | `DataBoxTextColumn` | `DataGridTextColumn` | Rename only |
| CheckBox columns | `DataBoxCheckBoxColumn` | `DataGridCheckBoxColumn` | Rename only |
| Template columns | `DataBoxTemplateColumn` | `DataGridTemplateColumn` | Rename + use `CellTemplate`/`CellEditingTemplate` |
| Items binding | `Items="{Binding}"` | `ItemsSource="{Binding}"` | Rename only |
| Column resize | `CanUserResize="True"` | `CanUserResize="True"` | Same property |
| Grid lines | `GridLinesVisibility="All"` | `GridLinesVisibility="All"` | Same property, same enum |
| Column visibility | `IsVisible="{Binding}"` on column | `IsVisible="{Binding}"` on column | Same |
| Single selection | `SelectionMode="Single"` | `SelectionMode="Single"` | Same |
| Multiple selection | `SelectionMode="Multiple"` | `SelectionMode="Extended"` | Rename value |
| Column width | `InitialWidth="100"` | `Width="100"` | Rename property |
| Column header | `Header="Name"` | `Header="Name"` | Same |

### Needs Extension (Achievable)

| Feature | DataBox API | DataGrid Strategy | Effort |
|---------|-------------|-------------------|--------|
| **Multi-column sort** | `SortState` property + `SortCommand` | Handle `Sorting` event, manage `DataGridCollectionView.SortDescriptions` | Medium |
| **CellClick event** | `CellClick` routed event | Attached behavior on `CellPointerPressed` (ClickCount==1) | Low |
| **CellDoubleClick event** | `CellDoubleClick` routed event | Attached behavior on `CellPointerPressed` (ClickCount==2) | Low |
| **ColumnWidthId** | `ColumnWidthId` + `ColumnWidths` dict | Attached property on column + `ColumnWidthChanged` event handler | Medium |
| **CopyToClipboard()** | Custom CSV export method | Use built-in `ClipboardCopyMode` + `CopyingRowClipboardContent` | Low |
| **Type-ahead search** | Built-in with hex `$` prefix | Attached `KeyDown` behavior with search logic | High |
| **DisableTypeAhead** | `DisableTypeAhead="True"` | Don't attach the search behavior | Trivial |

### DataBox Custom Extensions → DataGrid Equivalents

#### 1. SortState (Multi-Column Sort)

**DataBox:** Custom `SortState` class with list of `(column, direction)` tuples. Manages pseudo-classes on headers.

**DataGrid strategy:**

- Handle the `Sorting` event on DataGrid
- Maintain our own `SortState` with multiple sort descriptions
- Apply via `DataGridCollectionView.SortDescriptions`
- DataGrid already manages header sort indicators via pseudo-classes

```csharp
// Attached behavior sketch
public static class MultiSortBehavior {
    public static readonly AttachedProperty<SortState> SortStateProperty = ...;
    
    // On DataGrid.Sorting event:
    // - If Shift+Click: add to existing sorts
    // - If Click: replace all sorts with this column
    // - Update SortState, apply to CollectionView
}
```

#### 2. CellClick / CellDoubleClick Events

**DataBox:** Custom routed events with `DataBoxCellClickEventArgs` (column index, row item).

**DataGrid strategy:**

- DataGrid has `CellPointerPressed` event (Avalonia 11.2+)
- Check `e.PointerPressedEventArgs.ClickCount` for single vs double
- Extract column from `e.Column` and row item from `e.Row.DataContext`

```csharp
// Attached behavior sketch
public static class CellClickBehavior {
    public static readonly AttachedProperty<ICommand> CellClickCommandProperty = ...;
    public static readonly AttachedProperty<ICommand> CellDoubleClickCommandProperty = ...;
    
    // On CellPointerPressed:
    // - ClickCount == 1 → invoke CellClickCommand
    // - ClickCount == 2 → invoke CellDoubleClickCommand
}
```

#### 3. ColumnWidthId Persistence

**DataBox:** `ColumnWidthId` string property on each column, `ColumnWidths` dictionary on DataBox.

**DataGrid strategy:**

- Attached property `ColumnWidthId` on `DataGridColumn`
- Attached property `ColumnWidths` on `DataGrid` (bound to ViewModel)
- On `DataGrid.LoadingRow` or init: restore widths from dictionary
- On column resize complete: save widths to dictionary

#### 4. Type-Ahead Search with Hex Prefix

**DataBox:** Built-in keyboard search. Typing characters searches visible column text. `$` prefix triggers hex search.

**DataGrid strategy:**

- Attached `KeyDown` behavior on DataGrid
- Buffer keystrokes with timeout (500ms)
- Search first visible text column for matching rows
- `$` prefix → parse hex value for numeric column search

#### 5. Custom Row Styling

**DataBox:** Views use styles targeting `DataBoxRow` and `DataBoxCell` with bindings to ViewModel properties like `RowBrush`, `RowStyle`.

**DataGrid strategy:**

- `LoadingRow` event — set row properties based on DataContext
- Or use `DataGrid.RowTheme`/`CellTheme` with custom themes
- DataGridRow supports pseudo-classes and style bindings
- DataGridRow has `Foreground` and `Background` properties

### New Capabilities DataGrid Adds

| Feature | Description | Potential Use |
|---------|-------------|---------------|
| **Cell editing** | Begin/Commit/Cancel lifecycle | WatchListView (replace custom TextBox hack) |
| **Row details** | Expandable per-row sections | Breakpoint conditions, label comments |
| **Frozen columns** | Columns that don't scroll | Address columns in memory views |
| **Column reorder** | Drag columns to reorder | User customization |
| **Row grouping** | Group rows by property | Function list by module |
| **Cell validation** | `:invalid` pseudo-class | Watch expression validation |
| **Auto-generate columns** | From ItemsSource type | Rapid prototyping |

## Performance Considerations

| Scenario | DataBox | DataGrid | Notes |
|----------|---------|----------|-------|
| Basic rendering | VirtualizingStackPanel | DataGridRowsPresenter (row recycling) | DataGrid may be slightly faster |
| 10K rows | Acceptable | Acceptable (virtualized) | Both virtualize, need to profile |
| Rapid updates | Direct ListBox | INotifyCollectionChanged | DataGrid may be slower for bulk updates |
| Column resize | Custom drag | Built-in with events | DataGrid is smoother |
| Sort | Manual IComparer | CollectionView.SortDescriptions | DataGrid approach is more standard |

**Recommendation:** Profile MemorySearchWindow (largest dataset) early in the prototype phase.

## Migration Risk Assessment

| View | Risk | Reason |
|------|------|--------|
| RegisterTabView | 🟢 Very Low | No custom features, pure text display |
| AssemblerWindow | 🟢 Very Low | Only CellClick, 2 columns |
| FindResultListView | 🟢 Low | Sort + CellDoubleClick |
| CallStackView | 🟡 Low | Custom row styling |
| CheatListWindow | 🟡 Low | Mixed column types |
| FunctionListView | 🟡 Low | Custom row styling |
| LabelListView | 🟡 Medium | TemplateColumn + row styling |
| BreakpointListView | 🟡 Medium | CheckBox columns + events |
| MemorySearchWindow | 🟡 Medium | 10 columns, large dataset |
| ProfilerWindow | 🟡 Medium | 9 columns, CopyToClipboard |
| EventViewerWindow | 🟠 Medium-High | Color template column |
| SpriteViewerWindow | 🟠 Medium-High | Complex template columns |
| WatchListView | 🔴 High | Editable cells, custom key handlers |

## Conclusion

The migration is **feasible** and should proceed incrementally:

1. **RegisterTabView** proves the basic approach works
2. **Extension library** builds reusable infrastructure
3. **Phase 1** (5 simple views) validates at scale
4. **Phase 2** (8 complex views) handles edge cases
5. **Cleanup** removes 19 vendored files (~1,800 lines)

The biggest risk is **WatchListView** with its editable cells — but DataGrid's native editing support (`CellEditingTemplate`, `BeginningEdit`/`CommittingEdit` events) may actually be *better* than the current hack with TextBoxes inside DataBoxTemplateColumns.
