# Comprehensive Mesen2 DiztinGUIsh Integration Test
# This script will test the connection and show exactly what's happening

$ErrorActionPreference = "Stop"

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "Mesen2 → DiztinGUIsh Connection Test" -ForegroundColor Cyan  
Write-Host "========================================`n" -ForegroundColor Cyan

# Step 1: Find Mesen2
Write-Host "[1/6] Locating Mesen2..." -ForegroundColor Yellow
$mesenPath = "c:\Users\me\source\repos\Mesen2\bin\win-x64\Release\Mesen.exe"
if (Test-Path $mesenPath) {
    Write-Host "  ✅ Found: $mesenPath" -ForegroundColor Green
} else {
    Write-Host "  ❌ Mesen.exe not found at $mesenPath" -ForegroundColor Red
    Write-Host "  Build Mesen2 first!" -ForegroundColor Yellow
    exit 1
}

# Step 2: Check if Mesen2 is running
Write-Host "`n[2/6] Checking if Mesen2 is running..." -ForegroundColor Yellow
$mesenProcess = Get-Process -Name "Mesen" -ErrorAction SilentlyContinue
if ($mesenProcess) {
    Write-Host "  ✅ Mesen2 is running (PID: $($mesenProcess.Id))" -ForegroundColor Green
    Write-Host "  ⚠️  Make sure you have:" -ForegroundColor Yellow
    Write-Host "      1. Loaded a SNES ROM" -ForegroundColor White
    Write-Host "      2. Opened Tools → DiztinGUIsh Server" -ForegroundColor White
    Write-Host "      3. Clicked 'Start Server'" -ForegroundColor White
    Write-Host "      4. Started emulation (F5)" -ForegroundColor White
} else {
    Write-Host "  ⚠️  Mesen2 not running, starting it..." -ForegroundColor Yellow
    Start-Process $mesenPath
    Write-Host "  ⏳ Waiting 3 seconds for Mesen2 to start..." -ForegroundColor Yellow
    Start-Sleep -Seconds 3
    Write-Host "  ⚠️  Please:" -ForegroundColor Yellow
    Write-Host "      1. Load a SNES ROM" -ForegroundColor White
    Write-Host "      2. Open Tools → DiztinGUIsh Server" -ForegroundColor White
    Write-Host "      3. Click 'Start Server'" -ForegroundColor White
    Write-Host "      4. Start emulation (F5)" -ForegroundColor White
    Write-Host "  Press Enter when ready..." -ForegroundColor Cyan
    Read-Host
}

# Step 3: Check if port 9998 is listening
Write-Host "`n[3/6] Checking if port 9998 is listening..." -ForegroundColor Yellow
$listening = Get-NetTCPConnection -LocalPort 9998 -State Listen -ErrorAction SilentlyContinue
if ($listening) {
    Write-Host "  ✅ Port 9998 IS LISTENING" -ForegroundColor Green
    Write-Host "  Process ID: $($listening.OwningProcess)" -ForegroundColor White
} else {
    Write-Host "  ❌ Port 9998 NOT listening!" -ForegroundColor Red
    Write-Host "  Please start the server in Mesen2:" -ForegroundColor Yellow
    Write-Host "      Tools → DiztinGUIsh Server → Start Server" -ForegroundColor White
    Write-Host "  Press Enter when you've started the server..." -ForegroundColor Cyan
    Read-Host
    
    # Check again
    $listening = Get-NetTCPConnection -LocalPort 9998 -State Listen -ErrorAction SilentlyContinue
    if (-not $listening) {
        Write-Host "  ❌ Still not listening. Exiting." -ForegroundColor Red
        exit 1
    }
    Write-Host "  ✅ Port 9998 now listening!" -ForegroundColor Green
}

