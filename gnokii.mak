# Microsoft Developer Studio Generated NMAKE File, Based on gnokii.dsp



!IF "$(CFG)" == ""
CFG=gnokii - Win32 Debug
!MESSAGE No configuration specified. Defaulting to gnokii - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "gnokii - Win32 Release" && "$(CFG)" != "gnokii - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
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
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "gnokii - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\gnokii.exe"


CLEAN :
	-@erase "$(INTDIR)\cfgreader.obj"
	-@erase "$(INTDIR)\fbus-6110-auth.obj"
	-@erase "$(INTDIR)\fbus-6110-ringtones.obj"
	-@erase "$(INTDIR)\fbus-6110.obj"
	-@erase "$(INTDIR)\win32\getopt.obj"
	-@erase "$(INTDIR)\gnokii.obj"
	-@erase "$(INTDIR)\gsm-api.obj"
	-@erase "$(INTDIR)\gsm-networks.obj"
	-@erase "$(INTDIR)\rlp-common.obj"
	-@erase "$(INTDIR)\rlp-crc24.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\win32\winserial.obj"
	-@erase "$(OUTDIR)\gnokii.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "DEBUG" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D MODEL=\"6150\" /D PORT=\"COM2\" /D VERSION=\"Win32\" /Fp"$(INTDIR)\gnokii.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\gnokii.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\gnokii.pdb" /machine:I386 /out:"$(OUTDIR)\gnokii.exe" 
LINK32_OBJS= \
	"$(INTDIR)\cfgreader.obj" \
	"$(INTDIR)\fbus-6110-auth.obj" \
	"$(INTDIR)\fbus-6110-ringtones.obj" \
	"$(INTDIR)\fbus-6110.obj" \
	"$(INTDIR)\win32\getopt.obj" \
	"$(INTDIR)\gnokii.obj" \
	"$(INTDIR)\gsm-api.obj" \
	"$(INTDIR)\gsm-networks.obj" \
	"$(INTDIR)\rlp-common.obj" \
	"$(INTDIR)\rlp-crc24.obj" \
	"$(INTDIR)\win32\winserial.obj"

"$(OUTDIR)\gnokii.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "gnokii - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\gnokii.exe"


CLEAN :
	-@erase "$(INTDIR)\cfgreader.obj"
	-@erase "$(INTDIR)\fbus-6110-auth.obj"
	-@erase "$(INTDIR)\fbus-6110-ringtones.obj"
	-@erase "$(INTDIR)\fbus-6110.obj"
	-@erase "$(INTDIR)\win32\getopt.obj"
	-@erase "$(INTDIR)\gnokii.obj"
	-@erase "$(INTDIR)\gsm-api.obj"
	-@erase "$(INTDIR)\gsm-networks.obj"
	-@erase "$(INTDIR)\rlp-common.obj"
	-@erase "$(INTDIR)\rlp-crc24.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\win32\winserial.obj"
	-@erase "$(OUTDIR)\gnokii.exe"
	-@erase "$(OUTDIR)\gnokii.ilk"
	-@erase "$(OUTDIR)\gnokii.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D MODEL=\"6150\" /D PORT=\"COM2\" /D VERSION=\"Win32\" /Fp"$(INTDIR)\gnokii.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\gnokii.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\gnokii.pdb" /debug /machine:I386 /out:"$(OUTDIR)\gnokii.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\cfgreader.obj" \
	"$(INTDIR)\fbus-6110-auth.obj" \
	"$(INTDIR)\fbus-6110-ringtones.obj" \
	"$(INTDIR)\fbus-6110.obj" \
	"$(INTDIR)\win32\getopt.obj" \
	"$(INTDIR)\gnokii.obj" \
	"$(INTDIR)\gsm-api.obj" \
	"$(INTDIR)\gsm-networks.obj" \
	"$(INTDIR)\rlp-common.obj" \
	"$(INTDIR)\rlp-crc24.obj" \
	"$(INTDIR)\win32\winserial.obj"

"$(OUTDIR)\gnokii.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("gnokii.dep")
!INCLUDE "gnokii.dep"
!ELSE 
!MESSAGE Warning: cannot find "gnokii.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "gnokii - Win32 Release" || "$(CFG)" == "gnokii - Win32 Debug"
SOURCE=.\cfgreader.c

"$(INTDIR)\cfgreader.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=".\fbus-6110-auth.c"

"$(INTDIR)\fbus-6110-auth.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=".\fbus-6110-ringtones.c"

"$(INTDIR)\fbus-6110-ringtones.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=".\fbus-6110.c"

"$(INTDIR)\fbus-6110.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\win32\getopt.c

"$(INTDIR)\win32\getopt.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\gnokii.c

"$(INTDIR)\gnokii.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=".\gsm-api.c"

"$(INTDIR)\gsm-api.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=".\gsm-networks.c"

"$(INTDIR)\gsm-networks.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=".\rlp-common.c"

"$(INTDIR)\rlp-common.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=".\rlp-crc24.c"

"$(INTDIR)\rlp-crc24.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\win32\winserial.c

"$(INTDIR)\win32\winserial.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 
