# Installs the tinta.exe beside this script into C:\Program Files\Tinta and
# points .md files at it. Settings stay in %APPDATA%\Tinta, so an update or
# reinstall keeps them. Run through install.cmd (double-click).
$ErrorActionPreference = 'Stop'

$source = Join-Path $PSScriptRoot 'tinta.exe'
$targetDir = Join-Path $env:ProgramFiles 'Tinta'
$target = Join-Path $targetDir 'tinta.exe'

if (-not (Test-Path $source)) {
    Write-Host "tinta.exe was not found next to this script ($PSScriptRoot)." -ForegroundColor Red
    exit 1
}

# A running copy keeps the old exe locked
Get-Process tinta -ErrorAction SilentlyContinue |
    Where-Object { $_.Path -eq $target } |
    ForEach-Object {
        Write-Host 'Closing the running Tinta...'
        $_.CloseMainWindow() | Out-Null
        if (-not $_.WaitForExit(5000)) { $_.Kill() }
    }

# Only the copy into Program Files needs administrator rights
Write-Host "Copying Tinta to $targetDir (Windows will ask for permission)..."
$copy = "New-Item -ItemType Directory -Force -Path '$targetDir' | Out-Null; " +
        "Copy-Item -LiteralPath '$source' -Destination '$target' -Force"
$elevated = Start-Process powershell -Verb RunAs -Wait -PassThru -WindowStyle Hidden `
    -ArgumentList '-NoProfile', '-ExecutionPolicy', 'Bypass', '-Command', $copy
if ($elevated.ExitCode -ne 0 -or -not (Test-Path $target)) {
    Write-Host 'Copying failed or was cancelled.' -ForegroundColor Red
    exit 1
}

# Registration is per user, so it runs as you, not as administrator
Write-Host 'Registering Tinta for .md files...'
Start-Process $target -ArgumentList '/register' -Wait

Write-Host "Done. Tinta is installed at $target" -ForegroundColor Green
