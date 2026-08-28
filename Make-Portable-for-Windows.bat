@echo off
setlocal
cd /d "%~dp0"

echo Building and validating the DISSCO portable package...
echo.
rem A PowerShell 7 parent can pass incompatible module paths through CMD.
rem Let Windows PowerShell recreate its own defaults for this child only.
set "PSModulePath="
"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoLogo -NoProfile -ExecutionPolicy Bypass ^
  -File "%~dp0Make-Portable-for-Windows.ps1" %*

if errorlevel 1 (
  echo.
  echo Portable package creation FAILED.
  pause
  exit /b 1
)

echo.
echo Portable package creation completed.
echo See the dist folder for the portable directory and ZIP.
pause
