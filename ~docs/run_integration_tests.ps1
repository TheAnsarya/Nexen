#!/usr/bin/env pwsh
<#
.SYNOPSIS
DiztinGUIsh Integration Test Runner (PowerShell)

.DESCRIPTION
PowerShell script for running DiztinGUIsh-Mesen2 integration tests.
This script validates the streaming protocol implementation.

.PARAMETER TestType
The type of test to run: all, connect, cpu, breakpoints, memory (default: all)

.PARAMETER Host  
The server host address (default: 127.0.0.1)

.PARAMETER Port
The server port (default: 9998)

.EXAMPLE
.\run_integration_tests.ps1
.\run_integration_tests.ps1 -TestType cpu
.\run_integration_tests.ps1 -TestType all -Port 9999

.NOTES
Author: GitHub Copilot
Date: November 18, 2025
#>

param(
    [ValidateSet("all", "connect", "cpu", "breakpoints", "memory", "manual", "automated")]
    [string]$TestType = "all",
    
    [string]$Host = "127.0.0.1",
    
    [int]$Port = 9998,
    
    [string]$MesenPath = "",
    
    [switch]$Verbose
)

Write-Host "DiztinGUIsh-Mesen2 Integration Test Runner" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host ""

# Check if Python is available
try {
    $pythonVersion = python --version 2>&1
    Write-Host "✅ Found Python: $pythonVersion" -ForegroundColor Green
} catch {
    Write-Host "❌ Error: Python is not installed or not in PATH" -ForegroundColor Red
    Write-Host "Please install Python 3.6+ to run integration tests" -ForegroundColor Yellow
    Read-Host "Press Enter to exit"
    exit 1
}

Write-Host "Running test type: $TestType" -ForegroundColor White
Write-Host ""

# Get the directory where this script is located
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

Write-Host ""

# Handle different test types
if ($TestType -eq "manual") {
    # Run manual integration test
    Write-Host "Starting manual integration test..." -ForegroundColor Cyan
    Write-Host ""
    
    $testClientPath = Join-Path $scriptDir "manual_integration_test.py"
    
    if (-not (Test-Path $testClientPath)) {
        Write-Host "❌ Manual test client not found: $testClientPath" -ForegroundColor Red
        Read-Host "Press Enter to exit"
        exit 1
    }
    
    $arguments = @()
    if ($Verbose) { $arguments += "--verbose" }
    $process = Start-Process -FilePath "python" -ArgumentList ($testClientPath + $arguments) -Wait -NoNewWindow -PassThru
    
} elseif ($TestType -eq "automated") {
    # Run automated integration test with Mesen2 startup
    Write-Host "Starting automated integration test..." -ForegroundColor Cyan
    Write-Host ""
    
    $testClientPath = Join-Path $scriptDir "automated_integration_test.py"
    
    if (-not (Test-Path $testClientPath)) {
        Write-Host "❌ Automated test client not found: $testClientPath" -ForegroundColor Red
        Read-Host "Press Enter to exit"
        exit 1
    }
    
    $arguments = @("--port", $Port.ToString())
    if ($MesenPath -ne "") { $arguments += @("--mesen-path", $MesenPath) }
    if ($Verbose) { $arguments += "--verbose" }
    $process = Start-Process -FilePath "python" -ArgumentList ($testClientPath + $arguments) -Wait -NoNewWindow -PassThru
    
} else {
    # Run standard protocol tests (requires server already running)
    
    # Check if Mesen2 DiztinGUIsh server is running
    Write-Host "Checking if Mesen2 DiztinGUIsh server is running..." -ForegroundColor Yellow
    try {
        $tcpClient = New-Object System.Net.Sockets.TcpClient
        $tcpClient.ReceiveTimeout = 2000
        $tcpClient.SendTimeout = 2000
        $tcpClient.Connect($Host, $Port)
        $tcpClient.Close()
        Write-Host "✅ Server is running on ${Host}:${Port}" -ForegroundColor Green
    } catch {
        Write-Host "❌ Mesen2 DiztinGUIsh server is not running" -ForegroundColor Red
        Write-Host ""
        Write-Host "Please follow these steps:" -ForegroundColor Yellow
        Write-Host "1. Start Mesen2" -ForegroundColor White
        Write-Host "2. Load a SNES ROM" -ForegroundColor White  
        Write-Host "3. Go to Tools → DiztinGUIsh Server" -ForegroundColor White
        Write-Host "4. Click 'Start Server' (default port 9998)" -ForegroundColor White
        Write-Host "5. Verify server status shows 'Running'" -ForegroundColor White
        Write-Host ""
        Write-Host "Alternatively, run with -TestType manual or -TestType automated" -ForegroundColor Cyan
        Write-Host ""
        Read-Host "Press Enter to exit"
        exit 1
    }

    Write-Host ""
    
    # Run the Python test client
    Write-Host "Starting integration tests..." -ForegroundColor Cyan
    Write-Host ""

    $testClientPath = Join-Path $scriptDir "test_diztinguish_client.py"

    if (-not (Test-Path $testClientPath)) {
        Write-Host "❌ Test client not found: $testClientPath" -ForegroundColor Red
        Read-Host "Press Enter to exit"
        exit 1
    }

    $arguments = @("--test", $TestType, "--host", $Host, "--port", $Port.ToString())
    $process = Start-Process -FilePath "python" -ArgumentList ($testClientPath, $arguments) -Wait -NoNewWindow -PassThru
}

if ($process.ExitCode -eq 0) {
    Write-Host ""
    Write-Host "✅ Integration tests completed successfully!" -ForegroundColor Green
} else {
    Write-Host ""
    Write-Host "❌ Integration tests failed!" -ForegroundColor Red
}

Write-Host ""
Read-Host "Press Enter to exit"