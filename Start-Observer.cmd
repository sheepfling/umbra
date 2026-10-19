@echo off
setlocal
cd /d "%~dp0"
where python >nul 2>&1
if not errorlevel 1 goto use_python
py -3 -c "import sys" >nul 2>&1
if not errorlevel 1 goto use_py
echo Python 3.11 or newer is needed. Install Python, then try again.
echo Download: https://www.python.org/downloads/
pause
exit /b 1

:use_py
py -3 "%~dp0tools\observer_playtest.py" %*
goto finished

:use_python
python "%~dp0tools\observer_playtest.py" %*

:finished
if not errorlevel 1 exit /b 0
echo.
echo The playtest could not start. The message above explains what to check.
pause
exit /b 1
