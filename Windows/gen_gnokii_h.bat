@echo off

set GNOKII_H_IN_PATH=..\include
set SED=sed.exe

if not exist %GNOKII_H_IN_PATH%\gnokii.h.in goto :path_error
call :path_search %SED%
if not exist %SED_PATH%%SED% goto :sed_error

findstr /r /c:"^GNOKII_LT_VERSION_" ..\configure.in > %TEMP%\gnokii_ver.txt

for /F %%i in (%TEMP%\gnokii_ver.txt) do set %%i
del /q %TEMP%\gnokii_ver.txt

rem echo Got versions!
rem echo GNOKII_LT_VERSION_CURRENT: %GNOKII_LT_VERSION_CURRENT%
rem echo GNOKII_LT_VERSION_REVISION: %GNOKII_LT_VERSION_REVISION%
rem echo GNOKII_LT_VERSION_AGE: %GNOKII_LT_VERSION_AGE%

rem Determine LibGnokii version
set /a LIBGNOKII_VERSION_MAJOR=%GNOKII_LT_VERSION_CURRENT% - %GNOKII_LT_VERSION_AGE%
set LIBGNOKII_VERSION_MINOR=%GNOKII_LT_VERSION_AGE%
set LIBGNOKII_VERSION_RELEASE=%GNOKII_LT_VERSION_REVISION%
set LIBGNOKII_VERSION_STRING=%LIBGNOKII_VERSION_MAJOR%.%LIBGNOKII_VERSION_MINOR%.%LIBGNOKII_VERSION_RELEASE%

echo Ver: %LIBGNOKII_VERSION_STRING%


rem include\gnokii.h.in has:
rem #define LIBGNOKII_VERSION_STRING "@LIBGNOKII_VERSION_STRING@"
rem #define LIBGNOKII_VERSION_MAJOR @LIBGNOKII_VERSION_MAJOR@
rem #define LIBGNOKII_VERSION_MINOR @LIBGNOKII_VERSION_MINOR@
rem #define LIBGNOKII_VERSION_RELEASE @LIBGNOKII_VERSION_RELEASE@

rem Prepare
echo s/@LIBGNOKII_VERSION_STRING@/%LIBGNOKII_VERSION_STRING%/ > %temp%\gnokii_version.sed
echo s/@LIBGNOKII_VERSION_MAJOR@/%LIBGNOKII_VERSION_MAJOR%/ >> %temp%\gnokii_version.sed
echo s/@LIBGNOKII_VERSION_MINOR@/%LIBGNOKII_VERSION_MINOR%/ >> %temp%\gnokii_version.sed
echo s/@LIBGNOKII_VERSION_RELEASE@/%LIBGNOKII_VERSION_RELEASE%/ >> %temp%\gnokii_version.sed

rem Execute sed with parameters
sed -f %temp%\gnokii_version.sed %GNOKII_H_IN_PATH%\gnokii.h.in > %GNOKII_H_IN_PATH%\gnokii.h
del /q %TEMP%\gnokii_version.sed

echo done!

goto :EOF

:path_search
set SED_PATH=
set SED_PATH=%~dp$PATH:1%
goto :EOF

:sed_error
echo Error! Cannot find sed.exe in path
goto :EOF

:path_error
echo Error! Cannot find gnokii.h.in
goto :EOF
