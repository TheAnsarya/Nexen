param(
	[string]$RepoRoot = (Get-Location).Path
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

$RepoRoot = (Resolve-Path $RepoRoot).Path
$targetDir = Join-Path $RepoRoot "UI/Assets/Proposed/ColorBold"
New-Item -ItemType Directory -Force -Path $targetDir | Out-Null
$defaultScale = 1.08

# Current icon -> MDI source icon + color
$map = @(
	@{ Current = "Help.png"; Mdi = "help-circle"; Color = "#1e88e5"; WhiteGlyph = $true; Scale = 1.08 },
	@{ Current = "Warning.png"; Mdi = "alert"; Color = "#f9a825" },
	@{ Current = "Error.png"; Mdi = "alert-circle"; Color = "#e53935" },
	@{ Current = "Record.png"; Mdi = "record-circle"; Color = "#d32f2f"; Scale = 1.08 },
	@{ Current = "MediaPlay.png"; Mdi = "play"; Color = "#2e7d32" },
	@{ Current = "MediaPause.png"; Mdi = "pause"; Color = "#fdd835" },
	@{ Current = "MediaStop.png"; Mdi = "stop"; Color = "#c62828" },
	@{ Current = "Close.png"; Mdi = "close-thick"; Color = "#d32f2f"; Scale = 1.18 },
	@{ Current = "Settings.png"; Mdi = "cog"; Color = "#546e7a" },
	@{ Current = "Find.png"; Mdi = "magnify"; Color = "#00838f" },
	@{ Current = "Refresh.png"; Mdi = "refresh"; Color = "#1565c0" },
	@{ Current = "ResetSettings.png"; Mdi = "backup-restore"; Color = "#ab47bc" },
	@{ Current = "Folder.png"; Mdi = "folder"; Color = "#fbc02d" },
	@{ Current = "Script.png"; Mdi = "script-text"; Color = "#3949ab" },
	@{ Current = "LogWindow.png"; Mdi = "file-document-outline"; Color = "#455a64" },
	@{ Current = "Debugger.png"; Mdi = "bug"; Color = "#2e7d32" },
	@{ Current = "Speed.png"; Mdi = "speedometer"; Color = "#00897b" },
	@{ Current = "HistoryViewer.png"; Mdi = "history"; Color = "#6d4c41" },
	@{ Current = "VideoOptions.png"; Mdi = "video-outline"; Color = "#1976d2" },
	@{ Current = "Repeat.png"; Mdi = "repeat"; Color = "#0277bd" },
	@{ Current = "Add.png"; Mdi = "plus-thick"; Color = "#2e7d32" },
	@{ Current = "Edit.png"; Mdi = "pencil"; Color = "#ef6c00" },
	@{ Current = "Copy.png"; Mdi = "content-copy"; Color = "#3949ab" },
	@{ Current = "Paste.png"; Mdi = "content-paste"; Color = "#6d4c41" },
	@{ Current = "SaveFloppy.png"; Mdi = "content-save"; Color = "#1565c0" },
	@{ Current = "Undo.png"; Mdi = "undo"; Color = "#6d4c41" },
	@{ Current = "Reload.png"; Mdi = "reload"; Color = "#1e88e5" },
	@{ Current = "NextArrow.png"; Mdi = "chevron-right"; Color = "#455a64"; Scale = 1.12 },
	@{ Current = "PreviousArrow.png"; Mdi = "chevron-left"; Color = "#455a64"; Scale = 1.12 },
	@{ Current = "Function.png"; Mdi = "function-variant"; Color = "#8e24aa" },
	@{ Current = "Enum.png"; Mdi = "format-list-numbered"; Color = "#00838f" },
	@{ Current = "Drive.png"; Mdi = "harddisk"; Color = "#546e7a" },
	@{ Current = "CheatCode.png"; Mdi = "code-tags"; Color = "#5e35b1" },
	@{ Current = "EditLabel.png"; Mdi = "label-outline"; Color = "#5e35b1" },
	@{ Current = "NesIcon.png"; Mdi = "custom-nes"; Color = "#e53935" },
	@{ Current = "SnesIcon.png"; Mdi = "custom-snes"; Color = "#8e24aa" },
	@{ Current = "GameboyIcon.png"; Mdi = "custom-gameboy"; Color = "#43a047" },
	@{ Current = "GbaIcon.png"; Mdi = "custom-gba"; Color = "#7e57c2" },
	@{ Current = "PceIcon.png"; Mdi = "custom-pce"; Color = "#00897b" },
	@{ Current = "SmsIcon.png"; Mdi = "custom-sms"; Color = "#fb8c00" },
	@{ Current = "WsIcon.png"; Mdi = "custom-ws"; Color = "#546e7a" },
	@{ Current = "LynxIcon.png"; Mdi = "paw"; Color = "#6d4c41" }
)

function GetCustomPlatformSvg([string]$currentName) {
	switch ($currentName) {
		"NesIcon.png" {
			return @"
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">
	<rect x="2.8" y="5" width="18.4" height="13.8" rx="1.6" fill="#cfd8dc"/>
	<rect x="2.8" y="5" width="18.4" height="2.4" rx="1.6" fill="#b0bec5"/>
	<rect x="4.1" y="8.4" width="15.8" height="7.9" rx="0.9" fill="#eceff1"/>
	<rect x="4.9" y="9.5" width="5.4" height="0.9" rx="0.3" fill="#d32f2f"/>
	<rect x="4.9" y="11" width="7.9" height="0.95" rx="0.3" fill="#263238"/>
	<rect x="14" y="9.3" width="5" height="5.1" rx="0.75" fill="#b0bec5"/>
	<circle cx="15.5" cy="12.9" r="0.65" fill="#78909c"/>
	<circle cx="17.7" cy="12.9" r="0.65" fill="#78909c"/>
</svg>
"@
		}
		"SnesIcon.png" {
			return @"
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">
	<ellipse cx="12" cy="7.2" rx="3.55" ry="2.2" fill="#3d5afe" transform="rotate(-18 12 7.2)"/>
	<ellipse cx="16.4" cy="11.4" rx="3.55" ry="2.2" fill="#e53935" transform="rotate(18 16.4 11.4)"/>
	<ellipse cx="12" cy="15.6" rx="3.55" ry="2.2" fill="#fdd835" transform="rotate(-18 12 15.6)"/>
	<ellipse cx="7.6" cy="11.4" rx="3.55" ry="2.2" fill="#43a047" transform="rotate(18 7.6 11.4)"/>
</svg>
"@
		}
		"GameboyIcon.png" {
			return @"
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">
	<rect x="4.3" y="1" width="15.4" height="22" rx="2.6" fill="#cfd8dc"/>
	<rect x="5.9" y="3.1" width="12.2" height="8.6" rx="0.85" fill="#7cb342"/>
	<rect x="7.8" y="14.1" width="3.4" height="1.05" rx="0.3" fill="#455a64"/>
	<rect x="8.98" y="12.92" width="1.05" height="3.4" rx="0.3" fill="#455a64"/>
	<ellipse cx="14.9" cy="14.6" rx="1.05" ry="0.82" fill="#8e24aa" transform="rotate(-25 14.9 14.6)"/>
	<ellipse cx="16.95" cy="16.1" rx="1.05" ry="0.82" fill="#8e24aa" transform="rotate(-25 16.95 16.1)"/>
	<rect x="8.2" y="18.55" width="2.95" height="0.58" rx="0.25" fill="#78909c"/>
	<rect x="13.05" y="18.55" width="2.95" height="0.58" rx="0.25" fill="#78909c"/>
</svg>
"@
		}
		"GbaIcon.png" {
			return @"
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">
	<path d="M3.4 9.2C3.9 7.5 5.3 6.3 7.2 6.3H16.8C18.7 6.3 20.1 7.5 20.6 9.2L21.4 12.2C21.9 14.2 20.4 16.2 18.3 16.2H5.7C3.6 16.2 2.1 14.2 2.6 12.2L3.4 9.2Z" fill="#7e57c2"/>
	<rect x="6.95" y="8.45" width="10.1" height="5.55" rx="0.9" fill="#4527a0"/>
	<rect x="4.55" y="11.02" width="2.35" height="0.9" rx="0.3" fill="#ede7f6"/>
	<rect x="5.27" y="10.3" width="0.9" height="2.35" rx="0.3" fill="#ede7f6"/>
	<circle cx="17.55" cy="10.95" r="0.78" fill="#d1c4e9"/>
	<circle cx="19.05" cy="12.45" r="0.78" fill="#d1c4e9"/>
	<rect x="8.15" y="15.05" width="7.7" height="0.62" rx="0.3" fill="#5e35b1"/>
	<rect x="4.35" y="7.12" width="2.7" height="0.6" rx="0.3" fill="#5e35b1"/>
	<rect x="16.95" y="7.12" width="2.7" height="0.6" rx="0.3" fill="#5e35b1"/>
</svg>
"@
		}
		"PceIcon.png" {
			return @"
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">
	<rect x="3" y="8" width="18" height="9.8" rx="2" fill="#f5f5f5"/>
	<rect x="4.1" y="9.1" width="15.8" height="7.5" rx="1.1" fill="#eceff1"/>
	<rect x="5.2" y="10.2" width="9.6" height="0.82" rx="0.3" fill="#90a4ae"/>
	<rect x="5.2" y="11.75" width="7.7" height="0.82" rx="0.3" fill="#90a4ae"/>
	<path d="M16 10L18.9 10" stroke="#ef6c00" stroke-width="1.25" stroke-linecap="round"/>
	<path d="M15.4 12.1L18.3 12.1" stroke="#ef6c00" stroke-width="1.25" stroke-linecap="round"/>
	<rect x="14.7" y="6.1" width="5.1" height="2.2" rx="0.55" fill="#ffe0b2"/>
	<rect x="15.35" y="6.72" width="3.8" height="0.54" rx="0.25" fill="#fb8c00"/>
</svg>
"@
		}
		"SmsIcon.png" {
			return @"
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">
	<rect x="2.3" y="5.5" width="19.4" height="13" rx="1.8" fill="#1f1f1f"/>
	<rect x="3.7" y="7" width="16.6" height="10" rx="1.2" fill="#30343a"/>
	<circle cx="8.2" cy="12.1" r="3.15" fill="#222831"/>
	<circle cx="8.2" cy="12.1" r="2.25" fill="none" stroke="#e53935" stroke-width="1.15"/>
	<circle cx="8.2" cy="12.1" r="1.35" fill="none" stroke="#fb8c00" stroke-width="0.9"/>
	<rect x="13.2" y="8.45" width="5.8" height="0.85" rx="0.3" fill="#9e9e9e"/>
	<circle cx="15.6" cy="12.35" r="0.72" fill="#eceff1"/>
	<circle cx="17.85" cy="12.35" r="0.72" fill="#eceff1"/>
</svg>
"@
		}
		"WsIcon.png" {
			return @"
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">
	<rect x="2.7" y="6" width="18.6" height="11.8" rx="2.1" fill="#f3f6f8"/>
	<rect x="7.5" y="8.15" width="9" height="5.7" rx="0.7" fill="#90caf9"/>
	<rect x="4.45" y="10.55" width="2.3" height="0.85" rx="0.3" fill="#546e7a"/>
	<rect x="5.18" y="9.82" width="0.85" height="2.3" rx="0.3" fill="#546e7a"/>
	<rect x="17.2" y="10.55" width="2.3" height="0.85" rx="0.3" fill="#546e7a"/>
	<rect x="17.93" y="9.82" width="0.85" height="2.3" rx="0.3" fill="#546e7a"/>
	<circle cx="10.8" cy="15.65" r="0.55" fill="#78909c"/>
	<circle cx="12" cy="15.65" r="0.55" fill="#78909c"/>
	<circle cx="13.2" cy="15.65" r="0.55" fill="#78909c"/>
</svg>
"@
		}
		default {
			return $null
		}
	}
}

$results = New-Object System.Collections.Generic.List[object]

foreach ($entry in $map) {
	$mdiName = [string]$entry.Mdi
	$currentName = [string]$entry.Current
	$color = [string]$entry.Color
	$svgName = [System.IO.Path]::GetFileNameWithoutExtension($currentName) + ".svg"
	$outPath = Join-Path $targetDir $svgName
	$customSvg = GetCustomPlatformSvg $currentName
	if (-not [string]::IsNullOrWhiteSpace($customSvg)) {
		Set-Content -Path $outPath -Value $customSvg -NoNewline

		$results.Add([pscustomobject]@{
			Current = $currentName
			Mdi = $mdiName
			Color = $color
			Output = $outPath
			Status = "ok"
		}) | Out-Null
		continue
	}

	$url = "https://raw.githubusercontent.com/Templarian/MaterialDesign/master/svg/$mdiName.svg"
	$scale = if ($entry.ContainsKey("Scale")) { [double]$entry.Scale } else { $defaultScale }
	$scaleText = $scale.ToString([System.Globalization.CultureInfo]::InvariantCulture)

	try {
		$raw = (Invoke-WebRequest -Uri $url -TimeoutSec 30).Content
		if (-not $raw) {
			throw "Empty SVG payload"
		}

		if ($entry.ContainsKey("WhiteGlyph") -and $entry.WhiteGlyph -eq $true) {
			$glyphRaw = (Invoke-WebRequest -Uri "https://raw.githubusercontent.com/Templarian/MaterialDesign/master/svg/help.svg" -TimeoutSec 30).Content
			$glyphPath = [regex]::Match($glyphRaw, '<path[^>]*>').Value
			$glyphPath = $glyphPath -replace 'fill="[^"]*"', ''
			if (-not $glyphPath) {
				throw "Failed to build help glyph"
			}
			$glyphPath = $glyphPath -replace '<path ', '<path fill="#ffffff" '

			$svg = @"
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">
	<circle cx="12" cy="12" r="10.4" fill="$color"/>
	<g transform="translate(12 12) scale(0.78) translate(-12 -12)">
		$glyphPath
	</g>
</svg>
"@
		} else {
			# Ensure bold/color appearance for previews.
			$svg = $raw -replace '<svg\s+', ("<svg fill=`"" + $color + "`" ")
			$svg = $svg -replace '<path ', ("<path transform=`"translate(12 12) scale(" + $scaleText + ") translate(-12 -12)`" ")
		}

		Set-Content -Path $outPath -Value $svg -NoNewline

		$results.Add([pscustomobject]@{
			Current = $currentName
			Mdi = $mdiName
			Color = $color
			Output = $outPath
			Status = "ok"
		}) | Out-Null
	} catch {
		$results.Add([pscustomobject]@{
			Current = $currentName
			Mdi = $mdiName
			Color = $color
			Output = $outPath
			Status = "failed"
		}) | Out-Null
	}
}

$licenseText = @"
Material Design Icons
https://github.com/Templarian/MaterialDesign
License: Apache-2.0
"@
Set-Content -Path (Join-Path $targetDir "LICENSE-MDI.txt") -Value $licenseText -NoNewline

$results | Sort-Object Current
