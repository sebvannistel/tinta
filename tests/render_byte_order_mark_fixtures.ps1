param(
    [string]$Binary = 'build/Release/tinta.exe',
    [string]$Output = 'out/byte-order-mark/fixed',
    [int[]]$Themes = @(0, 5)
)
# Prints and exports the byte order mark fixtures in each theme (Paper is 0,
# Midnight 5), and the same files without the mark beside them: the marked
# file must print and export exactly as its twin, its title a heading and
# no mark anywhere in the output
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$taskRepo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$taskOutput = Join-Path $taskRepo $Output
$taskBinary = if ([IO.Path]::IsPathRooted($Binary)) { $Binary } else { Join-Path $taskRepo $Binary }
$taskFixtures = @{
    'byte-order-mark' = '<h1[^>]*>Byte order mark</h1>'
    'byte-order-mark-frontmatter' = '<body>\s*<h1[^>]*>Front matter after a byte order mark</h1>'
}
$taskMark = [string][char]0xFEFF

# The twins: the same bytes after the mark, under the same file name
$taskTwins = Join-Path $taskOutput 'twins'
New-Item -ItemType Directory -Force $taskTwins | Out-Null
foreach ($taskName in $taskFixtures.Keys) {
    $taskBytes = [IO.File]::ReadAllBytes((Join-Path $PSScriptRoot "fixtures/$taskName.md"))
    if ($taskBytes.Length -lt 3 -or $taskBytes[0] -ne 0xEF -or $taskBytes[1] -ne 0xBB -or $taskBytes[2] -ne 0xBF) {
        throw "$taskName does not start with a byte order mark"
    }
    [IO.File]::WriteAllBytes((Join-Path $taskTwins "$taskName.md"), [byte[]]$taskBytes[3..($taskBytes.Length - 1)])
}

function Export-Document([string]$Exe, [string]$Source, [string]$Dest) {
    if (Test-Path -LiteralPath $Dest) { Remove-Item -LiteralPath $Dest -Recurse -Force }
    New-Item -ItemType Directory -Force $Dest | Out-Null
    foreach ($taskMode in @('printpages', 'exporthtml', 'exportdocx')) {
        $taskTarget = switch ($taskMode) {
            'printpages' { "$Dest/pages" }
            'exporthtml' { "$Dest/document.html" }
            'exportdocx' { "$Dest/document.docx" }
        }
        $taskProcess = Start-Process -FilePath $Exe -ArgumentList @('"' + $Source + '"', "--$taskMode", '"' + $taskTarget + '"') -WindowStyle Hidden -PassThru
        if (!$taskProcess.WaitForExit(30000)) {
            Stop-Process -Id $taskProcess.Id -Force
            $taskProcess.WaitForExit()
            throw "$Source $taskMode timed out"
        }
        if ($taskProcess.ExitCode -ne 0 -or !(Test-Path -LiteralPath $taskTarget)) { throw "$Source $taskMode failed" }
    }
}

function Read-DocxBody([string]$Path) {
    $taskZip = [IO.Compression.ZipFile]::OpenRead($Path)
    try {
        $taskReader = New-Object IO.StreamReader($taskZip.GetEntry('word/document.xml').Open())
        try { return $taskReader.ReadToEnd() } finally { $taskReader.Dispose() }
    } finally { $taskZip.Dispose() }
}

foreach ($taskTheme in $Themes) {
    $taskRunner = Join-Path $taskOutput "t$taskTheme/runner"
    New-Item -ItemType Directory -Force $taskRunner | Out-Null
    $taskExe = Join-Path $taskRunner 'tinta.exe'
    Copy-Item -LiteralPath $taskBinary -Destination $taskExe -Force
    @"
[Settings]
themeIndex=$taskTheme
followSystemTheme=0
hasAskedFileAssociation=1
openInTabs=0
checkUpdates=0
language=en
"@ | Set-Content -LiteralPath (Join-Path $taskRunner 'settings.ini')
    foreach ($taskName in $taskFixtures.Keys) {
        $taskSource = Join-Path $PSScriptRoot "fixtures/$taskName.md"
        $taskBefore = (Get-FileHash -LiteralPath $taskSource).Hash
        $taskDest = Join-Path $taskOutput "t$taskTheme/$taskName"
        $taskTwinDest = Join-Path $taskOutput "t$taskTheme/$taskName-twin"
        Export-Document $taskExe $taskSource $taskDest
        Export-Document $taskExe (Join-Path $taskTwins "$taskName.md") $taskTwinDest
        if ((Get-FileHash -LiteralPath $taskSource).Hash -ne $taskBefore) { throw "$taskName source changed" }

        $taskHtml = [IO.File]::ReadAllText("$taskDest/document.html", [Text.Encoding]::UTF8)
        if ($taskHtml -notmatch $taskFixtures[$taskName]) { throw "$taskName HTML does not open with its title heading" }
        if ($taskHtml.Contains($taskMark)) { throw "$taskName HTML carries the byte order mark" }
        if ($taskHtml -notmatch '<table>' -or $taskHtml -notmatch '<blockquote' -or $taskHtml -notmatch '<pre><code' -or
            $taskHtml -notmatch '<ul' -or $taskHtml -notmatch 'type="checkbox"') {
            throw "$taskName HTML lost mixed content"
        }
        if ($taskHtml -ne [IO.File]::ReadAllText("$taskTwinDest/document.html", [Text.Encoding]::UTF8)) {
            throw "$taskName HTML differs from the file without the mark"
        }
        $taskDocx = Read-DocxBody "$taskDest/document.docx"
        if ($taskDocx.Contains($taskMark) -or $taskDocx -notmatch '<w:pStyle w:val="Heading1"/>') {
            throw "$taskName DOCX carries the mark or lost its title heading"
        }
        if ($taskDocx -ne (Read-DocxBody "$taskTwinDest/document.docx")) { throw "$taskName DOCX differs from the file without the mark" }
        $taskPages = @(Get-ChildItem -LiteralPath "$taskDest/pages" -Filter 'page-*.png' | Sort-Object Name)
        $taskTwinPages = @(Get-ChildItem -LiteralPath "$taskTwinDest/pages" -Filter 'page-*.png' | Sort-Object Name)
        if (!$taskPages.Count -or $taskPages.Count -ne $taskTwinPages.Count) { throw "$taskName printed a different page count" }
        for ($i = 0; $i -lt $taskPages.Count; $i++) {
            if ((Get-FileHash -LiteralPath $taskPages[$i].FullName).Hash -ne (Get-FileHash -LiteralPath $taskTwinPages[$i].FullName).Hash) {
                throw "$taskName page $($i + 1) differs from the file without the mark"
            }
        }
        "t$taskTheme $taskName : $($taskPages.Count) pages, HTML and DOCX, identical to the file without the mark"
    }
}
