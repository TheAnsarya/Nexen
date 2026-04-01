# DiztinGUIsh-Mesen2 Integration: Complete User Guide

## 📋 Table of Contents
1. [Quick Start Guide](#quick-start-guide)
2. [Installation & Setup](#installation--setup)
3. [Connection Configuration](#connection-configuration)
4. [User Interface Overview](#user-interface-overview)
5. [Workflow Examples](#workflow-examples)
6. [Advanced Features](#advanced-features)
7. [Performance Optimization](#performance-optimization)
8. [Troubleshooting](#troubleshooting)
9. [FAQ](#faq)
10. [Video Tutorials](#video-tutorials)

---

## 🚀 Quick Start Guide

### Prerequisites
- **Mesen2 Emulator** (latest version with DiztinGUIsh integration)
- **DiztinGUIsh** (with Mesen2 integration support)
- **Windows 10/11** or compatible operating system
- **.NET 9.0 Runtime** or newer

### 5-Minute Setup

1. **Launch Mesen2** with your SNES ROM
2. **Enable DiztinGUIsh Server**: Tools → DiztinGUIsh → Start Server
3. **Open DiztinGUIsh** and load your project
4. **Connect**: Tools → Mesen2 Integration → Connect to Mesen2
5. **Start Debugging**: Use real-time trace viewing and analysis!

---

## 💾 Installation & Setup

### Step 1: Install Prerequisites

#### Mesen2 Installation
```powershell
# Download from: https://github.com/SourMesen/Mesen2/releases
# Extract to desired location
# Run Mesen.exe
```

#### DiztinGUIsh Installation
```powershell
# Clone repository
git clone https://github.com/TheAnsarya/DiztinGUIsh.git
cd DiztinGUIsh

# Build solution
dotnet build --configuration Release

# Run application
dotnet run --project Diz.App.Winforms
```

### Step 2: Initial Configuration

#### Mesen2 Configuration
1. **Open Mesen2**
2. **Go to**: Tools → Options → DiztinGUIsh
3. **Configure**:
   - Port: `1234` (default)
   - Auto-start server: ✅ Enabled
   - Verbose logging: ⚠️ Only for debugging

#### DiztinGUIsh Configuration
1. **Open DiztinGUIsh**
2. **Navigate to**: Tools → Mesen2 Integration → Connection Settings
3. **Set**:
   - Host: `localhost`
   - Port: `1234`
   - Timeout: `5000ms`
   - Auto-reconnect: ✅ Recommended

---

## 🔌 Connection Configuration

### Basic Connection Setup

#### Method 1: Using the Connection Dialog
1. **Open**: Tools → Mesen2 Integration → Connection Settings
2. **Configure Server Details**:
   ```
   Host: localhost
   Port: 1234
   Timeout: 5000ms
   ```
3. **Test Connection**: Click "Test Connection"
4. **Save Settings**: Click "OK"

#### Method 2: Manual Configuration
Edit the configuration file directly:
```json
{
  "Mesen2Integration": {
    "DefaultHost": "localhost",
    "DefaultPort": 1234,
    "ConnectionTimeoutMs": 5000,
    "AutoReconnect": true,
    "AutoReconnectDelayMs": 3000,
    "MaxReconnectAttempts": 5,
    "VerboseLogging": false
  }
}
```

### Advanced Connection Options

#### Network Configuration
For remote connections:
```json
{
  "DefaultHost": "192.168.1.100",
  "DefaultPort": 1234,
  "ConnectionTimeoutMs": 10000,
  "FirewallBypass": true
}
```

#### Security Settings
```json
{
  "EnableEncryption": false,
  "AllowRemoteConnections": false,
  "MaxConcurrentConnections": 1,
  "ConnectionWhitelist": ["127.0.0.1", "192.168.1.0/24"]
}
```

---

## 🎛️ User Interface Overview

### Main Menu Integration

The integration adds a comprehensive **Mesen2 Integration** submenu to DiztinGUIsh:

```
Tools → Mesen2 Integration
├── Connect to Mesen2        [Ctrl+M, C]
├── Disconnect from Mesen2   [Ctrl+M, D]
├── ─────────────────────────
├── Dashboard                [Ctrl+M, B]
├── Status Window           [Ctrl+M, S]
├── Trace Viewer            [Ctrl+M, T]
├── ─────────────────────────
├── Connection Settings     [Ctrl+M, N]
└── Advanced Settings       [Ctrl+M, A]
```

### Dialog Windows Overview

#### 1. **Connection Dialog** [Ctrl+M, N]
- **Purpose**: Configure connection parameters
- **Features**:
  - Real-time connection testing
  - Validation with error indicators
  - Auto-reconnect configuration
  - Connection history

#### 2. **Status Window** [Ctrl+M, S]
- **Purpose**: Monitor integration health
- **Live Metrics**:
  - Connection status
  - Messages sent/received
  - Bandwidth usage
  - Error count
  - Uptime statistics

#### 3. **Dashboard** [Ctrl+M, B]
- **Purpose**: Central control center
- **Quick Actions**:
  - One-click connect/disconnect
  - System status overview
  - Quick access to all features
  - Performance summary

#### 4. **Trace Viewer** [Ctrl+M, T]
- **Purpose**: Real-time execution analysis
- **Capabilities**:
  - Live execution trace display
  - Color-coded instruction types
  - Advanced filtering
  - Export functionality
  - Performance metrics

#### 5. **Advanced Settings** [Ctrl+M, A]
- **Purpose**: Fine-tune integration behavior
- **Options**:
  - Network timeouts
  - Logging verbosity
  - Performance parameters
  - Debug options

---

## 🔄 Workflow Examples

### Example 1: Basic ROM Analysis

#### Setup Phase
```
1. Launch Mesen2 → Load ROM → Enable DiztinGUIsh Server
2. Launch DiztinGUIsh → Load project → Connect to Mesen2
3. Verify connection in Status Window
```

#### Analysis Phase
```
1. Set breakpoints in DiztinGUIsh
2. Run game in Mesen2
3. Monitor execution in Trace Viewer
4. Analyze CPU states and memory access
5. Export trace data for further analysis
```

#### Results
- **Real-time disassembly** updates as code executes
- **Memory access patterns** clearly visible
- **Call flow analysis** with stack traces
- **Performance bottleneck** identification

### Example 2: Advanced Debugging Session

#### Pre-Session Setup
1. **Configure Advanced Settings**:
   ```json
   {
     "EnableExecutionTrace": true,
     "TraceFrameInterval": 1,
     "MaxTracesPerFrame": 1000,
     "EnableMemoryAccess": true,
     "EnableCdlUpdates": true
   }
   ```

2. **Set Up Breakpoints**:
   - Function entry points
   - Memory access regions
   - Specific addresses of interest

#### Active Debugging
1. **Real-time Monitoring**:
   - Watch Trace Viewer for execution flow
   - Monitor Status Window for performance
   - Use Dashboard for quick controls

2. **Data Collection**:
   - CPU state snapshots
   - Memory dumps at key points
   - Execution traces with timing
   - CDL (Code/Data/Graphics) updates

3. **Analysis Tools**:
   - Filter traces by address ranges
   - Search for specific opcodes
   - Export data in multiple formats
   - Generate performance reports

#### Post-Session Analysis
1. **Export Data**: CSV, TXT, or binary formats
2. **Generate Reports**: Execution summaries and statistics
3. **Save Session**: Configuration and findings for future reference

---

## 🚀 Advanced Features

### Real-Time CPU State Monitoring

#### Configuration
```json
{
  "CpuStateTracking": {
    "EnableAutoUpdate": true,
    "UpdateIntervalMs": 16,
    "TrackRegisters": ["A", "X", "Y", "SP", "DP", "DB", "PC"],
    "TrackFlags": ["N", "V", "M", "X", "D", "I", "Z", "C"]
  }
}
```

#### Usage
```csharp
// Subscribe to CPU state changes
client.CpuStateReceived += (sender, e) => {
    var state = e.CpuState;
    Console.WriteLine($"PC: ${state.PC:X6}, A: ${state.A:X4}");
};
```

### Memory Synchronization

#### Live Memory Dumps
```csharp
// Request specific memory regions
await client.RequestMemoryDumpAsync(
    memoryType: MemoryType.SnesWram,
    startAddress: 0x7E0000,
    length: 0x10000
);
```

#### Auto-Sync Configuration
```json
{
  "MemorySync": {
    "EnableAutoSync": true,
    "SyncIntervalMs": 100,
    "WatchRegions": [
      {"start": "0x7E0000", "end": "0x7FFFFF", "name": "WRAM"},
      {"start": "0x000000", "end": "0x001FFF", "name": "LowRAM"}
    ]
  }
}
```

### Breakpoint Management

#### Programmatic Breakpoints
```csharp
// Add execution breakpoint
await client.AddBreakpointAsync(new Mesen2Breakpoint {
    Address = 0x808000,
    Type = BreakpointType.Execute,
    Enabled = true,
    Condition = "A == 0x1234"
});

// Memory access breakpoint
await client.AddBreakpointAsync(new Mesen2Breakpoint {
    Address = 0x7E0100,
    Type = BreakpointType.WriteMemory,
    Enabled = true
});
```

### Label Synchronization

#### Automated Label Sync
```csharp
// Sync all labels from DiztinGUIsh to Mesen2
await client.SyncLabelsAsync();

// Add specific label
await client.AddLabelAsync(0x808000, "MainGameLoop", LabelType.Function);
```

---

## ⚡ Performance Optimization

### Network Optimization

#### Bandwidth Management
```json
{
  "Performance": {
    "MaxBandwidthKbps": 1024,
    "CompressionLevel": 6,
    "BufferSize": 8192,
    "BatchUpdates": true,
    "BatchIntervalMs": 16
  }
}
```

#### Latency Reduction
```json
{
  "Network": {
    "TcpNoDelay": true,
    "SocketBufferSize": 65536,
    "UseKeepalive": true,
    "KeepaliveIntervalMs": 1000
  }
}
```

### Trace Performance

#### Optimized Trace Settings
```json
{
  "TraceOptimization": {
    "MaxTracesPerSecond": 60000,
    "FilterDuplicates": true,
    "CompressTraces": true,
    "CircularBufferSize": 100000,
    "EnableVirtualScrolling": true
  }
}
```

#### Memory Usage Control
```json
{
  "MemoryManagement": {
    "MaxMemoryMB": 512,
    "GcCollectionInterval": 5000,
    "PurgeOldTraces": true,
    "MaxTraceAge": 300000
  }
}
```

### CPU Usage Optimization

#### Thread Management
```json
{
  "Threading": {
    "WorkerThreadCount": 2,
    "UseBackgroundThreads": true,
    "ThreadPriority": "Normal",
    "MaxConcurrentOperations": 4
  }
}
```

---

## 🔧 Troubleshooting

### Connection Issues

#### Problem: Cannot Connect to Mesen2
**Symptoms:**
- Connection dialog shows "Connection Failed"
- Status window indicates "Disconnected"
- Error message: "Connection refused"

**Solutions:**
1. **Verify Mesen2 Server**:
   ```
   ✓ Mesen2 is running
   ✓ ROM is loaded
   ✓ DiztinGUIsh server is started (Tools → DiztinGUIsh → Start Server)
   ✓ Port 1234 is not blocked by firewall
   ```

2. **Check Network Configuration**:
   ```powershell
   # Test port connectivity
   Test-NetConnection -ComputerName localhost -Port 1234
   
   # Check firewall rules
   Get-NetFirewallRule -DisplayName "*Mesen*"
   ```

3. **Verify Configuration**:
   ```json
   {
     "DefaultHost": "localhost",
     "DefaultPort": 1234,
     "ConnectionTimeoutMs": 5000
   }
   ```

#### Problem: Frequent Disconnections
**Symptoms:**
- Connection drops every few minutes
- Auto-reconnect constantly triggering
- Network error messages

**Solutions:**
1. **Increase Timeouts**:
   ```json
   {
     "ConnectionTimeoutMs": 10000,
     "AutoReconnectDelayMs": 5000,
     "MaxReconnectAttempts": 10
   }
   ```

2. **Check Network Stability**:
   ```powershell
   # Monitor network latency
   ping -t localhost
   
   # Check for packet loss
   pathping localhost
   ```

### Performance Issues

#### Problem: High CPU Usage
**Symptoms:**
- CPU usage above 50%
- UI becomes unresponsive
- Trace viewer lags behind

**Solutions:**
1. **Reduce Trace Frequency**:
   ```json
   {
     "TraceFrameInterval": 5,
     "MaxTracesPerFrame": 100,
     "EnableVirtualScrolling": true
   }
   ```

2. **Optimize Memory Usage**:
   ```json
   {
     "CircularBufferSize": 10000,
     "PurgeOldTraces": true,
     "MaxTraceAge": 60000
   }
   ```

#### Problem: Memory Leaks
**Symptoms:**
- Memory usage constantly increasing
- Application crashes after extended use
- System becomes slow

**Solutions:**
1. **Enable Memory Management**:
   ```json
   {
     "MaxMemoryMB": 256,
     "GcCollectionInterval": 2000,
     "ForceGcOnLowMemory": true
   }
   ```

2. **Monitor Memory Usage**:
   ```csharp
   // Check memory usage programmatically
   var memoryUsage = GC.GetTotalMemory(false);
   if (memoryUsage > maxMemoryBytes) {
       GC.Collect();
   }
   ```

### Data Integrity Issues

#### Problem: Missing or Corrupt Trace Data
**Symptoms:**
- Gaps in execution traces
- Incorrect CPU state information
- Checksum errors

**Solutions:**
1. **Enable Data Validation**:
   ```json
   {
     "EnableChecksums": true,
     "ValidateTraceData": true,
     "RetransmitOnError": true
   }
   ```

2. **Increase Buffer Sizes**:
   ```json
   {
     "SocketBufferSize": 131072,
     "ApplicationBufferSize": 65536
   }
   ```

---

## ❓ FAQ

### General Questions

**Q: Is the integration compatible with all Mesen2 versions?**
A: The integration requires Mesen2 with DiztinGUIsh support. Check the compatibility matrix in the release notes.

**Q: Can I use the integration with ROM hacks?**
A: Yes! The integration works with any ROM that runs in Mesen2, including ROM hacks and homebrew.

**Q: Does the integration affect emulation performance?**
A: Minimal impact when optimally configured. Disable unnecessary features for maximum performance.

### Technical Questions

**Q: What network protocols are used?**
A: TCP/IP with a custom binary protocol for efficient data transmission.

**Q: Can I connect multiple DiztinGUIsh instances?**
A: Currently, only one connection per Mesen2 instance is supported.

**Q: How much bandwidth does the integration use?**
A: Typically 10-100 KB/s depending on trace frequency and features enabled.

### Troubleshooting Questions

**Q: Why does connection fail on first attempt but succeed on retry?**
A: This can happen if Mesen2's server takes time to initialize. Enable auto-reconnect or wait a few seconds.

**Q: Can I use the integration over a network?**
A: Yes, but ensure proper firewall configuration and consider security implications.

**Q: What should I do if traces appear corrupted?**
A: Enable data validation, increase buffer sizes, and check network connectivity.

---

## 🎥 Video Tutorials

### Tutorial 1: Basic Setup (5 minutes)
**Topics Covered:**
- Installing prerequisites
- Initial configuration
- First connection
- Basic usage overview

**Link:** `[To be created - Basic Setup Tutorial]`

### Tutorial 2: Advanced Debugging (15 minutes)
**Topics Covered:**
- Setting up complex debugging scenarios
- Using advanced features
- Performance optimization
- Troubleshooting common issues

**Link:** `[To be created - Advanced Debugging Tutorial]`

### Tutorial 3: Real-World Analysis (20 minutes)
**Topics Covered:**
- Complete ROM analysis workflow
- Finding and fixing performance issues
- Documenting discoveries
- Sharing results

**Link:** `[To be created - Real-World Analysis Tutorial]`

---

## 📞 Support and Resources

### Documentation
- **API Reference**: [API_REFERENCE.md](API_REFERENCE.md)
- **Developer Guide**: [DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md)
- **Troubleshooting**: [TROUBLESHOOTING.md](TROUBLESHOOTING.md)

### Community
- **GitHub Issues**: Report bugs and request features
- **Discord Server**: `[Community server link]`
- **Forum**: `[Community forum link]`

### Contributing
- **Source Code**: Available on GitHub
- **Bug Reports**: Use the issue tracker
- **Feature Requests**: Submit detailed proposals
- **Pull Requests**: Follow contribution guidelines

---

## 🏷️ Version Information

**Current Version:** v1.0.0
**Last Updated:** November 18, 2025
**Compatibility:** Mesen2 v0.7.0+, DiztinGUIsh v1.0+
**Platform:** Windows 10/11, .NET 9.0+

---

*This guide is continuously updated. Check for the latest version regularly!*