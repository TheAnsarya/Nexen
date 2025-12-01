# Quick test: Is Mesen2 DiztinGUIsh server ACTUALLY running?

$port = 9998

Write-Host "Testing port $port..." -ForegroundColor Cyan

# Method 1: Check with netstat
$listening = netstat -an | Select-String ":$port\s+.*LISTENING"

if ($listening) {
    Write-Host "✅ Port $port IS LISTENING (via netstat)" -ForegroundColor Green
    Write-Host $listening
} else {
    Write-Host "❌ Port $port NOT listening (via netstat)" -ForegroundColor Red
}

Write-Host ""

# Method 2: Check with Get-NetTCPConnection  
$conn = Get-NetTCPConnection -LocalPort $port -State Listen -ErrorAction SilentlyContinue

if ($conn) {
    Write-Host "✅ Port $port IS LISTENING (via PowerShell)" -ForegroundColor Green
    $proc = Get-Process -Id $conn.OwningProcess -ErrorAction SilentlyContinue
    if ($proc) {
        Write-Host "   Process: $($proc.ProcessName) (PID: $($proc.Id))" -ForegroundColor Gray
    }
} else {
    Write-Host "❌ Port $port NOT listening (via PowerShell)" -ForegroundColor Red
    Write-Host ""
    Write-Host "CHECKLIST:" -ForegroundColor Yellow
    Write-Host "1. Is Mesen2 running?" -ForegroundColor White
    Write-Host "2. Is a SNES ROM loaded?" -ForegroundColor White
    Write-Host "3. Did you click Tools → DiztinGUIsh Server → Start Server?" -ForegroundColor White
    Write-Host "4. Is emulation RUNNING (not paused)? Press F5!" -ForegroundColor White
}

Write-Host ""
Write-Host "If server is running, connect from DiztinGUIsh now." -ForegroundColor Cyan
