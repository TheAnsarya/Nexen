# Epic 21 Icon Coverage Audit (2026-04-21)

## Scope

This audit covers #1413 and verifies icon consistency on the two dense settings menu surfaces used most often during configuration and debugging:

- `UI/Windows/ConfigWindow.axaml`
- `UI/Debugger/Windows/DebuggerConfigWindow.axaml`

## Inventory Snapshot

| Surface | Tab headers | Headers with icon | Non-icon headers | Notes |
|---|---|---|---|---|
| ConfigWindow | 18 | 16 | 2 | 2 non-icon headers are intentional visual separators (`Rectangle`) |
| DebuggerConfigWindow | 7 | 5 | 2 | 2 non-icon headers are intentional visual separators (`Rectangle`) |

## Resolved Gaps

- Added automated regression checks to keep iconized header coverage stable in:
	- `Tests/UI/UiScrollabilityMarkupTests.cs`
- Added guardrails that reject plain text-only top-level tab headers for both settings windows.
- Kept intentional separator headers as non-icon exceptions.

## QA Checklist Integration

Add this checklist to UI QA runs for Epic 21 menu consistency:

1. ConfigWindow: every functional top-level tab header includes an icon.
2. DebuggerConfigWindow: every functional top-level tab header includes an icon.
3. Separator tabs are present only as disabled divider entries.
4. Icon + text alignment remains horizontally centered and readable at 100% and 150% DPI.
5. Automated guard `SettingsWindows_TabHeadersKeepIconCoverage` passes.

## Verification Command

```powershell
dotnet test Tests/Nexen.Tests.csproj -c Release --filter "FullyQualifiedName~UiScrollabilityMarkupTests" --nologo
```
