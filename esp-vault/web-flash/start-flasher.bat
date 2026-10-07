@echo off
echo Starting LocalVault Web Flasher on http://localhost:8000
echo Open Chrome or Edge and go to http://localhost:8000
echo Press Ctrl+C to stop.
echo.
python -m http.server 8000
pause
