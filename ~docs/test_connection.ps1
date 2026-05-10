# Quick connection test for DiztinGUIsh → Nexen2

Write-Host "Testing connection to Nexen2 DiztinGUIsh server on localhost:9998..." -ForegroundColor Cyan

try {
    $client = New-Object System.Net.Sockets.TcpClient
    $client.ReceiveTimeout = 3000
    $client.SendTimeout = 3000
    
    Write-Host "Attempting connection..." -ForegroundColor Yellow
    $client.Connect("localhost", 9998)
    
    if ($client.Connected) {
        Write-Host "✅ SUCCESS! Connected to Nexen2 on port 9998" -ForegroundColor Green
        Write-Host "The DiztinGUIsh server is running and accepting connections." -ForegroundColor Green
        $client.Close()
    }
}
catch {
    Write-Host "❌ FAILED! Could not connect to port 9998" -ForegroundColor Red
    Write-Host "Error: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host ""
    Write-Host "This means one of:" -ForegroundColor Yellow
    Write-Host "  1. Nexen2 is not running" -ForegroundColor Yellow
    Write-Host "  2. No ROM is loaded in Nexen2" -ForegroundColor Yellow
    Write-Host "  3. The Lua script 'start_diztinguish_server.lua' has NOT been run" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "To fix:" -ForegroundColor Cyan
    Write-Host "  1. Start Nexen2" -ForegroundColor Cyan
    Write-Host "  2. Load a SNES ROM (File → Open)" -ForegroundColor Cyan
    Write-Host "  3. Open Script Window (Debug → Script Window)" -ForegroundColor Cyan
    Write-Host "  4. Load and run: ~docs\start_diztinguish_server.lua" -ForegroundColor Cyan
}
