<#
.SYNOPSIS
Bundle a built xoreos.exe, its DLLs and the launch scripts into a portable zip.

.DESCRIPTION
Build first, for example:
  powershell -ExecutionPolicy Bypass -File scripts\bootstrap_windows.ps1 -Configuration Release
then run this script. Without -BuildDir it looks in build-vcpkg-portable
(the bootstrap default), then build-vcpkg.
#>
param(
	[string]$BuildDir = "",
	[ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
	[string]$Configuration = "Release",
	[string]$OutputRoot = "builds",
	[string]$BundleName = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot

if ($BuildDir) {
	$buildDirCandidates = @($BuildDir)
} else {
	$buildDirCandidates = @("build-vcpkg-portable", "build-vcpkg")
}

# Multi-config generators put the exe in bin\<Configuration>, Ninja in bin\.
$exePath = $null
$searched = @()
foreach ($candidate in $buildDirCandidates) {
	if ([System.IO.Path]::IsPathRooted($candidate)) {
		$candidateRoot = $candidate
	} else {
		$candidateRoot = Join-Path $repoRoot $candidate
	}
	foreach ($candidateExe in @((Join-Path $candidateRoot "bin\$Configuration\xoreos.exe"), (Join-Path $candidateRoot "bin\xoreos.exe"))) {
		$searched += $candidateExe
		if (-not $exePath -and (Test-Path $candidateExe)) {
			$exePath = $candidateExe
			$buildRoot = $candidateRoot
		}
	}
}

if (-not $exePath) {
	throw ("xoreos.exe ($Configuration) not found. Looked in:`n  " + ($searched -join "`n  ") +
		"`nBuild it first: powershell -ExecutionPolicy Bypass -File scripts\bootstrap_windows.ps1 -Configuration $Configuration" +
		"`nor pass -BuildDir / -Configuration to match your build.")
}

$binDir = Split-Path -Parent $exePath
Write-Host "Packaging $exePath"

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

if ($Configuration -eq "Debug") {
	$vcpkgBinDir = Join-Path $buildRoot "vcpkg_installed\x64-windows\debug\bin"
} else {
	$vcpkgBinDir = Join-Path $buildRoot "vcpkg_installed\x64-windows\bin"
}
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
# Prefer pyw (no console window), then py, then python. The "python" on PATH
# can be the Microsoft Store stub, so it is only the last resort.
$guiBatContent = @"
@echo off
setlocal
cd /d "%~dp0"
set "GUI=%~dp0scripts\xoreos_launcher_gui.py"
where pyw >nul 2>nul
if not errorlevel 1 (
	start "" pyw -3 "%GUI%"
	exit /b 0
)
where py >nul 2>nul
if not errorlevel 1 (
	py -3 "%GUI%"
	if errorlevel 1 goto failed
	exit /b 0
)
where python >nul 2>nul
if not errorlevel 1 (
	python "%GUI%"
	if errorlevel 1 goto failed
	exit /b 0
)
echo Python 3 with Tk is needed for the GUI launcher - install it from python.org,
echo or use Launch_xoreos.bat instead.
pause
exit /b 1
:failed
echo.
echo The GUI launcher exited with an error. If Python is not installed, get Python 3
echo with Tk from python.org, or use Launch_xoreos.bat instead.
pause
exit /b 1
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
