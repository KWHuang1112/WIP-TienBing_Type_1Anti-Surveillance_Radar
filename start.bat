@echo off

REM Run Python script in background
start "" python screenSwitcher.py

REM wait 5 seconds
timeout /t 5 >nul

REM Open website
start "" "http://192.168.4.1"

echo Done!
pause