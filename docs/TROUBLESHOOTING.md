# DiztinGUIsh-Mesen2 Integration: Troubleshooting Guide

## 📋 Quick Diagnostic Checklist

### 🚨 Emergency Quick Fixes
- **Can't Connect?** → Check Mesen2 server is running (Tools → DiztinGUIsh → Start Server)
- **Frequent Disconnects?** → Increase timeout from 5s to 10s in Connection Settings
- **High CPU Usage?** → Reduce trace frequency to 1/5 frames
- **Memory Issues?** → Enable automatic garbage collection in Advanced Settings
- **Missing Data?** → Enable data validation and increase buffer sizes

---

## 🔍 Diagnostic Tools

### Built-in Diagnostics

#### Connection Health Check
```csharp
// Access via Tools → Mesen2 Integration → Advanced Settings → Diagnostics
public class ConnectionDiagnostics
{
    public async Task<DiagnosticReport> RunFullDiagnostics()
    {
        var report = new DiagnosticReport();
        
        // Network connectivity
        report.NetworkStatus = await TestNetworkConnectivity();
        
        // Port availability
        report.PortStatus = await TestPortAvailability(1234);
        
        // Mesen2 server status
        report.ServerStatus = await TestMesen2Server();
        
        // Protocol compatibility
        report.ProtocolStatus = await TestProtocolVersion();
        
        return report;
    }
}
```

#### System Health Monitor
```
Real-time Metrics:
┌─────────────────────────────────────┐
│ Connection Status: ✅ Connected      │
│ Uptime: 02:34:17                   │
│ Messages Sent: 15,432              │
│ Messages Received: 15,429          │
│ Bandwidth: 42.3 KB/s               │
│ CPU Usage: 3.2%                   │
│ Memory Usage: 128.4 MB             │
│ Error Count: 0                     │
│ Last Error: None                   │
└─────────────────────────────────────┘
```

### External Diagnostic Commands

#### PowerShell Network Diagnostics
```powershell
# Complete network diagnostic script
function Test-DiztinguishConnection {
    param(
        [string]$Host = "localhost",
        [int]$Port = 1234
    )
    
    Write-Host "🔍 DiztinGUIsh-Mesen2 Connection Diagnostics" -ForegroundColor Cyan
    Write-Host "=" * 50
    
    # Test basic connectivity
    Write-Host "📡 Testing basic connectivity..." -ForegroundColor Yellow
    $tcpTest = Test-NetConnection -ComputerName $Host -Port $Port -WarningAction SilentlyContinue
    if ($tcpTest.TcpTestSucceeded) {
        Write-Host "✅ Port $Port is reachable on $Host" -ForegroundColor Green
    } else {
        Write-Host "❌ Port $Port is NOT reachable on $Host" -ForegroundColor Red
    }
    
    # Check firewall rules
    Write-Host "🛡️  Checking Windows Firewall..." -ForegroundColor Yellow
    $firewallRules = Get-NetFirewallRule | Where-Object {
        $_.DisplayName -like "*Mesen*" -or 
        $_.DisplayName -like "*DiztinGUIsh*"
    }
    if ($firewallRules) {
        Write-Host "✅ Found firewall rules:" -ForegroundColor Green
        $firewallRules | ForEach-Object { Write-Host "   - $($_.DisplayName)" }
    } else {
        Write-Host "⚠️  No firewall rules found - may need to add exceptions" -ForegroundColor Yellow
    }
    
    # Check port usage
    Write-Host "🔌 Checking port usage..." -ForegroundColor Yellow
    $portInfo = Get-NetTCPConnection -LocalPort $Port -ErrorAction SilentlyContinue
    if ($portInfo) {
        Write-Host "✅ Port $Port is in use by:" -ForegroundColor Green
        $portInfo | ForEach-Object {
            $process = Get-Process -Id $_.OwningProcess -ErrorAction SilentlyContinue
            Write-Host "   - PID: $($_.OwningProcess), Process: $($process.ProcessName)"
        }
    } else {
        Write-Host "❌ Port $Port is not in use" -ForegroundColor Red
    }
    
    # Check system resources
    Write-Host "💻 Checking system resources..." -ForegroundColor Yellow
    $cpu = Get-Counter "\Processor(_Total)\% Processor Time" -SampleInterval 1 -MaxSamples 1
    $memory = Get-Counter "\Memory\Available MBytes" -SampleInterval 1 -MaxSamples 1
    
    Write-Host "   CPU Usage: $([math]::Round(100 - $cpu.CounterSamples[0].CookedValue, 1))%" -ForegroundColor White
    Write-Host "   Available Memory: $([math]::Round($memory.CounterSamples[0].CookedValue, 0)) MB" -ForegroundColor White
    
    Write-Host "=" * 50
    Write-Host "🏁 Diagnostics complete!" -ForegroundColor Cyan
}

# Usage
Test-DiztinguishConnection
```

