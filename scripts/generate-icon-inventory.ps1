param(
	[string]$RepoRoot = (Get-Location).Path,
	[string]$OutputPath = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path $RepoRoot).Path
$assetRoot = (Resolve-Path (Join-Path $RepoRoot "UI\Assets")).Path
if (-not (Test-Path $assetRoot)) {
	throw "Asset root not found: $assetRoot"
}

$assetFiles = Get-ChildItem $assetRoot -Recurse -File |
	Where-Object { $_.Extension -in ".png", ".svg", ".ico", ".icns" } |
	Sort-Object FullName

$assetMap = @{}
foreach ($assetFile in $assetFiles) {
	$relative = [System.IO.Path]::GetRelativePath($assetRoot, $assetFile.FullName)
	$key = ("Assets/" + ($relative -replace '\\', '/')).ToLowerInvariant()
	$assetMap[$key] = [PSCustomObject]@{
		AssetPath = "Assets/" + ($relative -replace '\\', '/')
		Extension = $assetFile.Extension.ToLowerInvariant()
		ReferenceCount = 0
		References = New-Object System.Collections.Generic.List[string]
	}
}

$sourceFiles = Get-ChildItem (Join-Path $RepoRoot "UI") -Recurse -File -Include *.axaml,*.cs
$referencePattern = [regex]'(?i)(?:/)?assets/[a-z0-9_./-]+\.(png|svg|ico|icns)'

foreach ($sourceFile in $sourceFiles) {
	$lineNumber = 0
	foreach ($line in Get-Content $sourceFile.FullName) {
		$lineNumber++
		$matches = $referencePattern.Matches($line)
		foreach ($match in $matches) {
			$raw = $match.Value.Replace('\\', '/')
			$normalized = if ($raw.StartsWith('/')) { $raw.Substring(1) } else { $raw }
			$key = $normalized.ToLowerInvariant()
			if ($assetMap.ContainsKey($key)) {
				$entry = $assetMap[$key]
				$entry.ReferenceCount++
				$entry.References.Add("$($sourceFile.FullName):$lineNumber")
			}
		}
	}
}

$usedAssets = @($assetMap.Values | Where-Object { $_.ReferenceCount -gt 0 })
$unusedAssets = @($assetMap.Values | Where-Object { $_.ReferenceCount -eq 0 })

$pngCount = @($assetMap.Values | Where-Object { $_.Extension -eq ".png" }).Count
$svgCount = @($assetMap.Values | Where-Object { $_.Extension -eq ".svg" }).Count
$icoCount = @($assetMap.Values | Where-Object { $_.Extension -eq ".ico" }).Count
$icnsCount = @($assetMap.Values | Where-Object { $_.Extension -eq ".icns" }).Count

$lines = New-Object System.Collections.Generic.List[string]
$lines.Add("# UI Icon Inventory")
$lines.Add("")
$lines.Add("Generated: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')")
$lines.Add("")
$lines.Add("## Summary")
$lines.Add("")
$lines.Add("- Total assets: $($assetMap.Count)")
$lines.Add("- Used assets: $($usedAssets.Count)")
$lines.Add("- Unused assets: $($unusedAssets.Count)")
$lines.Add("- By extension: png=$pngCount, svg=$svgCount, ico=$icoCount, icns=$icnsCount")
$lines.Add("")
$lines.Add("## Top Referenced Assets")
$lines.Add("")
$lines.Add("| Asset | References |")
$lines.Add("|---|---:|")

$topAssets = $assetMap.Values |
	Sort-Object AssetPath |
	Sort-Object ReferenceCount -Descending |
	Select-Object -First 40
foreach ($asset in $topAssets) {
	$lines.Add("| $($asset.AssetPath) | $($asset.ReferenceCount) |")
}

$report = [string]::Join([Environment]::NewLine, $lines)
if (-not [string]::IsNullOrWhiteSpace($OutputPath)) {
	$target = if ([System.IO.Path]::IsPathRooted($OutputPath)) { $OutputPath } else { Join-Path $RepoRoot $OutputPath }
	$targetDir = Split-Path $target -Parent
	if (-not (Test-Path $targetDir)) {
		New-Item -ItemType Directory -Path $targetDir -Force | Out-Null
	}
	Set-Content -Path $target -Value $report -NoNewline
	Write-Host "Inventory report written to $target"
}

$report
