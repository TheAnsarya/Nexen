# Capture DiztinGUIsh Debug Output
# This script runs DiztinGUIsh and captures all debug output to a log file

$logFile = "C:\Users\me\source\repos\Mesen2\~docs\diztinguish_debug.log"
$exePath = "C:\Users\me\source\repos\DiztinGUIsh\Diz.App.Winforms\bin\Debug\net9.0-windows\Diz.App.Winforms.exe"

Write-Host "=== DiztinGUIsh Debug Logger ===" -ForegroundColor Cyan
Write-Host "Executable: $exePath" -ForegroundColor Gray
Write-Host "Log file: $logFile" -ForegroundColor Gray
Write-Host ""
Write-Host "Instructions:" -ForegroundColor Yellow
Write-Host "  1. DiztinGUIsh will start in a moment" -ForegroundColor Yellow
Write-Host "  2. Open or create a project" -ForegroundColor Yellow
Write-Host "  3. Press Ctrl+F6 or menu → Connect to Mesen2" -ForegroundColor Yellow
Write-Host "  4. Watch this window for debug output" -ForegroundColor Yellow
Write-Host "  5. All output will be saved to: $logFile" -ForegroundColor Yellow
Write-Host ""
Write-Host "Starting DiztinGUIsh..." -ForegroundColor Green
Write-Host ""

# Clear old log
if (Test-Path $logFile) {
    Remove-Item $logFile
}

# Start the process and capture output
$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = $exePath
$psi.UseShellExecute = $false
$psi.RedirectStandardOutput = $true
$psi.RedirectStandardError = $true
$psi.CreateNoWindow = $false

$process = New-Object System.Diagnostics.Process
$process.StartInfo = $psi

# Event handlers for output
$outputHandler = {
    if (-not [string]::IsNullOrEmpty($EventArgs.Data)) {
        $timestamp = Get-Date -Format "HH:mm:ss.fff"
        $line = "[$timestamp] $($EventArgs.Data)"
        Write-Host $line -ForegroundColor Cyan
        Add-Content -Path $using:logFile -Value $line
    }
}

$errorHandler = {
    if (-not [string]::IsNullOrEmpty($EventArgs.Data)) {
        $timestamp = Get-Date -Format "HH:mm:ss.fff"
        $line = "[$timestamp] ERROR: $($EventArgs.Data)"
        Write-Host $line -ForegroundColor Red
        Add-Content -Path $using:logFile -Value $line
    }
}

$process.add_OutputDataReceived($outputHandler)
$process.add_ErrorDataReceived($errorHandler)

$process.Start() | Out-Null
$process.BeginOutputReadLine()
$process.BeginErrorReadLine()

Write-Host "DiztinGUIsh started (PID: $($process.Id))" -ForegroundColor Green
Write-Host "Waiting for output... (debug messages will appear here)" -ForegroundColor Gray
Write-Host ""

# Wait for process to exit
$process.WaitForExit()

Write-Host ""
Write-Host "DiztinGUIsh closed." -ForegroundColor Yellow
Write-Host "Log saved to: $logFile" -ForegroundColor Green
