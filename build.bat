@echo off

if not defined VCToolsInstallDir goto :EnvErr
if not defined WindowsSdkDir goto :EnvErr
if not defined WindowsSDKVersion goto :EnvErr

SET BINDIR=%~dp0bin
if not exist %BINDIR% mkdir %BINDIR%
pushd %BINDIR%

SET "DDKINC=%WindowsSdkDir%Include\%WindowsSDKVersion%km"
SET "DDKLIB=%WindowsSdkDir%lib\%WindowsSDKVersion%km\x64"
if exist "%DDKINC%\ntddk.h" goto :Build
    echo Kernel include files not found at %DDKINC%
    goto :EnvErr

:Build
echo "Building the setcr4 driver"
SET CL_OPTS=/Od /Zi /I"%DDKINC%"
SET LINK_OPTS=/subsystem:native /driver /entry:DriverEntry /opt:icf /opt:ref /incremental:no /nodefaultlib /nologo
SET LINK_LIBS="%DDKLIB%\ntoskrnl.lib"
cl %CL_OPTS% ../setcr4/driver.c /link %LINK_OPTS% /out:setcr4.sys %LINK_LIBS%

echo "Building the runsvc driver controller"
:: /GS- isn't ideal, but makes linking a minimal size & dependency binary possible
:: This binary doesn't use any C-runtime functionality, just the Win32 APIs
SET CL_OPTS=/Od /Zi /GS-
SET LINK_OPTS=/opt:icf /opt:ref /incremental:no /nodefaultlib /entry:main /nologo
SET LINK_LIBS=kernel32.lib advapi32.lib user32.lib
cl %CL_OPTS% ../runsvc/runsvc.c /link %LINK_OPTS% %LINK_LIBS%

echo "Building the test app"
SET CL_OPTS=/Od /Zi /GS-
SET LINK_OPTS=/opt:icf /opt:ref /incremental:no /nodefaultlib /entry:main /nologo
SET LINK_LIBS=kernel32.lib advapi32.lib user32.lib
ml64.exe /c /Zi /nologo ../testapp/pmc.asm
cl %CL_OPTS% ../testapp/testapp.c /link %LINK_OPTS% %LINK_LIBS%

popd
exit /b 0

:EnvErr
echo "Environment is not set up correctly. Review the readme."
popd
exit /b 1
