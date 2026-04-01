# Test sequence for Mesen2 → DiztinGUIsh connection
# Run this AFTER starting Mesen2 server

Write-Host "`n=== STEP 1: Verify Mesen2 server is listening ===" -ForegroundColor Cyan
$listening = Get-NetTCPConnection -LocalPort 9998 -State Listen -ErrorAction SilentlyContinue
if ($listening) {
    Write-Host "✅ Port 9998 IS LISTENING - Process: $($listening.OwningProcess)" -ForegroundColor Green
    Write-Host "   Ready to connect from DiztinGUIsh!" -ForegroundColor Green
} else {
    Write-Host "❌ Port 9998 NOT listening!" -ForegroundColor Red
    Write-Host "   Please start Mesen2 server first:" -ForegroundColor Yellow
    Write-Host "   1. Open Mesen2" -ForegroundColor Yellow
    Write-Host "   2. Load a SNES ROM" -ForegroundColor Yellow
    Write-Host "   3. Tools → DiztinGUIsh Server → Start Server" -ForegroundColor Yellow
    exit 1
}

Write-Host "`n=== STEP 2: Test connection ===" -ForegroundColor Cyan
try {
    $client = New-Object System.Net.Sockets.TcpClient
    $client.Connect("localhost", 9998)
    Write-Host "✅ Connection successful!" -ForegroundColor Green
    
    # Try to read handshake (5-byte header + payload)
    $stream = $client.GetStream()
    $stream.ReadTimeout = 3000
    
    Write-Host "`nReading handshake message..." -ForegroundColor Yellow
    $header = New-Object byte[] 5
    $bytesRead = $stream.Read($header, 0, 5)
    
    if ($bytesRead -eq 5) {
        $messageType = $header[0]
        $messageLength = [BitConverter]::ToUInt32($header, 1)
        Write-Host "✅ Received message header:" -ForegroundColor Green
        Write-Host "   Type: $messageType (should be 1 for Handshake)" -ForegroundColor White
        Write-Host "   Length: $messageLength bytes (should be 268 for Handshake)" -ForegroundColor White
        
        if ($messageLength -gt 0 -and $messageLength -lt 10000) {
            # Read payload
            $payload = New-Object byte[] $messageLength
            $totalRead = 0
            while ($totalRead -lt $messageLength) {
                $read = $stream.Read($payload, $totalRead, $messageLength - $totalRead)
                if ($read -eq 0) { break }
                $totalRead += $read
            }
            Write-Host "   Payload: $totalRead bytes received" -ForegroundColor White
            
            if ($totalRead -eq $messageLength) {
                Write-Host "✅ HANDSHAKE RECEIVED SUCCESSFULLY!" -ForegroundColor Green
                Write-Host "`n   This means Mesen2 server is working correctly!" -ForegroundColor Green
                Write-Host "   Now try connecting from DiztinGUIsh application." -ForegroundColor Green
            } else {
                Write-Host "❌ Incomplete payload: got $totalRead bytes, expected $messageLength" -ForegroundColor Red
            }
        } else {
            Write-Host "❌ Invalid message length: $messageLength" -ForegroundColor Red
        }
    } elseif ($bytesRead -eq 0) {
        Write-Host "❌ Connection closed by server (0 bytes read)" -ForegroundColor Red
        Write-Host "   This is the bug we're seeing!" -ForegroundColor Yellow
    } else {
        Write-Host "❌ Incomplete header: got $bytesRead bytes, expected 5" -ForegroundColor Red
    }
    
    $client.Close()
} catch {
    Write-Host "❌ Connection failed: $_" -ForegroundColor Red
}

Write-Host "`n"
