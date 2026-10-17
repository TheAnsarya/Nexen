# package-dependencies.ps1
# Packages NexenCore.dll and other native dependencies into Dependencies.zip
# Run this AFTER building the C++ InteropDLL project and BEFORE building the UI project
#
# Usage: .\scripts\package-dependencies.ps1 [-Configuration Release|Debug] [-ArchiveFormat zip|7z|both]

param(
    [ValidateSet("Release", "Debug")]
    [string]$Configuration = "Release",

    [ValidateSet("zip", "7z", "both")]
    [string]$ArchiveFormat = "zip"
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$outDir = Join-Path $repoRoot "bin\win-x64\$Configuration"
$uiDir = Join-Path $repoRoot "UI"
$depDir = Join-Path $outDir "Dependencies"

Write-Host "Packaging dependencies for $Configuration build..." -ForegroundColor Cyan
Write-Host "  Output dir: $outDir"
Write-Host "  UI dir: $uiDir"
Write-Host "  Archive format: $ArchiveFormat"

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

function Resolve-SevenZipCommand {
    $candidates = @("7z", "7zz", "7za")
    foreach ($candidate in $candidates) {
        $cmd = Get-Command $candidate -ErrorAction SilentlyContinue
        if ($cmd) {
            return $cmd.Source
        }
    }
    return $null
}

$createdArchives = @()

# Create Dependencies.zip when requested
$zipPath = Join-Path $outDir "Dependencies.zip"
$uiZipPath = Join-Path $uiDir "Dependencies.zip"
if ($ArchiveFormat -eq "zip" -or $ArchiveFormat -eq "both") {
    if (Test-Path $zipPath) {
        Remove-Item $zipPath -Force
    }

    Write-Host "  Creating Dependencies.zip..."
    Compress-Archive -Path "$depDir\*" -DestinationPath $zipPath -Force

    # Copy to UI project folder (for embedding)
    Copy-Item $zipPath $uiZipPath -Force
    $createdArchives += $zipPath
}

# Create Dependencies.7z when requested (requires 7-Zip CLI)
$sevenZipPath = Join-Path $outDir "Dependencies.7z"
if ($ArchiveFormat -eq "7z" -or $ArchiveFormat -eq "both") {
    $sevenZipExe = Resolve-SevenZipCommand
    if (-not $sevenZipExe) {
        if ($ArchiveFormat -eq "7z") {
            Write-Error "7-Zip CLI not found. Install 7z/7zz/7za or use -ArchiveFormat zip."
            exit 1
        }

        Write-Warning "7-Zip CLI not found. Skipping Dependencies.7z creation."
    } else {
        if (Test-Path $sevenZipPath) {
            Remove-Item $sevenZipPath -Force
        }

        Write-Host "  Creating Dependencies.7z with $sevenZipExe..."
        Push-Location $depDir
        try {
            & $sevenZipExe a -t7z -mx=9 -y $sevenZipPath .\* | Out-Null
        } finally {
            Pop-Location
        }

        if (-not (Test-Path $sevenZipPath)) {
            Write-Error "Dependencies.7z was not created successfully."
            exit 1
        }

        $createdArchives += $sevenZipPath
    }
}

# Summary
Write-Host ""
if ($createdArchives.Count -gt 0) {
    Write-Host "Dependency archives created successfully!" -ForegroundColor Green
    foreach ($archive in $createdArchives) {
        Write-Host "  - $archive"
    }
} else {
    Write-Warning "No archives were created."
}

if (Test-Path $uiZipPath) {
    Write-Host "  Embedded UI archive: $uiZipPath"
}
Write-Host ""
Write-Host "Contents:"
if (Test-Path $uiZipPath) {
    Expand-Archive -Path $uiZipPath -DestinationPath "$env:TEMP\nexen_deps_check" -Force
    Get-ChildItem "$env:TEMP\nexen_deps_check" -Recurse | Where-Object { -not $_.PSIsContainer } |
        ForEach-Object { Write-Host "  $($_.Name) - $([math]::Round($_.Length/1KB,1)) KB" }
    Remove-Item "$env:TEMP\nexen_deps_check" -Recurse -Force
} else {
    Write-Host "  No Dependencies.zip generated in this run."
}

Write-Host ""
Write-Host "Next step: Build the UI project with 'dotnet build -c $Configuration'" -ForegroundColor Yellow
