# About Dialog Modernization Plan

## Overview

The current About dialog is inherited from Mesen2 and needs complete modernization for Nexen. It contains outdated information, references to removed dependencies, and the wrong branding.

## Current Issues

### Branding Issues
- Copyright says "Sour" → Should be "2026 Ansarya"
- Homepage URL references Mesen → Should be `ansarya.com/Nexen/` or GitHub repo
- Build date is wrong/stale
- Title still references Mesen branding

### Content Issues
- References libraries and projects that are being removed
- Lists dependencies that may no longer be accurate
- Thanks section mentions wrong contributors
- Version information format may need updating

### Technical Issues
- Likely uses old Avalonia patterns
- May not follow modern MVVM properly
- Possibly uses reflection bindings where compiled bindings would be better

## Requirements

### New About Dialog Should Include

1. **Header Section**
   - Nexen logo/icon
   - Application name: "Nexen"
   - Version number (from assembly)
   - Build date (generated at compile time)

2. **Links Section**
   - Homepage: `https://ansarya.com/Nexen/` or `https://github.com/TheAnsarya/Nexen`
   - GitHub: `https://github.com/TheAnsarya/Nexen`
   - Issues: `https://github.com/TheAnsarya/Nexen/issues`
   - Discord (if applicable)

3. **About Section**
   - Brief description of Nexen
   - Supported systems: NES, SNES, GB, GBA, SMS, PCE, WS

4. **Credits Section**
   - Copyright: "© 2026 Ansarya"
   - Based on Mesen2 by Sour (with appropriate attribution)
   - Major contributors

5. **Dependencies Section**
   - List ONLY current dependencies
   - Avalonia UI
   - ReactiveUI
   - Other actively used libraries

6. **License Section**
   - Link to LICENSE file
   - Brief license summary

## Implementation Phases

### Phase 1: Data Collection
- Audit current dependencies in use
- Identify correct version/build info sources
- Determine final branding decisions

### Phase 2: ViewModel
- Create `AboutViewModel` with proper reactive properties
- Implement version/build date retrieval
- Create structured data for dependencies and credits

### Phase 3: View
- Design modern Avalonia UI layout
- Use compiled bindings
- Implement proper theming support
- Add clickable links with proper handlers

### Phase 4: Integration
- Replace old AboutWindow
- Update menu action
- Test across all supported themes

## Files to Modify/Create

- `UI/Windows/AboutWindow.axaml` - Complete rewrite
- `UI/Windows/AboutWindow.axaml.cs` - Minimal code-behind
- `UI/ViewModels/AboutViewModel.cs` - New ViewModel
- `UI/Resources/about-content.json` (optional) - Externalized content

## Dependencies to Document

Current active dependencies (verify each):
- Avalonia 11.3.x
- ReactiveUI
- AvaloniaEdit
- System.Text.Json
- (others - need to audit)

## Timeline

- Priority: Medium
- Estimated effort: 4-8 hours
- Should be completed as part of Epic 13 (UI Modernization)

## Related Issues

- Part of UI Modernization (Epic 13)
- Depends on final branding decisions
- Should coordinate with Check for Updates modernization