---

## 🔧 Connection Problems

### Problem: "Connection Refused" Error

#### Symptoms
```
Error: System.Net.Sockets.SocketException
Connection refused (localhost:1234)
Status: Disconnected
```

#### Root Causes & Solutions

##### Cause 1: Mesen2 Server Not Started
**Verification:**
```powershell
Get-Process -Name "Mesen" -ErrorAction SilentlyContinue
```

**Solution:**
1. Open Mesen2
2. Load a ROM file
3. Go to **Tools → DiztinGUIsh → Start Server**
4. Verify status bar shows "DiztinGUIsh Server: Running"

##### Cause 2: Port Already in Use
**Verification:**
```powershell
Get-NetTCPConnection -LocalPort 1234 | Format-Table LocalAddress, LocalPort, State, OwningProcess
```

**Solution:**
```powershell
# Find process using the port
$process = Get-NetTCPConnection -LocalPort 1234 | Select-Object -ExpandProperty OwningProcess
Get-Process -Id $process

# Option 1: Kill conflicting process
Stop-Process -Id $process -Force

# Option 2: Change DiztinGUIsh port
# Edit connection settings to use different port (e.g., 1235)
```

##### Cause 3: Firewall Blocking Connection
**Verification:**
```powershell
# Check if Windows Firewall is blocking
Test-NetConnection -ComputerName localhost -Port 1234 -DiagnoseRouting
```

**Solution:**
```powershell
# Add firewall exception
New-NetFirewallRule -DisplayName "Mesen2 DiztinGUIsh" -Direction Inbound -Protocol TCP -LocalPort 1234 -Action Allow
```

### Problem: "Connection Timeout" Error

#### Symptoms
```
Error: Connection attempt timed out after 5000ms
Status: Connection timeout
Retry attempts: 3/5
```

#### Solutions

##### Solution 1: Increase Timeout Values
```json
{
  "ConnectionTimeoutMs": 15000,
  "SocketTimeoutMs": 10000,
  "HandshakeTimeoutMs": 5000
}
```

##### Solution 2: Network Optimization
```json
{
  "TcpNoDelay": true,
  "SocketBufferSize": 65536,
  "EnableKeepalive": true,
  "KeepaliveIntervalMs": 1000
}
```

##### Solution 3: Retry Strategy
```json
{
  "MaxReconnectAttempts": 10,
  "AutoReconnectDelayMs": 3000,
  "ExponentialBackoff": true,
  "MaxBackoffDelayMs": 30000
}
```

### Problem: Frequent Random Disconnections

#### Symptoms
```
Disconnection pattern: Every 2-5 minutes
Auto-reconnect: Successful
Error logs: Socket closed unexpectedly
```

