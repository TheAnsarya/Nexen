# package-dependencies.ps1
# Packages NexenCore.dll and other native dependencies into Dependencies.zip
# Run this AFTER building the C++ InteropDLL project and BEFORE building the UI project
#
# Usage: .\scripts\package-dependencies.ps1 [-Configuration Release|Debug]

param(
    [ValidateSet("Release", "Debug")]
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$outDir = Join-Path $repoRoot "bin\win-x64\$Configuration"
$uiDir = Join-Path $repoRoot "UI"
$depDir = Join-Path $outDir "Dependencies"

Write-Host "Packaging dependencies for $Configuration build..." -ForegroundColor Cyan
Write-Host "  Output dir: $outDir"
Write-Host "  UI dir: $uiDir"

# Verify NexenCore.dll exists
$nexenCorePath = Join-Path $outDir "NexenCore.dll"
if (-not (Test-Path $nexenCorePath)) {
    Write-Error "NexenCore.dll not found at $nexenCorePath. Build the InteropDLL project first."
    exit 1
}

# Verify native libraries exist
$libHarfBuzz = Join-Path $outDir "libHarfBuzzSharp.dll"
$libSkia = Join-Path $outDir "libSkiaSharp.dll"

if (-not (Test-Path $libHarfBuzz)) {
    Write-Error "libHarfBuzzSharp.dll not found. Build the UI project first to restore NuGet packages."
    exit 1
}

if (-not (Test-Path $libSkia)) {
    Write-Error "libSkiaSharp.dll not found. Build the UI project first to restore NuGet packages."
    exit 1
}

# Clean and create Dependencies folder
if (Test-Path $depDir) {
    Remove-Item $depDir -Recurse -Force
}
New-Item -ItemType Directory -Path $depDir -Force | Out-Null

# Copy static dependencies from UI/Dependencies
$uiDepsDir = Join-Path $uiDir "Dependencies"
if (Test-Path $uiDepsDir) {
    Write-Host "  Copying static dependencies from UI/Dependencies..."
    Copy-Item "$uiDepsDir\*" $depDir -Recurse -Force
}

# Copy native libraries
Write-Host "  Copying native libraries..."
Copy-Item $libHarfBuzz $depDir -Force
Copy-Item $libSkia $depDir -Force
Copy-Item $nexenCorePath $depDir -Force

# Create Dependencies.zip
$zipPath = Join-Path $outDir "Dependencies.zip"
if (Test-Path $zipPath) {
    Remove-Item $zipPath -Force
}

Write-Host "  Creating Dependencies.zip..."
Compress-Archive -Path "$depDir\*" -DestinationPath $zipPath -Force

# Copy to UI project folder (for embedding)
$uiZipPath = Join-Path $uiDir "Dependencies.zip"
Copy-Item $zipPath $uiZipPath -Force

# Summary
Write-Host ""
Write-Host "Dependencies.zip created successfully!" -ForegroundColor Green
Write-Host "  Location: $uiZipPath"
Write-Host ""
Write-Host "Contents:"
Expand-Archive -Path $uiZipPath -DestinationPath "$env:TEMP\nexen_deps_check" -Force
Get-ChildItem "$env:TEMP\nexen_deps_check" -Recurse | Where-Object { -not $_.PSIsContainer } | 
    ForEach-Object { Write-Host "  $($_.Name) - $([math]::Round($_.Length/1KB,1)) KB" }
Remove-Item "$env:TEMP\nexen_deps_check" -Recurse -Force

Write-Host ""
Write-Host "Next step: Build the UI project with 'dotnet build -c $Configuration'" -ForegroundColor Yellow
