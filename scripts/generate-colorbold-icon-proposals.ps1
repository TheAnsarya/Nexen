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
	<rect x="3" y="5" width="18" height="14" rx="2.2" fill="#e0e0e0"/>
	<rect x="3" y="5" width="18" height="3.2" rx="2.2" fill="#b0bec5"/>
	<rect x="4.2" y="9.2" width="15.6" height="7.8" rx="1.2" fill="#eceff1"/>
	<rect x="5.8" y="10.6" width="8.2" height="1.1" rx="0.35" fill="#d32f2f"/>
	<rect x="5.8" y="12.6" width="11.8" height="1" rx="0.35" fill="#90a4ae"/>
	<circle cx="15.5" cy="15.2" r="0.95" fill="#546e7a"/>
	<circle cx="18" cy="15.2" r="0.95" fill="#546e7a"/>
</svg>
"@
		}
		"SnesIcon.png" {
			return @"
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">
	<circle cx="12" cy="12" r="9" fill="#eceff1"/>
	<ellipse cx="12" cy="8.4" rx="3.6" ry="2.3" fill="#3d5afe" transform="rotate(-20 12 8.4)"/>
	<ellipse cx="15.6" cy="12" rx="3.6" ry="2.3" fill="#e53935" transform="rotate(20 15.6 12)"/>
	<ellipse cx="12" cy="15.6" rx="3.6" ry="2.3" fill="#43a047" transform="rotate(-20 12 15.6)"/>
	<ellipse cx="8.4" cy="12" rx="3.6" ry="2.3" fill="#fdd835" transform="rotate(20 8.4 12)"/>
	<circle cx="12" cy="12" r="1.2" fill="#eceff1"/>
</svg>
"@
		}
		"GameboyIcon.png" {
			return @"
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">
	<rect x="5" y="1.8" width="14" height="20.4" rx="2.5" fill="#cfd8dc"/>
	<rect x="6.4" y="3.8" width="11.2" height="8" rx="0.8" fill="#7cb342"/>
	<rect x="8" y="14.2" width="3.1" height="1" rx="0.3" fill="#455a64"/>
	<rect x="9.05" y="13.15" width="1" height="3.1" rx="0.3" fill="#455a64"/>
	<ellipse cx="14.5" cy="14.7" rx="1" ry="0.8" fill="#8e24aa" transform="rotate(-25 14.5 14.7)"/>
	<ellipse cx="16.5" cy="16.2" rx="1" ry="0.8" fill="#8e24aa" transform="rotate(-25 16.5 16.2)"/>
	<rect x="8.2" y="18.2" width="2.8" height="0.55" rx="0.25" fill="#78909c"/>
	<rect x="13" y="18.2" width="2.8" height="0.55" rx="0.25" fill="#78909c"/>
</svg>
"@
		}
		"GbaIcon.png" {
			return @"
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">
	<rect x="2" y="6.7" width="20" height="10.6" rx="5.3" fill="#7e57c2"/>
	<rect x="6.8" y="8.5" width="10.4" height="5.6" rx="0.95" fill="#4527a0"/>
	<rect x="4.4" y="10.8" width="2.6" height="0.9" rx="0.3" fill="#ede7f6"/>
	<rect x="5.25" y="9.95" width="0.9" height="2.6" rx="0.3" fill="#ede7f6"/>
	<circle cx="17.8" cy="10.9" r="0.8" fill="#b39ddb"/>
	<circle cx="19.3" cy="12.4" r="0.8" fill="#b39ddb"/>
	<rect x="8.2" y="15.2" width="7.6" height="0.65" rx="0.3" fill="#5e35b1"/>
</svg>
"@
		}
		"PceIcon.png" {
			return @"
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">
	<rect x="2.8" y="6" width="18.4" height="12" rx="2" fill="#f5f5f5"/>
	<rect x="4" y="7.2" width="16" height="9.6" rx="1.2" fill="#eceff1"/>
	<path d="M5.5 11.6H18.5" stroke="#90a4ae" stroke-width="1" stroke-linecap="round"/>
	<path d="M5.5 13.3H15.8" stroke="#90a4ae" stroke-width="1" stroke-linecap="round"/>
	<path d="M16.6 8.4L19 10.7L16.6 13" fill="none" stroke="#ef6c00" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/>
	<circle cx="18.2" cy="14.6" r="0.8" fill="#00897b"/>
</svg>
"@
		}
		"SmsIcon.png" {
			return @"
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">
	<rect x="2.5" y="6" width="19" height="12" rx="2" fill="#212121"/>
	<rect x="4" y="7.2" width="16" height="9.6" rx="1.2" fill="#37474f"/>
	<circle cx="8.4" cy="12" r="3" fill="#263238"/>
	<circle cx="8.4" cy="12" r="2.1" fill="none" stroke="#e53935" stroke-width="1"/>
	<path d="M6.8 10.4H10M6.8 12H10M6.8 13.6H10" stroke="#e53935" stroke-width="0.75" stroke-linecap="round"/>
	<circle cx="15.8" cy="10.8" r="0.75" fill="#fbc02d"/>
	<circle cx="18" cy="13.1" r="0.75" fill="#fbc02d"/>
</svg>
"@
		}
		"WsIcon.png" {
			return @"
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">
	<rect x="5" y="2.8" width="14" height="18.4" rx="2.3" fill="#eceff1"/>
	<rect x="6.3" y="4.5" width="11.4" height="7" rx="0.8" fill="#90caf9"/>
	<rect x="7.9" y="14.2" width="2.8" height="0.95" rx="0.35" fill="#455a64"/>
	<rect x="8.85" y="13.25" width="0.95" height="2.8" rx="0.35" fill="#455a64"/>
	<circle cx="14.6" cy="14.6" r="0.82" fill="#5c6bc0"/>
	<circle cx="16.2" cy="16.1" r="0.82" fill="#5c6bc0"/>
	<circle cx="12" cy="19.1" r="0.62" fill="#78909c"/>
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
