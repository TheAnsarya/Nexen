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
	@{ Current = "Accept.png"; Mdi = "check-bold"; Color = "#2e7d32" },
	@{ Current = "Exclamation.png"; Mdi = "alert-circle"; Color = "#f9a825" },
	@{ Current = "Expand.png"; Mdi = "arrow-expand-all"; Color = "#1565c0" },
	@{ Current = "Export.png"; Mdi = "export"; Color = "#00838f" },
	@{ Current = "Import.png"; Mdi = "import"; Color = "#00838f" },
	@{ Current = "SelectAll.png"; Mdi = "select-all"; Color = "#546e7a" },
	@{ Current = "Update.png"; Mdi = "update"; Color = "#1e88e5" },
	@{ Current = "Camera.png"; Mdi = "camera"; Color = "#6d4c41" },
	@{ Current = "Microphone.png"; Mdi = "microphone"; Color = "#8e24aa" },
	@{ Current = "WebBrowser.png"; Mdi = "web"; Color = "#1565c0" },
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
	<rect x="2.4" y="7.1" width="19.2" height="9.8" rx="1.5" fill="#d8dde1"/>
	<rect x="3.2" y="7.9" width="17.6" height="8.2" rx="1.1" fill="#b0bec5"/>
	<rect x="8.4" y="8.8" width="7.2" height="6.4" rx="0.7" fill="#263238"/>
	<rect x="4.5" y="11.2" width="2.8" height="0.95" rx="0.3" fill="#263238"/>
	<rect x="5.43" y="10.27" width="0.95" height="2.8" rx="0.3" fill="#263238"/>
	<rect x="9.2" y="11.1" width="2.2" height="0.7" rx="0.3" fill="#90a4ae"/>
	<rect x="12.6" y="11.1" width="2.2" height="0.7" rx="0.3" fill="#90a4ae"/>
	<circle cx="17.3" cy="10.75" r="1.02" fill="#d32f2f"/>
	<circle cx="18.8" cy="12.35" r="1.02" fill="#d32f2f"/>
