# GitHub Issues Creation Script for Mesen2 DiztinGUIsh Integration
# This script uses GitHub CLI (gh) to create all issues programmatically
# Install GitHub CLI: https://cli.github.com/

# Repository configuration
$REPO = "TheAnsarya/Mesen2"
$PROJECT_URL = "https://github.com/users/TheAnsarya/projects/4"

# Check if gh is installed
if (!(Get-Command gh -ErrorAction SilentlyContinue)) {
    Write-Error "GitHub CLI (gh) is not installed. Please install from https://cli.github.com/"
    exit 1
}

# Check if authenticated
$authStatus = gh auth status 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Error "Not authenticated with GitHub. Run 'gh auth login' first."
    exit 1
}

Write-Host "Creating GitHub issues for Mesen2 DiztinGUIsh Integration..." -ForegroundColor Green
Write-Host "Repository: $REPO" -ForegroundColor Cyan
Write-Host "Project Board: $PROJECT_URL" -ForegroundColor Cyan
Write-Host ""

# Store issue numbers for cross-referencing
$issueNumbers = @{}

# Function to create issue and add to project
function Create-Issue {
    param(
        [string]$Title,
        [string]$Body,
        [string[]]$Labels,
        [string]$Milestone = $null,
        [string]$ParentIssue = $null
    )
    
    Write-Host "Creating: $Title" -ForegroundColor Yellow
    
    # Build the command
    $cmd = "gh issue create --repo $REPO --title `"$Title`" --body `"$Body`""
    
    # Add labels
    if ($Labels) {
        $labelString = $Labels -join ","
        $cmd += " --label `"$labelString`""
    }
    
    # Add milestone
    if ($Milestone) {
        $cmd += " --milestone `"$Milestone`""
    }
    
    # Execute command and capture issue URL
    $issueUrl = Invoke-Expression $cmd
    
    if ($LASTEXITCODE -eq 0) {
        $issueNumber = ($issueUrl -split '/')[-1]
        Write-Host "  ✓ Created issue #$issueNumber" -ForegroundColor Green
        
        # Add to project board
        # Note: This requires gh project extension or manual linking
        Write-Host "  → Please manually link to project: $PROJECT_URL" -ForegroundColor DarkGray
        
        return $issueNumber
    } else {
        Write-Host "  ✗ Failed to create issue" -ForegroundColor Red
        return $null
    }
}

Write-Host "═══════════════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host "STEP 1: Creating Epic Issues" -ForegroundColor Cyan
Write-Host "═══════════════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host ""

# Epic #4: Real-Time Streaming Integration (HIGHEST PRIORITY)
$epic4Body = @"
Implement real-time bidirectional streaming integration between Mesen2 and DiztinGUIsh using TCP sockets. This enables live disassembly updates, execution trace streaming, CDL synchronization, and bidirectional debug control.

## Background

While CDL export/import and label exchange provide offline workflows, real-time streaming creates a superior debugging experience by:
- Showing code execution in real-time during emulation
- Automatically detecting CPU context (M/X flags) from execution traces
- Enabling bidirectional control (set breakpoints, push labels, request memory dumps)
- Eliminating manual export/import steps

