!IF !DEFINED(VCTOOLSINSTALLDIR) || !DEFINED(WINDOWSSDKDIR) || !DEFINED(WINDOWSSDKVERSION)
!  ERROR Developer Command Prompt is not configured correctly.
!ENDIF

DDKINC=$(WINDOWSSDKDIR)Include\$(WINDOWSSDKVERSION)km
DDKLIB=$(WINDOWSSDKDIR)lib\$(WINDOWSSDKVERSION)km\x64

!IF !EXIST("$(DDKINC)\ntddk.h")
!  ERROR Kernel include files not found
!ENDIF

!IF "$(BUILD)" == "RELEASE" || "$(build)" == "release"
_BUILD=release
!ELSE
_BUILD=debug
!ENDIF

BINDIR=bin\$(_BUILD)^\

!IF !EXIST("$(BINDIR)")
!  IF [MKDIR $(BINDIR)] != 0
!    ERROR Could not create the $(BINDIR) directory.
!  ENDIF
!ENDIF

CPPFLAGS=/Zi /GS- /Fo$(BINDIR) /Fd$(BINDIR)
CPPFLAGS=$(CPPFLAGS) /permissive- /W3 /WX /Zc:inline /Zc:externConstexpr
LNKFLAGS=/debug /opt:ref,icf /nodefaultlib /nologo

DRVOPTS=/subsystem:native /driver /entry:DriverEntry "$(DDKLIB)\ntoskrnl.lib"
USROPTS=/entry:main kernel32.lib advapi32.lib user32.lib

!IF "$(_BUILD)" == "release"
CPPFLAGS=$(CPPFLAGS) /O2 /Gy /DNDEBUG
!ELSE
CPPFLAGS=$(CPPFLAGS) /Od /D_DEBUG
!ENDIF

all: $(BINDIR)setcr4.sys $(BINDIR)runsvc.exe $(BINDIR)testapp.exe

$(BINDIR)setcr4.sys: .\setcr4\driver.c .\setcr4\driver.h
  $(CPP) $(CPPFLAGS) /I"$(DDKINC)" .\setcr4\driver.c /link $(DRVOPTS) /OUT:$@

$(BINDIR)runsvc.exe: .\runsvc\runsvc.c .\setcr4\driver.h
  $(CPP) $(CPPFLAGS) .\runsvc\runsvc.c /link $(USROPTS) /OUT:$@

$(BINDIR)testapp.exe: .\testapp\testapp.c
  $(CPP) $(CPPFLAGS) .\testapp\testapp.c /link $(USROPTS) /OUT:$@

clean:
  -rmdir /s /q .\bin
