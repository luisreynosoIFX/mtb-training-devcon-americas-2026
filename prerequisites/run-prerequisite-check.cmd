@echo off
set "SCRIPT=%~dp0check-prerequisites.ps1"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT%" -OutputPath "%~dp0PSOC_DevCon_prerequisites_report.html"
if errorlevel 1 pause