#### Diagnostic Script
```powershell
# Monitor connection stability
function Monitor-DiztinguishConnection {
    param([int]$DurationMinutes = 10)
    
    $endTime = (Get-Date).AddMinutes($DurationMinutes)
    $disconnections = @()
    
    while ((Get-Date) -lt $endTime) {
        $connection = Test-NetConnection -ComputerName localhost -Port 1234 -WarningAction SilentlyContinue
        
        if (-not $connection.TcpTestSucceeded) {
            $disconnections += @{
                Time = Get-Date
                Duration = "Unknown"
            }
            Write-Host "❌ Disconnection detected at $(Get-Date)" -ForegroundColor Red
            
            # Wait for reconnection
            do {
                Start-Sleep -Seconds 1
                $connection = Test-NetConnection -ComputerName localhost -Port 1234 -WarningAction SilentlyContinue
            } while (-not $connection.TcpTestSucceeded)
            
            Write-Host "✅ Reconnected at $(Get-Date)" -ForegroundColor Green
        }
        
        Start-Sleep -Seconds 5
    }
    
    Write-Host "📊 Connection Stability Report:" -ForegroundColor Cyan
    Write-Host "   Total disconnections: $($disconnections.Count)"
    Write-Host "   Monitoring duration: $DurationMinutes minutes"
    Write-Host "   Average stability: $([math]::Round(100 - ($disconnections.Count / ($DurationMinutes * 12) * 100), 2))%"
}
```

#### Solutions

##### Network Stack Reset
```powershell
# Reset network adapters
netsh winsock reset
netsh int ip reset
ipconfig /flushdns
Restart-Computer
```

##### Connection Hardening
```json
{
  "SocketOptions": {
    "EnableKeepalive": true,
    "KeepaliveTime": 60,
    "KeepaliveInterval": 10,
    "KeepaliveRetries": 5
  },
  "ReconnectionStrategy": {
    "EnableHeartbeat": true,
    "HeartbeatIntervalMs": 10000,
    "MissedHeartbeatsBeforeDisconnect": 3
  }
}
```

---

## ⚡ Performance Problems

### Problem: High CPU Usage

#### Symptoms
```
CPU Usage: >50% constantly
UI Responsiveness: Laggy
Trace Viewer: Falling behind real-time
Memory: Slowly increasing
```

#### Performance Profiling
```powershell
# Monitor Mesen2 and DiztinGUIsh performance
function Monitor-DiztinguishPerformance {
    $processes = @("Mesen", "Diz.App.Winforms")
    
    foreach ($processName in $processes) {
        $process = Get-Process -Name $processName -ErrorAction SilentlyContinue
        if ($process) {
            Write-Host "📊 $processName Performance:" -ForegroundColor Cyan
            
            # CPU usage over 30 seconds
            $cpuCounter = "\Process($processName)\% Processor Time"
            $cpuSamples = Get-Counter -Counter $cpuCounter -SampleInterval 1 -MaxSamples 30
            $avgCpu = ($cpuSamples.CounterSamples | Measure-Object -Property CookedValue -Average).Average
            
            Write-Host "   Average CPU: $([math]::Round($avgCpu, 1))%"
            Write-Host "   Memory: $([math]::Round($process.WorkingSet64 / 1MB, 1)) MB"
            Write-Host "   Handles: $($process.HandleCount)"
            Write-Host "   Threads: $($process.Threads.Count)"
        }
    }
}
```

#### Solutions

##### Reduce Trace Frequency
```json
{
  "TraceSettings": {
    "FrameInterval": 10,        // Every 10th frame instead of every frame
    "MaxTracesPerFrame": 50,    // Limit traces per frame
    "EnableFiltering": true,    // Filter out repetitive traces
    "UseCompressionLevel": 6    // Compress trace data
  }
}
```

##### Optimize Memory Usage
```json
{
  "MemoryOptimization": {
    "MaxTracesInMemory": 10000,
    "PurgeOldTracesMs": 30000,
    "EnableGarbageCollection": true,
    "GcIntervalMs": 5000,
    "MaxMemoryUsageMB": 256
  }
}
```

##### Thread Pool Optimization
```json
{
  "Threading": {
    "WorkerThreads": 2,
    "IOThreads": 2,
    "MaxConcurrentOperations": 2,
    "UseBackgroundThreads": true,
    "ThreadPriority": "Normal"
  }
}
```

### Problem: Memory Leaks

