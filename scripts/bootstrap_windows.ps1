param(
	[string]$BuildDir = "build-vcpkg-portable",
	[string]$Configuration = "Debug",
	[string]$VcpkgRoot = "",
	[switch]$SkipBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot

function Require-Command {
	param([string]$Name)

	if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
		throw "Required command not found: $Name"
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
		$version = (($version | Select-Object -First 1).Trim().Split("."))[0]

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

	throw "Neither Ninja nor a supported Visual Studio C++ toolchain was found. Install Ninja or Visual Studio Build Tools with Desktop C++."
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

Require-Command git
Require-Command cmake

if (-not (Test-Path $toolchain)) {
	$parent = Split-Path -Parent $VcpkgRoot
	if (-not (Test-Path $parent)) {
		New-Item -ItemType Directory -Path $parent -Force | Out-Null
	}

	Write-Host "Cloning vcpkg into $VcpkgRoot"
	git clone https://github.com/microsoft/vcpkg $VcpkgRoot
}

if (-not (Test-Path $vcpkgExe)) {
	$bootstrap = Join-Path $VcpkgRoot "bootstrap-vcpkg.bat"
	if (-not (Test-Path $bootstrap)) {
		throw "vcpkg bootstrap script not found at $bootstrap"
	}

	Write-Host "Bootstrapping vcpkg"
	& $bootstrap -disableMetrics
	if ($LASTEXITCODE -ne 0) {
		throw "vcpkg bootstrap failed with exit code $LASTEXITCODE"
	}
}

$env:VCPKG_ROOT = $VcpkgRoot
$buildPath = Join-Path $repoRoot $BuildDir
$generator = Get-PreferredGenerator
Reset-BuildDirectoryIfGeneratorChanged -BuildPath $buildPath -GeneratorName $generator.Name

Write-Host "Configuring xoreos"
Write-Host "Generator       : $($generator.Name)"
cmake -S $repoRoot -B $buildPath -G $generator.Name @($generator.Args) "-DCMAKE_POLICY_VERSION_MINIMUM:STRING=3.5" "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=$toolchain"
if ($LASTEXITCODE -ne 0) {
	throw "CMake configure failed with exit code $LASTEXITCODE"
}

if (-not $SkipBuild) {
	Write-Host "Building xoreos ($Configuration)"
	cmake --build $buildPath --config $Configuration --target xoreos
	if ($LASTEXITCODE -ne 0) {
		throw "CMake build failed with exit code $LASTEXITCODE"
	}
}

Write-Host ""
Write-Host "Ready."
Write-Host "Build directory : $buildPath"
Write-Host "vcpkg root      : $VcpkgRoot"
Write-Host "Binary          : $(Join-Path $buildPath "bin\$Configuration\xoreos.exe")"
