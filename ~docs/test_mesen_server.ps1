# Test if Mesen2 DiztinGUIsh server is actually listening on port 9998

Write-Host "Testing Mesen2 DiztinGUIsh server connection..." -ForegroundColor Cyan
Write-Host ""

# Test 1: Check if port 9998 is listening
Write-Host "Test 1: Checking if port 9998 is listening..." -ForegroundColor Yellow
$listening = Get-NetTCPConnection -LocalPort 9998 -State Listen -ErrorAction SilentlyContinue

if ($listening) {
    Write-Host "✅ Port 9998 IS listening!" -ForegroundColor Green
    Write-Host "   Process: $($listening.OwningProcess)" -ForegroundColor Gray
    
    # Get process name
    $process = Get-Process -Id $listening.OwningProcess -ErrorAction SilentlyContinue
    if ($process) {
        Write-Host "   Process Name: $($process.ProcessName)" -ForegroundColor Gray
        Write-Host "   Executable: $($process.Path)" -ForegroundColor Gray
    }
} else {
    Write-Host "❌ Port 9998 is NOT listening!" -ForegroundColor Red
    Write-Host ""
    Write-Host "SOLUTION: In Mesen2, you need to:" -ForegroundColor Yellow
    Write-Host "1. Go to Debug → Script Window (or press F7)" -ForegroundColor White
    Write-Host "2. Click 'Load Script' button" -ForegroundColor White
    Write-Host "3. Browse to: c:\Users\me\source\repos\Mesen2\~docs\start_diztinguish_server.lua" -ForegroundColor White
    Write-Host "4. The script will run and start the server" -ForegroundColor White
    Write-Host "5. Check the Mesen2 log window - should see: '✅ DiztinGUIsh server started on port 9998'" -ForegroundColor White
    Write-Host ""
}

Write-Host ""

# Test 2: Try to connect
Write-Host "Test 2: Attempting TCP connection to localhost:9998..." -ForegroundColor Yellow

try {
    $client = New-Object System.Net.Sockets.TcpClient
    $connectTask = $client.ConnectAsync("localhost", 9998)
    
    if ($connectTask.Wait(2000)) {
        Write-Host "✅ Connection successful!" -ForegroundColor Green
        Write-Host "   Server is ready to accept connections" -ForegroundColor Gray
        $client.Close()
    } else {
        Write-Host "❌ Connection timeout!" -ForegroundColor Red
        Write-Host "   Server may not be running" -ForegroundColor Gray
    }
} catch {
    Write-Host "❌ Connection failed: $($_.Exception.Message)" -ForegroundColor Red
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "NEXT STEPS:" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

if (-not $listening) {
    Write-Host "1. Start Mesen2" -ForegroundColor White
    Write-Host "2. Load a SNES ROM" -ForegroundColor White
    Write-Host "3. Open Script Window (Debug → Script Window or F7)" -ForegroundColor White
    Write-Host "4. Load the script: ~docs\start_diztinguish_server.lua" -ForegroundColor White
    Write-Host "5. Start emulation (press F5 or click Run)" -ForegroundColor White
    Write-Host "6. THEN try connecting from DiztinGUIsh" -ForegroundColor White
} else {
    Write-Host "✅ Server is running! You can now connect from DiztinGUIsh" -ForegroundColor Green
}

Write-Host ""
