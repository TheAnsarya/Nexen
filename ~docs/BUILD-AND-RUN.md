# Build and Run Nexen

## Quick Build & Run (Development)

```powershell
# Clean, rebuild UI, copy core, and launch
Remove-Item "c:\Users\me\source\repos\Nexen\bin\win-x64\Release\*" -Recurse -Force -ErrorAction SilentlyContinue
dotnet build c:\Users\me\source\repos\Nexen\UI\UI.csproj -c Release -o c:\Users\me\source\repos\Nexen\bin\win-x64\Release
Copy-Item "c:\Users\me\source\repos\Nexen\bin\win-x64\Release-Test\NexenCore.dll" -Destination "c:\Users\me\source\repos\Nexen\bin\win-x64\Release\" -Force
Start-Process "c:\Users\me\source\repos\Nexen\bin\win-x64\Release\Nexen.exe"
```

## One-liner

```powershell
Remove-Item "c:\Users\me\source\repos\Nexen\bin\win-x64\Release\*" -Recurse -Force -ErrorAction SilentlyContinue; dotnet build c:\Users\me\source\repos\Nexen\UI\UI.csproj -c Release -o c:\Users\me\source\repos\Nexen\bin\win-x64\Release; Copy-Item "c:\Users\me\source\repos\Nexen\bin\win-x64\Release-Test\NexenCore.dll" -Destination "c:\Users\me\source\repos\Nexen\bin\win-x64\Release\" -Force; Start-Process "c:\Users\me\source\repos\Nexen\bin\win-x64\Release\Nexen.exe"
```

## Notes

- The native `NexenCore.dll` must be present for the app to run
- Use `Release-Test` folder as source for recent native core builds
- Always clean the Release folder before rebuilding to avoid stale files
