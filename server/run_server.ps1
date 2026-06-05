$ErrorActionPreference = "Stop"

$serverDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$workspace = Split-Path -Parent $serverDir
$python = Join-Path $workspace ".venv\Scripts\python.exe"

if (-not (Test-Path $python)) {
    throw "Python virtual environment not found at $python"
}

& $python -m pip install -r (Join-Path $serverDir "requirements.txt")
& $python (Join-Path $serverDir "app.py")