</svg>
"@
		}
		"SnesIcon.png" {
			return @"
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">
	<defs>
		<mask id="snesBlueCut">
			<rect x="0" y="0" width="24" height="24" fill="#ffffff"/>
			<ellipse cx="13.9" cy="9.1" rx="1.05" ry="0.72" transform="rotate(-18 13.9 9.1)" fill="#000000"/>
		</mask>
		<mask id="snesRedCut">
			<rect x="0" y="0" width="24" height="24" fill="#ffffff"/>
			<ellipse cx="14.8" cy="13.8" rx="1.05" ry="0.72" transform="rotate(18 14.8 13.8)" fill="#000000"/>
		</mask>
	</defs>
	<ellipse cx="12" cy="7.05" rx="3.65" ry="2.25" fill="#3d5afe" transform="rotate(-18 12 7.05)" mask="url(#snesBlueCut)"/>
	<ellipse cx="16.45" cy="11.45" rx="3.65" ry="2.25" fill="#e53935" transform="rotate(18 16.45 11.45)" mask="url(#snesRedCut)"/>
	<ellipse cx="12" cy="15.8" rx="3.65" ry="2.25" fill="#fdd835" transform="rotate(-18 12 15.8)"/>
	<ellipse cx="7.55" cy="11.45" rx="3.65" ry="2.25" fill="#43a047" transform="rotate(18 7.55 11.45)"/>
</svg>
"@
		}
		"GameboyIcon.png" {
			return @"
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">
	<rect x="4" y="0.8" width="16" height="22.4" rx="2.7" fill="#cfd8dc"/>
	<rect x="5.7" y="3" width="12.6" height="8.8" rx="0.9" fill="#7cb342"/>
	<rect x="7.7" y="14" width="3.45" height="1.08" rx="0.3" fill="#455a64"/>
	<rect x="8.88" y="12.82" width="1.08" height="3.45" rx="0.3" fill="#455a64"/>
	<ellipse cx="15" cy="14.5" rx="1.07" ry="0.84" fill="#8e24aa" transform="rotate(-25 15 14.5)"/>
	<ellipse cx="17.08" cy="16.02" rx="1.07" ry="0.84" fill="#8e24aa" transform="rotate(-25 17.08 16.02)"/>
	<rect x="8.15" y="18.55" width="3" height="0.6" rx="0.25" fill="#78909c"/>
	<rect x="13.08" y="18.55" width="3" height="0.6" rx="0.25" fill="#78909c"/>
</svg>
"@
		}
		"GbaIcon.png" {
			return @"
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">
	<path d="M4 8.7C4.5 7 5.8 6 7.7 6H16.3C18.2 6 19.5 7 20 8.7L20.9 12C21.4 13.9 20 15.8 18 15.8H6C4 15.8 2.6 13.9 3.1 12L4 8.7Z" fill="#7e57c2"/>
	<rect x="6.6" y="8.2" width="10.8" height="5.9" rx="0.95" fill="#4527a0"/>
	<rect x="4.6" y="10.95" width="2.25" height="0.85" rx="0.3" fill="#ede7f6"/>
	<rect x="5.3" y="10.25" width="0.85" height="2.25" rx="0.3" fill="#ede7f6"/>
	<circle cx="17.65" cy="10.95" r="0.76" fill="#d1c4e9"/>
	<circle cx="19.1" cy="12.35" r="0.76" fill="#d1c4e9"/>
	<rect x="8" y="14.85" width="8" height="0.6" rx="0.3" fill="#5e35b1"/>
	<rect x="4.45" y="6.7" width="2.55" height="0.58" rx="0.28" fill="#5e35b1"/>
	<rect x="17" y="6.7" width="2.55" height="0.58" rx="0.28" fill="#5e35b1"/>
</svg>
"@
		}
		"PceIcon.png" {
			return @"
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">
	<rect x="2.7" y="7.7" width="18.6" height="10.2" rx="2" fill="#f4f4f4"/>
	<rect x="3.9" y="8.9" width="16.2" height="7.8" rx="1.1" fill="#eceff1"/>
	<rect x="4.8" y="10" width="9.6" height="0.8" rx="0.3" fill="#90a4ae"/>
	<rect x="4.8" y="11.5" width="8.2" height="0.8" rx="0.3" fill="#90a4ae"/>
	<rect x="14.9" y="9.6" width="4.2" height="0.75" rx="0.3" fill="#fb8c00"/>
	<rect x="14.4" y="11.1" width="4.7" height="0.75" rx="0.3" fill="#fb8c00"/>
	<rect x="13.8" y="6" width="6" height="1.9" rx="0.5" fill="#ffe0b2"/>
	<rect x="14.45" y="6.57" width="4.7" height="0.52" rx="0.25" fill="#fb8c00"/>
</svg>
"@
		}
		"SmsIcon.png" {
			return @"
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">
	<rect x="2.2" y="5.2" width="19.6" height="13.6" rx="1.8" fill="#1f1f1f"/>
	<rect x="3.5" y="6.7" width="17" height="10.6" rx="1.2" fill="#2f343a"/>
	<circle cx="8.4" cy="12" r="3.35" fill="#222831"/>
	<circle cx="8.4" cy="12" r="2.4" fill="none" stroke="#e53935" stroke-width="1.2"/>
	<circle cx="8.4" cy="12" r="1.35" fill="none" stroke="#fb8c00" stroke-width="0.92"/>
	<rect x="12.9" y="8.3" width="6.2" height="0.9" rx="0.3" fill="#9ea5ad"/>
	<rect x="12.9" y="9.75" width="3.8" height="0.55" rx="0.25" fill="#74808c"/>
	<circle cx="15.7" cy="12.45" r="0.73" fill="#eceff1"/>
	<circle cx="18" cy="12.45" r="0.73" fill="#eceff1"/>
</svg>
"@
		}
		"WsIcon.png" {
			return @"
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">
	<rect x="4.2" y="2.6" width="15.6" height="18.8" rx="2.2" fill="#f3f6f8"/>
	<rect x="5.9" y="4.3" width="12.2" height="8.2" rx="0.85" fill="#90caf9"/>
	<rect x="6.8" y="14.2" width="2.55" height="0.92" rx="0.3" fill="#546e7a"/>
	<rect x="7.62" y="13.38" width="0.92" height="2.55" rx="0.3" fill="#546e7a"/>
	<rect x="14.65" y="14.2" width="2.55" height="0.92" rx="0.3" fill="#546e7a"/>
	<rect x="15.48" y="13.38" width="0.92" height="2.55" rx="0.3" fill="#546e7a"/>
	<circle cx="10.6" cy="18" r="0.6" fill="#78909c"/>
	<circle cx="12" cy="18" r="0.6" fill="#78909c"/>
	<circle cx="13.4" cy="18" r="0.6" fill="#78909c"/>
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
