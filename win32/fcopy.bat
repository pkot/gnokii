@echo off
if not "%1"=="make" goto info
echo Preparing files
copy ..\..\common\*.c *.c > nul
copy ..\..\common\data\*.c *.c > nul
copy ..\..\common\devices\*.c *.c > nul
copy ..\..\common\links\*.c *.c > nul
copy ..\..\common\phones\*.c *.c > nul
copy ..\..\common\files\*.c *.c > nul
copy ..\..\getopt\win32\*.c *.c > nul
copy ..\..\getopt\win32\*.h *.h > nul
copy ..\..\gnokii\gnokii.c gnokii.c > nul
copy ..\..\gnokii\gnokii.h gnokii.h > nul
copy ..\..\include\*.h *.h > nul
mkdir data > nul
copy ..\..\include\data\*.h data\*.h >nul
mkdir devices > nul
copy ..\..\include\devices\*.h devices\*.h >nul
mkdir links > nul
copy ..\..\include\links\*.h links\*.h >nul
mkdir phones > nul
copy ..\..\include\phones\*.h phones\*.h >nul
del unixserial.* > nul
copy config1 config.h
goto end
:info
echo Used by bat in subdirs for copying source files
:end