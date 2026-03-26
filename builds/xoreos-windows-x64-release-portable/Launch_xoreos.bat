@echo off
setlocal
cd /d "%~dp0"
set "LOGDIR=%~dp0logs"
powershell -ExecutionPolicy Bypass -File "%~dp0scripts\run_xoreos_windows.ps1" -BuildDir "." -Configuration "Release" -LogDir "%LOGDIR%"
echo.
echo If the engine closed unexpectedly, check logs:
echo   %LOGDIR%\xoreos-runtime.log
echo   %LOGDIR%\xoreos-console.log
pause
endlocal
