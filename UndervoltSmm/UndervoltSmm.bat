@echo off
pushd .
cd %~dp0..
set buildDir=Build\IntelFrameworkModuleAll\RELEASE_VS2017\X64\UndervoltSmm\UndervoltSmm\OUTPUT

BaseTools\Bin\Win32\GenSec.exe -v -s EFI_SECTION_PE32^
    -o UndervoltSmm\UndervoltSmm.pe32^
    %buildDir%\UndervoltSmm.efi
BaseTools\Bin\Win32\GenSec.exe -v -s EFI_SECTION_USER_INTERFACE^
    -o UndervoltSmm\UndervoltSmm.ui -n "UndervoltSmm"
BaseTools\Bin\Win32\GenSec.exe -v -s EFI_SECTION_VERSION^
    -o UndervoltSmm\UndervoltSmm.ver -n "1.0"
BaseTools\Bin\Win32\GenSec.exe -v -s EFI_SECTION_SMM_DEPEX^
    -o UndervoltSmm\UndervoltSmm.depex^
    %buildDir%\UndervoltSmm.depex
BaseTools\Bin\Win32\GenFfs.exe -v -g "1DB43EC9-DF5F-4CF5-AAF0-0E85DB4E149A"^
    -o UndervoltSmm\UndervoltSmm.ffs^
    -i UndervoltSmm\UndervoltSmm.depex^
    -i UndervoltSmm\UndervoltSmm.pe32^
    -i UndervoltSmm\UndervoltSmm.ui^
    -i UndervoltSmm\UndervoltSmm.ver^
    -t EFI_FV_FILETYPE_SMM

popd