# View DiztinGUIsh Debug Output in Real-Time
# This monitors the Windows debug output stream for DiztinGUIsh messages

Write-Host "=== Real-Time Debug Output Monitor ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "Monitoring System.Diagnostics.Debug.WriteLine output..." -ForegroundColor Yellow
Write-Host "Looking for messages starting with '[DiztinGUIsh]' or '[MesenLiveTraceClient]'" -ForegroundColor Yellow
Write-Host ""
Write-Host "IMPORTANT: You need DebugView to see this output!" -ForegroundColor Red
Write-Host "Download from: https://learn.microsoft.com/en-us/sysinternals/downloads/debugview" -ForegroundColor Cyan
Write-Host ""
Write-Host "Alternative: Check the console output when starting DiztinGUIsh" -ForegroundColor Yellow
Write-Host ""

# Check if DebugView is available
$dbgView = Get-Process -Name "Dbgview*" -ErrorAction SilentlyContinue
if ($dbgView) {
    Write-Host "✅ DebugView is running (PID: $($dbgView.Id))" -ForegroundColor Green
} else {
    Write-Host "⚠️  DebugView is NOT running" -ForegroundColor Yellow
    Write-Host "   Launch DebugView as Administrator to see debug messages" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "In DebugView, add filters:" -ForegroundColor Cyan
Write-Host "  - Include: *DiztinGUIsh*" -ForegroundColor Gray
Write-Host "  - Include: *MesenLiveTraceClient*" -ForegroundColor Gray
Write-Host ""

# Alternative: Check console log
$logFile = "C:\Users\me\source\repos\Mesen2\~docs\diztinguish_debug.log"
if (Test-Path $logFile) {
    Write-Host "Found existing log file: $logFile" -ForegroundColor Green
    Write-Host "Showing last 20 lines:" -ForegroundColor Gray
    Write-Host ""
    Get-Content $logFile -Tail 20 | ForEach-Object {
        if ($_ -match "DiztinGUIsh|MesenLiveTraceClient") {
            Write-Host $_ -ForegroundColor Cyan
        } elseif ($_ -match "ERROR|Exception|FAILED") {
            Write-Host $_ -ForegroundColor Red
        } else {
            Write-Host $_ -ForegroundColor Gray
        }
    }
} else {
    Write-Host "No log file found yet." -ForegroundColor Yellow
    Write-Host "Run: .\capture_diz_debug.ps1 to start DiztinGUIsh with logging" -ForegroundColor Cyan
}

Write-Host ""
Write-Host "To monitor in real-time, run:" -ForegroundColor Cyan
Write-Host "  Get-Content '$logFile' -Wait -Tail 0" -ForegroundColor Gray
