@echo off
REM Start LocalVault server hidden, open it in the browser, close this window.
setlocal
cd /d "%~dp0"
where node >nul 2>&1
if errorlevel 1 (
  echo Node.js not found. Install Node 18+ first: https://nodejs.org
  pause
  exit /b 1
)
set "VBS=%TEMP%\LocalVaultServer.vbs"
(
  echo Set WshShell = CreateObject^("WScript.Shell"^)
  echo WshShell.Run "node ""%~dp0server.js""", 0, False
) > "%VBS%"
wscript "%VBS%"
start "" "http://127.0.0.1:18765"
echo Server started hidden. Stop it with stop-windows.bat.
exit