Mesen2 already has robust socket infrastructure (\`Utilities/Socket.h\`) used for netplay, which can be leveraged for this integration.

**CRITICAL DISCOVERY:** DiztinGUIsh already has socket-based tracelog capture from their BSNES integration! This validates our entire approach and means we can reuse existing client infrastructure.

## Goals

- Implement DiztinGUIsh Bridge server in Mesen2
- Implement Mesen2 Client in DiztinGUIsh  
- Design and implement binary message protocol
- Stream execution traces with CPU context (PC, opcode, M/X flags, DB/DP registers)
- Stream memory access events for CDL generation
- Stream CPU state snapshots and CDL updates
- Enable bidirectional commands (breakpoints, memory requests, label sync)
- Implement connection lifecycle (handshake, heartbeat, disconnect)
- Add UI for connection management in both tools
- Optimize performance for low-latency streaming

## Success Criteria

- [ ] Mesen2 can accept connections from DiztinGUIsh on configurable port
- [ ] Handshake protocol successfully negotiates protocol version and ROM
- [ ] Execution traces stream in real-time at 15+ Hz (every 4 frames at 60 FPS)
- [ ] M/X flag detection from traces achieves 95%+ accuracy
- [ ] CDL data synchronizes correctly between tools
- [ ] Labels can be pushed bidirectionally without data loss
- [ ] Breakpoints set from DiztinGUIsh trigger correctly in Mesen2
- [ ] Latency < 100ms for local connections
- [ ] Bandwidth < 200 KB/s with default settings
- [ ] No crashes or memory leaks in 24-hour connection test
- [ ] User documentation and troubleshooting guide completed
- [ ] Integration tests pass for all message types

## References

- [Streaming Integration Technical Spec](https://github.com/$REPO/blob/master/docs/diztinguish/streaming-integration.md)
- [DiztinGUIsh Repository Analysis](https://github.com/$REPO/blob/master/docs/diztinguish/diztinguish-repository-analysis.md)
- [API Documentation](https://github.com/$REPO/blob/master/docs/README.md)

## Timeline

**Phase 1: Foundation (Weeks 1-4)**
- DiztinGUIsh Bridge infrastructure
- Message protocol implementation
- Execution trace streaming
- CDL integration

**Phase 2: Synchronization (Weeks 5-8)**
- State snapshots
- Label bidirectional sync
- Breakpoint control

**Phase 3: Polish (Weeks 9-12)**
- UI improvements
- Error handling
- Reconnection logic

**Phase 4: Optimization (Weeks 13-16)**
- Performance tuning
- Testing
- Documentation

**Priority:** 🔥 HIGHEST - This is the recommended long-term integration approach

**Project Board:** $PROJECT_URL
"@

$epic4 = Create-Issue `
    -Title "[EPIC] Real-Time Streaming Integration via Socket Connection" `
    -Body $epic4Body `
    -Labels @("enhancement", "epic", "streaming", "networking", "high-priority")

if ($epic4) {
    $issueNumbers["Epic4"] = $epic4
    Write-Host ""
    Write-Host "⭐ Epic #$epic4 created - This is the PRIMARY integration approach" -ForegroundColor Magenta
    Write-Host "   Please PIN this issue in the GitHub UI" -ForegroundColor DarkGray
    Write-Host ""
}

Start-Sleep -Seconds 2

Write-Host "═══════════════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host "STEP 2: Creating Streaming Implementation Issues" -ForegroundColor Cyan
Write-Host "═══════════════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host ""

# Issue S1: DiztinGUIsh Bridge Infrastructure
$s1Body = @"
Create the core DiztinGUIsh Bridge component in Mesen2 that acts as a TCP server, accepting connections from DiztinGUIsh and managing the streaming session.

## Tasks

- [ ] Create \`Core/Debugger/DiztinguishBridge.h/.cpp\`
- [ ] Create \`Core/Debugger/DiztinguishMessage.h\`
- [ ] Create \`Core/Debugger/DiztinguishProtocol.h\`
- [ ] Implement server socket using existing \`Utilities/Socket\` class
- [ ] Implement connection acceptance logic
- [ ] Implement handshake protocol (version negotiation, ROM checksum)
- [ ] Create thread-safe message queue for outgoing messages
- [ ] Implement message encoding framework
- [ ] Add connection lifecycle management (connect, heartbeat, disconnect)
- [ ] Add error handling and recovery
- [ ] Add unit tests for message encoding/decoding

## Technical Details

- Leverage existing \`Utilities/Socket.h\` (proven in netplay)
- Use separate thread for socket I/O to avoid blocking emulation
- Default port: 9998 (configurable)
- Support only one client connection at a time initially
- Implement graceful disconnect and cleanup

## Files to Create

- \`Core/Debugger/DiztinguishBridge.h\`
- \`Core/Debugger/DiztinguishBridge.cpp\`
- \`Core/Debugger/DiztinguishMessage.h\`
- \`Core/Debugger/DiztinguishProtocol.h\`

## Files to Modify

- \`Core/SNES/Debugger/SnesDebugger.h/.cpp\` (integrate bridge)

**Epic:** #$($issueNumbers["Epic4"])

**Priority:** High  
**Estimated Effort:** 5-7 days
"@

$s1 = Create-Issue `
    -Title "[STREAMING] Create DiztinGUIsh Bridge server infrastructure" `
    -Body $s1Body `
    -Labels @("enhancement", "mesen2", "streaming", "networking", "phase-1") `
    -Milestone "Phase 1 - Streaming Foundation"

if ($s1) { $issueNumbers["S1"] = $s1 }
Start-Sleep -Seconds 2

# Issue S2: Execution Trace Streaming
$s2Body = @"
Implement real-time streaming of CPU execution traces including program counter, opcodes, CPU flags, and register state to enable accurate disassembly in DiztinGUIsh.

## Tasks

- [ ] Hook SNES CPU execution loop
- [ ] Capture per-instruction trace data:
  - [ ] Program Counter (PC)
  - [ ] Opcode and operands
  - [ ] M/X flag state
  - [ ] Data Bank (DB) register
  - [ ] Direct Page (DP) register
  - [ ] CPU flags (NVMXDIZC)
  - [ ] Calculated effective address
- [ ] Implement frame-based batching (collect traces per frame)
- [ ] Add configurable throttling (send every N frames)
- [ ] Implement \`EXEC_TRACE\` message type
- [ ] Add performance optimization (max entries per frame)
- [ ] Minimize emulation overhead
- [ ] Add unit tests

## Message Format

See \`streaming-integration.md\` for \`EXEC_TRACE\` message specification.

## Performance Target

- < 5% emulation overhead with default settings
- Support 1000+ instructions per frame
- Configurable frame interval (default: every 4 frames = 15 Hz)

## Files to Modify

- \`Core/SNES/Snes.h/.cpp\` (CPU execution hooks)
- \`Core/Debugger/DiztinguishBridge.cpp\` (trace encoding)
- \`Core/Debugger/DiztinguishMessage.h\` (message type)

**Epic:** #$($issueNumbers["Epic4"])  
**Depends On:** #$($issueNumbers["S1"])

**Priority:** High  
**Estimated Effort:** 5-7 days
"@

$s2 = Create-Issue `
    -Title "[STREAMING] Stream execution traces from Mesen2 to DiztinGUIsh" `
    -Body $s2Body `
    -Labels @("enhancement", "mesen2", "streaming", "phase-1") `
    -Milestone "Phase 1 - Streaming Foundation"

if ($s2) { $issueNumbers["S2"] = $s2 }
Start-Sleep -Seconds 2

# Issue S3: Memory Access Streaming and CDL
$s3Body = @"
Stream memory access events (read/write/execute) to enable real-time CDL generation in DiztinGUIsh. Also implement differential CDL update streaming.

## Tasks

- [ ] Hook memory access events in SNES core
- [ ] Capture access type (read/write/execute)
- [ ] Capture access address and value
- [ ] Implement \`MEMORY_ACCESS\` message type
- [ ] Implement CDL tracking in SNES core
- [ ] Add differential CDL updates (changed flags only)
- [ ] Implement \`CDL_UPDATE\` message type
- [ ] Add periodic full CDL snapshots
- [ ] Optimize for minimal bandwidth
- [ ] Add unit tests

## Message Types

- \`MEMORY_ACCESS\`: Individual memory operations
- \`CDL_UPDATE\`: Changed CDL flags (differential)
- \`CDL_SNAPSHOT\`: Full CDL state (periodic)

## Performance Target

- Batch memory events per frame
- Differential CDL updates only
- Full snapshot every 60 seconds max

## Files to Modify

- \`Core/SNES/SnesMemoryManager.h/.cpp\` (memory hooks)
- \`Core/Debugger/DiztinguishBridge.cpp\` (CDL tracking)
- \`Core/Debugger/DiztinguishMessage.h\` (message types)

**Epic:** #$($issueNumbers["Epic4"])  
**Depends On:** #$($issueNumbers["S1"])

**Priority:** High  
**Estimated Effort:** 5-7 days
"@

$s3 = Create-Issue `
    -Title "[STREAMING] Stream memory access events and CDL updates" `
    -Body $s3Body `
    -Labels @("enhancement", "mesen2", "streaming", "cdl", "phase-1") `
    -Milestone "Phase 1 - Streaming Foundation"

if ($s3) { $issueNumbers["S3"] = $s3 }
Start-Sleep -Seconds 2

# Issue S4: CPU State Snapshots
$s4Body = @"
Implement periodic CPU state snapshots to synchronize CPU context (registers, flags, stack) between Mesen2 and DiztinGUIsh.

## Tasks

- [ ] Define CPU state structure
- [ ] Capture all 65816 registers (A, X, Y, S, D, DB, PC)
- [ ] Capture processor flags (NVMXDIZC, E)
- [ ] Implement \`CPU_STATE\` message type
- [ ] Add configurable snapshot interval
- [ ] Add on-demand state requests
- [ ] Optimize structure packing
- [ ] Add unit tests

## CPU State Structure

\`\`\`cpp
struct CpuStateSnapshot {
    uint16_t A;      // Accumulator
    uint16_t X;      // X register
    uint16_t Y;      // Y register
    uint16_t S;      // Stack pointer
    uint16_t D;      // Direct page
    uint8_t  DB;     // Data bank
    uint24_t PC;     // Program counter
    uint8_t  P;      // Processor status
    uint8_t  E;      // Emulation mode
};
\`\`\`

## Files to Modify

- \`Core/SNES/Snes.h/.cpp\` (state capture)
- \`Core/Debugger/DiztinguishBridge.cpp\` (state encoding)
- \`Core/Debugger/DiztinguishMessage.h\` (message type)

**Epic:** #$($issueNumbers["Epic4"])  
**Depends On:** #$($issueNumbers["S1"])

**Priority:** Medium  
**Estimated Effort:** 3-5 days
"@

$s4 = Create-Issue `
    -Title "[STREAMING] Implement CPU state snapshot streaming" `
    -Body $s4Body `
    -Labels @("enhancement", "mesen2", "streaming", "phase-1") `
    -Milestone "Phase 1 - Streaming Foundation"

if ($s4) { $issueNumbers["S4"] = $s4 }
Start-Sleep -Seconds 2

# Issue S5: Label Bidirectional Sync
$s5Body = @"
Implement bidirectional label synchronization between Mesen2 and DiztinGUIsh, allowing labels to be created/updated in either tool and automatically synchronized.

## Tasks

- [ ] Define label sync message format
- [ ] Implement \`LABEL_ADD\` message type
- [ ] Implement \`LABEL_UPDATE\` message type
- [ ] Implement \`LABEL_DELETE\` message type
- [ ] Implement \`LABEL_SYNC_REQUEST\` message type
- [ ] Add label change detection in Mesen2
- [ ] Add label import from DiztinGUIsh
- [ ] Handle label conflicts (same address, different name)
- [ ] Add bulk label sync on connection
- [ ] Add unit tests

## Label Message Format

\`\`\`cpp
struct LabelMessage {
    uint32_t address;     // SNES address
    uint8_t  type;        // Code, Data, etc.
    char     name[256];   // Label name
};
\`\`\`

## Conflict Resolution

- Newest timestamp wins
- Option to prefer Mesen2 or DiztinGUIsh labels
- User prompt for important conflicts

## Files to Modify

- \`Core/Debugger/DiztinguishBridge.cpp\` (label sync)
- \`Core/Debugger/DiztinguishMessage.h\` (message types)
- \`Core/SNES/Debugger/SnesDebugger.cpp\` (label management)

**Epic:** #$($issueNumbers["Epic4"])  
**Depends On:** #$($issueNumbers["S1"])

**Priority:** Medium  
**Estimated Effort:** 5-7 days
"@

$s5 = Create-Issue `
    -Title "[STREAMING] Implement bidirectional label synchronization" `
    -Body $s5Body `
    -Labels @("enhancement", "mesen2", "streaming", "labels", "phase-2") `
    -Milestone "Phase 2 - Synchronization"

if ($s5) { $issueNumbers["S5"] = $s5 }
Start-Sleep -Seconds 2

# Issue S6: Breakpoint Control
$s6Body = @"
Enable DiztinGUIsh to set and control breakpoints in Mesen2, allowing users to debug from the disassembly view.

## Tasks

- [ ] Implement \`BREAKPOINT_ADD\` message type
- [ ] Implement \`BREAKPOINT_REMOVE\` message type
- [ ] Implement \`BREAKPOINT_HIT\` message type (Mesen2 → DiztinGUIsh)
- [ ] Add breakpoint registration in Mesen2
- [ ] Add breakpoint removal
- [ ] Send breakpoint hit notifications
- [ ] Support execution and access breakpoints
- [ ] Add conditional breakpoint support (future)
- [ ] Add unit tests

## Breakpoint Types

- **Execution:** Break when PC reaches address
- **Read:** Break when address is read
- **Write:** Break when address is written
- **Access:** Break on read or write

## Workflow

1. User sets breakpoint in DiztinGUIsh disassembly view
2. DiztinGUIsh sends \`BREAKPOINT_ADD\` to Mesen2
3. Mesen2 registers breakpoint
4. When hit, Mesen2 pauses and sends \`BREAKPOINT_HIT\`
5. DiztinGUIsh highlights line and shows CPU state

## Files to Modify

- \`Core/Debugger/DiztinguishBridge.cpp\` (breakpoint messages)
- \`Core/Debugger/DiztinguishMessage.h\` (message types)
- \`Core/SNES/Debugger/SnesDebugger.cpp\` (breakpoint management)

**Epic:** #$($issueNumbers["Epic4"])  
**Depends On:** #$($issueNumbers["S1"]), #$($issueNumbers["S4"])

**Priority:** Medium  
**Estimated Effort:** 5-7 days
"@

$s6 = Create-Issue `
    -Title "[STREAMING] Enable breakpoint control from DiztinGUIsh" `
    -Body $s6Body `
    -Labels @("enhancement", "mesen2", "streaming", "debugging", "phase-2") `
    -Milestone "Phase 2 - Synchronization"

if ($s6) { $issueNumbers["S6"] = $s6 }
Start-Sleep -Seconds 2

# Issue S7: Memory Dump Requests
$s7Body = @"
Allow DiztinGUIsh to request memory dumps from Mesen2 for analysis without requiring file export/import.

## Tasks

- [ ] Implement \`MEMORY_DUMP_REQUEST\` message type
- [ ] Implement \`MEMORY_DUMP_RESPONSE\` message type
- [ ] Support ROM dumps
- [ ] Support RAM dumps (WRAM, SRAM)
- [ ] Support VRAM dumps
- [ ] Support range-based requests
- [ ] Add compression for large dumps (optional)
- [ ] Add unit tests

## Request Format

\`\`\`cpp
struct MemoryDumpRequest {
    uint8_t  type;        // ROM, RAM, VRAM, etc.
    uint32_t start_addr;  // Start address
    uint32_t length;      // Number of bytes
};
\`\`\`

## Response Format

\`\`\`cpp
struct MemoryDumpResponse {
    uint8_t  type;
    uint32_t start_addr;
    uint32_t length;
    uint8_t  data[];      // Variable length
};
\`\`\`

## Files to Modify

- \`Core/Debugger/DiztinguishBridge.cpp\` (memory dump handling)
- \`Core/Debugger/DiztinguishMessage.h\` (message types)
- \`Core/SNES/SnesMemoryManager.cpp\` (memory access)

**Epic:** #$($issueNumbers["Epic4"])  
**Depends On:** #$($issueNumbers["S1"])

**Priority:** Low  
**Estimated Effort:** 3-5 days
"@

$s7 = Create-Issue `
    -Title "[STREAMING] Implement memory dump request/response" `
    -Body $s7Body `
    -Labels @("enhancement", "mesen2", "streaming", "memory", "phase-3") `
    -Milestone "Phase 3 - Advanced Features"

if ($s7) { $issueNumbers["S7"] = $s7 }
Start-Sleep -Seconds 2

# Issue S8: Connection UI in Mesen2
$s8Body = @"
Add user interface in Mesen2 for managing the DiztinGUIsh connection.

## Tasks

- [ ] Add "DiztinGUIsh" menu to debugger
- [ ] Add "Start Server" / "Stop Server" menu items
- [ ] Add connection status indicator
- [ ] Add configuration dialog:
  - [ ] Port number setting
  - [ ] Auto-start option
  - [ ] Streaming options (trace frequency, CDL updates, etc.)
- [ ] Add connection log window
- [ ] Add statistics display (messages sent/received, bandwidth)
- [ ] Add error notifications
- [ ] Write user documentation

## UI Mockup

\`\`\`
Debugger Menu
├── File
├── ...
└── DiztinGUIsh                    <-- NEW
    ├── Start Server               <-- NEW
    ├── Stop Server                <-- NEW
    ├── Configuration...           <-- NEW
    ├── Connection Log...          <-- NEW
    └── About DiztinGUIsh         <-- NEW
\`\`\`

## Files to Create/Modify

- \`UI/Debugger/DiztinguishConnectionWindow.axaml\`
- \`UI/Debugger/DiztinguishConnectionWindow.axaml.cs\`
- \`UI/Debugger/ViewModels/DiztinguishConnectionViewModel.cs\`

**Epic:** #$($issueNumbers["Epic4"])  
**Depends On:** #$($issueNumbers["S1"])

**Priority:** Medium  
**Estimated Effort:** 5-7 days
"@

$s8 = Create-Issue `
    -Title "[STREAMING] Add connection UI in Mesen2" `
    -Body $s8Body `
    -Labels @("enhancement", "mesen2", "ui", "streaming", "phase-3") `
    -Milestone "Phase 3 - Polish"

if ($s8) { $issueNumbers["S8"] = $s8 }
Start-Sleep -Seconds 2

# Issue S9: DiztinGUIsh Client Implementation
$s9Body = @"
Implement Mesen2 client in DiztinGUIsh to connect to Mesen2's streaming server.

**NOTE:** This issue is for the DiztinGUIsh repository, but tracked here for visibility.

## Tasks

- [ ] Create \`Diz.Import.Mesen\` project
- [ ] Implement \`MesenProtocol.cs\` (message definitions)
- [ ] Implement \`MesenLiveTraceClient.cs\` (TCP client)
- [ ] Implement message decoder
- [ ] Process \`EXEC_TRACE\` messages → update IData
- [ ] Process \`CDL_UPDATE\` messages → update CDL
- [ ] Process \`MEMORY_ACCESS\` messages
- [ ] Process \`CPU_STATE\` messages
- [ ] Implement label sync (send/receive)
- [ ] Implement breakpoint control (send \`BREAKPOINT_ADD\`, receive \`BREAKPOINT_HIT\`)
- [ ] Add connection UI (host, port, connect/disconnect)
- [ ] Add status indicator
- [ ] Add dependency injection registration
- [ ] Add unit tests

## Architecture

Leverage existing DiztinGUIsh BSNES socket integration pattern.

## Files to Create

- \`Diz.Import.Mesen/MesenLiveTraceClient.cs\`
- \`Diz.Import.Mesen/MesenProtocol.cs\`
- \`Diz.Import.Mesen/MesenConnectionDialog.cs\`
- \`Diz.Import.Mesen/ServiceRegistration.cs\`

## Files to Modify

- \`Diz.Controllers/controllers/IProjectController.cs\` (add Mesen connection)

**Epic:** #$($issueNumbers["Epic4"])  
**Repository:** IsoFrieze/DiztinGUIsh

**Priority:** High  
**Estimated Effort:** 7-10 days
"@

$s9 = Create-Issue `
    -Title "[STREAMING] Implement Mesen2 client in DiztinGUIsh" `
    -Body $s9Body `
    -Labels @("enhancement", "diztinguish", "streaming", "phase-1") `
    -Milestone "Phase 1 - Streaming Foundation"

if ($s9) { $issueNumbers["S9"] = $s9 }
Start-Sleep -Seconds 2

# Issue S10: Integration Testing and Documentation
$s10Body = @"
Create comprehensive integration tests and user documentation for the Mesen2-DiztinGUIsh streaming integration.

## Tasks

### Integration Tests
- [ ] Test handshake protocol (version mismatch, ROM mismatch)
- [ ] Test execution trace streaming (accuracy, performance)
- [ ] Test CDL synchronization (correctness)
- [ ] Test label bidirectional sync (conflicts, bulk sync)
- [ ] Test breakpoint control (set, hit, remove)
- [ ] Test connection lifecycle (connect, disconnect, reconnect)
- [ ] Test error handling (network errors, protocol errors)
- [ ] Test performance (24-hour stability test)
- [ ] Test with various ROMs (LoROM, HiROM, SA-1, SuperFX)

### Performance Benchmarks
- [ ] Measure emulation overhead (target: < 5%)
- [ ] Measure bandwidth (target: < 200 KB/s)
- [ ] Measure latency (target: < 100ms)
- [ ] Measure memory usage

### Documentation
- [ ] User guide: "Getting Started with Live Tracing"
- [ ] User guide: "Troubleshooting Connection Issues"
- [ ] Technical spec: Protocol reference
- [ ] Architecture document: System design
- [ ] FAQ

## Test ROMs

- Super Mario World (LoROM)
- Super Metroid (HiROM)
- Yoshi's Island (SuperFX)
- Super Mario RPG (SA-1)

## Files to Create

- \`Tests/Integration/DiztinguishStreamingTests.cpp\`
- \`docs/diztinguish/user-guide.md\`
- \`docs/diztinguish/troubleshooting.md\`
- \`docs/diztinguish/protocol-reference.md\`

**Epic:** #$($issueNumbers["Epic4"])  
**Depends On:** All S1-S9

**Priority:** Medium  
**Estimated Effort:** 5-7 days
"@

$s10 = Create-Issue `
    -Title "[STREAMING] Integration testing and documentation" `
    -Body $s10Body `
    -Labels @("enhancement", "testing", "documentation", "streaming", "phase-4") `
    -Milestone "Phase 4 - Testing & Documentation"

if ($s10) { $issueNumbers["S10"] = $s10 }

Write-Host ""
Write-Host "═══════════════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host "SUMMARY" -ForegroundColor Cyan
Write-Host "═══════════════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host ""

Write-Host "Created Issues:" -ForegroundColor Green
foreach ($key in $issueNumbers.Keys | Sort-Object) {
    $num = $issueNumbers[$key]
    Write-Host "  $key -> #$num" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Next Steps:" -ForegroundColor Cyan
Write-Host "  1. Visit $PROJECT_URL" -ForegroundColor White
Write-Host "  2. Link each issue to the project board" -ForegroundColor White
Write-Host "  3. Pin Epic #$($issueNumbers['Epic4']) (highest priority)" -ForegroundColor White
Write-Host "  4. Create milestones:" -ForegroundColor White
Write-Host "     - Phase 1 - Streaming Foundation" -ForegroundColor DarkGray
Write-Host "     - Phase 2 - Synchronization" -ForegroundColor DarkGray
Write-Host "     - Phase 3 - Polish" -ForegroundColor DarkGray
Write-Host "     - Phase 4 - Testing & Documentation" -ForegroundColor DarkGray
Write-Host ""
Write-Host "✓ All issues created successfully!" -ForegroundColor Green
