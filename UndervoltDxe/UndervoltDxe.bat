@echo off
pushd .
cd %~dp0..

call edksetup.bat
call build clean
call build -m UndervoltDxe\UndervoltDxe.inf
call build -m UndervoltDxe\EfiVariableProxyDxe.inf

set buildDir=Build\UefiCpu\RELEASE_VS2017\X64\UndervoltDxe\UndervoltDxe\OUTPUT
del /f /q %buildDir%\*.ui %buildDir%\*.ver %buildDir%\*.pe32 %buildDir%\*.dep UndervoltDxe\UndervoltDxe.ffs
BaseTools\Bin\Win32\GenSec.exe -v -s EFI_SECTION_PE32^
    -o %buildDir%\UndervoltDxe.pe32^
    %buildDir%\UndervoltDxe.efi
BaseTools\Bin\Win32\GenSec.exe -v -s EFI_SECTION_DXE_DEPEX^
    -o %buildDir%\UndervoltDxe.dep^
    %buildDir%\UndervoltDxe.depex
BaseTools\Bin\Win32\GenSec.exe -v -s EFI_SECTION_USER_INTERFACE^
    -o %buildDir%\UndervoltDxe.ui -n "UndervoltDxe"
BaseTools\Bin\Win32\GenSec.exe -v -s EFI_SECTION_VERSION^
    -o %buildDir%\UndervoltDxe.ver -n "1.0"
BaseTools\Bin\Win32\GenFfs.exe -v -g "1DB43EC9-DF5F-4CF5-AAF0-0E85DB4E149A"^
    -o UndervoltDxe\UndervoltDxe.ffs^
    -i %buildDir%\UndervoltDxe.dep^
    -i %buildDir%\UndervoltDxe.pe32^
    -i %buildDir%\UndervoltDxe.ui^
    -i %buildDir%\UndervoltDxe.ver^
    -t EFI_FV_FILETYPE_DRIVER

set buildDir=Build\UefiCpu\RELEASE_VS2017\X64\UndervoltDxe\EfiVariableProxyDxe\OUTPUT
del /f /q %buildDir%\*.ui %buildDir%\*.ver %buildDir%\*.pe32 %buildDir%\*.dep EfiVariableProxyDxe\EfiVariableProxyDxe.ffs
BaseTools\Bin\Win32\GenSec.exe -v -s EFI_SECTION_PE32^
    -o %buildDir%\EfiVariableProxyDxe.pe32^
    %buildDir%\EfiVariableProxyDxe.efi
BaseTools\Bin\Win32\GenSec.exe -v -s EFI_SECTION_DXE_DEPEX^
    -o %buildDir%\EfiVariableProxyDxe.dep^
    %buildDir%\EfiVariableProxyDxe.depex
BaseTools\Bin\Win32\GenSec.exe -v -s EFI_SECTION_USER_INTERFACE^
    -o %buildDir%\EfiVariableProxyDxe.ui -n "EfiVariableProxyDxe"
BaseTools\Bin\Win32\GenSec.exe -v -s EFI_SECTION_VERSION^
    -o %buildDir%\EfiVariableProxyDxe.ver -n "1.0"
BaseTools\Bin\Win32\GenFfs.exe -v -g "DD4995E2-5A16-43E7-A73F-F1EDB66E6850"^
    -o UndervoltDxe\EfiVariableProxyDxe.ffs^
    -i %buildDir%\EfiVariableProxyDxe.dep^
    -i %buildDir%\EfiVariableProxyDxe.pe32^
    -i %buildDir%\EfiVariableProxyDxe.ui^
    -i %buildDir%\EfiVariableProxyDxe.ver^
    -t EFI_FV_FILETYPE_DRIVER

popd