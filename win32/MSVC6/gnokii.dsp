# Microsoft Developer Studio Project File - Name="gnokii" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=gnokii - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "gnokii.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "gnokii.mak" CFG="gnokii - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "gnokii - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "gnokii - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "gnokii - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x415 /d "NDEBUG"
# ADD RSC /l 0x415 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "gnokii - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE RSC /l 0x415 /d "_DEBUG"
# ADD RSC /l 0x415 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "gnokii - Win32 Release"
# Name "gnokii - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=atbus.c
# End Source File
# Begin Source File

SOURCE=ateric.c
# End Source File
# Begin Source File

SOURCE=atgen.c
# End Source File
# Begin Source File

SOURCE=atnok.c
# End Source File
# Begin Source File

SOURCE=atsie.c
# End Source File
# Begin Source File

SOURCE=cfgreader.c
# End Source File
# Begin Source File

SOURCE=device.c
# End Source File
# Begin Source File

SOURCE="fbus-3110.c"
# End Source File
# Begin Source File

SOURCE="fbus-phonet.c"
# End Source File
# Begin Source File

SOURCE=fbus.c
# End Source File
# Begin Source File

SOURCE=generic.c
# End Source File
# Begin Source File

SOURCE=getopt.c
# End Source File
# Begin Source File

SOURCE=gnokii.c
# End Source File
# Begin Source File

SOURCE="gsm-api.c"
# End Source File
# Begin Source File

SOURCE="gsm-bitmaps.c"
# End Source File
# Begin Source File

SOURCE="gsm-common.c"
# End Source File
# Begin Source File

SOURCE="gsm-encoding.c"
# End Source File
# Begin Source File

SOURCE="gsm-error.c"
# End Source File
# Begin Source File

SOURCE="gsm-filetypes.c"
# End Source File
# Begin Source File

SOURCE="gsm-networks.c"
# End Source File
# Begin Source File

SOURCE="gsm-ringtones.c"
# End Source File
# Begin Source File

SOURCE="gsm-sms.c"
# End Source File
# Begin Source File

SOURCE="gsm-statemachine.c"
# End Source File
# Begin Source File

SOURCE=misc.c
# End Source File
# Begin Source File

SOURCE=nk3110.c
# End Source File
# Begin Source File

SOURCE=nk6100.c
# End Source File
# Begin Source File

SOURCE=nk7110.c
# End Source File
# Begin Source File

SOURCE=nokia.c
# End Source File
# Begin Source File

SOURCE=winserial.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=links\atbus.h
# End Source File
# Begin Source File

SOURCE=phones\ateric.h
# End Source File
# Begin Source File

SOURCE=phones\atgen.h
# End Source File
# Begin Source File

SOURCE=phones\atnok.h
# End Source File
# Begin Source File

SOURCE=phones\atsie.h
# End Source File
# Begin Source File

SOURCE=links\cbus.h
# End Source File
# Begin Source File

SOURCE="links\fbus-3110.h"
# End Source File
# Begin Source File

SOURCE="links\fbus-phonet.h"
# End Source File
# Begin Source File

SOURCE=links\fbus.h
# End Source File
# Begin Source File

SOURCE=phones\generic.h
# End Source File
# Begin Source File

SOURCE=win32\getopt.h
# End Source File
# Begin Source File

SOURCE=phones\nk2110.h
# End Source File
# Begin Source File

SOURCE=phones\nk3110.h
# End Source File
# Begin Source File

SOURCE=phones\nk6100.h
# End Source File
# Begin Source File

SOURCE=phones\nk7110.h
# End Source File
# Begin Source File

SOURCE=phones\nokia.h
# End Source File
# Begin Source File

SOURCE=devices\winserial.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
