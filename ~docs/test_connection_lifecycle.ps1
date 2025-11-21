# Quick test to check Mesen2 DiztinGUIsh server connection lifecycle
# Tests if server is running, accepting connections, and streaming data

param(
    [string]$HostName = "localhost",
    [int]$Port = 9998
)

function Test-MesenConnection {
    param($HostName, $PortNumber)
    
    Write-Host "🧪 Testing connection to Mesen2 at ${HostName}:${PortNumber}" -ForegroundColor Cyan
    Write-Host ("=" * 50) -ForegroundColor Gray
    
    $tcpClient = $null
    $stream = $null
    
    try {
        # Test 1: Basic TCP connection
        Write-Host "📡 Attempting TCP connection..." -ForegroundColor Yellow
        $tcpClient = New-Object System.Net.Sockets.TcpClient
        $connectTask = $tcpClient.ConnectAsync($HostName, $PortNumber)
        
        if ($connectTask.Wait(5000)) {
            Write-Host "✅ Connected to ${HostName}:${PortNumber}" -ForegroundColor Green
            $stream = $tcpClient.GetStream()
            $stream.ReadTimeout = 5000
        } else {
            Write-Host "❌ Connection timeout" -ForegroundColor Red
            return $false
        }
        
        # Test 2: Wait for handshake message
        Write-Host "🤝 Waiting for handshake message..." -ForegroundColor Yellow
        
        # Read message header (5 bytes: type + length)
        $headerBytes = New-Object byte[] 5
        $bytesRead = 0
        $timeoutStart = Get-Date
        
        while ($bytesRead -lt 5 -and ((Get-Date) - $timeoutStart).TotalSeconds -lt 5) {
            if ($stream.DataAvailable) {
                $read = $stream.Read($headerBytes, $bytesRead, 5 - $bytesRead)
                $bytesRead += $read
            } else {
                Start-Sleep -Milliseconds 100
            }
        }
        
        if ($bytesRead -lt 5) {
            Write-Host "❌ Incomplete header received: $bytesRead bytes" -ForegroundColor Red
            return $false
        }
        
        $msgType = $headerBytes[0]
        $msgLength = [BitConverter]::ToUInt32($headerBytes, 1)
        
        Write-Host "📦 Message received - Type: $msgType, Length: $msgLength" -ForegroundColor Blue
        
        # Read message body if present
        if ($msgLength -gt 0) {
            $bodyBytes = New-Object byte[] $msgLength
            $bodyBytesRead = 0
            
            while ($bodyBytesRead -lt $msgLength -and ((Get-Date) - $timeoutStart).TotalSeconds -lt 10) {
                if ($stream.DataAvailable) {
                    $read = $stream.Read($bodyBytes, $bodyBytesRead, $msgLength - $bodyBytesRead)
                    $bodyBytesRead += $read
                } else {
                    Start-Sleep -Milliseconds 100
                }
            }
            
            Write-Host "📄 Message body: $bodyBytesRead bytes received" -ForegroundColor Blue
            
            # If handshake (type 0), decode some basic info
            if ($msgType -eq 0 -and $bodyBytesRead -ge 21) {
                $version = [BitConverter]::ToUInt32($bodyBytes, 0)
                $emuNameLen = $bodyBytes[4]
                if ($bodyBytesRead -ge (5 + $emuNameLen + 16)) {
                    $emuName = [System.Text.Encoding]::UTF8.GetString($bodyBytes, 5, $emuNameLen)
                    Write-Host "🎮 Handshake - Version: $version, Emulator: $emuName" -ForegroundColor Green
                }
            }
        }
        
        # Test 3: Wait for additional messages
        Write-Host "⏳ Waiting for streaming data (5 seconds)..." -ForegroundColor Yellow
        $messageCount = 1  # Already got handshake
        $streamStart = Get-Date
        $stream.ReadTimeout = 1000  # Short timeout for subsequent messages
        
        while (((Get-Date) - $streamStart).TotalSeconds -lt 5) {
            try {
                $headerBytes = New-Object byte[] 5
                $bytesRead = 0
                
                # Try to read header with timeout
                $readTask = $stream.ReadAsync($headerBytes, 0, 5)
                if ($readTask.Wait(1000)) {
                    $bytesRead = $readTask.Result
                }
                
                if ($bytesRead -lt 5) {
                    Start-Sleep -Milliseconds 100
                    continue
                }
                
                $msgType = $headerBytes[0]
                $msgLength = [BitConverter]::ToUInt32($headerBytes, 1)
                
                # Read body if present
                if ($msgLength -gt 0) {
                    $bodyBytes = New-Object byte[] $msgLength
                    $readTask = $stream.ReadAsync($bodyBytes, 0, $msgLength)
                    if (-not $readTask.Wait(1000) -or $readTask.Result -lt $msgLength) {
                        break
                    }
                }
                
                $messageCount++
                Write-Host "📨 Message #${messageCount}: Type $msgType, Length $msgLength" -ForegroundColor Blue
                
            } catch {
                Start-Sleep -Milliseconds 100
                continue
            }
        }
        
        Write-Host "📊 Total messages received: $messageCount" -ForegroundColor Cyan
        
        if ($messageCount -eq 1) {
            Write-Host "⚠️  Only handshake received - no streaming data" -ForegroundColor Yellow
            Write-Host "💡 Possible causes:" -ForegroundColor White
            Write-Host "   • Mesen2 emulation is paused" -ForegroundColor Gray
            Write-Host "   • No ROM is loaded" -ForegroundColor Gray
            Write-Host "   • DiztinGUIsh server is not actively streaming" -ForegroundColor Gray
            return $false
        } else {
            Write-Host "✅ Streaming is working! Received $($messageCount - 1) data messages" -ForegroundColor Green
            return $true
        }
        
    } catch [System.Net.Sockets.SocketException] {
        Write-Host "❌ Connection refused - Mesen2 server not running or port blocked" -ForegroundColor Red
        return $false
    } catch {
        Write-Host "❌ Connection error: $($_.Exception.Message)" -ForegroundColor Red
        return $false
    } finally {
        if ($stream) { $stream.Close() }
        if ($tcpClient) { $tcpClient.Close() }
    }
}

# Run the test
$success = Test-MesenConnection -HostName $HostName -PortNumber $Port

if ($success) {
    Write-Host ""
    Write-Host "🎉 Connection test PASSED - streaming is working" -ForegroundColor Green
    Write-Host "💡 If DiztinGUIsh still has issues, check the UI cancellation logic" -ForegroundColor White
} else {
    Write-Host ""
    Write-Host "❌ Connection test FAILED" -ForegroundColor Red
    Write-Host "🔧 Troubleshooting steps:" -ForegroundColor White
    Write-Host "   1. Make sure Mesen2 is running" -ForegroundColor Gray
    Write-Host "   2. Load a ROM in Mesen2" -ForegroundColor Gray
    Write-Host "   3. Enable DiztinGUIsh server: emu.startDiztinguishServer($Port)" -ForegroundColor Gray
    Write-Host "   4. Start emulation (unpause)" -ForegroundColor Gray
}

# Pause so user can see results
Write-Host ""
Write-Host "Press any key to exit..." -ForegroundColor DarkGray
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")