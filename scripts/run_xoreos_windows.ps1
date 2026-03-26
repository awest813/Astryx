param(
	[string]$GamePath = "",
	[string]$Target = "",
	[string]$BuildDir = "build-vcpkg-portable",
	[string]$Configuration = "Debug",
	[string]$LogDir = "",
	[int]$Width = 0,
	[int]$Height = 0,
	[switch]$Fullscreen,
	[switch]$Windowed,
	[switch]$SkipVideos,
	[switch]$ListSources
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot

function Get-XoreosExecutable {
	param(
		[string]$RepoRoot,
		[string]$BuildDir,
		[string]$Configuration
	)

	$candidates = @(
		(Join-Path $RepoRoot "$BuildDir\bin\$Configuration\xoreos.exe"),
		(Join-Path $RepoRoot "build-vcpkg\bin\$Configuration\xoreos.exe"),
		(Join-Path $RepoRoot "build-vcpkg-portable\bin\$Configuration\xoreos.exe"),
		(Join-Path $RepoRoot "build\bin\$Configuration\xoreos.exe")
	)

	foreach ($candidate in $candidates) {
		if (Test-Path $candidate) {
			return $candidate
		}
	}

	return $null
}

function Add-RuntimeDllPaths {
	param(
		[string]$RepoRoot
	)

	$candidates = @(
		(Join-Path $RepoRoot "build-vcpkg\vcpkg_installed\x64-windows\bin"),
		(Join-Path $RepoRoot "build-vcpkg\vcpkg_installed\x64-windows\debug\bin"),
		(Join-Path $RepoRoot "build-vcpkg-portable\vcpkg_installed\x64-windows\bin"),
		(Join-Path $RepoRoot "build-vcpkg-portable\vcpkg_installed\x64-windows\debug\bin"),
		(Join-Path $RepoRoot ".deps\vcpkg\installed\x64-windows\bin"),
		(Join-Path $RepoRoot ".deps\vcpkg\installed\x64-windows\debug\bin")
	)

	$existing = @()
	foreach ($dir in $candidates) {
		if (Test-Path $dir) {
			$existing += [System.IO.Path]::GetFullPath($dir)
		}
	}

	if ($existing.Count -eq 0) {
		return @()
	}

	$pathParts = @()
	if ($env:PATH) {
		$pathParts = $env:PATH -split ';'
	}

	$toPrepend = @()
	foreach ($dir in $existing) {
		if ($pathParts -notcontains $dir) {
			$toPrepend += $dir
		}
	}

	if ($toPrepend.Count -gt 0) {
		$env:PATH = ($toPrepend -join ';') + ';' + $env:PATH
	}

	return $existing
}

function Get-XoreosConfigPath {
	$candidates = @()

	if ($env:APPDATA) {
		$candidates += (Join-Path $env:APPDATA "xoreos\xoreos.conf")
	}
	if ($env:USERPROFILE) {
		$candidates += (Join-Path $env:USERPROFILE ".config\xoreos\xoreos.conf")
		$candidates += (Join-Path $env:USERPROFILE "xoreos\xoreos.conf")
	}

	foreach ($candidate in $candidates) {
		if (Test-Path $candidate) {
			return $candidate
		}
	}

	if ($candidates.Count -gt 0) {
		return $candidates[0]
	}

	return $null
}

function Get-ConfigTargets {
	param([string]$ConfigPath)

	$entries = @()
	if (-not $ConfigPath -or -not (Test-Path $ConfigPath)) {
		return $entries
	}

	$currentName = $null
	$currentPath = $null
	$currentDescription = $null

	foreach ($line in Get-Content $ConfigPath) {
		$trimmed = $line.Trim()
		if (-not $trimmed -or $trimmed.StartsWith("#") -or $trimmed.StartsWith(";")) {
			continue
		}

		if ($trimmed -match '^\[(.+)\]$') {
			if ($currentName -and $currentName -ne "xoreos" -and $currentPath) {
				$entries += [pscustomobject]@{
					Name = $currentName
					Path = $currentPath
					Description = $currentDescription
					Source = "Config target"
					LaunchKind = "Target"
					LaunchValue = $currentName
				}
			}

			$currentName = $matches[1]
			$currentPath = $null
			$currentDescription = $null
			continue
		}

		if (-not $currentName) {
			continue
		}

		if ($trimmed -match '^path=(.+)$') {
			$currentPath = $matches[1].Trim()
			continue
		}

		if ($trimmed -match '^description=(.+)$') {
			$currentDescription = $matches[1].Trim()
		}
	}

	if ($currentName -and $currentName -ne "xoreos" -and $currentPath) {
		$entries += [pscustomobject]@{
			Name = $currentName
			Path = $currentPath
			Description = $currentDescription
			Source = "Config target"
			LaunchKind = "Target"
			LaunchValue = $currentName
		}
	}

	return $entries
}

function Get-DetectedGames {
	$roots = @()

	foreach ($candidate in @(
		"${env:ProgramFiles(x86)}\Steam\steamapps\common",
		"${env:ProgramFiles}\Steam\steamapps\common",
		"C:\GOG Games",
		"D:\GOG Games",
		"${env:ProgramFiles(x86)}\GOG Galaxy\Games",
		"${env:ProgramFiles}\GOG Galaxy\Games"
	)) {
		if ($candidate -and (Test-Path $candidate)) {
			$roots += $candidate
		}
	}

	$gameFolderHints = @(
		@{ Label = "KotOR I"; Folder = "swkotor" },
		@{ Label = "KotOR I"; Folder = "Star Wars - Knights of the Old Republic" },
		@{ Label = "KotOR II"; Folder = "Knights of the Old Republic II" },
		@{ Label = "KotOR II"; Folder = "Star Wars Knights of the Old Republic II" },
		@{ Label = "Neverwinter Nights"; Folder = "Neverwinter Nights" },
		@{ Label = "Jade Empire"; Folder = "Jade Empire" }
	)

	$entries = @()
	foreach ($root in $roots | Select-Object -Unique) {
		foreach ($hint in $gameFolderHints) {
			$path = Join-Path $root $hint.Folder
			if (Test-Path $path) {
				$entries += [pscustomobject]@{
					Name = $hint.Label
					Path = $path
					Description = $root
					Source = "Detected install"
					LaunchKind = "Path"
					LaunchValue = $path
				}
			}
		}
	}

	$entries | Sort-Object Path -Unique
}

function Get-LaunchChoice {
	param([object[]]$Sources)

	if (-not $Sources -or $Sources.Count -eq 0) {
		$manual = Read-Host "No saved or detected installs were found. Enter a game path"
		if (-not $manual) {
			throw "No game path provided."
		}

		return [pscustomobject]@{
			LaunchKind = "Path"
			LaunchValue = $manual
			Name = "Manual path"
			Source = "Manual"
		}
	}

	Write-Host ""
	Write-Host "Available games and targets:"
	for ($i = 0; $i -lt $Sources.Count; $i++) {
		$entry = $Sources[$i]
		$desc = if ($entry.Description) { " | $($entry.Description)" } else { "" }
		Write-Host ("[{0}] {1} ({2}){3}" -f ($i + 1), $entry.Name, $entry.Source, $desc)
		Write-Host ("     {0}" -f $entry.Path)
	}
	Write-Host "[M] Manual path"

	while ($true) {
		$choice = (Read-Host "Choose a number or M").Trim()
		if ($choice -match '^[Mm]$') {
			$manual = Read-Host "Enter a game path"
			if ($manual) {
				return [pscustomobject]@{
					LaunchKind = "Path"
					LaunchValue = $manual
					Name = "Manual path"
					Source = "Manual"
				}
			}
		}

		$index = 0
		if ([int]::TryParse($choice, [ref]$index)) {
			if ($index -ge 1 -and $index -le $Sources.Count) {
				return $Sources[$index - 1]
			}
		}

		Write-Warning "Please choose a valid option."
	}
}

$exePath = Get-XoreosExecutable -RepoRoot $repoRoot -BuildDir $BuildDir -Configuration $Configuration
$configPath = Get-XoreosConfigPath
$configTargets = Get-ConfigTargets -ConfigPath $configPath
$detectedGames = Get-DetectedGames

$combined = @()
$seenPaths = @{}

foreach ($entry in @($configTargets)) {
	if (-not $entry) {
		continue
	}

	$key = "$($entry.LaunchKind)|$($entry.LaunchValue)"
	if (-not $seenPaths.ContainsKey($key)) {
		$combined += $entry
		$seenPaths[$key] = $true
	}
}

foreach ($entry in @($detectedGames)) {
	if (-not $entry) {
		continue
	}

	$key = "$($entry.LaunchKind)|$($entry.LaunchValue)"
	if (-not $seenPaths.ContainsKey($key)) {
		$combined += $entry
		$seenPaths[$key] = $true
	}
}

if ($ListSources) {
	Write-Host "Executable : $(if ($exePath) { $exePath } else { '<not built yet>' })"
	if ($configPath) {
		Write-Host "Config file: $configPath"
	}
	foreach ($entry in $combined) {
		Write-Host ("{0} | {1} | {2}" -f $entry.Source, $entry.Name, $entry.Path)
	}
	return
}

$launchKind = ""
$launchValue = ""

if ($Target) {
	$launchKind = "Target"
	$launchValue = $Target
} elseif ($GamePath) {
	$launchKind = "Path"
	$launchValue = [System.IO.Path]::GetFullPath($GamePath)
} else {
	Write-Host "xoreos launcher"
	Write-Host "Executable : $(if ($exePath) { $exePath } else { '<not built yet>' })"
	if ($configPath) {
		Write-Host "Config file: $configPath"
	}

	$choice = Get-LaunchChoice -Sources $combined
	$launchKind = $choice.LaunchKind
	$launchValue = $choice.LaunchValue
}

if (-not $exePath) {
	throw "xoreos.exe was not found in the known build directories. Run scripts/bootstrap_windows.ps1 first."
}

$dllPaths = @(Add-RuntimeDllPaths -RepoRoot $repoRoot)

$arguments = New-Object System.Collections.Generic.List[string]
if ($launchKind -eq "Target") {
	$arguments.Add($launchValue)
} else {
	$resolvedGamePath = [System.IO.Path]::GetFullPath($launchValue)
	$arguments.Add("-p$resolvedGamePath")
}

if ($Width -gt 0) {
	$arguments.Add("--width=$Width")
}
if ($Height -gt 0) {
	$arguments.Add("--height=$Height")
}
if ($Fullscreen) {
	$arguments.Add("--fullscreen=true")
}
if ($Windowed) {
	$arguments.Add("--fullscreen=false")
}
if ($SkipVideos) {
	$arguments.Add("--skipvideos=true")
}

if ($LogDir) {
	$resolvedLogDir = [System.IO.Path]::GetFullPath($LogDir)
	if (-not (Test-Path $resolvedLogDir)) {
		New-Item -ItemType Directory -Path $resolvedLogDir -Force | Out-Null
	}

	$arguments.Add("--logfile=$(Join-Path $resolvedLogDir 'xoreos-runtime.log')")
	$arguments.Add("--consolelog=$(Join-Path $resolvedLogDir 'xoreos-console.log')")
}

Write-Host ""
Write-Host ("Launching: {0} {1}" -f $exePath, ($arguments -join " "))
if ($dllPaths.Count -gt 0) {
	Write-Host ("Using runtime DLL search paths: {0}" -f ($dllPaths -join ", "))
}
& $exePath @arguments
