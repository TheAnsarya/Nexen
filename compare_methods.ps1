function Get-MethodNames($filePath) {
    if (-not (Test-Path $filePath)) { return @() }
    $content = Get-Content $filePath
    # Simple regex for C++ methods: ClassName::MethodName
    $methods = $content | Select-String -Pattern "GenesisVdp::([a-zA-Z0-9_~]+)\(" | ForEach-Object { $_.Matches.Groups[1].Value } | Select-Object -Unique
    return $methods
}

$nexenPath = "Core/Genesis/GenesisVdp.cpp"
$hoshirunaPath = "C:\Users\me\source\repos\Mesen2-Expanded\Core\Genesis\GenesisVdp.cpp"

$nexenMethods = Get-MethodNames $nexenPath
$hoshirunaMethods = Get-MethodNames $hoshirunaPath

$sharedMethods = $nexenMethods | Where-Object { $hoshirunaMethods -contains $_ } | Sort-Object

Write-Host "Nexen Total Methods: $($nexenMethods.Count)"
Write-Host "Hoshiruna Total Methods: $($hoshirunaMethods.Count)"
Write-Host "Shared Method Names: $($sharedMethods.Count)"
Write-Host "
Top 40 Shared Names (Sorted):"
$sharedMethods | Select-Object -First 40
