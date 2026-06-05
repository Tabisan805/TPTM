$ErrorActionPreference = "Stop"

if (-not (Get-Command flutter -ErrorAction SilentlyContinue)) {
    throw "Flutter SDK is not installed or is not available in PATH."
}

$appDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$mainFile = Join-Path $appDir "lib\main.dart"
$backupFile = Join-Path $env:TEMP "smart_security_main.dart"

Copy-Item -LiteralPath $mainFile -Destination $backupFile -Force
Push-Location $appDir
try {
    flutter create . --platforms=android
    Copy-Item -LiteralPath $backupFile -Destination $mainFile -Force

    $manifest = Join-Path $appDir "android\app\src\main\AndroidManifest.xml"
    $content = Get-Content -LiteralPath $manifest -Raw
    if ($content -notmatch "usesCleartextTraffic") {
        $content = $content -replace "<application", '<application android:usesCleartextTraffic="true"'
        Set-Content -LiteralPath $manifest -Value $content -Encoding utf8
    }

    flutter pub get
} finally {
    Pop-Location
    Remove-Item -LiteralPath $backupFile -Force -ErrorAction SilentlyContinue
}
