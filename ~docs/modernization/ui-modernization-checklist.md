# UI Modernization - Implementation Checklist

## Epic #158: UI Framework and Package Modernization

Use this checklist to track implementation progress.

---

## Phase 1: Package Updates (#159) ✅ COMPLETED

### Pre-Flight ✅
- [x] Review release notes for each package
- [x] Backup current working state (git tag)
- [x] Note any known breaking changes

### Package Updates ✅
- [x] Update `Avalonia.ReactiveUI` 11.3.9 → kept (latest stable)
- [x] Update `Dock.Avalonia` 11.3.9 → 11.3.11.7
- [x] Update `Dock.Avalonia.Themes.Fluent` 11.3.9 → 11.3.11.7
- [x] Update `Dock.Model.Mvvm` 11.3.9 → 11.3.11.7
- [x] Check `Avalonia.AvaloniaEdit` for updates → 11.4.1
- [x] Also updated: Serilog 4.3.0, BenchmarkDotNet 0.15.8, test packages

### Verification ✅
- [x] `dotnet restore` succeeds
- [x] `dotnet build -c Release` succeeds
- [x] Application starts without errors
- [x] Debugger docking works
- [x] Theme switching works
- [x] Menu shortcuts function
- [x] No visual regressions

---

## Phase 2: Disposal Pattern (#160) ✅ COMPLETED

### Base Class Updates ✅
- [x] Add `using System.Reactive.Disposables;`
- [x] Replace `HashSet<IDisposable>` with `CompositeDisposable`
- [x] Update `DisposableViewModel.Dispose()`
- [x] Update `NexenUserControl` to match

### ViewModels Using Disposal ✅
- [x] Audit all ViewModels for disposal patterns
- [x] Update any custom disposal implementations
- [x] Added `GC.SuppressFinalize(this)` for proper disposal pattern

---

## Phase 3: Compiled Bindings (#161)

### Discovery
- [ ] Generate list of AXAML files without `x:DataType`
- [ ] Categorize by complexity

### Views (by folder) ✅ Audited
- [x] `UI/Views/*.axaml` - all have x:DataType
- [x] `UI/Windows/*.axaml` - all have x:DataType
- [x] `UI/Controls/*.axaml` - added x:DataType where needed
- [x] `UI/Debugger/Views/*.axaml` - all have x:DataType
- [x] `UI/Debugger/Windows/*.axaml` - all have x:DataType
- [x] `UI/Debugger/Controls/*.axaml` - added x:DataType where needed
- [x] `UI/Debugger/StatusViews/*.axaml` - all have x:DataType

### Note on Remaining Files
Files without x:DataType are either:
- Style/theme files (no bindings)
- Controls using only ElementName bindings
- App.axaml (no data bindings)
All are valid and don't need x:DataType.

### Verification ✅
- [x] Build with no binding warnings
- [x] All views render correctly
- [x] AvaloniaUseCompiledBindingsByDefault=true in csproj

---

## Phase 4: Style Modernization (#162)

### Audit
- [ ] `AvaloniaComboBoxStyles.xaml`
- [ ] `AvaloniaContextMenuStyles.xaml`
- [ ] `AvaloniaEditStyles.xaml`
- [ ] `AvaloniaMenuItemStyles.xaml`
- [ ] `AvaloniaNumericUpDownStyles.xaml`
- [ ] `AvaloniaScrollViewerStyles.xaml`
- [ ] `DataBoxStyles.xaml`
- [ ] `DebugStatusStyles.xaml`
- [ ] `DockStyles.xaml`
- [ ] `NexenStyles.Dark.xaml`
- [ ] `NexenStyles.Light.xaml`
- [ ] `NexenStyles.xaml`
- [ ] `StartupStyles.xaml`

### Updates
- [ ] Remove deprecated APIs
- [ ] Consolidate duplicate styles
- [ ] Update to ControlTheme where appropriate
- [ ] Test theme switching

---

## Phase 5: Control Modernization (#163)

### Core Controls
- [ ] `NexenMenu.cs`
- [ ] `NexenNumericUpDown.cs`
- [ ] `NexenNumericTextBox.cs`
- [ ] `NexenSlider.axaml`
- [ ] `NexenScrollContentPresenter.cs`

### Input Controls
- [ ] `KeyBindingButton.axaml`
- [ ] `MultiKeyBindingButton.axaml`
- [ ] `ControllerButton.axaml`
- [ ] `InputComboBox.axaml`

### Selection Controls
- [ ] `EnumComboBox.axaml`
- [ ] `EnumRadioButton.cs`

### Layout Controls
- [ ] `GroupBox.axaml`
- [ ] `OptionSection.axaml`
- [ ] `SystemSpecificSettings.axaml`

### Specialized
- [ ] `PaletteConfig.axaml`
- [ ] `StateGrid.axaml`
- [ ] `PathSelector.axaml`

### Rendering
- [ ] `NativeRenderer.cs`
- [ ] `SoftwareRendererView.axaml`
- [ ] `SimpleImageViewer.cs`
- [ ] `PianoRollControl.cs`

### Accessibility
- [ ] Add AutomationProperties to all controls
- [ ] Test keyboard navigation
- [ ] Verify focus order

---

## Phase 6: Performance (#164)

### Virtualization
- [ ] Disassembly view
- [ ] Memory viewer
- [ ] Watch window
- [ ] Trace logger
- [ ] Breakpoint list

### Profiling
- [ ] Baseline measurements captured
- [ ] Startup time measured
- [ ] Memory usage profiled
- [ ] Scroll performance tested

### Optimizations
- [ ] Lazy loading implemented where needed
- [ ] Unnecessary bindings removed
- [ ] Image caching verified
- [ ] Event handler cleanup verified

---

## Final Verification

### Functional Testing
- [ ] All emulator cores work
- [ ] Debugger fully functional
- [ ] TAS editor works
- [ ] Settings save/load
- [ ] Recent games work
- [ ] NetPlay (if applicable)

### Visual Testing
- [ ] Light theme correct
- [ ] Dark theme correct
- [ ] All dialogs render
- [ ] Docking layouts work
- [ ] Menu displays correct

### Platform Testing
- [ ] Windows x64
- [ ] (Optional) Linux x64
- [ ] (Optional) macOS

---

## Documentation

- [ ] Update ARCHITECTURE-OVERVIEW.md
- [ ] Style guide complete
- [ ] Control usage documented
- [ ] Migration notes for future reference
