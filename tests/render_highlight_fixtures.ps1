param(
    [string]$Binary = 'build/Release/tinta.exe',
    [string]$Output = 'out/highlight-seams/fixed',
    [int[]]$Themes = @(0, 5),
    [string[]]$Fixtures = @('highlight-seams', 'markdown-regression-control', 'footnotes',
                            'math-mixed-layout', 'superscript-subscript')
)
# Exports print pages, HTML and DOCX for each fixture in each theme (Paper is
# 0, Midnight 5) to <Output>/t<theme>/<fixture>, for comparison between builds
$ErrorActionPreference = 'Stop'
$taskRepo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$taskOutput = Join-Path $taskRepo $Output
$taskBinary = if ([IO.Path]::IsPathRooted($Binary)) { $Binary } else { Join-Path $taskRepo $Binary }
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
    foreach ($taskName in $Fixtures) {
        $taskSource = Join-Path $PSScriptRoot "fixtures/$taskName.md"
        $taskBefore = (Get-FileHash -LiteralPath $taskSource).Hash
        $taskDest = Join-Path $taskOutput "t$taskTheme/$taskName"
        if (Test-Path -LiteralPath $taskDest) { Remove-Item -LiteralPath $taskDest -Recurse -Force }
        New-Item -ItemType Directory -Force $taskDest | Out-Null
        foreach ($taskMode in @('printpages', 'exporthtml', 'exportdocx')) {
            $taskTarget = switch ($taskMode) {
                'printpages' { "$taskDest/pages" }
                'exporthtml' { "$taskDest/document.html" }
                'exportdocx' { "$taskDest/document.docx" }
            }
            $taskProcess = Start-Process -FilePath $taskExe -ArgumentList @('"' + $taskSource + '"', "--$taskMode", '"' + $taskTarget + '"') -WindowStyle Hidden -PassThru
            if (!$taskProcess.WaitForExit(30000)) {
                Stop-Process -Id $taskProcess.Id -Force
                $taskProcess.WaitForExit()
                throw "$taskName $taskMode timed out"
            }
            if ($taskProcess.ExitCode -ne 0 -or !(Test-Path -LiteralPath $taskTarget)) { throw "$taskName $taskMode failed" }
        }
        $taskHtml = Get-Content -LiteralPath "$taskDest/document.html" -Raw -Encoding UTF8
        if ($taskHtml -notmatch '<mark>') { throw "$taskName lost its highlights" }
        if ($taskName -eq 'highlight-seams' -and
            ($taskHtml -notmatch '<blockquote' -or $taskHtml -notmatch '<table>' -or $taskHtml -notmatch '<h1' -or
             [regex]::Matches($taskHtml, '<mark>').Count -lt 27 -or
             $taskHtml -notmatch 'All surrounding content')) {
            throw 'Highlight fixture lost mixed content or a highlight'
        }
        if ((Get-FileHash -LiteralPath $taskSource).Hash -ne $taskBefore) { throw "$taskName source changed" }
        $taskPages = @(Get-ChildItem -LiteralPath "$taskDest/pages" -Filter 'page-*.png')
        if (!$taskPages.Count) { throw "$taskName produced no print pages" }
        "t$taskTheme $taskName : $($taskPages.Count) native pages, HTML and DOCX"
    }
}
