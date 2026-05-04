Add-Type -AssemblyName System.Web

function Get-NodeKey([System.Xml.XmlNode]$node) {
$parts = New-Object System.Collections.Generic.List[string]
$cur = $node
while($cur -ne $null -and $cur.NodeType -eq [System.Xml.XmlNodeType]::Element) {
$idAttr = $cur.Attributes['ID']
$idVal = if($idAttr -ne $null) { $idAttr.Value } else { '' }
$parts.Add("$($cur.Name)[$idVal]")
$cur = $cur.ParentNode
if($cur -ne $null -and $cur.Name -eq '#document') { break }
}
[array]::Reverse($parts.ToArray()) | Out-Null
return (($parts.ToArray() | Select-Object -Reverse) -join '/')
}

function Build-Map([xml]$doc) {
$map = @{}
$nodes = $doc.SelectNodes('//*[@ID]')
foreach($n in $nodes) {
$key = Get-NodeKey $n
$map[$key] = $n
}
return $map
}

function Protect-Tokens([string]$text) {
$map = @{}
$i = 0
$out = [regex]::Replace($text, '\{\d+\}|\$[0-9a-fA-F]+|%\d*\$?[sdifxX]|\{[^{}]*\}', {
param($m)
$token = "__TOK${i}__"
$map[$token] = $m.Value
$i++
return $token
})
return @{ Text = $out; Map = $map }
}

function Restore-Tokens([string]$text, $map) {
$out = $text
foreach($k in $map.Keys) {
$out = $out.Replace($k, [string]$map[$k])
}
return $out
}

function Translate-Google([string]$text, [string]$targetLang, $cache) {
if([string]::IsNullOrWhiteSpace($text)) { return $text }
if($cache.ContainsKey($text)) { return $cache[$text] }

$leadingUnderscore = $text.StartsWith('_')
$core = if($leadingUnderscore) { $text.Substring(1) } else { $text }

if([string]::IsNullOrWhiteSpace($core)) {
$cache[$text] = $text
return $text
}

$protected = Protect-Tokens $core
$q = [System.Web.HttpUtility]::UrlEncode($protected.Text)
$url = "https://translate.googleapis.com/translate_a/single?client=gtx&sl=en&tl=$targetLang&dt=t&q=$q"

$translated = $null
for($attempt = 0; $attempt -lt 4; $attempt++) {
try {
$resp = Invoke-RestMethod -Uri $url -Method Get -TimeoutSec 30
$segments = @()
foreach($seg in $resp[0]) {
$segments += [string]$seg[0]
}
$translated = ($segments -join '')
break
} catch {
Start-Sleep -Milliseconds (250 * ($attempt + 1))
}
}

if($null -eq $translated) {
$translated = $core
}

$translated = Restore-Tokens $translated $protected.Map
if($leadingUnderscore) { $translated = '_' + $translated }

$cache[$text] = $translated
return $translated
}

$enPath = 'UI/Localization/resources.en.xml'
$esPath = 'UI/Localization/resources.es.xml'
$jaPath = 'UI/Localization/resources.ja.xml'

[xml]$enDoc = Get-Content $enPath -Raw
[xml]$esDoc = Get-Content $esPath -Raw
[xml]$jaDoc = Get-Content $jaPath -Raw

$enMap = Build-Map $enDoc
$esMap = Build-Map $esDoc
$jaMap = Build-Map $jaDoc

$esMissing = New-Object System.Collections.Generic.List[string]
$jaMissing = New-Object System.Collections.Generic.List[string]

foreach($k in $enMap.Keys) {
$enVal = [string]$enMap[$k].InnerText
if(-not $esMap.ContainsKey($k) -or [string]::IsNullOrWhiteSpace([string]$esMap[$k].InnerText) -or [string]$esMap[$k].InnerText -eq $enVal) {
$esMissing.Add($k)
}
if(-not $jaMap.ContainsKey($k) -or [string]::IsNullOrWhiteSpace([string]$jaMap[$k].InnerText) -or [string]$jaMap[$k].InnerText -eq $enVal) {
$jaMissing.Add($k)
}
}

"Context-aware untranslated count -> ES: $($esMissing.Count), JA: $($jaMissing.Count)"

$esUnique = $esMissing | ForEach-Object { [string]$enMap[$_].InnerText } | Sort-Object -Unique
$jaUnique = $jaMissing | ForEach-Object { [string]$enMap[$_].InnerText } | Sort-Object -Unique
"Unique EN strings to translate -> ES: $($esUnique.Count), JA: $($jaUnique.Count)"

$esCache = @{}
$jaCache = @{}

$idx = 0
foreach($s in $esUnique) {
$idx++
if($idx % 100 -eq 0) { "ES translating $idx / $($esUnique.Count)" }
[void](Translate-Google $s 'es' $esCache)
}

$idx = 0
foreach($s in $jaUnique) {
$idx++
if($idx % 100 -eq 0) { "JA translating $idx / $($jaUnique.Count)" }
[void](Translate-Google $s 'ja' $jaCache)
}

foreach($k in $esMissing) {
$src = [string]$enMap[$k].InnerText
$esMap[$k].InnerText = [string]$esCache[$src]
}

foreach($k in $jaMissing) {
$src = [string]$enMap[$k].InnerText
$jaMap[$k].InnerText = [string]$jaCache[$src]
}

$settings = New-Object System.Xml.XmlWriterSettings
$settings.Indent = $true
$settings.IndentChars = "`t"
$settings.NewLineChars = "`r`n"
$settings.NewLineHandling = [System.Xml.NewLineHandling]::Replace
$settings.Encoding = New-Object System.Text.UTF8Encoding($false)

$writer = [System.Xml.XmlWriter]::Create($esPath, $settings)
$esDoc.Save($writer)
$writer.Close()

$writer = [System.Xml.XmlWriter]::Create($jaPath, $settings)
$jaDoc.Save($writer)
$writer.Close()

# Re-count after update
[xml]$esDoc2 = Get-Content $esPath -Raw
[xml]$jaDoc2 = Get-Content $jaPath -Raw
$esMap2 = Build-Map $esDoc2
$jaMap2 = Build-Map $jaDoc2
$esLeft = 0
$jaLeft = 0
foreach($k in $enMap.Keys) {
$ev = [string]$enMap[$k].InnerText
if(-not $esMap2.ContainsKey($k) -or [string]::IsNullOrWhiteSpace([string]$esMap2[$k].InnerText) -or [string]$esMap2[$k].InnerText -eq $ev) { $esLeft++ }
if(-not $jaMap2.ContainsKey($k) -or [string]::IsNullOrWhiteSpace([string]$jaMap2[$k].InnerText) -or [string]$jaMap2[$k].InnerText -eq $ev) { $jaLeft++ }
}
"Remaining untranslated -> ES: $esLeft, JA: $jaLeft"
