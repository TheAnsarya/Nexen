# Verify which MesenCore.dll version is loaded
Write-Host "`n=== MesenCore.dll Version Check ===" -ForegroundColor Cyan

$mainDll = Get-Item "c:\Users\me\source\repos\Mesen2\bin\win-x64\Release\MesenCore.dll"
$depDll = Get-Item "c:\Users\me\source\repos\Mesen2\bin\win-x64\Release\Dependencies\MesenCore.dll"

Write-Host "`nMain DLL:" -ForegroundColor Yellow
Write-Host "  Path: $($mainDll.FullName)"
Write-Host "  Modified: $($mainDll.LastWriteTime)"
Write-Host "  Size: $($mainDll.Length) bytes"

Write-Host "`nDependencies DLL:" -ForegroundColor Yellow  
Write-Host "  Path: $($depDll.FullName)"
Write-Host "  Modified: $($depDll.LastWriteTime)"
Write-Host "  Size: $($depDll.Length) bytes"

if ($mainDll.LastWriteTime -eq $depDll.LastWriteTime) {
    Write-Host "`n✓ DLLs are synchronized" -ForegroundColor Green
} else {
    Write-Host "`n✗ DLLs are NOT synchronized!" -ForegroundColor Red
    Write-Host "  Main is newer by: $($mainDll.LastWriteTime - $depDll.LastWriteTime)"
}

Write-Host "`n=== Connection Test ===" -ForegroundColor Cyan
Write-Host "1. Close Mesen2 completely"
Write-Host "2. Start Mesen2 fresh"
Write-Host "3. Load a ROM"
Write-Host "4. Open Tools → Debugger (CRITICAL!)"
Write-Host "5. Start DiztinGUIsh Server"
Write-Host "6. Connect from DiztinGUIsh"
Write-Host "7. Check logs for 'ROM=' (should NOT be 'Test ROM')"
Write-Host ""
