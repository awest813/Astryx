xoreos Portable (Windows x64)
=============================

Quick start:
1. Extract this zip to a writable folder (for example C:\Games\xoreos).
2. Double-click Launch_xoreos.bat (or Launch_xoreos_GUI.bat if Python is installed).
3. Pick a detected game install or enter your game folder manually.
4. Start with windowed mode first if this is your first launch.

Notes:
- This package includes runtime DLL dependencies in bin\Release.
- Runtime logs are written to logs\xoreos-runtime.log and logs\xoreos-console.log.
- Game assets are not bundled. Point xoreos at your installed game folder.
- Saved targets are stored in your standard xoreos config location under APPDATA.

Command-line alternative:
  powershell -ExecutionPolicy Bypass -File scripts\run_xoreos_windows.ps1 -BuildDir . -Configuration Release
