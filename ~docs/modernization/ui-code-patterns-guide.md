# UI Modernization - Code Patterns Guide

## Overview

This guide documents the target code patterns for the UI modernization effort (Epic #158). Use these patterns when implementing changes.

---

## 1. ViewModel Patterns

### 1.1 Base ViewModel

```csharp
using System;
using System.Reactive.Disposables;
using ReactiveUI;

namespace Nexen.ViewModels;

/// <summary>
/// Base class for all ViewModels.
/// </summary>
public class ViewModelBase : ReactiveObject {
}

/// <summary>
/// ViewModel with disposable resource management.
/// </summary>
public class DisposableViewModel : ViewModelBase, IDisposable {
    private readonly CompositeDisposable _disposables = new();
    private bool _disposed;

    public bool Disposed => _disposed;

    public void Dispose() {
        if (_disposed) return;
        _disposed = true;
        
        _disposables.Dispose();
        DisposeView();
        GC.SuppressFinalize(this);
    }

    protected virtual void DisposeView() { }

    protected T AddDisposable<T>(T disposable) where T : IDisposable {
        _disposables.Add(disposable);
        return disposable;
    }
}
```

### 1.2 Reactive Properties

```csharp
using ReactiveUI.Fody.Helpers;

public class MyViewModel : DisposableViewModel {
    // Simple reactive property (Fody weaves INotifyPropertyChanged)
    [Reactive] public string Name { get; set; } = "";
    
    // Computed property with ObservableAsPropertyHelper
    private readonly ObservableAsPropertyHelper<bool> _isValid;
    public bool IsValid => _isValid.Value;
    
    public MyViewModel() {
        _isValid = this.WhenAnyValue(x => x.Name)
            .Select(name => !string.IsNullOrEmpty(name))
            .ToProperty(this, x => x.IsValid);
        
        AddDisposable(_isValid);
    }
}
```

### 1.3 Commands

```csharp
using System.Reactive;
using ReactiveUI;

public class MyViewModel : DisposableViewModel {
    public ReactiveCommand<Unit, Unit> SaveCommand { get; }
    public ReactiveCommand<string, bool> ValidateCommand { get; }
    
    public MyViewModel() {
        // Command with canExecute
        var canSave = this.WhenAnyValue(x => x.IsValid);
        SaveCommand = ReactiveCommand.CreateFromTask(SaveAsync, canSave);
        
        // Command with parameter and return value
        ValidateCommand = ReactiveCommand.Create<string, bool>(Validate);
        
        AddDisposable(SaveCommand);
        AddDisposable(ValidateCommand);
    }
    
    private async Task SaveAsync() { /* ... */ }
    private bool Validate(string input) => !string.IsNullOrEmpty(input);
}
```

---

## 2. AXAML Patterns

### 2.1 View Declaration with Compiled Bindings

```xml
<UserControl xmlns="https://github.com/avaloniaui"
             xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
             xmlns:vm="using:Nexen.ViewModels"
             xmlns:d="http://schemas.microsoft.com/expression/blend/2008"
             xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006"
             mc:Ignorable="d" d:DesignWidth="400" d:DesignHeight="300"
             x:Class="Nexen.Views.MyView"
             x:DataType="vm:MyViewModel">
    
    <Design.DataContext>
        <vm:MyViewModel />
    </Design.DataContext>
    
    <StackPanel>
        <TextBox Text="{Binding Name}" />
        <Button Content="Save" Command="{Binding SaveCommand}" />
    </StackPanel>
</UserControl>
```

### 2.2 Control Templates

```xml
<!-- Modern ControlTheme pattern -->
<ControlTheme x:Key="NexenPrimaryButton" TargetType="Button">
    <Setter Property="Background" Value="{DynamicResource AccentBrush}" />
    <Setter Property="Foreground" Value="{DynamicResource AccentForegroundBrush}" />
    <Setter Property="Padding" Value="16,8" />
    <Setter Property="CornerRadius" Value="4" />
    
    <Style Selector="^:pointerover">
        <Setter Property="Background" Value="{DynamicResource AccentBrushHover}" />
    </Style>
    
    <Style Selector="^:pressed">
        <Setter Property="Background" Value="{DynamicResource AccentBrushPressed}" />
    </Style>
</ControlTheme>
```

### 2.3 Resource References

```xml
<!-- Use DynamicResource for theme-aware values -->
<TextBlock Foreground="{DynamicResource TextForegroundBrush}" />

<!-- Use StaticResource for compile-time constants -->
<Grid Margin="{StaticResource DefaultMargin}" />
```

---

## 3. Custom Control Patterns

### 3.1 StyledProperty Declaration

```csharp
using Avalonia;
using Avalonia.Controls;

public class MyControl : Control {
    public static readonly StyledProperty<string> TextProperty =
        AvaloniaProperty.Register<MyControl, string>(nameof(Text), defaultValue: "");
    
    public static readonly StyledProperty<bool> IsActiveProperty =
        AvaloniaProperty.Register<MyControl, bool>(nameof(IsActive));

    public string Text {
        get => GetValue(TextProperty);
        set => SetValue(TextProperty, value);
    }

    public bool IsActive {
        get => GetValue(IsActiveProperty);
        set => SetValue(IsActiveProperty, value);
    }

    static MyControl() {
        // Property changed handler
        TextProperty.Changed.AddClassHandler<MyControl>((x, e) => x.OnTextChanged(e));
        
        // Affect render
        AffectsRender<MyControl>(IsActiveProperty);
    }

    private void OnTextChanged(AvaloniaPropertyChangedEventArgs e) {
        // Handle property change
    }
}
```

### 3.2 Template Parts

```csharp
using Avalonia.Controls.Primitives;

[TemplatePart(Name = "PART_ContentHost", Type = typeof(ContentPresenter))]
public class MyTemplatedControl : TemplatedControl {
    private ContentPresenter? _contentHost;

    protected override void OnApplyTemplate(TemplateAppliedEventArgs e) {
        base.OnApplyTemplate(e);
        
        _contentHost = e.NameScope.Find<ContentPresenter>("PART_ContentHost");
        
        if (_contentHost != null) {
            // Configure template part
        }
    }
}
```

### 3.3 Accessibility

```csharp
using Avalonia.Automation;

public class AccessibleButton : Button {
    static AccessibleButton() {
        AutomationProperties.AutomationIdProperty.OverrideDefaultValue<AccessibleButton>("");
    }

    protected override AutomationPeer OnCreateAutomationPeer() {
        return new ButtonAutomationPeer(this);
    }
}
```

```xml
<Button Content="Save"
        AutomationProperties.Name="Save document"
        AutomationProperties.HelpText="Saves the current document to disk" />
```

---

## 4. Performance Patterns

### 4.1 Virtualization

```xml
<!-- For large lists -->
<ListBox ItemsSource="{Binding Items}">
    <ListBox.ItemsPanel>
        <ItemsPanelTemplate>
            <VirtualizingStackPanel />
        </ItemsPanelTemplate>
    </ListBox.ItemsPanel>
</ListBox>
```

### 4.2 Lazy Loading

```csharp
public class TabViewModel : DisposableViewModel {
    private readonly Lazy<ExpensiveChildViewModel> _child;
    
    public ExpensiveChildViewModel Child => _child.Value;
    
    public TabViewModel() {
        _child = new Lazy<ExpensiveChildViewModel>(() => {
            var vm = new ExpensiveChildViewModel();
            AddDisposable(vm);
            return vm;
        });
    }
}
```

### 4.3 Batched Updates

```csharp
using Avalonia.Threading;

// Batch multiple property changes
Dispatcher.UIThread.RunBatched(() => {
    Property1 = value1;
    Property2 = value2;
    Property3 = value3;
});
```

### 4.4 Binding Modes

```xml
<!-- OneTime for static data -->
<TextBlock Text="{Binding Title, Mode=OneTime}" />

<!-- OneWay is default for most bindings -->
<TextBlock Text="{Binding DynamicText}" />

<!-- TwoWay for editable controls -->
<TextBox Text="{Binding Name, Mode=TwoWay}" />
```

---

## 5. Style Patterns

### 5.1 Theme-Aware Colors

```xml
<ResourceDictionary>
    <!-- Define in both Dark and Light themes -->
    <Color x:Key="PrimaryColor">#0066CC</Color>
    <SolidColorBrush x:Key="PrimaryBrush" Color="{StaticResource PrimaryColor}" />
</ResourceDictionary>
```

### 5.2 Conditional Styles

```xml
<Style Selector="Button.danger">
    <Setter Property="Background" Value="{DynamicResource DangerBrush}" />
</Style>

<Style Selector="TextBox:focus">
    <Setter Property="BorderBrush" Value="{DynamicResource FocusBorderBrush}" />
</Style>

<Style Selector="ListBoxItem:selected">
    <Setter Property="Background" Value="{DynamicResource SelectionBrush}" />
</Style>
```

---

## 6. Common Migrations

### 6.1 HashSet to CompositeDisposable ✅ IMPLEMENTED

```csharp
// Before (DEPRECATED)
private HashSet<IDisposable> _disposables = new();
public void Dispose() {
    foreach (var d in _disposables) d.Dispose();
    _disposables.Clear();
}

// After (CURRENT - using System.Reactive.Disposables)
private readonly CompositeDisposable _disposables = [];
public void Dispose() {
    if (Disposed) return;
    Disposed = true;
    _disposables.Dispose();
    DisposeView();
    GC.SuppressFinalize(this);
}
```

Both `DisposableViewModel` and `NexenUserControl` now use this pattern.

### 6.2 Missing x:DataType

```xml
<!-- Before -->
<UserControl xmlns="...">
    <TextBlock Text="{Binding Name}" />
</UserControl>

<!-- After -->
<UserControl xmlns="..."
             x:DataType="vm:MyViewModel">
    <TextBlock Text="{Binding Name}" />
</UserControl>
```

### 6.3 Deprecated Style Selectors

```xml
<!-- Old -->
<Style Selector="Button /template/ ContentPresenter">

<!-- New (if applicable) -->
<Style Selector="Button > ContentPresenter">
```

---

## 7. Testing Checklist

After any modernization change, verify:

- [ ] Build succeeds without warnings
- [ ] No visual regressions
- [ ] Theme switching works
- [ ] Keyboard navigation works
- [ ] Memory usage stable
- [ ] No binding errors in output
