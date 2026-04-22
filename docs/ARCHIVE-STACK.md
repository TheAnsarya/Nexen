# Archive Stack

This document describes how Nexen handles archive formats across runtime and build pipelines.

## Support Matrix

| Area | ZIP | 7z |
|---|---|---|
| Runtime ROM loading (UI path) | Read | Read |
| Runtime ROM loading (native C++) | Read | No |
| Runtime archive writing (native C++) | Write | No |
| Dependency packaging script (`scripts/package-dependencies.ps1`) | Write | Optional write (CLI-based) |
| UI embedded dependency payload (`UI/Dependencies.zip`) | Required | No |
| Movie converter (`MovieConverter`) | Read/Write | No |

## Current Components

### Runtime Archive Readers

- `UI/Utilities/ArchiveHelper.cs` uses SharpCompress to read ZIP and 7z archives.
- The UI path handles archive entry enumeration and extraction to a temp file before ROM load.
- `Utilities/ArchiveReader.cpp` remains for native ZIP support only (`Utilities/ZipReader`).

### Runtime Archive Writers

- ZIP writing is implemented in `Utilities/ZipWriter` using miniz.
- There is no native or managed 7z writer in Nexen runtime.

### Build/Packaging Archive Creation

- Windows dependency packaging uses `scripts/package-dependencies.ps1`.
- The script always supports creating `Dependencies.zip` (PowerShell `Compress-Archive`).
- The script can optionally create `Dependencies.7z` when `7z`/`7zz`/`7za` is installed and selected via `-ArchiveFormat`.

## Direction And Policy

Nexen no longer vendors SevenZip source code in-repo.

- 7z runtime read support is provided via NuGet package dependencies in the UI project.
- Native SevenZip source integration (`SevenZip/` project and `SZReader`) has been removed.
- Embedded third-party source trees should be avoided when package-based or framework-based alternatives are viable.

## Important Clarification

Creating ZIP files does not imply 7z output support.

- ZIP creation in Nexen uses ZIP tooling (`miniz`, `Compress-Archive`, or `zip` in platform-specific build steps).
- 7z creation requires separate tooling. In Nexen's packaging script, that path is explicit and optional.

## Usage Examples

```powershell
# Standard embedded payload for UI build/publish
.\scripts\package-dependencies.ps1 -Configuration Release -ArchiveFormat zip

# Also generate a sidecar .7z for distribution/testing workflows
.\scripts\package-dependencies.ps1 -Configuration Release -ArchiveFormat both
```

## Related Files

- `UI/Utilities/ArchiveHelper.cs`
- `UI/Utilities/LoadRomHelper.cs`
- `Utilities/ArchiveReader.cpp`
- `Utilities/ZipReader.cpp`
- `Utilities/ZipWriter.cpp`
- `scripts/package-dependencies.ps1`
- `UI/UI.csproj`
