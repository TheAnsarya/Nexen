# Pansy Package-Only Validation Benchmark (2026-04-15)

## Scope

This checkpoint records local validation and timing snapshots for Nexen package-only consumption of `Pansy.Core`.

## Commands

```powershell
# Managed tests
dotnet test Tests/Nexen.Tests.csproj -c Release --nologo -v q

# Full solution build
& "C:\Program Files\Microsoft Visual Studio\18\Insiders\MSBuild\Current\Bin\MSBuild.exe" `
	Nexen.sln /p:Configuration=Release /p:Platform=x64 /t:Build /m /nologo /v:m
```

## Results

- Managed tests time snapshot: `6.85s`
- Full solution build time snapshot: `28.29s`
- Build/test status: pass

## Notes

- `Pansy.Core` resolution is package-based in managed project files.
- CI workflow updates remove sibling `pansy` checkout to keep package-only assumptions aligned with local behavior.
