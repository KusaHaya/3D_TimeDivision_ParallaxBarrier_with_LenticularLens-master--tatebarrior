@echo off
setlocal
cd /d "%~dp0"
rem Change COM3 to the Arduino port if necessary.
set "PROJECTPB_SERIAL_PORT=COM3"
rem Horizontal correction gain is saved in ProjectPB_settings.ini (P key).
rem Remove "rem" below only to force a value; it overrides the saved one.
rem set "PROJECTPB_HORIZONTAL_GAIN=0.7"
rem EyeTracker-RS OSC destination must match this UDP port.
set "PROJECTPB_OSC_PORT=8000"
set "PROJECTPB_OSC_X_SIGN=-1"
set "PROJECTPB_OSC_Y_SIGN=-1"
rem Render thread priority: remove "rem" to compare [DROP] counts without the boost.
rem set "PROJECTPB_RENDER_PRIORITY=normal"
set "PROJECTPB_VIDEO=%~dp0media\avater.mp4"
echo Arduino: %PROJECTPB_SERIAL_PORT%
echo EyeTracker-RS: OSC UDP 127.0.0.1:%PROJECTPB_OSC_PORT%
echo Start EyeTracker-RS before pressing h in ProjectPB.
"%~dp0ProjectPB Slanted barrier test.ver.exe"
echo Exit code: %ERRORLEVEL%
pause
endlocal