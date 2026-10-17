# Build and Run Nexen

## Quick Build & Run (Development)

The output goes to `C:\bin\win-x64\Release\` (outside the repo) and requires the native `NexenCore.dll` to be present.

```powershell
# 1. Clean output folder
Remove-Item "C:\bin\win-x64\Release\*" -Recurse -Force -ErrorAction SilentlyContinue

# 2. Build UI (outputs to C:\bin\win-x64\Release\)
dotnet build c:\Users\me\source\repos\Nexen\UI\UI.csproj -c Release

# 3. Copy native core DLL (from repo bin folder)
Copy-Item "c:\Users\me\source\repos\Nexen\bin\win-x64\Release\NexenCore.dll" "C:\bin\win-x64\Release\" -Force

# 4. Launch
Start-Process "C:\bin\win-x64\Release\Nexen.exe"
```

## One-liner

```powershell
Remove-Item "C:\bin\win-x64\Release\*" -Recurse -Force -ErrorAction SilentlyContinue; dotnet build c:\Users\me\source\repos\Nexen\UI\UI.csproj -c Release; Copy-Item "c:\Users\me\source\repos\Nexen\bin\win-x64\Release\NexenCore.dll" "C:\bin\win-x64\Release\" -Force; Start-Process "C:\bin\win-x64\Release\Nexen.exe"
```

## Notes

- The C# UI outputs to `C:\bin\win-x64\Release\` (configured in UI.csproj)
- The native `NexenCore.dll` is built separately with Visual Studio/MSBuild
- The native core DLL exists in `c:\Users\me\source\repos\Nexen\bin\win-x64\Release\`
- Always clean the Release folder before rebuilding to avoid stale files
