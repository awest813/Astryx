@echo off
setlocal
cd /d "%~dp0"
python "%~dp0scripts\xoreos_launcher_gui.py"
echo.
echo GUI closed.
pause
endlocal
