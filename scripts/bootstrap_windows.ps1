<#
.SYNOPSIS
Fetch vcpkg, build the libraries from vcpkg.json, then configure and build xoreos.

.DESCRIPTION
Self-contained Windows build. Clones vcpkg into .deps\vcpkg (once), lets it
build every dependency (the first run takes a long time), then configures
CMake with Ninja Multi-Config when Ninja and a compiler are on PATH (for
example in a Developer PowerShell), otherwise with the installed Visual
Studio 2022/2026 generator.

Prerequisites: Git, CMake 3.21+, and Visual Studio or Build Tools with the
"Desktop development with C++" workload. Ninja is optional.

.PARAMETER BuildDir
Build directory, absolute or relative to the repository root.

.PARAMETER Configuration
Debug, Release, RelWithDebInfo or MinSizeRel.

.PARAMETER VcpkgRoot
vcpkg checkout to use. Defaults to $env:VCPKG_ROOT, else .deps\vcpkg.

.PARAMETER VcpkgRef
vcpkg commit or tag to check out when cloning, to pin library versions.

.PARAMETER Test
Also build and run the unit tests (target "check").

.PARAMETER SkipBuild
Configure only (vcpkg still builds the libraries).

.EXAMPLE
powershell -ExecutionPolicy Bypass -File scripts\bootstrap_windows.ps1

