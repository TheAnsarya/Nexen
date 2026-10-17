<#
.SYNOPSIS
    Generates Doxygen API documentation for Nexen.

.DESCRIPTION
    This script runs Doxygen to generate API documentation.
    Requires Doxygen to be installed and in PATH.

.PARAMETER Open
    Open the generated documentation in the default browser.

.PARAMETER Clean
    Remove existing documentation before generating.

.EXAMPLE
    .\generate-docs.ps1
    Generates documentation.

.EXAMPLE
    .\generate-docs.ps1 -Open
    Generates documentation and opens it in browser.

.EXAMPLE
    .\generate-docs.ps1 -Clean -Open
    Cleans existing docs, regenerates, and opens in browser.
#>

param(
    [switch]$Open,
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

# Check if Doxygen is available
$doxygen = Get-Command doxygen -ErrorAction SilentlyContinue
if (-not $doxygen) {
    Write-Error @"
Doxygen not found in PATH.

Please install Doxygen:
  - Windows: Download from https://www.doxygen.nl/download.html
  - Or use: choco install doxygen.install (requires admin)
  - Or use: scoop install doxygen

After installation, ensure doxygen.exe is in your PATH.
"@
    exit 1
}

Write-Host "Using Doxygen: $($doxygen.Source)" -ForegroundColor Cyan
Write-Host "Version: $(doxygen --version)" -ForegroundColor Cyan

# Get repository root
$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not (Test-Path "$repoRoot\Doxyfile")) {
    $repoRoot = $PSScriptRoot
}
if (-not (Test-Path "$repoRoot\Doxyfile")) {
    Write-Error "Doxyfile not found. Run this script from the repository root or scripts directory."
    exit 1
}

Push-Location $repoRoot

try {
    # Clean existing documentation if requested
    if ($Clean) {
        Write-Host "`nCleaning existing documentation..." -ForegroundColor Yellow
        if (Test-Path "docs\api\html") {
            Remove-Item -Recurse -Force "docs\api\html"
        }
        if (Test-Path "docs\api\xml") {
            Remove-Item -Recurse -Force "docs\api\xml"
        }
        if (Test-Path "docs\doxygen-warnings.log") {
            Remove-Item -Force "docs\doxygen-warnings.log"
        }
    }

    # Generate documentation
    Write-Host "`nGenerating documentation..." -ForegroundColor Green
    $startTime = Get-Date

    & doxygen Doxyfile

    $elapsed = (Get-Date) - $startTime
    Write-Host "`nDocumentation generated in $($elapsed.TotalSeconds.ToString('F1')) seconds" -ForegroundColor Green

    # Check for warnings
    if (Test-Path "docs\doxygen-warnings.log") {
        $warnings = Get-Content "docs\doxygen-warnings.log"
        if ($warnings.Count -gt 0) {
            Write-Host "`nWarnings: $($warnings.Count)" -ForegroundColor Yellow
            Write-Host "See docs\doxygen-warnings.log for details"
        }
    }

    # Output location
    $htmlPath = Join-Path $repoRoot "docs\api\html\index.html"
    if (Test-Path $htmlPath) {
        Write-Host "`nDocumentation available at:" -ForegroundColor Cyan
        Write-Host "  $htmlPath"

        # Open in browser if requested
        if ($Open) {
            Write-Host "`nOpening in browser..." -ForegroundColor Cyan
            Start-Process $htmlPath
        }
    } else {
        Write-Warning "Documentation index not found at expected location."
    }
}
finally {
    Pop-Location
}
