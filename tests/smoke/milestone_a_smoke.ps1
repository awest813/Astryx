param(
	[string]$BuildDir = "build"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$platformExt = if ($env:OS -eq "Windows_NT") { ".exe" } else { "" }

$tests = @(
	@{
		Name = "Endar Spire Golden Path"
		Path = "tests/engines/kotorbase/test_endar_spire_golden$platformExt"
	},
	@{
		Name = "Combat Formula Regression"
		Path = "tests/engines/kotorbase/test_combat$platformExt"
	},
	@{
		Name = "Crash Regression Guards"
		Path = "tests/smoke/crash_regression$platformExt"
	}
)

$failures = @()

Write-Host "Milestone A smoke check"
Write-Host "Build directory: $BuildDir"

foreach ($test in $tests) {
	$fullPath = Join-Path $BuildDir $test.Path
	if (!(Test-Path $fullPath)) {
		$failures += "missing binary: $($test.Path)"
		Write-Warning "Missing $($test.Name) at $fullPath"
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
