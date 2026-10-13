@echo off
REM Stop the hidden LocalVault server.
powershell -NoProfile -Command "$server = [System.IO.Path]::GetFullPath('%~dp0server.js'); $escaped = [regex]::Escape($server); Get-CimInstance Win32_Process -Filter \"Name='node.exe'\" | Where-Object { $_.CommandLine -match $escaped } | ForEach-Object { Stop-Process -Id $_.ProcessId -Force }"
echo LocalVault server stopped.
