param(
    [string]$Binary = 'build/Release/tinta.exe',
    [string]$Output = 'out/issue-206/fixed'
)
$ErrorActionPreference = 'Stop'
$taskRepo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$taskOutput = Join-Path $taskRepo $Output
$taskBinary = if ([IO.Path]::IsPathRooted($Binary)) { $Binary } else { Join-Path $taskRepo $Binary }
New-Item -ItemType Directory -Force "$taskOutput/runner" | Out-Null
$taskExe = Join-Path $taskOutput 'runner/tinta.exe'
Copy-Item -LiteralPath $taskBinary -Destination $taskExe -Force
@'
[Settings]
themeIndex=0
followSystemTheme=0
hasAskedFileAssociation=1
openInTabs=0
checkUpdates=0
language=en
'@ | Set-Content -LiteralPath "$taskOutput/runner/settings.ini"
foreach ($taskName in @('superscript-subscript','nested-inline-extensions','markdown-regression-control','tab-drop-position')) {
    $taskSource = Join-Path $PSScriptRoot "fixtures/$taskName.md"
    $taskBefore = (Get-FileHash -LiteralPath $taskSource).Hash
    $taskDest = Join-Path $taskOutput $taskName
    New-Item -ItemType Directory -Force $taskDest | Out-Null
    foreach ($taskMode in @('printpages','exporthtml','exportdocx')) {
        $taskTarget = switch ($taskMode) {
            'printpages' { "$taskDest/pages" }
            'exporthtml' { "$taskDest/document.html" }
            'exportdocx' { "$taskDest/document.docx" }
        }
        $taskProcess = Start-Process -FilePath $taskExe -ArgumentList @('"'+$taskSource+'"',"--$taskMode",'"'+$taskTarget+'"') -WindowStyle Hidden -PassThru
        if (!$taskProcess.WaitForExit(30000)) {
            Stop-Process -Id $taskProcess.Id -Force
            $taskProcess.WaitForExit()
            throw "$taskName $taskMode timed out"
        }
        if ($taskProcess.ExitCode -ne 0 -or !(Test-Path -LiteralPath $taskTarget)) { throw "$taskName $taskMode failed" }
    }
    $taskHtml = Get-Content -LiteralPath "$taskDest/document.html" -Raw -Encoding UTF8
    if ($taskHtml -notmatch '<blockquote' -or $taskHtml -notmatch '<table>' -or $taskHtml -notmatch '<h1') {
        throw "$taskName lost mixed Markdown content"
    }
    if ($taskName -eq 'superscript-subscript' -and
        ($taskHtml -notmatch '<sup>2</sup>' -or $taskHtml -notmatch '<sub>2</sub>' -or
         [regex]::Matches($taskHtml,'class="math-(inline|display)"').Count -ne 6)) {
        throw 'Script fixture lost a script or equation'
    }
    if ($taskName -eq 'superscript-subscript' -and $taskHtml -notmatch '<del>struck x<sup>2</sup></del>') {
        throw 'Script fixture lost the superscript inside strikethrough'
    }
    if ($taskName -eq 'nested-inline-extensions' -and
        ($taskHtml -notmatch '<del>struck x<sup>2</sup></del>' -or
         $taskHtml -notmatch '<mark>marked H<sub>2</sub>O</mark>' -or
         $taskHtml -notmatch '<del>a <sub>b</sub></del>' -or
         $taskHtml -notmatch '<mark><del>x<sup>2</sup></del></mark>' -or
         [regex]::Matches($taskHtml,'class="math-(inline|display)"').Count -ne 7)) {
        throw 'Nested fixture lost a nested span or equation'
    }
    if ((Get-FileHash -LiteralPath $taskSource).Hash -ne $taskBefore) { throw "$taskName source changed" }
    $taskPages = @(Get-ChildItem -LiteralPath "$taskDest/pages" -Filter 'page-*.png')
    if (!$taskPages.Count) { throw "$taskName produced no print pages" }
    "$taskName : $($taskPages.Count) native pages, HTML and DOCX"
}
