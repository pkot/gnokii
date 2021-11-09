# Microsoft Developer Studio Project File - Name="libgnokii" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=libgnokii - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libgnokii.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libgnokii.mak" CFG="libgnokii - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libgnokii - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libgnokii - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libgnokii - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GNOKIIDLL_EXPORTS" /D "HAVE_CONFIG_H" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX- /O2 /I "." /I "../../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "HAVE_CONFIG_H" /D "LIBGNOKII_DLL_EXPORT" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib ws2_32.lib /nologo /dll /machine:I386
# ADD LINK32 /nologo /dll /machine:I386
# SUBTRACT LINK32 /verbose /nodefaultlib

!ELSEIF  "$(CFG)" == "libgnokii - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GNOKIIDLL_EXPORTS" /D "HAVE_CONFIG_H" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /GX /Zi /Od /I "." /I "../../include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "HAVE_CONFIG_H" /D "LIBGNOKII_DLL_EXPORT" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib ws2_32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /dll /incremental:no /debug /machine:I386 /out:"Debug/gnokiid.dll"
# SUBTRACT LINK32 /nodefaultlib

!ENDIF 

# Begin Target

# Name "libgnokii - Win32 Release"
# Name "libgnokii - Win32 Debug"
# Begin Group "links"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\links\atbus.c
# End Source File
# Begin Source File

SOURCE=..\..\common\links\cbus.c
# End Source File
# Begin Source File

SOURCE="..\..\common\links\fbus-3110.c"
# End Source File
# Begin Source File

SOURCE="..\..\common\links\fbus-phonet.c"
# End Source File
# Begin Source File

SOURCE=..\..\common\links\fbus.c
# End Source File
# Begin Source File

SOURCE=..\..\common\links\gnbus.c
# End Source File
# Begin Source File

SOURCE=..\..\common\links\m2bus.c
# End Source File
# Begin Source File

SOURCE=..\..\common\links\utils.c
# End Source File
# End Group
# Begin Group "phones"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\phones\atbosch.c
# End Source File
# Begin Source File

SOURCE=..\..\common\phones\ateric.c
# End Source File
# Begin Source File

SOURCE=..\..\common\phones\atgen.c
# End Source File
# Begin Source File

SOURCE=..\..\common\phones\athuawei.c
# End Source File
# Begin Source File

SOURCE=..\..\common\phones\atlg.c
# End Source File
# Begin Source File

SOURCE=..\..\common\phones\atmot.c
# End Source File
# Begin Source File

SOURCE=..\..\common\phones\atnok.c
# End Source File
# Begin Source File

SOURCE=..\..\common\phones\atsag.c
# End Source File
# Begin Source File

SOURCE=..\..\common\phones\atsam.c
# End Source File
# Begin Source File

SOURCE=..\..\common\phones\atsie.c
# End Source File
# Begin Source File

SOURCE=..\..\common\phones\atsoer.c
# End Source File
# Begin Source File

SOURCE=..\..\common\phones\dc2711.c
# End Source File
# Begin Source File

SOURCE=..\..\common\phones\fake.c
# End Source File
# Begin Source File

SOURCE=..\..\common\phones\generic.c
# End Source File
# Begin Source File

SOURCE=..\..\common\phones\gnapplet.c
# End Source File
# Begin Source File

SOURCE=..\..\common\phones\nk2110.c
# End Source File
# Begin Source File

SOURCE=..\..\common\phones\nk3110.c
# End Source File
# Begin Source File

SOURCE=..\..\common\phones\nk6100.c
# End Source File
# Begin Source File

SOURCE=..\..\common\phones\nk6160.c
# End Source File
# Begin Source File

SOURCE=..\..\common\phones\nk6510.c
# End Source File
# Begin Source File

SOURCE=..\..\common\phones\nk7110.c
# End Source File
# Begin Source File

SOURCE=..\..\common\phones\nokia.c
# End Source File
# Begin Source File

SOURCE=..\..\common\phones\pcsc.c
# End Source File
# End Group
# Begin Group "devices"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\devices\tekram.c
# End Source File
# Begin Source File

SOURCE=..\..\common\devices\winserial.c
# End Source File
# End Group
# Begin Group "include"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\gnokii.h.in

!IF  "$(CFG)" == "libgnokii - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Generating gnokii.h
IntDir=.\Release
ProjDir=.
InputPath=..\..\include\gnokii.h.in

"$(IntDir)\gnokii.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	$(ProjDir)\..\gen_gnokii_h.bat $(IntDir)\gnokii.h

# End Custom Build

!ELSEIF  "$(CFG)" == "libgnokii - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Generating gnokii.h
IntDir=.\Debug
ProjDir=.
InputPath=..\..\include\gnokii.h.in

"$(IntDir)\gnokii.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	$(ProjDir)\..\gen_gnokii_h.bat $(IntDir)\gnokii.h

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=..\..\common\cfgreader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\compat.c
# End Source File
# Begin Source File

SOURCE=..\..\common\device.c
# End Source File
# Begin Source File

SOURCE="..\..\common\gsm-api.c"
# End Source File
# Begin Source File

SOURCE="..\..\common\gsm-auth.c"
# End Source File
# Begin Source File

SOURCE="..\..\common\gsm-bitmaps.c"
# End Source File
# Begin Source File

SOURCE="..\..\common\gsm-call.c"
# End Source File
# Begin Source File

SOURCE="..\..\common\gsm-common.c"
# End Source File
# Begin Source File

SOURCE="..\..\common\gsm-encoding.c"
# End Source File
# Begin Source File

SOURCE="..\..\common\gsm-error.c"
# End Source File
# Begin Source File

SOURCE="..\..\common\gsm-filetypes.c"
# End Source File
# Begin Source File

SOURCE="..\..\common\gsm-mms.c"
# End Source File
# Begin Source File

SOURCE="..\..\common\gsm-networks.c"
# End Source File
# Begin Source File

SOURCE="..\..\common\gsm-ringtones.c"
# End Source File
# Begin Source File

SOURCE="..\..\common\gsm-sms.c"
# End Source File
# Begin Source File

SOURCE="..\..\common\gsm-statemachine.c"
# End Source File
# Begin Source File

SOURCE=..\..\common\ldif.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libfunctions.c
# End Source File
# Begin Source File

SOURCE=..\..\common\localcharset.c
# End Source File
# Begin Source File

SOURCE=..\..\common\map.c
# End Source File
# Begin Source File

SOURCE=..\..\common\misc.c
# End Source File
# Begin Source File

SOURCE="..\..\common\nokia-decoding.c"
# End Source File
# Begin Source File

SOURCE=..\..\common\pkt.c
# End Source File
# Begin Source File

SOURCE=..\..\common\readmidi.c
# End Source File
# Begin Source File

SOURCE="..\..\common\sms-nokia.c"
# End Source File
# Begin Source File

SOURCE=..\..\common\vcal.c
# End Source File
# Begin Source File

SOURCE=..\..\common\vcard.c
# End Source File
# End Target
# End Project