#### Memory Leak Detection Script
```powershell
function Monitor-MemoryLeaks {
    param(
        [string]$ProcessName = "Diz.App.Winforms",
        [int]$DurationMinutes = 30
    )
    
    $measurements = @()
    $endTime = (Get-Date).AddMinutes($DurationMinutes)
    
    Write-Host "🔍 Monitoring memory usage for $ProcessName..." -ForegroundColor Cyan
    
    while ((Get-Date) -lt $endTime) {
        $process = Get-Process -Name $ProcessName -ErrorAction SilentlyContinue
        if ($process) {
            $measurement = @{
                Time = Get-Date
                WorkingSet = $process.WorkingSet64
                PrivateMemory = $process.PrivateMemorySize64
                VirtualMemory = $process.VirtualMemorySize64
            }
            $measurements += $measurement
            
            Write-Host "$(Get-Date -Format 'HH:mm:ss'): $([math]::Round($process.WorkingSet64/1MB, 1)) MB" -ForegroundColor White
        }
        
        Start-Sleep -Seconds 30
    }
    
    # Analyze for leaks
    if ($measurements.Count -gt 2) {
        $firstMeasurement = $measurements[0].WorkingSet
        $lastMeasurement = $measurements[-1].WorkingSet
        $growth = $lastMeasurement - $firstMeasurement
        $growthRate = $growth / $DurationMinutes / 1MB
        
        Write-Host "📊 Memory Analysis:" -ForegroundColor Cyan
        Write-Host "   Initial Memory: $([math]::Round($firstMeasurement/1MB, 1)) MB"
        Write-Host "   Final Memory: $([math]::Round($lastMeasurement/1MB, 1)) MB"
        Write-Host "   Total Growth: $([math]::Round($growth/1MB, 1)) MB"
        Write-Host "   Growth Rate: $([math]::Round($growthRate, 2)) MB/min"
        
        if ($growthRate > 1) {
            Write-Host "⚠️  Possible memory leak detected!" -ForegroundColor Red
        } else {
            Write-Host "✅ Memory usage appears stable" -ForegroundColor Green
        }
    }
}
```

#### Memory Leak Solutions

##### Enable Aggressive Cleanup
```json
{
  "MemoryManagement": {
    "EnableAggressiveGC": true,
    "ForceGCOnLowMemory": true,
    "LowMemoryThresholdMB": 100,
    "MaxObjectLifetimeMs": 300000,
    "EnableWeakReferences": true
  }
}
```

##### Object Pool Configuration
```csharp
// In Advanced Settings
public class ObjectPoolSettings
{
    public bool EnableObjectPooling { get; set; } = true;
    public int MaxPoolSize { get; set; } = 1000;
    public int PreallocationCount { get; set; } = 100;
    public TimeSpan ObjectExpirationTime { get; set; } = TimeSpan.FromMinutes(5);
}
```

---

## 📊 Data Integrity Problems

### Problem: Missing or Corrupt Trace Data

#### Symptoms
```
Trace Viewer: Gaps in execution timeline
Data Validation: Checksum failures
Error Logs: "Invalid trace format"
CPU States: Inconsistent register values
```

#### Data Integrity Checker
```powershell
function Test-TraceDataIntegrity {
    $logPath = "$env:USERPROFILE\AppData\Local\DiztinGUIsh\Logs\traces.log"
    
    if (Test-Path $logPath) {
        Write-Host "🔍 Analyzing trace data integrity..." -ForegroundColor Cyan
        
        $content = Get-Content $logPath
        $totalLines = $content.Count
        $errorLines = $content | Where-Object { $_ -match "ERROR|CORRUPT|INVALID" }
        $checksumFailures = $content | Where-Object { $_ -match "checksum" -and $_ -match "fail" }
        
        Write-Host "📊 Trace Data Analysis:" -ForegroundColor White
        Write-Host "   Total log entries: $totalLines"
        Write-Host "   Error entries: $($errorLines.Count)"
        Write-Host "   Checksum failures: $($checksumFailures.Count)"
        Write-Host "   Error rate: $([math]::Round($errorLines.Count / $totalLines * 100, 2))%"
        
        if ($errorLines.Count -gt 0) {
            Write-Host "⚠️  Recent errors:" -ForegroundColor Yellow
            $errorLines | Select-Object -Last 5 | ForEach-Object {
                Write-Host "   $_" -ForegroundColor Red
            }
        }
    }
}
```

#### Solutions

