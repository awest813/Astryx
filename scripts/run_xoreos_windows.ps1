<#
.SYNOPSIS
Start xoreos from PowerShell: pick a saved game or a detected install, then launch.

.DESCRIPTION
Without -GamePath or -Target, lists your saved games and the installs found in
the usual Steam and GOG folders, and asks which one to start. For a window
instead, use scripts\xoreos_launcher_gui.py.

.PARAMETER GamePath
Start the game installed in this folder (the folder with swkotor.exe).

.PARAMETER Target
Start a game saved in xoreos.conf under this name.

.PARAMETER BuildDir
Use the xoreos.exe from this build folder (relative to the repository, or absolute).

.PARAMETER Configuration
Preferred build configuration when several exist (Debug or Release).

.PARAMETER LogDir
Write xoreos-runtime.log and xoreos-console.log into this folder.

.PARAMETER ListSources
Print the engine, config file and games found, then exit.

.EXAMPLE
powershell -ExecutionPolicy Bypass -File scripts\run_xoreos_windows.ps1 -GamePath "C:\GOG Games\Star Wars - KotOR"
#>
param(
	[string]$GamePath = "",
	[string]$Target = "",
	[string]$BuildDir = "",
	[string]$Configuration = "",
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

# Program files xoreos uses to recognise each game (src/engines/*/probes.cpp)
$gamePrograms = @(
	@{ Label = "KotOR I"; Program = "swkotor.exe" },
	@{ Label = "KotOR II"; Program = "swkotor2.exe" },
	@{ Label = "Neverwinter Nights"; Program = "nwmain.exe" },
	@{ Label = "Jade Empire"; Program = "JadeEmpire.exe" }
)

function Stop-WithMessage {
	param([string[]]$Lines)

	foreach ($line in $Lines) {
		Write-Host $line -ForegroundColor Red
	}
	exit 1
}

function Resolve-RepoPath {
	param([string]$Path)

	if ([System.IO.Path]::IsPathRooted($Path)) {
		return $Path
	}
	return (Join-Path $repoRoot $Path)
}

# The newest xoreos.exe: XOREOS_EXE, then -BuildDir, then any build folder.
function Get-XoreosExecutable {
	if ($env:XOREOS_EXE) {
		if (Test-Path -LiteralPath $env:XOREOS_EXE -PathType Leaf) {
			return $env:XOREOS_EXE
		}
		Stop-WithMessage @("XOREOS_EXE is set, but '$($env:XOREOS_EXE)' isn't a file.")
	}

	$bases = @()
	if ($BuildDir) {
		$bases += (Resolve-RepoPath $BuildDir)
	} else {
		$bases += $repoRoot
		foreach ($pattern in @("build*", "builds\*")) {
			$bases += @(Get-ChildItem -Path (Join-Path $repoRoot $pattern) -Directory -ErrorAction SilentlyContinue | ForEach-Object { $_.FullName })
		}
	}

	$configs = @("", "Release", "RelWithDebInfo", "MinSizeRel", "Debug")
	if ($Configuration) {
		$configs = @($Configuration) + ($configs | Where-Object { $_ -ne $Configuration })
	}

	$found = @()
	foreach ($base in $bases) {
		foreach ($config in $configs) {
			$candidate = if ($config) { Join-Path $base "bin\$config\xoreos.exe" } else { Join-Path $base "bin\xoreos.exe" }
			if (Test-Path -LiteralPath $candidate -PathType Leaf) {
				$found += Get-Item -LiteralPath $candidate
			}
		}
	}

	if ($found.Count -eq 0) {
		return $null
	}
	if ($Configuration) {
		$preferred = @($found | Where-Object { $_.Directory.Name -eq $Configuration })
		if ($preferred.Count -gt 0) {
			$found = $preferred
		}
	}
	return ($found | Sort-Object LastWriteTime -Descending | Select-Object -First 1).FullName
}

function Add-RuntimeDllPaths {
	param([string]$ExePath)

	$debug = $ExePath -match '\\Debug\\'
	$installed = @()
	foreach ($pattern in @("build*\vcpkg_installed\*", "builds\*\vcpkg_installed\*", ".deps\vcpkg\installed\*")) {
		$installed += @(Get-ChildItem -Path (Join-Path $repoRoot $pattern) -Directory -ErrorAction SilentlyContinue)
	}

	$existing = @()
	foreach ($dir in $installed) {
		$candidates = @(Join-Path $dir.FullName "bin")
		if ($debug) {
			$candidates = @((Join-Path $dir.FullName "debug\bin")) + $candidates
		}
		foreach ($candidate in $candidates) {
			if (Test-Path -LiteralPath $candidate) {
				$existing += [System.IO.Path]::GetFullPath($candidate)
			}
		}
	}

	if ($existing.Count -gt 0) {
		$pathParts = @()
		if ($env:PATH) {
			$pathParts = $env:PATH -split ';'
		}
		$toPrepend = @($existing | Where-Object { $pathParts -notcontains $_ })
		if ($toPrepend.Count -gt 0) {
			$env:PATH = ($toPrepend -join ';') + ';' + $env:PATH
		}
	}

	return $existing
}

# Where xoreos reads its config and writes its log (src/common/platform.cpp)
function Get-XoreosConfigDir {
	if ($env:APPDATA) {
		return (Join-Path $env:APPDATA "xoreos")
	}
	if ($env:USERPROFILE) {
		return (Join-Path $env:USERPROFILE "xoreos")
	}
	return (Get-Location).Path
}

function Get-ConfigTargets {
	param([string]$ConfigPath)

	$entries = @()
	if (-not $ConfigPath -or -not (Test-Path -LiteralPath $ConfigPath)) {
		return $entries
	}

	$currentName = $null
	$currentPath = $null
	$currentDescription = $null

	$flush = {
		if ($currentName -and $currentName -ne "xoreos" -and $currentPath) {
			[pscustomobject]@{
				Name = $currentName
				Path = $currentPath
				Description = $currentDescription
				Source = "Saved"
				LaunchKind = "Target"
				LaunchValue = $currentName
			}
		}
	}

	foreach ($line in Get-Content -LiteralPath $ConfigPath) {
		$trimmed = $line.Trim()
		if (-not $trimmed -or $trimmed.StartsWith(";")) {
			continue
		}
		$trimmed = ($trimmed -split '#', 2)[0].Trim()
		if (-not $trimmed) {
			continue
		}

		if ($trimmed -match '^\[(.+)\]$') {
			$entries += @(& $flush)
			$currentName = $matches[1]
			$currentPath = $null
			$currentDescription = $null
			continue
		}

		if (-not $currentName) {
			continue
		}

		if ($trimmed -match '^path\s*=\s*(.+)$') {
			$currentPath = $matches[1].Trim()
		} elseif ($trimmed -match '^description\s*=\s*(.+)$') {
			$currentDescription = $matches[1].Trim()
		}
	}

	$entries += @(& $flush)
	return $entries
}

function Get-GameInFolder {
	param([string]$Folder)

	foreach ($game in $gamePrograms) {
		if (Test-Path -LiteralPath (Join-Path $Folder $game.Program) -PathType Leaf) {
			return $game.Label
		}
	}
	return $null
}

# Folders whose subfolders may be game installs: Steam libraries and GOG folders
function Get-SearchRoots {
	$steamRoots = @()
	try {
		$steamPath = (Get-ItemProperty -Path "HKCU:\Software\Valve\Steam" -Name SteamPath -ErrorAction Stop).SteamPath
		if ($steamPath) {
			$steamRoots += $steamPath
		}
	} catch {
	}
	foreach ($programFiles in @(${env:ProgramFiles(x86)}, $env:ProgramFiles)) {
		if ($programFiles) {
			$steamRoots += (Join-Path $programFiles "Steam")
		}
	}

	$roots = @()
	foreach ($steam in $steamRoots) {
		if (-not (Test-Path -LiteralPath $steam)) {
			continue
		}
		$roots += (Join-Path $steam "steamapps\common")
		foreach ($vdf in @((Join-Path $steam "steamapps\libraryfolders.vdf"), (Join-Path $steam "config\libraryfolders.vdf"))) {
			if (Test-Path -LiteralPath $vdf) {
				foreach ($match in [regex]::Matches((Get-Content -LiteralPath $vdf -Raw), '"path"\s+"([^"]+)"')) {
					$roots += (Join-Path ($match.Groups[1].Value -replace '\\\\', '\') "steamapps\common")
				}
			}
		}
	}

	foreach ($programFiles in @(${env:ProgramFiles(x86)}, $env:ProgramFiles)) {
		if ($programFiles) {
			$roots += (Join-Path $programFiles "GOG Galaxy\Games")
		}
	}
	# Only local hard drives: probing an empty CD drive can pop up a "no disk" dialog
	try {
		foreach ($drive in @(Get-CimInstance -ClassName Win32_LogicalDisk -Filter "DriveType=3")) {
			$roots += "$($drive.DeviceID)\GOG Games"
			$roots += "$($drive.DeviceID)\Games"
		}
	} catch {
	}

	$unique = @()
	foreach ($root in $roots) {
		if ((Test-Path -LiteralPath $root) -and -not ($unique -contains $root)) {
			$unique += $root
		}
	}
	return $unique
}

function Get-DetectedGames {
	$entries = @()
	foreach ($root in Get-SearchRoots) {
		foreach ($folder in @(Get-ChildItem -LiteralPath $root -Directory -ErrorAction SilentlyContinue)) {
			$label = Get-GameInFolder $folder.FullName
			if ($label) {
				$entries += [pscustomobject]@{
					Name = $label
					Path = $folder.FullName
					Description = $root
					Source = "Found"
					LaunchKind = "Path"
					LaunchValue = $folder.FullName
				}
			}
		}
	}
	return $entries
}

function Get-LaunchChoice {
	param([object[]]$Sources)

	if (-not $Sources -or $Sources.Count -eq 0) {
		Write-Host ""
		Write-Host "No saved games, and no installs found in the usual Steam and GOG folders."
		$manual = Read-Host "Enter the folder you installed the game in"
		if (-not $manual) {
			Stop-WithMessage @("No game folder given.")
		}
		return [pscustomobject]@{ LaunchKind = "Path"; LaunchValue = $manual }
	}

	Write-Host ""
	Write-Host "Games:"
	for ($i = 0; $i -lt $Sources.Count; $i++) {
		$entry = $Sources[$i]
		$saved = if ($entry.LaunchKind -eq "Target") { " (saved)" } else { "" }
		Write-Host ("  [{0}] {1}{2}" -f ($i + 1), $entry.Name, $saved)
		Write-Host ("      {0}" -f $entry.Path)
	}
	Write-Host "  [M] Another folder"

	while ($true) {
		$choice = (Read-Host "Choose a number or M").Trim()
		if ($choice -match '^[Mm]$') {
			$manual = Read-Host "Enter the folder you installed the game in"
			if ($manual) {
				return [pscustomobject]@{ LaunchKind = "Path"; LaunchValue = $manual }
			}
			continue
		}

		$index = 0
		if ([int]::TryParse($choice, [ref]$index) -and $index -ge 1 -and $index -le $Sources.Count) {
			return $Sources[$index - 1]
		}

		Write-Host "Please type one of the numbers shown, or M." -ForegroundColor Yellow
	}
}

$exePath = Get-XoreosExecutable
$configPath = Join-Path (Get-XoreosConfigDir) "xoreos.conf"
$logPath = Join-Path (Get-XoreosConfigDir) "xoreos.log"
$configTargets = @(Get-ConfigTargets -ConfigPath $configPath)

$combined = @()
$seenPaths = @{}
foreach ($entry in @($configTargets) + @(Get-DetectedGames)) {
	if (-not $entry) {
		continue
	}
	$key = $entry.Path.TrimEnd('\', '/').ToLowerInvariant()
	if (-not $seenPaths.ContainsKey($key)) {
		$combined += $entry
		$seenPaths[$key] = $true
	}
}

if ($ListSources) {
	Write-Host "Executable : $(if ($exePath) { $exePath } else { '<not built yet>' })"
	Write-Host "Config file: $configPath"
	Write-Host "Engine log : $logPath"
	foreach ($entry in $combined) {
		Write-Host ("{0} | {1} | {2}" -f $entry.Source, $entry.Name, $entry.Path)
	}
	exit 0
}

if (-not $exePath) {
	Stop-WithMessage @(
		"Can't find xoreos.exe. Build it first:",
		"  powershell -ExecutionPolicy Bypass -File scripts\bootstrap_windows.ps1",
		"If you built it elsewhere, pass -BuildDir or set XOREOS_EXE."
	)
}

if ($Width -lt 0 -or $Height -lt 0) {
	Stop-WithMessage @("-Width and -Height must be positive numbers.")
}
if ($Fullscreen -and $Windowed) {
	Stop-WithMessage @("Use either -Fullscreen or -Windowed, not both.")
}

$launchKind = ""
$launchValue = ""

if ($Target) {
	$launchKind = "Target"
	$launchValue = $Target
} elseif ($GamePath) {
	$launchKind = "Path"
	$launchValue = $GamePath
} else {
	Write-Host "xoreos: $exePath"
	$choice = Get-LaunchChoice -Sources $combined
	$launchKind = $choice.LaunchKind
	$launchValue = $choice.LaunchValue
}

$arguments = New-Object System.Collections.Generic.List[string]
if ($launchKind -eq "Target") {
	if (-not ($configTargets | Where-Object { $_.Name -eq $launchValue })) {
		Stop-WithMessage @(
			"There's no game saved as '$launchValue' in $configPath.",
			"Run without -Target to see your games, or use -GamePath."
		)
	}
	$arguments.Add($launchValue)
} else {
	if (-not (Test-Path -LiteralPath $launchValue -PathType Container)) {
		Stop-WithMessage @("The game folder doesn't exist: $launchValue")
	}
	$resolvedGamePath = (Resolve-Path -LiteralPath $launchValue).ProviderPath
	if (-not (Get-GameInFolder $resolvedGamePath)) {
		Write-Host "Warning: no game program (such as swkotor.exe) in $resolvedGamePath." -ForegroundColor Yellow
		Write-Host "xoreos will probably not recognise it. Use the folder that contains swkotor.exe." -ForegroundColor Yellow
	}
	$arguments.Add("--path=$resolvedGamePath")
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
	$resolvedLogDir = Resolve-RepoPath $LogDir
	New-Item -ItemType Directory -Path $resolvedLogDir -Force | Out-Null
	$logPath = Join-Path $resolvedLogDir "xoreos-runtime.log"
	$arguments.Add("--logfile=$logPath")
	$arguments.Add("--consolelog=$(Join-Path $resolvedLogDir 'xoreos-console.log')")
}

$dllPaths = @(Add-RuntimeDllPaths -ExePath $exePath)

Write-Host ""
Write-Host ("Launching: {0} {1}" -f $exePath, (($arguments | ForEach-Object { if ($_ -match '\s') { "`"$_`"" } else { $_ } }) -join " "))
if ($dllPaths.Count -gt 0) {
	Write-Host ("Using runtime DLLs from: {0}" -f ($dllPaths -join ", "))
}

& $exePath @arguments
$status = $LASTEXITCODE
if ($status -ne 0) {
	Write-Host ""
	Write-Host "xoreos stopped with an error (exit code $status). The messages above say why;" -ForegroundColor Red
	Write-Host "the full log is in $logPath" -ForegroundColor Red
}
exit $status
