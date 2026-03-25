param(
	[string]$RepoRoot = (Get-Location).Path
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

$RepoRoot = (Resolve-Path $RepoRoot).Path
$sourceDir = Join-Path $RepoRoot "UI/Assets/Proposed/ColorBold"
$targetDir = Join-Path $RepoRoot "UI/Assets/Proposed/ColorBadge"
New-Item -ItemType Directory -Force -Path $targetDir | Out-Null

if (-not (Test-Path $sourceDir)) {
	throw "ColorBold source directory not found: $sourceDir"
}

$badgeMap = @{
	"Help.svg"       = "#1976d2";
	"Warning.svg"    = "#f9a825";
	"Error.svg"      = "#d32f2f";
	"Record.svg"     = "#c62828";
	"MediaPlay.svg"  = "#2e7d32";
	"MediaPause.svg" = "#ef6c00";
	"MediaStop.svg"  = "#ad1457";
	"Close.svg"      = "#b71c1c";
	"Settings.svg"   = "#546e7a";
	"Find.svg"       = "#00838f";
}

$results = New-Object System.Collections.Generic.List[object]

foreach ($name in $badgeMap.Keys) {
	$inPath = Join-Path $sourceDir $name
	$outPath = Join-Path $targetDir $name
	$bg = $badgeMap[$name]

	if (-not (Test-Path $inPath)) {
		$results.Add([pscustomobject]@{ File = $name; Status = "missing" }) | Out-Null
		continue
	}

	$svgRaw = Get-Content $inPath -Raw
	$symbol = [regex]::Match($svgRaw, '<path[^>]*>').Value
	if (-not $symbol) {
		$results.Add([pscustomobject]@{ File = $name; Status = "no-path" }) | Out-Null
		continue
	}

	$symbol = $symbol -replace 'fill="[^"]*"', 'fill="#ffffff"'
	if ($symbol -notmatch 'fill="#ffffff"') {
		$symbol = $symbol -replace '<path ', '<path fill="#ffffff" '
	}

	$svgOut = @"
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">
	<circle cx="12" cy="12" r="11" fill="$bg"/>
	$symbol
</svg>
"@
	Set-Content -Path $outPath -Value $svgOut -NoNewline
	$results.Add([pscustomobject]@{ File = $name; Status = "ok" }) | Out-Null
}

$licenseText = @"
Derived proposal variants generated from ColorBold candidates.
Primary source: Material Design Icons (Apache-2.0)
"@
Set-Content -Path (Join-Path $targetDir "LICENSE-DERIVED.txt") -Value $licenseText -NoNewline

$results | Sort-Object File