##### Enable Data Validation
```json
{
  "DataIntegrity": {
    "EnableChecksums": true,
    "ValidateSequenceNumbers": true,
    "DetectDuplicates": true,
    "RetransmitCorruptData": true,
    "MaxRetransmissionAttempts": 3
  }
}
```

##### Buffer Size Optimization
```json
{
  "BufferManagement": {
    "SocketReceiveBufferSize": 131072,
    "SocketSendBufferSize": 131072,
    "ApplicationBufferSize": 65536,
    "EnableBufferCompression": true
  }
}
```

### Problem: Synchronization Issues

#### Symptoms
```
CPU States: Out of sync with execution
Memory Dumps: Stale data
Breakpoints: Not triggering correctly
Labels: Mismatch between tools
```

#### Synchronization Diagnostic
```csharp
public class SyncDiagnostics
{
    public async Task<SyncReport> CheckSynchronization()
    {
        var report = new SyncReport();
        
        // Check CPU state sync
        var mesenCpuState = await GetMesenCpuState();
        var localCpuState = GetLocalCpuState();
        report.CpuStateSyncDelta = Math.Abs(mesenCpuState.PC - localCpuState.PC);
        
        // Check memory sync
        var mesenMemory = await GetMesenMemoryChecksum();
        var localMemory = GetLocalMemoryChecksum();
        report.MemorySyncMatch = mesenMemory == localMemory;
        
        // Check breakpoint sync
        var mesenBreakpoints = await GetMesenBreakpoints();
        var localBreakpoints = GetLocalBreakpoints();
        report.BreakpointSyncCount = localBreakpoints.Intersect(mesenBreakpoints).Count();
        
        return report;
    }
}
```

#### Synchronization Solutions

##### Force Re-synchronization
```json
{
  "SynchronizationSettings": {
    "EnablePeriodicSync": true,
    "SyncIntervalMs": 1000,
    "ForceFullSyncOnError": true,
    "MaxSyncRetries": 5,
    "SyncTimeoutMs": 10000
  }
}
```

---

## 🛡️ Security and Firewall Issues

### Windows Firewall Configuration

#### Automatic Firewall Setup
```powershell
function Setup-DiztinguishFirewall {
    Write-Host "🛡️  Configuring Windows Firewall for DiztinGUIsh..." -ForegroundColor Cyan
    
    # Remove existing rules
    Get-NetFirewallRule -DisplayName "*DiztinGUIsh*" | Remove-NetFirewallRule -Confirm:$false
    Get-NetFirewallRule -DisplayName "*Mesen2*" | Remove-NetFirewallRule -Confirm:$false
    
    # Add new rules
    New-NetFirewallRule -DisplayName "Mesen2 DiztinGUIsh Server" `
        -Direction Inbound -Protocol TCP -LocalPort 1234 -Action Allow `
        -Profile Domain,Private,Public
    
    New-NetFirewallRule -DisplayName "DiztinGUIsh Client" `
        -Direction Outbound -Protocol TCP -RemotePort 1234 -Action Allow `
        -Profile Domain,Private,Public
    
    Write-Host "✅ Firewall rules configured successfully" -ForegroundColor Green
}

Setup-DiztinguishFirewall
```

### Antivirus Compatibility

#### Common Antivirus Issues
```
Windows Defender: May quarantine network connections
Norton/McAfee: May block socket operations  
Avast/AVG: May interfere with process communication
```

#### Antivirus Whitelist Setup
```powershell
# Windows Defender exclusions
function Add-DefenderExclusions {
    $mesenPath = "C:\Path\To\Mesen2"
    $diztinguishPath = "C:\Path\To\DiztinGUIsh"
    
    Add-MpPreference -ExclusionPath $mesenPath
    Add-MpPreference -ExclusionPath $diztinguishPath
    Add-MpPreference -ExclusionProcess "Mesen.exe"
    Add-MpPreference -ExclusionProcess "Diz.App.Winforms.exe"
    
    Write-Host "✅ Windows Defender exclusions added" -ForegroundColor Green
}
```

---

## 🔄 Recovery Procedures

### Complete Reset Procedure

