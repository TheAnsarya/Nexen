#!/usr/bin/env pwsh
# Format markdown files to fix common linting issues
# Fixes:
# - MD060: Table column style (adds spaces around pipes)
# - MD040: Fenced code blocks without language (adds 'text' as default)
# - MD032: Lists without surrounding blank lines
# - MD031: Fenced code blocks without surrounding blank lines
# - MD058: Tables without surrounding blank lines
# - Converts leading spaces to tabs

param(
	[Parameter(Mandatory = $false)]
	[string]$Path = ".",

	[Parameter(Mandatory = $false)]
	[switch]$DryRun
)

$ErrorActionPreference = "Stop"

function Format-MarkdownFile {
	param([string]$FilePath)

	$content = Get-Content -Path $FilePath -Raw -Encoding UTF8
	$originalContent = $content
	$modified = $false

	# Split into lines for processing
	$lines = $content -split "`r?`n"
	$newLines = @()
	$inCodeBlock = $false
	$codeBlockLanguage = ""

	for ($i = 0; $i -lt $lines.Count; $i++) {
		$line = $lines[$i]
		$prevLine = if ($i -gt 0) { $lines[$i - 1] } else { "" }
		$nextLine = if ($i -lt $lines.Count - 1) { $lines[$i + 1] } else { "" }

		# Track code blocks
		if ($line -match '^```(\w*)') {
			if (-not $inCodeBlock) {
				$inCodeBlock = $true
				$codeBlockLanguage = $matches[1]

				# MD040: Add language to code blocks without one
				if ([string]::IsNullOrWhiteSpace($codeBlockLanguage)) {
					# Try to guess language based on content
					$guessedLang = "text"
					if ($i + 1 -lt $lines.Count) {
						$nextContent = $lines[$i + 1]
						if ($nextContent -match '^\s*(class|namespace|public|private|using|void|int|string|bool|var)\s') {
							$guessedLang = "csharp"
						} elseif ($nextContent -match '^\s*(#include|void|int|char|struct|class|namespace)\s') {
							$guessedLang = "cpp"
						} elseif ($nextContent -match '^\s*(def |import |from |class |if |for |while )') {
							$guessedLang = "python"
						} elseif ($nextContent -match '^\s*(function|const|let|var|=>|export|import)\s') {
							$guessedLang = "javascript"
						} elseif ($nextContent -match '^\s*(\$|Get-|Set-|New-|Remove-|Write-|param)') {
							$guessedLang = "powershell"
						} elseif ($nextContent -match '^\s*(git |cd |ls |mkdir |rm |cp |mv |echo |cat )') {
							$guessedLang = "bash"
						} elseif ($nextContent -match '^\s*<[a-zA-Z]') {
							$guessedLang = "xml"
						} elseif ($nextContent -match '^\s*\{') {
							$guessedLang = "json"
						}
					}
					$line = "``````$guessedLang"
					$modified = $true
				}

				# MD031: Ensure blank line before code block (if not at start or after blank)
				if ($i -gt 0 -and -not [string]::IsNullOrWhiteSpace($prevLine)) {
					$newLines += ""
					$modified = $true
				}
			} else {
				$inCodeBlock = $false
				$codeBlockLanguage = ""
			}
		}

		# Don't modify content inside code blocks (except the fence itself)
		if ($inCodeBlock -and -not ($line -match '^```')) {
			$newLines += $line
			continue
		}

		# MD060: Fix table formatting (add spaces around pipes)
		if ($line -match '^\s*\|.*\|' -and -not $inCodeBlock) {
			# This is a table row
			$originalLine = $line

			# Split by pipe, trim each cell, rejoin with proper spacing
			$cells = $line -split '\|'
			$fixedCells = @()

			foreach ($cell in $cells) {
				$trimmed = $cell.Trim()
				if ($trimmed -eq "" -and $fixedCells.Count -eq 0) {
					# First empty cell (before first pipe)
					$fixedCells += ""
				} elseif ($trimmed -eq "" -and $cell -eq $cells[-1]) {
					# Last empty cell (after last pipe)
					$fixedCells += ""
				} elseif ($trimmed -match '^[-:]+$') {
					# Separator row - keep dashes with spaces
					$fixedCells += " $trimmed "
				} else {
					# Regular cell
					$fixedCells += " $trimmed "
				}
			}

			$line = $fixedCells -join '|'
			# Trim trailing space from last pipe
			$line = $line -replace '\|\s*$', '|'
			# Ensure leading pipe
			if (-not ($line -match '^\|')) {
				$line = "|$line"
			}

			if ($line -ne $originalLine) {
				$modified = $true
			}
		}

		# MD032/MD058: Add blank lines around lists and tables
		# Check if this line is a list item
		$isListItem = $line -match '^\s*[-*+]\s+' -or $line -match '^\s*\d+\.\s+'
		$isTableRow = $line -match '^\s*\|.*\|'
		$prevIsBlank = [string]::IsNullOrWhiteSpace($prevLine)
		$prevIsListItem = $prevLine -match '^\s*[-*+]\s+' -or $prevLine -match '^\s*\d+\.\s+'
		$prevIsTableRow = $prevLine -match '^\s*\|.*\|'
		$prevIsHeading = $prevLine -match '^#+\s'
		$prevIsCodeFence = $prevLine -match '^```'

		# Add blank line before list if needed
		if ($isListItem -and -not $prevIsBlank -and -not $prevIsListItem -and -not $prevIsHeading -and $i -gt 0) {
			$newLines += ""
			$modified = $true
		}

		# Add blank line before table if needed
		if ($isTableRow -and -not $prevIsBlank -and -not $prevIsTableRow -and -not $prevIsHeading -and $i -gt 0) {
			$newLines += ""
			$modified = $true
		}

		$newLines += $line

		# Check if we need to add blank line after list ends
		$nextIsListItem = $nextLine -match '^\s*[-*+]\s+' -or $nextLine -match '^\s*\d+\.\s+'
		$nextIsBlank = [string]::IsNullOrWhiteSpace($nextLine)
		$nextIsTableRow = $nextLine -match '^\s*\|.*\|'

		if ($isListItem -and -not $nextIsListItem -and -not $nextIsBlank -and $i -lt $lines.Count - 1) {
			$newLines += ""
			$modified = $true
		}

		# Add blank line after table ends
		if ($isTableRow -and -not $nextIsTableRow -and -not $nextIsBlank -and $i -lt $lines.Count - 1) {
			$newLines += ""
			$modified = $true
		}

		# MD031: Add blank line after code block
		if ($line -match '^```$' -or ($line -match '^```' -and -not $inCodeBlock)) {
			if (-not $inCodeBlock -and -not $nextIsBlank -and $i -lt $lines.Count - 1) {
				$newLines += ""
				$modified = $true
			}
		}
	}

	# Rejoin lines
	$newContent = $newLines -join "`r`n"

	# Convert leading spaces to tabs (outside code blocks)
	# This is tricky - we need to be careful about code blocks
	$finalLines = $newContent -split "`r?`n"
	$finalOutput = @()
	$inCode = $false

	foreach ($line in $finalLines) {
		if ($line -match '^```') {
			$inCode = -not $inCode
		}

		if (-not $inCode) {
			# Convert leading spaces to tabs (4 spaces = 1 tab)
			$originalLine = $line
			while ($line -match '^(\t*)    (.*)$') {
				$line = $matches[1] + "`t" + $matches[2]
			}
			if ($line -ne $originalLine) {
				$modified = $true
			}
		}

		$finalOutput += $line
	}

	$newContent = $finalOutput -join "`r`n"

	# Ensure file ends with newline
	if (-not $newContent.EndsWith("`n")) {
		$newContent += "`r`n"
	}

	# Remove multiple consecutive blank lines
	$newContent = $newContent -replace "(`r?`n){3,}", "`r`n`r`n"

	if ($modified -or ($newContent -ne $originalContent)) {
		return @{
			Modified = $true
			Content = $newContent
		}
	}

	return @{
		Modified = $false
		Content = $originalContent
	}
}

# Find all markdown files
$mdFiles = Get-ChildItem -Path $Path -Filter "*.md" -Recurse -File |
	Where-Object { $_.FullName -notmatch 'vcpkg_installed|node_modules' }

$totalFiles = $mdFiles.Count
$modifiedCount = 0

Write-Host "Processing $totalFiles markdown files..." -ForegroundColor Cyan

foreach ($file in $mdFiles) {
	$result = Format-MarkdownFile -FilePath $file.FullName

	if ($result.Modified) {
		$modifiedCount++
		if ($DryRun) {
			Write-Host "Would modify: $($file.FullName)" -ForegroundColor Yellow
		} else {
			Set-Content -Path $file.FullName -Value $result.Content -NoNewline -Encoding UTF8
			Write-Host "Modified: $($file.FullName)" -ForegroundColor Green
		}
	}
}

Write-Host ""
if ($DryRun) {
	Write-Host "Dry run complete. $modifiedCount files would be modified." -ForegroundColor Cyan
} else {
	Write-Host "Formatting complete. $modifiedCount files modified." -ForegroundColor Green
}