.EXAMPLE
powershell -ExecutionPolicy Bypass -File scripts\bootstrap_windows.ps1 -Configuration Release -Test
#>
param(
	[string]$BuildDir = "build-vcpkg-portable",
	[ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
	[string]$Configuration = "Debug",
	[string]$VcpkgRoot = "",
	[string]$VcpkgRef = "",
	[switch]$Test,
	[switch]$SkipBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot

function Require-Command {
	param(
		[string]$Name,
		[string]$InstallHint
	)

	if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
		throw "Required command not found: $Name. Install it with: $InstallHint (then open a new terminal so PATH is updated)."
	}
}

function Test-NinjaCompilerSupport {
	if (-not (Get-Command ninja -ErrorAction SilentlyContinue)) {
		return $false
	}

	foreach ($compiler in @("cl", "clang++", "g++")) {
		if (Get-Command $compiler -ErrorAction SilentlyContinue) {
			return $true
		}
	}

	return $false
}

function Get-PreferredGenerator {
	if (Test-NinjaCompilerSupport) {
		return @{
			Name = "Ninja Multi-Config"
			Args = @()
		}
	}

	$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
	if (Test-Path $vswhere) {
		$version = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion
		$version = $version | Select-Object -First 1
		if ($version) {
			$version = ($version.Trim().Split("."))[0]
		}

		switch ($version) {
			"18" {
				return @{
					Name = "Visual Studio 18 2026"
					Args = @("-A", "x64")
				}
			}
			"17" {
				return @{
					Name = "Visual Studio 17 2022"
					Args = @("-A", "x64")
				}
			}
		}
	}

	throw ("No usable C++ toolchain found. Install Visual Studio 2022 or 2026 (Community or Build Tools) " +
		"with the 'Desktop development with C++' workload, for example: " +
		"winget install Microsoft.VisualStudio.2022.BuildTools --override `"--add Microsoft.VisualStudio.Workload.VCTools --includeRecommended --passive`". " +
		"Alternatively run this script from a 'Developer PowerShell for VS' with Ninja installed (winget install Ninja-build.Ninja).")
}

function Reset-BuildDirectoryIfGeneratorChanged {
	param(
		[string]$BuildPath,
		[string]$GeneratorName
	)

	$cachePath = Join-Path $BuildPath "CMakeCache.txt"
	if (-not (Test-Path $cachePath)) {
		return
	}

	$cachedGenerator = $null
	foreach ($line in Get-Content $cachePath) {
		if ($line -match '^CMAKE_GENERATOR:INTERNAL=(.+)$') {
			$cachedGenerator = $matches[1].Trim()
			break
		}
	}

	if (-not $cachedGenerator -or $cachedGenerator -eq $GeneratorName) {
		return
	}

	Write-Host "Existing build directory uses '$cachedGenerator'; resetting cache for '$GeneratorName'."

	$cmakeFilesPath = Join-Path $BuildPath "CMakeFiles"
	if (Test-Path $cachePath) {
		Remove-Item -LiteralPath $cachePath -Force
	}
	if (Test-Path $cmakeFilesPath) {
		Remove-Item -LiteralPath $cmakeFilesPath -Recurse -Force
	}
}

if (-not $VcpkgRoot) {
	if ($env:VCPKG_ROOT) {
		$VcpkgRoot = $env:VCPKG_ROOT
	} else {
		$VcpkgRoot = Join-Path $repoRoot ".deps\vcpkg"
	}
}

$VcpkgRoot = [System.IO.Path]::GetFullPath($VcpkgRoot)
$toolchain = Join-Path $VcpkgRoot "scripts\buildsystems\vcpkg.cmake"
$vcpkgExe = Join-Path $VcpkgRoot "vcpkg.exe"

Require-Command git "winget install Git.Git"
Require-Command cmake "winget install Kitware.CMake"

if (-not (Test-Path $toolchain)) {
	if ((Test-Path $VcpkgRoot) -and (Get-ChildItem -Force -LiteralPath $VcpkgRoot | Select-Object -First 1)) {
		throw "$VcpkgRoot exists but is not a vcpkg checkout (no scripts\buildsystems\vcpkg.cmake). Remove it or pass -VcpkgRoot."
	}

	$parent = Split-Path -Parent $VcpkgRoot
	if (-not (Test-Path $parent)) {
		New-Item -ItemType Directory -Path $parent -Force | Out-Null
	}

	Write-Host "Cloning vcpkg into $VcpkgRoot"
	git clone https://github.com/microsoft/vcpkg $VcpkgRoot
	if ($LASTEXITCODE -ne 0) {
		throw "Could not clone vcpkg (exit code $LASTEXITCODE). Check your network connection, or clone https://github.com/microsoft/vcpkg yourself and pass -VcpkgRoot."
	}

	if ($VcpkgRef) {
		Write-Host "Checking out vcpkg $VcpkgRef"
		git -C $VcpkgRoot checkout --quiet $VcpkgRef
		if ($LASTEXITCODE -ne 0) {
			throw "Could not check out vcpkg ref '$VcpkgRef'."
		}
	}
} elseif ($VcpkgRef) {
	Write-Host "Note: -VcpkgRef is only applied when cloning; $VcpkgRoot already exists."
}

if (-not (Test-Path $vcpkgExe)) {
	$bootstrap = Join-Path $VcpkgRoot "bootstrap-vcpkg.bat"
	if (-not (Test-Path $bootstrap)) {
		throw "vcpkg bootstrap script not found at $bootstrap"
	}

	Write-Host "Bootstrapping vcpkg"
	& $bootstrap -disableMetrics
	if ($LASTEXITCODE -ne 0) {
		throw "vcpkg bootstrap failed with exit code $LASTEXITCODE. It downloads a prebuilt vcpkg.exe, so check your network connection."
	}
}

$env:VCPKG_ROOT = $VcpkgRoot
if ([System.IO.Path]::IsPathRooted($BuildDir)) {
	$buildPath = [System.IO.Path]::GetFullPath($BuildDir)
} else {
	$buildPath = [System.IO.Path]::GetFullPath((Join-Path $repoRoot $BuildDir))
}
$generator = Get-PreferredGenerator
Reset-BuildDirectoryIfGeneratorChanged -BuildPath $buildPath -GeneratorName $generator.Name

Write-Host "Configuring xoreos"
Write-Host "Generator       : $($generator.Name)"
Write-Host "Build directory : $buildPath"
Write-Host "The first configure builds all libraries from vcpkg.json; this can take a while."
cmake -S $repoRoot -B $buildPath -G $generator.Name @($generator.Args) "-DCMAKE_POLICY_VERSION_MINIMUM:STRING=3.5" "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=$toolchain"
if ($LASTEXITCODE -ne 0) {
	Write-Host ""
	Write-Host "CMake configure failed. Common causes:"
	Write-Host "  - A vcpkg port failed to build: its log is named above; see also $(Join-Path $buildPath 'vcpkg-manifest-install.log')"
	Write-Host "  - A missing library: the CMake error above names it and how to get it."
	Write-Host "  - A stale build directory: delete '$buildPath' (or pass another -BuildDir) and retry."
	throw "CMake configure failed with exit code $LASTEXITCODE"
}

if ($SkipBuild) {
	Write-Host ""
	Write-Host "Configured (build skipped). Build with: cmake --build `"$buildPath`" --config $Configuration --target xoreos"
	exit 0
}

Write-Host "Building xoreos ($Configuration)"
cmake --build $buildPath --config $Configuration --target xoreos
if ($LASTEXITCODE -ne 0) {
	throw "CMake build failed with exit code $LASTEXITCODE"
}

if ($Test) {
	Write-Host "Building and running unit tests (target check)"
	cmake --build $buildPath --config $Configuration --target check
	if ($LASTEXITCODE -ne 0) {
		throw "Unit tests failed with exit code $LASTEXITCODE"
	}
}

$launchArgs = ""
if ($BuildDir -ne "build-vcpkg-portable") {
	$launchArgs += " -BuildDir `"$BuildDir`""
}
if ($Configuration -ne "Debug") {
	$launchArgs += " -Configuration $Configuration"
}

Write-Host ""
Write-Host "Build finished."
Write-Host "  Build directory : $buildPath"
Write-Host "  vcpkg root      : $VcpkgRoot"
Write-Host "  Binary          : $(Join-Path $buildPath "bin\$Configuration\xoreos.exe")"
if (-not $Test) {
	Write-Host "  Run unit tests  : cmake --build `"$buildPath`" --config $Configuration --target check"
}
Write-Host ""
Write-Host "Launch KotOR (needs your own game files):"
Write-Host "  powershell -ExecutionPolicy Bypass -File scripts\run_xoreos_windows.ps1$launchArgs"
Write-Host "  py -3 scripts\xoreos_launcher_gui.py      (graphical launcher)"
