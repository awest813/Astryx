param(
	[string]$BuildDir = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$platformExt = if ($env:OS -eq "Windows_NT") { ".exe" } else { "" }

if ([string]::IsNullOrWhiteSpace($BuildDir)) {
	if (Test-Path "build-vcpkg") {
		$BuildDir = "build-vcpkg"
	} else {
		$BuildDir = "build"
	}
}

function Resolve-TestBinary {
	param(
		[string]$BuildDir,
		[string[]]$Candidates
	)

	foreach ($candidate in $Candidates) {
		$fullPath = Join-Path $BuildDir $candidate
		if (Test-Path $fullPath) {
			return $fullPath
		}
	}

	return $null
}

$tests = @(
	@{
		Name = "Endar Spire Golden Path"
		Paths = @(
			"tests/engines/kotorbase/test_endar_spire_golden$platformExt",
			"bin/Release/tests_engines_kotorbase_test_endar_spire_golden$platformExt",
			"bin/Debug/tests_engines_kotorbase_test_endar_spire_golden$platformExt"
		)
	},
	@{
		Name = "Taris Progression State"
		Paths = @(
			"tests/engines/kotorbase/test_taris_progression$platformExt",
			"bin/Release/tests_engines_kotorbase_test_taris_progression$platformExt",
			"bin/Debug/tests_engines_kotorbase_test_taris_progression$platformExt"
		)
	},
	@{
		Name = "Alignment/XP Baseline"
		Paths = @(
			"tests/engines/kotorbase/test_alignment_xp$platformExt",
			"bin/Release/tests_engines_kotorbase_test_alignment_xp$platformExt",
			"bin/Debug/tests_engines_kotorbase_test_alignment_xp$platformExt"
		)
	},
	@{
		Name = "Combat Baseline"
		Paths = @(
			"tests/engines/kotorbase/test_combat$platformExt",
			"bin/Release/tests_engines_kotorbase_test_combat$platformExt",
			"bin/Debug/tests_engines_kotorbase_test_combat$platformExt"
		)
	},
	@{
		Name = "NWScript Function Stability"
		Paths = @(
			"tests/engines/kotorbase/test_nwscript_functions$platformExt",
			"bin/Release/tests_engines_kotorbase_test_nwscript_functions$platformExt",
			"bin/Debug/tests_engines_kotorbase_test_nwscript_functions$platformExt"
		)
	},
	@{
		Name = "Crash Regression Guards"
		Paths = @(
			"tests/smoke/crash_regression$platformExt",
			"bin/Release/tests_smoke_crash_regression$platformExt",
			"bin/Debug/tests_smoke_crash_regression$platformExt"
		)
	}
)

$failures = @()

Write-Host "KotOR load-to-Taris progression smoke check"
Write-Host "Build directory: $BuildDir"

foreach ($test in $tests) {
	$fullPath = Resolve-TestBinary -BuildDir $BuildDir -Candidates $test.Paths
	if (!$fullPath) {
		$failures += "missing binary: $($test.Paths -join ', ')"
		Write-Warning "Missing $($test.Name) under $BuildDir"
		continue
	}

	Write-Host "Running $($test.Name)..."
	& $fullPath
	if ($LASTEXITCODE -ne 0) {
		$failures += "$($test.Name) failed with exit code $LASTEXITCODE"
	}
}

if ($failures.Count -gt 0) {
	Write-Error ("KotOR load-to-Taris smoke failed: " + ($failures -join "; "))
}

Write-Host "KotOR load-to-Taris progression smoke passed."
Write-Host "For manual progression checks, see docs/KOTOR_PROGRESS_TO_DANTOOINE_SMOKE.md."
