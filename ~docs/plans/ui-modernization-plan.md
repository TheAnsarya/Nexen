# UI Modernization Plan

## Epic: [Epic 13] UI Framework and Package Modernization

**Status:** Planning  
**Created:** 2026-02-08  
**Priority:** High

---

## 1. Current State Analysis

### 1.1 Current Package Versions

| Package | Current Version | Latest Stable | Notes |
|---------|-----------------|---------------|-------|
| Avalonia | 11.3.11 | 11.3.11 | ✅ Up to date |
| Avalonia.Desktop | 11.3.11 | 11.3.11 | ✅ Up to date |
| Avalonia.Themes.Fluent | 11.3.11 | 11.3.11 | ✅ Up to date |
| Avalonia.ReactiveUI | 11.3.9 | 11.3.11 | ⚠️ Minor update |
| Avalonia.AvaloniaEdit | 11.3.0 | 11.3.x | ⚠️ Update available |
| Avalonia.Controls.ColorPicker | 11.3.11 | 11.3.11 | ✅ Up to date |
| Dock.Avalonia | 11.3.9 | 11.3.11.6 | ⚠️ Update available |
| Dock.Avalonia.Themes.Fluent | 11.3.9 | 11.3.11.6 | ⚠️ Update available |
| Dock.Model.Mvvm | 11.3.9 | 11.3.11.6 | ⚠️ Update available |
| ReactiveUI.Fody | 19.5.41 | 19.5.41 | ✅ Up to date (ReactiveUI 22.3.1) |
| System.IO.Hashing | 10.0.2 | 10.0.2 | ✅ Up to date |
| ELFSharp | 2.17.3 | 2.17.x | ✅ Check for updates |

### 1.2 Framework Versions

| Framework | Current | Latest | Notes |
|-----------|---------|--------|-------|
| .NET | 10.0 | 10.0 | ✅ Up to date |
| C# Language | C# 13+ | C# 13 | ✅ Up to date |

### 1.3 Current Architecture Patterns

#### MVVM Implementation

- **Base Classes:**
  - `ViewModelBase` : `ReactiveObject` (basic reactive support)
  - `DisposableViewModel` : `ViewModelBase`, `IDisposable` (resource tracking)
- **Reactive Patterns:**
  - `[Reactive]` attribute from ReactiveUI.Fody
  - `WhenAnyValue()` for property observation
  - `ReactiveCommand<TIn, TOut>` for commands
- **Disposal:** Manual `AddDisposable()` pattern with HashSet

#### View-ViewModel Binding

- Compiled bindings enabled (`AvaloniaUseCompiledBindingsByDefault`)
- `x:DataType` used in AXAML files
- Mix of code-behind and pure XAML approaches

#### Menu System

- Recently modernized `ShortcutMenuAction` and `DebugMenuAction` classes
- `MenuActionBase` abstract base for all menu items
- Custom context menu action system

#### Control Architecture

- Custom controls: `NexenUserControl`, `NexenWindow`, `NexenMenu`
- Custom numeric controls: `NexenNumericUpDown`, `NexenNumericTextBox`
- Specialized renderers: `NativeRenderer`, `SoftwareRendererView`

---

## 2. Modernization Opportunities

### 2.1 Package Updates (Low Risk)

**Immediate Updates:**

```xml
<PackageReference Include="Avalonia.ReactiveUI" Version="11.3.11" />
<PackageReference Include="Dock.Avalonia" Version="11.3.11.6" />
<PackageReference Include="Dock.Avalonia.Themes.Fluent" Version="11.3.11.6" />
<PackageReference Include="Dock.Model.Mvvm" Version="11.3.11.6" />
```

### 2.2 ReactiveUI Modernization (Medium Risk)

**Current State:**

- Using `ReactiveUI.Fody` 19.5.41 (based on ReactiveUI 22.3.1)
- Manual disposal tracking with `AddDisposable()`

**Modernization Options:**

1. **Adopt `CompositeDisposable` from System.Reactive:**

   ```csharp
   // Instead of custom HashSet disposal
   private readonly CompositeDisposable _disposables = new();
   
   public void Dispose() {
       _disposables.Dispose();
   }
   ```

2. **Use `ReactiveUI.SourceGenerators`** (Future):
   - Source generators replacing Fody for `[Reactive]`
   - Better AOT compatibility
   - No IL weaving required

3. **Standardize `ObservableAsPropertyHelper<T>`:**

   ```csharp
   private readonly ObservableAsPropertyHelper<bool> _isLoading;
   public bool IsLoading => _isLoading.Value;
   ```

### 2.3 XAML/AXAML Modernization (Low-Medium Risk)

**Opportunities:**

1. **Ensure 100% Compiled Bindings:**
   - Audit all `.axaml` files for missing `x:DataType`
   - Remove reflection-based bindings

2. **Modern Control Templates:**
   - Review and update style files in `UI/Styles/`
   - Adopt new Avalonia 11.x theming features

3. **Resource Dictionary Optimization:**
   - Consolidate duplicate styles
   - Use `MergedDictionaries` effectively

