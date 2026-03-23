param(
	[string]$BuildDir = "build"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$platformExt = if ($env:OS -eq "Windows_NT") { ".exe" } else { "" }

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
			"bin/Debug/tests_engines_kotorbase_test_endar_spire_golden$platformExt"
		)
	},
	@{
		Name = "Combat Formula Regression"
		Paths = @(
			"tests/engines/kotorbase/test_combat$platformExt",
			"bin/Debug/tests_engines_kotorbase_test_combat$platformExt"
		)
	},
	@{
		Name = "Crash Regression Guards"
		Paths = @(
			"tests/smoke/crash_regression$platformExt",
			"bin/Debug/tests_smoke_crash_regression$platformExt"
		)
	}
)

$failures = @()

Write-Host "Milestone A smoke check"
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
	Write-Error ("Milestone A smoke failed: " + ($failures -join "; "))
}

Write-Host "Milestone A smoke passed."
Write-Host "For manual acceptance walkthrough, see docs/MILESTONE_A_SMOKE.md."