#### Step 1: Clean Shutdown
```powershell
# Graceful shutdown script
function Stop-DiztinguishServices {
    Write-Host "🛑 Stopping DiztinGUIsh services..." -ForegroundColor Yellow
    
    # Stop DiztinGUIsh
    Get-Process -Name "Diz.App.Winforms" -ErrorAction SilentlyContinue | Stop-Process -Force
    
    # Stop Mesen2
    Get-Process -Name "Mesen" -ErrorAction SilentlyContinue | Stop-Process -Force
    
    # Wait for processes to fully terminate
    Start-Sleep -Seconds 3
    
    # Clear any lingering TCP connections
    Get-NetTCPConnection -LocalPort 1234 -ErrorAction SilentlyContinue | 
        ForEach-Object { Stop-Process -Id $_.OwningProcess -Force }
    
    Write-Host "✅ Services stopped cleanly" -ForegroundColor Green
}
```

#### Step 2: Configuration Reset
```powershell
function Reset-DiztinguishConfiguration {
    Write-Host "🔧 Resetting configuration..." -ForegroundColor Yellow
    
    $configPaths = @(
        "$env:USERPROFILE\AppData\Local\DiztinGUIsh\config.json",
        "$env:USERPROFILE\AppData\Local\Mesen\DiztinguishSettings.xml",
        "$env:USERPROFILE\AppData\Roaming\DiztinGUIsh\connections.json"
    )
    
    foreach ($path in $configPaths) {
        if (Test-Path $path) {
            $backupPath = "$path.backup.$(Get-Date -Format 'yyyyMMdd_HHmmss')"
            Copy-Item $path $backupPath
            Remove-Item $path
            Write-Host "   Backed up and removed: $path" -ForegroundColor White
        }
    }
    
    Write-Host "✅ Configuration reset complete" -ForegroundColor Green
}
```

#### Step 3: Clean Restart
```powershell
function Restart-DiztinguishServices {
    Write-Host "🚀 Starting services..." -ForegroundColor Yellow
    
    # Start Mesen2 first
    $mesenPath = "C:\Path\To\Mesen2\Mesen.exe"
    if (Test-Path $mesenPath) {
        Start-Process $mesenPath
        Start-Sleep -Seconds 5
        Write-Host "   Mesen2 started" -ForegroundColor White
    }
    
    # Start DiztinGUIsh
    $dizPath = "C:\Path\To\DiztinGUIsh\Diz.App.Winforms.exe"
    if (Test-Path $dizPath) {
        Start-Process $dizPath
        Start-Sleep -Seconds 3
        Write-Host "   DiztinGUIsh started" -ForegroundColor White
    }
    
    Write-Host "✅ Services restarted" -ForegroundColor Green
    Write-Host "📝 Please reconfigure connection settings" -ForegroundColor Cyan
}
```

---

## 📝 Log Analysis

### Log File Locations
```
DiztinGUIsh Logs:
  %USERPROFILE%\AppData\Local\DiztinGUIsh\Logs\
  ├── application.log      (General application events)
  ├── connection.log       (Network connection events)
  ├── traces.log          (Trace data processing)
  └── errors.log          (Error messages only)

Mesen2 Logs:
  %USERPROFILE%\AppData\Local\Mesen\
  ├── DiztinguishServer.log (Server events)
  └── NetworkActivity.log   (Network traffic)
```

### Log Analysis Tools

#### Error Pattern Detection
```powershell
function Analyze-DiztinguishLogs {
    $logDir = "$env:USERPROFILE\AppData\Local\DiztinGUIsh\Logs"
    
    if (Test-Path $logDir) {
        Write-Host "📊 Analyzing DiztinGUIsh logs..." -ForegroundColor Cyan
        
        # Get all log files modified in last 24 hours
        $recentLogs = Get-ChildItem $logDir -Filter "*.log" | 
            Where-Object { $_.LastWriteTime -gt (Get-Date).AddDays(-1) }
        
        foreach ($logFile in $recentLogs) {
            Write-Host "`n📄 Analyzing: $($logFile.Name)" -ForegroundColor Yellow
            
            $content = Get-Content $logFile.FullName
            
            # Count error types
            $errors = $content | Where-Object { $_ -match "ERROR" }
            $warnings = $content | Where-Object { $_ -match "WARN" }
            $connectionErrors = $content | Where-Object { $_ -match "Connection" -and $_ -match "ERROR" }
            
            Write-Host "   Total lines: $($content.Count)"
            Write-Host "   Errors: $($errors.Count)" -ForegroundColor $(if($errors.Count -gt 0){"Red"}else{"Green"})
            Write-Host "   Warnings: $($warnings.Count)" -ForegroundColor $(if($warnings.Count -gt 0){"Yellow"}else{"Green"})
            Write-Host "   Connection errors: $($connectionErrors.Count)" -ForegroundColor $(if($connectionErrors.Count -gt 0){"Red"}else{"Green"})
            
            # Show recent errors
            if ($errors.Count -gt 0) {
                Write-Host "   Recent errors:" -ForegroundColor Red
                $errors | Select-Object -Last 3 | ForEach-Object {
                    Write-Host "     $_"
                }
            }
        }
    }
}