### 2.4 AOT/Trimming Readiness (Medium Risk)

**Current State:**

- `IsAotCompatible` = true
- `JsonSerializerIsReflectionEnabledByDefault` = false
- Trimmer root assemblies defined

**Opportunities:**

1. **Eliminate Reflection Usage:**
   - Audit `ReactiveHelper.RegisterRecursiveObserver()`
   - Check `EnumComboBox` and similar controls

2. **Source Generator Migration:**
   - Consider CommunityToolkit.Mvvm as alternative
   - Or wait for ReactiveUI.SourceGenerators

### 2.5 Performance Patterns (Medium Risk)

**Opportunities:**

1. **Virtualization:**
   - Ensure `VirtualizingStackPanel` used for large lists
   - Debugger views (disassembly, memory) audit

2. **Lazy Loading:**
   - Defer view creation for tabs
   - Use `Lazy<T>` for expensive ViewModels

3. **Memory Management:**
   - Review bitmap/image disposal
   - Audit `DynamicBitmap` usage patterns

---

## 3. Proposed Implementation Phases

### Phase 1: Package Updates (1-2 days)

- Update all packages to latest stable versions
- Test thoroughly for regressions
- Low risk, immediate benefits

### Phase 2: Disposal Pattern Standardization (3-5 days)

- Introduce `CompositeDisposable`
- Standardize disposal across ViewModels
- Update `DisposableViewModel` base class

### Phase 3: Compiled Bindings Audit (2-3 days)

- Audit all 156+ AXAML files
- Add missing `x:DataType` declarations
- Remove any reflection-based bindings

### Phase 4: Style Modernization (3-5 days)

- Review all 13 style files
- Update to Avalonia 11.x best practices
- Consolidate duplicate styles

### Phase 5: Control Modernization (5-7 days)

- Audit custom controls
- Update styled property patterns
- Improve accessibility

### Phase 6: Performance Optimization (3-5 days)

- Virtualization audit
- Memory usage profiling
- Lazy loading implementation

---

## 4. Risk Assessment

| Change Area | Risk Level | Mitigation |
|-------------|------------|------------|
| Package updates | Low | Incremental updates, test between each |
| Disposal patterns | Medium | Keep backward compatibility |
| Compiled bindings | Low | Gradual file-by-file migration |
| Style updates | Medium | Visual regression testing |
| Control updates | Medium | Preserve existing APIs |
| Performance | Low | Benchmarks before/after |

---

## 5. Testing Strategy

### 5.1 Required Testing

1. **Visual Regression Testing:**
   - All configuration dialogs
   - Debugger windows
   - Main window layouts

2. **Functional Testing:**
   - Menu shortcuts (all systems)
   - Window docking/undocking
   - Theme switching

3. **Performance Testing:**
   - Disassembly view scrolling
   - Memory viewer updates
   - Large file handling

### 5.2 Platforms

- Windows x64 (primary)
- Linux x64 (if supported)
- macOS (if supported)

---

## 6. Documentation Requirements

- [ ] Migration guide for each phase
- [ ] Updated architecture documentation
- [ ] Style guide for new patterns
- [ ] Control usage guidelines

---

## 7. Related Documents

- [ARCHITECTURE-OVERVIEW.md](../ARCHITECTURE-OVERVIEW.md)
- [CPP-DEVELOPMENT-GUIDE.md](../CPP-DEVELOPMENT-GUIDE.md)
- [CODE-DOCUMENTATION-STYLE.md](../CODE-DOCUMENTATION-STYLE.md)

---

## 8. Appendix: File Inventory

### ViewModels (38 files)

```text
UI/ViewModels/
├── AudioConfigViewModel.cs
├── AudioPlayerViewModel.cs
├── CheatDatabaseViewModel.cs
├── CheatEditWindowViewModel.cs
├── CheatListWindowViewModel.cs
├── ColorPickerViewModel.cs
├── ConfigViewModel.cs
├── ControllerConfigViewModel.cs
├── ... (30 more)
└── ViewModelBase.cs
```

### Controls (48 files)

```text
UI/Controls/
├── BoolToGridLengthConverter.cs
├── ButtonWithIcon.axaml[.cs]
├── CheckBoxWarning.axaml[.cs]
├── ... (45 more)
```

### Views (156+ AXAML files)

- Distributed across Views/, Debugger/Views/, Windows/

### Styles (13 files)

```text
UI/Styles/
├── AvaloniaComboBoxStyles.xaml
├── AvaloniaContextMenuStyles.xaml
├── AvaloniaEditStyles.xaml
├── AvaloniaMenuItemStyles.xaml
├── AvaloniaNumericUpDownStyles.xaml
├── AvaloniaScrollViewerStyles.xaml
├── DataBoxStyles.xaml
├── DebugStatusStyles.xaml
├── DockStyles.xaml
├── NexenStyles.Dark.xaml
├── NexenStyles.Light.xaml
├── NexenStyles.xaml
└── StartupStyles.xaml
```