# Step 4: Test connection and handshake
Write-Host "`n[4/6] Testing connection and handshake..." -ForegroundColor Yellow
try {
    $client = New-Object System.Net.Sockets.TcpClient
    Write-Host "  Connecting to localhost:9998..." -ForegroundColor White
    $client.Connect("localhost", 9998)
    Write-Host "  ✅ TCP connection established!" -ForegroundColor Green
    
    $stream = $client.GetStream()
    $stream.ReadTimeout = 5000
    
    # Read message header (5 bytes)
    Write-Host "  Reading message header (5 bytes)..." -ForegroundColor White
    $header = New-Object byte[] 5
    $bytesRead = $stream.Read($header, 0, 5)
    
    if ($bytesRead -eq 0) {
        Write-Host "  ❌ CONNECTION CLOSED BY SERVER (0 bytes read)" -ForegroundColor Red
        Write-Host "" -ForegroundColor Red
        Write-Host "  THIS IS THE BUG!" -ForegroundColor Red -BackgroundColor Yellow
        Write-Host "" -ForegroundColor Red
        Write-Host "  The server accepted the connection but immediately closed it." -ForegroundColor Yellow
        Write-Host "  This means SendHandshake() in DiztinguishBridge.cpp is failing." -ForegroundColor Yellow
        Write-Host "" -ForegroundColor Yellow
        Write-Host "  Check Mesen2 log window for exceptions:" -ForegroundColor Cyan
        Write-Host "    - Look for '[DiztinGUIsh] Error sending message:'" -ForegroundColor White
        Write-Host "    - Look for '[DiztinGUIsh] Error flushing socket:'" -ForegroundColor White
        $client.Close()
        exit 1
    }
    
    if ($bytesRead -lt 5) {
        Write-Host "  ❌ Incomplete header: got $bytesRead bytes, expected 5" -ForegroundColor Red
        $client.Close()
        exit 1
    }
    
    Write-Host "  ✅ Received 5-byte header!" -ForegroundColor Green
    
    # Parse header
    $messageType = $header[0]
    $messageLength = [BitConverter]::ToUInt32($header, 1)
    
    Write-Host "    Message Type: $messageType (should be 1 for Handshake)" -ForegroundColor White
    Write-Host "    Message Length: $messageLength bytes (should be 268 for Handshake)" -ForegroundColor White
    
    if ($messageType -ne 1) {
        Write-Host "  ❌ Wrong message type! Expected 1 (Handshake), got $messageType" -ForegroundColor Red
        $client.Close()
        exit 1
    }
    
    if ($messageLength -ne 268) {
        Write-Host "  ⚠️  Unexpected length! Expected 268, got $messageLength" -ForegroundColor Yellow
    }
    
    # Read payload
    Write-Host "  Reading payload ($messageLength bytes)..." -ForegroundColor White
    $payload = New-Object byte[] $messageLength
    $totalRead = 0
    while ($totalRead -lt $messageLength) {
        $read = $stream.Read($payload, $totalRead, $messageLength - $totalRead)
        if ($read -eq 0) {
            Write-Host "  ❌ Connection closed while reading payload" -ForegroundColor Red
            $client.Close()
            exit 1
        }
        $totalRead += $read
    }
    
    Write-Host "  ✅ Received complete payload ($totalRead bytes)!" -ForegroundColor Green
    
    # Parse handshake
    $protocolMajor = [BitConverter]::ToUInt16($payload, 0)
    $protocolMinor = [BitConverter]::ToUInt16($payload, 2)
    $romChecksum = [BitConverter]::ToUInt32($payload, 4)
    $romSize = [BitConverter]::ToUInt32($payload, 8)
    $romName = [System.Text.Encoding]::ASCII.GetString($payload, 12, 256).TrimEnd([char]0)
    
    Write-Host "`n  ✅✅✅ HANDSHAKE RECEIVED SUCCESSFULLY! ✅✅✅" -ForegroundColor Green -BackgroundColor DarkGreen
    Write-Host "    Protocol: $protocolMajor.$protocolMinor" -ForegroundColor White
    Write-Host "    ROM Checksum: $romChecksum" -ForegroundColor White
    Write-Host "    ROM Size: $romSize" -ForegroundColor White
    Write-Host "    ROM Name: '$romName'" -ForegroundColor White
    
    $client.Close()
    
} catch {
    Write-Host "  ❌ Exception: $_" -ForegroundColor Red
    Write-Host "  Stack: $($_.ScriptStackTrace)" -ForegroundColor Red
    exit 1
}

# Step 5: Find DiztinGUIsh
Write-Host "`n[5/6] Locating DiztinGUIsh..." -ForegroundColor Yellow
$dizPath = "c:\Users\me\source\repos\DiztinGUIsh\Diz.App.Winforms\bin\Debug\net9.0-windows\Diz.App.Winforms.exe"
if (Test-Path $dizPath) {
    Write-Host "  ✅ Found: $dizPath" -ForegroundColor Green
} else {
    Write-Host "  ❌ DiztinGUIsh not found at $dizPath" -ForegroundColor Red
    Write-Host "  Build DiztinGUIsh first!" -ForegroundColor Yellow
    exit 1
}

# Step 6: Start DiztinGUIsh
Write-Host "`n[6/6] Ready to test with DiztinGUIsh!" -ForegroundColor Yellow
Write-Host "  When DiztinGUIsh opens:" -ForegroundColor Cyan
Write-Host "    1. Create or open a project" -ForegroundColor White
Write-Host "    2. Go to Tools → Import → Import Mesen2 Live Trace" -ForegroundColor White
Write-Host "    3. Use localhost:9998" -ForegroundColor White
Write-Host "    4. Click 'Start Streaming'" -ForegroundColor White
Write-Host "    5. Check the log file for results" -ForegroundColor White
Write-Host "" -ForegroundColor White
Write-Host "  Log file location:" -ForegroundColor Cyan
Write-Host "    %LOCALAPPDATA%\DiztinGUIsh\Logs\mesen_connection_$(Get-Date -Format 'yyyy-MM-dd').log" -ForegroundColor White
Write-Host "" -ForegroundColor White
Write-Host "  Press Enter to start DiztinGUIsh..." -ForegroundColor Cyan
Read-Host

Start-Process $dizPath

Write-Host "`n✅ Test sequence complete!" -ForegroundColor Green
Write-Host "   Check the log file for connection results." -ForegroundColor White
