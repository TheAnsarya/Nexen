@echo off
REM DiztinGUIsh Integration Test Runner
REM Usage: run_integration_tests.bat [test_type]
REM   test_type: all, connect, cpu, breakpoints, memory (default: all)

setlocal EnableDelayedExpansion

echo DiztinGUIsh-Mesen2 Integration Test Runner
echo ==========================================

REM Check if Python is available
python --version >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: Python is not installed or not in PATH
    echo Please install Python 3.6+ to run integration tests
    pause
    exit /b 1
)

REM Set test type (default to 'all' if not specified)
set test_type=%1
if "!test_type!"=="" set test_type=all

echo Running test type: !test_type!
echo.

REM Check if Mesen2 is running and server is available
echo Checking if Mesen2 DiztinGUIsh server is running...
python -c "import socket; s=socket.socket(); s.settimeout(2); s.connect(('127.0.0.1', 9998)); print('✅ Server is running'); s.close()" 2>nul
if %errorlevel% neq 0 (
    echo ❌ Mesen2 DiztinGUIsh server is not running
    echo.
    echo Please:
    echo 1. Start Mesen2
    echo 2. Load a SNES ROM
    echo 3. Enable DiztinGUIsh server (menu or hotkey)
    echo 4. Verify server starts on port 9998
    echo.
    pause
    exit /b 1
)

echo ✅ Mesen2 server detected on port 9998
echo.

REM Run the test client
echo Starting integration tests...
echo.
python "%~dp0test_diztinguish_client.py" --test !test_type!

if %errorlevel% equ 0 (
    echo.
    echo ✅ Integration tests completed successfully!
) else (
    echo.
    echo ❌ Integration tests failed!
)

echo.
pause