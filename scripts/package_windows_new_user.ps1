param(
	[string]$BuildDir = "build-vcpkg",
	[string]$Configuration = "Release",
	[string]$OutputRoot = "builds",
	[string]$BundleName = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$buildRoot = Join-Path $repoRoot $BuildDir
$binDir = Join-Path $buildRoot ("bin\" + $Configuration)
$exePath = Join-Path $binDir "xoreos.exe"

if (-not (Test-Path $exePath)) {
	throw "xoreos executable not found: $exePath"
}

if (-not $BundleName) {
	$BundleName = "xoreos-windows-x64-" + $Configuration.ToLowerInvariant() + "-portable"
}

$outputDir = Join-Path $repoRoot $OutputRoot
$stagingDir = Join-Path $outputDir $BundleName
$zipPath = Join-Path $outputDir ($BundleName + ".zip")

if (-not (Test-Path $outputDir)) {
	New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
}

if (Test-Path $stagingDir) {
	Remove-Item -Recurse -Force $stagingDir
}
if (Test-Path $zipPath) {
	Remove-Item -Force $zipPath
}

$bundleBinDir = Join-Path $stagingDir ("bin\" + $Configuration)
$bundleScriptsDir = Join-Path $stagingDir "scripts"
New-Item -ItemType Directory -Path $bundleBinDir -Force | Out-Null
New-Item -ItemType Directory -Path $bundleScriptsDir -Force | Out-Null

Copy-Item -Path $exePath -Destination $bundleBinDir -Force

$localBinDlls = @(Get-ChildItem -Path $binDir -Filter *.dll -File -ErrorAction SilentlyContinue)
foreach ($dll in $localBinDlls) {
	Copy-Item -Path $dll.FullName -Destination $bundleBinDir -Force
}

$vcpkgBinDir = Join-Path $buildRoot "vcpkg_installed\x64-windows\bin"
$vcpkgDlls = @(Get-ChildItem -Path $vcpkgBinDir -Filter *.dll -File -ErrorAction SilentlyContinue)
foreach ($dll in $vcpkgDlls) {
	Copy-Item -Path $dll.FullName -Destination $bundleBinDir -Force
}

$launcherSource = Join-Path $PSScriptRoot "run_xoreos_windows.ps1"
Copy-Item -Path $launcherSource -Destination $bundleScriptsDir -Force
$guiLauncherSource = Join-Path $PSScriptRoot "xoreos_launcher_gui.py"
if (Test-Path $guiLauncherSource) {
	Copy-Item -Path $guiLauncherSource -Destination $bundleScriptsDir -Force
}

$batPath = Join-Path $stagingDir "Launch_xoreos.bat"
$batContent = @"
@echo off
setlocal
cd /d "%~dp0"
set "LOGDIR=%~dp0logs"
powershell -ExecutionPolicy Bypass -File "%~dp0scripts\run_xoreos_windows.ps1" -BuildDir "." -Configuration "$Configuration" -LogDir "%LOGDIR%"
echo.
echo If the engine closed unexpectedly, check logs:
echo   %LOGDIR%\xoreos-runtime.log
echo   %LOGDIR%\xoreos-console.log
pause
endlocal
"@
Set-Content -Path $batPath -Value $batContent -Encoding ASCII

$guiBatPath = Join-Path $stagingDir "Launch_xoreos_GUI.bat"
$guiBatContent = @"
@echo off
setlocal
cd /d "%~dp0"
python "%~dp0scripts\xoreos_launcher_gui.py"
echo.
echo GUI closed.
pause
endlocal
"@
Set-Content -Path $guiBatPath -Value $guiBatContent -Encoding ASCII

$readmePath = Join-Path $stagingDir "README_FIRST_RUN.txt"
$readme = @"
xoreos Portable (Windows x64)
=============================

Quick start:
1. Extract this zip to a writable folder (for example C:\Games\xoreos).
2. Double-click Launch_xoreos.bat (or Launch_xoreos_GUI.bat if Python is installed).
3. Pick a detected game install or enter your game folder manually.
4. Start with windowed mode first if this is your first launch.

Notes:
- This package includes runtime DLL dependencies in bin\$Configuration.
- Runtime logs are written to logs\xoreos-runtime.log and logs\xoreos-console.log.
- Game assets are not bundled. Point xoreos at your installed game folder.
- Saved targets are stored in your standard xoreos config location under APPDATA.

Command-line alternative:
  powershell -ExecutionPolicy Bypass -File scripts\run_xoreos_windows.ps1 -BuildDir . -Configuration $Configuration
"@
Set-Content -Path $readmePath -Value $readme -Encoding ASCII

foreach ($doc in @("COPYING", "AUTHORS", "README.md")) {
	$src = Join-Path $repoRoot $doc
	if (Test-Path $src) {
		Copy-Item -Path $src -Destination $stagingDir -Force
	}
}

Compress-Archive -Path (Join-Path $stagingDir "*") -DestinationPath $zipPath -CompressionLevel Optimal

$dllCount = (Get-ChildItem -Path $bundleBinDir -Filter *.dll -File | Measure-Object).Count
Write-Host "Bundle created:"
Write-Host "  Staging: $stagingDir"
Write-Host "  Zip    : $zipPath"
Write-Host "  EXE    : $(Join-Path $bundleBinDir 'xoreos.exe')"
Write-Host "  DLLs   : $dllCount"
