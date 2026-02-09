# Check for Updates Modernization Plan

## Overview

The "Check for Updates" feature needs modernization to work with Nexen's distribution model and GitHub releases.

## Current Issues

### Endpoint Issues
- Currently points to Mesen update servers (no longer valid)
- Needs to use GitHub Releases API
- May have hardcoded URLs

### Functionality Issues
- Update checking logic may be outdated
- Version comparison may need updating
- Download/install flow needs review

### UI Issues
- Dialog may use old Avalonia patterns
- May not show proper release notes
- User experience could be improved

## Requirements

### Update Source
- Use GitHub Releases API: `https://api.github.com/repos/TheAnsarya/Nexen/releases/latest`
- Parse release information from GitHub response
- Support pre-release vs stable release options

### Version Comparison
- Compare semantic versions properly
- Handle build numbers/date stamps
- Support "ignore this version" option

### Update Flow Options

#### Option A: Direct Download
1. Check GitHub for latest release
2. Show release notes and download link
3. User manually downloads and installs

#### Option B: In-App Update
1. Check GitHub for latest release
2. Download release asset directly
3. Extract/install (platform-specific)

### User Preferences
- Enable/disable automatic update checks on startup
- Check frequency setting
- Pre-release channel option

## Implementation Phases

### Phase 1: Service Layer
- Create `IUpdateService` interface
- Implement `GitHubUpdateService`
- Parse GitHub Releases API response
- Version comparison logic

### Phase 2: ViewModel
- Create `UpdateCheckViewModel`
- Implement async update checking
- Handle network errors gracefully
- Reactive properties for UI binding

### Phase 3: View
- Modern update available dialog
- Release notes display (markdown support?)
- Download progress indication
- Settings integration

### Phase 4: Integration
- Hook into application startup (optional check)
- Menu action implementation
- Settings for update preferences

## Technical Details

### GitHub Releases API

```http
GET https://api.github.com/repos/TheAnsarya/Nexen/releases/latest
Accept: application/vnd.github.v3+json
```

Response contains:
- `tag_name` - Version tag
- `name` - Release name
- `body` - Release notes (markdown)
- `published_at` - Release date
- `assets[]` - Download files

### Version Parsing

Current version from:
- Assembly version attribute
- Build-time generated version file
- Embedded resource

### Platform Considerations

| Platform | Asset Pattern | Install Method |
|----------|---------------|----------------|
| Windows  | `Nexen-win-x64.zip` | Extract & replace |
| Linux    | `Nexen-linux-x64.tar.gz` | Extract & replace |
| macOS    | `Nexen-osx-arm64.zip` | Extract & replace |

## Files to Modify/Create

### New Files
- `UI/Services/IUpdateService.cs` - Interface
- `UI/Services/GitHubUpdateService.cs` - Implementation
- `UI/ViewModels/UpdateCheckViewModel.cs` - ViewModel
- `UI/Windows/UpdateAvailableWindow.axaml` - Dialog
- `UI/Windows/UpdateAvailableWindow.axaml.cs` - Code-behind

### Modified Files
- `UI/Config/PreferencesConfig.cs` - Update settings
- `UI/ViewModels/MainMenuViewModel.cs` - Menu action
- `App.axaml.cs` - Startup check hook

## Configuration Options

```csharp
public class UpdateConfig
{
    public bool CheckOnStartup { get; set; } = true;
    public bool IncludePreReleases { get; set; } = false;
    public string? IgnoredVersion { get; set; }
    public DateTime? LastCheckTime { get; set; }
    public int CheckIntervalDays { get; set; } = 7;
}
```

## Error Handling

- Network unavailable → Silent fail with option to retry
- Rate limited → Cache and retry later
- Parse errors → Log and graceful degradation
- No releases found → Show "no updates available"

## Timeline

- Priority: Medium-Low
- Estimated effort: 6-10 hours
- Should be completed after About dialog
- Part of Epic 13 (UI Modernization)

## Related Issues

- Part of UI Modernization (Epic 13)
- Depends on GitHub repository being set up for releases
- Should coordinate with About dialog modernization
- Needs release pipeline to be functional first
