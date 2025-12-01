# Real-time connection monitor for Mesen2 ↔ DiztinGUIsh

Write-Host "=== Live Connection Monitor ===" -ForegroundColor Cyan
Write-Host "Monitoring TCP connection on port 9998..." -ForegroundColor Yellow
Write-Host "Press Ctrl+C to stop" -ForegroundColor Gray
Write-Host ""

$lastState = $null
$connectionStart = $null
$totalMessages = 0

while ($true) {
    # Check for established connection
    $conn = Get-NetTCPConnection -LocalPort 9998 -State Established -ErrorAction SilentlyContinue
    
    $timestamp = Get-Date -Format "HH:mm:ss.fff"
    
    if ($conn) {
        if ($lastState -ne "Connected") {
            $connectionStart = Get-Date
            Write-Host "[$timestamp] ✅ CONNECTION ESTABLISHED" -ForegroundColor Green
            Write-Host "  Remote: $($conn.RemoteAddress):$($conn.RemotePort)" -ForegroundColor Gray
            Write-Host "  State: $($conn.State)" -ForegroundColor Gray
            $lastState = "Connected"
        }
        
        # Show connection duration
        if ($connectionStart) {
            $duration = (Get-Date) - $connectionStart
            $durationStr = "{0:D2}:{1:D2}:{2:D2}" -f $duration.Hours, $duration.Minutes, $duration.Seconds
            Write-Host "`r[$timestamp] 🟢 Connected for $durationStr" -NoNewline -ForegroundColor Green
        }
    }
    else {
        if ($lastState -eq "Connected") {
            Write-Host "" # New line after duration counter
            $duration = (Get-Date) - $connectionStart
            Write-Host "[$timestamp] ❌ CONNECTION LOST after $($duration.TotalSeconds.ToString('F1'))s" -ForegroundColor Red
            $connectionStart = $null
        }
        elseif ($lastState -ne "Disconnected") {
            Write-Host "[$timestamp] ⚫ No connection" -ForegroundColor Gray
        }
        $lastState = "Disconnected"
    }
    
    Start-Sleep -Milliseconds 500
}
