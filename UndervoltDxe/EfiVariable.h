#ifndef __EFI_VARIABLE_H__
#define __EFI_VARIABLE_H__

#include <Uefi/UefiBaseType.h>

EFI_GUID AMISetupVariableGuid = {0xEC87D643, 0xEBA4, 0x4BB5, {0xA1, 0xE5, 0x3F, 0x3E, 0x36, 0xB2, 0x0D, 0xA9}}; // AMI Setup
CHAR16 *AmiSetupVariableName = L"Setup";

VOID EFIAPI SetVariable(UINTN Offset, UINT8 Value);

#endif