# Usage
Analyze-DiztinguishLogs
```

---

## 📞 Getting Help

### Automated Support Information Collection

```powershell
function Collect-DiztinguishSupportInfo {
    Write-Host "🔍 Collecting DiztinGUIsh support information..." -ForegroundColor Cyan
    
    $supportInfo = @{}
    
    # System information
    $supportInfo.System = @{
        OS = (Get-WmiObject -Class Win32_OperatingSystem).Caption
        Architecture = (Get-WmiObject -Class Win32_Processor).Architecture
        Memory = [math]::Round((Get-WmiObject -Class Win32_ComputerSystem).TotalPhysicalMemory / 1GB, 1)
        DotNetVersion = (Get-ChildItem "HKLM:\SOFTWARE\Microsoft\NET Framework Setup\NDP" | 
            Sort-Object Name | Select-Object -Last 1).PSChildName
    }
    
    # Application versions
    $supportInfo.Applications = @{}
    
    $mesenExe = Get-ChildItem -Path "C:\" -Name "Mesen.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($mesenExe) {
        $supportInfo.Applications.Mesen2 = (Get-ItemProperty $mesenExe.FullName).VersionInfo.FileVersion
    }
    
    # Network configuration
    $supportInfo.Network = @{
        Port1234Available = (Test-NetConnection -ComputerName localhost -Port 1234 -WarningAction SilentlyContinue).TcpTestSucceeded
        FirewallRules = (Get-NetFirewallRule | Where-Object { $_.DisplayName -like "*Mesen*" -or $_.DisplayName -like "*DiztinGUIsh*" }).Count
    }
    
    # Recent errors
    $errorLogPath = "$env:USERPROFILE\AppData\Local\DiztinGUIsh\Logs\errors.log"
    if (Test-Path $errorLogPath) {
        $recentErrors = Get-Content $errorLogPath | Select-Object -Last 10
        $supportInfo.RecentErrors = $recentErrors
    }
    
    # Output support information
    $timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $outputPath = "$env:USERPROFILE\Desktop\DiztinGUIsh_SupportInfo_$timestamp.json"
    
    $supportInfo | ConvertTo-Json -Depth 3 | Out-File $outputPath
    
    Write-Host "✅ Support information saved to: $outputPath" -ForegroundColor Green
    Write-Host "📧 Please include this file when requesting support" -ForegroundColor Cyan
}

# Usage
Collect-DiztinguishSupportInfo
```

### Support Channels

#### GitHub Issues
- **Repository**: https://github.com/TheAnsarya/DiztinGUIsh
- **Before posting**: Run `Collect-DiztinguishSupportInfo` and attach the output
- **Include**: Detailed steps to reproduce, expected vs actual behavior

#### Community Support
- **Discord**: [Community server link]
- **Forum**: [Community forum link]
- **Reddit**: r/SNESdev, r/RomHacking

#### Professional Support
- **Email**: [support email]
- **Response time**: 24-48 hours
- **Includes**: Priority troubleshooting, configuration assistance

---

This comprehensive troubleshooting guide provides systematic approaches to diagnose and resolve common issues with the DiztinGUIsh-Mesen2 integration. Use the diagnostic scripts to quickly identify problems and follow the detailed solutions to resolve them effectively.