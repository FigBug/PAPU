echo on
cd "%~dp0%"
set ROOT=%cd%
cd "%ROOT%"
echo "%ROOT%"

call pcresaveprojects.bat
call pcbuildprojects.bat
