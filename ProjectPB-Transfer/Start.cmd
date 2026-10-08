@echo off
setlocal
cd /d "%~dp0"
rem Set the Arduino port shown in Windows Device Manager.
set "PROJECTPB_SERIAL_PORT=COM3"
set "PROJECTPB_VIDEO=.\media\avater.mp4"
"ProjectPB Slanted barrier test.ver.exe"
if errorlevel 1 pause
endlocal
