# build-custom-roms.ps1 - Assemble custom test ROMs using Poppy
# Requires Poppy CLI to be available (dotnet run --project from poppy repo)
#
# Usage:
#   .\build-custom-roms.ps1 [-PoppyPath <path>] [-OutputRoot <path>]

[CmdletBinding()]
param(
	[string]$PoppyPath = "",
	[string]$OutputRoot = "$PSScriptRoot\custom-roms\bin"
)

$ErrorActionPreference = "Stop"

# Find Poppy CLI
if (-not $PoppyPath) {
	# Try sibling repo
	$sibling = Join-Path (Split-Path $PSScriptRoot -Parent | Split-Path -Parent) "poppy\src\Poppy.CLI"
	if (Test-Path "$sibling\Poppy.CLI.csproj") {
		$PoppyPath = $sibling
		Write-Host "Found Poppy CLI at: $PoppyPath" -ForegroundColor Green
	} else {
		Write-Error "Poppy CLI not found. Specify -PoppyPath or ensure poppy repo is a sibling directory."
		exit 1
	}
}

# Create output directory
New-Item -ItemType Directory -Path $OutputRoot -Force | Out-Null

# Find all .pasm files in custom-roms subdirectories
$sourceRoot = "$PSScriptRoot\custom-roms"
$sourceFiles = Get-ChildItem -Path $sourceRoot -Filter "*.pasm" -Recurse -File

if ($sourceFiles.Count -eq 0) {
	Write-Host "No .pasm source files found in $sourceRoot" -ForegroundColor Yellow
	exit 0
}

Write-Host "Building $($sourceFiles.Count) custom test ROM(s)..." -ForegroundColor Cyan

$failed = 0
foreach ($source in $sourceFiles) {
	$relDir = $source.Directory.Name
	$outDir = Join-Path $OutputRoot $relDir
	New-Item -ItemType Directory -Path $outDir -Force | Out-Null

	$outName = [System.IO.Path]::ChangeExtension($source.Name, ".pce")
	# Determine output extension by system
	switch ($relDir) {
		"pce" { $outName = [System.IO.Path]::ChangeExtension($source.Name, ".pce") }
		"ws" { $outName = [System.IO.Path]::ChangeExtension($source.Name, ".ws") }
		"lynx" { $outName = [System.IO.Path]::ChangeExtension($source.Name, ".lnx") }
		"nes" { $outName = [System.IO.Path]::ChangeExtension($source.Name, ".nes") }
		"snes" { $outName = [System.IO.Path]::ChangeExtension($source.Name, ".sfc") }
		"gb" { $outName = [System.IO.Path]::ChangeExtension($source.Name, ".gb") }
		"gba" { $outName = [System.IO.Path]::ChangeExtension($source.Name, ".gba") }
		default { $outName = [System.IO.Path]::ChangeExtension($source.Name, ".bin") }
	}
	$outFile = Join-Path $outDir $outName

	Write-Host "  [$relDir] $($source.Name) -> $outName" -ForegroundColor White
	try {
		dotnet run --project $PoppyPath -c Release -- assemble $source.FullName -o $outFile 2>&1
		if ($LASTEXITCODE -ne 0) {
			Write-Host "    FAILED (exit code $LASTEXITCODE)" -ForegroundColor Red
			$failed++
		} else {
			$size = (Get-Item $outFile).Length
			Write-Host "    OK ($size bytes)" -ForegroundColor Green
		}
	} catch {
		Write-Host "    ERROR: $_" -ForegroundColor Red
		$failed++
	}
}

Write-Host ""
if ($failed -gt 0) {
	Write-Host "$failed of $($sourceFiles.Count) ROM(s) failed to build." -ForegroundColor Red
	exit 1
} else {
	Write-Host "All $($sourceFiles.Count) ROM(s) built successfully." -ForegroundColor Green
}
