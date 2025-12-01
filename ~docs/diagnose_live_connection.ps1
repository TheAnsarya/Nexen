# Diagnostic script to check Mesen2 ↔ DiztinGUIsh live connection

Write-Host "=== Mesen2 ↔ DiztinGUIsh Connection Diagnostics ===" -ForegroundColor Cyan
Write-Host ""

# Check 1: Are both processes running?
Write-Host "[1] Process Check:" -ForegroundColor Yellow
$mesen = Get-Process -Name "Mesen" -ErrorAction SilentlyContinue
$diz = Get-Process -Name "Diz.App.Winforms" -ErrorAction SilentlyContinue

if ($mesen) {
    Write-Host "  ✅ Mesen2 is running (PID: $($mesen.Id))" -ForegroundColor Green
} else {
    Write-Host "  ❌ Mesen2 is NOT running!" -ForegroundColor Red
    exit
}

if ($diz) {
    Write-Host "  ✅ DiztinGUIsh is running (PID: $($diz.Id))" -ForegroundColor Green
} else {
    Write-Host "  ❌ DiztinGUIsh is NOT running!" -ForegroundColor Red
    exit
}

Write-Host ""

# Check 2: Is port 9998 listening?
Write-Host "[2] Server Port Check:" -ForegroundColor Yellow
$listening = Get-NetTCPConnection -LocalPort 9998 -State Listen -ErrorAction SilentlyContinue
if ($listening) {
    Write-Host "  ✅ Port 9998 is LISTENING (server started)" -ForegroundColor Green
} else {
    Write-Host "  ❌ Port 9998 is NOT listening!" -ForegroundColor Red
    Write-Host "     The Lua script may not be running in Mesen2." -ForegroundColor Yellow
    Write-Host "     In Mesen2: Debug → Script Window → Load start_diztinguish_server.lua" -ForegroundColor Yellow
}

Write-Host ""

# Check 3: Is there an established connection?
Write-Host "[3] Connection Check:" -ForegroundColor Yellow
$established = Get-NetTCPConnection -LocalPort 9998 -State Established -ErrorAction SilentlyContinue
if ($established) {
    Write-Host "  ✅ Port 9998 has ESTABLISHED connection" -ForegroundColor Green
    Write-Host "     Remote: $($established.RemoteAddress):$($established.RemotePort)" -ForegroundColor Gray
} else {
    Write-Host "  ❌ No ESTABLISHED connection on port 9998!" -ForegroundColor Red
    Write-Host "     In DiztinGUIsh: Press Ctrl+F6 or menu → Connect to Mesen2" -ForegroundColor Yellow
}

Write-Host ""

# Check 4: Quick connection test
Write-Host "[4] Connection Test:" -ForegroundColor Yellow
try {
    $client = New-Object System.Net.Sockets.TcpClient
    $client.ReceiveTimeout = 1000
    $client.Connect("localhost", 9998)
    
    if ($client.Connected) {
        Write-Host "  ✅ Successfully connected to port 9998" -ForegroundColor Green
        $client.Close()
    }
}
catch {
    Write-Host "  ❌ Cannot connect to port 9998" -ForegroundColor Red
}

Write-Host ""

# Check 5: Look for Mesen2 debug log (if it exists)
Write-Host "[5] Checking for Mesen2 debug output:" -ForegroundColor Yellow
$logPath = "$env:USERPROFILE\Documents\Mesen2\DebugLog.txt"
if (Test-Path $logPath) {
    $recentLogs = Get-Content $logPath -Tail 10 | Where-Object { $_ -match "DiztinGUIsh" }
    if ($recentLogs) {
        Write-Host "  Found recent DiztinGUIsh activity in Mesen2 log:" -ForegroundColor Green
        $recentLogs | ForEach-Object { Write-Host "    $_" -ForegroundColor Gray }
    } else {
        Write-Host "  ⚠️  No recent DiztinGUIsh activity in Mesen2 log" -ForegroundColor Yellow
    }
} else {
    Write-Host "  (No Mesen2 debug log found at $logPath)" -ForegroundColor Gray
}

Write-Host ""
Write-Host "=== Diagnostic Summary ===" -ForegroundColor Cyan

# Provide guidance based on results
if (-not $listening) {
    Write-Host "🔴 ISSUE: Server not started in Mesen2" -ForegroundColor Red
    Write-Host "   FIX: In Mesen2 → Debug → Script Window → Run start_diztinguish_server.lua" -ForegroundColor Yellow
}
elseif (-not $established) {
    Write-Host "🔴 ISSUE: DiztinGUIsh not connected" -ForegroundColor Red
    Write-Host "   FIX: In DiztinGUIsh → Press Ctrl+F6 or menu → Connect to Mesen2" -ForegroundColor Yellow
}
else {
    Write-Host "✅ Connection appears healthy!" -ForegroundColor Green
    Write-Host ""
    Write-Host "If no data is flowing:" -ForegroundColor Yellow
    Write-Host "  1. Make sure a ROM is loaded in Mesen2" -ForegroundColor Cyan
    Write-Host "  2. Make sure the game is UNPAUSED (playing)" -ForegroundColor Cyan
    Write-Host "  3. Check Mesen2 logs for 'Sent ExecTraceBatch' messages" -ForegroundColor Cyan
    Write-Host "  4. Enable Debug → Show Log Window in Mesen2 to see real-time logs" -ForegroundColor Cyan
}

Write-Host ""
