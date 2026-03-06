# DataBox → Avalonia DataGrid Migration Code Plan

**Issue:** #180 | **Created:** 2026-02-09

## Architecture Overview

```text
Current:                          Target:
┌──────────────────┐             ┌──────────────────────────┐
│  DataBox (vendor) │             │  Avalonia DataGrid        │
│  19 source files  │      ──►   │  (built-in, maintained)   │
│  ~1,800 lines     │             │                          │
│  12+ custom ext.  │             │  + DataGridExtensions/   │
└──────────────────┘             │    (our behaviors)        │
                                  └──────────────────────────┘
```

## Extension Library Design

### Location: `UI/Controls/DataGridExtensions/`

### 1. ColumnWidthPersistence (Attached Behavior)

```csharp
// Tracks column widths by ID and persists to config
public static class DataGridColumnWidthBehavior
{
    public static readonly AttachedProperty<string> ColumnWidthIdProperty = ...;
    public static readonly AttachedProperty<Dictionary<string, double>> ColumnWidthsProperty = ...;
    
    // On column resize → update dictionary
    // On DataGrid loaded → restore widths from dictionary
}
```

### 2. MultiColumnSort (Attached Behavior)

```csharp
// Replicates DataBox SortState with multi-column support
public static class DataGridMultiSortBehavior
{
    public static readonly AttachedProperty<SortState> SortStateProperty = ...;
    
    // On column header click → update SortState → apply CollectionView sorting
}
```

### 3. CellClickEvents (Attached Behavior)

```csharp
// Adds CellClick and CellDoubleClick events to DataGrid
public static class DataGridCellClickBehavior
{
    public static readonly RoutedEvent<DataGridCellClickEventArgs> CellClickEvent = ...;
    public static readonly RoutedEvent<DataGridCellClickEventArgs> CellDoubleClickEvent = ...;
}
```

### 4. ClipboardExport (Extension Method)

```csharp
// CopyToClipboard CSV export
public static class DataGridClipboardExtensions
{
    public static void CopyAllToClipboard(this DataGrid grid) { ... }
}
```

### 5. TypeAheadSearch (Attached Behavior)

```csharp
// Keyboard search with hex prefix ($) support
public static class DataGridTypeAheadBehavior
{
    public static readonly AttachedProperty<bool> EnableTypeAheadProperty = ...;
    // On key press → search visible rows → select matching
}
```

## AXAML Migration Pattern

### Before (DataBox)

```xml
<DataBox Items="{Binding Items}"
         Selection="{Binding Selection}"
         SortState="{Binding SortState}"
         GridLinesVisibility="All"
         ColumnWidths="{Binding ColumnWidths}"
         CellClick="OnCellClick"
         CellDoubleClick="OnCellDoubleClick">
    <DataBox.Columns>
        <DataBoxTextColumn Header="Name" Binding="{Binding Name}" 
                           CanUserResize="True" InitialWidth="100" 
                           ColumnWidthId="Name" />
    </DataBox.Columns>
</DataBox>
```

### After (DataGrid + Extensions)

```xml
<DataGrid ItemsSource="{Binding Items}"
          SelectedItem="{Binding SelectedItem}"
          ext:MultiSort.SortState="{Binding SortState}"
          GridLinesVisibility="All"
          ext:ColumnWidth.Widths="{Binding ColumnWidths}"
          ext:CellClick.Command="{Binding CellClickCommand}"
          ext:CellClick.DoubleClickCommand="{Binding CellDoubleClickCommand}">
    <DataGrid.Columns>
        <DataGridTextColumn Header="Name" Binding="{Binding Name}"
                            CanUserResize="True" Width="100"
                            ext:ColumnWidth.Id="Name" />
    </DataGrid.Columns>
</DataGrid>
```

## Migration Checklist Per View

For each of the 13 DataBox instances:

- [ ] Replace `<DataBox>` → `<DataGrid>`
- [ ] Replace `Items=` → `ItemsSource=`
- [ ] Replace `Selection=` → `SelectedItem=`/`SelectedItems=`
- [ ] Replace `DataBoxTextColumn` → `DataGridTextColumn`
- [ ] Replace `DataBoxCheckBoxColumn` → `DataGridCheckBoxColumn`
- [ ] Replace `DataBoxTemplateColumn` → `DataGridTemplateColumn`
- [ ] Replace `InitialWidth=` → `Width=`
- [ ] Add `ext:ColumnWidth.Id=` for persistence
- [ ] Add `ext:MultiSort.SortState=` if sorting used
- [ ] Wire up CellClick/CellDoubleClick if events used
- [ ] Update code-behind event handlers
- [ ] Test dark/light theme
- [ ] Test column resize persistence
- [ ] Visual regression comparison

## Risk Mitigation

1. **Spike branch first** — Don't touch master until prototype validates approach
2. **One view at a time** — Merge each view separately, test between merges
3. **Feature flags** — Consider #if DATABOX_MIGRATION to toggle old/new at compile time
4. **Screenshot testing** — Before/after comparisons for every view
5. **Performance profiling** — MemorySearchWindow with 10K+ rows must not regress

## Estimated Effort

| Phase | Issues | Est. Time | Notes |
|-------|--------|-----------|-------|
| Research | #182, #183 | 1-2 hours | Mostly done (this doc) |
| Prototype | #184 | 2-3 hours | RegisterTabView spike |
| Extensions | #185 | 4-6 hours | Core behavior library |
| Phase 1 (5 views) | #186 | 3-4 hours | Simple views |
| Phase 2 (8 views) | #187 | 6-10 hours | Complex views |
| Cleanup | #188 | 30 min | Delete vendor code |
| **Total** | | **~16-25 hours** | Across multiple sessions |
