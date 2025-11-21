# Detailed Mesen2 <-> DiztinGUIsh Connection Test
# This script tests the TCP connection between Mesen2 server and DiztinGUIsh client

Write-Host "=====================================" -ForegroundColor Cyan
Write-Host "Mesen2 Connection Test - Detailed" -ForegroundColor Cyan
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host ""

# Step 1: Check if port 9998 is already in use
Write-Host "[1] Checking if port 9998 is available..." -ForegroundColor Yellow
$portCheck = Get-NetTCPConnection -LocalPort 9998 -ErrorAction SilentlyContinue
if ($portCheck) {
    Write-Host "    WARNING: Port 9998 is already in use by process $($portCheck.OwningProcess)" -ForegroundColor Red
    Write-Host "    Process: $(Get-Process -Id $portCheck.OwningProcess -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Name)" -ForegroundColor Red
} else {
    Write-Host "    ✓ Port 9998 is available" -ForegroundColor Green
}
Write-Host ""

# Step 2: Check firewall rules
Write-Host "[2] Checking Windows Firewall for port 9998..." -ForegroundColor Yellow
$firewallRule = Get-NetFirewallPortFilter -ErrorAction SilentlyContinue | Where-Object {$_.LocalPort -eq 9998}
if ($firewallRule) {
    Write-Host "    ✓ Firewall rule found for port 9998" -ForegroundColor Green
} else {
    Write-Host "    WARNING: No firewall rule found - connection might be blocked" -ForegroundColor Red
    Write-Host "    Run this as Admin to create rule:" -ForegroundColor Yellow
    Write-Host "    New-NetFirewallRule -DisplayName 'Mesen2 DiztinGUIsh' -Direction Inbound -LocalPort 9998 -Protocol TCP -Action Allow" -ForegroundColor Gray
}
Write-Host ""

# Step 3: Test basic TCP connectivity to localhost:9998
Write-Host "[3] Testing basic TCP connection to localhost:9998..." -ForegroundColor Yellow
try {
    $tcpClient = New-Object System.Net.Sockets.TcpClient
    $connectTask = $tcpClient.ConnectAsync("localhost", 9998)
    $timeoutTask = [System.Threading.Tasks.Task]::Delay(3000)
    
    $completedTask = [System.Threading.Tasks.Task]::WaitAny($connectTask, $timeoutTask)
    
    if ($completedTask -eq 0 -and $tcpClient.Connected) {
        Write-Host "    ✓ Successfully connected to localhost:9998!" -ForegroundColor Green
        $tcpClient.Close()
    } else {
        Write-Host "    ✗ Connection FAILED - timeout or refused" -ForegroundColor Red
        Write-Host "    This means Mesen2 server is NOT listening on port 9998" -ForegroundColor Red
    }
    $tcpClient.Dispose()
} catch {
    Write-Host "    ✗ Connection FAILED with exception:" -ForegroundColor Red
    Write-Host "    $_" -ForegroundColor Red
}
Write-Host ""

# Step 4: Instructions
Write-Host "[4] Next Steps:" -ForegroundColor Cyan
Write-Host "    1. Launch Mesen2: c:\Users\me\source\repos\Mesen2\bin\win-x64\Release\Mesen.exe" -ForegroundColor White
Write-Host "    2. Load a ROM in Mesen2" -ForegroundColor White
Write-Host "    3. Open Lua Console (Tools -> Script Window)" -ForegroundColor White
Write-Host "    4. Run: emu.startDiztinguishServer(9998)" -ForegroundColor White
Write-Host "    5. Check Mesen2 log for: '[DiztinGUIsh] Server started successfully on port 9998'" -ForegroundColor White
Write-Host "    6. Re-run this script to verify server is listening" -ForegroundColor White
Write-Host ""
Write-Host "    After server starts, you should see:" -ForegroundColor White
Write-Host "    - '[DiztinGUIsh] Enabled SNES debugger for streaming'" -ForegroundColor Gray
Write-Host "    - '[DiztinGUIsh] Created socket, attempting to bind to port 9998...'" -ForegroundColor Gray
Write-Host "    - '[DiztinGUIsh] Socket bound successfully, calling Listen(1)...'" -ForegroundColor Gray
Write-Host "    - '[DiztinGUIsh] Server thread started, waiting for client connection...'" -ForegroundColor Gray
Write-Host "    - '[DiztinGUIsh] Calling Accept() - this will block until client connects...'" -ForegroundColor Gray
Write-Host ""

Write-Host "=====================================" -ForegroundColor Cyan
Write-Host "Test Complete" -ForegroundColor Cyan
Write-Host "=====================================" -ForegroundColor Cyan
