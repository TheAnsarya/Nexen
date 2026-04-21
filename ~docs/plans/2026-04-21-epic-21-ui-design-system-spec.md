# Epic 21 UI Design System and Component Spec (2026-04-21)

## Scope

This specification addresses #1416 and standardizes settings-surface UI rules for Avalonia implementation across:

- Onboarding settings (`SetupWizardWindow`)
- Main settings (`ConfigWindow`)
- Debugger settings (`DebuggerConfigWindow`)
- Nested settings views (`UI/Views/*ConfigView.axaml`)

## Design Tokens

### Typography Tokens

| Token | Value | Implementation source |
|---|---|---|
| `NexenFont` | `Microsoft Sans Serif` | `UI/App.axaml` |
| `NexenFontSize` | `11` | `UI/App.axaml` |
| `NexenMenuFont` | `Segoe UI` | `UI/App.axaml` |
| `NexenMenuFontSize` | `12` | `UI/App.axaml` |
| `NexenMonospaceFont` | `Consolas` | `UI/App.axaml` |
| `NexenMonospaceFontSize` | `12` | `UI/App.axaml` |

### Color and Border Tokens

| Token | Value | Usage guidance |
|---|---|---|
| `NexenGrayBorderColor` | `#80808080` | Divider, neutral section outlines, separators |
| `NexenGrayBackgroundColor` | `#40a0a0a0` | Low-emphasis neutral background blocks |
| `ThemeBackgroundBrush` | dynamic theme resource | Window backgrounds for light/dark support |
| `ThemeForegroundBrush` | dynamic theme resource | Primary text foreground |

Source: `UI/Styles/NexenStyles.xaml`

### Spacing and Sizing Tokens

| Token | Value | Implementation source |
|---|---|---|
| `SettingsWindowDefaultSize` | `760 x 620` | `UI/Windows/ConfigWindow.axaml`, `UI/Debugger/Windows/DebuggerConfigWindow.axaml` |
| `SettingsWindowMinSize` | `700 x 560` | Same as above |
| `ActionButtonMinHeight` | `36` | Bottom action buttons in settings windows |
| `DenseScrollPadding` | `0 0 2 0` | Standard `ScrollViewer` wrapper for dense settings sections |
| `DenseStackSpacing` | `4` to `8` | Applied in dense debugger integration blocks |

## Avalonia Mapping Rules

| Design rule | Avalonia mapping |
|---|---|
| Dense setting panels must be scroll-safe | Wrap panel content in `ScrollViewer` with `AllowAutoHide="False"` and `HorizontalScrollBarVisibility="Auto"` |
| Settings windows must be resize-safe | Set `CanResize="True"`, explicit `Width/Height`, and explicit `MinWidth/MinHeight` |
| Touch-friendly actions must remain clickable | Set action button `MinHeight="36"` |
| Narrow layout navigation must remain coherent | Switch `TabStripPlacement` from `Left` to `Top` below compact breakpoint (`900`) |
| Visual section separators should be neutral and theme-safe | Use `NexenGrayBorderColor` for line/divider separators |

## Component Anatomy and Variants

### 1. Settings Window Shell

Required anatomy:

- Root `Window` with explicit default/min sizes
- Bottom action panel with confirm/cancel buttons
- Main `TabControl` using icon + label headers
- Content area with per-tab scroll containers for dense tabs

Variants:

- Primary settings shell: `UI/Windows/ConfigWindow.axaml`
- Debugger settings shell: `UI/Debugger/Windows/DebuggerConfigWindow.axaml`

### 2. Tab Header Item

Required anatomy:

- Horizontal stack
- Icon (`Image`) then text (`TextBlock`)
- Margin between icon and label (`0 0 10 0`)

Variants:

- Functional tab header (icon + text)
- Separator tab header (line `Rectangle`, disabled tab)

### 3. Dense Settings Section

Required anatomy:

- `OptionSection` container
- Optional explanatory hint text (`TextBlock` with wrapping)
- Nested controls grouped by intent with stable spacing

Variants:

- Toggle list section
- Numeric settings section (label + `NumericUpDown`)
- Hybrid section with feature toggles and dependent controls

### 4. Form Controls

Required anatomy:

- Label (where needed)
- Bound input control
- Constraints (min/max/is-enabled bindings)

Variants:

- `CheckBox` toggle row
- `EnumComboBox` select row
- `NumericUpDown` bounded integer row
- Path picker row (`PathSelector`) for file/folder paths

### 5. List and Grid Surfaces

Required anatomy:

- Border/divider lines using theme-safe neutral tokens
- Readability-first row heights and padding
- Selection and focus visibility retained under both themes

Variants:

- Shortcut list/grid surfaces
- Controller mapping list surfaces
- Tooling list panes inside debugger windows

## Implementation Constraints and Notes

- Preserve existing Avalonia MVVM architecture and bindings.
- Prefer incremental container-level changes over cross-cutting control rewrites.
- Keep dynamic resources for theme-aware surfaces.
- Use standardized dense-scroll wrapper values to minimize divergence.

## Review Sign-Off Checklist

1. Token mapping table updated when core values change.
2. Any new settings window follows default/min size and resize rules.
3. Dense tabs include standardized scroll container wrappers.
4. Functional tab headers use icon + label composition.
5. Action controls in settings shells preserve touch-friendly minimum target size.
6. Theme-sensitive colors come from dynamic/static resource tokens, not ad-hoc literals.
7. Design-to-Avalonia mapping remains aligned with current implementation patterns.

## References

- `UI/App.axaml`
- `UI/Styles/NexenStyles.xaml`
- `UI/Windows/ConfigWindow.axaml`
- `UI/Debugger/Windows/DebuggerConfigWindow.axaml`
- `UI/Windows/SetupWizardWindow.axaml`
- `Tests/UI/UiScrollabilityMarkupTests.cs`
