# Microsoft Developer Studio Generated NMAKE File, Based on xgnokii.dsp
!IF "$(CFG)" == ""
CFG=xgnokii - Win32 Debug
!MESSAGE No configuration specified. Defaulting to xgnokii - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "xgnokii - Win32 Release" && "$(CFG)" != "xgnokii - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "xgnokii.mak" CFG="xgnokii - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "xgnokii - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "xgnokii - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "xgnokii - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\xgnokii.exe"


CLEAN :
	-@erase "$(INTDIR)\cfgreader.obj"
	-@erase "$(INTDIR)\fbus-6110-auth.obj"
	-@erase "$(INTDIR)\fbus-6110.obj"
	-@erase "$(INTDIR)\gsm-api.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\winserial.obj"
	-@erase "$(INTDIR)\xgnokii.obj"
	-@erase "$(INTDIR)\xgnokii_cfg.obj"
	-@erase "$(INTDIR)\xgnokii_common.obj"
	-@erase "$(INTDIR)\xgnokii_contacts.obj"
	-@erase "$(INTDIR)\xgnokii_sms.obj"
	-@erase "$(INTDIR)\xgnokii_netmon.obj"
	-@erase "$(OUTDIR)\xgnokii.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D MODEL=\"6150\" /D PORT=\"COM2\" /D VERSION=\"Win32\" /D XVERSION=\"GTK_Win\" /Fp"$(INTDIR)\xgnokii.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\xgnokii.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib gdk-1.3.lib gtk-1.3.lib glib-1.3.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\xgnokii.pdb" /machine:I386 /out:"$(OUTDIR)\xgnokii.exe" 
LINK32_OBJS= \
	"$(INTDIR)\cfgreader.obj" \
	"$(INTDIR)\fbus-6110-auth.obj" \
	"$(INTDIR)\fbus-6110.obj" \
	"$(INTDIR)\gsm-api.obj" \
	"$(INTDIR)\winserial.obj" \
	"$(INTDIR)\xgnokii.obj" \
	"$(INTDIR)\xgnokii_cfg.obj" \
	"$(INTDIR)\xgnokii_common.obj" \
	"$(INTDIR)\xgnokii_contacts.obj" \
	"$(INTDIR)\xgnokii_netmon.obj" \
	"$(INTDIR)\xgnokii_sms.obj"

"$(OUTDIR)\xgnokii.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "xgnokii - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\xgnokii.exe"


CLEAN :
	-@erase "$(INTDIR)\cfgreader.obj"
	-@erase "$(INTDIR)\fbus-6110-auth.obj"
	-@erase "$(INTDIR)\fbus-6110.obj"
	-@erase "$(INTDIR)\gsm-api.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\winserial.obj"
	-@erase "$(INTDIR)\xgnokii.obj"
	-@erase "$(INTDIR)\xgnokii_cfg.obj"
	-@erase "$(INTDIR)\xgnokii_common.obj"
	-@erase "$(INTDIR)\xgnokii_contacts.obj"
	-@erase "$(INTDIR)\xgnokii_sms.obj"
	-@erase "$(INTDIR)\xgnokii_netmon.obj"
	-@erase "$(OUTDIR)\xgnokii.exe"
	-@erase "$(OUTDIR)\xgnokii.ilk"
	-@erase "$(OUTDIR)\xgnokii.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D MODEL=\"6150\" /D PORT=\"COM2\" /D VERSION=\"Win32\" /D XVERSION=\"GTK_Win\" /Fp"$(INTDIR)\xgnokii.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

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

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\xgnokii.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib gdk-1.3.lib gtk-1.3.lib glib-1.3.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\xgnokii.pdb" /debug /machine:I386 /out:"$(OUTDIR)\xgnokii.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\cfgreader.obj" \
	"$(INTDIR)\fbus-6110-auth.obj" \
	"$(INTDIR)\fbus-6110.obj" \
	"$(INTDIR)\gsm-api.obj" \
	"$(INTDIR)\winserial.obj" \
	"$(INTDIR)\xgnokii.obj" \
	"$(INTDIR)\xgnokii_cfg.obj" \
	"$(INTDIR)\xgnokii_common.obj" \
	"$(INTDIR)\xgnokii_contacts.obj" \
	"$(INTDIR)\xgnokii_netmon.obj" \
	"$(INTDIR)\xgnokii_sms.obj"

"$(OUTDIR)\xgnokii.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("xgnokii.dep")
!INCLUDE "xgnokii.dep"
!ELSE 
!MESSAGE Warning: cannot find "xgnokii.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "xgnokii - Win32 Release" || "$(CFG)" == "xgnokii - Win32 Debug"
SOURCE=..\cfgreader.c

"$(INTDIR)\cfgreader.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE="..\fbus-6110-auth.c"

"$(INTDIR)\fbus-6110-auth.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE="..\fbus-6110.c"

"$(INTDIR)\fbus-6110.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE="..\gsm-api.c"

"$(INTDIR)\gsm-api.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\win32\winserial.c

"$(INTDIR)\winserial.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\xgnokii.c

"$(INTDIR)\xgnokii.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\xgnokii_cfg.c

"$(INTDIR)\xgnokii_cfg.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\xgnokii_common.c

"$(INTDIR)\xgnokii_common.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\xgnokii_contacts.c

"$(INTDIR)\xgnokii_contacts.obj" : $(SOURCE) "$(INTDIR)"

SOURCE=.\xgnokii_netmon.c

"$(INTDIR)\xgnokii_netmon.obj" : $(SOURCE) "$(INTDIR)"

SOURCE=.\xgnokii_sms.c

"$(INTDIR)\xgnokii_sms.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF
